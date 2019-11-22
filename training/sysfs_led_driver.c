#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/uaccess.h>


MODULE_AUTHOR("TUNG");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("THIS MODULE IS TO CONTROL LED IN RASPI 3 BOARD");

static int value;
static char *mesg;

static struct kobject *val_obj;
static ssize_t val_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t val_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

static ssize_t direct_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t direct_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

static struct kobj_attribute val_kobj_attr = __ATTR(value, 0664, val_show, val_store);
static struct kobj_attribute direct_kobj_attr = __ATTR(direction, 0664, direct_show, direct_store);

static struct attribute *attr[] = {
	&val_kobj_attr.attr,
	&direct_kobj_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attr,
};

static ssize_t val_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "nguyen thanh tung\n");
}

static ssize_t val_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	printk("get value in function: %s\n", __func__);
	sscanf(buf, "%d", &value);
	printk("get from user: %s", mesg );
	return count;
}

static ssize_t direct_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	sprintf(buf, "nguyen huy nam\n");
	return sizeof("nguyen huy name");;
}

static ssize_t direct_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	return 0;
}

static int __init exam_init(void)
{
	int ret;
	val_obj = kobject_create_and_add("test_file", kernel_kobj->parent);
	if(val_obj == NULL)
	{
		printk(KERN_ALERT "can not create kboject\n");
		return -ENOMEM;
	}
	ret = sysfs_create_group(val_obj, &attr_group);
	if(ret)
	{
		printk(KERN_ALERT "can not create file\n");
	}
	return ret;
}

static void __exit exam_exit(void)
{
	kobject_put(val_obj);
	printk("goodbye\n");
}


module_init(exam_init);
module_exit(exam_exit);
