#include <game.h>

// Operating system is a C program!
/*int main(const char *args) {
  ioe_init();

  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");

  splash();

  puts("Press any key to see its key code...\n");
  while (1) {
    int flag=print_key();
    if(flag == 1) return 0;//printf("%d\n",flag);
    
  }
  return 0;
}*/
#define FPS 10 
void kbd_event(int key){
  

}
int uptimer(){
  AM_TIMER_UPTIME_T timer;
  ioe_read(AM_TIMER_UPTIME,&timer);
  return timer.us;
}
void screen_update(){


}
void game_progress(){


}
int main(const char *args){
  ioe_init();
  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");

  splash();

  int next_frame = 0 , key ;
  
  while (1) {
    while (uptimer() < next_frame) ; // 等待一帧的到来
    while ((key = print_key()) != AM_KEY_NONE) {
      if(key==1) return 0;
      kbd_event(key);         // 处理键盘事件
    }
  game_progress();          // 处理一帧游戏逻辑，更新物体的位置等
  screen_update();          // 重新绘制屏幕
  next_frame += 1000 / FPS; // 计算下一帧的时间
}
  return 0;
}