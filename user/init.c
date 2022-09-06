#include "ulib.h"

void hello_test();
int dfs_fork();
int main() {
  // Example:
  //printf("pid = %d\n", getpid());
  //hello_test();
  dfs_fork();
  printstr("finish dfs-fork\n");
  return 0;
}




/*void hello_test(){
    int pid=fork(),x=0;
    char *fmt;
    if(pid==0){
      fmt="Parent0 #";
    }else{
      fmt="child0  #";
    }
    int pid2=fork();
    if(pid2==0){
      fmt="Parent1 #";
    }else{
      fmt="child1  #";
    }
    while(1){
      printstr(fmt);
      printnum(x++,10);
      kputc('\n');
      sleep(2);
    }
}*/

//#define printf printstr
#define DEST  '+'
#define EMPTY '.'
static struct move {
  int x, y, ch;
} moves[] = {
  { 0, 1, '>' },
  { 1, 0, 'v' },
  { 0, -1, '<' },
  { -1, 0, '^' },
};

static char map[][16] = {
  "#####",
  "#..+#",
  "##..#",
  "#####",
  "",
};

void display();

void dfs(int x, int y) {
  if (map[x][y] == DEST) {
    display();
  } else {
    sleep(1);
    int nfork = 0;

    for (struct move *m = moves; m < moves + 4; m++) {
      int x1 = x + m->x, y1 = y + m->y;
      if (map[x1][y1] == DEST || map[x1][y1] == EMPTY) {
        int pid = fork(); //assert(pid >= 0);
        if (pid == 0) { // map[][] copied
          if(pid != 0){exit(0);}
          map[x][y] = m->ch;
          dfs(x1, y1);
          exit(2); // clobbered map[][] discarded
        } else {
          nfork++;
          //waitpid(pid, NULL, 0); // wait here to serialize the search
        }
      }
    }
    int ret=0;
    while (nfork--) {//printnum(ret,10); printstr("\n");
    wait(&ret);//printnum(ret,10);ret=0;
    }
  }
  //while(1) {sleep(1);}
}

int dfs_fork(){
  dfs(1, 1);
  //while(1) {sleep(1);}
  while(wait(NULL)==0);
  return 1;

}

void display() {
  for (int i = 0; ; i++) {
    for (const char *s = map[i]; *s; s++) {
      switch (*s) {
        case EMPTY: printstr("   "); break;
        case DEST : printstr(" * "); break;
        case '>'  : printstr(" > "); break;
        case '<'  : printstr(" < "); break;
        case '^'  : printstr(" ^ "); break;
        case 'v'  : printstr(" v "); break;
        default   : printstr("###"); break;
      }
    }
    printstr("\n");
    if (strlen(map[i]) == 0) break;
  }
  //sleep(1); // to see the effect of parallel search
}