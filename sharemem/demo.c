#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/wait.h>

struct task_struct *thread;

int my_i2c_val = 0;
EXPORT_SYMBOL(my_i2c_val);

DECLARE_WAIT_QUEUE_HEAD(my_wait_queue);
EXPORT_SYMBOL(my_wait_queue);

int kthread_handle(void *data)
{
	pr_info("kthread_handle wait event\n");
	wait_event_interruptible(my_wait_queue, (my_i2c_val != 0));
	pr_info("finish waiting\n");

	return 0;
}

static int __init demo_init(void)
{
	thread = kthread_run(kthread_handle, NULL, "kthread");

	return 0;
}

static void __exit demo_exit(void)
{
	kthread_stop(thread);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
