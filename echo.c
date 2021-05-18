/*                                                     
 * $Id: hello.c,v 1.5 2004/10/26 03:32:21 corbet Exp $ 
 */
//#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h>	  /* kmalloc() */
#include <linux/mm.h>
#include <linux/fs.h>	 /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/aio.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/cdev.h>
#include <linux/ioport.h>
#include <linux/sched.h>

#define PORT_COM1 0x3f8
typedef short word;

//REGS

#define REG_RHR 0
#define REG_THR 0
#define REG_IER 1
#define REG_LCR 3
#define REG_LSR 5

//LCR
#define PARITY_NONE 0
#define PARITY_ODD 8
#define PARITY_EVEN 24

#define STOP_ONE 0
#define STOP_TWO 4

#define BITS_5 0
#define BITS_6 1
#define BITS_7 2
#define BITS_8 3

#define DLR_ON 128

//LSR
#define UART_LSR_TEMT 0x40 /* Transmitter empty */
#define UART_LSR_THRE 0x20 /* Transmit-hold-register empty */
#define UART_LSR_BI 0x10   /* Break interrupt indicator */
#define UART_LSR_FE 0x08   /* Frame error indicator, (stop bit) */
#define UART_LSR_PE 0x04   /* Parity error indicator */
#define UART_LSR_OE 0x02   /* Overrun error indicator */
#define UART_LSR_DR 0x01   /* Receiver data ready */

MODULE_LICENSE("Dual BSD/GPL");

dev_t echo_number;
struct cdev *echo_cdev;
struct file_operations echo_fops;
struct file *echo_file;
struct resource *echo_res;
int port_busy = 0;
//struct inode *echo_inode;

int setup_serial(int COM_port, int baud, unsigned char misc)
{
	word divisor;

	if (port_busy)
		return (port_busy);

	port_busy = COM_port;

	write_uart(COM_port, REG_IER, 0);
	write_uart(COM_port, REG_LCR, (int)DLR_ON);
	divisor = 0x1c200 / baud;
	//outpw(COM_port, divisor);	//original
	outw(divisor, COM_port); //como diz no guiao

	write_uart(COM_port, REG_LCR, (int)misc);
	return 1;
}

void write_uart(int COM_port, int reg, int data)
{

	//outp((COM_port + reg), data);		//original
	outb(data, (COM_port + reg)); //como diz no guiao
}

int read_uart(int COM_port, int reg)
{

	return (_inp(COM_port + reg));
}

void serial_write(unsigned char ch)
{
	while (1)
	{
		if (!((unsigned char)Read_UART(port_busy, REG_LSR) & 0x20))
		{
			schedule();
		}
		else break;
	}

	write_uart(port_busy, REG_THR, (int)ch);
}

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

	echo_res = request_region(0x3f8, 8, "echo");
	Setup_Serial(PORT_COM1, 96, BITS_8 | PARITY_EVEN | STOP_TWO);

	return 0;
}

/* The first version of the serp device driver is very simple. In its initialization function it should:

    initialize the UART with the following communication parameters: 8-bit chars, 2 stop bits, parity even, and 1200 bps. Because, we will not use interrupts, make sure that the UART is configured not to generate interrupts
    send one character via the UART
*/

static void hello_exit(void)
{
	unsigned int a = MAJOR(echo_number);

	unregister_chrdev_region(echo_number, 0);

	printk(KERN_ALERT "Goodbye, cruel world\n");
	printk(KERN_ALERT "major: %d\n", a);

	cdev_del(echo_cdev);

	release_region(0x3f8, 8);
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

// read will return the number of characters written by the DD on the device since it was last loaded
ssize_t read(struct file *filep, char __user *buff, size_t count, loff_t *offp)
{
	copy_to_user(buff, filep, (int)count);
	printk(KERN_ALERT "%s\n", (char *)buff);
	return count;
}

//a write to an echo device will make it print whatever an application writes to it on the console
ssize_t write(struct file *filep, const char __user *buff, size_t count, loff_t *offp)
{
	char *temp = kmalloc(count + 1, GFP_KERNEL);
	copy_from_user(temp, buff, (unsigned long)count);
	temp[count + 1] = '0';
	copy_to_user(buff, temp, (unsigned long)count + 1);
	printk(KERN_ALERT "%s\n", (char *)buff);
	kfree(temp);
}

module_init(hello_init);
module_exit(hello_exit);
