#!/usr/bin/env python3
import struct

files = [
    'flat.frag',
    'flat.vert',
    'player.frag',
    'player.png',
    'player.vert',
    'terrain.frag',
    'terrain.png',
    'terrain.vert',
]

head = bytearray()
body = bytearray()

head += struct.pack('<4sII', b'asse', len(files), 12)
ofs = 12 + 24*len(files)

for file in files:
    assert len(file) <= 16
    with open('../assets/'+file, 'rb') as f:
        contents = f.read()
        head += struct.pack('<II16s', ofs, len(contents), file.encode())
        body += contents
        ofs += len(contents)

with open('assets.bin', 'wb') as f:
    f.write(head)
    f.write(body)
