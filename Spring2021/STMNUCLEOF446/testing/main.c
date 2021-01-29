#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <linux/i2c.h>

static int read_reg(int busfd, short int addr, int buffer_size);
static int write_reg(int busfd, short int addr, unsigned char buffer, int buffer_size);
void test_write(int busfd, short int addr, unsigned char buffer, int buffer_size);
uint8_t single_range_measurement(int i2cbus);
void initialize_vl6180x(int i2cbus);
void VL6180X_Init(int i2cbus);

int main(int argc, char *argv[]) 
{
	int fd;
	int result;
	int id;
	// Open I2C bus
	char i2c_bus[] = "/dev/i2c-1";

	uint8_t range = 0;
	//unsigned char *buffer;

	unsigned char buffer;

	fd = open(i2c_bus, O_RDWR);
	
	if (fd < 0) {
		printf("Error opening I2C bus!\n"); 
		return -1;
	}

	// Set TOF slave address (0x29)
	result = ioctl(fd, I2C_SLAVE, 0x29);

	if (result < 0) {
		printf("Error opening TOF sensor!\n"); 
		return -1;
	}

	// Initialize Sensors
	initialize_vl6180x(fd);
	VL6180X_Init(fd);

	// Poll sensor until range sensor detects between range_low and range_high (0.5 to 4 inches)	
	for (int i = 0; i < 100; i++) {
		range = single_range_measurement(fd);
		printf("Range: %d\n", range);
		usleep(50000);
	}
		
	close(fd);
	
	return 0;
}

void test_write(int busfd, short int addr, unsigned char buffer, int buffer_size) {

	unsigned char result;

	// Read original value of register
	result = read_reg(busfd, addr, 1);
	printf("Addr: 0x%04x, Before: 0x%x\n", addr, result);
	
	sleep(1);

	// Write new value to that register
	write_reg(busfd, addr, buffer, 1);

	// Check if that register has changed
	result = read_reg(busfd, addr, 1);
	printf("Addr: 0x%04x, After: 0x%x\n", addr, result);

	return;
}

static int read_reg(int busfd, short int addr, int buffer_size)
{
	unsigned char reg_buf[2];
	int bytes_written;
	unsigned char data_read[1];
	
	// Reg_buf[0] represents upper 8 bit register, reg_buf[1] represents lower 8 bit address.
	reg_buf[0] = (addr >> 8) & 0xFF;
	reg_buf[1] = (addr >> 0) & 0xFF;
	
	// Tell VL6180X we want address held in reg_buf[0] and [1]
	bytes_written = write(busfd, reg_buf, 2);

	// If address cannot reach device, return failure
	if (bytes_written < 0) {
		printf("Failed to write to register!\n");
		return bytes_written;
	}

	// Read back register data
	read(busfd, data_read, buffer_size);
	
	return data_read[0];
}

static int write_reg(int busfd, short int addr, unsigned char data, int buffer_size)
{
	unsigned char reg_buf[3];
	int bytes_written;
	
	// Reg_buf[0] represents upper 8 bit address, reg_buf[1] represents low 8 bits. Reg_buf[2] represents data to write
	reg_buf[0] = (addr >> 8) & 0xFF;
	reg_buf[1] = (addr >> 0) & 0xFF;
	reg_buf[2] = data;


	bytes_written = write(busfd, reg_buf, 3);

	if (bytes_written < 0) {
		printf("Failed to write to register!\n");
		return bytes_written;
	}

	return bytes_written;
}

uint8_t single_range_measurement(int i2cbus)
{
	unsigned char buffer;
	int result;
	uint8_t range;
	char range_status;
	
	// Start single range measurement	
	write_reg(i2cbus, 0x018, 0x01, 1);
	
	// Read back range ready register. See if measurement is done
	range_status = read_reg(i2cbus, 0x04f, 1);
	range_status = range_status & 0x07;

	// While range ready register is not ready, keep checking
	while (range_status != 0x04) {
		range_status = read_reg(i2cbus, 0x04f, 1);
		range_status = range_status & 0x07;
		sleep(1);
	}

	// Range register can now be read. Get range reading!
	range = read_reg(i2cbus, 0x062, 1);

	// Write to range reset register to get ready for new measurement
	write_reg(i2cbus, 0x015, 0x07, 1);
	return range;


}

void VL6180X_Init(int i2cbus) 
{

	char reset;
	
	// Checks if reset bit is high. Necessary for initialize_vl6180x, cannot write private registers when reset flag is high
	reset = read_reg(i2cbus, 0x016, 1);
	
	// If reset flag is high on startup, turn it off
	if (reset == 1) {
		write_reg(i2cbus, 0x016, 0x00, 1);
	}

	return;


}

void initialize_vl6180x(int i2cbus) 
{

	//Initialize private registers as instructed by ST datasheet. Cannot find any information on why these are set, but device does not work properly without setting them
	write_reg(i2cbus, 0x0207, 0x01, 1);
	write_reg(i2cbus, 0x0208, 0x01, 1);
	write_reg(i2cbus, 0x0096, 0x00, 1);
	write_reg(i2cbus, 0x0097, 0xfd, 1);
	write_reg(i2cbus, 0x00e3, 0x00, 1);
	write_reg(i2cbus, 0x00e4, 0x04, 1);
	write_reg(i2cbus, 0x00e5, 0x02, 1);
	write_reg(i2cbus, 0x00e6, 0x01, 1);
	write_reg(i2cbus, 0x00e7, 0x03, 1);
	write_reg(i2cbus, 0x00f5, 0x02, 1);
	write_reg(i2cbus, 0x00d9, 0x05, 1);
	write_reg(i2cbus, 0x00db, 0xce, 1);
	write_reg(i2cbus, 0x00dc, 0x03, 1);
	write_reg(i2cbus, 0x00dd, 0xf8, 1);
	write_reg(i2cbus, 0x009f, 0x00, 1);
	write_reg(i2cbus, 0x00a3, 0x3c, 1);
	write_reg(i2cbus, 0x00b7, 0x00, 1);
	write_reg(i2cbus, 0x00bb, 0x3c, 1);
	write_reg(i2cbus, 0x00b2, 0x09, 1);
	write_reg(i2cbus, 0x00ca, 0x09, 1);
	write_reg(i2cbus, 0x0198, 0x01, 1);
	write_reg(i2cbus, 0x01b0, 0x17, 1);
	write_reg(i2cbus, 0x01ad, 0x00, 1);
	write_reg(i2cbus, 0x00ff, 0x05, 1);
	write_reg(i2cbus, 0x0100, 0x05, 1);
	write_reg(i2cbus, 0x0199, 0x05, 1);
	write_reg(i2cbus, 0x01a6, 0x1b, 1);
	write_reg(i2cbus, 0x01ac, 0x3e, 1);
	write_reg(i2cbus, 0x01a7, 0x1f, 1);
	write_reg(i2cbus, 0x0030, 0x00, 1);

	
	write_reg(i2cbus, 0x0011, 0x10, 1); // Enables polling for "New Sample Ready" condition. Useful for interrupts later
	write_reg(i2cbus, 0x010a, 0x30, 1); // Set averaging period to ST community recommendation. Best combination of sample time/noise reduction
	write_reg(i2cbus, 0x003f, 0x46, 1); // Sets light and dark gain. Useful if light sensor becomes necessary
	write_reg(i2cbus, 0x0031, 0xff, 1); // Sets number of measurements before auto calibration to half max. Best combination of accuracy and up time
	write_reg(i2cbus, 0x0040, 0x63, 1); // ALS integration time to 100ms
	write_reg(i2cbus, 0x002e, 0x01, 1); // Perform temperature calibration. Very useful for temperate climates	
	write_reg(i2cbus, 0x001b, 0x09, 1); // Set default ranging inter-measurement period to 100 ms. Helps reduce noise.
	write_reg(i2cbus, 0x003e, 0x31, 1); // Set default ALS inter measurement period to 500 ms. ALS noise potentially reduced
	write_reg(i2cbus, 0x0014, 0x24, 1); // Configured interrupt on "New Sample Ready" condition. Useful for interrupts later

	return;

}
