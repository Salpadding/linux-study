#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <errno.h>

void init_swapping() {}


int send_sig(long sig,struct task_struct * p,int priv) {
    return 0;
}


int sys_ptrace()
{
	return -ENOSYS;
}


int sys_rename()
{
	return -ENOSYS;
}

void hard_reset_now() {

}


unsigned long timer_active = 0;
