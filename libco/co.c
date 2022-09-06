#include "co.h"
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
//#include <rand.h>
#define STACK_SIZE 1024*64
#ifdef LOCAL_MACHINE
  #define debug(...) printf(__VA_ARGS__)
#else
  #define debug(s) assert(1)
#endif
enum co_status {
  CO_NEW = 1, // 新创建，还未执行过
  CO_RUNNING, // 已经执行过
  CO_WAITING, // 在 co_wait 上等待
  CO_DEAD,    // 已经结束，但还未释放资源
};

struct co {
  char *name;
  void (*func)(void *); // co_start 指定的入口地址和参数
  void *arg;
  int num;
  enum co_status status;  // 协程的状态
  struct co *    waiter;  // 是否有其他协程在等待当前协程
  jmp_buf        context; // 寄存器现场 (setjmp.h)
  uint8_t        stack[STACK_SIZE]; // 协程的堆栈
};
struct co *current;
struct co *pool[200];
int maxsize=0;
struct co boot;
__attribute__((constructor)) void co_init(){
    boot.name="main";
    boot.status=CO_RUNNING;
    current=&boot;
    debug("finish inishlize...\n");
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co *t=malloc(sizeof(struct co));
  t->name=(char *)name;
  t->func=func;
  t->arg=arg;
  t->status=CO_NEW;
  t->num=maxsize;
  pool[maxsize]=t;
  maxsize++;
  //t->context
  return t;
}

void co_wait(struct co *co) {
  while(co->status!=CO_DEAD){
      current->status=CO_WAITING;
      co_yield();
  }
  for(int i= co->num;i<maxsize-1;i++){
      pool[i+1]->num=i;
      pool[i]=pool[i+1];
  }
  pool[maxsize-1]=0;
  
  maxsize--;
  free(co);
}

void co_yield() {
  int val=setjmp(current->context);
  if(val==0){
      int k=random()%maxsize;
      while(pool[k]->status == CO_DEAD) k=random()%maxsize;
      if(pool[k]->status == CO_NEW){
	    pool[k]->status=CO_RUNNING;
	    void * sp=pool[k]->stack+sizeof(pool[k]->stack);
	    asm volatile (
            #if  __x86_64__
		    "movq %0, %%rsp \n" : :"b" ((uintptr_t)sp)
            #else
		    "movl %0, %%esp \n"
		    
		    : :"b" ((uintptr_t)sp) 
		    :
            #endif		   
		   );
            current=pool[k];
	    current->func(current->arg);
	    current->status=CO_DEAD;
	    current=&boot;
	    longjmp(current->context,1);
      }
      else if(pool[k]->status == CO_WAITING||pool[k]->status == CO_RUNNING){
	  current=pool[k];  
	  longjmp(current->context,1);
      }
  }
  else{
      current->status=CO_RUNNING;
  }  
}
