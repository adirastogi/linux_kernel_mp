#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>
#include "mp1_given.h"

#define proc_dir_name "mp1"
#define proc_file_name "status"
#define DEBUG 1
#define MAX_BUFSIZE 1000
#define MY_LIMIT 10000

MODULE_LICENSE("GPL");
MODULE_AUTHOR("gp31");
MODULE_DESCRIPTION("CS-423 MP1");

char proc_write_buffer[MAX_BUFSIZE];

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_file;
static int numbytes_printed = 0;
static void* to_print=NULL;
static unsigned long procfs_buffer_size = 0;




int read_from_proc_file(char * buffer, char** buffer_location,off_t offset, int buffer_length, int* eof, void* data){

    printk(KERN_ALERT "reading the procfile");
    if (offset>0) return 0;
    else return sprintf(buffer,proc_write_buffer,procfs_buffer_size);
}

int write_to_proc_file(struct file* file, const char *buffer, unsigned long count , void * data ) 
{
    procfs_buffer_size = count;
    if (procfs_buffer_size > MAX_BUFSIZE ){
        procfs_buffer_size = MAX_BUFSIZE;
    }

    if (copy_from_user(proc_write_buffer, buffer, procfs_buffer_size)){
        return -EFAULT;
    }

    return procfs_buffer_size ;
}

/* put linked list implementation here. this method returns the 
 pointer (a void to next element in the linked list, right now its 
 just printing an integer */
void* get_next_node(void){
    static int val = 0;
    val++;
    if(val>=500)
        return NULL;
    else return &val;
}

/* ---- sequence operations on the proc file */
void * seq_start_op(struct seq_file * sf, loff_t * pos){
    printk(KERN_ALERT "Inside start\n");
    if(*pos==0){
        printk(KERN_ALERT "Starting the sequence\n");
        to_print = get_next_node();
        (*pos)++;
        printk(KERN_ALERT "Dequeued element %d\n",*(int*)pos);
    }
    else printk(KERN_ALERT "Starting new page\n");
    if (to_print) return to_print;
    else return NULL;
}

void * seq_next_op(struct seq_file* sf, void * v, loff_t * pos){
    /* find out how many chars you printed,since this is always called after show */
    /* if more than kernel page size,call stop which will reinitialize*/
    printk(KERN_ALERT "Inside next\n");
    to_print = get_next_node();
    (*pos)++;
    printk(KERN_ALERT "Dequeued element %d\n",*(int*)pos);
    if(to_print) return to_print;
    else return NULL;
}

void seq_stop_op(struct seq_file* sf, void *v){
    printk(KERN_ALERT "Inside stop\n");
    if(v){
        printk(KERN_ALERT "Kernel page overflow,calling start again\n");
    }else {
        printk(KERN_ALERT "Stopping for good\n");
    }
}

/* This will take in a void poitner to a link list element 
   and print the element, right now does it with an int */
int seq_show_op(struct seq_file* sf, void * v){
    printk(KERN_ALERT "Inside show\n");
    numbytes_printed += sprintf(proc_write_buffer,"The value is %d\n",*(int*)v);
    seq_printf(sf,proc_write_buffer);
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
    return seq_open(file,&seq_ops);
}

/* Define the file ops for the proc filesystem entry*/
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = open_proc_file,
    .read = seq_read,
    .write = NULL,
    .llseek = seq_lseek,
    .release = seq_release
};

/* create the proc file */
int create_proc_file(void){

    proc_file = proc_create(proc_file_name,644,proc_dir,&fops);
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
    printk(KERN_INFO "Removed the proc file system entries");
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



    printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
