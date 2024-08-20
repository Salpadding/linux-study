import struct

bin = open('tools/system.bin', 'rb').read()

inf = gdb.inferiors()[0]



def addr_of(x):
    sym = gdb.lookup_global_symbol(x)
    return struct.unpack('<I', sym.value().address.bytes)[0]

text_start = addr_of('_text')
text_end = addr_of('_data')
text_len = text_end - text_start

print(f'text_len = {hex(text_len)}')
mem = inf.read_memory(text_start, text_len).tobytes()

for i in range(0, text_len):
    if bin[i] != mem[i]:
        print(f'memory check failed at {hex(i + text_start)}')
        break


print('check ok')



