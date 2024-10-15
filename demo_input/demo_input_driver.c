#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#define NUMBER_OF_GPIO_PIN	6
#define BACK_KEY		"back"
#define ENTER_KEY		"Enter"
#define UP_KEY			"up"
#define DOWN_KEY		"down"
#define LEFT_KEY		"left"
#define RIGHT_KEY		"right"

const char *key_name_arr[NUMBER_OF_GPIO_PIN] = {BACK_KEY, ENTER_KEY, UP_KEY,
						DOWN_KEY, LEFT_KEY, RIGHT_KEY};

const int key_event_arr[NUMBER_OF_GPIO_PIN] = {KEY_BACK, KEY_ENTER, KEY_UP,
					KEY_DOWN, KEY_LEFT, KEY_RIGHT};

struct gpio_data {
	const char *name;
	int key_event;
	struct gpio_desc *desc;
};

struct input_driver_data {
	struct platform_device *pdev;
	struct input_dev *input_dev;
	struct gpio_data gpio_arr[NUMBER_OF_GPIO_PIN];
	int irq_arr[NUMBER_OF_GPIO_PIN];
	spinlock_t lock;
};

static irqreturn_t btn_interrupt_handle(int irq, void *dev_id)
{
	int i;
	int val;
	unsigned long flags;
	struct input_driver_data *input_data = (struct input_driver_data *)dev_id;

	pr_info("Jump to btn_interrupt_handle with irq No: %d\n", irq);
	
	spin_lock_irqsave(&input_data->lock, flags);
	for (i = 0; i <NUMBER_OF_GPIO_PIN; i++) {
		if (input_data->irq_arr[i] == irq) {
			val = gpiod_get_value(input_data->gpio_arr[i].desc);
			input_report_key(input_data->input_dev,
					input_data->gpio_arr[i].key_event,
					1);
			input_report_key(input_data->input_dev,
					input_data->gpio_arr[i].key_event,
					0);
			pr_info("interrupt happen for key %s, gpio pin val: %d\n",
					input_data->gpio_arr[i].name, val);
			input_sync(input_data->input_dev);
		}
	}
	spin_unlock_irqrestore(&input_data->lock, flags);
	return IRQ_HANDLED;
}

static struct of_device_id demo_input_table[] = {
	{ .compatible = "raspi4,demo_input"},
	{ /* NULL */ },
};
MODULE_DEVICE_TABLE(of, demo_input_table);

static int demo_input_probe(struct platform_device *pdev)
{
	int ret;
	int i = 0;
	struct input_driver_data *input_data;
	struct input_dev *input_dev;

	input_data = devm_kzalloc(&pdev->dev, sizeof(struct input_driver_data),
				GFP_KERNEL);
	if (!input_data) {
		pr_err("devm_kzalloc: fails\n");
		return -ENOMEM;
	}
	spin_lock_init(&input_data->lock);

	/* Setup input device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("input_allocate_device failed\n");
		return -ENOMEM;
	}
	input_dev->name = "demo_input";
	input_dev->dev.parent = &pdev->dev;
	input_data->input_dev = input_dev;
	input_data->pdev = pdev;
	set_bit(EV_KEY, input_data->input_dev->evbit);
	set_bit(EV_REP, input_data->input_dev->evbit);

	/*get gpio pins */
	for (i = 0; i < NUMBER_OF_GPIO_PIN; i++) {
		set_bit(key_event_arr[i], input_data->input_dev->keybit);
		input_data->gpio_arr[i].name = key_name_arr[i];
		input_data->gpio_arr[i].key_event = key_event_arr[i];
		input_data->gpio_arr[i].desc = gpiod_get_index(&pdev->dev, 
				"inputkey", i, GPIOD_IN);
		if (!input_data->gpio_arr[i].desc) {
			pr_err("gpiod_get_index: Cannot get gpiod index: %d\n",
					i);
			return -ENODEV;
		}
		gpiod_set_debounce(input_data->gpio_arr[i].desc, 400);

		input_data->irq_arr[i] = gpiod_to_irq(
				input_data->gpio_arr[i].desc
				);
		pr_info("Debug: keyname->%s, keyevent->%d, key_irq->%d\n",
				input_data->gpio_arr[i].name,
				input_data->gpio_arr[i].key_event,
				input_data->irq_arr[i]);
		ret = request_irq(input_data->irq_arr[i], btn_interrupt_handle,
				IRQF_TRIGGER_RISING | IRQF_ONESHOT,
				input_data->gpio_arr[i].name,
				input_data);
		if (ret < 0) {
			pr_err("Cannot request_irq for %s\n",
					input_data->gpio_arr[i].name);
			return ret;
		}

	}

	/* register input device */
	ret = input_register_device(input_data->input_dev);
	if (ret) {
		pr_err("input_register_device failed\n");
		goto gpio_err;
	}

	platform_set_drvdata(pdev, input_data);
	return 0;

gpio_err:
	for (i = 0; i < NUMBER_OF_GPIO_PIN; i++) {
		gpiod_put(input_data->gpio_arr[i].desc);
	}
	return ret;
}

static int demo_input_remove(struct platform_device *pdev)
{
	int i;
	struct input_driver_data *input_data = platform_get_drvdata(pdev);

	for (i = 0; i < NUMBER_OF_GPIO_PIN; i++) {
		free_irq(input_data->irq_arr[i], NULL);
		gpiod_put(input_data->gpio_arr[i].desc);
	}
	input_free_device(input_data->input_dev);
	return 0;
}

static struct platform_driver demo_input_driver = {
	.probe = demo_input_probe,
	.remove = demo_input_remove,
	.driver = {
		.name = "demo_input",
		.of_match_table = demo_input_table,
		.owner = THIS_MODULE,
	},
};
module_platform_driver(demo_input_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thanhtungdt4");
MODULE_DESCRIPTION("demo input driver");
