#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Murat Toprak");

static int pseudo_major = 0;
static int pseudo_minor = 0;

struct cdev pseudo_cdev;
struct semaphore pseudo_sem;

static int capacity = 0;
static char *pseudo_data = NULL;

module_param(capacity, int, S_IRUGO);

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

	// if (*f_pos >= capacity) {*f_pos = 0;}

	// if (*f_pos + count > capacity) {
	// 	count = capacity - *f_pos;
	// }

	for (i = 0; i < count; ++i) {
		if (*f_pos >= capacity) { *f_pos = 0; }

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

struct file_operations pseudo_fops = {
	.owner = THIS_MODULE,
	.open = pseudo_open,
	.release = pseudo_release,
	.read = pseudo_read,
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
	return 0;
}

static void pseudo_exit(void) {
	dev_t devno = 0;

	cdev_del(&pseudo_cdev);
	devno = MKDEV(pseudo_major, pseudo_minor);
	unregister_chrdev_region(devno, 1);
	kfree(pseudo_data);
}

module_init(pseudo_init);
module_exit(pseudo_exit);