#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

//#include "bme280.h"

#define BME_280_ADDRESS 0x77
#define BME_280_WRITE 0xEE
#define BME_280_READ 0xEF

int read_reg(int busfd, uint8_t addr);
int write_reg(int busfd, uint8_t addr, uint8_t data);
void bme280_init(int busfd);
int8_t * burst_read(int busfd, uint8_t addr, uint8_t number_of_bytes);
float read_humidity(int busfd);
int read_data(int busfd);


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

	bme280_init(fd);	
		
	printf("Device ID: %x\n", ID);

	for (int i = 0; i < 10; i++) {

		//printf("Status: %x\n", read_reg(fd, 0xF3));
		read_data(fd);
		printf("\n\n\n");
		sleep(2);
	}

	read_data(fd);

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

int8_t * burst_read(int busfd, uint8_t addr, uint8_t bytes_to_read) 
{
	unsigned char reg_buf[1];
	uint8_t bytes_written;
	int8_t *data_read = malloc(sizeof (int8_t) * bytes_to_read);

	//Pressure Variables. Registers 0, 1, and 2 hold pressure MSB, LSB, and XLSB. Will be used in BOSCH compensation API to readout data
	uint32_t p_data_xlsb;
	uint32_t p_data_lsb;
	uint32_t p_data_msb;

	//Temperature Variables.
	uint32_t t_data_xlsb;
	uint32_t t_data_lsb;
	uint32_t t_data_msb;

	//Humidity Variables
	uint32_t h_data_lsb;
	uint32_t h_data_msb;


	reg_buf[0] = addr;

	bytes_written = write(busfd, reg_buf, 1);

	if (bytes_written < 0) {
		printf("Error writting register address!\n");
		exit(1);
	}

	read(busfd, data_read, bytes_to_read);

	p_data_msb = (uint32_t)data_read[0] << 12;
	p_data_lsb = (uint32_t)data_read[1] << 4;
	p_data_xlsb = (uint32_t)data_read[2] >> 4;

	t_data_msb = (uint32_t)data_read[3] << 12;
	t_data_lsb = (uint32_t)data_read[4] << 4;
	t_data_xlsb = (uint32_t)data_read[5] >> 4;

	h_data_msb = (uint32_t)data_read[6] << 8;
	h_data_lsb = (uint32_t)data_read[7];

	// Now use BOSCH compensation api to return as 32 bit floats
	comp_pressure(p_data_msb, p_data_lsb, p_data_xlsb);
	comp_temp(t_data_msb, t_data_lsb, t_data_xlsb);
	comp_humidity(h_data_msb, h_data_lsb);

	return data_read;
}

uint32_t comp_pressure(uint32_t p_msb, uint32_t p_lsb, uint32_t p_xlsb) 
{
	double var1;
	double var2;
	double var3;
	double pressure;
	double pressure_min = 30000.0;
	double pressure_max = 110000.0;

	var1 = 





}
uint32_t comp_temp(uint32_t t_msb, uint32_t t_lsb, uint32_t t_xlsb)
{




}
uint32_t comp_humidity(uint32_t h_msb, uint32_t h_lsb)
{




}


void bme280_init(int busfd)
{
	write_reg(busfd, 0xF2, 0x7); // Writes to humidity oversampling register to oversample 16 times. This means to meet contract specs, simply average 2 measurements
	write_reg(busfd, 0xF4, 0xE3); // Writes to Temperature and Pressure controller to oversample 16 times.
	write_reg(busfd, 0xF5, 0x00); // Sets period of measurements to 0.5ms (f = 2kHz). Also turns off IIR for T/P sensor
	
	printf("Setup complete!");
	return;
}
/*
float read_humidity(int busfd) 
{
	uint8_t raw_humidity[2];
	uint16_t int_humidity;

	raw_humidity = burst_read(busfd, 0xFD, 2);

	int_humidity = (raw_humidity[0] << 8) + raw_humidity[0];



}
*/

int read_data(int busfd)
{
	//int8_t all_sensor_data[8];
	int8_t *sensor_data;
	
	uint8_t humidity_MSB;
	uint8_t humidity_LSB;

	sensor_data = burst_read(busfd, 0xF7, 8);

	for (int i = 0; i < 8; i++) {
		printf("%d, %x \n", i, *(sensor_data+i));
	}

	return 0;
	//humidity_MSB = all_sensor_data[6];
	//humidity_LSB = all_sensor_data[7];


}

