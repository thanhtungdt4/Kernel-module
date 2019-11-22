#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/init.h>
#include <linux/sysfs.h>
#include <linux/io.h>

#define GPIO_ADDR_BASE			0x3F000000
#define GPIO_PIN			17
#define GPIOFSEL1			0x4
#define GPIO_SET0			0x1c
#define GPIO_CLR0			0x28
#define GPIO_LVL0			0x34

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TUNG");
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("THIS MODULE TO CONTROL LED OF RASPI 3");

static void __iomem *gpio_base;
static int value_get_from_user = 0;
static char message[10] = {0};

static struct kobject *kobj;

static int get_value_pin(void)
{
	return (*((int __iomem*)(gpio_base + GPIO_LVL0))) & (1 << GPIO_PIN);
}

static int get_direction(void)
{
	/*return 0 is input, 1 is output*/
	return (*((int __iomem*)(gpio_base + GPIOFSEL1))) & (1 << 21);
}

static void set_pin_mode(int mode)
{
	int __iomem *set_mode = (int __iomem*)(gpio_base + GPIOFSEL1);
	/*mode = 1 set output, mode = 0 set input*/
	if(!mode)
	{
		*set_mode = (*set_mode) & (~(7 << 21) | (0 << 21));
	}
	*set_mode = (*set_mode) & (~(7 << 21) | (1 <<21));
}

static void set_pin_value(int value)
{
	int __iomem * set_high = (int __iomem *)(gpio_base + GPIO_SET0);
	int __iomem * set_low = (int __iomem *)(gpio_base + GPIO_CLR0);
	/*value = 1 set high, value = 0 set low*/
	if(value)
	{
		*set_high = 1 << GPIO_PIN;
	}
	*set_low = 1 << GPIO_PIN;
}


static ssize_t val_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return 0;
}
static ssize_t val_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	printk(KERN_INFO "Val_store function is called\n");
	sscanf(buf, "%d", &value_get_from_user);
	printk(KERN_INFO "get from user: %d\n", value_get_from_user);
	if(get_direction())
	{
		if(value_get_from_user == 1);
	}
        return count;
}

static ssize_t direction_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
        return 0;
}

static ssize_t direction_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	printk(KERN_INFO "direction store function is called\n");
	sscanf(buf, "%s", message);
	printk(KERN_INFO "get from user: %s\n", message);
        return count;
}

static struct kobj_attribute val_kobj_attr = __ATTR(value, 0644, val_show, val_store);
static struct kobj_attribute dir_kobj_attr = __ATTR(direction, 0644, direction_show, direction_store);

static struct attribute *attr[] = {
	&val_kobj_attr.attr,
	&dir_kobj_attr.attr,
	NULL,
};
static struct attribute_group attr_group = {
	.attrs = attr,
};

static int __init exam_init(void)
{
	int ret;
	printk(KERN_INFO "Call from init function\n");
	/*gpio_base = ioremap(GPIO_ADDR_BASE, 1024);
	if(gpio_base == NULL)
	{
		printk(KERN_ALERT "Can not map to to virtual addr\n");
		return -ENOMEM;
	}*/
	kobj = kobject_create_and_add("my_gpio_module", kernel_kobj->parent);
	if(kobj == NULL)
	{
		printk(KERN_ALERT "can not create kobject\n");
		return -1;
	}
	printk(KERN_INFO "create successfully kobject\n");
	ret = sysfs_create_group(kobj, &attr_group);
	if(ret)
	{
		printk(KERN_ALERT "can not create group files\n");
		kobject_put(kobj);
	}
	printk(KERN_INFO "created group file\n");
	return ret;
}

static void __exit exam_exit(void)
{
	printk(KERN_INFO "goodbye\n");
	kobject_put(kobj);
	//iounmap(gpio_base);
	printk(KERN_INFO "exit\n");
}

module_init(exam_init);
module_exit(exam_exit);
