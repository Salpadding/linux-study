tlb = gdb.execute('monitor info tlb',to_string=True)

lines = []
for line in tlb.split('\n'):
    if not ':' in line: continue

    splited = line.split(':')
    vaddr = int(splited[0], 16)
    paddr_attr = splited[1].split()
    paddr = int(paddr_attr[0], 16)
    attr = paddr_attr[1][-4:len(paddr_attr[1])]

    lines.append([vaddr, paddr, attr])


if not len(lines):
    exit(0)


to_merge = []
maps = []

def merge(to_merge, maps):
    if len(to_merge) == 0: return
    maps.append({
        'vstart': hex(to_merge[0][0]),
        'vend': hex(to_merge[-1][0] + 4096),
        'pstart': hex(to_merge[0][1]),
        'pend': hex(to_merge[-1][1] + 4096),
        'size': hex(to_merge[-1][1] - to_merge[0][1] + 4096),
        'attr': to_merge[0][2],
    })

for i in range(0, len(lines)):
    if len(to_merge) == 0:
        to_merge.append(lines[i])
        continue

    cur = lines[i]
    if cur[2] == to_merge[-1][2] and \
        cur[1] == to_merge[-1][1] + 4096 and \
        cur[0] == to_merge[-1][0] + 4096:
        to_merge.append(cur)
        continue
    
    merge(to_merge, maps)
    to_merge = [cur]

merge(to_merge, maps)


print(len(maps))
for m in maps: print(m)
