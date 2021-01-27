#!/usr/bin/python

import time

def main():
    print('Running main script!')

    pin = machine.Pin(0, machine.Pin.OUT)

    while True:
        pin.value(1)
        time.sleep(0.5)
        pin.value(0)
        time.sleep(0.5)

    return

if __name__ == '__main__':
    main()

