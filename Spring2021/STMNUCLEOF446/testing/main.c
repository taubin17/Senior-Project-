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

static int read_reg(int busfd, short int addr, unsigned char buffer, int buffer_size);
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

	// Initialize Sensors!
	
	initialize_vl6180x(fd);
	//test_write(fd, 0x000, 0xB4, 1);	
	id = read_reg(fd, 0x000, buffer, 1);
	printf("ID: %d\n", id);
	VL6180X_Init(fd);
	sleep(3);
	

	for (int i = 0; i < 100; i++) {
	
		range = single_range_measurement(fd);
		printf("Range: %d\n", range);
		sleep(1);
	}	
	close(fd);
	
	return 0;
	// Write private registers given in ST datasheet
	//configure_private_reg(fd)
}

void test_write(int busfd, short int addr, unsigned char buffer, int buffer_size) {

	unsigned char result;

	result = read_reg(busfd, addr, buffer, 1);
	printf("Before: %x\n", result);
	
	sleep(1);

	write_reg(busfd, addr, buffer, 1);
	result = read_reg(busfd, addr, buffer, 1);
	printf("After: %x\n", result);

	return;
}

static int read_reg(int busfd, short int addr, unsigned char buffer, int buffer_size)
{
	unsigned char reg_buf[2];
	int result;
	unsigned char data_read[1];

	reg_buf[0] = (addr >> 0) & 0xFF;
	reg_buf[1] = (addr >> 8) & 0xFF;

	//printf("%02x, %02x\n", reg_buf[0], reg_buf[1]);

	
	result = write(busfd, reg_buf, 2);

	if (result < 0) {
		printf("Failed to write to register!\n");
		return result;
	}

	//printf("Wrote to device at addr 0x%03x \n", addr);
	read(busfd, data_read, buffer_size);
	
	return data_read[0];
}

static int write_reg(int busfd, short int addr, unsigned char buffer, int buffer_size)
{
	unsigned char reg_buf[3];
	int result;

	reg_buf[0] = (addr >> 8) & 0xFF;
	reg_buf[1] = (addr >> 0) & 0xFF;
	reg_buf[2] = buffer;
	
	//printf("%02x, %02x, %02x \n", reg_buf[0], reg_buf[1], reg_buf[2]);

	result = write(busfd, reg_buf, 3);

	if (result < 0) {
		printf("Failed to write to register!\n");
		return result;
	}

	//printf("Wrote to device at addr 0x%03x \n", addr);
	
	return result;
}


uint8_t single_range_measurement(int i2cbus)
{
	unsigned char buffer;
	int result;
	uint8_t range;
	char range_status;
	
	//write_reg(i2cbus, 0x015, 0x07, 1);
		
	write_reg(i2cbus, 0x018, 0x01, 1);
	
	range_status = read_reg(i2cbus, 0x04f, buffer, 1);
	range_status = range_status & 0x07;

	while (range_status != 0x04) {
		range_status = read_reg(i2cbus, 0x04f, buffer, 1);
		range_status = range_status & 0x07;
		sleep(1);
	}
	range = read_reg(i2cbus, 0x062, buffer, 1);

	write_reg(i2cbus, 0x015, 0x07, 1);
	return range;


}

void VL6180X_Init(int i2cbus) {

	char reset;

	reset = read_reg(i2cbus, 0x016, 0x00, 1);
	if (reset == 1) {
		write_reg(i2cbus, 0x016, 0x00, 1);
	}

	return;


}
void initialize_vl6180x(int i2cbus) 
{

	//Initialize private registers as instructed by ST datasheet
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

	
	write_reg(i2cbus, 0x0011, 0x10, 1);
	write_reg(i2cbus, 0x010a, 0x30, 1);
	write_reg(i2cbus, 0x003f, 0x46, 1);
	write_reg(i2cbus, 0x0031, 0xff, 1);
	write_reg(i2cbus, 0x0040, 0x63, 1);
	write_reg(i2cbus, 0x002e, 0x01, 1);	
	write_reg(i2cbus, 0x001b, 0x09, 1);
	write_reg(i2cbus, 0x003e, 0x31, 1);
	write_reg(i2cbus, 0x0014, 0x24, 1);

	return;

}

/*
void configure_private_reg(int i2c_bus) {
	
	int result;

	result = vl6180x_write_reg(, data)

	return

	}
	
void vl6180x_write_reg(uint16_t addr, uint8_t data) 
{
	uint8_t buffer[] = {(addr >> 8), addr & 0xff, 
	}
*/

