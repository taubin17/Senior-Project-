#!/usr/bin/python
from time import sleep
import board
import busio
import adafruit_bme280
import sys
import termios
import tty

# A defined value of measurements to take for getting breath data
calibration_measurements = 100


def main():
    
    # Create termios struct for detecting user input
    orig_settings = termios.tcgetattr(sys.stdin)

    # Open File to write results
    try:
        fd = open('results.csv', 'a')
    # IF file doesn't exist, make it
    except:
        fd = open('results.csv', 'w')
        fd.write("Participant, room humidity, mask on measurement, mask off measurement\n")
        fd.flush()
        data = fd.read()
        print(data)

    # Variables designated for creating the BME object, as well as reading in data from BME280
    i2c = busio.I2C(board.SCL, board.SDA)
    bme280 = adafruit_bme280.Adafruit_BME280_I2C(i2c)
    baseline_pressure = bme280.pressure
    while (True):
        # bme280 = adafruit_bme280.Adafruit_BME280_I2C(i2c)
        humidity = bme280.humidity
        temperature = bme280.temperature
        pressure = bme280.pressure
        
        if (pressure - baseline_pressure > 1):
            print('Pressure: ', pressure)
        # print('Pressure: ',humidity,' Temperature: ', temperature, ' Pressure: ', pressure)

    
    
    # Close our results file
    fd.close()

    exit()


if __name__ == '__main__':
    main()
    exit()
