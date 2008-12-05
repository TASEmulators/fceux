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
        screen = pygame.display.set_mode((width, height))
        pygame.display.set_caption("Press any key. . .")

    def get_key(self):
        while 1:
            for event in pygame.event.get():
                if event.type == KEYDOWN:
                    pygame.display.quit()
                    return event.key
          
if __name__ == "__main__":
    kg = KeyGrabber()
    print kg.get_key()
      
