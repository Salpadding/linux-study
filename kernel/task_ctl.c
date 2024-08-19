#include <linux/sched.h>
#include <linux/task_ctl.h>

task_ctl_t task_ctl_data[NR_TASKS];
task_ctl_t *task_ctl_current;
