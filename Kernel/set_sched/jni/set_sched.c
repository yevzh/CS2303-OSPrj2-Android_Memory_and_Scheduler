#define _GNU_SOURCE
#include <ctype.h>
// #include <fcntl.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define SCHED_NORMAL 0
#define SCHED_FIFO 1
#define SCHED_RR 2
#define SCHED_BATCH 3
#define SCHED_IDLE 5
#define SCHED_RAS 6

static void change_scheduler() {
  int ret, tmp;
  struct sched_param param;
  pid_t pid;
  int policy, oldpolicy;
  oldpolicy = sched_getscheduler(pid);
  printf("Please input the schedule policy you want to change (0-NORMAL, "
         "1-FIFO, 2-RR, 6-RAS):");
  scanf("%d", &policy);
  if (policy != 0 && policy != 1 && policy != 2 && policy != 6) {
    perror("Invalid schedule policy. Aborting...\n");
    exit(-1);
  }
  // ret = sched_setscheduler(pid, policy, &param);
  // if (ret < 0) {
  //   perror("Changing scheduler failed. Aborting...");
  //   exit(-1);
  // }
  switch (policy) {
  case 0: {
    printf("Current scheduling algorithm is SCHED_NORMAL\n");
    break;
  }
  case 1: {
    printf("Current scheduling algorithm is SCHED_FIFO\n");
    break;
  }
  case 2: {
    printf("Current scheduling algorithm is SCHED_RR\n");
    break;
  }
  case 6: {
    printf("Current scheduling algorithm is SCHED_RAS\n");
    break;
  }
  default: {
    printf("No scheduler!\n");
  }
  }

  printf("Please input the process id(PID) you want to modify: ");
  scanf("%d", &tmp);
  pid = tmp;

  // struct pid *currentPID = find_get_pid(pid);
  // struct task_struct *tsk = pid_task(currentPID, PIDTYPE_PID);
  if (!pid) {
    printf("Invalid PID!\n");
    exit(-1);
  }

  int trace_flag = syscall(364, pid);
  if (policy == 6) {
    unsigned int wcounts = syscall(363, pid);
    if (trace_flag) {
      printf("Tracing! Wcount for this task: %u\n", wcounts);
    } else {
      syscall(361, pid);
      printf("Start trace for task %d\n", pid);
    }
  }

  printf("Set process's priority: ");
  scanf("%d", &tmp);
  if (policy != 6)
    param.sched_priority = tmp;
  else
    param.sched_priority = 0;
  oldpolicy = sched_getscheduler(pid);
  switch (oldpolicy) {
  case 0: {
    printf("pre scheduler: SCHED_NORMAL\n");
    break;
  }
  case 1: {
    printf("pre scheduler: SCHED_FIFO\n");
    break;
  }
  case 2: {
    printf("pre scheduler: SCHED_RR\n");
    break;
  }
  case 6: {
    printf("pre scheduler: SCHED_RAS\n");
    break;
  }
  default: {
    printf("No scheduler!\n");
  }
  }
  switch (policy) {
  case 0: {
    printf("cur scheduler: SCHED_NORMAL\n");
    break;
  }
  case 1: {
    printf("cur scheduler: SCHED_FIFO\n");
    break;
  }
  case 2: {
    printf("cur scheduler: SCHED_RR\n");
    break;
  }
  case 6: {
    printf("cur scheduler: SCHED_RAS\n");
    break;
  }
  default: {
    printf("No scheduler!\n");
  }
  }
  ret = sched_setscheduler(pid, policy, &param);
  if (ret < 0) {
    perror("Changing scheduler failed. Aborting...");
    exit(-1);
  }
}
int main() {
  change_scheduler();
  printf("Switch finished successfully!\n");
}