#!/usr/bin/python

import board
import busio
import adafruit_bme280
from adafruit_ht16k33 import segments
from string import ascii_lowercase
import time

        
def display_initialize(i2c):
    display = segments.Seg7x4(i2c)
    display.fill(0)
    return display

def write_to_display(display, data):

    display.fill(0)
    display.print(data)
    
    return 

if __name__ == '__main__':
    main()
