#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/spi/spi.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/cdev.h>

#include "lcd_ioctl.h"
#include "lcdILI9341.h"

struct lcd_device {
	int major;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct spi_device *spi_dev;
	struct mutex mutex;
};

#define to_lcd_device(x) container_of(x, struct lcd_device, cdev)

static ssize_t dev_write(struct file *, const char __user *, size_t, loff_t *);
static long dev_ioctl(struct file *, unsigned int, unsigned long);

static struct file_operations fops = {
	.write = dev_write,
	.unlocked_ioctl = dev_ioctl,
};

static ssize_t dev_write(struct file *filep, const char __user *buf, size_t len,
			loff_t *ofset)
{
	return 0;
}

static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	unsigned char *value = (unsigned char *)argp;
	unsigned char rotation;
	int invert_display;
	struct lcd_device *lcd_dev = to_lcd_device(filep->f_inode->i_cdev);
	pixel_t pixel;

	mutex_lock(&lcd_dev->mutex);
	switch (cmd) {
	case DRAW_PIXEL:
		copy_from_user(&pixel, argp, sizeof(pixel_t));
		drawPixel(lcd_dev->spi_dev, pixel.x, pixel.y, pixel.color);
		break;

	case SET_ROTATION:
		get_user(rotation, value);
		setRotaion(lcd_dev->spi_dev, rotation);
		break;

	case INVERT_DISPLAY:
		get_user(invert_display, value);
		invertDisplay(lcd_dev->spi_dev, invert_display);
		break;

	case LCD_RESET:
		pr_info("clearscree\n");
		fillScreen(lcd_dev->spi_dev, 0xffff);
		break;

	default:
		return -ENOTTY;
	}
	mutex_unlock(&lcd_dev->mutex);
		
	return 0;
}

static int dev_probe(struct spi_device *spi)
{
	int ret;
	struct lcd_device *lcd_dev;

	pr_info("Jumpt to %s function\n", __func__);

	lcd_dev = devm_kzalloc(&spi->dev, sizeof(struct lcd_device), GFP_KERNEL);
	lcd_dev->spi_dev = spi;

	spi_set_drvdata(spi, lcd_dev);
	ret = alloc_chrdev_region(&lcd_dev->major, 0, 1, "lcdILI9341");
	if (ret < 0) {
		pr_err("Failed to alloc chrdev\n");
		goto Failed_alloc_chrdev;
	}
	pr_info("in probe func: major is %d\n", lcd_dev->major);

	cdev_init(&lcd_dev->cdev, &fops);
	ret = cdev_add(&lcd_dev->cdev, lcd_dev->major, 1);
	if (ret < 0) {
		pr_err("Failed to add cdev\n");
		goto Faled_cdev_add;
	}

	lcd_dev->class = class_create(THIS_MODULE, "lcdILI9341");
	if (IS_ERR(lcd_dev->class)) {
		ret = (int)PTR_ERR(lcd_dev->class);
		pr_err("Failed to create class\n");
		goto Failed_create_class;
	}

	lcd_dev->device = device_create(lcd_dev->class, NULL, lcd_dev->major,
					NULL, "lcdILI9341");
	if (IS_ERR(lcd_dev->device)) {
		ret = (int)PTR_ERR(lcd_dev->device);
		pr_err("Failed to create device\n");
		goto Failed_create_device;
	}
	mutex_init(&lcd_dev->mutex);

	/* lcd begin */
	lcd_begin(spi);

	return 0;

Failed_create_device:
	class_destroy(lcd_dev->class);
Failed_create_class:
	cdev_del(&lcd_dev->cdev);
Faled_cdev_add:
	unregister_chrdev_region(lcd_dev->major, 1);
Failed_alloc_chrdev:
	return ret;
}

static int dev_remove(struct spi_device *spi)
{
	struct lcd_device *lcd_dev = spi_get_drvdata(spi);

	cdev_del(&lcd_dev->cdev);
	device_destroy(lcd_dev->class, 1);
	class_destroy(lcd_dev->class);
	unregister_chrdev_region(lcd_dev->major, 1);
	mutex_destroy(&lcd_dev->mutex);

	return 0;
}

static struct of_device_id lcd_of_match[] = {
	{ .compatible = "tft,lcdILI9341" },
	{ /* NULL */ },
};
MODULE_DEVICE_TABLE(of, lcd_of_match);

static struct spi_driver lcd_driver = {
	.remove = dev_remove,
	.probe = dev_probe,
	.driver = {
		.name = "lcdILI9341",
		.of_match_table = lcd_of_match,
		.owner = THIS_MODULE,
	},
};

module_spi_driver(lcd_driver);

MODULE_AUTHOR("tungnt@fih-foxconn.com");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");
