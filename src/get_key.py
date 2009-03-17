#!/usr/bin/env python   
# get_key.py - an sdl keygrabber for gfceux
# Lukas Sabota
# December 2008
# 
# Licensed under the GPL
#

import sys
try:
    import pygame
    from pygame.locals import *
    has_pygame = True
except ImportError:
    self.has_pygame = False

class KeyGrabber:
    """ KeyGrabber is a wrapper that gets an SDL key from the user using pygame """
    def __init__(self, width=800, height=100):
        pygame.init()
        pygame.joystick.init()
        if pygame.joystick.get_count() > 0:
            for x in range(0, pygame.joystick.get_count()):
                joy = pygame.joystick.Joystick(x)
                joy.init()
                print "joystick " + str(x) + " initialized."
        screen = pygame.display.set_mode((width, height))
        pygame.display.set_caption("Press any key. . .")
        

    def get_key(self):
    	""" returns a tuple with information about the key pressed
		 (device_string, key_number, joy_number) """
        while 1:
            for event in pygame.event.get():
                if event.type == KEYDOWN:
                    pygame.display.quit()
                    return ("Keyboard", event.key)   
                if event.type == JOYBUTTONDOWN:
                    pygame.display.quit()
                    return ("Joystick",  event.button, event.joy)
                # TODO: figure out how fceux saves axes and hat movements and implement
                #       this might even be easier if we just make the C++ fceux code more modular
                #       and use a C++ snippet based off code in drivers/sdl/input.cpp and drivers/sdl/sdl-joystick.cpp
                #if event.type == JOYAXISMOTION:
                #    pygame.display.quit()
                #    return event.joy, event.axis, event.value
          
if __name__ == "__main__":
    kg = KeyGrabber()
    print kg.get_key()
      
