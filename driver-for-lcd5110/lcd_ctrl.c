#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>

#include "lcd_ioctl.h"
#include "lcd_5110.h"

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

/*this funtion to write string from user to LCD*/
static ssize_t dev_write(struct file *filep, const char __user *buf, size_t len,
                        loff_t *offset)
{
	int ret;
	Draw_String_t *str = NULL;

	str = kmalloc(sizeof(Draw_String_t), GFP_KERNEL);

	ret = copy_from_user(str, buf, sizeof(Draw_String_t));
	if (ret) {
		pr_err("can not copy from user\n");
		return -ENOMSG;
	}

	pr_info("get string from user: %s\n", str->message);
	LCD_Puts(str->message, str->pixel, str->font);

	return len;
}

static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	unsigned char *value = (unsigned char *)argp;
	unsigned char contrast;
	Position_t *pos = NULL;
	Draw_Pixel_t *pixel = NULL;
	Draw_Shape_t *shape = NULL;
	Draw_Circle_t *cir = NULL;

	switch (cmd) {
	case IOCTL_SEND_BUFF:
			LCD_Refresh();
			break;

	case IOCTL_CLEAR:
			LCD_Clear();
			break;

	case IOCTL_HOME:
			LCD_Home();
			break;

	case IOCTL_SET_CONTRAST:
			get_user(contrast, value);
			pr_info("contrast value is: %d\n", (int)contrast);
			LCD_SetContrast(contrast);
			break;

	case IOCTL_GOTOXY:
			pos = kmalloc(sizeof(Position_t), GFP_KERNEL);
			copy_from_user(pos, argp, sizeof(Position_t));
			pr_info("x = %d, y = %d\n",(int)pos->x, (int)pos->y);
			LCD_GotoXY(pos->x, pos->y);
			break;

	case IOCTL_DRAW_PIXEL:
			pixel = kmalloc(sizeof(Draw_Pixel_t), GFP_KERNEL);
			copy_from_user(pixel, argp, sizeof(Draw_Pixel_t));
			pr_info("x = %d, y = %d\n", (int)pixel->x, (int)pixel->y);
			LCD_DrawPixel(pixel->x, pixel->y, pixel->pixel);
			break;

	case IOCTL_DRAW_LINE:
			shape = kmalloc(sizeof(Draw_Shape_t), GFP_KERNEL);
			copy_from_user(shape, argp, sizeof(Draw_Shape_t));
			pr_info("x = %d, y = %d\n", (int)shape->x0, (int)shape->y0);
			LCD_DrawLine(shape->x0, shape->y0, shape->x1, shape->y1,
					shape->pixel);
			break;

	case IOCTL_DRAW_RECT:
			shape = kmalloc(sizeof(Draw_Shape_t), GFP_KERNEL);
			copy_from_user(shape, argp, sizeof(Draw_Shape_t));
			pr_info("x = %d, y = %d\n", (int)shape->x0, (int)shape->y0);
			LCD_DrawRectangle(shape->x0, shape->y0, shape->x1,
					shape->y1, shape->pixel);
			break;

	case IOCTL_DRAW_FILL_RECT:
			shape = kmalloc(sizeof(Draw_Shape_t), GFP_KERNEL);
			copy_from_user(shape, argp, sizeof(Draw_Shape_t));
			pr_info("x = %d, y = %d\n", (int)shape->x0, (int)shape->y0);
			LCD_DrawFilledRectangle(shape->x0, shape->y0, shape->x1,
					shape->y1, shape->pixel);
			break;

	case IOCTL_DRAW_CIRCLE:
			cir = kmalloc(sizeof(Draw_Circle_t), GFP_KERNEL);
			copy_from_user(cir, argp, sizeof(Draw_Circle_t));
			pr_info("x = %d, y = %d\n", (int)cir->x, (int)cir->y);
			LCD_DrawCircle(cir->x, cir->y, cir->r, cir->pixel);
			break;

	case IOCTL_DRAW_FILL_CIRCLE:
			cir = kmalloc(sizeof(Draw_Circle_t), GFP_KERNEL);
			copy_from_user(cir, argp, sizeof(Draw_Circle_t));
			pr_info("x = %d, y = %d\n", (int)cir->x, (int)cir->y);
			LCD_DrawFilledCircle(cir->x, cir->y, cir->r, cir->pixel);
			break;

	default:
		return -ENOTTY;
	}

	return 0;
}

static int __init lcd_init(void)
{
	int ret;

	LCD_Init(0x38);

        ret = misc_register(&my_dev);
	if (ret) {
		pr_err("can not register device\n");
		return ret;
	}
	pr_info("Init successfully\n");

	return ret;
}

static void __exit lcd_exit(void)
{
	misc_deregister(&my_dev);
	LCD_free_IO();
	pr_info("goodbye\n");
}

module_init(lcd_init);
module_exit(lcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tungnt58@fsoft.com.vn + LongVH12@fsoft.com.vn");
MODULE_DESCRIPTION("This module to control lcd5110");
