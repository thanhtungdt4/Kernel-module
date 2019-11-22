 /*
 * @file	char_driver.c
 * @author	Luu An Phu
 * @date	3 June 2018
 * @version 1.0
 * @brief	Vi du ve mot character driver don gian, cho phep user doc ghi thong tin tu device file
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define  DEVICE_NAME "simple_char"
#define  CLASS_NAME  "simple_char_device"
 
MODULE_LICENSE("GPL");				//Loai license ma driver su dung, cho phep driver truy cap vao cac function co cung license
MODULE_AUTHOR("Luu An Phu");		//Ten tac gia, user co the xem duoc thong tin nay sau khi load driver
MODULE_DESCRIPTION("A simple Linux char driver");	//Mo ta ve driver
MODULE_VERSION("0.1");				//Version cua driver
 
static int    major_number;
static char   message[256];
//static short  size_of_message;
//static int    numberOpens = 0;
static struct class*  char_class;
static struct device* char_device;

static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

static int __init char_init(void)
{
	printk(KERN_INFO "%s: Khoi tao driver\n", __func__);
	/* Cap phat dong major number cho char device */
	major_number = register_chrdev(0, DEVICE_NAME, &fops);
	if (major_number < 0) {
		printk(KERN_ALERT "%s khong cap phat duoc major number\n", __func__);
		return major_number;
	}
 	printk(KERN_INFO "Register successfully major no is: %d\n", major_number);
	/* Dang ky class cho device */
	char_class = class_create(THIS_MODULE, CLASS_NAME);
  	 if (IS_ERR(char_class)) {
		printk(KERN_ALERT "%s Khong dang ky duoc device class\n", __func__);
		unregister_chrdev(major_number, DEVICE_NAME);
		return PTR_ERR(char_class);
	}

	printk(KERN_INFO "Tao thanh cong class\n");
	/* Dang ky char device */
	char_device = device_create(char_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
	if (IS_ERR(char_device)) {
		printk(KERN_ALERT "%s Khong tao duoc device\n", __func__);
		class_destroy(char_class);
		unregister_chrdev(major_number, DEVICE_NAME);
		return PTR_ERR(char_device);
	}

	printk(KERN_INFO "Khoi tao thanh cong trong %s function\n", __func__);
	return 0;
}

static void __exit char_exit(void)
{
	device_destroy(char_class, MKDEV(major_number, 0));
	class_unregister(char_class);
	class_destroy(char_class);
	unregister_chrdev(major_number, DEVICE_NAME);
	printk(KERN_INFO "%s: driver exit\n", __func__);
}

static int dev_open(struct inode *inodep, struct file *filep)
{
	return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
	int error = 0;
	/* Muon ghi vao bo nho tren user, phai su dung copy_to_user */
	error = copy_to_user(buffer, message, strlen(message));

	if (error != 0) {
		printk(KERN_INFO "%s: Khong the ghi du lieu vao bo nho cua user\n", __func__);
		return -EFAULT;
	}

	printk(KERN_INFO "%s: Gui chuoi %s len user-space\n",__func__,  message);
	return 0;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
	int error = 0;
	memset(message, 0, sizeof(message));
	error = copy_from_user(message, buffer, len);
	if(error != 0){
		printk(KERN_ALERT "Khong the nhan du lieu tu user\n");
	}

	printk(KERN_INFO "%s: Da ghi chuoi %s vao message\n",__func__, buffer);

	return len;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
	return 0;
}

module_init(char_init);
module_exit(char_exit);
