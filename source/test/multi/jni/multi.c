/*
This is the test program for problem3: multi process racing
Use a for loop to fork n child processes
Each of them request a given range of virtual memory
Create an environment of racing
Check whether the RAS Scheduler can schedule processes in a weighted-round-robin way
Author: WeimingZhang  ID:520021911141
*/
#include <ctype.h>
#include <fcntl.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define MAX 100000

static int alloc_size;
static char *memory;
static int times;
void segv_handler(int signal_number)
{
  // printf("find memory accessed!\n");
  mprotect(memory, alloc_size, PROT_READ | PROT_WRITE);
  times++;
  // printf("set memory read write!\n");
}

int main()
{

  pid_t pid;
  int k = MAX;

  int n = 0;
  struct timespec ras_time;
  struct sched_param param;

  param.sched_priority = 0;
  printf("Input number of pros: ");
  scanf("%d", &n);
  // n = 8;
  sched_setscheduler(getpid(), 2, &param);
  printf("parent pid:  %d\n", getpid());
  int i, status;
  for (i = 0; i < n; i++)
  {
    pid = fork();
    // The parent pid
    if (pid)
    {
    }
    // The child pid: to race for timeslices
    else
    {
      int fd;
      struct sigaction sa;
      unsigned long wcount;
      memset(&sa, 0, sizeof(sa));
      sa.sa_handler = &segv_handler;
      sigaction(SIGSEGV, &sa, NULL);
      alloc_size = 10 * getpagesize();
      fd = open("/dev/zero", O_RDONLY);
      memory = mmap(NULL, alloc_size, PROT_READ, MAP_PRIVATE, fd, 0);
      close(fd);
      pid_t pid_i = getpid();
      printf("chd pid: %d\n", pid_i);
      sched_setscheduler(pid_i, 6, &param);
      // start trace
      syscall(361, pid_i);
      // mprotect(memory, alloc_size, PROT_READ);
      while (k--)
      {
        // if (rand() % 1000 == 1)
        //   k--;
        // write fault
        mprotect(memory, alloc_size, PROT_READ);
        memory[0] = k;
      }
      printf("chd%d STOP!\n", pid_i);
      munmap(memory, alloc_size);
      return 0;
    }
  }

  return 0;
}
