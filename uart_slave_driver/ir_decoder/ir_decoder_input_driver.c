#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/serdev.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>

#define NUMBER_OF_KEY           6
#define BACK_KEY                "back"
#define ENTER_KEY               "Enter"
#define UP_KEY                  "up"
#define DOWN_KEY                "down"
#define LEFT_KEY                "left"
#define RIGHT_KEY               "right"

struct key_info {
	const char *name;
	int key_event;
	unsigned int key_code;
};

struct ir_decoder_input_dev {
	struct input_dev            *input_dev;
	struct serdev_device        *sdev;
	const struct key_info       *k_info;
};

const struct key_info k_info[NUMBER_OF_KEY] = {
	{
		.name = BACK_KEY,
		.key_event = KEY_BACK,
		.key_code = 0x16,
	},
	{
		.name = ENTER_KEY,
		.key_event = KEY_ENTER,
		.key_code = 0x1C,
	},
	{
		.name = UP_KEY,
		.key_event = KEY_UP,
		.key_code = 0x18,
	},
	{
		.name = DOWN_KEY,
		.key_event = KEY_DOWN,
		.key_code = 0x52,
	},
	{
		.name = LEFT_KEY,
		.key_event = KEY_LEFT,
		.key_code = 0x08,
	},
	{
		.name = RIGHT_KEY,
		.key_event = KEY_RIGHT,
		.key_code = 0x5A,
	},
};

static int ir_decoder_receive_buf(struct serdev_device *sdev,
			const unsigned char *data, size_t count)
{
	int i;
	struct ir_decoder_input_dev *ir_dev;

	ir_dev = serdev_device_get_drvdata(sdev);

	if (count == 3) {
		pr_info("Get data from ir_module: %02X\n", data[2]);
		for (i = 0; i < NUMBER_OF_KEY; i++) {
			if (data[2] == ir_dev->k_info[i].key_code) {
				pr_info("%s is pressed\n", ir_dev->k_info[i].name);
				input_report_key(ir_dev->input_dev,
						ir_dev->k_info[i].key_event,
						1);
				input_report_key(ir_dev->input_dev,
						ir_dev->k_info[i].key_event,
						0);
				input_sync(ir_dev->input_dev);
			}
		}
	}

	serdev_device_write_flush(sdev);

	return count;
}

static struct serdev_device_ops ir_decoder_input_ops = {
	.receive_buf = ir_decoder_receive_buf,
	.write_wakeup = serdev_device_write_wakeup,
};

static int ir_decoder_input_probe(struct serdev_device *sdev)
{
    int ret;
    int i;
    int baudrate;
    struct ir_decoder_input_dev *ir_dev;
    struct input_dev *input_dev;

    pr_info("[Tung] Jumpt to ir_decoder_input_probe function\n");
    ir_dev = devm_kzalloc(&sdev->dev, sizeof(struct ir_decoder_input_dev),
				GFP_KERNEL);
    if (!ir_dev) {
        return -ENOMEM;
    }

    if (of_property_read_u32(sdev->dev.of_node, "current-speed", &baudrate) == 0) {
        pr_info("[Tung] baudrate: %d\n", baudrate);
    }

    ir_dev->sdev = sdev;
    serdev_device_set_drvdata(sdev, ir_dev);

    ret = serdev_device_open(sdev);
    if (ret) {
        pr_err("[Tung]: Failed to open serial device\n");
        return ret;
    }

    serdev_device_set_baudrate(sdev, baudrate);
    serdev_device_set_flow_control(sdev, false);
    serdev_device_set_client_ops(sdev, &ir_decoder_input_ops);

    /* Setup input device */
    input_dev = input_allocate_device();
    if (!input_dev) {
	    pr_err("input_allocate_device failed\n");
	    return -ENOMEM;
    }
    input_dev->name = "ir_decoder_input";
    input_dev->dev.parent = &sdev->dev;
    ir_dev->input_dev = input_dev;
    ir_dev->k_info = k_info;
    set_bit(EV_KEY, ir_dev->input_dev->evbit);
    set_bit(EV_REP, ir_dev->input_dev->evbit);
    for (i = 0; i < NUMBER_OF_KEY; i++) {
	    set_bit(ir_dev->k_info[i].key_event, ir_dev->input_dev->keybit);
    }

    /* register input device */
    ret = input_register_device(ir_dev->input_dev);
    if (ret) {
	    pr_err("input_register_device failed\n");
	    return ret;
    }

    return 0;
}

static void ir_decoder_input_remove(struct serdev_device *sdev)
{
	struct ir_decoder_input_dev *ir_dev;

	ir_dev = serdev_device_get_drvdata(sdev);
	input_free_device(ir_dev->input_dev);
	serdev_device_close(sdev);
}

static struct of_device_id ir_decoder_input_table[] = {
	{ .compatible = "ti,ir_decoder" },
	{ /* NULL */},
};
MODULE_DEVICE_TABLE(of, ir_decoder_input_table);

static struct serdev_device_driver ir_decoder_input_driver = {
	.probe = ir_decoder_input_probe,
	.remove = ir_decoder_input_remove,
	.driver = {
		.name = "ir_decoder_module",
		.of_match_table = of_match_ptr(ir_decoder_input_table),
	},
};

static int __init ir_decoder_input_init(void)
{
	pr_info("[Tung] Jumpt to ir_decoder_input_init function\n");
	return serdev_device_driver_register(&ir_decoder_input_driver);
}

static void __exit ir_decoder_input_exit(void)
{
	pr_info("[Tung] Jumpt to ir_decoder_input_exit function\n");
	serdev_device_driver_unregister(&ir_decoder_input_driver);
}

late_initcall(ir_decoder_input_init);
module_exit(ir_decoder_input_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tungnt");
MODULE_DESCRIPTION("ir decoder module driver");
