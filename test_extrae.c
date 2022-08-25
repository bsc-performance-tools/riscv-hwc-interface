#include "extrae.h"
int main(){
  Extrae_init();
  for(int i = 0; i < 100; i++){
    Extrae_eventandcounters(100, i);
    //Extrae_next_hwc_set();
    //asm volatile ("vsetvli zero, %0, e64, m1"::"r"(i*3%256));
    asm volatile ("vsetvli zero, %0, e64, m1"::"r"(100));
    for(int j = 0; j < 10000; j++){
      asm volatile ("vfadd.vv v0, v0, v0");
      asm volatile ("vfadd.vv v0, v0, v0");
      asm volatile ("vfadd.vv v0, v0, v0");
      asm volatile ("vfadd.vv v0, v0, v0");
      asm volatile ("vfadd.vv v0, v0, v0");
      asm volatile ("vfadd.vv v0, v0, v0");
      asm volatile ("vfadd.vv v0, v0, v0");
      asm volatile ("vfadd.vv v0, v0, v0");
      asm volatile ("vfadd.vv v0, v0, v0");
      asm volatile ("vfadd.vv v0, v0, v0");
      asm volatile ("vfadd.vv v0, v0, v0");
      asm volatile ("vfadd.vv v0, v0, v0");
    }
  }
  Extrae_fini();
}
