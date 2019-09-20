#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/spi/spi.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>

#include "L3G4200D.h"

/*Max speed of l3g4200d*/
#define L3G4200D_MAX_CLK		10000000
#define BIT_PER_WORD			8

typedef unsigned char	byte;

static struct spi_device *spi_device;

struct spi_l3g4200d {
	struct spi_device *sdev;
	struct proc_dir_entry *proc_entry;
	struct mutex lock;
	struct file_operations fops;
};

#define to_l3g4200d(x) container_of(x, struct spi_l3g4200d, fops)

static byte read_register(struct spi_device *spi, byte address)
{
	byte data, msg = 0x00;

	address |= 0x80; /*This tells the L3G4200D we're reading*/
	spi_write(spi, &address, sizeof(address));
	spi_write(spi, &msg, sizeof(msg));
	spi_read(spi, &data, sizeof(data));

	return data;
}

static void write_register(struct spi_device *spi, byte address, byte data)
{
	address &= 0x7F; /* This tell the L3G4200D we're writing*/

	spi_write(spi, &address, sizeof(address));
	spi_write(spi, &data, sizeof(data));
}

static int setup_L3g4200d(struct spi_device *spi, byte fullscale)
{
	if (read_register(spi, L3G4200D_WHO_AM_I) != 0xD3)
		return -1;

	write_register(spi, L3G4200D_CTRL_REG1, 0x0f);
	write_register(spi, L3G4200D_CTRL_REG2, 0x00);
	write_register(spi, L3G4200D_CTRL_REG3, 0x08);

	fullscale &= 0x03;
	write_register(spi, L3G4200D_CTRL_REG4, fullscale << 4);
	write_register(spi, L3G4200D_CTRL_REG5, 0x00);

	return 0;
}

static int get_gyro_Xvalue(struct spi_device *spi)
{
	int x;

	x = (read_register(spi, L3G4200D_OUT_X_H) & 0xFF) << 8;
	x |= read_register(spi, L3G4200D_OUT_X_L) & 0xFF;

	return x;
}

static int get_gyro_Yvalue(struct spi_device *spi)
{
        int y;

        y = (read_register(spi, L3G4200D_OUT_Y_H) & 0xFF) << 8;
        y |= read_register(spi, L3G4200D_OUT_Y_L) & 0xFF;

        return y;
}

static int get_gyro_Zvalue(struct spi_device *spi)
{
        int z;

        z = (read_register(spi, L3G4200D_OUT_Z_H) & 0xFF) << 8;
        z |= read_register(spi, L3G4200D_OUT_Z_L) & 0xFF;

        return z;
}

int seq_file_write(struct seq_file *m, void *data)
{
	int x, y, z;

	struct spi_l3g4200d *gyro_spi = (struct spi_l3g4200d *)data;
	x = get_gyro_Xvalue(gyro_spi->sdev);
	y = get_gyro_Yvalue(gyro_spi->sdev);
	z = get_gyro_Zvalue(gyro_spi->sdev);
	seq_printf(m, "X = %d\n", x);
	pr_info("X = %d\n", x);

	seq_printf(m, "Y = %d\n", y);
	pr_info("Y = %d\n", y);

	seq_printf(m, "Z = %d\n", z);
	pr_info("Z = %d\n", z);	

	return 0;
}

static int seq_file_open(struct inode *inodep, struct file *filep)
{
	struct spi_l3g4200d *gyro_spi;

	gyro_spi = to_l3g4200d(filep->f_op);
	setup_L3g4200d(gyro_spi->sdev, 2);

	mutex_lock(&gyro_spi->lock);
	single_open(filep, seq_file_write, gyro_spi);
	mutex_unlock(&gyro_spi->lock);

	return 0;
}

static struct spi_board_info l3g4200d_info = {
	.modalias = "l3g4200d",
	.chip_select = 0,
	.max_speed_hz = L3G4200D_MAX_CLK,
	.bus_num = 0,
	.mode = SPI_MODE_3,
};

static int l3g4200d_probe(struct spi_device *spi)
{
	struct spi_l3g4200d *gyro_spi;

	gyro_spi = kmalloc(sizeof(struct spi_l3g4200d), GFP_KERNEL);
	gyro_spi->sdev = spi;
	gyro_spi->fops.owner = THIS_MODULE;
	gyro_spi->fops.open = seq_file_open;
	gyro_spi->fops.release = seq_release;
	gyro_spi->fops.read = seq_read;

	gyro_spi->proc_entry = proc_create("l3g4200d", 0664, NULL, &gyro_spi->fops);
	if (gyro_spi->proc_entry == NULL) {
		pr_err("can not create proc file system\n");
		return -ENOMEM;
	}

	mutex_init(&gyro_spi->lock);

	spi_set_drvdata(spi, gyro_spi);
	pr_info("%s function is called\n", __func__);

	return 0;
}

static int l3g4200d_remove(struct spi_device *spi)
{
	struct spi_l3g4200d *gyro_spi = spi_get_drvdata(spi);
	proc_remove(gyro_spi->proc_entry);
	mutex_destroy(&gyro_spi->lock);
	kfree(gyro_spi);
	pr_info("%s function is called\n", __func__);

	return 0;
}

static struct spi_device_id l3g4200d_idtable[] = {
        {"l3g4200d", 0},
        { },
};
MODULE_DEVICE_TABLE(spi, l3g4200d_idtable);

static struct spi_driver l3g4200d_driver = {
	.driver = {
			.name = "l3g4200d",
	},
	.id_table = l3g4200d_idtable,
	.probe = l3g4200d_probe,
	.remove = l3g4200d_remove,
};

static int __init l3g4200d_init(void)
{
	int ret;

	/*ret = spi_register_board_info(l3g4200d_info, ARRAY_SIZE(l3g4200d_info));
	if (ret) {
		pr_err("Can not register board info\n");
		return ret;
	}*/

	struct spi_controller *master;

	master = spi_busnum_to_master(l3g4200d_info.bus_num);
	if (master == NULL) {
		pr_err("%s Failed\n", __func__);
		return -ENODEV;
	}

	spi_device = spi_new_device(master, &l3g4200d_info);
	if (!spi_device) {
		pr_err("can not add new device\n");
		return -ENODEV;
	}

	spi_device->bits_per_word = BIT_PER_WORD;
	ret = spi_setup(spi_device);
	if (ret) {
		spi_unregister_device(spi_device);
		return -ENODEV;
	}

	ret = spi_register_driver(&l3g4200d_driver);
	if (ret) {
		pr_err("Can not register spi driver\n");
		return ret;
	}
	pr_info("Register driver successfully\n");

	return 0;
}

static void __exit l3g4200d_exit(void)
{
	spi_unregister_driver(&l3g4200d_driver);
	spi_unregister_device(spi_device);
	pr_info("goodbye\n");
}

module_init(l3g4200d_init);
module_exit(l3g4200d_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("tungnt58@fsoft.com.vn");
MODULE_DESCRIPTION("This module to get gyroscope sensor value"); 
