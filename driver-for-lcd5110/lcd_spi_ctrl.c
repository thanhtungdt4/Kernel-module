#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <asm/delay.h>

#include "lcd_5110.h"

#define LCD_CLK_MAX		10000000
#define BIT_PER_WORD		8

static struct spi_device *lcd_dev;
static char message[20];
static int count;

static int dev_open(struct inode *, struct file *);
static int dev_close(struct inode *, struct file *);
static ssize_t dev_write(struct file *, const char __user *, size_t, loff_t *);
static long dev_ioctl(struct file *, unsigned int, unsigned long);

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.release = dev_close,
	.unlocked_ioctl = dev_ioctl,
	.write = dev_write,
};

static struct miscdevice my_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "framebuf",
	.fops = &fops,
};

static struct spi_board_info lcd_info = {
	.modalias = "lcd5110",
	.chip_select = 0,
	.max_speed_hz = LCD_CLK_MAX,
	.bus_num = 0,
	.mode = SPI_MODE_3,
};

static int dev_open(struct inode *inodep, struct file *filep)
{
	count = 0;
	pr_info("open file\n");
	return 0;
}

static int dev_close(struct inode *inodep, struct file *filep)
{
	pr_info("close file\n");
	return 0;
}

/*this funtion to write string from user to LCD*/
static ssize_t dev_write(struct file *filep, const char __user *buf, size_t len,
			loff_t *offset)
{
	int ret;

	if (count != 0)
		return 0;
	count++;
	ret = copy_from_user(message, buf, len);
	if (ret) {
		pr_err("can not copy from user\n");
	}
	udelay(100);
	pr_info("get string from user: %s\n", message);
	LCD_GotoXY(14, 3);
	LCD_Puts(message, LCD_Pixel_Set, LCD_FontSize_3x5);
	LCD_Refresh(lcd_dev);

	return len;
}

static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static int lcd_probe(struct spi_device *spi)
{
	int ret;

	LCD_Init(spi, 0x38);
	ret = misc_register(&my_dev);
	if (ret) {
		pr_err("can not register device\n");
		return ret;
	}

	return ret;
}

static int lcd_remove(struct spi_device *spi)
{
	misc_deregister(&my_dev);
	LCD_free_IO();

	return 0;
}

static struct spi_device_id lcd_idtable[] = {
	{"lcd5110", 0},
	{ },
};
MODULE_DEVICE_TABLE(spi, lcd_idtable);

static struct spi_driver lcd_driver = {
	.driver = {
		.name = "lcd5110",
	},
	.id_table = lcd_idtable,
	.probe = lcd_probe,
	.remove = lcd_remove,
};

static int __init lcd_init(void)
{
	int ret;
	struct spi_controller *master;

	master = spi_busnum_to_master(lcd_info.bus_num);
	if (master == NULL) {
		pr_err("%s Failed\n", __func__);
		return -ENODEV;
	}

	lcd_dev = spi_new_device(master, &lcd_info);
	if (!lcd_dev) {
		pr_err("can not add new device\n");
		return -ENODEV;
	}
	lcd_dev->bits_per_word = BIT_PER_WORD;
	ret = spi_setup(lcd_dev);
	if (ret) {
		spi_unregister_device(lcd_dev);
		return -ENODEV;
	}

	ret = spi_register_driver(&lcd_driver);
	if (ret) {
		pr_err("Can not register spi driver\n");
		return ret;
	}

	pr_info("Register driver successfully\n");

	return ret;
}

static void __exit lcd_exit(void)
{
	spi_unregister_driver(&lcd_driver);
	spi_unregister_device(lcd_dev);
	pr_info("goodbye\n");
}

module_init(lcd_init);
module_exit(lcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tung + Long");
MODULE_DESCRIPTION("This module to control lcd5110");
