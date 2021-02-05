#!/usr/bin/python

import tof
import board
import busio
import adafruit_vl6180x
from time import sleep

def main():
    i2c = busio.I2C(board.SCL, board.SDA)

    range_sensor = adafruit_vl6180x.VL6180X(i2c)

    while True:
        print(range_sensor.range)
        #sleep(1)


if __name__ == '__main__':
    main()
