#!/usr/bin/env python

import random
import struct
import math
import os

fin = open('samples.bin', 'rb')
fout = open('samples_mul.bin', 'wb')
s=fin.read(4)
while True:
	if not s: break
	data = struct.unpack('<f', s )[0]
	#print(data)
	fout.write(struct.pack('<f',data*100000))			
	s=fin.read(4)
print("multiplied")
fin.close()
fout.close()
f = open('samples_mul.bin', 'r+b')
s=f.read(1)
i=0
while True:
	if not s: break
	if i<2:
		f.seek(-1, os.SEEK_CUR)
		f.write("\x00")
	s=f.read(1)
	i+=1
	i%=4
f.close()

