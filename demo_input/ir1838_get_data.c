#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>

struct task_struct 	*kthread;
struct gpio_desc *ir1838_pin;

uint32_t read_data(void)
{
    int i;
    int count = 0;
    uint32_t code = 0;

    while (!(gpiod_get_value(ir1838_pin)));
    while (gpiod_get_value(ir1838_pin));

    for (i = 0; i < 32; i++) {
        count = 0;
        while (!(gpiod_get_value(ir1838_pin)));
        while (gpiod_get_value(ir1838_pin)) {
            count++;
            usleep_range(100, 101);
        }
        pr_info("count: %d\n", count);
        if (count >  12) {
            code |= (1 << (31 - i));
        } else {
            code &= ~(1 << (31 - i));
        }
    }
    return code;
}

static int kthread_handler_func(void *data)
{
    uint32_t val;

    while (!kthread_should_stop()) {
        val = read_data();
        pr_info("val: %X\n", val);
    }

    return 0;
}

static int ir1838_input_probe(struct platform_device *pdev)
{
    ir1838_pin = gpiod_get_index(&pdev->dev, "input", 0, GPIOD_IN);
    kthread = kthread_create(kthread_handler_func, NULL, "report_input");
    if (kthread) {
		pr_info("Create kthread successfully\n");
		wake_up_process(kthread);
    }

    return 0;
}

static int ir1838_input_remove(struct platform_device *pdev)
{
    	kthread_stop(kthread);
	gpiod_put(ir1838_pin);

    	return 0;
}

static struct of_device_id ir1838_input_table[] = {
	{ .compatible = "raspi4,ir1838_input_driver" },
	{ /* NULL */ },
};
MODULE_DEVICE_TABLE(of, ir1838_input_table);

static struct platform_driver ir1838_input_driver = {
	.probe = ir1838_input_probe,
	.remove = ir1838_input_remove,
	.driver = {
		.name = "ir1838_input",
		.of_match_table = ir1838_input_table,
		.owner = THIS_MODULE,
	},
};
module_platform_driver(ir1838_input_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thanhtungdt4");
MODULE_DESCRIPTION("ir1838 input driver");
