import sys
import struct
from debug import common

# 打印 argv 和 envp

# ldt base 指针
ldt_base = 0x4000000
# 栈顶指针
sp = 0x3bfffc4


inf = common.inf_clazz(gdb)
max_envp = 64
argc = inf.u32(sp + ldt_base) 

print(f'argc = {argc}')

argv = []

argv_ptr = inf.u32(sp + ldt_base + 4) + ldt_base

print(f'argv at {hex(argv_ptr)}')

envp_ptr =  inf.u32(sp + ldt_base + 8) + ldt_base

print(f'envp at {hex(envp_ptr)}')

envp = []

for i in range(0, argc+1):
    arg_ptr = inf.u32(argv_ptr) 
    if arg_ptr == 0:
        argv.append('NULL')
        break

    argv.append(inf.c_str(arg_ptr + ldt_base, 256))
    argv_ptr += 4


for i in range(0, max_envp):
    env_ptr = inf.u32(envp_ptr) 
    if env_ptr == 0:
        envp.append('NULL')
        break

    envp.append(inf.c_str(env_ptr + ldt_base, 256))
    envp_ptr += 4

print('argv = ')
print(argv)
print('envp=')
print(envp)


