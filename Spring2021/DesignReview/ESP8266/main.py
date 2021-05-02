import uos
from machine import UART
import machine
import ustruct
import time
from machine import Pin

def main():

    # print("Test!")
    # Initialize ESP8266 for UART transmission
    uart = ESP8266_UART_init()

    # Open our file to write current test subject data to. This will be used to send to base station
    data_fd = open('current_test.csv', 'w')
    baseline_data_fd = open('baseline_data.csv', 'w')
    data_fd.write('TEMPERATURE, HUMIDITY\n')
    baseline_data_fd.write('TEMPERATURE, HUMIDITY\n')

    # If the UART buffer is not empty (If a transmission fro NUCLEO came in)

    # Get the baseline data from the NUCLEO, and save it to the baseline_data file
    # Then close the baseline_data_fd file
    baseline_data = get_baseline_data(uart)
    baseline_data_fd.write(str(baseline_data[1]) + ', ' + str(baseline_data[0]) + '\n')
    baseline_data_fd.close()

    # Now get the sample count from the NUCLEO
    samples_taken = get_sample_count(uart)

    # Sets the RX buffer size for the number of samples to receive (sample count * sizeof(float) * two (one for each sensor))
    set_buffer(uart, samples_taken)

    # Then we get our data
    data = get_sample_data(uart, samples_taken)

    for sample in range(len(data[0])):
        write_result(data_fd, data[0][sample], data[1][sample])# Write current temp and humidity with newline
        # print("TEMPERATURE: ", data[0][sample], "-----", "HUMIDITY: ", data[1][sample])

    # Close result file
    data_fd.close()

    # Reboot the device to prevent buffer issues
    # machine.reset()
    return


def write_result(fd, temp, humidity):
    str_temp = str(temp)
    str_humidity = str(humidity)

    fd.write(str_temp)
    fd.write(', ')
    fd.write(str_humidity)
    fd.write('\n')

    return


def get_sample_count(uart):
    while uart.any() == 0:
        pass

    samples_in_bytes = uart.read(4)
    [samples] = ustruct.unpack('<i', samples_in_bytes)

    # print(samples)
    return samples


def get_baseline_data(uart):
    while uart.any() == 0:
        pass

    # Read in temperature data, which is first 4 of 8 bytes sent
    temperature_in_bytes = uart.read(4)

    # Now read in humidity data, which is second 4 of 8 bytes sample_count
    humidity_in_bytes = uart.read(4)

    [baseline_temperature] = ustruct.unpack('<f', temperature_in_bytes)
    [baseline_humidity] = ustruct.unpack('<f', humidity_in_bytes)

    # baseline_humidity = baseline_humidity
    # baseline_temperature = baseline_temperature[-1]

    return [baseline_humidity, baseline_temperature]


def set_buffer(uart, sample_count):
    rx_buffer = sample_count * 4 * 2
    uart.init(rxbuf=rx_buffer)
    return


def get_sample_data(uart, sample_count):

    data = []

    humidity = []
    temperature = []

    while uart.any() == 0:
        pass

    while (len(humidity) < sample_count and len(temperature) < sample_count):
        temperature_in_bytes = uart.read(4)
        humidity_in_bytes = uart.read(4)

        time.sleep(0.01) # Sleep due to UPython read call being nonblock WITH NO FLAG TO CHANGE IT

        [temperature_measurement] = ustruct.unpack('<f', temperature_in_bytes)
        [humidity_measurement] = ustruct.unpack('<f', humidity_in_bytes)

        temperature.append(temperature_measurement)
        humidity.append(humidity_measurement)

    data.append(temperature)
    data.append(humidity)

    return data


def ESP8266_UART_init():

    uos.dupterm(None, 1) # Disconnects REPL from serial lines
    uart = UART(0, 115200) # Enables UART line once more, but does not reenable REPL
    uart.init()
    return uart

def handle_interrupt(pin):
    global test_started
    test_started = True


if __name__ == '__main__':
    main()
