#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define BME_280_ADDRESS 0x77
#define BME_280_WRITE 0xEE
#define BME_280_READ 0xEF

int read_reg(int busfd, uint8_t addr);
int write_reg(int busfd, uint8_t addr, uint8_t data);

int main(int argc, char *argv[])
{

	int result;
	int fd;
	uint8_t ID;

	char i2c_bus[] = "/dev/i2c-1";

	fd = open(i2c_bus, O_RDWR);

	if (fd < 0) {
		printf("Error opening I2C bus!\n");
		return -1;
	}

	// Set BME slave address (Default 0x77)
	result = ioctl(fd, I2C_SLAVE, 0x77);

	if (result < 0) {
		printf("Error opening BME280 Sensor");
		return -1;
	}

	// Initialize device registers
	ID = read_reg(fd, 0xD0); 
		
	printf("Device ID: %x\n", ID);

	return 0;

}

int write_reg(int busfd, uint8_t addr, uint8_t data)
{

	// BME280 Datasheet states to write to device over i2c, first send device address over line, then register address, then register data
	unsigned char reg_buf[2];
	int bytes_written;

	reg_buf[0] = addr;
	reg_buf[1] = data;

	bytes_written = write(busfd, reg_buf, 2);

	if (bytes_written < 0) {
		printf("Error writing to device!\n");
		return bytes_written;
	}

	return bytes_written;

}

int read_reg(int busfd, uint8_t addr)
{
	// BME280 Datasheet states to write device address followed by 0 bit high (0xEE), followed by register address with 0 bit 1 (0xEF). Device then returns 
	unsigned char reg_buf[1];
	int bytes_written;
	uint8_t data_read;

	// reg_buf[0] = BME_280_ADDRESS;
	reg_buf[0] = addr;
	
	// Write to the device what register we want read. By sending stop bit, device will go into read mode
	bytes_written = write(busfd, reg_buf, 1);
	
	if (bytes_written < 0) {
		printf("Error writting to device registers\n");
		return -1;
	}

	// Read back register dictated by the address we passed in
	read(busfd, &data_read, 1);

	// Return the data we read
	return data_read;	

}
