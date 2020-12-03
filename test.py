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
    baseline_humidity = bme280.humidity
    temperature = bme280.temperature
    
    # Get users name
    name = input('Please enter the name of the current test subject\n')

    # See if the device has settled down; if measurements are the same over 100 polls
    check_readiness(bme280)

    # Now that we are ready, please begin the first procedure of the device
    print(f'{name}, please proceed to breathe into the device without your mask')
    mask_off = read_humidity(bme280, baseline_humidity)
    print(mask_off)
    
    # Check to calibrate device once more
    check_readiness(bme280)

    # Now prompt the user to breathe with the mask on
    print(f'{name}, now proceed to breathe into the device with your mask on')
    mask_on = read_humidity(bme280, baseline_humidity)
    print(mask_on)


    # Take a direct ratio with mask on vs off
    on_vs_off = mask_on / mask_off

    # Get a scaled ratio between 1 and 0 of mask effectivity
    mask_efficiency = 1 - on_vs_off

    if mask_efficiency < 0:
        print("Error reading subjects mask effectivity, please try again!\n")
        exit()

    # Report user mask effectivity to terminal
    print(f'\n\n{name}, your mask was approximately {mask_efficiency * 100}% effective')
    




    # Write the data from the user to our results file only if device owner deems output is correct
    print("Please select if you would like to save the following results.\nSelect yes(Y) or no(N).")

    # Force linux to be character break, not line break (not require hitting enter for results)
    tty.setcbreak(sys.stdin)

    key = 0

    # Until user gives input
    while True:
        
        # Check the key they pressed
        key = sys.stdin.read(1)[0]
        # If answer is yes, save and leave
        if key == 'y' or key == "Y":
            # Revert the terminal back how i found it (fix that character break mode)
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, orig_settings)
            print(f'File for {name} has been saved.\n')
            # fd.write("Participant, room humidity, mask on measurement, mask off measurement\n")
            fd.write(str(name) + ',' + str(baseline_humidity) + ',' + str(mask_on) + ',' + str(mask_off) + ',' + str(mask_efficiency) + "%\n")
            fd.close()
            exit()

        # If answer is no, close files and leave
        if key == "n" or key == "N":
            # Revert the terminal and leave
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, orig_settings)
            fd.close()
            exit()

    # Close our results file
    fd.close()

    exit()


def check_readiness(bme):
    
    within_tolerance = 1
    
    past_temp = 0
    
    for x in range(calibration_measurements):
        
        current_temp = bme.temperature
        
        if abs(past_temp - current_temp) < 1:
            within_tolerance += 1
        
        past_temp = current_temp
   
    if within_tolerance == calibration_measurements:
        print('Device ready, begin sample')
        return 
    else:
        check_readiness(bme)


    
def read_humidity(bme280, baseline_humidity):
    
    humidity_flag = 0
    
    humidities = [0]

    print('Upon countdown hitting 0, please breathe into device\n')

    for seconds in range(3):
        print(3-seconds)
        sleep(1)
    print('0')
    while len(humidities) < 100:
        humidity = bme280.humidity - baseline_humidity
        temperature = bme280.temperature
        pressure = bme280.pressure
        humidities.append(humidity)
        
    return max(humidities)


if __name__ == '__main__':
    main()
    exit()
