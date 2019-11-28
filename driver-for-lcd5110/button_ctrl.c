#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>

#define LEFT_BTN			20
#define RIGHT_BTN			21
#define UP_BTN				18
#define DOWN_BTN			19

static int left_btn_irq;
static int right_btn_irq;
static int up_btn_irq;
static int down_btn_irq;

typedef enum {
	UP_STATE = 1,
	DOWN_STATE,
	LEFT_STATE,
	RIGHT_STATE
} Btn_State_t;

static Btn_State_t button_state;

static int dev_open(struct inode *, struct file *);
static int dev_close(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char __user *, size_t, loff_t *);

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.read = dev_read,
	.release = dev_close,
};

static struct miscdevice btn_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "buttons",
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

static ssize_t dev_read(struct file *filep, char __user *buf, size_t len,
			loff_t *offset)
{
	copy_to_user(buf, &button_state, sizeof(button_state));
	return sizeof(button_state);
}

static irqreturn_t button_irq_handle(int irq, void *data)
{
	Btn_State_t *btn_state = (Btn_State_t *)data;

	if (*btn_state == UP_STATE) {
		button_state = UP_STATE;
	} else if (*btn_state == DOWN_STATE) {
		button_state = DOWN_STATE;
	} else if (*btn_state == LEFT_STATE) {
		button_state = LEFT_STATE;
	} else {
		button_state = RIGHT_STATE;
	}

	return IRQ_HANDLED;
}

static int __init button_init(void)
{
	int ret;
	Btn_State_t btn_left = LEFT_STATE;
	Btn_State_t btn_right = RIGHT_STATE;
	Btn_State_t btn_up = UP_STATE;
	Btn_State_t btn_down = DOWN_STATE;

	if (gpio_request(LEFT_BTN, "left button")) {
		pr_err("gpio request failed at %d\n", LEFT_BTN);
		return -ENODEV;
	}

	if (gpio_request(RIGHT_BTN, "right button")) {
		pr_err("gpio request failed at %d\n", RIGHT_BTN);
		return -ENODEV;
	}

	if (gpio_request(UP_BTN, "up button")) {
		pr_err("gpio request failed at %d\n", UP_BTN);
		return -ENODEV;
	}

	if (gpio_request(DOWN_BTN, "down button")) {
		pr_err("gpio request failed at %d\n", DOWN_BTN);
		return -ENODEV;
	}

	if (left_btn_irq = gpio_to_irq(LEFT_BTN) < 0) {
		pr_err("gpio to irq maping failed at %d\n", LEFT_BTN);
		return left_btn_irq;
	}

	if (right_btn_irq = gpio_to_irq(RIGHT_BTN) < 0) {
		pr_err("gpio to irq maping failed at %d\n", RIGHT_BTN);
		return right_btn_irq;
	}

	if (up_btn_irq = gpio_to_irq(UP_BTN) < 0) {
		pr_err("gpio to irq maping failed at %d\n", UP_BTN);
		return up_btn_irq;
	}

	if (down_btn_irq = gpio_to_irq(DOWN_BTN) < 0) {
		pr_err("gpio to irq maping failed at %d\n", DOWN_BTN);
		return down_btn_irq;
	}

	if (request_irq(left_btn_irq, (irq_handler_t)button_irq_handle,
			IRQF_SHARED, "left button irq", &btn_left)) {
		pr_err("Request interrupt failed\n");
		return -1;
	}

	if (request_irq(right_btn_irq, (irq_handler_t)button_irq_handle,
			IRQF_SHARED, "right button irq", &btn_right)) {
		pr_err("Request interrupt failed\n");
		return -1;
	}

	if (request_irq(up_btn_irq, (irq_handler_t)button_irq_handle,
			IRQF_SHARED, "up button irq", &btn_up)) {
		pr_err("Request interrupt failed\n");
		return -1;
	}

	if (request_irq(down_btn_irq, (irq_handler_t)button_irq_handle,
			IRQF_SHARED, "down button irq", &btn_down)) {
		pr_err("Request interrupt failed\n");
		return -1;
	}

	ret = misc_register(&btn_dev);
	if (ret) {
		pr_err("can not register device\n");
		return ret;
	}

	pr_info("request successfully\n");

	return ret;
}

static void __exit button_exit(void)
{
	free_irq(left_btn_irq, NULL);
	free_irq(right_btn_irq, NULL);
	free_irq(up_btn_irq, NULL);
	free_irq(down_btn_irq, NULL);

	gpio_free(LEFT_BTN);
	gpio_free(RIGHT_BTN);
	gpio_free(UP_BTN);
	gpio_free(DOWN_BTN);

	pr_info("goodbye\n");
}

module_init(button_init);
module_exit(button_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tungnt58 + LongVH12");
MODULE_DESCRIPTION("This module to control button\n");
