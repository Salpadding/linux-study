#!python3

# see also
# qemu monitor:
# info registers
# info cpus
# info mtree
# info tlb
# info irq
# info qom-tree
# info lapic
# info pic
# info pci
# info mem
# info usb

# pass registers from qemu monitor
import struct

from debug import common
reg_parser = common.reg_parser

def parse_registers():
    # parse gdt
    p = reg_parser(gdb)
    p.parse_all()


    if p.gdt:
        for i in range(0, len(p.gdt)):
            if p.gdt[i]:
                print(f'gdt[{i}]={p.gdt[i]}')

    if p.ldt:
        for i in range(0, len(p.ldt)):
            if p.ldt[i]:
                print(f'ldt[{i}]={p.ldt[i]}')

    for x in p.sregs: print(x)

    if p.creg:
        for x in p.creg: print(x)




parse_registers()
