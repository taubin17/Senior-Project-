#!/usr/bin/python
import busio
import board
import adafruit_vl6180x

from time import sleep


def tof_initialize(i2c):
    sensor = adafruit_vl6180x.VL6180X(i2c)
    return sensor


def main():

    # Setup I2C connection
    i2c = busio.I2C(board.SCL, board.SDA)

    # Establish connection with VL6180X 
    sensor = adafruit_vl6180x.VL6180X(i2c)
    
    # Establish connection with display
    display = display_initialize(i2c)

    # Begin recording proximity to device. Average samples to increase accuracy in proximity reading

    while True:
        sleep(0.3)
        range_mm = check_range(sensor, 10)
        range_in = convert_mm_to_in(range_mm)
        print(f'Range: {range_in:.2f}')

def read_tof_mm(sensor):
    return sensor.range



def convert_mm_to_in(mm):

    return (mm / 25.4)


def check_range(VL6180X, sample_count):

    average = 0
    
    for measurement in range((sample_count - 1)):
        average += VL6180X.range

    average = average / sample_count

    return int(average)



if __name__ == '__main__':
    main()
