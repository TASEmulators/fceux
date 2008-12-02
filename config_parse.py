#!/usr/bin/env python

#TODO: fix writing

import re
import os

class FceuxConfigParser:
    def __init__(self, filename):
        try:
            f = open(filename, "r+")
        except:
            print "Can't open config."
        
        self.fn = filename
    
    def _open(self):
        try:
            self.f = open(self.fn, "r+")
            return 1
        except:
            print "Can't open config."
            return 0
			
    def _close(self):
        self.f.close()
		
        
	
    def readKey(self, keyname):
        self._open()
		# do some lines
        while 1:
            line = self.f.readline()
            if line == 0:
                break
            if re.search(keyname, line):
                self._close()
                return line

        # key not found
        self._close()
        return 0
    
    def writeKey(self, keyname, value):
        self._open()
        
        cursor = 0
        # find the key
        while 1:
            key = self.f.read(keyname.__len__())
            if key == "":
                return 0
            if key == keyname:
                print key
                break
            else:
                 self.f.readline()				
        # move back until we find a =
		#while self.f.read(1) != '=':
		#	self.f.seek(1, -2)		
        
		
        print self.f.read(3)
		
        self.f.write(value)
		
        self._close()
		

    def close(self):
        self.f.close()
		print "hey"
        
cp = FceuxConfigParser("/home/lukas/.fceux/fceux.cfg")
cp.writeKey("SDL.Fullscreen", "1")



            
        
        
        
    
