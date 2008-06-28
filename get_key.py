#!/usr/bin/env python
import pygame
import sys
from pygame.locals import *

pygame.init()
width = 300
height = 100
screen = pygame.display.set_mode((width, height))
pygame.display.set_caption("Press any key. . .")

while 1:
  for event in pygame.event.get():
    if event.type == KEYDOWN:
      print event.key
      sys.exit(event.key)
      
