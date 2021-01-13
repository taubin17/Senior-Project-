#!/usr/bin/python

import board
import busio
import adafruit_vcnl4010
import math

# In the event bus is not established in main application, call to open I2C bus
def I2C_initialize():
    
    i2c = busio.I2C(board.SCL, board.SDA)
    return i2c


def proximity_setup(i2c):
    
    # Setup sensor connection over I2C
    sensor = adafruit_vcnl4010.VCNL4010(i2c)
    return sensor


def print_raw(sensor):
    
    while True:
        print('Proximity from device: {0} mm' .format(sensor.proximity))

def poll_sensor(sensor):
    return sensor.proximity

# Function takes raw sensor data, and converts it to approximate mm distance
def calc_distance(sensor_reading):

        # VCNL4010 reads out values of 0 to 65535, corresponding to 0 to 200 mm
        # 0 corresponds to 200 mm away, whereas 65535 corresponds to 0 mm away

        # Creating a linear scaler, maximum distance over maximum readout gives distance per integer readout
        one_point = 200 / 65535

        # Now inverse the scaler to get mm readout

        distance = 200 - one_point * sensor_reading

        return distance


def convert_mm_to_inches(mm):

    inches = mm / 25.4
    return inches


def check_range((low, high)):
    low_in_mm = low * 25.4
    high_in_mm = high * 25.4




def test_program():
    
    i2c = I2C_initialize()
    prox = proximity_setup(i2c)
    
    print_raw(prox)

    while True:
        distance = poll_sensor(prox)
        to_mm = calc_distance(distance)
        square_root_mm = math.sqrt(to_mm) 
        to_in = convert_mm_to_inches(to_mm)

        print(f'mm: {to_mm}    Inches: {to_in}      SQRT: {square_root_mm / 2}')

if __name__ == '__main__':
    main()
