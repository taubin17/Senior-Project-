#!/usr/bin/python

import serial

ser = serial.Serial(
        port='/dev/serial0',
        baudrate = 9600,
        parity = serial.PARITY_NONE,
        stopbits = serial.STOPBITS_ONE,
        bytesize = serial.EIGHTBITS,
        timeout = 15
        )
readout = ser.readline()

print(str(readout))
readout.decode('utf-8')
# ser.write(b'gpio.mode(3, gpio.OUTPUT)')
# ser.write(b'while 1 do')
#ser.write(b'gpio.write(3, gpio.HIGH)')
# ser.write(b'tmr.delay(1000000)')
# ser.write(b'gpio.write(3, gpio.LOW)')
# ser.write(b'tmr.delay(1000000)')

#exit()
