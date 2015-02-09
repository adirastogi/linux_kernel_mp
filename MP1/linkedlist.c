#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
 
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Learning Linked Lists");
MODULE_AUTHOR("Dhruve Ashar");
 
struct ProcessInfo {
    struct list_head list; /* kernel's list structure */
    unsigned int pid;
    unsigned char cpuTime;
};
 
struct ProcessInfo processInfoList;
 
int init_module() {
    struct ProcessInfo *aNewProcessInfo, *aProcessInfo;
    unsigned int i;
 
    printk(KERN_INFO "initialize kernel module\n");
    INIT_LIST_HEAD(&processInfoList.list);    //or LIST_HEAD(mylist); 
 
    /* adding elements to mylist */
    for(i=0; i<5; ++i){
        aNewProcessInfo = kmalloc(sizeof(*aNewProcessInfo), GFP_KERNEL);
        aNewProcessInfo->pid = i;
        aNewProcessInfo->cpuTime = i * 10;
        INIT_LIST_HEAD(&aNewProcessInfo->list);
        /* add the new node to mylist */
        list_add_tail(&(aNewProcessInfo->list), &(processInfoList.list));
    }
    printk(KERN_INFO "traversing the list using list_for_each_entry()\n");
    list_for_each_entry(aProcessInfo, &processInfoList.list, list) {
        //access the member from aProcessInfo
        printk(KERN_INFO "ProcessInfo => PID: %d; CPU Time: %d\n", aProcessInfo->pid, aProcessInfo->cpuTime);
    }
    printk(KERN_INFO "\n");
    struct list_head *ptr;
    ptr = processInfoList.list.next;
    
    struct ProcessInfo *temp;
    temp = list_entry(ptr, struct ProcessInfo, list);
    printk(KERN_INFO "ProcessInfo => PID: %d; CPU Time: %d\n", temp->pid, temp->cpuTime);

    return 0;
}
 
void cleanup_module() {
    struct ProcessInfo *aProcessInfo, *tmp;
    printk(KERN_INFO "kernel module unloaded.\n");
    printk(KERN_INFO "deleting the list using list_for_each_entry_safe()\n");
    list_for_each_entry_safe(aProcessInfo, tmp, &processInfoList.list, list){
         printk(KERN_INFO "freeing node %d\n", aProcessInfo->pid);
         list_del(&aProcessInfo->list);
         kfree(aProcessInfo);
    }
}
