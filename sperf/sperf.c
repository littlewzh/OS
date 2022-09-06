#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
extern char **environ;
struct sys_info{
    double time;
    char name[64];
}Syscall[1024];
double total_time=0;
int sys_max=0;
void sys_sort(){
    for(int i=0;i<sys_max;i++){
	    for(int j=i+1;j<sys_max;j++){
	        if(Syscall[i].time<Syscall[j].time){
		        struct sys_info tmp=Syscall[i];
		        Syscall[i]=Syscall[j];
		        Syscall[j]=tmp;
	        }
        }
    }
}
double sys_time(char *bufs){
    int k=strlen(bufs)-1;
    //while((char)bufs[k] != '<' && k>0) {k--;assert(k>=0);}
    for (k = strlen(bufs) - 1; k >= 0; --k) {if (bufs[k] == '<') break;}
    if (k < 0) return 0;

    double ptime;
    sscanf(bufs+k+1,"%lf",&ptime);
    total_time+=ptime;
    return ptime;
}
void sys_name(char *bufs){
    int k=0,i=0;
    //while((char)bufs[k] != '(' && k<1023) {k++;assert(k<1024);}
    for (k = 0; k < strlen(bufs); ++k) { if (bufs[k] == '(') break;}
    if(k==strlen(bufs)) {return;}

    for(i=0;i<sys_max;i++){
	    if(strncmp(bufs,Syscall[i].name,k)==0){
    //	    printf("%s ",Syscall[i].name);
	        Syscall[i].time+=sys_time(bufs);
	        return;
	    }
    }
    if(i==sys_max){
	    sys_max++;
   //	printf("%d\n",sys_max);
	    strncpy(Syscall[i].name,bufs,k);
        Syscall[i].name[k]='\0';
   //	printf("%s ",Syscall[i].name);
	    Syscall[i].time=sys_time(bufs);
	    return ;
    }
}
void update_print(){  
    sys_sort();
    printf("===================================\n");
    for(int i=0;i<5 && i<sys_max;i++){
        //assert(total_time!=0);
        double ratio=(Syscall[i].time/total_time)*100.0;
	    printf("%s(%.0lf%%)\n",Syscall[i].name,ratio);
    }
    for(int i=0;i<80;i++){printf("%c",'\0');}
    fflush(stdout);
}
static char path[4096];
static char Path[30][128];
int main(int argc, char *argv[],char *envp[]) {
    char buf[1024];
    char temp[1024];
    char *pathstart=getenv("PATH");
    assert(pathstart!=NULL);
    strcpy(path,pathstart);
    //
    char *exec_argv[argc+2];
    exec_argv[0]="strace";
    exec_argv[1]="-T";
    for(int i=1;i<argc;i++){
	    exec_argv[i+1]=argv[i];
    }
    exec_argv[argc+1]=NULL;
    char **env=environ;
    //
    int fildes[2];
    time_t begin = time(NULL);
    if(pipe(fildes)!=0){
	    perror("pipe failed");
	    exit(EXIT_FAILURE);
    }
    int pid=fork();
    if(pid == 0){
        close(fildes[0]);
	    dup2(fildes[1],STDERR_FILENO);
        char*token = strtok(path, ":");
        char stracePath[100];
        memset(stracePath, '\0', 100);
        strcat(stracePath, token);
        strcat(stracePath, "/strace");
        int fd=open("/dev/null",O_CREAT|O_RDWR|O_TRUNC,S_IRUSR|S_IWUSR);
        dup2(fd,STDOUT_FILENO);
        //close(STDOUT_FILENO);
        while((execve(stracePath, exec_argv, env)) == -1){
            memset(stracePath, '\0', 100);
            strcat(stracePath, strtok(NULL, ":"));
            strcat(stracePath, "/strace");
        }
        assert(0);
    }
    else{
	    FILE *input = fdopen(fildes[0],"r");
	    while(1){
	        memset(buf,'\0',sizeof(buf));
	        do{
	            fgets(temp,1024,input);
	            strcat(buf,temp);
	            if(strncmp(temp,"+++",3)==0){update_print();printf("===================================\n");fflush(stdout);exit(0);}         
	        }while(buf[strlen(buf)-2] != '>');
	        sys_name(buf);
	        time_t now = time(NULL);
	        if(now-begin > 1){
                update_print();	     
		        begin=now;
	        }
	    }
    }
    //perror(argv[0]);
    //exit(EXIT_FAILURE);
    return 0;
}