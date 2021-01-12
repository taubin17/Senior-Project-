#!/usr/bin/python

import board
import busio
import adafruit_vcnl4010

def main():
    
    # Begin our I2C connection to the sensor
    i2c = busio.I2C(board.SCL, board.SDA)
    sensor = adafruit_vcnl4010.VCNL4010(i2c)

    while True:
        distance = calc_distance(sensor.proximity)
        print('Proximity from device: {0} mm' .format(distance))

# Function takes raw sensor data, and converts it to approximate mm distance
def calc_distance(sensor_reading):

        # VCNL4010 reads out values of 0 to 65535, corresponding to 0 to 200 mm
        # 0 corresponds to 200 mm away, whereas 65535 corresponds to 0 mm away

        # Creating a linear scaler, maximum distance over maximum readout gives distance per integer readout
        one_point = 200 / 65535

        # Now inverse the scaler to get mm readout

        distance = 200 - one_point * sensor_reading

        return distance

if __name__ == '__main__':
    main()
