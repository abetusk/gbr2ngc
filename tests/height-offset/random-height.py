#!/usr/bin/python

import sys
import math
import random

# min_x: 1.600000 mm, 0.062992125984252 inch
# max_x: 5.450000 mm, 0.214566929133858 inch
# min_y: -4.150000 mm, -0.163385826771654 inch
# max_y: -1.600000 mm, -0.062992125984252 inch
# min_z: -0.050000 mm, -0.00196850393700787 inch
# max_z: 0.100000 mm, 0.00393700787401575 inch

X = [1.0, 6.0]
Y = [-5.0, -1.0]

dX = X[1] - X[0]
dY = Y[1] - Y[0]

dx = 0.5
dy = 0.45

randmin = -0.15
randmax = 0.15


x=X[0]
while x < X[1]:
  y = Y[0];
  while y < Y[1]:

    u = randmin + (randmax-randmin)*random.random()
    print x, y, u

    y+=dy

  print ""
  x += dx

