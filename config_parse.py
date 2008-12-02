#!/usr/bin/env python

#TODO: fix writing
#TODO

import re
import os

class FceuxConfigParser:
    def __init__(self, filename):
        self.fn = filename
    
    def _open(self):
        try:
            self.f = open(self.fn, "r")
            return 1
        except:
            print "Can't open config."
            return 0
		
        
	
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
        self.f.close()
        return 0
    
    def writeKey(self, keyname, value):
        self._open()
        
        buf = ""

        cursor = 0
        # find the key
        while 1:
            data = self.f.read(keyname.__len__())
            buf += data
            if data == "":
                return 0
            if data == keyname:
                break
            else:
                 buf += self.f.readline()
        
        print value		

        buf += self.f.read(3)
		
        buf += str(value)
        buf += '\n'
        
        # ignore the rest of the old line
        self.f.readline()

        # read the rest of the file
        while 1:
            data = self.f.readline()
            if data == "":
                break
            buf += data
		
        self.f.close()

        # write the buffer to the config file
        self.f = open(self.fn, 'w')
        self.f.write(buf)
        self.f.close()
		




            
        
        
        
    
