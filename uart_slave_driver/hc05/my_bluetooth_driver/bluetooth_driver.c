#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/serdev.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

struct bluetooth_device {
	bool                 has_data;
	dev_t                major;
	wait_queue_head_t    wait_queue;
	struct mutex         lock;
	struct cdev          cdev;
	struct device        *device;
	struct class         *class;
	struct serdev_device *sdev;
};

#define to_bluetooth_dev(x) container_of(x, struct bluetooth_device, cdev)

static __poll_t bt_device_poll(struct file *filep, struct poll_table_struct *wait)
{
	__poll_t mask = 0;
	struct bluetooth_device *btdev = to_bluetooth_dev(filep->f_inode->i_cdev);

	poll_wait(filep, &btdev->wait_queue, wait);
	if (btdev->has_data) {
		btdev->has_data = false;
		mask |= POLLIN | POLLRDNORM;
	}

	return mask;
}

static ssize_t bt_device_read(struct file *filep, char __user *buf, 
	size_t size, loff_t *offset)
{
	return 0;
}

static ssize_t bt_device_write(struct file *filep, const char *buf, size_t len,
				loff_t *ofset)
{
	int ret;
	struct bluetooth_device *btdev = to_bluetooth_dev(filep->f_inode->i_cdev);
	char *message;

	message = kmalloc(len, GFP_KERNEL);
	if (!message)
		return -ENOMEM;

	ret = copy_from_user(message, buf, len);
	if (ret) {
		pr_err("can not copy from user\n");
		kfree(message);
		return -EFAULT;
	}
	pr_info("get from user: %s\n", message);

	ret = serdev_device_write(btdev->sdev, message, sizeof(message), 3*HZ);
	pr_info("ret = %d\n", ret);

	kfree(message);

	return len;
}

static struct file_operations fops = {
	.write = bt_device_write,
	.read  = bt_device_read,
	.poll  = bt_device_poll
};

static int bt_device_receive_buf(struct serdev_device *sdev,
			const unsigned char *data, size_t count)
{
	struct bluetooth_device *btdev = serdev_device_get_drvdata(sdev);

	pr_info("Jump to %s function\n", __func__);
	pr_info("raw msg: %.*s\n", (int)count, data);
	//print_hex_dump(KERN_INFO, "btdata: ", DUMP_PREFIX_OFFSET, 16, 1, data, count, true);

	if (count > 1) {
		char *temp = kzalloc(count, GFP_KERNEL);
		if (!temp)
			return -ENOMEM;
		memcpy(temp, data, count);
		temp[count] = '\0';
		pr_info("Get message: %s\n", temp);
		kfree(temp);
	}

	serdev_device_write_flush(sdev);

	return count;
}

static struct serdev_device_ops bluetooth_device_ops = {
	.receive_buf = bt_device_receive_buf,
	.write_wakeup = serdev_device_write_wakeup,
};

static int bluetooth_device_probe(struct serdev_device *sdev)
{
	struct bluetooth_device *btdev;
	int baudrate;
	int ret;

	btdev = devm_kzalloc(&sdev->dev, sizeof(struct bluetooth_device),
				GFP_KERNEL);
	if (!btdev)
		return -ENOMEM;

	if (of_property_read_u32(sdev->dev.of_node, "current-speed", &baudrate) == 0) {
		pr_info("[Tung] baudrate: %d\n", baudrate);
	}

	btdev->has_data = false;
	btdev->sdev = sdev;
	serdev_device_set_drvdata(sdev, btdev);

	pr_info("Tung: bus type is %s\n", sdev->ctrl->dev.bus->name);

	ret = serdev_device_open(sdev);
	if (ret) {
		pr_err("[Tung]: Failed to open serial device\n");
		goto Failed_alloc_chrdev;
	}

	init_waitqueue_head(&btdev->wait_queue);
	mutex_init(&btdev->lock);

	pr_info("[Tung]: open success device number %d on serdev bus\n", sdev->nr);
	pr_info("[Tung]: Number id bus %d\n", sdev->ctrl->nr);
	serdev_device_set_baudrate(sdev, baudrate);
	serdev_device_set_flow_control(sdev, false);
	serdev_device_set_client_ops(sdev, &bluetooth_device_ops);

	ret = alloc_chrdev_region(&btdev->major, 0, 1, "bt_device");
	if (ret < 0) {
		pr_err("Failed to alloc chrdev\n");
		goto Failed_alloc_chrdev;
	}

	cdev_init(&btdev->cdev, &fops);
	ret = cdev_add(&btdev->cdev, btdev->major, 1);
	if (ret < 0) {
		pr_err("Failed to add cdev\n");
		goto Faled_cdev_add;
	}

	btdev->class = class_create(THIS_MODULE, "bt_device");
	if (IS_ERR(btdev->class)) {
		ret = (int)PTR_ERR(btdev->class);
		pr_err("Failed to create class\n");
		goto Failed_create_class;
	}

	btdev->device = device_create(btdev->class, NULL, btdev->major,
				NULL, "bt_device");
	if (IS_ERR(btdev->device)) {
		ret = (int)PTR_ERR(btdev->device);
		pr_err("Failed to create device\n");
		goto Failed_create_device;
	}

	return 0;

Failed_create_device:
	class_destroy(btdev->class);
Failed_create_class:
	cdev_del(&btdev->cdev);
Faled_cdev_add:
	unregister_chrdev_region(btdev->major, 1);
Failed_alloc_chrdev:
	return ret;
}

static void bluetooth_device_remove(struct serdev_device *sdev)
{
	struct bluetooth_device *btdev;
	btdev = serdev_device_get_drvdata(sdev);

	mutex_destroy(&btdev->lock);
	serdev_device_close(sdev);
	device_destroy(btdev->class, btdev->major);
	class_destroy(btdev->class);
	cdev_del(&btdev->cdev);
	unregister_chrdev_region(btdev->major, 1);
}

static struct of_device_id bluetooth_device_table[] = {
	{ .compatible = "csr,hc06-bluetooth" },
	{ .compatible = "csr,hc05-bluetooth"},
	{ /* NULL */},
};
MODULE_DEVICE_TABLE(of, bluetooth_device_table);

static struct serdev_device_driver bluetooth_device_driver = {
	.probe = bluetooth_device_probe,
	.remove = bluetooth_device_remove,
	.driver = {
		.name = "bt_device",
		.of_match_table = of_match_ptr(bluetooth_device_table),
	},
};

module_serdev_device_driver(bluetooth_device_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tungnt");
MODULE_DESCRIPTION("Serial driver for bluetooth module");
