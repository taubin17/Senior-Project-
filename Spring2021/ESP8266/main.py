#!/usr/bin/python

import time
import machine
from machine import Pin, I2C

def main():
    print('Running main script!')

    i2c = I2C(scl=Pin(5), sda=Pin(4), freq=100000)

    create_new_VL6180X(i2c)
    default_settings(i2c)

    while True:
        test_i2c(i2c)
    return


def show_i2c(i2c):

    print(i2c.scan())


def create_new_VL6180X(i2c):

    # This function behaves like a class constructor. Sets registers dictated by datasheet
    i2c.writeto_mem(0x29,0x0207, b'0x01')
    i2c.writeto_mem(0x29,0x0208, b'0x01')
    i2c.writeto_mem(0x29,0x0096, b'0x00')
    i2c.writeto_mem(0x29,0x0097, b'0xfd')
    i2c.writeto_mem(0x29,0x00e3, b'0x00')
    i2c.writeto_mem(0x29,0x00e4, b'0x04')
    i2c.writeto_mem(0x29,0x00e5, b'0x02')
    i2c.writeto_mem(0x29,0x00e6, b'0x01')
    i2c.writeto_mem(0x29,0x00e7, b'0x03')
    i2c.writeto_mem(0x29,0x00f5, b'0x02')
    i2c.writeto_mem(0x29,0x00d9, b'0x05')
    i2c.writeto_mem(0x29,0x00db, b'0xce')
    i2c.writeto_mem(0x29,0x00dc, b'0x03')
    i2c.writeto_mem(0x29,0x00dd, b'0xf8')
    i2c.writeto_mem(0x29,0x009f, b'0x00')
    i2c.writeto_mem(0x29,0x00a3, b'0x3c')
    i2c.writeto_mem(0x29,0x00b7, b'0x00')
    i2c.writeto_mem(0x29,0x00bb, b'0x3c')
    i2c.writeto_mem(0x29,0x00b2, b'0x09')
    i2c.writeto_mem(0x29,0x00ca, b'0x09')  
    i2c.writeto_mem(0x29,0x0198, b'0x01')
    i2c.writeto_mem(0x29,0x01b0, b'0x17')
    i2c.writeto_mem(0x29,0x01ad, b'0x00')
    i2c.writeto_mem(0x29,0x00ff, b'0x05')
    i2c.writeto_mem(0x29,0x0100, b'0x05')
    i2c.writeto_mem(0x29,0x0199, b'0x05')
    i2c.writeto_mem(0x29,0x01a6, b'0x1b')
    i2c.writeto_mem(0x29,0x01ac, b'0x3e')
    i2c.writeto_mem(0x29,0x01a7, b'0x1f')
    i2c.writeto_mem(0x29,0x0030, b'0x00')

    return

def default_settings(i2c):

    i2c.writeto_mem(0x29, 0x0014, b'0x24')
    i2c.writeto_mem(0x29, 0x0011, b'0x10')
    i2c.writeto_mem(0x29, 0x010A, b'0x30')
    i2c.writeto_mem(0x29, 0x003f, b'0x46')
    i2c.writeto_mem(0x29, 0x002e, b'0xff')
    i2c.writeto_mem(0x29, 0x0040, b'0x63')
    i2c.writeto_mem(0x29, 0x002e, b'0x01')

    i2c.writeto_mem(0x29, 0x0014, b'0x24')

def test_i2c(i2c):

    # Tell device it is good to start measuring range
    i2c.writeto_mem(0x29, 0x0018, b'0x01')
    
    confirm = i2c.readfrom_mem(0x29, 0x0018, 0x01)
    print(confirm[0])

    time.sleep(0.1)


    # Wait for device to report back that measurement is ready
    # while ready is not True:
        # range_done = i2c.readfrom_mem(0x29, 0x04f, 0x01)
        # range_done = range_done[0]
        # range_ready = range_done & 0x04

        #print(range_done, range_ready)

        # if range_ready == 0x04:
            # ready = True
        # else:
            # ready = False
        # ready = True

    i2c.writeto_mem(0x29, 0x0015, b'0x07')

    range_in_mm = i2c.readfrom_mem(0x29, 0x0064, 0x01)
    range_in_mm = range_in_mm[0]

    print('Range: ', range_in_mm, ' mm')
    
    # Clears now old measurement
    i2c.writeto_mem(0x29, 0x0015, b'0x01')

    time.sleep(0.5)

if __name__ == '__main__':
    main()

