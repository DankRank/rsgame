#!/usr/bin/env python3
import struct

files = [
    'assets/flat.frag',
    'assets/flat.vert',
    'assets/terrain.frag',
    'assets/terrain.png',
    'assets/terrain.vert',
]

head = bytearray()
body = bytearray()

for file in files:
    with open('../'+file, 'rb') as f:
        contents = f.read()
        head += struct.pack('<256sII', file.encode(), len(body), len(contents))
        body += contents

tail = struct.pack('<8sII', 'assets00'.encode(), len(head), len(body))

with open('assets.bin', 'wb') as f:
    f.write(head)
    f.write(body)
    f.write(tail)
