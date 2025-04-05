/*
 * 7-bit I2C slave addresses:
 *  0x23 (ADDR pin low)
 *  0x5C (ADDR pin high)
 *
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/uaccess.h>

#define TIMEOUT 10

#define BH1750_POWER_DOWN		0x00
#define BH1750_ONE_TIME_H_RES_MODE	0x20 /* auto-mode for BH1721 */
#define BH1750_CHANGE_INT_TIME_H_BIT	0x40
#define BH1750_CHANGE_INT_TIME_L_BIT	0x60

DECLARE_WAIT_QUEUE_HEAD(wait_queue_etx);

struct bh1750_chip_info {
	u16 mtreg_min;
	u16 mtreg_max;
	u16 mtreg_default;
	int mtreg_to_usec;
	int mtreg_to_scale;
	int inc;
	u16 int_time_low_mask;
	u16 int_time_high_mask;
};

struct bh1750_device {
	struct i2c_client *i2c_dev;
    const struct bh1750_chip_info *chip_info;
    struct timer_list timer;
    u16 mtreg;
};

static struct bh1750_device *sensor_device;
static bool is_read = false;

static const struct bh1750_chip_info bh1750_chip_info = {
    .mtreg_min = 31,
    .mtreg_max = 254,
    .mtreg_default = 69,
    .mtreg_to_usec = 1740,
    .mtreg_to_scale = 57500000,
    .inc = 1,
    .int_time_low_mask = 0x001F,
    .int_time_high_mask = 0x00E0
};

static __poll_t bh1750_file_poll(struct file *filp, struct poll_table_struct *wait);
static int bh1750_file_open (struct inode *inodep, struct file *filep);
static int bh1750_file_close (struct inode *inodep, struct file *filep);
static ssize_t bh1750_file_read (struct file *filep, char __user *buf, 
                size_t size, loff_t *offset);

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = bh1750_file_open,
    .read           = bh1750_file_read,
    .poll           = bh1750_file_poll,
    .release        = bh1750_file_close
};

static struct miscdevice bh1750_miscdev = {
        .minor  = MISC_DYNAMIC_MINOR,
        .name   = "light_sensor",
        .fops   = &fops,
        .mode   = 0664,
};

static int bh1750_read(struct bh1750_device *data, int *val)
{
	int ret;
	__be16 result;
	const struct bh1750_chip_info *chip_info = data->chip_info;
	unsigned long delay = chip_info->mtreg_to_usec * data->mtreg;

	ret = i2c_smbus_write_byte(data->i2c_dev, BH1750_ONE_TIME_H_RES_MODE);
	if (ret < 0)
		return ret;

	usleep_range(delay + 15000, delay + 40000);

	ret = i2c_master_recv(data->i2c_dev, (char *)&result, 2);
	if (ret < 0)
		return ret;

	*val = be16_to_cpu(result);

	return 0;
}

static int bh1750_change_int_time(struct bh1750_device *data, int usec)
{
	int ret;
	u16 val;
	u8 regval;
	const struct bh1750_chip_info *chip_info = data->chip_info;

	if ((usec % chip_info->mtreg_to_usec) != 0)
		return -EINVAL;

	val = usec / chip_info->mtreg_to_usec;
	if (val < chip_info->mtreg_min || val > chip_info->mtreg_max)
		return -EINVAL;

	ret = i2c_smbus_write_byte(data->i2c_dev, BH1750_POWER_DOWN);
	if (ret < 0)
		return ret;

	regval = (val & chip_info->int_time_high_mask) >> 5;
	ret = i2c_smbus_write_byte(data->i2c_dev,
				   BH1750_CHANGE_INT_TIME_H_BIT | regval);
	if (ret < 0)
		return ret;

	regval = val & chip_info->int_time_low_mask;
	ret = i2c_smbus_write_byte(data->i2c_dev,
				   BH1750_CHANGE_INT_TIME_L_BIT | regval);
	if (ret < 0)
		return ret;

	data->mtreg = val;

	return 0;
}

static __poll_t bh1750_file_poll(struct file *filp, struct poll_table_struct *wait)
{
    __poll_t mask = 0;

    poll_wait(filp, &wait_queue_etx, wait);
    if (is_read) {
	    is_read = false;
	    mask |= ( POLLIN | POLLRDNORM);
    }
    return mask;
}

static int bh1750_file_open(struct inode *inodep, struct file *filep)
{
    return 0;
}

static int bh1750_file_close(struct inode *inodep, struct file *filep)
{
    return 0;
}

static ssize_t bh1750_file_read(struct file *filep, char __user *buf, 
                size_t size, loff_t *offset)
{
    int sensor_val, ret;

    ret = bh1750_read(sensor_device, &sensor_val);
    if (ret) {
        pr_err("DEBUG: Cannot read sensor data\n");
        return -1;
    }

    if (copy_to_user(buf, &sensor_val, sizeof(sensor_val)) > 0) {
	    pr_err("DEBUG: not all data copied to user\n");
    }
    
    return sizeof(sensor_val);
}

void timer_callback(struct timer_list * data)
{
    struct bh1750_device *sensor_dev = from_timer(sensor_dev, data, timer);

    is_read = true;
    wake_up(&wait_queue_etx);
    mod_timer(&sensor_dev->timer, jiffies + msecs_to_jiffies(TIMEOUT));
}

static int bh1750_probe(struct i2c_client *client)
{
    int ret, usec;
    struct bh1750_device *sensor_dev;

    pr_info("DEBUG: Jump to bh1750_probe function\n");

    sensor_dev = devm_kzalloc(&client->dev, sizeof(struct bh1750_device), 
                    GFP_KERNEL);
    if (!sensor_dev) {
        pr_err("Cannot allocate sensor_dev device\n");
        return -ENOMEM;
    }

    sensor_dev->chip_info = &bh1750_chip_info;
    sensor_dev->i2c_dev = client;
    sensor_device = sensor_dev;

    usec = sensor_dev->chip_info->mtreg_to_usec * sensor_dev->chip_info->mtreg_default;
    ret = bh1750_change_int_time(sensor_dev, usec);
    if (ret < 0)
		return ret;

    i2c_set_clientdata(client, sensor_dev);

    timer_setup(&sensor_dev->timer, timer_callback, 0);
    mod_timer(&sensor_dev->timer, jiffies + msecs_to_jiffies(TIMEOUT));

    ret = misc_register(&bh1750_miscdev);
    if (ret) {
        pr_info("misc_register failed\n");
        return ret;
    }

    return 0;
}

static int bh1750_remove(struct i2c_client *client)
{
    struct bh1750_device *sensor_dev = i2c_get_clientdata(client);

    del_timer(&sensor_dev->timer);
    misc_deregister(&bh1750_miscdev);

    return 0;
}

static const struct i2c_device_id bh1750_id[] = {
	{ "bh1750", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, bh1750_id);

static const struct of_device_id bh1750_of_match[] = {
	{ .compatible = "rohm,bh1750", },
	{ }
};
MODULE_DEVICE_TABLE(of, bh1750_of_match);

static struct i2c_driver bh1750_driver = {
	.driver = {
		.name = "bh1750",
		.of_match_table = bh1750_of_match,
		.owner = THIS_MODULE,
	},
	.probe_new = bh1750_probe,
	.remove = bh1750_remove,
	//.id_table = bh1750_id,

};
module_i2c_driver(bh1750_driver);

MODULE_AUTHOR("Tungnt");
MODULE_DESCRIPTION("bh1750 light sensor driver");
MODULE_LICENSE("GPL v2");
