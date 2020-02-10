#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>

#include "mcp_can_dfs.h"

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
	uint8_t can_len = 0;
	uint8_t can_buf[8];
	uint8_t canid;
	struct my_can_device *can_dev = to_my_can_device(filep->f_op);

	if (CAN_MSGAVAIL == checkReceive(can_dev->spi_dev)) {
		if (readMsgBuf(can_dev->spi_dev, &can_len, can_buf) == CAN_NOMSG) {
			pr_err("No Can message\n");
		
			if (copy_to_user(buf, can_buf, sizeof(can_buf))) {
				pr_err("Can not copy to user space\n");
			}
		}
		canid = getCanId();
		pr_info("data0 is %d, canid is %d\n", can_buf[0], canid);
		mcp2515_modifyRegister(can_dev->spi_dev, MCP_CANINTF, 0x01,
					0x00);
		mcp2515_modifyRegister(can_dev->spi_dev, MCP_CANINTF, 0x02,
					0x00);

		return sizeof(can_len);
	}

	return 0;
}

static ssize_t dev_write(struct file *filep, const char *buf, size_t len,
			loff_t *offset)
{
	struct my_can_device *can_dev = to_my_can_device(filep->f_op);

	uint8_t stmp[8] = {1, 1, 2, 3, 0, 5, 6, 7};

	sendMsgBuf(can_dev->spi_dev, 0x43, 0, 0, 8, stmp, true);
	return 0;
}

static irqreturn_t can_irq_handle(int irq, void *data)
{
	uint8_t can_len =0;
	uint8_t can_buf[8];
	uint8_t canid;
	uint8_t status;
	struct my_can_device *can_dev = (struct my_can_device *)data;

	pr_info("Tung: Jump to interrupt handler: debug %d\n", can_dev->debug);

	if (CAN_MSGAVAIL == checkReceive(can_dev->spi_dev)) {
		if (readMsgBuf(can_dev->spi_dev, &can_len, can_buf) == CAN_NOMSG) {
			pr_err("No Can message\n");
		}
		canid = getCanId();
		pr_info("data0 is %d, canid is %d\n", can_buf[0], canid);
	}

	status = readRxTxStatus(can_dev->spi_dev);
	if (status & MCP_RX0IF) {
		/*clear RX0 interrupt flags*/
		mcp2515_modifyRegister(can_dev->spi_dev, MCP_CANINTF, 0x01,
					0x00);
	} else if (status & MCP_RX1IF) {
		/* clear Rx1 interrupt flags*/
		mcp2515_modifyRegister(can_dev->spi_dev, MCP_CANINTF, 0x02,
					0x00);
	}

	return IRQ_HANDLED;
}

static int dev_probe(struct spi_device *spi)
{
	struct my_can_device *can_dev;
	int ret;

	can_dev = devm_kzalloc(&spi->dev, sizeof(struct my_can_device),
				GFP_KERNEL);
	pr_info("Tung: spi mode is %d\n", spi->mode);
	pr_info("Tung: interrupt number is %d\n", spi->irq);
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
	if (can_begin(spi, CAN_100KBPS, MCP_16MHz) == CAN_FAILINIT) {
		pr_err("Can not init CanSPI\n");
	} else
		pr_info("Can init sucessfully\n");

	ret = devm_request_irq(&spi->dev, spi->irq,
				(irq_handler_t)can_irq_handle,
				IRQF_TRIGGER_FALLING,
				"can driver", can_dev);
	if (ret) {
		pr_err("Tung: Can not request interrupt\n");
		return ret;
	}
	pr_info("Tung: Request irq OK\n");

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
