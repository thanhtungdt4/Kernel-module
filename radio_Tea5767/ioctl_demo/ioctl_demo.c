#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

#define MAGIC_NO                150

#define DEMO_CMD		_IOWR(MAGIC_NO, 0, int)

static long demo_ioctl(struct file *, unsigned int, unsigned long);
static int demo_open(struct inode *, struct file *);
static int demo_close(struct inode *, struct file *);

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = demo_open,
	.release = demo_close,
	.unlocked_ioctl = demo_ioctl,
};

static struct miscdevice btn_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "demo",
	.fops = &fops,
};

static int demo_open(struct inode *inodep, struct file *filep)
{
	return 0;
}

static int demo_close(struct inode *inodep, struct file *filep)
{
	return 0;
}

static long demo_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user*)arg;
	int val;
	int val2 = 6;
	int __user *user_data = argp;

	switch (cmd) {
	case DEMO_CMD:
//		int val;
//		int val2 = 6;
//		int __user *user_data = argp;

		copy_from_user(&val, argp, sizeof(val));
		pr_info("val get from user: %d\n", val);
		copy_to_user(user_data, &val2, sizeof(val2));
		break;
	}

	return 0;
}

static int __init demo_init(void)
{
	int ret;

	ret = misc_register(&btn_dev);

	return ret;
}

static void __exit demo_exit(void)
{
	misc_deregister(&btn_dev);
	pr_info("goodbye\n");
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tung");
MODULE_DESCRIPTION("This module to control button\n");
