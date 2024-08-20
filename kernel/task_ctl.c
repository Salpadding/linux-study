#include <linux/sched.h>
#include <linux/task_ctl.h>
#include <self_mapping.h>

task_ctl_t *task_ctl_data;
task_ctl_t *task_ctl_current;

void task_ctl_write_start(long cur_nr) {}

void task_ctl_write_end(long cur_nr) {}

void task_ctl_current_update(long cur_nr) {

  if (cur_nr < 0) {
    cur_nr = 0;
    while (cur_nr < NR_TASKS && task[cur_nr] != current)
      cur_nr++;

    if (cur_nr == NR_TASKS)
      panic("current task not found");
  }

  task_ctl_current = &task_ctl_data[cur_nr];
  task_ctl_current->task = task[cur_nr];
}
