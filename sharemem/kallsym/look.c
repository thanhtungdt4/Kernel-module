#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/kallsyms.h>
#include <linux/wait.h>

struct task_struct *thread;
unsigned long address;
unsigned long addr;
void *queue;
int *val;

int kthread_handle(void *data)
{
	wait_queue_head_t *wait = (wait_queue_head_t*)data;
	if (wait == NULL){
		pr_info("wait is NULL\n");
		return 0;
	}

	msleep(2000);
	wake_up_interruptible(wait);

	return 0;
}

static int __init demo_init(void)
{
	address = kallsyms_lookup_name("my_wait_queue");
	if (!address)
		pr_err("Can not find symbol\n");
	queue = (void *)address;

	thread = kthread_run(kthread_handle, queue, "kthread");
	addr = kallsyms_lookup_name("my_i2c_val");
	if (!addr)
		pr_err("Can noot find symbol 2\n");
	val = (int *)addr;
	*val = 6;

	return 0;
}

static void __exit demo_exit(void)
{
	kthread_stop(thread);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
