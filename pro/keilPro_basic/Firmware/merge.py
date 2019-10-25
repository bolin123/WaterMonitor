#!/usr/bin/python
# -*- coding: UTF-8 -*-
import sys
import re

fname='YumairFP01';
if len(sys.argv)>1:
	fname=sys.argv[1]
boot=open('bootloader.bin', 'rb');
app=open(fname+'.bin', 'rb');
out=open(fname+'(boot).bin', 'wb');

content=boot.read(); 
out.write(content);

target=b'$$$$####';
subtitute=b'YT-006XX';
out.seek(0x1800);
content=app.read();
content=content.replace(target, subtitute);
out.write(content);

boot.close();
app.close();
out.close();

print("merge done\n");