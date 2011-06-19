#!/usr/bin/env python

import serial, sys, time, struct

s = serial.Serial('/dev/ttyUSB0', 9600)
count = 1


def recv(loop = True):
  global count
  while True: 
    c = s.read()
    l = ord(s.read())
    arg = s.read(l)
    print "<%d> [0x%2.2x] > %s" % (count, ord(c), arg)  
    count += 1
    if not loop: break

def blaat ():
  while True:
    d = s.read()
    sys.stdout.write(d)

blaat()

s.write(struct.pack('BBBBBB', 0x11, 4, 0, 10, 255, 128))
recv(False)

time.sleep(1)

s.write(struct.pack('BBBBBB', 0x10, 4, 0, 255, 0, 0))
recv()







