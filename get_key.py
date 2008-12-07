#!/usr/bin/env python   
import sys
try:
    import pygame
    from pygame.locals import *
    has_pygame = True
except ImportError:
    self.has_pygame = False

class KeyGrabber:
    """ KeyGrabber is a wrapper that gets an SDL key from the user using pygame """
    def __init__(self, width=300, height=100):
        pygame.init()
        pygame.joystick.init()
        if pygame.joystick.get_count() > 0:
            print "Joystick found!"
            for x in range(0, pygame.joystick.get_count()):
                joy = pygame.joystick.Joystick(x)
                joy.init()
                print "joy " + str(x) + " initialized."
        screen = pygame.display.set_mode((width, height))
        pygame.display.set_caption("Press any key. . .")
        

    def get_key(self):
        while 1:
            for event in pygame.event.get():
                if event.type == KEYDOWN:
                    pygame.display.quit()
                    return ("Keyboard", event.key)
                # TODO: Make work with joystick.   Do buttons first.    
                if event.type == JOYBUTTONDOWN:
                    pygame.display.quit()
                    # TODO: Make sure we're returning the data we need
                    #   for config file here.  I'm not sure if this is 
                    #   the correct data.
                    return ("Joystick",  event.button, event.joy)
                #if event.type == JOYAXISMOTION:
                #    pygame.display.quit()
                #    return event.joy, event.axis, event.value
          
if __name__ == "__main__":
    kg = KeyGrabber()
    print kg.get_key()
      
