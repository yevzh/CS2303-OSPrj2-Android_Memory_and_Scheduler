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
void segv_handler(int signal_number) {
  // printf("find memory accessed!\n");
  mprotect(memory, alloc_size, PROT_READ | PROT_WRITE);
  times++;
  // printf("set memory read write!\n");
}

int main() {

  // int fd;
  // struct sigaction sa;
  // unsigned long wcount;
  // memset(&sa, 0, sizeof(sa));
  // sa.sa_handler = &segv_handler;
  // sigaction(SIGSEGV, &sa, NULL);
  // alloc_size = 10 * getpagesize();
  // fd = open("/dev/zero", O_RDONLY);
  // memory = mmap(NULL, alloc_size, PROT_READ, MAP_PRIVATE, fd, 0);
  // close(fd);
  // mprotect(memory, alloc_size, PROT_READ);
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
  for (i = 0; i < n; i++) {
    pid = fork();
    if (pid) {
      // printf("pid %d: %d\n", i, pid);
      // wait(NULL);
    } else {
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
      syscall(361, pid_i);
      // mprotect(memory, alloc_size, PROT_READ);
      while (k--) {
        // if (rand() % 1000 == 1)
        //   k--;
        mprotect(memory, alloc_size, PROT_READ);
        memory[0] = k;
      }
      printf("chd%d STOP!\n", pid_i);
      munmap(memory, alloc_size);
      return 0;
    }
  }
  // for (i = 0; i < n; i++) {
  //   pid = fork();
  //   // sched_setscheduler(pid, 6, &param);

  //   if (pid == -1) {
  //     perror("fork error;");
  //     exit(1);
  //   } else if (pid == 0) {
  //     break;
  //   } else
  //     printf("pid: %d\n", pid);
  // }
  // if (i < n) {
  //   // k *= i;
  //   pid_t pid_i = getpid();
  //   printf("chd pid: %d\n", pid_i);
  //   sched_setscheduler(pid_i, 6, &param);
  //   syscall(361, pid_i);
  //   // mprotect(memory, alloc_size, PROT_READ);
  //   while (k--) {
  //     mprotect(memory, alloc_size, PROT_READ);
  //     memory[0] = k;
  //   }
  //   printf("chd%d STOP!\n", pid_i);

  //   // k = MAX;

  //   // // sched_rr_get_interval(pid_i, &ras_time);
  //   // sleep(i);
  //   // while (k--) {
  //   //   mprotect(memory, alloc_size, PROT_READ);
  //   //   memory[0] = k;
  //   // }
  // }
  // if (i == n)
  //   wait(NULL);
  // // wait(NULL);
  // munmap(memory, alloc_size);
  return 0;
}
