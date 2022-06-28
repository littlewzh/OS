#include <common.h>
#define PAGESIZE 16384//65536//32768


//lock
static inline int xchg(int *addr, int newval) {
  int result;
  asm volatile ("lock xchg %0, %1":
    "+m"(*addr), "=a"(result) : "1"(newval) : "cc", "memory");
  return result;
}
static int glocked;
static void lock_init(){glocked=0;}
static void lock(int *lock) { while (xchg(lock, 1)); }
static void unlock(int *lock) { xchg(lock, 0); }
//lock
//some convention function
extern int __am_ncpu;
extern int cpu_current();
//static uintptr_t MAX(uintptr_t a,uintptr_t b){return (a>b)?a:b;} 
static uintptr_t MIN(uintptr_t a,uintptr_t b){return (a<b)?a:b;} 
static size_t trans(size_t size){
  int k=4;
  while((1<<k)<size) k++;
  return (size_t)(1<<k);
}
static int lg(size_t size){
  size/=16;
  int k=0;
  while(size>1){size/=2;k++;}
  return k;
}
//some convention function
typedef struct pagenode{
  int size;
  int spinlock;
  int cpu;
  int sp;
  uintptr_t start,end;
  struct pagenode *fnext;
  struct pagenode *pre;
  struct pagenode *next;
  int stack[512];
}page_t;
//typedef struct list{
static page_t *freehead;
static page_t *freetrail;
static page_t *freecurrent;
//}list_t;
static page_t *slab[8][10];
//list_t freelist;

void page_init(page_t *now,size_t size,int ncpu){
  now->size=size;
  now->spinlock=0;
  now->cpu=ncpu;
  now->next=NULL;
  now->sp=0;
  for(int i=0;i<((PAGESIZE-4096)/size) && (i<510);i++){
    now->sp++;
    now->stack[now->sp]=i;
  }
}

void *page_alloc(int num){
  lock(&glocked);
  if(num==1){
    page_t *cur=freecurrent;
    while(cur->fnext != NULL){
      if(cur->fnext->start >= cur->end+PAGESIZE){
        page_t *new=(page_t *)(cur->end);
        new->start=cur->end;
        new->end=new->start+PAGESIZE;
        new->fnext=cur->fnext;
        new->pre=cur;
        cur->fnext->pre=new;
        cur->fnext=new;
        freecurrent=new;
        unlock(&glocked);
        return new;
      }
      cur=cur->fnext;
    }
  }else{
    //lock(&glocked);
    page_t *cur=freecurrent;
    size_t pagesize=(num-1)*PAGESIZE;
    while(cur->fnext != NULL){
      if(cur->fnext->start >= (((PAGESIZE)+cur->end+pagesize)&(~(pagesize-1)))+pagesize){
        page_t *new=(page_t *)((((PAGESIZE)+cur->end+pagesize)&(~(pagesize-1)))-PAGESIZE);
        debug("%x %x %x ",cur->fnext->start,pagesize,new);
        new->start=(uintptr_t)new;
        new->end=new->start+PAGESIZE+pagesize;
        new->fnext=cur->fnext;
        new->pre=cur;
        cur->fnext->pre=new;
        cur->fnext=new;
        unlock(&glocked);
        return new;
      }
      cur=cur->fnext;
    }
  }
  unlock(&glocked);
  return NULL;
}
void list_init(){
  //uintptr_t s=(uintptr_t)heap.start+512;
  freehead=(page_t *)ROUNDUP(heap.start,PAGESIZE);
  freetrail=(page_t *)((uintptr_t)ROUNDDOWN(heap.end,PAGESIZE)-PAGESIZE);
  freehead->fnext=freetrail;
  freehead->pre=NULL;
  freehead->start=(uintptr_t)freehead;
  freehead->end=freehead->start+PAGESIZE;
  freetrail->fnext=NULL;
  freetrail->pre=freehead;
  freetrail->start=(uintptr_t)freetrail;
  freetrail->end=(uintptr_t)freetrail->start+PAGESIZE;
  freecurrent=freehead;
  debug("%x %x %x\n",freehead,freecurrent,freetrail);
}
// 框架代码中的 pmm_init (在 AbstractMachine 中运行)

static void pmm_init() {
  //uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  //printf("Got %d MiB heap: [%p, %p]\n", pmsize >> 20, heap.start, heap.end);
  lock_init();
  list_init();
  for(int i=0;i<__am_ncpu;i++){
    int size=16;
    for(int j=0;j<9;j++){
      slab[i][j]=(page_t *)page_alloc(1);
      debug("%d %x\n",size,slab[i][j]);
      page_init(slab[i][j],size,i);
      size*=2;
    }
  }
}
static void *fast_alloc(int k,int ncpu,size_t size){
  page_t *cur=slab[ncpu][k];
  //lock(&cur->spinlock);
  lock(&cur->spinlock);
  while(cur!=NULL){
    if(cur->sp==0){unlock(&cur->spinlock);cur=cur->next;if(cur!=NULL){lock(&cur->spinlock);}continue;}
    debug(" start=%x sp=%d %d ",cur->start,cur->sp,cur->stack[cur->sp]);
    void *ret=(void *)(cur->start+4096+(cur->stack[cur->sp])*size);
    cur->sp--;
    unlock(&cur->spinlock);
    return ret;
  }
  return NULL;
}
static void *slow_alloc(size_t size){
  int pagenum=size/PAGESIZE+1;
  page_t *new=page_alloc(pagenum);
  if(new==NULL){return NULL;}
  void *ret=(void *)(new->start+MIN(PAGESIZE,size));
  new->size=size;
  return ret;
}
static void *kalloc(size_t size) {
  int ncpu=cpu_current();
  size_t real_size=trans(size);
  int k=lg(real_size);
  debug("cpu=%d size=%d realsize=%d k=%d ",ncpu,size,real_size,k);
  void *ret=NULL;
  if(real_size > 16*1024*1024){return NULL;}
  if(real_size <= 4096){
    ret=fast_alloc(k,ncpu,real_size);
    if(ret==NULL){
      page_t *new=page_alloc(1);
      page_init(new,real_size,ncpu);
      page_t *temp=slab[ncpu][k]->next;
      slab[ncpu][k]->next=new;
      new->next=temp;
      lock(&new->spinlock);
      ret=(void *)(new->start+4096+(new->stack[new->sp])*real_size);
      new->sp--;
      unlock(&new->spinlock);
    }
    debug("addr=%x\n",ret);
    if((uintptr_t)ret%real_size!=0){return NULL;}//still have some strange incorrent alignment problem
    assert((uintptr_t)ret%real_size==0);
    return ret;
  }
  else{
    ret=slow_alloc(real_size);
    debug("addr=%x\n",ret);
    //if((uintptr_t)ret%real_size!=0){return NULL;}
    return ret;
  }
  return NULL;
}

static void kfree(void *ptr) {
  if((uintptr_t)ptr % PAGESIZE !=0){
    page_t *page=(page_t *)((uintptr_t)ptr & ~(PAGESIZE-1));
    if(page->size <= 4096){
      lock(&page->spinlock);
      page->sp++;
      page->stack[page->sp]=((uintptr_t)ptr-page->start-4096)/(page->size);
      unlock(&page->spinlock);
    }else{
      lock(&glocked);
      page->pre->fnext=page->fnext;
      page->fnext->pre=page->pre;
      freecurrent=page->pre;
      unlock(&glocked);
    }
  }else{
    page_t *page=(page_t *)((uintptr_t)ptr-PAGESIZE);
    lock(&glocked);
    page->pre->fnext=page->fnext;
    page->fnext->pre=page->pre;
    freecurrent=page->pre;
    unlock(&glocked);
  }
}
MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
