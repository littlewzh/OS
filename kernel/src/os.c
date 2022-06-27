#include <common.h>
#define task_alloc() pmm->alloc(sizeof(task_t))
//#define TEST1
//#define TEST2
#define TEST3
#ifdef TEST1
sem_t empty, fill;

static void producer() {
	while(1){kmt->sem_wait(&empty); putch('('); kmt->sem_signal(&fill);}
}

static void consumer() {
	while(1){kmt->sem_wait(&fill); putch(')');  kmt->sem_signal(&empty);}
}
static void create_threads() {
  kmt->sem_init(&empty,"empty",5);
  kmt->sem_init(&fill,"fill",0);
  for(int i=0;i<64;i++){
    kmt->create(pmm->alloc(sizeof(task_t)),"test-thread-1", producer, NULL);
    kmt->create(pmm->alloc(sizeof(task_t)),"test-thread-2", consumer, NULL);
  }
  
}
#endif
#ifdef TEST2
#include <devices.h>
static void tty_reader(void *arg) {
  device_t *tty = dev->lookup(arg);
  char cmd[128], resp[128], ps[16];
  sprintf(ps, "(%s) $ ", arg);
  while (1) {
    tty->ops->write(tty, 0, ps, strlen(ps));
    int nread = tty->ops->read(tty, 0, cmd, sizeof(cmd) - 1);
    cmd[nread] = '\0';
    sprintf(resp, "tty reader task: got %d character(s).\n", strlen(cmd));
    tty->ops->write(tty, 0, resp, strlen(resp));
  }
}
#endif





extern int uproc_create(task_t *task, const char *name);

static void os_init() {
  pmm->init();
  kmt->init();
  uproc->init();
  #ifdef TEST3
  uproc_create(task_alloc(),"init");
  #endif
  #ifdef TEST2
  dev->init();
  kmt->create(task_alloc(), "tty_reader", tty_reader, "tty1");
  kmt->create(task_alloc(), "tty_reader", tty_reader, "tty2");
  #endif
  #ifdef TEST1
  create_threads();
  #endif
}





static void os_run() {
  iset(true);                            //the interrupt is allowed
  /*for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    putch(*s == '*' ? '0' + cpu_current() : *s);
  }*/
  while (1) ;
}







struct callback{
  int seq;
  int event;
  handler_t call;
}handler[128];
extern spinlock_t traplock;
extern task_t *task_cpu[8];
extern task_t *task_head;
task_t task_boot[8];
int handler_sum=0;
task_t *round;


static Context *os_trap(Event ev, Context *ctx){
  assert(ienabled() == false); //to promise the interrupt closed
  Context *next=NULL;
  int id=cpu_current();
  //kmt->spin_lock(&traplock);
  if(task_cpu[id]==NULL){
    task_cpu[id]=&task_boot[id];
    task_cpu[id]->ctx=ctx;
    task_cpu[id]->status=RUNNING;
  }
  else{
    task_cpu[id]->ctx=ctx;
  }
  if(task_cpu[id]->status != BLOCKED) task_cpu[id]->status=RUNABLE;
  for(int i=0;i<handler_sum;i++){
    if(handler[i].event==EVENT_NULL || handler[i].event==ev.event) handler[i].call(ev,ctx);
  }
  //choose the next task event
  kmt->spin_lock(&traplock);
  if(task_head==NULL) {task_cpu[id]->status=RUNNING;next=task_cpu[id]->ctx;}
  else{
    if(round==NULL ){
      round=task_head;
    }
    else if(round==task_head){
      round=NULL;
    }
    else{
      if(round->next == NULL) {round=task_head;}
      else{round=round->next;}
    }
    if(round!=NULL){
      while(round->status != RUNABLE ){
      //if(round->next == NULL){round=task_head;continue;}
      round=round->next;
      if(round==NULL) break;
      }
    }
    //printf("the thread %s status is %d\n",round->name,round->status);
    if(round==NULL){task_cpu[id]=&task_boot[id];task_cpu[id]->status=RUNNING;next=task_boot[id].ctx;}
    else{
      assert(round->status == RUNABLE);
      task_cpu[id]=round;
      round->status=RUNNING;
      next=round->ctx;
    }
  }
  kmt->spin_unlock(&traplock);
  return next;
}
static void os_on_irq(int seq,int ev,handler_t fun){
  handler[handler_sum].seq=seq;
  handler[handler_sum].event=ev;
  handler[handler_sum].call=fun;
  handler_sum++;
  for(int i=0;i<handler_sum;i++){
    for(int j=i+1;j<handler_sum;j++){
      if(handler[i].seq < handler[j].seq){
        struct callback tmp={.call=handler[i].call,.event=handler[i].event,.seq=handler[i].seq};
        handler[i].call  = handler[j].call;
        handler[i].seq   = handler[j].seq;
        handler[i].event = handler[j].event;
        handler[j].call  = tmp.call;
        handler[j].seq   = tmp.seq;
        handler[j].event = tmp.event;
      }
    }
  }
}

MODULE_DEF(os) = {
  .init   = os_init,
  .run    = os_run,
  .trap   = os_trap,
  .on_irq = os_on_irq,
};
