#!/usr/bin/env python

# config_parse.py 
#   This module handles the reading and writing of keys to the fceux
#   config file.
#
# Lukas Sabota
# Licensed under the GPL

import re
import os

class FceuxConfigParser:
    def __init__(self, filename):
        self.fn = filename
    
    def _open(self, mode):
        try:
            self.f = open(self.fn, mode)
            return 1
        except:
            if mode == "r":
                print "Can't open config for reading."
            else:
                print "Couldn't write to the config."
            return 0
		
        
	
    def readKey(self, keyname):
        """ readKey()
                reads a key from the configfile and returns the value
        """
        self._open("r")
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
        return None
        
    def addKey(self, keyname, value=""):
        """ addKey()
                adds a key with a (optional) value to the config file.
        """
        
        self._open("a")
        self.f.write(keyname + " = " + str(value))
        self.f.close()
        
    def writeKey(self, keyname, value):
        """ writeKey()
                modifies an existing key in the config file.
        """
        self._open("r")
        
        buf = ""

        cursor = 0
        # find the key
        while 1:
            data = self.f.read(keyname.__len__())
            buf += data
            if data == "":
                print "key " + keyname + " not found"
                return None
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
        self._open('w')
        self.f.write(buf)
        self.f.close()
		
