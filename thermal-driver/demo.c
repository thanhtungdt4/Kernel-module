#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/kthread.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TUNG");

int i;
static int bit;

static int dev_open(struct inode *, struct file *);
static int dev_close(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char __user *,
			size_t, loff_t *);

static struct file_operations fops = {
	.open = dev_open,
	.release = dev_close,
	.read = dev_read,
	.write = dev_write,
};

struct miscdevice mydev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mydevice",
	.fops = &fops,
};

static int dev_open(struct inode *inodep, struct file *filep)
{
	pr_info("open file\n");
	return 0;
}

static int dev_close(struct inode *inodep, struct file *filep)
{
	pr_info("close file\n");
	return 0;
}

static ssize_t dev_read(struct file *filep, char __user *buf,
			size_t len, loff_t *offset)
{
	pr_info("read file\n");
	return 0;
}

static ssize_t dev_write(struct file *filep, const char __user *bug,
			size_t len, loff_t *offset)
{
	pr_info("write to file\n");
	return 0;
}

static void printf_fs(int b)
{
	pr_info("%d\n", b);
}

int thread_func(void *data)
{
        i = 0;

	while (i < 100 ) {
		i++;
		msleep(200);
		printf_fs(bit = !bit);
	}
	
	return 0;
}


static int __init example_init(void)
{
	int ret;

	ret = misc_register(&mydev);
	if (ret) {
		pr_alert("can not register misc device\n");
		return ret;
	}
	pr_info("register device successfully with minor number is: %d\n",
			mydev.minor);
	kthread_run(thread_func, NULL, "thread");

	return ret;
}

static void __exit example_exit(void)
{
	misc_deregister(&mydev);
	pr_info("goodbye\n");
}

module_init(example_init);
module_exit(example_exit);


