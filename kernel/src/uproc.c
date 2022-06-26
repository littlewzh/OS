#include <os.h>
#include <syscall.h>

#include "initcode.inc"
extern spinlock_t traplock;
extern task_t *task_head;
extern task_t *task_cpu[8]; 

static int uproc_kputc(task_t *task, char ch) {
  putch(ch); // safe for qemu even if not lock-protected
  return 0;
}
static int uproc_getpid(task_t *task){
    return 0;
}
static int uproc_sleep(task_t *task, int seconds){
    //printf("%d\n",seconds);
    uint64_t wakeup = io_read(AM_TIMER_UPTIME).us + 1000000L*seconds;
    while(io_read(AM_TIMER_UPTIME).us < wakeup){
        //printf("check\n");
        yield();
    }
    return 0;
}
static int64_t uproc_uptime(task_t *task){
    return 0;
}
static int uproc_fork(task_t *task){
    return 0;
}
static int uproc_wait(task_t *task, int *status){
    return 0;
}
static int uproc_exit(task_t *task, int status){
    panic("this is the init process which should not exit\n");
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
    void *pa = kalloc_safe((size_t)as->pgsize);
    void *va = (void *)(ev.ref & ~(as->pgsize-1L));

    if(va == as->area.start) {
        //printf("loader\n");
        memcpy(pa,_init,_init_len);
    }

    //printf("map: %p -> %p\n",pa,va);
    map(as,va,pa,MMAP_READ | MMAP_WRITE);
    return NULL;
}
static Context *syscall(Event ev,Context *ctx){
    void *ret=NULL;
    kmt->spin_unlock(&traplock);
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
            uproc_fork(task_cpu[cpu_current()]);
            break;
        }
        case SYS_getpid:{
            uproc_getpid(task_cpu[cpu_current()]);
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
            uproc_uptime(task_cpu[cpu_current()]);
            break;
        }
        case SYS_wait:{
            uproc_wait(task_cpu[cpu_current()],(int *)ctx->GPR1);
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
    os->on_irq(0,EVENT_PAGEFAULT, pagefault);
    os->on_irq(0,EVENT_SYSCALL,   syscall);

}
int uproc_create(task_t *task, const char *name){
    kmt->spin_lock(&traplock);
    task->name=(char *)name;
    task->stack=kalloc_safe(STACK_SIZE);
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
