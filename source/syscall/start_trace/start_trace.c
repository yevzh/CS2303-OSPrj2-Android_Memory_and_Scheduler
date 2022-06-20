/*
This is the source code for syscall:start_trace
The function is to start tracing a task
Author: WeimingZhang  ID:520021911141
*/
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/unistd.h>
MODULE_LICENSE("Dual BSD/GPL");
#define __NR_start_trace 361
static int (*oldcall)(void);
static int sys_start_trace(pid_t pid)
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
  else
  {
    printk(KERN_INFO "START TRACE!\n");
    if (tsk->trace_flag == 1)
    {
      printk(KERN_INFO "START TWICE!\n");
      return -EINVAL;
    }
    tsk->trace_flag = 1;
    tsk->wcounts = 0;
    printk(KERN_INFO "Current pid:%d\n", tsk->pid);
    printk(KERN_INFO "Set the trace flag:%d\n", tsk->trace_flag);
    printk(KERN_INFO "Write counts:%d\n", tsk->wcounts);
    // printk(KERN_INFO "Current thread:%d\n", tsk->pid);
    return 0;
  }
}
static int addsyscall_init(void)
{
  long *syscall = (long *)0xc000d8c4;
  oldcall = (int (*)(void))(syscall[__NR_start_trace]);
  syscall[__NR_start_trace] = (unsigned long)sys_start_trace;
  printk(KERN_INFO "module load!\n");
  return 0;
}
static void addsyscall_exit(void)
{
  long *syscall = (long *)0xc000d8c4;
  syscall[__NR_start_trace] = (unsigned long)oldcall;
  printk(KERN_INFO "module exit!\n");
}
module_init(addsyscall_init);
module_exit(addsyscall_exit);
