#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/timer.h>
#include <linux/seq_file.h>
#include <linux/workqueue.h>
#include "mp1_given.h"
#include <linux/mutex.h>

//---------header files for LinkedList
#include <linux/list.h>
#include <linux/slab.h>
//---------header files for LinkedList

#define proc_dir_name "mp1"
#define proc_file_name "status"
#define DEBUG 1
#define MAX_BUFSIZE 1000
#define TICK 1000

static DEFINE_MUTEX(list_lock);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("gp31");
MODULE_DESCRIPTION("CS-423 MP1");

char proc_write_buffer[MAX_BUFSIZE];

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_file;
static int numbytes_printed = 0;
static void* to_print_after_page=NULL;
static int finish_printing = 0;
static struct timer_list proc_timer;
static struct workqueue_struct * wque = NULL;


struct process_info {
    struct list_head list; //This is kernel's structure for 2-way list
    unsigned int pid;
    unsigned long cpu_time;
};

struct proc_work_t  {
    struct work_struct work_st;
};

struct process_info process_info_list; //Head of the list which stores info about all processes

void add_node_to_list(unsigned int process_id, unsigned int user_cpu_time){
    struct process_info *new_process_info;
    new_process_info = kmalloc(sizeof(*new_process_info), GFP_KERNEL);
    new_process_info->pid = process_id;
    new_process_info->cpu_time = user_cpu_time;
    INIT_LIST_HEAD(&new_process_info->list); //This step is important
    mutex_lock(&list_lock);
    list_add_tail(&(new_process_info->list), &(process_info_list.list));
    mutex_unlock(&list_lock);
}


/* convert char buffer to int pid */
unsigned int char_to_int(const char* buffer){
    char * p = buffer;
    unsigned int val = 0;
    while('0'<= *p && *p <='9' ) 
        val = 10*val + ( *p++ -'0');
    return val;
}
/* This function takes the pid written to the procfile by a user process
   (contained in the buffer buffer of size bufsize) and gets the stats for
   that process from timer and writes it to linked list */    
void write_process_id_to_list(const char * buffer, size_t bufsize){
    unsigned int pid= char_to_int(buffer);
    add_node_to_list(pid,0);
}

/* ---- sequence operations on the proc file */
void * seq_start_op(struct seq_file * sf, loff_t * pos){
    printk(KERN_ALERT "Inside start\n");
    if(*pos==0){    
        /* First call */
        ++(*pos);
        printk(KERN_ALERT "Beginning list\n");
        if(!list_empty(&process_info_list.list))
            return process_info_list.list.next; 
        else return NULL;
    }else if(*(pos)>0 && !finish_printing){
        /* New page of output */
        printk(KERN_ALERT "Restarting new page\n");
        return to_print_after_page; 
    }else {
        printk(KERN_ALERT "Exiting from start\n");
        to_print_after_page = NULL;
        return NULL;
    }
    
}

void * seq_next_op(struct seq_file* sf, void * v, loff_t * pos){
    /* find out how many chars you printed,since this is always called after show */
    /* if more than kernel page size,call stop which will reinitialize*/
    printk(KERN_ALERT "Inside next\n");
    struct list_head *ptr = (struct list_head *) v;
    struct process_info *temp = list_entry(ptr->next, struct process_info, list);
    if (temp == &process_info_list){
        printk(KERN_ALERT "Going here\n");
        return NULL;
    }else {
        (*pos)++;
        return &temp->list;
    }
}

void seq_stop_op(struct seq_file* sf, void *v){
    printk(KERN_ALERT "Inside stop\n");
    if(v){
        //called on page overflow.
        printk(KERN_ALERT "New page output \n");
        to_print_after_page = v;
    }else{
        if (!finish_printing){
            //called first time
            printk(KERN_ALERT "Stopping for good\n");
            finish_printing = 1;
        }else{
            //called second time from start
            printk(KERN_ALERT "Final stop call from start\n");
            finish_printing = 0;
        }
    }
}

/* This will take in a void poitner to a link list element 
   and print the element, right now does it with an int */
int seq_show_op(struct seq_file* sf, void * v){
    printk(KERN_ALERT "Inside show\n");
    
    struct list_head *ptr = (struct list_head *) v;
    struct process_info *temp = list_entry(ptr, struct process_info, list);
    numbytes_printed += sprintf(proc_write_buffer, "PID := %d, CPU Time:= %d\n", temp->pid, temp->cpu_time);
    seq_printf(sf, proc_write_buffer);
    return 0;
}
/*---seq operations on the proc file---*/


/* struct defining the sequence operations */
static const struct seq_operations seq_ops = {
    .start = seq_start_op,
    .next = seq_next_op,
    .stop = seq_stop_op,
    .show = seq_show_op
};


/* called when the proc file is opened */
int open_proc_file(struct inode* inode, struct file* file){
    mutex_lock(&list_lock);
    int ret_code = seq_open(file,&seq_ops);
    mutex_unlock(&list_lock);
    return ret_code;
}

/* called when the user process writes to proc file */
ssize_t user_write_proc_file(struct file *sf, const char  * buffer, size_t bytes, loff_t * pos){
    if (bytes > MAX_BUFSIZE)
        bytes = MAX_BUFSIZE-1;
    /* copy from userspace buffer to local buffer*/
    if(copy_from_user(proc_write_buffer,buffer,bytes))
        return -EFAULT;
    /* return the number of bytes writtern */
    proc_write_buffer[bytes]='\0';
    write_process_id_to_list(proc_write_buffer,bytes);
    printk(KERN_ALERT "Wrote the value %s to the buffer, buffer size %d\n",proc_write_buffer,bytes);
    return bytes;
}

/* Define the file ops for the proc filesystem entry*/
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = open_proc_file,
    .read = seq_read,
    .write = user_write_proc_file,
    .llseek = seq_lseek,
    .release = seq_release
};

/* create the proc file */
int create_proc_file(void){

    proc_file = proc_create(proc_file_name,0666,proc_dir,&fops);
    if(!proc_file){
        remove_proc_entry(proc_file_name,proc_dir);
        printk(KERN_ALERT "Failure to create proc file\n");
        return 0;
    }else{
        printk(KERN_ALERT "Proc file created\n");
        return 1; 
    }
}

/* create the proc directory */
int create_new_proc_dir(void){

    proc_dir = proc_mkdir(proc_dir_name,NULL);
    if(!proc_dir){
        remove_proc_entry(proc_dir_name,NULL);
        printk(KERN_ALERT "Failure to create proc directory\n");
        return 0;
    }else {
        printk(KERN_ALERT "Proc directory created\n");
        return 1; 
    }
}

/* remove the proc file */
void remove_proc_files(void){
    remove_proc_entry(proc_file_name,proc_dir);
    remove_proc_entry(proc_dir_name,NULL);
    printk(KERN_ALERT "Removed the proc file system entries");
}

void remove_linked_list(void){
    struct process_info *process_node, *temp;
    list_for_each_entry_safe(process_node, temp, &process_info_list.list, list){
        list_del(&process_node->list);
        kfree(process_node);
    }
}
//iterate the list and update the timers
void iterate_and_update_list(void){
    struct process_info *process_node, *temp;
    printk(KERN_ALERT "Iterating and updating the list\n");
    
    mutex_lock(&list_lock);
    list_for_each_entry_safe(process_node, temp, &process_info_list.list, list){
        printk(KERN_ALERT "Updating the entry for process id %d\n", process_node->pid);
        unsigned long cpu_use;
        if(get_cpu_use(process_node->pid,&cpu_use)==-1){
            //delete the entry from the list if pid is stale
            list_del(&process_node->list);
            kfree(process_node);
        }else{
            printk(KERN_ALERT "CPU use time : => %d\n", cpu_use);
            process_node->cpu_time=cpu_use;
        }
    }
    mutex_unlock(&list_lock);
}
//this is called when work is dequeued from the 
//workqueue (bottom half)
void workque_callback(struct work_struct * work){
    iterate_and_update_list();        
    kfree((void*)work);
    return ;
}
    
//function that is called as a callback when the timer expires
//this is the top half of f execution 
void timer_callback(unsigned long data){
    int ret; 
    //schedule the workqueue function
    printk(KERN_ALERT "Inside timer callback\n");
    struct proc_work_t* work = (struct proc_work_t*) kmalloc(sizeof(struct proc_work_t),GFP_KERNEL);
    if(work){
        INIT_WORK((struct work_struct*)work,workque_callback);
        if(wque) ret = queue_work(wque,(struct work_struct* )work);
        printk(KERN_ALERT "Scheduling new work\n");
    }
    //scheduled the new timer
    ret = mod_timer(&proc_timer,(jiffies+ msecs_to_jiffies(TICK)));
    if (ret) printk(KERN_ALERT "Error setting the timer\n");
}
//initilaize the timer
void create_timer_and_queue(void){

    int ret;
    //initilaize the workqueue 
    wque = create_workqueue("mp1_queue");  
    if(!wque) printk(KERN_ALERT "Error initilazing workqueue\n");
    //initilaize the timer and set it to fire first time
    setup_timer(&proc_timer,timer_callback,0);
    ret = mod_timer(&proc_timer,(jiffies+ msecs_to_jiffies(TICK)));
    if (ret) printk(KERN_ALERT "Error setting the timer\n");

}

void destroy_timer_and_queue(void){
    del_timer(&proc_timer);
    flush_workqueue(wque);
    destroy_workqueue(wque);
}
// mp1_init - Called when module is loaded
int __init mp1_init(void)
{
#ifdef DEBUG
    printk(KERN_ALERT "MP1 MODULE LOADING\n");
#endif
    if(!create_new_proc_dir() || !create_proc_file())
        return -ENOMEM;

    printk(KERN_ALERT "MP1 MODULE LOADED\n");
    
    INIT_LIST_HEAD(&process_info_list.list);
    /*
    unsigned int i=0;
    for(i=1; i<600; ++i){
        add_node_to_list(i, i*10);
    }
    */

    //init a timer during initialize
    create_timer_and_queue();
    return 0;   
}


// mp1_exit - Called when module is unloaded
void __exit mp1_exit(void)
{
#ifdef DEBUG
    printk(KERN_ALERT "MP1 MODULE UNLOADING\n");
#endif
    // Insert your code here ...
    remove_proc_files();
    remove_linked_list();
    destroy_timer_and_queue();

    printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
