/*
 *  ioctl test module -- Rich West.
 */

#include <linux/module.h> /*so every kernel needs this*/
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h> /* This returns an error code, one that I could loop up in the source code */
#include <linux/proc_fs.h> /*contains the proc filesystem constants/structures */
#include <asm/uaccess.h> /*user space memory access functions */
#include <linux/tty.h>  /* used for handling nested locking with pty pairs */
#include <linux/sched.h> /*used for scheduling */
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");


#define MY_WORK_QUEUE_NAME "primer_sched.c"

struct ioctl_test_t {
  int field1;
  char field2;
};

#define IOCTL_TEST _IOW(0, 6, struct ioctl_test_t)  //this is the ioctl number that encodes the major device number, type of ioctl, command, and type of parameter

void my_printk(char *string);
static int pseudo_device_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static int pseudo_device_read(struct file *file, char __user *buf, size_t size, loff_t *off);
char my_getchar ( void );
irqreturn_t irq_handler(int irq, void *dev_id);

static struct file_operations pseudo_dev_proc_operations;
static struct proc_dir_entry *proc_entry;

static unsigned short kbd_buffer = 0x0000; /* HByte=Status, LByte=Scancode */
static wait_queue_head_t kbd_irq_waitq;   //used for keyboard waitq





// second parameter might need to be to the device structure
irqreturn_t irq_handler(int irq, void *dev_id)
{
  static unsigned char scancode, status;
  status   = inb(0x64);
  scancode = inb(0x60);
  kbd_buffer = (unsigned short) ((status << 8) | (scancode & 0x00ff));
  wake_up_interruptible(&kbd_irq_waitq);

  return IRQ_HANDLED;

}



/*2nd arg: a pointer to a function that takes in (int, void *)
request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
	    const char *name, void *dev)


typedef irqreturn_t (*irq_handler_t)(int, void *);
*/
static int __init initialization_routine(void) {
  //goes to dmesg
  printk("<1> Loading module\n");

  pseudo_dev_proc_operations.ioctl = pseudo_device_ioctl;
  pseudo_dev_proc_operations.read = pseudo_device_read;

  /* Start create proc entry */
  proc_entry = create_proc_entry("ioctl_test", 0444, NULL);
  if(!proc_entry)
  {
    printk("<1> Error creating /proc entry.\n");
    return 1;
  }
  //proc_entry->owner = THIS_MODULE; <-- This is now deprecated
  proc_entry->proc_fops = &pseudo_dev_proc_operations;
  
  init_waitqueue_head(&kbd_irq_waitq);
  return request_irq(1, irq_handler, IRQF_SHARED, "test_keyboard_irq_handler", (void *) irq_handler);
}


/* 'printk' version that prints to active tty. */
void my_printk(char *string)
{
  struct tty_struct *my_tty;

  my_tty = current->signal->tty;

  if (my_tty != NULL) {
    (*my_tty->driver->ops->write)(my_tty, string, strlen(string));
    (*my_tty->driver->ops->write)(my_tty, "\015\012", 2);
  }
} 


static void __exit cleanup_routine(void) {

  printk("<1> Dumping module\n");
  remove_proc_entry("ioctl_test", NULL);
  free_irq(1,(void*)(irq_handler));
  return;
}





/***
 * ioctl() entry point...
 */
static int pseudo_device_ioctl(struct inode *inode, struct file *file,
				unsigned int cmd, unsigned long arg)
{
  struct ioctl_test_t ioc;
  
  switch (cmd){

  case IOCTL_TEST:
    copy_from_user(&ioc, (struct ioctl_test_t *)arg, 
		   sizeof(struct ioctl_test_t));
    printk("<1> ioctl: call to IOCTL_TEST (%d,%c)!\n", 
	   ioc.field1, ioc.field2);
    break;
  
  default:
    return -EINVAL;
    break;
  }
  
  return 0;
}



/***
 * read() entry point...
*/
static int pseudo_device_read(struct file *file, char *buf, size_t size, loff_t *off)
{
  
  interruptible_sleep_on(&kbd_irq_waitq);
  printk(KERN_INFO "This is the scancode that's about to be sent: %x\n", kbd_buffer);
  //printk(KERN_INFO "Tis is size_t %u, this is loff_t %p", size, off );


  copy_to_user((void*) buf, (void*) &kbd_buffer, sizeof(kbd_buffer));
	return(sizeof(kbd_buffer));
 
}


static inline unsigned char inb_test( unsigned short usPort ) {

    unsigned char uch;
    
    asm volatile( "inb %1,%0" : "=a" (uch) : "Nd" (usPort) );
    return uch;
}







module_init(initialization_routine); 
module_exit(cleanup_routine); 