#!/usr/bin/env python
# uninstall script for gfceux
# if gfceux was installed in an non-default prefix, you can specify it
# as an argument
#
# sudo ./uninstall.py [prefix]
#

import os
import dircache
prefix = "/usr/"
if sys.argv[1]:
  prefix == sys.argv[1]

files = ['share/pixmaps/gfceux.png', 'share/man/man1/gfceux.1', 
  'share/applications/gfceux.desktop', 'bin/gfceux']
for x in dircache.listdir(prefix+"share/gfceux"):
  files.append("share/gfceux/"+x)

for x in files:
  os.remove(prefix+x)
  
os.rmdir(prefix+"share/gfceux")
