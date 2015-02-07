#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include "mp1_given.h"

#define proc_dir_name "mp1"
#define proc_file_name "status"
#define DEBUG 1
#define MAX_BUFSIZE 1000

MODULE_LICENSE("GPL");
MODULE_AUTHOR("gp31");
MODULE_DESCRIPTION("CS-423 MP1");

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_file;
static unsigned long procfs_buffer_size = 0;
static const struct file_operations fops = {
.owner = THIS_MODULE,
.open = NULL,
.read = NULL 
};

char proc_write_buffer[MAX_BUFSIZE];

int create_new_proc_dir(void){

    proc_dir = proc_mkdir(proc_dir_name,NULL);
    if(!proc_dir){
      remove_proc_entry(proc_dir_name,NULL);
      printk(KERN_ALERT "Failure to create proc directory");
      return 1;
    }else {
      printk(KERN_ALERT "proc directory created");
 	return 0; 
    }
}

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

int create_proc_file(void){
	
    proc_file = proc_create(proc_file_name,644,proc_dir,&fops);
    if(!proc_file){
      remove_proc_entry(proc_file_name,proc_dir);
      printk(KERN_ALERT "Failure to create proc file");
      proc_file->read_proc=read_from_proc_file;
      proc_file->write_proc=write_to_proc_file;
      return 1;
    }else {
      printk(KERN_ALERT "proc file created");
 	return 0; 
    }
}

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
   // Insert your code here ...
   int status = create_new_proc_dir();   
   if (status==1)
	return -ENOMEM;

    status = create_proc_file();
    if (status==1)
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
