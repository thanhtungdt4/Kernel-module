#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/delay.h>

#include "Tea5767_ioctl.h"

#define I2C_TRY_COUNT		4

typedef unsigned char 	byte;

struct Tea5767_device {
	uint32_t _lvl;
	uint32_t _rdy;
	uint32_t _sel;
	uint32_t _staCnt;
	uint32_t _stations[20];
	int major;

	byte _freqH;
	byte _freqL;

	bool _muted;
	bool _search;
	bool _up;
	bool _stby;
	bool _snc;
	bool _stereo;

	struct cdev	cdev;
	struct i2c_client *dev_client;
	struct device *device;
	struct class *class;
};

#define to_Tea5767_device(x)	container_of(x, struct Tea5767_device, cdev)

static int Tea5767_open(struct inode *, struct file *);
static int Tea5767_close(struct inode *, struct file *);
static long Tea5767_ioctl(struct file *, unsigned int, unsigned long);

static struct file_operations fops = {
	.open = Tea5767_open,
	.release = Tea5767_close,
	.unlocked_ioctl = Tea5767_ioctl,
};

static void Tea5767_i2c_send(struct i2c_client *client)
{
	struct Tea5767_device *dev;
	int i, try = I2C_TRY_COUNT;
	byte data[5];
	s32 ret;

	dev = i2c_get_clientdata(client);

	pr_info("slave addr: %d\n", client->addr);
	memset(data, 0, 5);

	data[0] = (dev->_muted << 7) | (dev->_search << 6) | dev->_freqH;
	data[1] = dev->_freqL;
	data[2] = (dev->_up << 7) | (dev->_lvl & 0x3 << 5) | 0x10;
	data[3] = 0x10 | (dev->_stby << 6);
	data[4] = 0x00;

	for (i = 0; i < 5; i++) {
		pr_info("data write: data[%d]: %d\n", i, data[i]);
	}

	do
		ret = i2c_smbus_write_i2c_block_data(client, client->addr, 5,
							data);
	while ((ret == -ENXIO || ret == -EIO) && --try);
	if (ret) {
		pr_err("Can not write full data to device\n");
	}

	/*check i2c write block ok or not */
	memset(data, 0, 5);
	i2c_smbus_read_i2c_block_data(client, client->addr, 5, data);
	for (i = 0; i < 5; i++) {
		pr_info("Tea5767_i2c_send: data[%d]: %d\n", i, data[i]);
	}
}

static void Tea5767_i2c_get(struct i2c_client *client)
{
	struct Tea5767_device *dev;
	byte data[5];
	bool rf, blf;
	s32 ret;
	int try = I2C_TRY_COUNT;

	dev = i2c_get_clientdata(client);
	memset(data, 0, 5);
	do

		ret = i2c_smbus_read_i2c_block_data(client, client->addr, 5,
							data);
	while ((ret == -ENXIO || ret == -EIO) && --try);
	if (ret != 5) {
		pr_err("Can not read full data from device\n");
	}

	rf = data[0] & 0x80;
	blf = data[0] & 0x40;
	if (!rf)
		dev->_rdy = 0;
	else if (rf && (!blf))
		dev->_rdy = 1;
	else
		dev->_rdy = 2;
	dev->_freqH = data[0] & 0x3F;
	dev->_freqL = data[1];
	dev->_stereo = data[2] & 0x80;
	dev->_lvl = data[3] >> 4;

	pr_info("Tungnt: FreqH: %d, FreqL: %d, stereo: %d, lvl: %d\n", dev->_freqH,
			dev->_freqL, dev->_stereo, dev->_lvl);
}

static int Tea5767_setFrequency(struct i2c_client *client, uint32_t frequency)
{
	struct Tea5767_device *dev = i2c_get_clientdata(client);
	uint32_t freqC;

	if(frequency < 87500000 || frequency > 108000000)
		return -1;
	freqC = (frequency + 225000) / 8192;
	dev->_freqH = (freqC >> 8);
	dev->_freqL = freqC & 0XFF;
	Tea5767_i2c_send(client);

	return 0;
}

static uint32_t Tea5767_findStations(struct i2c_client *client, uint32_t minlvl)
{
	struct Tea5767_device *dev = i2c_get_clientdata(client);
	uint32_t llvl = 0;
	uint32_t lsta = 0;
	uint32_t curfreq;
	dev->_staCnt = 0;

	for (curfreq = 87500000; curfreq <= 108000000; curfreq += 100000) {
		Tea5767_setFrequency(client, curfreq);
		msleep(50);
		Tea5767_i2c_get(client);
		if (dev->_lvl >= minlvl && dev->_stereo) {
			dev->_stations[dev->_staCnt] = curfreq;
			if (lsta >= (curfreq - 300000)) {
				if (llvl <= dev->_lvl)
				    dev->_stations[dev->_staCnt - 1] = curfreq;
			} else
				dev->_staCnt++;
			lsta = curfreq;
			llvl = dev->_lvl;
		}
	}

	pr_info("Tungnt: station number: %d\n", dev->_staCnt);
	if (dev->_staCnt > 0)
		Tea5767_setFrequency(client, dev->_stations[0]);

	return dev->_staCnt;
}

static uint32_t Tea5767_init(struct i2c_client *client, uint32_t minlvl)
{
	struct Tea5767_device *dev = i2c_get_clientdata(client);

	Tea5767_findStations(client, 12);
	return dev->_staCnt;
}

static void Tea5767_setMuted(struct i2c_client *client, bool muted)
{
	struct Tea5767_device *dev = i2c_get_clientdata(client);

	dev->_muted = muted;
	Tea5767_i2c_send(client);
}

/*static bool Tea5767_setSearch(struct i2c_client *client, bool up, 
				uint32_t level)
{
	struct Tea5767_device *dev = i2c_get_clientdata(client);

	dev->_up = up;
	if(level < 1 || level > 3)
		return false;
	dev->_lvl = level;
	dev->_search = true;
	Tea5767_i2c_send(client);
	dev->_search = false;

	return true;
}*/

static void Tea5767_setStandby(struct i2c_client *client, bool stby)
{
	struct Tea5767_device *dev = i2c_get_clientdata(client);

	dev->_stby = stby;
	Tea5767_i2c_send(client);
}

static void Tea5767_setStereoNC(struct i2c_client *client, bool snc)
{
	struct Tea5767_device *dev = i2c_get_clientdata(client);

	dev->_snc = snc;
	Tea5767_i2c_send(client);
}

static uint32_t Tea5767_getFrequency(struct i2c_client *client)
{
	struct Tea5767_device *dev = i2c_get_clientdata(client);
	uint32_t freqI;

	Tea5767_i2c_get(client);
	freqI = (dev->_freqH << 8) | dev->_freqL;

	return (freqI * 8192 - 225000);
}

static uint32_t Tea5767_getReady(struct i2c_client *client)
{
	struct Tea5767_device *dev = i2c_get_clientdata(client);

	Tea5767_i2c_get(client);
	return dev->_rdy;
}

static bool Tea5767_isStereo(struct i2c_client *client)
{
	struct Tea5767_device *dev = i2c_get_clientdata(client);

	Tea5767_i2c_get(client);
	return dev->_stereo;
}

static bool Tea5767_isMuted(struct i2c_client *client)
{
	struct Tea5767_device *dev = i2c_get_clientdata(client);

	return dev->_muted;
}

static uint32_t Tea5767_getStations(struct i2c_client *client)
{
	struct Tea5767_device *dev = i2c_get_clientdata(client);

	return dev->_staCnt;
}

static void Tea5767_nextStation(struct i2c_client *client)
{
	struct Tea5767_device *dev = i2c_get_clientdata(client);
	uint32_t sta;

	if (dev->_staCnt  == 0)
		return;
	else if (dev->_staCnt == 1)
		sta = dev->_stations[0];
	else {
		dev->_sel++;
		if (dev->_sel >= dev->_staCnt)
			dev->_sel = 0;
		sta = dev->_stations[dev->_sel];
	}

	Tea5767_setFrequency(client, sta);
}

static void printStation(struct i2c_client *client)
{
	uint32_t i, stations;
	uint32_t freq;

	stations = Tea5767_getStations(client);
	for (i = 0; i < stations; i++) {
		msleep(50);
		freq = Tea5767_getFrequency(client);
		pr_info("Frequency station %d: %d\n\t", i, freq);
		Tea5767_nextStation(client);
	}
}


static int Tea5767_open(struct inode *inodep, struct file *filep)
{
	struct Tea5767_device *dev = to_Tea5767_device(filep->f_inode->i_cdev);

	pr_info("open file\n");
	pr_info("station number: %d\n", dev->_staCnt);

	return 0;
}

static int Tea5767_close(struct inode *inodep, struct file *filep)
{
	pr_info("close file\n");

	return 0;
}

static long Tea5767_ioctl(struct file *filep, unsigned int cmd, 
				unsigned long arg)
{
	struct Tea5767_device *dev = to_Tea5767_device(filep->f_inode->i_cdev);
	void __user *argp = (void __user*)arg;
	uint32_t *int_val = (uint32_t *)argp;
	bool *bool_val = (bool *)argp;
	uint32_t freq_from_user, freq_to_user, ready_to_user;
	uint32_t minlvl, stations_num, ret;
	bool muted, stby, snc;
	bool muted_to_user, snc_to_user;

	switch (cmd) {
	case SET_FREQUENCY:
		get_user(freq_from_user, int_val);
		pr_info("frequency from user: %d\n", freq_from_user);
		ret = Tea5767_setFrequency(dev->dev_client, freq_from_user);
		if (ret) {
			pr_info("Can not set frequency\n");
		}
		put_user(ret, int_val);
		break;

	case FIND_STATIONS:
		get_user(minlvl, int_val);
		pr_info("minlvl from user: %d\n", minlvl);
		Tea5767_findStations(dev->dev_client, minlvl);
		break;

	case SET_MUTED:
		get_user(muted, bool_val);
		pr_info("muted from user: %d\n", (uint32_t)muted);
		Tea5767_setMuted(dev->dev_client, muted);
		break;

	case SET_STANDBY:
		get_user(stby, bool_val);
		pr_info("stby from user: %d\n", (uint32_t)stby);
		Tea5767_setStandby(dev->dev_client, stby);
		break;

	case SET_STEREO_NC:
		get_user(snc, bool_val);
		pr_info("snc from user: %d\n", (uint32_t)snc);
		Tea5767_setStereoNC(dev->dev_client, snc);
		break;

	case GET_FREQUENCY:
		freq_to_user = Tea5767_getFrequency(dev->dev_client);
		put_user(freq_to_user, int_val);
		break;

	case GET_READY:
		ready_to_user = Tea5767_getReady(dev->dev_client);
		put_user(ready_to_user, int_val);
		break;

	case IS_MUTED_GET:
		muted_to_user = Tea5767_isMuted(dev->dev_client);
		put_user(muted_to_user, bool_val);
		break;

	case GET_STATIONS:
		stations_num = Tea5767_getStations(dev->dev_client);
		put_user(stations_num, int_val);
		break;

	case IS_STEREO:
		snc_to_user = Tea5767_isStereo(dev->dev_client);
		put_user(snc_to_user, bool_val);
		break;

	case NEXT_STATIONS:
		Tea5767_nextStation(dev->dev_client);
		break;

	default:
		return -ENOTTY;
	}

	return 0;
}

static int Tea5767_probe(struct i2c_client *client)
{
	int ret;
	struct Tea5767_device *dev;

	dev = devm_kzalloc(&client->dev, sizeof(struct Tea5767_device),
				GFP_KERNEL);
	dev->_lvl = 2;
	dev->_sel = 0;
	dev->_staCnt = 0;
	dev->_freqH = 0x00;
	dev->_freqL = 0x00;
	dev->_muted = false;
	dev->_search = false;
	dev-> _up = true;
	dev->_stby = false;
	dev->_snc = true;
	dev->dev_client = client;

	i2c_set_clientdata(client, dev);

	pr_info("Jump to probe function\n");
	ret = alloc_chrdev_region(&dev->major, 0, 1, "Tea5767-i2c");
	if (ret < 0) {
		pr_err("Failed to alloc char device\n");
		goto Failed_alloc_chrdev;
	}
	pr_info("in probe func: major is %d\n", dev->major);

	cdev_init(&dev->cdev, &fops);
	ret = cdev_add(&dev->cdev, dev->major, 1);
	if (ret < 0) {
		pr_err("Failed to add cdev device\n");
		goto Faled_cdev_add;
	}

	dev->class = class_create(THIS_MODULE, "Tea5767-i2c");
	if (IS_ERR(dev->class)) {
		ret = (int)PTR_ERR(dev->class);
		pr_err("Failed to create class\n");
		goto Failed_create_class;
	}

	dev->device = device_create(dev->class, NULL, dev->major, NULL,
					"Tea5767-i2c");
	if (IS_ERR(dev->device)) {
		ret = (int)PTR_ERR(dev->device);
		pr_err("Failed to create device\n");
		goto Failed_create_device;
	}

	if (i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK) == 0) {
		pr_err("i2c not support function\n");
		ret = -ENODEV;
		goto Failed_alloc_chrdev;
	}

	pr_info("Init radio and find available stations\n");
	Tea5767_init(client, 14);
	printStation(client);
	
	return 0;

Failed_create_device:
	class_destroy(dev->class);
Failed_create_class:
	cdev_del(&dev->cdev);
Faled_cdev_add:
	unregister_chrdev_region(dev->major, 1);
Failed_alloc_chrdev:
	return ret;
}

static int Tea5767_remove(struct i2c_client *client)
{
	struct Tea5767_device *dev = i2c_get_clientdata(client);

	cdev_del(&dev->cdev);
	device_destroy(dev->class, 1);
	class_destroy(dev->class);
	unregister_chrdev_region(dev->major, 1);
	return 0;
}

static const struct i2c_device_id Tea5767_device_id[] = {
	{ "Tea5767-i2c", 0},
	{ },
};
MODULE_DEVICE_TABLE(i2c, Tea5767_device_id);

static struct of_device_id Tea5767_of_match[] = {
	{ .compatible = "Tea5767,Tea5767-i2c" },
	{ /* NULL */ },
};
MODULE_DEVICE_TABLE(of, Tea5767_of_match);

static struct i2c_driver Tea5767_driver = {
	.probe_new = Tea5767_probe,
//	.probe = Tea5767_probe,
	.remove = Tea5767_remove,
//	.id_table = Tea5767_device_id,
	.driver = {
		.name = "Tea5767-i2c",
		.of_match_table = Tea5767_of_match,
		.owner = THIS_MODULE,
	},
};
module_i2c_driver(Tea5767_driver);

MODULE_AUTHOR("tungnt");
MODULE_VERSION("1.1");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RADIO TEA5767 DRIVER");
