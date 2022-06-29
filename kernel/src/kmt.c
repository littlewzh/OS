#include <os.h>

                  //the current task of the cpus
//atomic_xchg(&lk -> lock, 1)
spinlock_t traplock;
task_t *task_head;
task_t *task_cpu[8]; 
void *kalloc_safe(size_t size) {
  bool i = ienabled();
  iset(false);
  void *ret = pmm->alloc(size);
  if (i) iset(true);
  return ret;
}

void kfree_safe(void *ptr) {
  int i = ienabled();
  iset(false);
  pmm->free(ptr);
  if (i) iset(true);
}
//

//thread task
static void kmt_init(){
    kmt->spin_init(&traplock,"os_trap");
    task_head=NULL;
    for(int i=0;i<8;i++){
        task_cpu[i]=NULL;
    }

}
static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg){
    kmt->spin_lock(&traplock);
    task->name=(char *)name;
    task->stack=kalloc_safe(STACK_SIZE);
    Area kstack={.start=task->stack,.end=(void *)((uintptr_t)(task->stack)+STACK_SIZE)};
    task->ctx=kcontext(kstack,entry,arg);
    task->status=RUNABLE;
    //for(int i=0;i<8;i++){task->flag[i]=0;}
    task->next=NULL;
    task_t *tmp=task_head;
    if(tmp==NULL){
        task_head=task;
    }
    else{
        while(tmp->next != NULL) tmp=tmp->next;
        tmp->next=task;
    }
    kmt->spin_unlock(&traplock);
    return 0;
}
static void kmt_teardown(task_t *task){
    kmt->spin_lock(&traplock);
    kfree_safe(task->stack);
    task_t *now=task_head;
    if(task_head==task){
        task_head=NULL;
    }
    else{
        while(now->next != task){
            now=now->next;
            if(now==NULL) break;
        }
        assert(now!=NULL);
        now->next=task->next;
    }
    kfree_safe(task);
    kmt->spin_unlock(&traplock);
}



// spinlock 
static int spin_cpu_stat[8];
static int spin_cpu_n[8];
//The first thing to note is that after running the unlock, 
//the state of the interrupt should be the same as 
//before the corresponding lock was called
static void spin_init(spinlock_t *lk, const char *name){
    lk->name=(char *)name;
    lk->lock=0;
    lk->ncpu=-1;                         //represent the lock is not belong to any cpu
}
static void spin_lock(spinlock_t *lk){
    int st=ienabled();                   //record the state before acquire the lock
    iset(false);                         //close the interrupt
    while(atomic_xchg(& lk->lock, 1));   
    // can we change the sequence of line 41 and 42 ??????
    int id=cpu_current();
    lk->ncpu=id;
    if(spin_cpu_n[id]==0) spin_cpu_stat[id]=st;    //this is the first lock of the cpu
    spin_cpu_n[id]++;
}
static void spin_unlock(spinlock_t *lk){
    int id=cpu_current();
    spin_cpu_n[id]--;
    lk->ncpu=-1;
    atomic_xchg(& lk->lock, 0);
    if(spin_cpu_n[id]==0){ iset(spin_cpu_stat[id]);}
}


//semaphore
static void sem_init(sem_t *sem, const char *name, int value){
    kmt->spin_init(&sem->lock,name);
    sem->name=(char *)name;
    sem->val=value;
    sem->head=NULL;
    sem->trail=NULL;
}
static void sem_wait(sem_t *sem){
    kmt->spin_lock(&sem->lock);
    //kmt->spin_lock(&traplock);
    sem->val--;
    int flag=0;
    if(sem->val < 0){                     // 没有资源，需要等待
        flag=1;
        int id=cpu_current();
        struct taskqueue *tmp=kalloc_safe(sizeof(struct taskqueue));
        tmp->t=task_cpu[id];                  //how to record the all task
        //tmp->next=sem->head;
        //sem->head=tmp;
        kmt->spin_lock(&traplock);
        task_cpu[id]->status=BLOCKED;//run blocked
        kmt->spin_unlock(&traplock);
        tmp->next=NULL;
        if(sem->head==NULL){
            sem->head=tmp;
            sem->trail=tmp;
        }
        else{
            sem->trail->next=tmp;
            sem->trail=tmp;
        }
        kmt->spin_unlock(&sem->lock);
        //kmt->spin_unlock(&traplock);
        yield();
    }
    if(flag==0) kmt->spin_unlock(&sem->lock);
}
void sem_signal(sem_t *sem){
    kmt->spin_lock(&sem->lock);
    sem->val++;
    if(sem->head!= NULL){
        struct taskqueue *tmp=sem->head->next;
        kmt->spin_lock(&traplock);
        sem->head->t->status=RUNABLE;//run enable
        kmt->spin_unlock(&traplock);
        kfree_safe((void *)sem->head);
        sem->head=tmp;
    }
    kmt->spin_unlock(&sem->lock);
}


MODULE_DEF(kmt) = {
    .init=kmt_init,
    .create=kmt_create,
    .teardown=kmt_teardown,
    .spin_init=spin_init,
    .spin_lock=spin_lock,
    .spin_unlock=spin_unlock,
    .sem_init=sem_init,
    .sem_signal=sem_signal,
    .sem_wait=sem_wait,
    // TODO
};
