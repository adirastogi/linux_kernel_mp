#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include "mp1_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("gp31");
MODULE_DESCRIPTION("CS-423 MP1");

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_file;
static const struct file_operations fops = {
.owner = THIS_MODULE,
.open = NULL,
.read = NULL 
};

#define proc_dir_name "mp1"
#define proc_file_name "status"
#define DEBUG 1

int  create_new_proc_dir(void){

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
int create_proc_file(void){
	
    proc_file = proc_create(proc_file_name,644,proc_dir,&fops);
    if(!proc_file){
      remove_proc_entry(proc_file_name,proc_dir);
      printk(KERN_ALERT "Failure to create proc file");
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
