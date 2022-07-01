#include <os.h>
#include <syscall.h>

#include "initcode.inc"
extern spinlock_t traplock;
extern task_t *task_head;
extern task_t *task_cpu[8]; 
extern sleep_t sleep_head;
static uint32_t process_pid;
int uproc_create(task_t *task, const char *name);
static int uproc_kputc(task_t *task, char ch) {
  putch(ch); // safe for qemu even if not lock-protected
  return 0;
}
static int uproc_getpid(task_t *task){
    //task->pid;
    return task->pid;
}
static int uproc_sleep(task_t *task, int seconds){
    //printf("%d\n",seconds);
    uint64_t wakeup = io_read(AM_TIMER_UPTIME).us + 1000000L*seconds;
    /*while(io_read(AM_TIMER_UPTIME).us < wakeup){
        printf("%d   %d\n",io_read(AM_TIMER_UPTIME).us,wakeup);
        yield();
    }*/
    sleep_t *new=kalloc_safe(sizeof(sleep_t));
    new->s=task;
    new->wakeup=wakeup;
    new->next=NULL;
    //kmt->spin_lock(&traplock);
    task->status=BLOCKED;
    //kmt->spin_lock(&traplock);
    if(sleep_head.next==NULL){
        sleep_head.next=new;
    }else{
        new->next=sleep_head.next;
        sleep_head.next=new;
    }
    //task->status=RUNABLE;
    return 0;
}
static Context *uproc_wake(Event ev,Context *c){
     if(sleep_head.next!=NULL){
        sleep_t *cur=&sleep_head;
        uint64_t timer=io_read(AM_TIMER_UPTIME).us;
        
        while(cur->next!=NULL){
            if(timer > cur->next->wakeup){
                cur->next->s->status=RUNABLE;
                cur->next=cur->next->next;
            }
            else{
                cur=cur->next;
            }
        }
     }
     return NULL; 
}
static int64_t uproc_uptime(task_t *task){
    uint64_t now=io_read(AM_TIMER_UPTIME).us/1000;
    return now;
}
static int uproc_fork(task_t *task){

    task_t *child=kalloc_safe(sizeof(task_t));
    uproc_create(child,"ch");
    
    uintptr_t rsp0=child->ctx->rsp0;
    memcpy(child->ctx,task->ctx,sizeof(Context));
    //child->ctx=task->ctx;
    void *cr3=child->ctx->cr3;
    child->ctx->cr3=cr3;
    child->ctx->rsp0=rsp0;
    
    child->ctx->GPRx=0;
    child->parent=task;
    child->np=task->np;
    child->ppid=task->pid;
    
    for(int i=0;i< task->np;i++){
        int sz=task->as.pgsize;
        void *va=task->va[i];
        void *pa=task->pa[i];
        void *new_pa=kalloc_safe((size_t)sz);
        memcpy(new_pa,pa,(size_t)sz);
        child->va[i]=va;
        child->pa[i]=new_pa;
        debug("new map %d: %p -> %p\n",child->pid,new_pa,va);
        map(&child->as,va,new_pa,MMAP_READ | MMAP_WRITE);
    }
    //printf("\n%d\n",child->pid);
    return child->pid;
}
static int uproc_wait(task_t *task, int *status){
    int flag=0;
    task->ret=NULL;
    for(task_t *now=task_head;now!=NULL;now=now->next){
        if(now->ppid==task->pid && now->status != EXIT){
            flag=1;
            if(now->status == ZOMBIE){
                now->status=EXIT;
                if(status != NULL){
                    for(int i=0;i<task->np;i++){
                        if((uintptr_t)task->va[i]==((uintptr_t)status& ~(task->as.pgsize-1L))){
                            uintptr_t addr=(uintptr_t)task->pa[i]+(uintptr_t)status-(uintptr_t)task->va[i];
                            *(int *)addr=now->e_staus;
                            break;
                        }
                    }
                }
                for(int i=0;i<now->np;i++){
                    kfree_safe(now->pa[i]);
                }
                kmt->teardown(now);
                //task->status=WAIT;
                //task->ret=status;
                return 0;
            //break;
            }
            
        }
    }
    if(flag==0){ //don't find
        return -1;
    }
    task->status=WAIT;
    task->ret=status;
    return 0;
}
static int uproc_exit(task_t *task, int status){
    //kmt->spin_lock(&traplock);
    task->e_staus=status;
    task->status=ZOMBIE;

    if(task->parent->status==WAIT){
        task->status=EXIT;
        task->parent->status=RUNABLE;
        if(task->parent->ret != NULL){
            for(int i=0;i<task->parent->np;i++){
                if((uintptr_t)task->parent->va[i]==((uintptr_t)task->parent->ret& ~(task->parent->as.pgsize-1L))){
                    uintptr_t addr=(uintptr_t)task->parent->pa[i]+(uintptr_t)task->parent->ret-(uintptr_t)task->parent->va[i];
                    *(int *)addr=status;
                    break;
                }
            }
            //*(task->parent->ret)=status;
            //printf("\n%p\n",task->parent->ret);
        }
        for(int i=0;i<task->np;i++){
            kfree_safe(task->pa[i]);
        }
        kmt->teardown(task);
    }
    /*if(task->wait){
        task->parent->status=RUNABLE;
        if(task->parent->ret != NULL){
            for(int i=0;i<task->parent->np;i++){
                if((uintptr_t)task->parent->va[i]==((uintptr_t)task->parent->ret& ~(task->parent->as.pgsize-1L))){
                    uintptr_t addr=(uintptr_t)task->parent->pa[i]+(uintptr_t)task->parent->ret-(uintptr_t)task->parent->va[i];
                    *(int *)addr=status;
                    break;
                }
            }
            *(task->parent->ret)=status;
            //printf("\n%p\n",task->parent->ret);
        }
        task->parent->ret = NULL;
    }*/
    //kmt->spin_unlock(&traplock);
    //panic("this is the init process which should not exit\n");
    return 0;
}
static int uproc_kill(task_t *task, int pid){
    return 0;
}
static void *uproc_mmap(task_t *task, void *addr, int length, int prot, int flags){
    return NULL;
}


static void *uproc_alloc(int size){
    return pmm->alloc((size_t)size);
}
static Context *pagefault(Event ev,Context *ctx){
    assert(ev.event==EVENT_PAGEFAULT);
    AddrSpace *as = &(task_cpu[cpu_current()]->as);
    
    void *va = (void *)(ev.ref & ~(as->pgsize-1L));
    if(va == as->area.start) {
        
        int num= (_init_len+as->pgsize-1)/ as->pgsize;   printf("%d\n",num);
        for(int i=0;i<num;i++){
            void *pa= kalloc_safe((size_t)as->pgsize);
            int size=((_init_len-i*as->pgsize) > as->pgsize) ? as->pgsize : (_init_len-i*as->pgsize);
            memcpy(pa,(void *)((uintptr_t)_init+i*as->pgsize),size);
            
            task_cpu[cpu_current()]->va[task_cpu[cpu_current()]->np]=va;//(void *)((uintptr_t)va+i*as->pgsize);
            task_cpu[cpu_current()]->pa[task_cpu[cpu_current()]->np]=pa;
            task_cpu[cpu_current()]->np++;
            debug("addr: %p map: %p -> %p\n",ev.ref,pa,va);
            map(as,va,pa,MMAP_READ | MMAP_WRITE); 
            va=(void *)((uintptr_t)va+as->pgsize);
        }
        
    }
    else{
        void *pa = kalloc_safe((size_t)as->pgsize);
        task_cpu[cpu_current()]->va[task_cpu[cpu_current()]->np]=va;
        task_cpu[cpu_current()]->pa[task_cpu[cpu_current()]->np]=pa;
        task_cpu[cpu_current()]->np++;
        debug("addr: %p map: %p -> %p\n",ev.ref,pa,va);
        map(as,va,pa,MMAP_READ | MMAP_WRITE);  
    }
    
    return NULL;
}
static Context *syscall(Event ev,Context *ctx){
    void *ret=NULL;
    //kmt->spin_unlock(&traplock);
    iset(true);
    assert(ev.event==EVENT_SYSCALL);
    switch (ctx->GPRx){
        case SYS_exit:{
            uproc_exit(task_cpu[cpu_current()],ctx->GPR1);
            break;
        }
        case SYS_kputc:{
            uproc_kputc(task_cpu[cpu_current()],ctx->GPR1);
            break;
        }
        case SYS_fork:{
            ctx->GPRx=uproc_fork(task_cpu[cpu_current()]);
            break;
        }
        case SYS_getpid:{
            ctx->GPRx=uproc_getpid(task_cpu[cpu_current()]);
            break;
        }
        case SYS_kill:{
            uproc_kill(task_cpu[cpu_current()],ctx->GPR1);
            break;
        }
        case SYS_sleep:{
            uproc_sleep(task_cpu[cpu_current()],ctx->GPR1);
            break;
        }
        case SYS_uptime:{
            ctx->GPRx=uproc_uptime(task_cpu[cpu_current()]);
            break;
        }
        case SYS_wait:{
            ctx->GPRx=uproc_wait(task_cpu[cpu_current()],(int *)ctx->GPR1);
            break;
        }
        case SYS_mmap:{
            uproc_mmap(task_cpu[cpu_current()],(void *)ctx->GPR1,ctx->GPR2,ctx->GPR3,ctx->GPR4);
        }
        default:{
            panic("undefined syscall\n");
        }

    }
    iset(false);

    return ret;
}
static void uproc_init(){
    vme_init(uproc_alloc, pmm->free);
    sleep_head.next=NULL;
    os->on_irq(0,EVENT_PAGEFAULT, pagefault);
    os->on_irq(0,EVENT_SYSCALL,   syscall);
    os->on_irq(0,EVENT_NULL,      uproc_wake);

}
int uproc_create(task_t *task, const char *name){
    kmt->spin_lock(&traplock);
    task->name=(char *)name;
    task->stack=kalloc_safe(STACK_SIZE);
    task->pid=(++process_pid);
    task->ppid=0;
    task->np=0;
    task->wait=0;
    task->parent=NULL;
    task->ret=NULL;
    panic_on(process_pid>32760,"the pid number is too large");
    Area ustack={.start=task->stack,.end=(void *)((uintptr_t)(task->stack)+STACK_SIZE)};
    protect(&task->as);
    task->ctx=ucontext(&task->as,ustack,(void *)task->as.area.start);
    // 中断时保存上下文的栈为内核栈（ustack）,
    // 而用户程序运行在用户栈（as->area.end),
    //（这一点通过rsp与rsp0两个寄存器实现）
    // which is different with kernal thread
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


MODULE_DEF(uproc) = {
    .init   = uproc_init,
    .kputc  = uproc_kputc,
    .getpid = uproc_getpid,
    .sleep  = uproc_sleep,
    .uptime = uproc_uptime,
    .fork   = uproc_fork,
    .wait   = uproc_wait,
    .exit   = uproc_exit,
    .kill   = uproc_kill,
    .mmap   = uproc_mmap,
};
