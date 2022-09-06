#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <dlfcn.h> 
//#define LOCAL
#ifdef LOCAL
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...) 
#endif
static int number=0;
static int lib=0;
static int file_num=0;
int my_system(char *cmd){
  assert(cmd!=NULL);
  int pid=fork();
  int status;
  if(pid==0){
    //this is the child process
    execl("/bin/sh", "sh", "-c", cmd, (char *) NULL);
    printf("COMMAND ERROR!\n");
    return 1;//exit(0);
  }
  else{
    waitpid(pid,&status,0);
    
  }
  return 0;
}
void *load(char *content){
  file_num++;
  char filename[128],filename_so[128];
  sprintf(filename,"/tmp/%d.c",file_num);
  sprintf(filename_so,"/tmp/%d.so",file_num);
  debug("%s\n",filename);
  FILE *fd=fopen(filename,"w");
  fprintf(fd,"%s",content);
  fflush(fd);
  void *handle=NULL;
  char command[1024];
#ifdef __x86_64__
  sprintf(command,"gcc -w -fPIC -m64 -shared %s -o %s",filename,filename_so);
#else
  sprintf(command,"gcc -w -fPIC -m32 -shared %s -o %s",filename,filename_so);
#endif
  if(my_system(command)==0){
    handle=dlopen(filename_so,RTLD_GLOBAL|RTLD_NOW);
    if (!handle) {  
      debug("%s\n", dlerror());  
    }
  }
  return handle; 
}
void *create(char *buf){
  char fun[1024];
  sprintf(fun,"int __expr_wrapper_%d(){ return %s;}",number,buf);
  //debug("%s\n",fun);
  char name[32];
  sprintf(name,"__expr_wrapper_%d",number);
  void *phandle=load(fun);
  //while(1);
  return dlsym(phandle,name);
}
int main(int argc, char *argv[]) {
  static char line[4096];
  while (1) {
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }
    //debug("Got %zu chars : %s", strlen(line),line); // ??
    if(strncmp(line,"int",3)==0){
      //the input is function
      void *phandle=load(line);
      if(phandle!=NULL) printf("ADD FUNCTION SUCCESS\n");
    }
    else{
      line[strlen(line)-1]='\0';
      number++;
      int (*call)()=create(line);
      //assert(call !=NULL);
      if(call==NULL){printf("COMPILER ERROR!\n");continue;}
      int result=call();
      printf("%d\n",result);
      //
    }
    //my_system(line);
  }
}
