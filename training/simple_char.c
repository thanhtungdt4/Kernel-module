#define MODULE
#define LINUX
#define __KERNEL__

#include <linux/init.h>
#include <linux/kernel.h>       /* We're doing kernel work */
#include <linux/module.h>       /* Specifically, a module */


static int __init simple_char_init(void)
{
  printk("Hello, world - this is the kernel speaking\n");
  return 0;
}

static void __exit simple_char_exit(void)
{
  printk("Bye!\n");
}

module_init(simple_char_init);
module_exit(simple_char_exit);

MODULE_LICENSE("GPL");

