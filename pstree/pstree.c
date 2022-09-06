#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>
int p,v,n;                   //the flag of the parament of pstree
struct dir_node{
  int pid;                   //pid of the directory
  int ppid;                  //parent pid
  char stats;
  char name[100];            //the name of the directory
}proc[1024];
int counter=0,total=0;
struct dirent **namelist;    //store the information of the directory which is satisfied the request
int head[1024];
struct edge{
  int to;
  //int we;
  int next;
}e[1024];
int cnt=0;
void addedge(int u,int v){
  cnt++;
  e[cnt].to=v;
  //e[cnt].we=w;
  e[cnt].next=head[u];
  head[u]=cnt;
}
//queue<int> q;
void dfs(int x,int deep){
  if(p==1) { printf("%s(%d)\n",proc[x].name,proc[x].pid);}
  else {printf("%s\n",proc[x].name);}
  for(int i=head[x];i;i=e[i].next){
    for(int i=0;i<=deep;i++){printf("    ");}
    int y=e[i].to;
    dfs(y,deep+1);
  }
}
/*int num[1024];
void addedge(int x,int y,int z){
  
}*/
int selectfile(const struct dirent *dir){       //to judge the directory's name is number判断是否是数字目录
  int len=strlen(dir->d_name);
  for(int i=0;i<len;i++){
    if(isdigit(dir->d_name[i])==0) return 0;
  }
  return 1;
}
void getinfo(char *name){   //read the information of the direcotry and write them to proc[] 
  char s[20];
  strcpy(s,"/proc/");
  strcat(s,name);
  strcat(s,"/stat");
  //printf("%s\n",s);
  char string[100];
  FILE *fp=fopen(s,"r");
  if(fp){
    fgets(string,50,fp);
    char name[20];
    sscanf(string,"%d %s %c %d",&proc[counter].pid,name,&proc[counter].stats,&proc[counter].ppid);
    //printf("%s\n",name);
    sscanf(name,"(%[^)]",proc[counter].name);
    //printf("%s\n",string);
    //printf("%d %d %s\n",proc[counter].pid,proc[counter].ppid,proc[counter].name);
    fclose(fp);
    counter++;
  }
  else{assert(0);}
}
int  proc_sort(int flag,struct dirent **list){
  if(flag==0) return 0;
  else {
    int a,b;
    struct dirent *t;
    for(int i=0;i<total;i++){
      for(int j=i+1;j<total;j++){
        a=atoi(list[i]->d_name);
        t=list[i];
        b=atoi(list[j]->d_name);
        if(a<b){
          list[i]=list[j];
          list[j]=t;
        }
      }
    }
    return 0;
  }
}
int main(int argc, char *argv[]) {
  int ch;
  for(int i=1;i<argc;i++){
    if(strcmp("--show-pids",argv[i])==0||strcmp("-p",argv[i])==0) p=1;
    if(strcmp("--numeric-sort",argv[i])==0||strcmp("-n",argv[i])==0) n=1;
    if(strcmp("--version",argv[i])==0||strcmp("-V",argv[i])==0) v=1;
 
  }
  /*while((ch=getopt(argc,argv,"npV")) != -1){
    if(ch=='p') p=1;
    if(ch=='n') n=1;
    if(ch=='V') v=1;
  }*/
  if(v==1){
    fprintf(stderr,"this is littlewzh's pstree\n");
    return 0;
  }
  int s=0;
  //dir_node proc[1024];
  total=scandir("/proc",&namelist,selectfile,alphasort);
  //printf("total directory : %d\n",total);
  proc_sort(n,namelist);
  for(int i=0;i<total;i++){
    getinfo(namelist[i]->d_name);
  }
  memset(head,0,sizeof(head));
  for(int i=0;i<counter;i++){
    //if(proc[i].ppid=1) s=i;
    for(int j=0;j<counter;j++){
      if(proc[i].pid==proc[j].ppid) addedge(i,j);
    }
  }
  //printf("total directory : %d\ncounter : %d\n",total,counter);
  
  for(int i=0;i<counter;i++){
    if(proc[i].ppid==0) dfs(i,0);
  }
  /*for(int i=0;i<counter;i++){
    printf("%d %d %s\n",proc[counter].pid,proc[counter].ppid,proc[counter].name);
  }*/
  /*for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  printf("--------------------------\n");
  int ch;
  while((ch=getopt(argc,argv,"npv")) != -1 ){
    printf("optind: %d\n",optind);
    printf("arg: %c\n",ch);
    printf("--------------------------\n");
  }*/ 
  assert(!argv[argc]);
  
  
  return 0;
}
