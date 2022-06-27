#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#define RUNABLE 0
#define RUNNING 1
#define BLOCKED 2
#define STACK_SIZE 8192

void *kalloc_safe(size_t size);
void kfree_safe(void *ptr);

struct task {
  Context *ctx;
  uint32_t status;                   //the running status of the task
  char *name;
  //uint8_t flag[8];
  struct task *next;
  AddrSpace as;
  void *stack;
  //uint8_t stack[STACK_SIZE];
  // TODO
};

struct spinlock {
  char *name;
  int lock;
  int ncpu;                //which cpu acquire this lock
  // TODO
};
struct taskqueue {
  struct task *t;
  struct taskqueue *next;
};
struct semaphore {
  char *name;
  int val;
  struct spinlock lock;
  struct taskqueue *head;
  struct taskqueue *trail;
  // TODO
};