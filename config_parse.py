#!/usr/bin/env python

#TODO: fix writing

import re
import os

class ConfigParser:
    def __init__(self, filename):
        try:
            f = open(filename, "rw");
        except:
            print "Can't open config."
        
        self.f = f
    
    def readKey(self, keyname):
        self.f.seek(0)
        # do some lines
        while 1:
            line = self.f.readline()
            if line == 0:
                break
            if re.search(keyname, line):
                return line

        # key not found
        return 0
    
    def writeKey(self, keyname, value):
        self.f.seek(0)
        
        cursor = 0
        # find the key
        while 1:
            line = self.f.readline()
            if line == 0:
                return 0
            if re.search(keyname, line):
                cursor = self.f.tell()
                break
				
        newline =  line.split('=')[0] + '=' + str(value)
        self.f.seek(os.SEEK_CUR, -line.split('=')[0].__len__)		
        
		
                
        
        
cp = ConfigParser("/home/lukas/.fceultra/fceu.cfg")
cp.writeKey("Fullscreen", 1)
#print cp.readKey("Fullscreen")
            
        
        
        
    
