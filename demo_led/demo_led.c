#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>

struct demo_led_device {
	struct platform_device *pdev;
	struct device *dev;
	struct gpio_desc *led;
	struct mutex lock;
};

static ssize_t demo_led_show(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	int val;
	int size;
	struct demo_led_device *led_dev = dev_get_drvdata(dev);

	mutex_lock(&led_dev->lock);
	val = gpiod_get_value(led_dev->led);
	mutex_unlock(&led_dev->lock);
	pr_info("Debug: %s val->%d\n", __func__, val);
	size = sprintf(buf, "%d\n", val);
	return size;

}

static ssize_t demo_led_store(struct device *dev, struct device_attribute *attr,
					 const char *buf, size_t count)
{
	int val;
	struct demo_led_device *led_dev = dev_get_drvdata(dev);

	sscanf(buf, "%d", &val);
	pr_info("Debug: %s val->%d\n", __func__, val);
	mutex_lock(&led_dev->lock);
	gpiod_set_value(led_dev->led, val);
	mutex_unlock(&led_dev->lock);
	return count;
}

DEVICE_ATTR(demo_led, 0664, demo_led_show, demo_led_store);

static int demo_led_probe(struct platform_device *pdev)
{
	int ret;
	struct demo_led_device *led_dev;

	pr_info("Jump to %s\n", __func__);

	led_dev = devm_kzalloc(&pdev->dev, sizeof(struct demo_led_device),
				GFP_KERNEL);
	if (!led_dev) {
		pr_err("devm_kzalloc: fails\n");
		ret = -ENOMEM;
		goto Failed;
	}
	mutex_init(&led_dev->lock);
	led_dev->pdev = pdev;
	led_dev->dev = &pdev->dev;
	led_dev->led = gpiod_get_index(led_dev->dev, "demoled", 0,
			GPIOD_OUT_LOW);
	if (!led_dev->led) {
		pr_err("gpiod_get_index: Cannot get gpiod\n");
		ret = -ENODEV;
		goto Failed;
	}
	platform_set_drvdata(pdev, led_dev);
	dev_set_drvdata(led_dev->dev, led_dev);
	ret = device_create_file(led_dev->dev, &dev_attr_demo_led);
	if (ret) {
		pr_err("Failed to create sysfs file\n");
		ret = -1;
		goto Failed;
	}
	return 0;
Failed:
	return ret;
}

static int demo_led_remove(struct platform_device *pdev)
{
	struct demo_led_device *led_dev = platform_get_drvdata(pdev);

	gpiod_set_value(led_dev->led, 0);
	device_remove_file(led_dev->dev, &dev_attr_demo_led);
	gpiod_put(led_dev->led);
	mutex_destroy(&led_dev->lock);
	return 0;
}

static struct of_device_id demo_led_table[] = {
	{ .compatible = "raspi4,demo_led"},
	{ /* NULL */ },
};
MODULE_DEVICE_TABLE(of, demo_led_table);

static struct platform_driver demo_led_driver = {
	.probe = demo_led_probe,
	.remove = demo_led_remove,
	.driver = {
		.name = "demo_led",
		.of_match_table = demo_led_table,
		.owner = THIS_MODULE,
	},
};
module_platform_driver(demo_led_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thanhtungdt4");
MODULE_DESCRIPTION("demo led driver");

