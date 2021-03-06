//We need this header in all kernel modules
#include <linux/module.h>
//Absolutely because we are doing kernel job
#include <linux/kernel.h>
//And this is needed for the macros
#include <linux/init.h>
//For create and register a procfs entry
#include <linux/proc_fs.h>
//For providing read function of the entry with ease
#include <linux/seq_file.h>
//For finding the parent process ID of the module
#include <asm/current.h>
//For using task_struct
#include <linux/sched.h>
//For using diffrent time related functions
#include <linux/time.h>
//For obtaining CPU status
#include <linux/kernel_stat.h>
//For counting jiffies and HZ
#include <linux/jiffies.h>
//For using cputime_to_timespec function
#include <linux/sched/cputime.h>
//For ktime_get_ts function to obtain uptime
#include <linux/timekeeping.h>


//It is always good to have a meaningful constant as a return code
#define SUCCESS 0
//This will be our module name
#define DEVICE_NAME "uptimeprocfs"


//These are some useful information that could reveald with modinfo command
//Set module license to get rid of tainted kernel warnings
MODULE_LICENSE("GPL");
//Introduce the module's developer, it's functionality and version
MODULE_AUTHOR("Aliireeza Teymoorian <teymoorian@gmail.com>");
MODULE_DESCRIPTION("This is just a simple Hello World using /proc filesystem device driver, which could calculate both uptime and idle time in the same format as /proc/uptime");
MODULE_VERSION("1.0.2");


//Creating a proc directory entry structure
static struct proc_dir_entry* our_proc_file;


//This function calls on demand of read request from seq_files
static int proc_show(struct seq_file *m, void *v){
	printk(KERN_INFO "UPTIMEPROCFS: Generating output for user space with seq_files.\n");
	//We are going to count processes which are currently running on the context of the Kernel
	static struct timespec calc_uptime;
	static struct timespec calc_idle;
	int i;
	ktime_t calc_idletime = 0;

	ktime_get_ts(&calc_uptime);

	for_each_possible_cpu(i)
		calc_idletime += kcpustat_cpu(i).cpustat[CPUTIME_IDLE];
	calc_idle = ktime_to_timespec(calc_idletime);
	

	seq_printf(m, "%lu.%02lu %lu.%02lu\n",
			(unsigned long) calc_uptime.tv_sec,
			(calc_uptime.tv_nsec / (NSEC_PER_SEC / 100)),
			(unsigned long) calc_idle.tv_sec,
			(calc_idle.tv_nsec / (NSEC_PER_SEC / 100)));
	return SUCCESS;
}


//This is where system functionallity triggers every time some process try to read from our proc entry
static int proc_open(struct inode *inode, struct file *file){
	printk(KERN_INFO "UPTIMEPROCFS: Open Function, Process \"%s:%i\"\n", current->comm, current->pid);
	printk(KERN_INFO "UPTIMEPROCFS: Open Function, USER \"UID:%i\"\n", get_current_user()->uid.val);
	return single_open(file, proc_show, NULL);
}



//Struct file_operations is the key to the functionality of the module
//functions that defined here are going to add to the kernel functionallity
//in order to respond to userspace access demand to the corresponding /proc entry
static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};


//Your module's entry point
static int __init uptime_procfs_init(void){
	//These mesages will not show to the user but instead they will apear in kernel log
	//You can observe the kernel log in /dev/kmsg or with using dmsg command
	printk(KERN_INFO "UPTIMEPROCFS: Initialization.\n");
	printk(KERN_INFO "UPTIMEPROCFS: Init Module, Process \"%s:%i\"\n", current->comm, current->pid);

	our_proc_file = proc_create(DEVICE_NAME, 0644 , NULL, &fops);
	//Put an error message in kernel log if cannot create proc entry
	if(!our_proc_file){
		printk(KERN_ALERT "UPTIMEPROCFS: Registration Failure.\n");
		//Because of this fact that procfs is a ram filesystem, this error means the lack of enough memory
		return -ENOMEM;
	}

	printk(KERN_INFO "UPTIMEPROCFS: /proc/%s has been created.\n", DEVICE_NAME);
	//The init_module should return a value to the rest of kernel that asure
	//them to its successfully registration of its functionality
	return SUCCESS;
}


//You sould clean up the mess before exiting the module
static void __exit uptime_procfs_exit(void){
	//Initiating module unloading procedure
	printk(KERN_INFO "UPTIMEPROCFS: Cleanup Module, Process \"%s:%i\"\n", current->comm, current->pid);

	//Remove proc filesystem entry from system
	remove_proc_entry(DEVICE_NAME, NULL);
	printk(KERN_INFO "UPTIMEPROCFS: /proc/%s has been removed.\n", DEVICE_NAME);

	printk(KERN_INFO "UPTIMEPROCFS: GoodBye.\n");
	//The cleanup_module function doesn't need to return any value to the rest of the kernel
}

//Now we need to define init-module and cleanup_module aliases
module_init(uptime_procfs_init);
module_exit(uptime_procfs_exit);
