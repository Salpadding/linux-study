include ../../rules.mk

objs := tty_io.o console.o serial.o keyboard.o rs_io.o tty_ioctl.o pty.o

chr_drv.o: $(objs)
	$(LD) -r -o $@ $^

keyboard.o: keyboard.S
	$(CPP) -o keyboard.s $^
	$(AS) -o $@ --32 keyboard.s


clean:
	rm $(objs) chr_drv.o

