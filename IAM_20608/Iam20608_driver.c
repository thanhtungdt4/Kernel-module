#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

struct Iam20680_device {
	struct cdev             cdev;
	struct i2c_client       *i2c_client;
        struct device           *device;
        struct class            *class;

	int                     major;
        uint16_t                accel_x;
        uint16_t                accel_y;
        uint16_t                accel_z;
        uint16_t                gyro_x;
        uint16_t                gyro_y;
        uint16_t                gyro_z; 	
};

#define to_Iam20680_device(x) container_of(x, struct Iam20680_device, cdev)

static int Iam20680_open(struct inode*, struct file*);
static int Iam20680_close(struct inode*, struct file*);
static ssize_t Iam20680_read(struct file*, char*, size_t, loff_t*);
static ssize_t Iam20680_write(struct file*, const char*, size_t, loff_t*);

static struct file_operations fops = {
	.open = Iam20680_open,
	.release =  Iam20680_close,
	.read = Iam20680_read,
	.write = Iam20680_write,
};

static int Iam20680_open(struct inode* inodep, struct file* filep)
{
	return 0;
}

static int Iam20680_close(struct inode* inodep, struct file* filep)
{
	return 0;
}

static ssize_t Iam20680_read(struct file* filep, char *buf, size_t len,
				loff_t *offset)
{
	struct Iam20680_device *dev = to_Iam20680_device(filep->f_inode->i_cdev);


	return 0;
}

static ssize_t Iam20680_write(struct file* filep, const char  *buf, size_t len,
				loff_t *offset)
{
	struct Iam20680_device *dev = to_Iam20680_device(filep->f_inode->i_cdev);

	return 0;
}

static ssize_t AccelX_Show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct Iam20680_device *mdev = dev_get_drvdata(dev);

	return 0;
}

static ssize_t AccelY_Show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct Iam20680_device *mdev = dev_get_drvdata(dev);

	return 0;
}

static ssize_t AccelZ_Show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct Iam20680_device *mdev = dev_get_drvdata(dev);

	return 0;
}

DEVICE_ATTR(Accel_X, S_IRUSR | S_IRGRP, AccelX_Show, NULL);
DEVICE_ATTR(Accel_Y, S_IRUSR | S_IRGRP, AccelY_Show, NULL);
DEVICE_ATTR(Accel_Z, S_IRUSR | S_IRGRP, AccelZ_Show, NULL);

static struct attribute *Accel_attrs[] = {
	&dev_attr_Accel_X.attr,
	&dev_attr_Accel_Y.attr,
	&dev_attr_Accel_Z.attr,
	NULL
};

static const struct attribute_group Accel_attr_group = {
	.name  = "Accel",
	.attrs = Accel_attrs,
};

static ssize_t GyroX_Show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct Iam20680_device *mdev = dev_get_drvdata(dev);
	
	return 0;
}

static ssize_t GyroY_Show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct Iam20680_device *mdev = dev_get_drvdata(dev);

	return 0;
}

static ssize_t GyroZ_Show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct Iam20680_device *mdev = dev_get_drvdata(dev);

	return 0;
}

DEVICE_ATTR(Gyro_X, S_IRUSR | S_IRGRP, GyroX_Show, NULL);
DEVICE_ATTR(Gyro_Y, S_IRUSR | S_IRGRP, GyroY_Show, NULL);
DEVICE_ATTR(Gyro_Z, S_IRUSR | S_IRGRP, GyroZ_Show, NULL);

static struct attribute *Gyro_attrs[] = {
	&dev_attr_Gyro_X.attr,
	&dev_attr_Gyro_Y.attr,
	&dev_attr_Gyro_Z.attr,
	NULL
};                                                                              
                                                                                
static const struct attribute_group Gyro_attr_group = {
	.name  = "Gyro",
	.attrs = Gyro_attrs,
};                                              

static const struct attribute_group *Attribute_Group[] = {
	&Accel_attr_group,
	&Gyro_attr_group
};

static int Iam20680_probe(struct i2c_client *client)
{
	int ret;
	struct Iam20680_device *dev;
	dev = devm_kzalloc(&client->dev, sizeof(struct Iam20680_device),
				GFP_KERNEL);
	pr_info("Tungnt: Jump to probe function\n");
	if (dev == NULL) {
		pr_err("Can not allocate device\n");
		ret = -ENOMEM;
		goto Failed;
	}

	/* Initialize value for struct element */
	dev->accel_x = 0;
	dev->accel_y = 0;
	dev->accel_z = 0;
	dev->gyro_x = 0;
	dev->gyro_y = 0;
	dev->gyro_z = 0;
	dev->i2c_client = client;
	i2c_set_clientdata(client, dev);

	ret = alloc_chrdev_region(&dev->major, 0, 1, "IAM20680");
	if (ret < 0) {
		pr_err("Failed to alloc char device\n");
		goto Failed;
	}

	pr_info("Tungnt: major is %d\n", dev->major);

	cdev_init(&dev->cdev, &fops);
	ret = cdev_add(&dev->cdev, dev->major, 1);
	if (ret < 0) {
		pr_err("Failed to add cdev device\n");
		goto Failed_cdev_add;
	}

	dev->class = class_create(THIS_MODULE, "IAM20680");
	if (IS_ERR(dev->class)) {
		ret = (int)PTR_ERR(dev->class);
		pr_err("Failed to create class\n");
		goto Failed_create_class;
	}

	dev->device = device_create(dev->class, NULL, dev->major, NULL,
					"IAM20680");
	dev_set_drvdata(dev->device, dev);

	if (IS_ERR(dev->device)) {
		ret = (int)PTR_ERR(dev->device);
		pr_err("Failed to create device\n");
		goto Failed_create_device;
	}

	ret = sysfs_create_groups(&dev->device->kobj, Attribute_Group);
	if  (ret) {
		pr_err("sysfs create Failed\n");
		goto Failed;
	}

	if (i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK |
				I2C_FUNC_SMBUS_BYTE_DATA) == 0) {
		pr_err("i2c not support function\n");
		ret = -ENODEV;
		goto Failed;
	}

	pr_info("Tungnt: Probe function success\n");
	return 0;

Failed_create_device:
	class_destroy(dev->class);
Failed_create_class:
	cdev_del(&dev->cdev);
Failed_cdev_add:
	unregister_chrdev_region(dev->major, 1);
Failed:
	return ret;
}

static int Iam20680_remove(struct i2c_client *client)
{
	struct Iam20680_device *dev = i2c_get_clientdata(client);

	sysfs_remove_groups(&dev->device->kobj, Attribute_Group);
	cdev_del(&dev->cdev);
	device_destroy(dev->class, 1);
	class_destroy(dev->class);
	unregister_chrdev_region(dev->major, 1);

	return 0;
}

static const struct i2c_device_id IAM20680_id[] = {
	{ "IAM20680", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, IAM20680_id);

static struct of_device_id IAM20680_of_match[] = {
	{ .compatible = "invensense,iam20680" },
	{ /* NULL */ },
};
MODULE_DEVICE_TABLE(of, IAM20680_of_match);

static struct i2c_driver Iam20608_driver = {
	.probe_new = Iam20680_probe,
	.remove = Iam20680_remove,
	.id_table = IAM20680_id,
	.driver = {
		.name = "IAM20680",
		.owner = THIS_MODULE,
		.of_match_table = IAM20680_of_match,
	},
};
module_i2c_driver(Iam20608_driver);

MODULE_AUTHOR("Thanhtungdt4@gmail.com");
MODULE_VERSION("1.1");
