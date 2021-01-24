#!/usr/bin/python
import busio
import board
import adafruit_vl6180x



def tof_initialize(i2c):
    sensor = adafruit_vl6180x.VL6180X(i2c)
    return sensor

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
