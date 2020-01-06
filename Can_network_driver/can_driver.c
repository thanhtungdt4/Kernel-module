#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "MCP2515.h"

struct my_can_device {
	struct spi_device *spi_dev;
	struct file_operation fops;
	struct miscdevice my_dev;
};

#define to_my_can_device(x)	container_of(x, struct my_can_device, fops)

static int dev_open(struct inode *, struct file *);
static int dev_close(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static int dev_probe(struct spi_device *spi)
{
	struct my_can_device *can_dev;

	can_dev = devm_kzalloc(&spi->dev, sizeof(struct my_can_device),
				GFP_KERNEL);
	can_dev->spi_dev = spi;

	can_dev->fops.open = dev_open;
	can_dev->fops.release = dev_close;
	can_dev->fops.read = dev_read;
	can_dev->fops.write = dev_write;

	can_dev->my_dev.name = "mcp2515";
	can_dev->my_dev.minor =  MISC_DYNAMIC_MINOR;
	can_dev->my_dev.fops = &can_dev->fops;
	spi_set_drvdata(spi, can_dev);

	misc_register(&can_dev->my_dev);

	return 0;
}

static int dev_remove(struct spi_device *spi)
{
	struct my_can_device *can_dev = spi_get_drvdata(spi);

	misc_deregister(&can_dev->my_dev);
	return 0;
}

static of_device_id can_of_match[] = {
	{ .compatible = "mcp,mcp2515"
	{ /*NULL*/ },
};
MODULE_DEVICE_TABLE(of, can_of_match);

static struct spi_driver can_driver = {
	.probe = dev_probe,
	.remove = dev_remove,
	.driver = {
		.name = "mcp2515",
		.of_match_table = can_of_match,
		.owner = THIS_MODULE,
	},
};
module_spi_driver(can_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tungnt<thanhtungdt4.haui@gmail.com>");
MODULE_DESCRIPTION("Driver for module can mcp2515");
