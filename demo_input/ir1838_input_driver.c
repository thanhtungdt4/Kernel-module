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

#include "ir1838_input_driver.h"

#define NUM_OF_KEYS		6
#define BACK_KEY		1
#define ENTER_KEY		2
#define UP_KEY			3
#define DOWN_KEY		4
#define LEFT_KEY		5
#define RIGHT_KEY		6
#define EXPIRE_TIME		50

const uint64_t key_code_arr[NUM_OF_KEYS] = {BACK_KEY, ENTER_KEY, UP_KEY,
					DOWN_KEY, LEFT_KEY, RIGHT_KEY};

const int key_event_array[NUM_OF_KEYS] = {KEY_BACK, KEY_ENTER, KEY_UP,
				KEY_DOWN, KEY_LEFT, KEY_RIGHT};

struct ir1838_input_device {
	struct hrtimer	 		timer;
	struct platform_device 		*pdev;
	struct gpio_desc 		*ir1838_pin;
	struct input_dev 		*input_dev;
	struct completion 		start_report_input;
	struct task_struct 		*kthread;
	uint64_t 			key_code;
	int 				key_event[NUM_OF_KEYS];
	ktime_t 			ktime;
	struct irparams 		ir_result;
	struct decode_result 		result;
	unsigned int 			bit_cnt;
	unsigned int 			bits[32];
};

int MATCH_MARK(int measured_ticks, int desired_us)
{
	return measured_ticks >= TICKS_LOW(desired_us + MARK_EXCESS) && 
		measured_ticks <= TICKS_HIGH(desired_us + MARK_EXCESS);
}

int MATCH_SPACE(int measured_ticks, int desired_us)
{
	return measured_ticks >= TICKS_LOW(desired_us - MARK_EXCESS) && 
		measured_ticks <= TICKS_HIGH(desired_us - MARK_EXCESS);
}

static int decode(struct ir1838_input_device *ir1838_dev)
{
	long data = 0;
	int offset = 1;
	int i, j;
	struct irparams ir_result = ir1838_dev->ir_result;
	struct decode_result *results = &ir1838_dev->result;

	results->rawbuf = ir_result.rawbuf;
	results->rawlen = ir_result.rawlen;

	//memset(results, 0, sizeof(struct decode_result));
	for (j = 0; j < RAWBUF; j++) {
		pr_info("DEBUG: rawbuf data\n");
		pr_info("DEBUG: %d\n", results->rawbuf[j]);
	}
	if(!MATCH_MARK(results->rawbuf[offset], NEC_HDR_MARK)) {
		return DECODE_FAILED;
	}

	for (j = 0; j < RAWBUF; i++) {
		offset++;
		if (ir_result.rawlen == 4 && 
			MATCH_SPACE(results->rawbuf[offset], NEC_RPT_SPACE) &&
			MATCH_MARK(results->rawbuf[offset+1], NEC_BIT_MARK)) {
			results->bits = 0;
			results->value = REPEAT;
			return DECODE_SUCCESS;
		}
		if (ir_result.rawlen < 2 * NEC_BITS + 4) {
			return DECODE_FAILED;
		}

		if (!MATCH_SPACE(results->rawbuf[offset], NEC_HDR_SPACE)) {
			return DECODE_FAILED;
		}
		offset++;
		for (i = 0; i < NEC_BITS; i++) {
			if (!MATCH_MARK(results->rawbuf[offset], NEC_BIT_MARK)) {
				return DECODE_FAILED;
			}
			offset++;
			if (MATCH_SPACE(results->rawbuf[offset], NEC_ONE_SPACE)) {
				data = (data << 1) | 1;
			} else if (MATCH_SPACE(results->rawbuf[offset], NEC_ZERO_SPACE)) {
				data <<= 1;
			} else {
				return DECODE_FAILED;
			}
			offset++;
		}
		results->bits = NEC_BITS;
		results->value = data;
		return DECODE_SUCCESS;
	}
	return DECODE_SUCCESS;
}

static void resume(struct ir1838_input_device *ir1838_dev)
{
	ir1838_dev->ir_result.rcvstate = STATE_IDLE;
	ir1838_dev->ir_result.rawlen = 0;
}

enum hrtimer_restart timer_handler_func(struct hrtimer *timer)
{
	int irdata;
	struct ir1838_input_device *ir1838_dev = 
		container_of(timer, struct ir1838_input_device, timer);

	hrtimer_forward_now(timer, ir1838_dev->ktime);

	irdata = gpiod_get_value(ir1838_dev->ir1838_pin);

	ir1838_dev->ir_result.timer++;
	if (ir1838_dev->ir_result.rawlen >= RAWBUF) {
		resume(ir1838_dev);
	}
	switch (ir1838_dev->ir_result.rcvstate) {
	case STATE_IDLE:
		if (irdata == MARK) {
			if (ir1838_dev->ir_result.timer < GAP_TICKS || 
					ir1838_dev->ir_result.timer > 10000) {
				ir1838_dev->ir_result.timer = 0;
			} else {
				ir1838_dev->ir_result.rawlen = 0;
				pr_info("DEBUG: STATE IDLE: timer: %d\n", ir1838_dev->ir_result.timer);
				ir1838_dev->ir_result.rawbuf[ir1838_dev->ir_result.rawlen++] =
					ir1838_dev->ir_result.timer;
				ir1838_dev->ir_result.timer = 0;
				ir1838_dev->ir_result.rcvstate = STATE_MARK;
			}
		}
		break;
	case STATE_MARK:
		if (irdata == SPACE) {
			pr_info("DEBUG: STATE_MARK: Line: %d, timer: %d\n", __LINE__, ir1838_dev->ir_result.timer);
			ir1838_dev->ir_result.rawbuf[ir1838_dev->ir_result.rawlen++] = 
				ir1838_dev->ir_result.timer;
			ir1838_dev->ir_result.timer = 0;
			ir1838_dev->ir_result.rcvstate = STATE_SPACE;
		}
		break;
	case STATE_SPACE:
		if (irdata == MARK) {
			pr_info("DEBUG: START_SPACE: rawlen: %d, timer: %d\n", 
			ir1838_dev->ir_result.rawlen, ir1838_dev->ir_result.timer);
			ir1838_dev->ir_result.rawbuf[ir1838_dev->ir_result.rawlen++] = 
				ir1838_dev->ir_result.timer;
			ir1838_dev->ir_result.timer = 0;
			ir1838_dev->ir_result.rcvstate = STATE_MARK;
		} else {
			if (ir1838_dev->ir_result.timer > GAP_TICKS) {
				ir1838_dev->ir_result.rcvstate = STATE_STOP;
				pr_info("DEBUG: START_SPACE: timer: %d, change state to STATE_STOP\n", 
						ir1838_dev->ir_result.timer);
			}
		}
		break;
	case STATE_STOP:
		pr_info("DEBUG: Line: %d, Jump to START_STOP\n", __LINE__);
		if (irdata == MARK) {
			ir1838_dev->ir_result.timer = 0;
			complete(&ir1838_dev->start_report_input);
			pr_info("DEBUG: Line: %d, call complete function\n", __LINE__);
		}
		break;
	}

	return HRTIMER_RESTART;
}

static int kthread_handler_func(void *data)
{
	int val;
	unsigned long timeout;
	struct ir1838_input_device *ir1838_dev = (struct ir1838_input_device *)data;

	timeout = msecs_to_jiffies(720000);
	val = gpiod_get_value(ir1838_dev->ir1838_pin);
	pr_info("DEBUG: gpio pin val: %d\n", val);
	pr_info("DEBUG: Jumpt to %s func\n", __func__);
	while (!kthread_should_stop()) {
		pr_info("DEBUG: Waiting for data from ir1838 sensor...\n");
		if (!wait_for_completion_timeout(&ir1838_dev->start_report_input, timeout)) {
			pr_info("wait_for_completion timeout\n");
		} else {
			// Start to decode
			if (decode(ir1838_dev)) {
				pr_info("DEBUG: decode value: %d\n", 
						ir1838_dev->result.value);
				resume(ir1838_dev);
			} else {
				pr_info("DEBUG: Decode value failed\n");
				resume(ir1838_dev);
			}
		}
	}
	return 0;
}

static struct of_device_id ir1838_input_table[] = {
	{ .compatible = "raspi4,ir1838_input_driver" },
	{ /* NULL */ },
};
MODULE_DEVICE_TABLE(of, ir1838_input_table);

static int ir1838_input_probe(struct platform_device *pdev)
{
	int ret;
	int i;
	struct ir1838_input_device *ir1838_dev;
	struct input_dev *input_dev;

	pr_info("DEBUG: Jump to %s func\n", __func__);
	ir1838_dev = devm_kzalloc(&pdev->dev, 
			sizeof(struct ir1838_input_device), GFP_KERNEL);
	if (!ir1838_dev) {
		pr_err("devm_kzalloc: fails\n");
		return -ENOMEM;
	}
	/* Setup input device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("input_allocate_device failed\n");
		return -ENOMEM;
	}
	input_dev->name = "ir1838_input";
	input_dev->dev.parent = &pdev->dev;
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_REP, input_dev->evbit);
	ir1838_dev->input_dev = input_dev;
	for (i = 0; i < NUM_OF_KEYS; i++) {
		set_bit(key_event_array[i], ir1838_dev->input_dev->keybit);
		ir1838_dev->key_event[i] = key_code_arr[i];
	}

	ir1838_dev->pdev = pdev;
	ir1838_dev->ir_result.rcvstate = STATE_IDLE;
	ir1838_dev->ir_result.rawlen = 0;
	ir1838_dev->ir_result.timer = 0;
	ir1838_dev->bit_cnt = 0;
	memset(ir1838_dev->bits, 0, sizeof(ir1838_dev->bits));
	ir1838_dev->ir1838_pin = gpiod_get_index(&pdev->dev, "input", 0,
				GPIOD_IN);
	if (!ir1838_dev->ir1838_pin) {
		pr_err("gpiod_get_index: Cannot get gpiod desc\n");
		return -ENODEV;
	}

	/* register input device */
	ret = input_register_device(ir1838_dev->input_dev);
	if (ret) {
		pr_err("input_register_device failed\n");
		return ret;
	}

	platform_set_drvdata(pdev, ir1838_dev);
	// init completion
	init_completion(&ir1838_dev->start_report_input);

	// Setup timer and start timer
	ir1838_dev->ktime = ktime_set(0, EXPIRE_TIME * NSEC_PER_USEC);
	hrtimer_init(&ir1838_dev->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_HARD);
	ir1838_dev->timer.function = timer_handler_func;
	hrtimer_start(&ir1838_dev->timer, ir1838_dev->ktime, HRTIMER_MODE_REL_HARD);

	// Create kernel thread and start thread
	ir1838_dev->kthread = kthread_create(kthread_handler_func, ir1838_dev,
		       	"report_input");
	if (ir1838_dev->kthread) {
		pr_info("Create kthread successfully\n");
		wake_up_process(ir1838_dev->kthread);
	} else {
		pr_info("Create kthread failed\n");
		gpiod_put(ir1838_dev->ir1838_pin);
		input_free_device(ir1838_dev->input_dev);
		return -1;
	}
	pr_info("DEBUG: End %s func\n", __func__);

	return 0;
}

static int ir1838_input_remove(struct platform_device *pdev)
{
	struct ir1838_input_device *ir1838_dev = platform_get_drvdata(pdev);

	hrtimer_cancel(&ir1838_dev->timer);
	gpiod_put(ir1838_dev->ir1838_pin);
	input_free_device(ir1838_dev->input_dev);
	kthread_stop(ir1838_dev->kthread);

	return 0;
}

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
