#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {           //不包括‘\0’
  assert(s!=NULL);
  char *ps=(char *)s;
  int ret=0;
  while(*ps++){
    ret++;
  }
  return ret;
  //panic("Not implemented");
}

char *strcpy(char *dst, const char *src) {       //会拷贝’\0’
  assert(dst != NULL && src != NULL);
    char *ret = dst; 
    while ((*dst++=*src++)!='\0');
    return ret;
}

char *strncpy(char *dst, const char *src, size_t n) {               //目前无论是对与strcpy，还是strncpy，我都没有去考虑内存区重叠这种情况。
  assert(dst!=NULL&&src!=NULL&&n<=0);
  int size=strlen(src);
  if(n>size) {return strncpy(dst,src,size);}
  else {
    int cnt=n;
    char *ret=dst;
    while(cnt--){
      *dst++=*src++;
    }
    *dst++='\0';                    //strncpy不会去帮你管最后的‘\0’,所以需要我们手动加上
    return ret;                  
  }
  
  //panic("Not implemented");
}

char *strcat(char *dst, const char *src) {
  assert(dst != NULL && src != NULL);
  char *tmp=dst;
  while(*dst){
    dst++;

  }
  while((*dst++=*src++)!='\0');
  return tmp;
  //panic("Not implemented");
}

int strcmp(const char *s1, const char *s2) {
  int ret = 0;  
    while(!(ret=*(unsigned char*)s1-*(unsigned char*)s2) && *s1&&*s2){  
        s1++;  
        s2++;
    }  
    if(ret < 0){  
        return -1;  
    }  
    else if(ret > 0){  
        return 1;
    }  
    return 0;
  //panic("Not implemented");
}

int strncmp(const char *s1, const char *s2, size_t n) {
    int ret = 0;  
    int cnt=n;
    while(!(ret=*(unsigned char*)s1-*(unsigned char*)s2) &&cnt--){  
        s1++;  
        s2++;
    }  
    if(ret < 0){  
        return -1;  
    }  
    else if(ret > 0){  
        return 1;
    }  
    return 0;
  //panic("Not implemented");
}

void *memset(void *s, int c, size_t n) {
  assert(s!=NULL);
  char *ps=(char *)s;
  char pc=(char)c;
  while(n--){
    *ps++=pc;

  }
  return s;
  //nic("Not implemented");
}

void *memmove(void *dst, const void *src, size_t n) {
  return memcpy(dst,src,n);
  //panic("Not implemented");
}

void *memcpy(void *out, const void *in, size_t n) {
  //if(out==NULL||in==NULL) {return NULL;}
  assert(out!=NULL&&in!=NULL);
  char *pout=(char *)out;
  char *pin=(char *)in;
  if(pout>pin&&pout<pin+n){
    pout=pout+n-1;
    pin=pin+n-1;
    while(n--){
    *pout=*pin;
    pout--;
    pin--;
    }
  }
  else {
    while(n--){
      *pout=*pin;
      pout++;
      pin++;
    }
  }
  return out;
  //panic("Not implemented");
}

int memcmp(const void *s1, const void *s2, size_t n) {
  
  char *ps1=(char *)s1;
  char *ps2=(char *)s2;
  //assert(ps1!=NULL||ps2!=NULL);
  if(ps1==NULL||ps2==NULL||n<0){
    return 0;
  }
  while(n--){
    if(*ps1==*ps2){
      continue;
    }
    else {break;}
  }
  return (*ps1-*ps2);
  //panic("Not implemented");
}

#endif