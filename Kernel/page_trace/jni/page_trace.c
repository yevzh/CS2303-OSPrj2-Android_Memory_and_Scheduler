#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int alloc_size;
static char *memory;
static int times;

void segv_handler(int signal_number) {
  //printf("find memory accessed!\n");
  mprotect(memory, alloc_size, PROT_READ | PROT_WRITE);
  times++;
  //printf("set memory read write!\n");
}

int main() {
  int fd;
  struct sigaction sa;
  unsigned long wcount;
  printf("Start memory trace testing program!\n");
  syscall(361, getpid());

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = &segv_handler;
  sigaction(SIGSEGV, &sa, NULL);
  times = 0;

  alloc_size = 10 * getpagesize();
  fd = open("/dev/zero", O_RDONLY);
  memory = mmap(NULL, alloc_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  int n,i;
  scanf("%d", &n);

  for ( i = 0; i < n; i++) {
    mprotect(memory, alloc_size, PROT_READ);
    memory[0] = i;
  }
  wcount = syscall(363, getpid());
  printf("Task pid: %d, wcounts: %lu,times: %d\n", getpid(), wcount, times);
  syscall(362, getpid());
  wcount = syscall(363, getpid());
  printf("Task pid: %d, wcounts: %lu,times: %d\n", getpid(), wcount, times);
  munmap(memory, alloc_size);
  return 0;
}
