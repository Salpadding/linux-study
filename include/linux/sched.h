#ifndef _SCHED_H
#define _SCHED_H

typedef struct {
  unsigned short limit_low;
  unsigned short base_low;
  unsigned char base_mid;
  unsigned char access;
  unsigned char granularity;
  unsigned char base_high;
} __attribute__((packed)) gdt_entry_t;

#define HZ 100

#define NR_TASKS 64
#define TASK_SIZE 0x04000000
#define LIBRARY_SIZE 0x00400000

#if (TASK_SIZE & 0x3fffff)
#error "TASK_SIZE must be multiple of 4M"
#endif

#if (LIBRARY_SIZE & 0x3fffff)
#error "LIBRARY_SIZE must be a multiple of 4M"
#endif

#if (LIBRARY_SIZE >= (TASK_SIZE / 2))
#error "LIBRARY_SIZE too damn big!"
#endif

#if (((TASK_SIZE >> 16) * NR_TASKS) != 0x10000)
#error "TASK_SIZE*NR_TASKS must be 4GB"
#endif

#define LIBRARY_OFFSET (TASK_SIZE - LIBRARY_SIZE)

#define CT_TO_SECS(x) ((x) / HZ)
#define CT_TO_USECS(x) (((x) % HZ) * 1000000 / HZ)

#define FIRST_TASK task[0]
#define LAST_TASK task[NR_TASKS - 1]

#include <linux/fs.h>
#include <linux/head.h>
#include <linux/mm.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/time.h>

#if (NR_OPEN > 32)
#error                                                                         \
    "Currently the close-on-exec-flags and select masks are in one long, max 32 files/proc"
#endif

#define TASK_RUNNING 0
#define TASK_INTERRUPTIBLE 1
#define TASK_UNINTERRUPTIBLE 2
#define TASK_ZOMBIE 3
#define TASK_STOPPED 4

#ifndef NULL
#define NULL ((void *)0)
#endif

extern int copy_page_tables(unsigned long from, unsigned long to, long size);
extern int free_page_tables(unsigned long from, unsigned long size);

extern void sched_init(void);
extern void schedule(void);
extern void trap_init(void);
extern int tty_write(unsigned minor, char *buf, int count);

typedef int (*fn_ptr)();

struct i387_struct {
  long cwd;
  long swd;
  long twd;
  long fip;
  long fcs;
  long foo;
  long fos;
  long st_space[20]; /* 8*10 bytes for each FP-reg = 80 bytes */
};

struct tss_struct {
  long back_link; /* 16 high bits zero */
  long esp0;
  long ss0; /* 16 high bits zero */
  long esp1;
  long ss1; /* 16 high bits zero */
  long esp2;
  long ss2; /* 16 high bits zero */
  long cr3;
  long eip;
  long eflags;
  long eax, ecx, edx, ebx;
  long esp;
  long ebp;
  long esi;
  long edi;
  long es;           /* 16 high bits zero */
  long cs;           /* 16 high bits zero */
  long ss;           /* 16 high bits zero */
  long ds;           /* 16 high bits zero */
  long fs;           /* 16 high bits zero */
  long gs;           /* 16 high bits zero */
  long ldt;          /* 16 high bits zero */
  long trace_bitmap; /* bits: trace 0, bitmap 16-31 */
  struct i387_struct i387;
};

struct task_struct {
  /* these are hardcoded - don't touch */
  long state; /* -1 unrunnable, 0 runnable, >0 stopped */
  long counter;
  long priority;
  long signal;
  struct sigaction sigaction[32];
  long blocked; /* bitmap of masked signals */
                /* various fields */
  int exit_code;
  unsigned long start_code, end_code, end_data, brk, start_stack;
  long pid, pgrp, session, leader;
  int groups[NGROUPS];
  /*
   * pointers to parent process, youngest child, younger sibling,
   * older sibling, respectively.  (p->father can be replaced with
   * p->p_pptr->pid)
   */
  struct task_struct *p_pptr, *p_cptr, *p_ysptr, *p_osptr;
  unsigned short uid, euid, suid;
  unsigned short gid, egid, sgid;
  unsigned long timeout, alarm;
  long utime, stime, cutime, cstime, start_time;
  struct rlimit rlim[RLIM_NLIMITS];
  unsigned int flags; /* per process flags, defined below */
  unsigned short used_math;
  /* file system info */
  int tty; /* -1 if no tty, so it must be signed */
  unsigned short umask;
  struct m_inode *pwd;
  struct m_inode *root;
  struct m_inode *executable;
  struct m_inode *library;
  unsigned long close_on_exec;
  struct file *filp[NR_OPEN];
  /* ldt for this task 0 - zero 1 - cs 2 - ds&ss */
  struct desc_struct ldt[3];
  /* tss for this task */
  struct tss_struct tss;
};

/*
 * Per process flags
 */
#define PF_ALIGNWARN 0x00000001 /* Print alignment warning msgs */
                                /* Not implemented yet, only for 486*/

/*
 *  INIT_TASK is used to set up the first task table, touch at
 * your own risk!. Base=0, limit=0x9ffff (=640kB)
 */
#define INIT_TASK                                                              \
  /* state etc */ {                                                            \
      0,                                                                       \
      15,                                                                      \
      15,                                                                      \
      /* signals */ 0,                                                         \
      {                                                                        \
          {},                                                                  \
      },                                                                       \
      0,                                                                       \
      /* ec,brk... */ 0,                                                       \
      0,                                                                       \
      0,                                                                       \
      0,                                                                       \
      0,                                                                       \
      0,                                                                       \
      /* pid etc.. */ 0,                                                       \
      0,                                                                       \
      0,                                                                       \
      0, /* suppl grps*/                                                       \
      {                                                                        \
          NOGROUP,                                                             \
      },                                                                       \
      /* proc links*/ &init_task.task,                                         \
      0,                                                                       \
      0,                                                                       \
      0,                                                                       \
      /* uid etc */ 0,                                                         \
      0,                                                                       \
      0,                                                                       \
      0,                                                                       \
      0,                                                                       \
      0,                                                                       \
      /* timeout */ 0,                                                         \
      0,                                                                       \
      0,                                                                       \
      0,                                                                       \
      0,                                                                       \
      0,                                                                       \
      0, /* rlimits */                                                         \
      {{0x7fffffff, 0x7fffffff},                                               \
       {0x7fffffff, 0x7fffffff},                                               \
       {0x7fffffff, 0x7fffffff},                                               \
       {0x7fffffff, 0x7fffffff},                                               \
       {0x7fffffff, 0x7fffffff},                                               \
       {0x7fffffff, 0x7fffffff}},                                              \
      /* flags */ 0,                                                           \
      /* math */ 0,                                                            \
      /* fs info */ -1,                                                        \
      0022,                                                                    \
      NULL,                                                                    \
      NULL,                                                                    \
      NULL,                                                                    \
      NULL,                                                                    \
      0, /* filp */                                                            \
      {                                                                        \
          NULL,                                                                \
      },                                                                       \
      {                                                                        \
          {0, 0},                                                              \
          /* ldt */ {0x000003ff, 0x00c0fa00},                                  \
          {0x000003ff, 0x00c0f200},                                            \
      },                                                                       \
      /*tss*/ {0,       PAGE_SIZE * INIT_STACK_PAGES + (long)&init_task,       \
               0x10,    0,                                                     \
               0,       0,                                                     \
               0,       (long)&pg_dir,                                         \
               0,       0,                                                     \
               0,       0,                                                     \
               0,       0,                                                     \
               0,       0,                                                     \
               0,       0,                                                     \
               0x17,    0x17,                                                  \
               0x17,    0x17,                                                  \
               0x17,    0x17,                                                  \
               _LDT(0), 0x80000000,                                            \
               {}},                                                            \
  }

extern struct task_struct *task[NR_TASKS];
extern struct task_struct *last_task_used_math;
extern struct task_struct *current;
extern unsigned long volatile jiffies;
extern unsigned long startup_time;
extern int jiffies_offset;

#define CURRENT_TIME (startup_time + (jiffies + jiffies_offset) / HZ)

extern void add_timer(long jiffies, void (*fn)(void));
extern void sleep_on(struct task_struct **p);
extern void interruptible_sleep_on(struct task_struct **p);
extern void wake_up(struct task_struct **p);
extern int in_group_p(gid_t grp);

/*
 * Entry into gdt where to find first TSS. 0-nul, 1-cs, 2-ds, 3-syscall
 * 4-TSS0, 5-LDT0, 6-TSS1 etc ...
 */
#define FIRST_TSS_ENTRY 4
#define FIRST_LDT_ENTRY (FIRST_TSS_ENTRY + 1)
#define _TSS(n) ((((unsigned long)n) << 4) + (FIRST_TSS_ENTRY << 3))
#define _LDT(n) ((((unsigned long)n) << 4) + (FIRST_LDT_ENTRY << 3))
#define ltr(n) __asm__("ltr %%ax" ::"a"(_TSS(n)))
#define lldt(n) __asm__("lldt %%ax" ::"a"(_LDT(n)))
#define str(n)                                                                 \
  __asm__("str %%ax\n\t"                                                       \
          "subl %2,%%eax\n\t"                                                  \
          "shrl $4,%%eax"                                                      \
          : "=a"(n)                                                            \
          : "a"(0), "i"(FIRST_TSS_ENTRY << 3))
/*
 *	switch_to(n) should switch tasks to task nr n, first
 * checking that n isn't the current task, in which case it does nothing.
 * This also clears the TS-flag if the task we switched to has used
 * tha math co-processor latest.
 */
#define switch_to(n)                                                           \
  {                                                                            \
    struct {                                                                   \
      long a, b;                                                               \
    } __tmp;                                                                   \
    __asm__("cmpl %%ecx,current\n\t"                                           \
            "je 1f\n\t"                                                        \
            "movw %%dx,%1\n\t"                                                 \
            "xchgl %%ecx,current\n\t"                                          \
            "ljmp %0\n\t"                                                      \
            "cmpl %%ecx,last_task_used_math\n\t"                               \
            "jne 1f\n\t"                                                       \
            "clts\n"                                                           \
            "1:" ::"m"(*&__tmp.a),                                             \
            "m"(*&__tmp.b), "d"(_TSS(n)), "c"((long)task[n]));                 \
  }
// jackeylea

#define PAGE_ALIGN(n) (((n) + 0xfff) & 0xfffff000)

static void _set_base(char *addr, unsigned long base) {
  *((unsigned short *)(&addr[2])) = (unsigned short)(base & 0xffff);
  addr[4] = (char)((base >> 16) & 0xff);
  addr[7] = (char)((base >> 24) & 0xff);
}

static void _set_limit(char *addr, unsigned long limit) {
  *((unsigned short *)(addr)) = (unsigned short)(limit & 0xffff);
  addr[6] = (addr[6] & 0xf0) | ((char)((limit >> 16) & 0xf));
}

#define set_base(ldt, base) _set_base(((char *)&(ldt)), base)
#define set_limit(ldt, limit) _set_limit(((char *)&(ldt)), (limit - 1) >> 12)

static unsigned long _get_base(const char *addr) {
  gdt_entry_t *dst = (void *)addr;
  return ((unsigned long)dst->base_low) |
         (((unsigned long)dst->base_mid) << 16) |
         (((unsigned long)dst->base_high) << 24);
}

#define get_base(ldt) _get_base(((char *)&(ldt)))

#define get_limit(segment)                                                     \
  ({                                                                           \
    unsigned long __limit;                                                     \
    __asm__("lsll %1,%0\n\tincl %0" : "=r"(__limit) : "r"(segment));           \
    __limit;                                                                   \
  })

typedef union {
  struct task_struct task;
  char stack[PAGE_SIZE * INIT_STACK_PAGES];
} task_union_t;

extern task_union_t init_task;

typedef char task_ctl_string_t[32];

typedef struct {
  unsigned long task_nr;
  struct task_struct *task;
  unsigned long bits;
  task_ctl_string_t filename;
  task_ctl_string_t argv[32];
  task_ctl_string_t envp[32];

} task_ctl_t;

extern task_ctl_t *task_ctl_data;
extern task_ctl_t *task_ctl_current;

void task_ctl_current_update(long nr);

// 代码段写保护
#define TASK_CTL_INIT_FORK (1UL)
#define TASK_CTL_SH_BANG (2UL)

extern char init_user_stack[PAGE_SIZE * INIT_STACK_PAGES];

#endif
