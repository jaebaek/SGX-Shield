#!/usr/bin/python

import sys

if len(sys.argv) == 1:
    print 'Usage:', sys.argv[0], ' <ptrace long>'
    exit(1)

tmp = sys.argv[1] # 'tmp'

# objdump result
f = open(tmp + '/dump')
p = False
pos = 0
tk = ''
dump = dict()
for l in f:
    v = l.split(':')
    if (len(v) == 2) and ('<' in v[0]):
        if len(tk) > 4:
            dump[pos] = tk
            tk = ''
        pos = int(v[0].split()[0], 16)
        p = True
    elif p:
        tk = tk + l

# readelf result
f = open(tmp + '/symtab')
symtab = dict()
for l in f:
    v = l.split()
    symtab[int(v[0])] = int(v[1], 16)

def analyze(log):
    # performance eval result
    ptrace = open(tmp + '/' + log + '.ptrace')
    out = open(tmp + '/' + log + '.analyze', 'w')
    total = 0
    freq = []
    for l in ptrace:
        v = l.split()
        if len(v) == 2:
            total = int(v[1], 16)
        elif len(v) == 3:
            freq.append((int(v[0]), v[1], int(v[2])))

    # sort by frequency
    freq = sorted(freq, key=lambda sf: sf[2], reverse=True)

    out.write('total: %d\n\n' % total)
    for sf in freq:
        if sf[0] == 0:
            continue
        out.write(str(sf[0]) + ' ' + sf[1] + ' --> ' + str(sf[2]) + '\n')
        out.write(str(dump[symtab[sf[0]]]))

ptrace_logs = [
    "DoEmFloatIteration",
    "DoNumSortIteration",
    "DoStringSortIteration",
    "DoBitfieldIteration",
    "DoFPUTransIteration",
    "DoAssignIteration",
    "DoIDEAIteration",
    "DoHuffIteration",
    "DoNNetIteration",
    "DoLUIteration",
]

for p in ptrace_logs:
    analyze(p)
