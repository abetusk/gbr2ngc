#!/usr/bin/env python

import argparse as ap
import sys
import os
import re

def scmp( l, st ):
  n = len(l)
  m = len(st)
  if n<m: return False
  return l[:m] == st

parser = ap.ArgumentParser(description='Convert Excellon drill file to GCode.')
parser.add_argument("--input", "-i", required=True, type=str, help="input file")
parser.add_argument("--output", "-o", type=str, help="output file")
parser.add_argument("--height", "-Z", type=float, default=1.0/8.0, help="output file")
parser.add_argument("--depth", "-z", type=float, default=-1.0/8.0, help="output file")

args = parser.parse_args()

inpfn = ""
outfn = "-"

ifp = sys.stdin
ofp = sys.stdout

if args.input:
  inpfn = args.input
  if inpfn != "-":
    ifp = open( inpfn, "r" )

if args.output:
  outfn = args.output
  if outfn != "-":
    ofp = open( outfn, "w" )

height = args.height
depth = args.depth

drill_points = []

digre = "-?\d+|-?\d+\.\d+|-?\.\d+"

curx = "0.0"
cury = "0.0"


for l in ifp:
  if l[0] == ';': continue
  if l[0] == '%': continue
  if scmp( l, "M48" ): continue

  m = re.match( r'^\s*X(' + digre + r')Y(' + digre + r')$', l )
  if m:
    drill_points.append( [ m.group(1), m.group(2) ] )
    curx = m.group(1)
    cury = m.group(2)
  else:
    m = re.match( r'^\s*X(' + digre + r')$', l )
    if m:
      drill_points.append( [ m.group(1), cury ] )
      curx = m.group(1)

    m = re.match( r'^\s*Y(' + digre + r')$', l )
    if m:
      drill_points.append( [ curx, m.group(1) ] )
      cury = m.group(1)


ofp.write("g90\n")
ofp.write("g0 f100\n")
ofp.write("g1 f10\n")
ofp.write("g0 z " + str(height) + "\n")


for d in drill_points:
  ofp.write("g0 x " + d[0] + " y " + d[1] + "\n" )
  ofp.write("g1 z " + str(depth) + "\n")
  ofp.write("g1 z " + str(height) + "\n")



