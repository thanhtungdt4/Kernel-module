#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/reboot.h>
#include <linux/io.h>
#include <linux/pwm.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <linux/types.h>
#include <linux/mutex.h>

#define MAGIC_NO		100
#define SET_REBOOT_CMD		_IOW(MAGIC_NO, 0, char *)
#define MAX_DEVICE		5
#define BUF_SIZE		256

#define TRILED_REG_TYPE			0x04
#define TRILED_REG_SUBTYPE		0x05
#define TRILED_REG_EN_CTL		0x46
#define TRILED_EN_CTL_MAX_BIT		7
#define TRILED_TYPE					0x19

#define PWM_PERIOD_DEFAULT_NS		1000000

struct proc_dir_entry   *proc_dir;

typedef struct pwm_config {
	int enable;
	int duty_cycle;
} pwm_config_t;

#define CMD_LED_ON		_IOW(MAGIC_NO, 1, pwm_config_t)
#define CMD_LED_OFF		_IOW(MAGIC_NO, 2, pwm_config_t)

typedef enum pwm_enable {
	Disable,
	Enable,
} pwm_enable_t;

struct device_info {
	const char *name;
	const char *author;
	int version;
};

struct my_example_dev {
	dev_t			major;
	struct cdev		cdev;
	struct device		*mdevice;
	struct class		*mclass;
	struct platform_device	*pdev;
	struct regmap			*regmap;
	struct pwm_device		*pwm_dev;
	struct mutex			lock;
	struct device_info	dev_info;
	int 			array[5];
	u16				base_addr;
	u8				subtype;
};
#define to_my_dev(x) container_of(x, struct my_example_dev, cdev)

static int exam_config_pwm(struct my_example_dev *mdev, pwm_enable_t enable, u64 duty);

static int dev_open(struct inode *, struct file *);
static int dev_close(struct inode *, struct file *);
static long dev_ioctl(struct file *, unsigned int, unsigned long);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);

static struct device_info proc_dev_info;

/* function for sysfs */
static ssize_t sysfs_show(struct device *, struct device_attribute *,
				char *);

static struct file_operations dev_fops = {
	.open = dev_open,
	.release = dev_close,
	.unlocked_ioctl = dev_ioctl,
	.read = dev_read,
};

static int proc_open_ex(struct inode *inodep, struct file *filep)
{
	pr_info("open proc file\n");

	return 0;
}

static int proc_close(struct inode *inodep, struct file *filep)
{
	pr_info("Close proc file\n");
	return 0;
}

static ssize_t proc_read(struct file *filep, char __user *buf, size_t len, loff_t *ofset)
{
	//struct my_example_dev *mdev = to_my_dev_ops(filep->f_op);
	char kernel_buf[BUF_SIZE];
	int count = 0;

	memset(kernel_buf, 0, BUF_SIZE);
	if (*ofset > 0)
		return 0;
	count += sprintf(kernel_buf, "device name: %s\n", proc_dev_info.name);
	count += sprintf(kernel_buf + count, "author: %s\n", proc_dev_info.author);
	count += sprintf(kernel_buf + count, "version: %d\n", proc_dev_info.version);

	if (copy_to_user(buf, kernel_buf, count))
		return -EFAULT;
	*ofset = count;

	return count;
}

static struct file_operations proc_fops = {
	.open = proc_open_ex,
	.read = proc_read,
	.release = proc_close,
};

DEVICE_ATTR(example, S_IRUSR | S_IRGRP, sysfs_show, NULL);

static int dev_open(struct inode *inodep, struct file *filep)
{
	pr_info("Open file\n");

	return 0;
}

static int dev_close(struct inode *inodep, struct file *filep)
{
	pr_info("Close file\n");

	return 0;
}

static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	struct my_example_dev *mdev = to_my_dev(filep->f_inode->i_cdev);
	pwm_config_t pwm_config;
	void __user *argp = (void __user *)arg;
	char __user *user_cmd = argp;
	char buf[10];

	pr_info("device version: %d\n", mdev->dev_info.version);

	switch (cmd) {
	case SET_REBOOT_CMD:
		memset(buf, 0, sizeof(buf));
		copy_from_user(buf, user_cmd, sizeof(buf));
		pr_info("get from user: %s\n", buf);
		if (!strcmp(buf, "reboot"))
			kernel_restart(NULL);
		break;

	case CMD_LED_ON:
		copy_from_user(&pwm_config, argp, sizeof(pwm_config));
		pr_info("get duty_cycle from user: %d\n", pwm_config.duty_cycle);
		//exam_config_pwm(mdev, Enable, pwm_config.duty_cycle);
		exam_config_pwm(mdev, pwm_config.enable, pwm_config.duty_cycle);
		break;

	case CMD_LED_OFF:
		copy_from_user(&pwm_config, argp, sizeof(pwm_config));
		pr_info("get duty_cycle from user: %d\n", pwm_config.duty_cycle);
		//exam_config_pwm(mdev, Disable, pwm_config.duty_cycle);
		exam_config_pwm(mdev, pwm_config.enable, pwm_config.duty_cycle);
		break;
		
	default:
		return -ENOTTY;
	}

	return 0;
}

/*This function is for debug */
static ssize_t dev_read(struct file *filep, char *buf, size_t len,
				loff_t *ofset)
{
	struct my_example_dev *mdev = to_my_dev(filep->f_inode->i_cdev);

	pr_info("Turn on LED\n");
	exam_config_pwm(mdev, Enable, 500000);

	return 0;
}

static int tri_led_read(struct my_example_dev *mdev, u16 addr, u8 *val)
{
	int ret;
	unsigned int tmp;
	
	mutex_lock(&mdev->lock);
	ret = regmap_read(mdev->regmap, mdev->base_addr + addr, &tmp);
	if (ret < 0)
		pr_err("read addr Failed\n");
	else
		*val = (u8)tmp;
	mutex_unlock(&mdev->lock);
	
	return ret;
}

static int tri_led_write(struct my_example_dev *mdev, u16 addr, u8 mask, u8 val)
{
	int ret;
	
	mutex_lock(&mdev->lock);
	ret = regmap_update_bits(mdev->regmap, mdev->base_addr + addr, mask, val);
	if (ret < 0)
		pr_err("Update addr 0x%x to val 0x%x with mask 0x%x failed\n", addr, val, mask);
	
	mutex_unlock(&mdev->lock);
	
	return ret;
}

static int tri_led_init(struct my_example_dev *mdev)
{
	int ret = 0;
	u8 val;
	
	ret = tri_led_read(mdev, TRILED_REG_TYPE, &val);
	if (ret < 0) {
		pr_err("Read REG_TYPE failed\n");
		return ret;
	}
	
	if (val != TRILED_TYPE) {
		pr_err("invalid subtype(%d)\n", val);
		return -ENODEV;
	}
	
	ret = tri_led_read(mdev, TRILED_REG_SUBTYPE, &val);
	if (ret < 0) {
		pr_info("Read REG_SUBTYPE failed\n");
		return ret;
	}
	pr_info("subtype is %d\n", val);
	mdev->subtype = val;
	
	return 0;
}

static int exam_config_pwm(struct my_example_dev *mdev, pwm_enable_t enable, u64 duty)
{
	struct pwm_state state;
	u8 mask = 0, val = 0;
	int ret;
	
	pwm_get_state(mdev->pwm_dev, &state);
	state.enabled = enable;
	state.period = PWM_PERIOD_DEFAULT_NS;
	state.duty_cycle = duty;
	
	ret = pwm_apply_state(mdev->pwm_dev, &state);
	if (ret < 0) {
		pr_err("Apply PWM state Failed\n");
	}
	
	mask |= 1 << TRILED_EN_CTL_MAX_BIT;
	val = mask;
	
	ret = tri_led_write(mdev, TRILED_REG_EN_CTL, mask, val);
	if (ret < 0)
		pr_err("Update addr 0x%x failed\n", TRILED_REG_EN_CTL);
	
	return ret;
}

static ssize_t sysfs_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct my_example_dev *mdev = dev_get_drvdata(dev);
	int count = 0;
	int i, array_len;
	char kernel_buf[25];

	memset(kernel_buf, 0, 25);
	pr_info("Tungnt: device version: %d\n",mdev->dev_info.version);
	array_len = ARRAY_SIZE(mdev->array);
	for (i = 0; i <array_len; i++) {
		count += sprintf(kernel_buf + count, " %d\n", mdev->array[i]);
	}

	pr_info("kernel array: %s\n", kernel_buf);

	return sprintf(buf, kernel_buf, count);
}

static int my_example_probe(struct platform_device *pdev)
{
	int ret;
	struct my_example_dev *mdev;
	struct device_node *node = pdev->dev.of_node;
	//struct pwm_args pargs;
	const __be32 *addr;
	

	pr_info("Tungnt: Jump to probe func\n");

	mdev = devm_kzalloc(&pdev->dev, sizeof(struct my_example_dev),
				GFP_KERNEL);
	if (!mdev) {
		pr_err("malloc failed\n");
		ret = -ENOMEM;
		goto Failed;
	}

	mdev->pdev = pdev;
	mdev->regmap = dev_get_regmap(mdev->pdev->dev.parent, NULL);
	if (!mdev->regmap) {
		pr_err("Getting regmap failed\n");
		ret = -EINVAL;
		goto Failed;
	}

	addr = of_get_address(node, 0, NULL, NULL);
	if (!addr) {
		pr_err("Getting address failed\n");
		ret = -EINVAL;
		goto Failed;
	}
	mdev->base_addr = be32_to_cpu(addr[0]);
	mdev->pwm_dev = devm_pwm_get(&mdev->pdev->dev, "pwm-led");
	if (IS_ERR(mdev->pwm_dev)) {
		pr_err("Get pwd device failed\n");
		ret = PTR_ERR(mdev->pwm_dev);
		goto Failed;
	}
	
	ret = of_property_read_string(pdev->dev.of_node, "my_name",
			&mdev->dev_info.name);
	if (ret) {
		pr_err("Can not get name property\n");
		goto Failed;
	}
	pr_info("name is %s\n", mdev->dev_info.name);
	proc_dev_info.name = mdev->dev_info.name;

	ret = of_property_read_string(pdev->dev.of_node, "author",
			&mdev->dev_info.author);
	if (ret) {
		pr_err("Can not get author property\n");
		goto Failed;
	}
	pr_info("Author is %s\n", mdev->dev_info.author);
	proc_dev_info.author = mdev->dev_info.author;

	ret = of_property_read_u32(pdev->dev.of_node, "my_version",
			&mdev->dev_info.version);
	if (ret) {
		pr_err("Can not get version property\n");
		goto Failed;
	}
	pr_info("Version is %d\n", mdev->dev_info.version);
	proc_dev_info.version = mdev->dev_info.version;

	ret = of_property_read_u32_array(pdev->dev.of_node, "array",
			mdev->array, ARRAY_SIZE(mdev->array));
	if (ret) {
		pr_err("Can not get array property\n");
		goto Failed;
	}
	pr_info("second element of array is %d\n", mdev->array[1]);

	platform_set_drvdata(pdev, mdev);

	ret = alloc_chrdev_region(&mdev->major, 0, MAX_DEVICE, "example");
	if (ret < 0) {
		pr_err("Alloc char device failed\n");
		goto Failed;
	}
	pr_info("major is %df\n", mdev->major);

	cdev_init(&mdev->cdev, &dev_fops);
	ret = cdev_add(&mdev->cdev, mdev->major, 1);
	if (ret < 0) {
		pr_err("Failed to add cdev\n");
		goto Failed_cdev_add;
	}

	mdev->mclass = class_create(THIS_MODULE, "example");
	if (IS_ERR(mdev->mclass)) {
		ret = (int)PTR_ERR(mdev->mclass);
		pr_err("Failed to create class\n");
		goto Failed_create_class;
	}

	mdev->mdevice = device_create(mdev->mclass, NULL, mdev->major,
					NULL, "example");
	if (IS_ERR(mdev->mdevice)) {
		ret = (int)PTR_ERR(mdev->mdevice);
		pr_err("Failed to create device\n");
		goto Failed_create_device;
	}

	dev_set_drvdata(mdev->mdevice, mdev);
	ret = device_create_file(mdev->mdevice, &dev_attr_example);
	if (ret) {
		pr_err("Failed to create sysfs file\n");
		goto Failed_create_device;
	}
	pr_info("Create sysfs OK\n");

	/* Create procfs */
	proc_dir = proc_create("example", 0444, NULL, &proc_fops);
	if (proc_dir == NULL) {
		pr_err("create procfs failed\n");
		ret = -ENOMEM;
		goto Failed_create_sysfs;
	}
	pr_info("Create procfs OK\n");
	
	mutex_init(&mdev->lock);
	ret = tri_led_init(mdev);
	if (ret < 0) {
		pr_err("HW init failed\n");
		goto Failed;
	}

	return 0;

Failed_create_sysfs:
	device_remove_file(mdev->mdevice, &dev_attr_example);
Failed_create_device:
	class_destroy(mdev->mclass);
Failed_create_class:
	cdev_del(&mdev->cdev);
Failed_cdev_add:
	unregister_chrdev_region(mdev->major, 1);
Failed:
	return ret;

}

static int my_example_remove(struct platform_device *pdev)
{
	struct my_example_dev *mdev = platform_get_drvdata(pdev);

	device_remove_file(mdev->mdevice, &dev_attr_example);
	cdev_del(&mdev->cdev);
	device_destroy(mdev->mclass, 1);
	class_destroy(mdev->mclass);
	unregister_chrdev_region(mdev->major, 1);
	proc_remove(proc_dir);
	mutex_destroy(&mdev->lock);

	return 0;
}

static struct of_device_id my_example_table[] = {
	{ .compatible = "example-node"},
	{ /* NULL */ },
};
MODULE_DEVICE_TABLE(of, my_example_table);

static struct platform_driver my_example_driver = {
	.probe = my_example_probe,
	.remove = my_example_remove,
	.driver = {
		.name = "example",
		.of_match_table = my_example_table,
		.owner = THIS_MODULE,
	},
};
module_platform_driver(my_example_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tungnt");
MODULE_DESCRIPTION("example module");

