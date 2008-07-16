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
            
            self.f.seek(cursor)
            while 1:
                c = self.f.read(1)
#                self.f.seek(5, 1)
                print c,

                if c == '=':
                    self.f.seek(1, os.SEEK_CUR)
                    #self.f.write(value)
                    self.f.flush()
                
        
        
cp = ConfigParser("/home/lukas/.fceultra/fceu.cfg")
cp.writeKey("Fullscreen", 1)
#print cp.readKey("Fullscreen")
            
        
        
        
    
