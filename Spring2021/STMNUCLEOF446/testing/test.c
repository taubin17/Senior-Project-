#include <stdio.h>
#include <unistd.h>


struct identifier
{

	uint8_t dev_addr;
	int8_t fd;

}
int main(int argc, char *argv[])
{
	struct bme280_dev dev;
	
	struct identifier id;
	int result;

	char i2c_bud[] = "/dev/i2c-1";

	id.fd = open(i2c_bus, O_RDWR);
	id.dev_addr = 0x77;

	if (id.fd < 0) {
		printf("Error opening i2c bus!\n");
		exit(1)
	}
	
	result = ioctl(id.fd, I2C_SLAVE, id.dev_addr);

	if (result < 0) {
		printf("Error calling IOCTL!\n");
		exit(1);
	}
	
	dev.intf = BME280_I2C_INTF;
	dev.read = user_i2c_read;
	dev.write = user_i2c_write;
	dev.delay_us = user_delay_us;
	
