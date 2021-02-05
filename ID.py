#!/usr/bin/python
from time import sleep
import board
import busio
import adafruit_bme280
import sys
import termios
import tty
import os
from time import sleep


# Import all sensor/display packages
from Spring2021.HT16K33.display import *
from Spring2021.VL6180.tof import *

# A defined value of measurements to take for getting breath data
calibration_measurements = 100

# Defined measurement range tuple, format shown here -> (low_end (inches), high_end (inches)). 
prox_range = (0.5, 4)

# Adding proximity detection, import motion script
sys.path.append('/home/pi/SeniorProject/WinterBreak2020/Motion/')


def main():
    
    # Create termios struct for detecting user input
    orig_settings = termios.tcgetattr(sys.stdin)

    result_file_exists = os.path.isfile('results.csv')

    # Open File to write results
    if result_file_exists:
        fd = open('results.csv', 'a')
    # IF file doesn't exist, make it
    else:
        print("File not found, creating one now!")
        fd = open('results.csv', 'w')
        fd.write("Participant Name, Room Humidity (%RH), Mask-On Humidity (%RH), Mask-Off Humidity, Mask-On VS Mask-Off Efficiency (%)\n")
        fd.flush()
        
    # Variables designated for creating the BME object, as well as reading in data from BME280
    i2c = busio.I2C(board.SCL, board.SDA)



    # Setup TOF sensor
    range_sensor = tof_initialize(i2c)

    read_id(range_sensor)

    # See if the device has settled down; if measurements are the same over 100 polls
    check_readiness(bme280)

    # Now that we are ready, please begin the first procedure of the device
    print(f'{name}, please proceed to place your mouth between {prox_range[0]} to {prox_range[1]} inches away!')

    # Check the range sensor, and see if testees mask is within range
    within_distance = False
    
    while within_distance is not True:
        distance_in_mm = check_range(range_sensor, 10)
        distance_in_inches = convert_mm_to_in(distance_in_mm)
        write_to_display(display, round(distance_in_inches, 3))
        within_distance = compare_range(prox_range, distance_in_inches)
        sleep(0.05)
    
    # Clear display of range data
    display_clear(display)
    
    # With testee in range, begin testing
    print(f'{name}, please proceed to breathe into the device without your mask')
    mask_off = read_humidity(bme280, baseline_humidity)
    print(mask_off)
    
    # Write testee mask off report to display
    write_to_display(display, round(mask_off, 2))

    # Check to calibrate device once more
    check_readiness(bme280)

    # Now prompt the user to breathe with the mask on
    print(f'{name}, now proceed to breathe into the device with your mask on')
    mask_on = read_humidity(bme280, baseline_humidity)
    print(mask_on)

    # Write testee mask on report to display
    write_to_display(display, round(mask_on, 2))

    # Take a direct ratio with mask on vs off
    on_vs_off = mask_on / mask_off

    # Get a scaled ratio between 1 and 0 of mask effectivity
    mask_efficiency = 1 - on_vs_off
    
    # If device reports mask has negative effect, report bad measurement and try again
    if mask_efficiency < 0:
        print("Error reading subjects mask effectivity, please try again!\n")
        exit()

    # Report user mask effectivity to terminal
    print(f'\n\n{name}, your mask was approximately {mask_efficiency * 100}% effective')
    
    # Report mask effectivity to display
    write_to_display(display, round(mask_efficiency * 100, 2))
    prompt_writeup(baseline_humidity, mask_on, mask_off, mask_efficiency, name, fd, orig_settings)
    
    display_clear(display)
    fd.close()
    
    return

# Function takes in specified range, and checks if device is acceptable distance away
def compare_range(specified_range, current_distance):

    print(f'Range: {current_distance}, ')

    # If the current distance is between low and high end of specified range, return good. Otherwise, return False
    if current_distance > specified_range[0] and current_distance < specified_range[1]:
        print("Within acceptable range, please stay still!")
        return True
    else:
        if current_distance < specified_range[0]:
            print('Too close, move back!')
            return False
        
        else:
            print('Too far, move closer!')
            return False





def prompt_writeup(baseline_humidity, mask_on, mask_off, mask_efficiency, name, fd, orig_settings):
   
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
            fd.write(str(name) + ', ' + str(baseline_humidity) + '%, ' + str(mask_on) + '%, ' + str(mask_off) + '%, ' + str(mask_efficiency * 100) + "%\n")
            return

        # If answer is no, close files and leave
        if key == "n" or key == "N":
            # Revert the terminal and leave
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, orig_settings)
            return

def check_readiness(bme):
    
    # Variable to define maximum variance between two measurements
    within_tolerance = 1
    
    # Empty variable to hold previous temperature
    past_temp = 0
    

    # For each calibration measurement
    for x in range(calibration_measurements):
        
        current_temp = bme.temperature
        
        # Check if the current and previous measurement are within tolerance. If so, increment by 1
        if abs(past_temp - current_temp) < 1:
            within_tolerance += 1
        
        past_temp = current_temp
   
    # If all measurements for one cycle were within calibration, report device is calibrated and ready to measure. Otherwise, try to calibrate again
    if within_tolerance == calibration_measurements:
        print('Device ready, begin sample')
        return 
    else:
        check_readiness(bme)


    
def read_humidity(bme280, baseline_humidity):
    
    # Create empty list to hold measured humidity readings. This is useful for we can report the maximum reading as worst case for mask on and mask off 
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
