#!/usr/bin/python

import board
import busio
from display import *
from adafruit_ht16k33 import segments

def main():
    i2c = busio.I2C(board.SCL, board.SDA)
    display = display_initialize(i2c)
    
    display.fill(0)

    write_to_display(display, "ABCD")
    

if __name__ == '__main__':
    main()
