#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/device.h>

#define PSEUDO_INC _IOW('p', 1, signed char)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Murat Toprak");
MODULE_DESCRIPTION("Basic character device driver implementation");

static int pseudo_major = 0;
static int pseudo_minor = 0;

struct cdev pseudo_cdev;
struct semaphore pseudo_sem;

static int capacity = 0;
static char *pseudo_data = NULL;

module_param(capacity, int, S_IRUGO);

static struct class *pseudo_class;
static struct device *pseudo_device;

int pseudo_open(struct inode *inode, struct file *filp) {
	printk(KERN_DEBUG "pseudo: opening device\n");
	return 0;
}

int pseudo_release(struct inode *inode, struct file *filp) {
	printk(KERN_DEBUG "pseudo: closing device\n");
	return 0;
}

ssize_t pseudo_read(struct file *filp, char __user *buf,
					size_t count, loff_t *f_pos) {

	int i = 0;
	int err = 0;

	// Checking availability
	if (down_interruptible(&pseudo_sem) != 0) { 
		return -ERESTARTSYS;
	}

	for (i = 0; i < count; ++i) {
		if (*f_pos >= capacity) { *f_pos = 0; }

		// copy_to_user(destination, source, size)
		err = copy_to_user(buf + i, pseudo_data + *f_pos, 1);
		if (err != 0) {
			up(&pseudo_sem);
			return -EFAULT;
		}
		(*f_pos)++;
	}
	up(&pseudo_sem);
	return count;
}

ssize_t pseudo_write(struct file *filp, const char __user *buf,
					size_t count, loff_t *f_pos) {
	int err = 0;

	// Check availability
	if (down_interruptible(&pseudo_sem)) {
		return -ERESTARTSYS;
	}

	// wrote all
	if (*f_pos >= capacity) {
		goto out;
	}

	// partial write
	if (*f_pos + count > capacity) {
		count = capacity - *f_pos;
	}

	// where the magic happens
	err = copy_from_user(pseudo_data + *f_pos, buf, count);
	if (err != 0) {
		up(&pseudo_sem);
		return -EFAULT;
	}

	(*f_pos) += count;

out:
	up(&pseudo_sem);
	return count;
}

loff_t pseudo_llseek(struct file *filp, loff_t off, int whence) {
	loff_t newpos = 0;

	switch(whence) {
	// SEEK_SET: from beginning, SEEK_CUR: from current, SEEK_END: from end
	case SEEK_SET: 
		newpos = off;
		break;

	case SEEK_CUR:
		newpos = filp->f_pos + off;
		break;

	case SEEK_END:
		newpos = capacity + off - 1;
		break;

	// default can't happen
	}

	if (newpos < 0 || newpos >= capacity) 
		return -EINVAL;

	filp->f_pos = newpos;
	return newpos;

}

/* unlocked ioctl, without taking the file's lock */
long pseudo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	signed char increment;
	int err;
	int i;

	switch(cmd) {
	case PSEUDO_INC:
		// Copy the ioctl argument from user space to kernel space
		err = copy_from_user(&increment, (signed char __user *)arg, sizeof(signed char));
		if (err < 0) {
			return -EFAULT;
		}

		// Perform the PSEUDO_INC operation
		// iterate over each byte in the device buffer and increment that byte with the given value
		for (i = 0; i < capacity; i++) {
			pseudo_data[i] += increment;
		}
		break;
	
	default:
		return -ENOTTY;  // invalid ioctl
	}
	return 0;
}

struct file_operations pseudo_fops = {
	.owner = THIS_MODULE,
	.open = pseudo_open,
	.release = pseudo_release,
	.read = pseudo_read,
	.write = pseudo_write,
	.llseek = pseudo_llseek,
	.unlocked_ioctl = pseudo_ioctl,
};

void pseudo_fill(void) {
	for (int i = 0; i < capacity; ++i)
		pseudo_data[i] = i;
}

static int pseudo_init(void) {
	dev_t devno = 0;
	int result = -1;
	int err = 0;

	// Filling with data
	pseudo_data = kmalloc(capacity, GFP_KERNEL);
	if (pseudo_data == NULL) {
		return -ENOMEM;
	}
	pseudo_fill();

	if (pseudo_major == 0) {
		result = alloc_chrdev_region(&devno, pseudo_minor, 1, "pseudo");
		pseudo_major = MAJOR(devno);
	} else {
		devno = MKDEV(pseudo_major, pseudo_minor); 
		result = register_chrdev_region(devno, 1, "pseudo");
	}

	if (result < 0) {
		printk(KERN_WARNING "pseudo: can't get major %d\n", pseudo_major);
		return result;
	}

	sema_init(&pseudo_sem, 1);
	cdev_init(&pseudo_cdev, &pseudo_fops);
	err = cdev_add(&pseudo_cdev, devno, 1);

	if (err != 0) {
		printk(KERN_NOTICE "Error %d adding pseudo device\n", err);
	}

	// Create a device class
	pseudo_class = class_create("pseudo");
	if (IS_ERR(pseudo_class)) {
		printk(KERN_ERR "pseudo: failed to register device class\n");
        return PTR_ERR(pseudo_class);
	}

	// Create the device node
	/* 
	struct device *device_create(
		struct class *class: device class to which the new device belongs, 
		struct device *parent: parent device of the new device, NULL if not, 
		dev_t devt: device number of the new device,
		void *drvdata: device specific data, NULL if not, 
		const char *fmt: name of the new device,
		...);
	*/
	pseudo_device = device_create(pseudo_class, NULL, devno, NULL, "pseudo");
	if (IS_ERR(pseudo_device)) {
		printk(KERN_ERR "pseudo: failed to create the device node\n");
		return PTR_ERR(pseudo_device);
	}

	printk(KERN_INFO "pseudo: device has been created successfully");
	return 0;
}

static void pseudo_exit(void) {
	dev_t devno = 0;
	devno = MKDEV(pseudo_major, pseudo_minor);

	// Remove the device node
	device_destroy(pseudo_class, devno);

	// Remove the device class
	class_destroy(pseudo_class);

	// Remove the character device
	cdev_del(&pseudo_cdev);

	// Unregister the major number 
	unregister_chrdev_region(devno, 1);
	
	// Free the buffer
	kfree(pseudo_data);
}

module_init(pseudo_init);
module_exit(pseudo_exit);
