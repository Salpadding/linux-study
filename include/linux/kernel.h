#ifndef _KERNEL_H
#define _KERNEL_H

/*
 * 'kernel.h' contains some often-used function prototypes etc
 */
void verify_area(void *addr, int count);
extern void do_exit(long error_code);
int printf(const char *fmt, ...);
int printk(const char *fmt, ...);
void console_print(const char *str);
int tty_write(unsigned ch, char *buf, int count);
void *malloc(unsigned int size);
void free_s(void *obj, int size);
extern void hd_times_out(void);
extern void sysbeepstop(void);
extern void blank_screen(void);
extern void unblank_screen(void);

extern int beepcount;
extern int hd_timeout;
extern int blankinterval;
extern int blankcount;

#define free(x) free_s((x), 0)

/*
 * This is defined as a macro, but at some point this might become a
 * real subroutine that sets a flag if it returns true (to do
 * BSD-style accounting where the process is flagged if it uses root
 * privs).  The implication of this is that you should do normal
 * permissions checks first, and check suser() last.
 */
#define suser() (current->euid == 0)

extern char _end;
extern char _bss;
extern char _ebss;
extern char _edata;
extern char _text;
extern char _etext;
extern char _erodata;
extern char _data;

typedef struct {
  unsigned short length;
  void *addr;
} __attribute__((packed)) desc_ptr_t;

void mem_init();
void trap_init(void);
void chr_dev_init(void);
void tty_init(void);
void sched_init(void);
void console_print(const char *x);

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define panic(x)                                                               \
  ({                                                                           \
    __asm__ __volatile__("cli\n\t");                                           \
    console_print(x);                                                          \
    while (1)                                                                  \
      ;                                                                        \
  })

#define INIT_STACK_PAGES 4

#endif
