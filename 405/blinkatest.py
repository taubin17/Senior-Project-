import board
import digitalio
import busio

print('Hello blinka')

pin = digitalio.DigitalInOut(board.D4)
print('Digital IO ok!')

i2c = busio.I2C(board.SCL, board.SDA)
print('I2C okay!')

print('Done')
