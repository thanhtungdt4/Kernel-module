#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/serdev.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#define CMD		"AT+VERSION?"

struct hc06_bluetooth {
	int data;
	dev_t			major;
	struct cdev 		cdev;
	struct device 		*device;
	struct class 		*class;
	struct serdev_device 	*sdev;
};

#define to_bluetooth_dev(x) container_of(x, struct hc06_bluetooth, cdev)

static int dev_open(struct inode *inodep, struct file *filep)
{
	pr_info("open file\n");

	return 0;
}

static ssize_t dev_write(struct file *filep, const char *buf, size_t len,
				loff_t *ofset)
{
	int ret;
	struct hc06_bluetooth *hc06 = to_bluetooth_dev(filep->f_inode->i_cdev);
	char message[20];

	ret = copy_from_user(message, buf, len);
	if (ret) {
		pr_err("can not copy from user\n");
	}
	pr_info("get from user: %s\n", message);

	ret = serdev_device_write(hc06->sdev, message, sizeof(message), 2*HZ);
	pr_info("ret = %d\n", ret);

	return len;
}

static struct file_operations fops = {
	.open = dev_open,
	.write = dev_write,
};

static int hc06_receive_buf(struct serdev_device *sdev,
			const unsigned char *data, size_t count)
{
	pr_info("Jump to %s function\n", __func__);
	pr_info("read from hc06: %s\n", data);

	return 0;
}

static struct serdev_device_ops hc06_bluetooth_ops = {
	.receive_buf = hc06_receive_buf,
	.write_wakeup = serdev_device_write_wakeup,
};

static int hc06_bluetooth_probe(struct serdev_device *sdev)
{
	struct hc06_bluetooth *hc06;
	int ret;

	hc06 = devm_kzalloc(&sdev->dev, sizeof(struct hc06_bluetooth),
				GFP_KERNEL);
	if (!hc06)
		return -ENOMEM;

	hc06->data = 1;
	hc06->sdev = sdev;
	serdev_device_set_drvdata(sdev, hc06);

	pr_info("Tung: bus type is %s\n", sdev->ctrl->dev.bus->name);

	ret = serdev_device_open(sdev);
	if (ret) {
		pr_err("[Tung]: Failed to open serial device\n");
		goto Failed_alloc_chrdev;
	}

	pr_info("[Tung]: open success device number %d on serdev bus\n", sdev->nr);
	pr_info("[Tung]: Number id bus %d\n", sdev->ctrl->nr);
	serdev_device_set_baudrate(sdev, 9600);
	serdev_device_set_flow_control(sdev, false);
	serdev_device_set_client_ops(sdev, &hc06_bluetooth_ops);

	ret = serdev_device_write(sdev, CMD, sizeof(CMD), 2*HZ);
	if (ret < 0)
		pr_err("Tung: Failed to send command\n");
	pr_info("Tung: ret is %d\n", ret);

	ret = alloc_chrdev_region(&hc06->major, 0, 1, "hc06-bluetooth");
	if (ret < 0) {
		pr_err("Failed to alloc chrdev\n");
		goto Failed_alloc_chrdev;
	}

	cdev_init(&hc06->cdev, &fops);
	ret = cdev_add(&hc06->cdev, hc06->major, 1);
	if (ret < 0) {
		pr_err("Failed to add cdev\n");
		goto Faled_cdev_add;
	}

	hc06->class = class_create(THIS_MODULE, "hc06-bluetooth");
	if (IS_ERR(hc06->class)) {
		ret = (int)PTR_ERR(hc06->class);
		pr_err("Failed to create class\n");
		goto Failed_create_class;
	}

	hc06->device = device_create(hc06->class, NULL, hc06->major,
				NULL, "hc06-bluetooth");
	if (IS_ERR(hc06->device)) {
		ret = (int)PTR_ERR(hc06->device);
		pr_err("Failed to create device\n");
		goto Failed_create_device;
	}

	return 0;

Failed_create_device:
	class_destroy(hc06->class);
Failed_create_class:
	cdev_del(&hc06->cdev);
Faled_cdev_add:
	unregister_chrdev_region(hc06->major, 1);
Failed_alloc_chrdev:
	return ret;
}

static void hc06_bluetooth_remove(struct serdev_device *sdev)
{
	struct hc06_bluetooth *hc06;

	hc06 = serdev_device_get_drvdata(sdev);
	serdev_device_close(sdev);
	cdev_del(&hc06->cdev);
	device_destroy(hc06->class, 1);
	class_destroy(hc06->class);
	unregister_chrdev_region(hc06->major, 1);
}

static struct of_device_id hc06_bluetooth_table[] = {
	{ .compatible = "CSR,hc06-bluetooth" },
	{ /* NULL */},
};
MODULE_DEVICE_TABLE(of, hc06_bluetooth_table);

static struct serdev_device_driver hc06_bluetooth_driver = {
	.probe = hc06_bluetooth_probe,
	.remove = hc06_bluetooth_remove,
	.driver = {
		.name = "hc06-bluetooth",
		.of_match_table = of_match_ptr(hc06_bluetooth_table),
	},
};

module_serdev_device_driver(hc06_bluetooth_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tungnt");
MODULE_DESCRIPTION("Serial driver for hc06 bluetooth module");
