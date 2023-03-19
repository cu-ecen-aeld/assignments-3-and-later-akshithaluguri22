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
    int entry_count = 0;
    size_t offs;
    ssize_t ret = 0;
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry *entry_buff;

    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);   
    /**
     * TODO: handle read
     */
     
    mutex_lock(&aesd_device.mtx_lock);
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
    
   	mutex_unlock(&aesd_device.mtx_lock);
    
    return ret;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    int packet_len = 0;
    int i=0;
    ssize_t ret = 0;
    bool packet_flag = false;
    char *ret_entry;
    char *tmp_buf;
    struct aesd_dev *dev= filp->private_data;
    struct aesd_buffer_entry entry_buff;

    
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    
    mutex_lock(&aesd_device.mtx_lock);

    tmp_buf = (char *)kmalloc(count, GFP_KERNEL);
    
    if( tmp_buf == NULL )
    {
        ret = -ENOMEM;
        goto mtx_unlock;
    }


    if(copy_from_user(tmp_buf, buf, count))
    {
        ret = -EFAULT;
        goto free_and_unlock;
    }

 
    while( i< count)
    {
        if(tmp_buf[i] == '\n')
        {
            packet_flag = true;
            packet_len = i+1;
            break;
        }
       i++;
    }

    if( dev->buffer_size == 0 )
    {
        dev->buffer = (char *)kmalloc(count, GFP_KERNEL);
        if( dev->buffer == NULL )
        {
            ret = -ENOMEM;
            goto free_and_unlock;
        }
        memcpy(dev->buffer, tmp_buf, count);
        dev->buffer_size += count;
    }
    else
    {
        int extra;
        if(!packet_flag)
        {
            extra = count;
        }
        else
        {
            extra = packet_len;
        }
        
        dev->buffer = (char *)krealloc(dev->buffer, dev->buffer_size + extra , GFP_KERNEL);
        
        if( NULL == dev->buffer )
        {
            ret = -ENOMEM;
            goto free_and_unlock;
        }
        memcpy(dev->buffer + dev->buffer_size, tmp_buf, extra);
        
        dev->buffer_size = dev->buffer_size + extra;
    }

    if(true == packet_flag)
    {
        entry_buff.buffptr = dev->buffer;
        entry_buff.size = dev->buffer_size;
        ret_entry = aesd_circular_buffer_add_entry(&dev->c_buff, &entry_buff);
        if( ret_entry != NULL )
        {
            kfree(ret_entry);
        }
        dev->buffer_size = 0;
    }
    
    ret = count;
    
    free_and_unlock :
    		kfree(tmp_buf);
    mtx_unlock : 
    		mutex_unlock(&aesd_device.mtx_lock);
    		
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
    
    int result;
    dev_t dev = 0;
    result = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar");
    
    aesd_major = MAJOR(dev);
    
    if (result < 0) 
    {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    mutex_init(&aesd_device.mtx_lock);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) 
    {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    int index;
    struct aesd_buffer_entry *buffer_element;
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    AESD_CIRCULAR_BUFFER_FOREACH(buffer_element, &aesd_device.c_buff, index)
    {
        kfree(buffer_element->buffptr);
    }
    mutex_destroy(&aesd_device.mtx_lock);

    unregister_chrdev_region(devno, 1);
}


module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
