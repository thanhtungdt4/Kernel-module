#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/pwm.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#include "motor_ioctl.h"

#define MAX_DEVICE		4
#define MOTOR1_INPUT1		14
#define MOTOR1_INPUT2		15

#define MOTOR2_INPUT1		5
#define MOTOR2_INPUT2		6

static int dev_open(struct inode *, struct file *);
static int dev_close(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static long dev_ioctl(struct file *, unsigned int, unsigned long);

typedef enum pin_state {
	LOW,
	HIGHT
} pin_state_t;

void gpio_set_pin(int gpio, pin_state_t state);

struct motor_device {
	dev_t			major;
	int			motor1_input1;
	int			motor1_input2;
	int			motor2_input1;
	int			motor2_input2;
	char 			dev_name[6];
	int 			dev_id;
	struct cdev		cdev;
	struct device		*dev;
	struct class		*mclass;
	struct pwm_device	*pwm_dev;
	struct platform_device	*pdev;
};

#define to_motor_dev(x)		container_of(x, struct motor_device, cdev)

static struct file_operations fops = {
	.open = dev_open,
	.release = dev_close,
	.read = dev_read,
	.unlocked_ioctl = dev_ioctl,
};

static int dev_open(struct inode *inodep, struct file *filep)
{
	return 0;
}

static int dev_close(struct inode *inodep, struct file *filep)
{
	return 0;
}

static ssize_t dev_read(struct file *filep, char *buf, size_t len,
				loff_t *ofset)
{
	/*This function for debug purpose */
	struct motor_device *motor_dev = to_motor_dev(filep->f_inode->i_cdev);
	unsigned int duty;
	unsigned int period;
	struct pwm_state state;

	pwm_get_state(motor_dev->pwm_dev, &state);
	duty = state.duty_cycle;
	period = state.period;

	pr_info("duty circle is %u; period is %u, pwm_enable: %d\n", duty,
		 period, motor_dev->pwm_dev->state.enabled);

	pwm_config(motor_dev->pwm_dev, 5500000, 6000000);
	if (pwm_enable(motor_dev->pwm_dev)) {
		pr_err("Can not enable pwm device\n");
	}

	pr_info("pwm_enable: %d\n", motor_dev->pwm_dev->state.enabled);

	return 0;
}

static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	int duty_cycle;
	void __user *argp = (void __user *)arg;
	int *value = (int *)argp;
	struct motor_device *motor_dev = to_motor_dev(filep->f_inode->i_cdev);

	switch (cmd) {
	case SET_DUTY_CYCLE:
		get_user(duty_cycle, value);
		pr_info("duty cycle %d\n", duty_cycle);
		pwm_config(motor_dev->pwm_dev, duty_cycle, 6000000);
		pr_info("duty cycle is set: %d\n", motor_dev->pwm_dev->state.duty_cycle);
		break;

	case GO_STRAIGHT:
		if (motor_dev->dev_id == 1) {
			gpio_set_pin(motor_dev->motor1_input1, HIGHT);
			gpio_set_pin(motor_dev->motor1_input2, LOW);
		} else {
			gpio_set_pin(motor_dev->motor2_input1, HIGHT);
			gpio_set_pin(motor_dev->motor2_input2, LOW);
		}
		break;

	case GO_BACK:
		if (motor_dev->dev_id == 1) {
			gpio_set_pin(motor_dev->motor1_input1, LOW);
			gpio_set_pin(motor_dev->motor1_input2, HIGHT);
		} else {
			gpio_set_pin(motor_dev->motor2_input1, LOW);
			gpio_set_pin(motor_dev->motor2_input2, HIGHT);
		}
		break;

	case STOP_MOTOR:
		if (motor_dev->dev_id == 1) {
			gpio_set_pin(motor_dev->motor1_input1, LOW);
			gpio_set_pin(motor_dev->motor1_input2, LOW);
		} else {
			gpio_set_pin(motor_dev->motor2_input1, LOW);
			gpio_set_pin(motor_dev->motor2_input2, LOW);
		}
		break;

	case ENABLE_PWM_DEV:
		pwm_enable(motor_dev->pwm_dev);
		break;

	default:
		return -ENOTTY;
	}

	return 0;
}

void gpio_set_pin(int gpio, pin_state_t state)
{
	gpio_direction_output(gpio, 1);
	gpio_set_value(gpio, state);
}

int motor_probe(struct platform_device *pdev)
{
	int ret;
	struct motor_device *motor_dev;

	motor_dev = devm_kzalloc(&pdev->dev, sizeof(struct motor_device),
					GFP_KERNEL);
	motor_dev->pdev = pdev;
	motor_dev->motor1_input1 = MOTOR1_INPUT1;
	motor_dev->motor1_input2 = MOTOR1_INPUT2;
	motor_dev->motor2_input1 = MOTOR2_INPUT1;
	motor_dev->motor2_input2 = MOTOR2_INPUT2;
	of_property_read_u32(pdev->dev.of_node, "dev_id", &motor_dev->dev_id);

	pr_info("device id is %d\n", motor_dev->dev_id);

	sprintf(motor_dev->dev_name, "motor%d", motor_dev->dev_id);
        motor_dev->pwm_dev = devm_pwm_get(&pdev->dev, motor_dev->dev_name);
	if (IS_ERR(motor_dev->pwm_dev)) {
		pr_err("%s: Can not get pwm device\n", __func__);
		return PTR_ERR(motor_dev->pwm_dev);
	}

	pwm_config(motor_dev->pwm_dev, 3000000, 6000000);

	platform_set_drvdata(pdev, motor_dev);
	
	ret = alloc_chrdev_region(&motor_dev->major, 0, MAX_DEVICE, "motor");
	if (ret < 0) {
		pr_err("Alloc char device failed\n");
		goto Failed_alloc_chrdev;
	}

	pr_info("%s major is %d\n", __func__, motor_dev->major);

	cdev_init(&motor_dev->cdev, &fops);
	ret = cdev_add(&motor_dev->cdev, motor_dev->major, 1);
	if (ret < 0) {
		pr_err("Failed to add cdev\n");
		goto Failed_cdev_add;
	}

	motor_dev->mclass = class_create(THIS_MODULE, motor_dev->dev_name);
	if (IS_ERR(motor_dev->mclass)) {
		ret = (int)PTR_ERR(motor_dev->mclass);
		pr_err("Failed to create class\n");
		goto Failed_create_class;
	}

	motor_dev->dev = device_create(motor_dev->mclass, NULL,
			motor_dev->major, NULL, "motor%d", motor_dev->dev_id);
	if (IS_ERR(motor_dev->dev)) {
		ret = (int)PTR_ERR(motor_dev->dev);
		pr_err("Failed to create device\n");
		goto Failed_create_device;
	}

	if (motor_dev->dev_id == 1) {
		if (!gpio_is_valid(motor_dev->motor1_input1)) {
			pr_err("gpio pin %d is not available\n",
				motor_dev->motor1_input1);
			ret = -EIO;
			goto Failed_alloc_chrdev;
		}
		gpio_request(motor_dev->motor1_input1, "motor1 input1");

		if (!gpio_is_valid(motor_dev->motor1_input2)) {
			pr_err("gpio pin %d is not available\n",
				motor_dev->motor1_input2);
			ret = -EIO;
			goto Failed_alloc_chrdev;
		}
		gpio_request(motor_dev->motor1_input2, "motor1 input2");

		if (!gpio_is_valid(motor_dev->motor2_input1)) {
			pr_err("gpio pin %d is not available\n",
				motor_dev->motor2_input1);
			ret = -EIO;
			goto Failed_alloc_chrdev;
		}
		gpio_request(motor_dev->motor2_input1, "motor2 input1");

		if (!gpio_is_valid(motor_dev->motor2_input2)) {
			pr_err("gpio pin %d is not available\n",
				motor_dev->motor2_input2);
			ret = -EIO;
			goto Failed_alloc_chrdev;
		}
		gpio_request(motor_dev->motor2_input2, "motor2 input2");
	}

	pr_info("%s All successfully\n", __func__);

	pr_info("%s: name is: %s; hw pwm is %u, pwm device is %u\n", __func__,
		motor_dev->pwm_dev->label,
		motor_dev->pwm_dev->hwpwm,
		motor_dev->pwm_dev->pwm);

	return 0;

Failed_create_device:
	class_destroy(motor_dev->mclass);
Failed_create_class:
	cdev_del(&motor_dev->cdev);
Failed_cdev_add:
	unregister_chrdev_region(motor_dev->major, 1);
Failed_alloc_chrdev:
	return ret;

}

int motor_remove(struct platform_device *pdev)
{
	struct motor_device *motor_dev = platform_get_drvdata(pdev);

	cdev_del(&motor_dev->cdev);
	device_destroy(motor_dev->mclass, 1);
	class_destroy(motor_dev->mclass);
	unregister_chrdev_region(motor_dev->major, 1);

	if (motor_dev->dev_id == 1) {
		gpio_free(motor_dev->motor1_input1);
		gpio_free(motor_dev->motor1_input2);
		gpio_free(motor_dev->motor2_input1);
		gpio_free(motor_dev->motor2_input2);
	}

	return 0;
}

static struct of_device_id motor_of_match[] = {
	{ .compatible = "pwm-motor1-control" },
	{ .compatible = "pwm-motor2-control" },
	{ /*NULL*/ },
};
MODULE_DEVICE_TABLE(of, motor_of_match);

static struct platform_driver motor_driver = {
	.probe = motor_probe,
	.remove = motor_remove,
	.driver = {
		.name = "pwm-motor",
		.of_match_table = motor_of_match,
		.owner = THIS_MODULE,
	},
};

module_platform_driver(motor_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tungnt@fih-foxconn.com");
MODULE_DESCRIPTION("This driver to control DC motor");
