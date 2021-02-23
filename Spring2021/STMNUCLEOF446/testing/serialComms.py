#!/usr/bin/python

import serial

def main():
    trial = 0
    with serial.Serial('/dev/ttyUSB0', 115200,bytesize=8, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=None, xonxoff=False, rtscts=False, write_timeout=None, dsrdtr=False, inter_byte_timeout=None, exclusive=None) as ser:
        while True:
            line = ser.readline()
            trial += 1
            print("Trial: ", trial, " --- ", str(line, 'utf-8'))


if __name__ == '__main__':
    main()
