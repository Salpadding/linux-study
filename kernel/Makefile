include ../rules.mk

objs := vsprintf.o traps.o  printk.o sched.o sys_call.o fork.o \
		sys.o exit.o signal.o asm.o mktime.o task_ctl.o

kernel.o: $(objs)
	$(LD) -r -o $@ $^


clean:
	rm $(objs) kernel.o
