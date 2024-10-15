#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/completion.h>
#include <linux/kthread.h>

static struct {
  struct completion start_report_input;
  struct task_struct *kthread;
  int gpio;
  int irq;
  struct timer_list timer;
  uint32_t     pulse;
  uint32_t     space;
  size_t  count;
  uint32_t     data;
} ir;

#define is_head(p, s) (p > 8900 && p < 9100 && s > 4400 && s < 4600)
#define is_repeat(p, s) (p > 8900 && p < 9100 && s > 2150 && s < 2350)
#define is_bfalse(p, s) (p > 500 && p < 650 && s > 500 && s < 650)
#define is_btrue(p, s) (p > 500 && p < 650 && s > 1500 && s < 1750)

static irqreturn_t ir_rx(int irq, void* dev) {
  static ktime_t last = 0;
  uint32_t duration;

  duration = (uint32_t)ktime_to_us(ktime_get() - last);

  pr_info("DEBUG: duration: %d, count: %d\n", duration, ir.count);

  if (!gpio_get_value(ir.gpio)) {
    ir.space = duration;
  } else {
    ir.pulse = duration;
    goto irq_out;
  }

  if (is_head(ir.pulse, ir.space)) {
    ir.count = ir.data = 0;
  } else if (is_repeat(ir.pulse, ir.space)) {
    ir.count = 32;
  } else if (is_btrue(ir.pulse, ir.space)) {
    ir.data |= 1 << ir.count++;
  } else if (is_bfalse(ir.pulse, ir.space)) {
    ir.data |= 0 << ir.count++;
  } else {
    goto irq_out;
  }

  if (ir.count >= 32) {
    complete(&ir.start_report_input);
  }

irq_out:
  last = ktime_get();
  return IRQ_HANDLED;
}

static int kthread_handler_func(void *data)
{
    unsigned long timeout;

    timeout = msecs_to_jiffies(720000);
    while (!kthread_should_stop()) {
        if (!wait_for_completion_timeout(&ir.start_report_input, timeout)) {
            pr_info("wait_for_completion timeout\n");
        } else {
            pr_info("DEBUG: decode value: %d\n", ir.data);
        }
    }
    return 0;
}

static int __init ir_init(void) {
  int rc = 0;

  init_completion(&ir.start_report_input);
  ir.gpio = 25;
  if ((rc = gpio_request_one(ir.gpio, GPIOF_IN, "IR")) < 0) {
    printk(KERN_ERR "ERROR%d: can not request gpio%d\n", rc, ir.gpio);
    return rc;
  }

  ir.irq = gpio_to_irq(ir.gpio);
  if ((rc = request_irq(ir.irq, ir_rx,
              IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
              "IR", NULL)) < 0) {
    printk(KERN_ERR "ERROR%d: can not request irq\n", ir.irq);
    return rc;
  }

    ir.kthread = kthread_create(kthread_handler_func, NULL, "report_input");
    if (ir.kthread) {
        pr_info("Create kthread successfully\n");
        wake_up_process(ir.kthread);
    }

  return 0;
}
module_init(ir_init);

static void __exit ir_exit(void) {
  free_irq(ir.irq, NULL);
  gpio_free(ir.gpio);
  kthread_stop(ir.kthread);
}
module_exit(ir_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Philon");