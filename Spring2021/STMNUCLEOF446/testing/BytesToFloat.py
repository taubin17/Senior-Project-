#!/usr/bin/python

import serial
import struct
from sys import getsizeof

def main():
    trial = 0

    bytes_received = b'\xb6\xf3\x9d\x3f'

    samples_taken = get_sample_count()

    data = get_measurement(samples_taken)

def get_sample_count():

    with serial.Serial('/dev/ttyUSB0', 115200,bytesize=8, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=None, xonxoff=False, rtscts=False, write_timeout=None, dsrdtr=False, inter_byte_timeout=None, exclusive=None) as ser:
        # Get the amount of samples that will be transmitted
        samples_in_bytes = ser.read(4)

        [samples] = struct.unpack('<i', samples_in_bytes)
        print(samples)
        
        return samples


def get_measurement(sample_count):

    data = []
    humidity = []
    temperature = []

    with serial.Serial('/dev/ttyUSB0', 115200,bytesize=8, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=None, xonxoff=False, rtscts=False, write_timeout=None, dsrdtr=False, inter_byte_timeout=None, exclusive=None) as ser:
        while (len(humidity) < sample_count and len(temperature) < sample_count):

            temperature_in_bytes = ser.read(4)
            humidity_in_bytes = ser.read(4)

            [temperature_measurement] = struct.unpack('<f', temperature_in_bytes)
            [humidity_measurement] = struct.unpack('<f', humidity_in_bytes)

            temperature.append(temperature_measurement)
            humidity.append(humidity_measurement)
        
        
        data.append(temperature)
        data.append(humidity)

    return data

if __name__ == '__main__':
    main()
