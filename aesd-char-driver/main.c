/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h>
#include <linux/string.h>
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Akshith Aluguri"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    filp->private_data = container_of(inode->i_cdev, struct aesd_dev, cdev);;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    filp->private_data = NULL;
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t ret = 0;
    struct aesd_dev *dev;
    struct aesd_buffer_entry *entry_buff;
    int entry_count = 0;
    size_t offs;
    
    dev = filp->private_data;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    
    /**
     * TODO: handle read
     */
     
    mutex_lock(&aesd_device.lock);
    
    entry_buff = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->c_buff, *f_pos, &offs);
    
    if( entry_buff == NULL )
    {
        *f_pos = 0;
        goto exit;
    }

    if( count > (entry_buff->size - offs) )
    {
        *f_pos += entry_buff->size - offs;
        entry_count = entry_buff->size - offs;
    }
    else
    {
        *f_pos += count;
        entry_count = count;
    }

    if( copy_to_user(buf, entry_buff->buffptr+offs, entry_count))
    {
        ret = -EFAULT;
        goto exit;
    }

    ret = entry_count;

    exit : 
    
   	mutex_unlock(&aesd_device.lock);
    
    return ret;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t ret = 0;
    char *tmp_buf;
    bool packet_flag = false;
    struct aesd_dev *dev;
    int packet_len = 0;
    struct aesd_buffer_entry write_entry;
    char *ret_entry;
    int i;
    
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
     
    dev = filp->private_data;

    mutex_lock(&aesd_device.lock);

    // malloc a new buffer
    tmp_buf = (char *)kmalloc(count, GFP_KERNEL);
    if( tmp_buf == NULL )
    {
        ret = -ENOMEM;
        goto end;
    }

    // copy from user buffer to new buffer
    if(copy_from_user(tmp_buf, buf, count))
    {
        ret = -EFAULT;
        goto free_and_end;
    }

    // check for newline char 
    for(i=0; i< count; i++)
    {
        if(tmp_buf[i] == '\n')
        {
            packet_flag = true;
            packet_len = i+1;
            break;
        }
    }

    // copy the packet
    if( dev->buff_size == 0 )
    {
        dev->buff_ptr = (char *)kmalloc(count, GFP_KERNEL);
        if( dev->buff_ptr == NULL )
        {
            ret = -ENOMEM;
            goto free_and_end;
        }
        memcpy(dev->buff_ptr, tmp_buf, count);
        dev->buff_size += count;
    }
    else
    {
        int extra;
        if(packet_flag)
        {
            extra = packet_len;
        }
        else
        {
            extra = count;
        }
        dev->buff_ptr = (char *)krealloc(dev->buff_ptr, dev->buff_size + extra , GFP_KERNEL);
        if( dev->buff_ptr == NULL )
        {
            ret = -ENOMEM;
            goto free_and_end;
        }
        memcpy(dev->buff_ptr + dev->buff_size, tmp_buf, extra);
        dev->buff_size += extra;
    }

    // add entry to circular buffer
    if(packet_flag)
    {
        write_entry.buffptr = dev->buff_ptr;
        write_entry.size = dev->buff_size;
        ret_entry = aesd_circular_buffer_add_entry(&dev->c_buff, &write_entry);
        if( ret_entry != NULL )
        {
            kfree(ret_entry);
        }
        dev->buff_size = 0;
    }
    ret = count;
    free_and_end : kfree(tmp_buf);
    end : mutex_unlock(&aesd_device.lock);
    return ret;
    
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */
     
    mutex_init(&aesd_device.lock);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
	int index;
    struct aesd_buffer_entry *ptr;
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    AESD_CIRCULAR_BUFFER_FOREACH(ptr, &aesd_device.c_buff, index)
    {
        kfree(ptr->buffptr);
    }
    mutex_destroy(&aesd_device.lock);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
