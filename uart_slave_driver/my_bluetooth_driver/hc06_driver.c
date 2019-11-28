#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/serdev.h>
#include <linux/of.h>

struct hc06_bluetooth {
	int data;
	struct serdev_device *sdev;
};

static int hc06_receive_buf(struct serdev_device *sdev,
			const unsigned char *data, size_t count)
{
	pr_info("Jump to %s function\n", __func__);
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

	hc06->sdev = sdev;
	serdev_device_set_drvdata(sdev, hc06);

	ret = serdev_device_open(sdev);
	if (ret) {
		pr_err("[Tung]: Failed to open serial device\n");
		return ret;
	}

	pr_info("[Tung]: open success device number %d on serdev bus\n", sdev->nr);
	pr_info("[Tung]: Number id bus %d\n", sdev->ctrl->nr);
	serdev_device_set_baudrate(sdev, 9600);
	serdev_device_set_flow_control(sdev, false);
	serdev_device_set_client_ops(sdev, &hc06_bluetooth_ops);

	return 0;
}

static void hc06_bluetooth_remove(struct serdev_device *sdev)
{
	struct hc06_bluetooth *hc06;

	hc06 = serdev_device_get_drvdata(sdev);
	serdev_device_close(sdev);
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
