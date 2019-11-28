#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>

#include "L3G4200D.h"

#define DEV_NAME		"l3g4200d"

static struct kobject *my_kobj;
static struct i2c_adapter *i2c_master;
static struct i2c_client *i2c_gyroscope;

static void convert_val(int *val)
{
	if (*val & (1 << 16) == 0)
		*val = *val;
	else {
		*val &= 0x7fff;
		*val *= -1;
	}
}

static ssize_t Xcor_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	int x;

	x = (i2c_smbus_read_byte_data(i2c_gyroscope, L3G4200D_OUT_X_H) & 0xff) << 8;
	x |= i2c_smbus_read_byte_data(i2c_gyroscope, L3G4200D_OUT_X_L) & 0xff;
	convert_val(&x);

	return sprintf(buf, "%d\n", x);
}

static ssize_t Xcor_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	pr_info("X store\n");
	return 0;
}

static ssize_t Ycor_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	int y;

	y = (i2c_smbus_read_byte_data(i2c_gyroscope, L3G4200D_OUT_Y_H) & 0xff) << 8;
	y |= i2c_smbus_read_byte_data(i2c_gyroscope, L3G4200D_OUT_Y_L) & 0xff;
	convert_val(&y);

	return sprintf(buf, "%d\n", y);
}
static ssize_t Ycor_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	pr_info("Y store\n");
	return 0;
}

static ssize_t Zcor_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	int z;

	z = (i2c_smbus_read_byte_data(i2c_gyroscope, L3G4200D_OUT_Z_H) & 0xff) << 8;
	z |= i2c_smbus_read_byte_data(i2c_gyroscope, L3G4200D_OUT_Z_L) & 0xff;
	convert_val(&z);

	return sprintf(buf, "%d\n", z);
}

static ssize_t Zcor_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	pr_info("Z store\n");
	return 0;
}

static struct kobj_attribute X_attr = __ATTR(X_val, 0644, Xcor_show, Xcor_store);
static struct kobj_attribute Y_attr = __ATTR(Y_val, 0644, Ycor_show, Ycor_store);
static struct kobj_attribute Z_attr = __ATTR(Z_val, 0644, Zcor_show, Zcor_store);

static struct attribute *gyro_attr[] = {
	&X_attr.attr,
	&Y_attr.attr,
	&Z_attr.attr,
	NULL,
};

static struct attribute_group gyro_attr_grp = {
	.attrs = gyro_attr,
};

static void l3g4200_init(struct i2c_client *i2c_dev, int scale)
{
	if (i2c_smbus_read_byte_data(i2c_dev, L3G4200D_WHO_AM_I) != 0xd3) {
		pr_err("l3g4200d busy\n");
		return;
	}

	i2c_smbus_write_byte_data(i2c_dev, L3G4200D_CTRL_REG1, 0x0f);
	i2c_smbus_write_byte_data(i2c_dev, L3G4200D_CTRL_REG2, 0x00);
	i2c_smbus_write_byte_data(i2c_dev, L3G4200D_CTRL_REG3, 0x08);

	if (scale == 250) {
		i2c_smbus_write_byte_data(i2c_dev, L3G4200D_CTRL_REG4, 0x00);
	} else if (scale == 500) {
		i2c_smbus_write_byte_data(i2c_dev, L3G4200D_CTRL_REG4, 0x10);
	} else {
		i2c_smbus_write_byte_data(i2c_dev, L3G4200D_CTRL_REG4, 0x30);
	}

	i2c_smbus_write_byte_data(i2c_dev, L3G4200D_CTRL_REG5, 0x00);
}

static int gyroscope_probe(struct i2c_client *i2c_dev, const struct i2c_device_id *i2c_dev_id)
{
	int ret;

	pr_info("Jump to probe function\n");
	if (!i2c_check_functionality(i2c_dev->adapter, I2C_FUNC_SMBUS_BYTE_DATA 
					| I2C_FUNC_SMBUS_BYTE)) {
		pr_err("i2c not support function\n");
		return -ENODEV;
	}

	l3g4200_init(i2c_dev, 2000);
	i2c_gyroscope = i2c_dev;

	my_kobj = kobject_create_and_add("gyroscope_val", kernel_kobj->parent);
	if (!my_kobj) {
		pr_err("can not create kobject\n");
		return -1;
	}

	pr_info("create successfully kobject\n");
	ret = sysfs_create_group(my_kobj, &gyro_attr_grp);
	if (ret) {
		pr_err("can not create sysfs\n");
		return ret;
	}

	pr_info("Create successfully sys file system\n");

	return 0;
}

static int gyroscope_remove(struct i2c_client *i2c_dev)
{
	sysfs_remove_group(my_kobj, &gyro_attr_grp);
	kobject_put(my_kobj);
	pr_info("%s is called\n", __func__);

	return 0;
}

static struct i2c_board_info gyroscope_info = {
	.type = "l3g4200d",
	.addr = 0x69,
};

static struct i2c_device_id gyroscope_id[] = {
	{"l3g4200d", 0},
	{/* NULL*/    },
};
MODULE_DEVICE_TABLE(i2c, gyroscope_id);

static struct i2c_driver gyro_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "l3g4200d",
	},
	.id_table = gyroscope_id,
	.probe = gyroscope_probe,
	.remove = gyroscope_remove,
};

static int __init gyroscope_init(void)
{
	int ret;

/*#ifdef CONFIG_I2C_BOARDINFO
	ret = i2c_register_board_info(0, &gyroscope_info, 0);
	if (ret < 0) {
		pr_info("can not register board info\n");
		goto exit1;
	}
#else*/
	i2c_master = i2c_get_adapter(1);
	if (i2c_master == NULL) {
		pr_err("Can not get i2c master\n");
		goto exit1;
	}

	i2c_gyroscope = i2c_new_device(i2c_master, &gyroscope_info);
	if (i2c_gyroscope == NULL) {
		pr_err("Can not add new device\n");
		goto exit2;
	}
//#endif
	pr_info("add new device successfully\n");

	ret = i2c_add_driver(&gyro_driver);
	if (ret < 0) {
		pr_err("can not register driver\n");
		goto exit3;
	}

	return 0;
exit3:
	i2c_unregister_device(i2c_gyroscope);
exit2:
	i2c_put_adapter(i2c_master);
exit1:
	return -ENODEV;

}

static void __exit gyroscope_exit(void)
{
	i2c_del_driver(&gyro_driver);
	i2c_unregister_device(i2c_gyroscope);
	pr_info("goodbye\n");
}

module_init(gyroscope_init);
module_exit(gyroscope_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tungnt58");
