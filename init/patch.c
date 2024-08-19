#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/tty.h>

void panic(const char *str) {
  __asm__ __volatile__("cli");
  console_print(str);
  while (1)
    ;
}


