#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "MCP2515.h"
#include "CANSPI.h"

static uCAN_MSG txMessage;
static uCAN_MSG rxMessage;

struct my_can_device {
	int debug;
	struct spi_device *spi_dev;
	struct file_operations fops;
	struct miscdevice my_dev;
};

#define to_my_can_device(x)	container_of(x, struct my_can_device, fops)

static int dev_open(struct inode *, struct file *);
static int dev_close(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static int dev_open(struct inode *inodep, struct file *filep)
{
	struct my_can_device *can_dev = to_my_can_device(filep->f_op);

	pr_info("Tung: debug %d\n", can_dev->debug);
	pr_info("open file\n");

	return 0;
}

static int dev_close(struct inode *inodep, struct file *filep)
{
	pr_info("close file\n");

	return 0;
}

static ssize_t dev_read(struct file *filep, char *buf, size_t len,
			loff_t *offset)
{
	struct my_can_device *can_dev = to_my_can_device(filep->f_op);

	if (CANSPI_Receive(can_dev->spi_dev, &rxMessage)) {
		if (copy_to_user(buf, &rxMessage, sizeof(rxMessage))) {
			pr_err("Can not copy to user space\n");
		}
		pr_info("data0 is %d\n", rxMessage.frame.data0);

		return sizeof(rxMessage);
	}

	return 0;
}

static ssize_t dev_write(struct file *filep, const char *buf, size_t len,
			loff_t *offset)
{
	struct my_can_device *can_dev = to_my_can_device(filep->f_op);

	txMessage.frame.idType = dSTANDARD_CAN_MSG_ID_2_0B;
	txMessage.frame.id = 0x0A;
	txMessage.frame.dlc = 8;
	txMessage.frame.data0 = 0;
	txMessage.frame.data1 = 1;
	txMessage.frame.data2 = 2;
	txMessage.frame.data3 = 3;
	txMessage.frame.data4 = 4;
	txMessage.frame.data5 = 5;
	txMessage.frame.data6 = 6;
	txMessage.frame.data7 = 7;

	CANSPI_Transmit(can_dev->spi_dev, &txMessage);

	return 0;
}

static int dev_probe(struct spi_device *spi)
{
	struct my_can_device *can_dev;

	can_dev = devm_kzalloc(&spi->dev, sizeof(struct my_can_device),
				GFP_KERNEL);
	pr_info("Tung: spi mode is %d\n", spi->mode);
	can_dev->spi_dev = spi;

	can_dev->debug = 1;
	can_dev->fops.open = dev_open;
	can_dev->fops.release = dev_close;
	can_dev->fops.read = dev_read;
	can_dev->fops.write = dev_write;

	can_dev->my_dev.name = "mcp2515";
	can_dev->my_dev.minor =  MISC_DYNAMIC_MINOR;
	can_dev->my_dev.fops = &can_dev->fops;
	spi_set_drvdata(spi, can_dev);

	misc_register(&can_dev->my_dev);

	/*Initialize Can device*/
	if (!CANSPI_Initialize(spi, CAN_500KBPS, MCP_16MHz)) {
		pr_err("Can not init CanSPI\n");
	}

	return 0;
}

static int dev_remove(struct spi_device *spi)
{
	struct my_can_device *can_dev = spi_get_drvdata(spi);

	misc_deregister(&can_dev->my_dev);
	return 0;
}

static struct of_device_id can_of_match[] = {
	{ .compatible = "mcp,mcp2515" },
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
