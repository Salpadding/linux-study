/*
 *  linux/kernel/fork.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  'fork.c' contains the help-routines for the 'fork' system call
 * (see also system_call.s), and some misc functions ('verify_area').
 * Fork is rather simple, once you get the hang of it, but the memory
 * management can be a bitch. See 'mm/mm.c': 'copy_page_tables()'
 */
#include <errno.h>

#include <asm/segment.h>
#include <asm/system.h>
#include <linux/head.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/task_ctl.h>
#include <self_mapping.h>
#include <string.h>

extern void write_verify(unsigned long address);
static int copy_init_task(unsigned long from, unsigned long to);

long last_pid = 0;

void verify_area(void *addr, int size) {
  unsigned long start;

  start = (unsigned long)addr;
  size += start & 0xfff;
  start &= 0xfffff000;
  start += get_base(current->ldt[2]);
  while (size > 0) {
    size -= 4096;
    write_verify(start);
    start += 4096;
  }
}

int copy_mem(int nr, struct task_struct *p) {
  unsigned long old_data_base, new_data_base, data_limit;
  unsigned long old_code_base, new_code_base, code_limit;

  code_limit = get_limit(0x0f);
  data_limit = get_limit(0x17);
  old_code_base = get_base(current->ldt[1]);
  old_data_base = get_base(current->ldt[2]);
  if (old_data_base != old_code_base)
    panic("We don't support separate I&D");
  if (data_limit < code_limit)
    panic("Bad data_limit");
  new_data_base = new_code_base = nr * TASK_SIZE;
  p->start_code = new_code_base;
  set_base(p->ldt[1], new_code_base);
  set_base(p->ldt[2], new_data_base);

  if (task_ctl_current->bits & TASK_CTL_INIT_FORK) {
    if (copy_init_task(old_code_base, new_data_base)) {
      return -ENOMEM;
    }
  } else {
    if (copy_page_tables(old_data_base, new_data_base, data_limit)) {
      free_page_tables(new_data_base, data_limit);
      return -ENOMEM;
    }
  }
  return 0;
}

// copy new task as thread rather than process
static int copy_init_task(unsigned long from, unsigned long to) {
  mmap(to + (unsigned long)&_text, (unsigned long)&_text,
       (((unsigned long)&_data) - ((unsigned long)&_text)) >> 12, 5);
  mmap(to + (unsigned long)&_data, (unsigned long)&_data,
       (((unsigned long)&_end) - ((unsigned long)&_data)) >> 12, 7);

  unsigned long i = 0;
  unsigned long tmp;

  // 防止被 free 掉
  while (i < (unsigned long)&_end) {
    mem_map[i >> 12]++;
    i += PAGE_SIZE;
  }

  i = (unsigned long)init_user_stack;
  // 用户态栈区域不共享
  while (i < ((unsigned long)init_user_stack + sizeof(init_user_stack))) {
    tmp = get_free_page();
    memcpy((void *)tmp, (void *)(i + from), PAGE_SIZE);
    mmap(to + i, tmp, 1, 7);
    mem_map[i >> 12]--;
    i += PAGE_SIZE;
  }

  invalidate();
  return 0;
}

/*
 *  Ok, this is the main fork-routine. It copies the system process
 * information (task[nr]) and sets up the necessary registers. It
 * also copies the data segment in it's entirety.
 */
int copy_process(int nr, long ebp, long edi, long esi, long gs, long none,
                 long ebx, long ecx, long edx, long orig_eax, long fs, long es,
                 long ds, long eip, long cs, long eflags, long esp, long ss) {
  struct task_struct *p;
  int i;
  struct file *f;

  task_ctl_data[nr].bits = task_ctl_current->bits;

  p = (struct task_struct *)get_free_page();
  if (!p)
    return -EAGAIN;
  task[nr] = p;
  memcpy(p, current, sizeof(struct task_struct));
  p->state = TASK_UNINTERRUPTIBLE;
  p->pid = last_pid;
  p->counter = p->priority;
  p->signal = 0;
  p->alarm = 0;
  p->leader = 0; /* process leadership doesn't inherit */
  p->utime = p->stime = 0;
  p->cutime = p->cstime = 0;
  p->start_time = jiffies;
  p->tss.back_link = 0;
  p->tss.esp0 = PAGE_SIZE + (long)p;
  p->tss.ss0 = 0x10;
  p->tss.eip = eip;
  p->tss.eflags = eflags;
  p->tss.eax = 0;
  p->tss.ecx = ecx;
  p->tss.edx = edx;
  p->tss.ebx = ebx;
  p->tss.esp = esp;
  p->tss.ebp = ebp;
  p->tss.esi = esi;
  p->tss.edi = edi;
  p->tss.es = es & 0xffff;
  p->tss.cs = cs & 0xffff;
  p->tss.ss = ss & 0xffff;
  p->tss.ds = ds & 0xffff;
  p->tss.fs = fs & 0xffff;
  p->tss.gs = gs & 0xffff;
  p->tss.ldt = _LDT(nr);
  p->tss.trace_bitmap = 0x80000000;
  if (last_task_used_math == current)
    __asm__("clts ; fnsave %0 ; frstor %0" ::"m"(p->tss.i387));
  if (copy_mem(nr, p)) {
    task[nr] = NULL;
    free_page((long)p);
    return -EAGAIN;
  }
  for (i = 0; i < NR_OPEN; i++)
    if (f = p->filp[i])
      f->f_count++;
  if (current->pwd)
    current->pwd->i_count++;
  if (current->root)
    current->root->i_count++;
  if (current->executable)
    current->executable->i_count++;
  if (current->library)
    current->library->i_count++;
  set_tss_desc(gdt + (nr << 1) + FIRST_TSS_ENTRY, &(p->tss));
  set_ldt_desc(gdt + (nr << 1) + FIRST_LDT_ENTRY, &(p->ldt));
  p->p_pptr = current;
  p->p_cptr = 0;
  p->p_ysptr = 0;
  p->p_osptr = current->p_cptr;
  if (p->p_osptr)
    p->p_osptr->p_ysptr = p;
  current->p_cptr = p;
  p->state = TASK_RUNNING; /* do this last, just in case */
  return last_pid;
}

// 因为最后 4MB 用来作 self mapping 了 所以只能63个
int find_empty_process(void) {
  int i;

repeat:
  if ((++last_pid) < 0)
    last_pid = 1;
  for (i = 0; i < NR_TASKS - 1; i++)
    if (task[i] && ((task[i]->pid == last_pid) || (task[i]->pgrp == last_pid)))
      goto repeat;
  for (i = 1; i < NR_TASKS - 1; i++)
    if (!task[i])
      return i;
  return -EAGAIN;
}
