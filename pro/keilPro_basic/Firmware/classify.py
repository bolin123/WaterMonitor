#!/usr/bin/python
# -*- coding: UTF-8 -*-
import sys
import re

if len(sys.argv) != 3:
	sys.exit("need 2 args: filename + type!");

app=open(sys.argv[1]+'.bin', 'rb');
out=open(sys.argv[1]+'_'+sys.argv[2]+'.bin', 'wb');

target=b'$$$$####';
substitute=bytes(sys.argv[2], encoding="utf8");

content=app.read();
content=content.replace(target, substitute);
out.write(content);

app.close();
out.close();

print("classify done\n");