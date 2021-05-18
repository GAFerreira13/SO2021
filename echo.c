/*                                                     
 * $Id: hello.c,v 1.5 2004/10/26 03:32:21 corbet Exp $ 
 */                                                    
//#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/mm.h>
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/aio.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>

MODULE_LICENSE("Dual BSD/GPL");

dev_t echo_number;
struct cdev *echo_cdev;
struct file_operations echo_fops;
struct file *echo_file;
//struct inode *echo_inode;



static int hello_init(void)
{
	int a, b = 0;


	printk(KERN_ALERT "Hello, world\n");

	a = alloc_chrdev_region(&echo_number, 0, 1, "echo");
	if (a != 0)
	{
		printk(KERN_ALERT "Erro a criar major device number");
	}

	printk(KERN_ALERT "major: %d\n", MAJOR(echo_number));

	echo_cdev = cdev_alloc();
	echo_cdev->ops = &echo_fops;
	echo_cdev->owner = THIS_MODULE;

	echo_fops.llseek = &no_llseek;
	echo_fops.read = &read;
	echo_fops.write = &write;
	echo_fops.open = &open;
	echo_fops.release = &release;

	b = cdev_add(echo_cdev, echo_number, 1);

	return 0;
}

static void hello_exit(void)
{
	unsigned int a = MAJOR(echo_number);
	
	unregister_chrdev_region(echo_number, 0);

	printk(KERN_ALERT "Goodbye, cruel world\n");
	printk(KERN_ALERT "major: %d\n", a);

	cdev_del(echo_cdev);
}

int open(struct inode *inodep, struct file *filep)
{
	int a = 0;
	filep->private_data = inodep->i_cdev;
	printk(KERN_ALERT "int open\n");
	
	a = nonseekable_open(inodep, filep);

	return 0;
}

int release(struct inode *inodep, struct file *filep)
{
	printk(KERN_ALERT "int release\n");
	return 0;
}

/*
int flush(struct inode *inodep, fl_owner_t id)
{

}
*/

/*
*Both read and write should return the number of bytes transferred, if the operation is successful. Otherwise, if no byte is successfully transferred, then they should return a negative number. However, if there is an error after successfully transferring some bytes, both should return the number of bytes transferred, and an error code in the following call of the function. This requires the DD to recall the occurrence of an error from a call to the next.
*/

// a read will return the number of characters written by the DD on the device since it was last loaded
ssize_t read(struct file *filep, char __user *buff, size_t count, loff_t *offp)
{
	copy_to_user(buff, filep, (int)count);
	printk(KERN_ALERT "%s\n", (char*) buff);
	return count;

}

//a write to an echo device will make it print whatever an application writes to it on the console
ssize_t write(struct file *filep, const char __user *buff, size_t count, loff_t *offp)
{
	char *temp = kmalloc(count + 1, GFP_KERNEL);
	copy_from_user(temp, buff, (unsigned long)count);
	temp[count+1] = '0';
	copy_to_user(buff, temp, (unsigned long)count+1);	
	printk(KERN_ALERT "%s\n", (char*) buff);
	kfree(temp);


}


module_init(hello_init);
module_exit(hello_exit);
