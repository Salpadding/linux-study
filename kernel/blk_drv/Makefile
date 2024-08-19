include ../../rules.mk

objs := floppy.o hd.o ll_rw_blk.o ramdisk.o

blk_drv.o: $(objs)
	$(LD) -r -o $@ $^

clean:
	rm $(objs) blk_drv.o


