#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/sched.h>

#include "ser_cdev.h"

#define MAX_DEV_NUM 4

static int fop_open(struct inode *inode, struct file *filp);
static int fop_release(struct inode *inode, struct file *filp);
static ssize_t fop_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t fop_write(struct file *filp, const char __user *buf, size_t count, loff_t *fpos);
static unsigned int fop_poll(struct file *filp, struct poll_table_struct *wait);

unsigned int device_major;

char *str_dev_prefix="ser";

static unsigned int next_id=0;

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = fop_open,
    .release = fop_release,
    .read = fop_read,
    .write = fop_write,
    .poll = fop_poll,
};

int ser_setup_cdev(void) {
	dev_t dev;
	
    if(alloc_chrdev_region(&dev,0,MAX_DEV_NUM,str_dev_prefix)){
        printk(KERN_ERR "can't allocate device number\n");
        device_major = 0;
        return -ENODEV;
    }

    device_major = MAJOR(dev);
    printk(KERN_DEBUG "register cdev \"%s\" %d:[0-%d]\n"
        ,str_dev_prefix,device_major,MAX_DEV_NUM-1);
	next_id=0;
	return 0;
}

void ser_cleanup_cdev(void){

    printk(KERN_DEBUG "unregister cdev \"%s\" %d:[0-%d]\n"
        ,str_dev_prefix,device_major,MAX_DEV_NUM-1);
    if(device_major){
        unregister_chrdev_region(MKDEV(device_major,0),MAX_DEV_NUM);
    }
}


int ser_add_cdev(struct ser_dev *dev)
{
    int err;

    dev_info(dev->dev, "set up cdev %d:%d\n",device_major,next_id);

    cdev_init(&dev->cdev, &fops);

    dev->cdev.owner = THIS_MODULE;

    err = cdev_add(&dev->cdev, MKDEV(device_major,next_id), 1);
    if(err){
		dev_err(dev->dev, "fail to add cdev %d:%d %d\n",
				device_major,next_id,err);
		return err;
    }
	next_id+=1;
    return 0;
}

int ser_del_cdev(struct ser_dev *dev)
{
    dev_info(dev->dev, "remove cdev %d:%d\n",device_major,MINOR(dev->cdev.dev));
    cdev_del(&dev->cdev);
    return 0;
}

static int fop_open(struct inode *inode, struct file *filp)
{
    struct ser_dev *dev;
    dev = container_of(inode->i_cdev, struct ser_dev, cdev);

    filp->private_data = dev;

	if(ser_start(dev)) {
		dev_err(dev->dev, "can not start device\n");
		return -1;
	}

    dev_dbg(dev->dev,"device open\n");
    return 0;
}

static int fop_release(struct inode *inode, struct file *filp)
{
    struct ser_dev *dev = filp->private_data;

	if(ser_stop(dev)) {
		dev_err(dev->dev, "can not stop device\n");
		return -1;
	}

    dev_dbg(dev->dev,"close device\n");
    return 0;
}

static ssize_t fop_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct ser_dev *dev = filp->private_data;
	size_t size=0;

    dev_dbg(dev->dev, "fop read %zd @ %p\n", count, buf);    

    /* lock mutex to keep transaction integrity */
    if(mutex_lock_interruptible(&dev->r_mutex)){
        dev_dbg(dev->dev,"read: user interupt\n");
        return -EAGAIN;
    }

    while(0==size){ 
		size = ser_next_read_size(dev);
		if(size){
			void *ptr = ser_next_read_ptr(dev);

			if(size > count)
				size = count;

            if(copy_to_user(buf, ptr, size)){
                dev_err(dev->dev, "error copy to user 0x%p -> 0x%p\n", 
                        ptr, buf);
                goto ERR;
            }
			ser_pop_read_data(dev, size);
		}
        else if(filp->f_flags & O_NONBLOCK){
			break;
        }
        else {
            /* wait for ready */
            dev_dbg(dev->dev,"read: going to sleep\n");
            wait_event_interruptible_timeout(dev->r_wait,
                    ser_next_read_size(dev),HZ);
            if(signal_pending(current)){
                dev_dbg(dev->dev,"read: user interupt\n");
				break;
            }
            else {
                dev_dbg(dev->dev,"read: wake up\n");
            }
        }

    }
    mutex_unlock(&dev->r_mutex);

    return size;
ERR:
    mutex_unlock(&dev->r_mutex);
    return -EIO;
}

static ssize_t fop_write(struct file *filp, const char __user *buf, size_t count, loff_t *fpos)
{
	return 0;
}

static unsigned int fop_poll(struct file *filp, struct poll_table_struct *wait)
{
	struct ser_dev *dev = filp->private_data;
	unsigned int mask=0;

	/* check for read */
	poll_wait(filp, &dev->r_wait, wait);
	if(ser_next_read_size(dev))
		mask |= POLLIN | POLLRDNORM;

	return mask; 
}


