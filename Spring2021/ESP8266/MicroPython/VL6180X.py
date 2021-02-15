#!/usr/bin/python

from machine import Pin, I2C
import ubinascii as ubin
from time import sleep

def main():
    i2c = I2C(scl=Pin(5), sda=Pin(4), freq=100000)

    #for each in i2c.scan():
      #  print(hex(each))

    ID = read_reg(i2c, 0x29, 0x000, 1)
    bytesID = ubin.hexlify(ID)

    test_write(i2c, 0x29, 0x018, 1)
    

def read_reg(i2c, device_addr, reg_addr, bytes_to_read):
    upper = (reg_addr >> 8) & 0xFF
    lower = (reg_addr >> 8) & 0xFF

    reg_addr = [(upper), (lower)]

    i2c.writeto(device_addr, bytearray(reg_addr))

    data = i2c.readfrom(device_addr, bytes_to_read)

    return data

def write_reg(i2c, device_addr, reg_addr, data):
    upper = (reg_addr >> 8) & 0xFF
    lower = (reg_addr >> 0) & 0xFF
    
    to_write = [upper, lower, data]

    for each in range(len(to_write)):
        print(each, to_write[each])

    bytes_written = i2c.writeto(device_addr, bytearray(to_write))

    if (bytes_written < len(to_write)):
        print("Error, write unsuccessful!")
        exit(-1)

    return

def test_write(i2c, device_addr, reg_addr, bytes_to_read):
    pre_test = read_reg(i2c, device_addr, reg_addr, bytes_to_read)
    pre_test_to_string = ubin.hexlify(pre_test)
    print("Pre test result: ", pre_test_to_string, " of writing to register: ", hex(reg_addr))

    sleep(1)
    write_reg(i2c, device_addr, reg_addr, 0x01)
    sleep(1)
    test = read_reg(i2c, device_addr, reg_addr, bytes_to_read)
    test_to_string = ubin.hexlify(test)
    print("Test result: ", test_to_string)

    return


