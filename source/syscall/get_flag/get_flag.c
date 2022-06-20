/*
This is the source code for syscall:get_flag
The function is to get the trace_flag of a task
Author: WeimingZhang  ID:520021911141
*/
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/unistd.h>
MODULE_LICENSE("Dual BSD/GPL");
#define __NR_get_flag 364
static int (*oldcall)(void);
static int sys_get_flag(pid_t pid)
{
  struct pid *currentPID = find_get_pid(pid);
  struct task_struct *tsk = pid_task(currentPID, PIDTYPE_PID);
  if (!currentPID)
    return -EINVAL;
  if (!tsk)
  {
    printk(KERN_INFO "INVALID PID!\n");
    return -EINVAL;
  }
  return tsk->trace_flag;
}
static int addsyscall_init(void)
{
  long *syscall = (long *)0xc000d8c4;
  oldcall = (int (*)(void))(syscall[__NR_get_flag]);
  syscall[__NR_get_flag] = (unsigned long)sys_get_flag;
  printk(KERN_INFO "module load!\n");
  return 0;
}
static void addsyscall_exit(void)
{
  long *syscall = (long *)0xc000d8c4;
  syscall[__NR_get_flag] = (unsigned long)oldcall;
  printk(KERN_INFO "module exit!\n");
}
module_init(addsyscall_init);
module_exit(addsyscall_exit);
