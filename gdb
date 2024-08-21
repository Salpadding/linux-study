target remote 127.0.0.1:1234

file tools/system.elf

set architecture i386

b sys_uselib
