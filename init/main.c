#ifndef CONFIG_PATCH
#define CONFIG_PATCH
#endif

#define __LIBRARY__

#include <asm/io.h>
#include <asm/system.h>
#include <fcntl.h>
#include <linux/head.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

extern unsigned long startup_time;
int vsprintf(char *buf, const char *fmt, va_list args);

static int sprintf(char *str, const char *fmt, ...) {
  va_list args;
  int i;

  va_start(args, fmt);
  i = vsprintf(str, fmt, args);
  va_end(args);
  return i;
}

#define SYSCALL0(nr)                                                           \
  ({                                                                           \
    unsigned long __res = nr;                                                  \
    __asm__ __volatile__("int $0x80" : "+a"(__res));                           \
    __res;                                                                     \
  })

#define SYSCALL1(nr, arg0)                                                     \
  ({                                                                           \
    unsigned long __res = nr;                                                  \
    __asm__ __volatile__("int $0x80" : "+a"(__res) : "b"(arg0));               \
    __res;                                                                     \
  })

#define SYSCALL2(nr, arg0, arg1)                                               \
  ({                                                                           \
    unsigned long __res = nr;                                                  \
    __asm__ __volatile__("int $0x80" : "+a"(__res) : "b"(arg0), "c"(arg1));    \
    __res;                                                                     \
  })

#define SYSCALL3(nr, arg0, arg1, arg2)                                         \
  ({                                                                           \
    unsigned long __res = nr;                                                  \
    __asm__ __volatile__("int $0x80"                                           \
                         : "+a"(__res)                                         \
                         : "b"(arg0), "c"(arg1), "d"(arg2));                   \
    __res;                                                                     \
  })

extern int ROOT_DEV;
void blk_dev_init();

struct drive_info {
  char dummy[32];
} drive_info;

/*
 * This is set up by the setup-routine at boot-time
 */
#define EXT_MEM_K (*(unsigned short *)0x90002)
#define CON_ROWS ((*(unsigned short *)0x9000e) & 0xff)
#define CON_COLS (((*(unsigned short *)0x9000e) & 0xff00) >> 8)
#define DRIVE_INFO (*(struct drive_info *)0x90080)
#define ORIG_ROOT_DEV (*(unsigned short *)0x901FC)
#define ORIG_SWAP_DEV (*(unsigned short *)0x901FA)

/*
 * Yeah, yeah, it's ugly, but I cannot find how to do this correctly
 * and this seems to work. I anybody has more info on the real-time
 * clock I'd be interested. Most of this was trial and error, and some
 * bios-listing reading. Urghh.
 */

#define CMOS_READ(addr)                                                        \
  ({                                                                           \
    outb_p(0x80 | addr, 0x70);                                                 \
    inb_p(0x71);                                                               \
  })

#define BCD_TO_BIN(val) ((val) = ((val) & 15) + ((val) >> 4) * 10)

long kernel_mktime(struct tm *tm);

static void time_init(void) {
  struct tm time;

  do {
    time.tm_sec = CMOS_READ(0);
    time.tm_min = CMOS_READ(2);
    time.tm_hour = CMOS_READ(4);
    time.tm_mday = CMOS_READ(7);
    time.tm_mon = CMOS_READ(8);
    time.tm_year = CMOS_READ(9);
  } while (time.tm_sec != CMOS_READ(0));
  BCD_TO_BIN(time.tm_sec);
  BCD_TO_BIN(time.tm_min);
  BCD_TO_BIN(time.tm_hour);
  BCD_TO_BIN(time.tm_mday);
  BCD_TO_BIN(time.tm_mon);
  BCD_TO_BIN(time.tm_year);
  time.tm_mon--;
  startup_time = kernel_mktime(&time);
}

static void main_init();
long rd_init(long mem_start, int length);
void buffer_init(long buffer_end);

__attribute__((aligned(8))) desc_table idt;
;

__attribute__((aligned(8))) desc_ptr_t idt_ptr = {
    .length = sizeof(idt) - 1,
    .addr = idt,
};

__attribute__((aligned(8))) desc_table gdt;

__attribute__((aligned(8))) desc_ptr_t gdt_ptr = {
    .length = sizeof(gdt) - 1,
    .addr = gdt,
};

#define UL(x) ((unsigned long)(x))
#define BSS_SIZE (UL(&_ebss) - UL(&_bss))

static void enable_sse() {
  unsigned int cr4;

  __asm__ __volatile__("mov %%cr4, %0\n" : "=r"(cr4));

  // Set OSFXSR (bit 9) and OSXMMEXCPT (bit 10)
  cr4 |= (1 << 9) | (1 << 10);

  __asm__ __volatile__("mov %0, %%cr4\n" : : "r"(cr4));
}

#define FORK() SYSCALL0(__NR_fork)
#define SETSID() SYSCALL0(__NR_setsid)
#define SETUP(x) SYSCALL1(__NR_setup, x)
#define PAUSE() SYSCALL0(__NR_pause)
#define ALARM(x) SYSCALL1(__NR_alarm, x)
#define DUP(fd) SYSCALL1(__NR_dup, fd)
#define WRITE_S(fd, buf, len) SYSCALL3(__NR_write, fd, buf, len)
#define EXECVE(path, argv_rc, envp_rc)                                         \
  SYSCALL3(__NR_execve, path, argv_rc, envp_rc)

#define CLOSE(fd) SYSCALL1(__NR_close, fd)
#define SYNC() SYSCALL0(__NR_sync)

int open(const char *filename, int flag, ...);

char printbuf[1024];

int printf(const char *fmt, ...) {

  va_list args;
  int i;

  va_start(args, fmt);
  WRITE_S(1, printbuf, i = vsprintf(printbuf, fmt, args));
  va_end(args);
  return i;
}

static char *argv_rc[] = {"/bin/sh", NULL};
static char *envp_rc[] = {"HOME=/", NULL, NULL};

static char *argv[] = {"-/bin/sh", NULL};
static char *envp[] = {"HOME=/usr/root", NULL, NULL};

static char term[32];

#define CON_ROWS ((*(unsigned short *)0x9000e) & 0xff)
#define CON_COLS (((*(unsigned short *)0x9000e) & 0xff00) >> 8)

int main() {
  __asm__ __volatile__(
      "movl %0, %%esp\n\t"
      "movl %%esp, %%ebp\n\t" ::"r"(((unsigned long)&init_task) +
                                    PAGE_SIZE * INIT_STACK_PAGES));

  // setup fpu, write protection
  __asm__ __volatile__("mov %%cr0, %%eax\n"
                       "or  %0, %%eax\n"
                       "mov %%eax, %%cr0\n"
                       "fninit\n"
                       :
                       : "i"(0x10002)
                       : "eax");
  enable_sse();

  __asm__ __volatile__("xorl %%eax, %%eax\n"
                       "cld\n"
                       "rep\n"
                       "stosl\n"
                       :
                       : "c"(BSS_SIZE / 4), "D"(UL(&_bss))
                       : "eax");

  gdt[1].a = 0x0000ffff;
  gdt[1].b = 0x00CF9A00;

  gdt[2].b = 0x00CF9200;
  gdt[2].a = 0x0000ffff;

  __asm__ __volatile__("lidt %0\n"
                       "lgdt %1\n"
                       "jmp $(1<<3), $1f\n"
                       "movl $(2<<3), %%eax\n"
                       "movw %%ax, %%ds\n"
                       "movw %%ax, %%es\n"
                       "movw %%ax, %%fs\n"
                       "movw %%ax, %%gs\n"
                       "movw %%ax, %%ss\n"
                       "1:\n" ::"m"(idt_ptr),
                       "m"(gdt_ptr));

  memcpy(&drive_info, &DRIVE_INFO, sizeof(drive_info));
  sprintf(term, "TERM=con%dx%d", CON_COLS, CON_ROWS);

  mem_init();

  trap_init();
  blk_dev_init();
  chr_dev_init();
  tty_init();
  printk(term);
  printk("\n");

  time_init();
  sched_init();
  buffer_init(BUFFER_MEMORY_END);

  // ramdisk at 8MB, length = 32MB
  rd_init(RAMDISK_MEMORY_START, RAMDISK_MEMORY_SIZE);
  ROOT_DEV = 0x0101;

  __asm__ __volatile__("sti\n\t");

  move_to_user_mode(((char *)init_user_stack) + sizeof(init_user_stack));

  unsigned long pid;

  if (pid = FORK()) {
    // parent process become idle process
    while (1) {
      PAUSE();
    }
  } else {
    // child process sleep loop
    main_init();
    while (1) {
      ALARM(1);
      PAUSE();
    }
  }
}

static void main_init() {
  int pid, i;
  SETUP(&drive_info);
  open("/dev/tty1", O_RDWR, 0);
  DUP(0);
  DUP(0);

  printf("tty open ok\n");

  if (!(pid = FORK())) {
    // child process
    printf("fork pid = %d\n", pid);
    CLOSE(0);
    if (open("/etc/rc", O_RDONLY, 0))
      _exit(1);
    EXECVE("/bin/sh", argv_rc, envp_rc);
    printf("/etc/rc failed\n");
    _exit(2);
  }
  printf("fork pid = %d\n", pid);

  if (pid > 0)
    while (pid != wait(&i))
      ;

  printf("wait pid %d done\n", pid);

  /* nothing */;
  while (1) {
    if ((pid = FORK()) < 0) {
      printf("Fork failed in init\r\n");
      continue;
    }
    if (!pid) {
      CLOSE(0);
      CLOSE(1);
      CLOSE(2);
      SETSID();
      (void)open("/dev/tty1", O_RDWR, 0);
      (void)DUP(0);
      (void)DUP(0);
      _exit(EXECVE("/bin/sh", argv, envp));
    }
    while (1)
      if (pid == wait(&i))
        break;
    printf("\n\rchild %d died with code %04x\n\r", pid, i);
    SYNC();
  }
  _exit(0); /* NOTE! _exit, not exit() */
}
