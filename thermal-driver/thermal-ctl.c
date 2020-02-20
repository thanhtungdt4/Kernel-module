#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/io.h>

#include "ADC0808.h"

static struct timer_list time_list;
static struct task_struct *tsk_str;
static unsigned char temp;
static unsigned int *gpio_addr;
static int bit;

/*This function is to supply clock for ADC*/
void timer_handle(unsigned long tm)
{
	
	gpio_set_value(gpio_addr, ADC0808_CLK, bit = !bit);
	mod_timer(&time_list, jiffies + usecs_to_jiffies(1562));
}

int thread_func(void *data)
{
	while (!kthread_should_stop()) {
		temp = ADC0808_Read(gpio_addr, 0);
		temp_show(gpio_addr, temp);
		msleep(500);
	}

	return 0;
}

int write_seq(struct seq_file *sef, void *data)
{
	seq_printf(sef, "Temperature is: %d *C", temp);

	return 0;
}

static int dev_open(struct inode *inodep, struct file *filep)
{
	single_open(filep, write_seq, NULL);

	return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.read = seq_read,
	.release = seq_release,
};

static int __init example_init(void)
{
	gpio_addr = (unsigned int *)ioremap(GPIO_BASE_ADDR, 0x100);
	setup_lcd(gpio_addr);
	adc_gpio_set_output(gpio_addr);

	write_string(gpio_addr, "Temp is:");
	temp = ADC0808_Read(gpio_addr, 0);
	temp_show(gpio_addr, temp);
	goto_xy(gpio_addr, 0, 14);
	write_string(gpio_addr, "*C");

	proc_create("example", 0664, NULL, &fops);

	setup_timer(&time_list, timer_handle, 0);
	mod_timer(&time_list, jiffies + usecs_to_jiffies(1562));

	tsk_str = kthread_run(thread_func, NULL, "thread");
	pr_info("create proc file system\n");

	return 0;
}

static void __exit example_exit(void)
{
	kthread_stop(tsk_str);
	remove_proc_entry("example", NULL);
	del_timer(&time_list);
	iounmap(gpio_addr);
	pr_info("goodbye\n");
}

module_init(example_init);
module_exit(example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tung");
