#!/usr/bin/env python
# uninstall script for gfceux
# if gfceux was installed in an non-default prefix, you can specify it
# as an argument
#
# sudo ./uninstall.py [prefix]
#
# prefix must include slash at end
#TODO: fix that
#TODO: remove python package? (site-package/gfceux/etc)

import os
import dircache
import sys

prefix = "/usr/"
try:
    if sys.argv[1]:
        prefix = sys.argv[1]
except IndexError:
    pass

# first include data places scattered in /usr
files = [
    "share/pixmaps/gfceux.png",
    "share/man/man1/gfceux.1", 
    "share/applications/gfceux.desktop",
    "bin/gfceux"]
# then include the files in our /usr/share/gfceux
for x in dircache.listdir(prefix + "share/gfceux"):
  files.append("share/gfceux/" + x)

for x in files:
  os.remove(prefix+x)

# go ahead and remove our shared folder if its empty 
os.rmdir(prefix+"share/gfceux")
