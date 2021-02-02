#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "bme.h"

#define BME_280_ADDRESS 0x77
#define BME_280_WRITE 0xEE
#define BME_280_READ 0xEF

static void parse_calib_data(int busfd, struct bme280_calib_data *calib);
uint8_t read_reg(int busfd, uint8_t addr);
int write_reg(int busfd, uint8_t addr, uint8_t data);
void bme280_init(int busfd);
uint8_t * burst_read(int busfd, uint8_t addr, uint8_t number_of_bytes);
float read_humidity(int busfd);
int read_data(int busfd, struct bme280_calib_data *calib);
int32_t comp_temp(uint32_t temp_msb, uint32_t temp_lsb, uint32_t temp_xlsb, struct bme280_calib_data *calib);
uint32_t comp_humidity(uint32_t humidity_msb, uint32_t humidity_lsb, struct bme280_calib_data *calib);

int main(int argc, char *argv[])
{

	int result;
	int fd;
	uint8_t ID;
	struct bme280_calib_data calib;

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

	parse_calib_data(fd, &calib);
	
	//sleep(5);

	bme280_init(fd);	
		
	printf("Device ID: %x\n", ID);

	for (int i = 0; i < 1000; i++) {

		//printf("Status: %x\n", read_reg(fd, 0xF3));
		read_data(fd, &calib);
		//printf("\n\n\n");
		usleep(100000);
	}


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

uint8_t read_reg(int busfd, uint8_t addr)
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

uint8_t * burst_read(int busfd, uint8_t addr, uint8_t bytes_to_read) 
{
	unsigned char reg_buf[1];
	uint8_t bytes_written;
	uint8_t *data_read = malloc(sizeof (uint8_t) * bytes_to_read);

	reg_buf[0] = addr;

	bytes_written = write(busfd, reg_buf, 1);

	if (bytes_written < 0) {
		printf("Error writting register address!\n");
		exit(1);
	}

	read(busfd, data_read, bytes_to_read);

	return data_read;
}

static void parse_calib_data(int busfd, struct bme280_calib_data *calib)
{
	// Get temp and pressure calibration data
	uint8_t * readout = burst_read(busfd, 0x88, 26);
	
	int16_t dig_h4_msb;
	int16_t dig_h4_lsb;
	int16_t dig_h5_msb;
	int16_t dig_h5_lsb;
	
	calib->dig_t1 = BME280_CONCAT_BYTES(readout[1], readout[0]);
	calib->dig_t2 = (int16_t)BME280_CONCAT_BYTES(readout[3], readout[2]);
	calib->dig_t3 = (int16_t)BME280_CONCAT_BYTES(readout[5], readout[4]);
	calib->dig_p1 = BME280_CONCAT_BYTES(readout[7], readout[6]);
	calib->dig_p2 = (int16_t)BME280_CONCAT_BYTES(readout[9], readout[8]);
	calib->dig_p3 = (int16_t)BME280_CONCAT_BYTES(readout[11], readout[10]);
	calib->dig_p4 = (int16_t)BME280_CONCAT_BYTES(readout[13], readout[12]);
	calib->dig_p5 = (int16_t)BME280_CONCAT_BYTES(readout[15], readout[14]);
	calib->dig_p6 = (int16_t)BME280_CONCAT_BYTES(readout[17], readout[16]);
	calib->dig_p7 = (int16_t)BME280_CONCAT_BYTES(readout[19], readout[18]);
	calib->dig_p8 = (int16_t)BME280_CONCAT_BYTES(readout[21], readout[20]);
	calib->dig_p9 = (int16_t)BME280_CONCAT_BYTES(readout[23], readout[22]);
	calib->dig_h1 = readout[25];

	// Temperature and Pressure calibrations nearly complete (need t_fine, which is on board temp calibration register)
	free(readout);

	readout = burst_read(busfd, 0xE1, 7);
	
	calib->dig_h2 = (int16_t)BME280_CONCAT_BYTES(readout[1], readout[0]);
	calib->dig_h3 = readout[2];
	dig_h4_msb = (int16_t)(int8_t)readout[3] * 16;
	dig_h4_lsb = (int16_t)(readout[4] & 0x0F);
	calib->dig_h4 = dig_h4_msb | dig_h4_lsb;
	dig_h5_msb = (int16_t)(int8_t)readout[5] * 16;
	dig_h5_lsb = (int16_t)(readout[4] >> 4);
	calib->dig_h5 = dig_h5_msb | dig_h5_lsb;
	calib->dig_h6 = (int8_t)readout[6];


	return;	
		




}
/*
uint32_t comp_pressure(uint32_t p_msb, uint32_t p_lsb, uint32_t p_xlsb, struct bme280_calib_data *calib) 
{
	double var1;
	double var2;
	double var3;
	double pressure;
	double pressure_min = 30000.0;
	double pressure_max = 110000.0;

	//var1 = 





}

*/
int32_t comp_temp(uint32_t t_msb, uint32_t t_lsb, uint32_t t_xlsb, struct bme280_calib_data *calib)
{
	int32_t var1;
	int32_t var2;
	
	uint32_t uncomp_temp = t_msb | t_lsb | t_xlsb;

	int32_t temperature;
	int32_t temperature_min = -4000;
	int32_t temperature_max = 8500;

	var1 = ((int32_t)uncomp_temp / 8) - ((int32_t)calib->dig_t1 * 2);
	var1 = (var1 * ((int32_t)calib->dig_t2)) / 2048;
	var2 = (int32_t)((uncomp_temp / 16) - ((int32_t)calib->dig_t1));
	var2 = (var2 * var2) / 4096 * ((int32_t)calib->dig_t3) / 16384;
	calib->t_fine = (var1 + var2);
	temperature = (calib->t_fine * 5 + 128) / 256;

	//printf("Non corrected temp: %d ----- ", temperature);

	if (temperature > temperature_max) {
		
		temperature = temperature_max;

	}

	if (temperature < temperature_min) {

		temperature = temperature_min;

	}

	return temperature;

}

uint32_t comp_humidity(uint32_t h_msb, uint32_t h_lsb, struct bme280_calib_data *calib)
{

	int32_t var1;
	int32_t var2;
	int32_t var3;
	int32_t var4;
	int32_t var5;

	uint32_t uncomp_humidity = h_msb | h_lsb;

	uint32_t humidity;
	uint32_t humidity_max = 102400;

	var1 = calib->t_fine - ((int32_t)76800);
	var2 = (int32_t)(uncomp_humidity * 16384);
	var3 = (int32_t)(((int32_t)calib->dig_h4) * 1048576);
	var4 = ((int32_t)calib->dig_h5) * var1;
	var5 = (((var2 - var3) - var4) + (int32_t)16384) / 32768;
	var2 = (var1 * ((int32_t)calib->dig_h6)) / 1024;
	var3 = (var1 * ((int32_t)calib->dig_h3)) / 2048;
	var4 = ((var2 * (var3 + (int32_t)32768)) / 1024) + (int32_t)2097152;
	var2 = ((var4 * ((int32_t)calib->dig_h2)) + 8192) / 16384;
	var3 = var5 * var2;

	var4 = ((var3 / 32768) * (var3 / 32768)) / 128;
	var5 = var3 - ((var4 * ((int32_t)calib->dig_h1)) / 16);
	var5 = (var5 < 0 ? 0 : var5);
	var5 = (var5 > 419430400 ? 419430400 : var5);
	
	humidity = (uint32_t) (var5 / 4096);

	//printf("Humidity: %d -----", humidity);

	if (humidity > humidity_max) {

		humidity = humidity_max;
	
	}

	return humidity;

}

void bme280_init(int busfd)
{
	write_reg(busfd, 0xF2, 0x7); // Writes to humidity oversampling register to oversample 16 times. This means to meet contract specs, simply average 2 measurements
	write_reg(busfd, 0xF4, 0xE3); // Writes to Temperature and Pressure controller to oversample 16 times.
	write_reg(busfd, 0xF5, 0x00); // Sets period of measurements to 0.5ms (f = 2kHz). Also turns off IIR for T/P sensor
	
	printf("Setup complete!");
	return;
}

int read_data(int busfd, struct bme280_calib_data *calib)
{
	uint8_t *sensor_data;
	
	uint32_t humidity_msb;
	uint32_t humidity_lsb;
	uint32_t temp_msb;
	uint32_t temp_lsb;
	uint32_t temp_xlsb;
	uint32_t press_msb;
	uint32_t press_lsb;
	uint32_t press_xlsb;

	int32_t temperature;
	uint32_t pressure;
	uint32_t humidity;

	float new_temp;
	float new_humidity;


	sensor_data = burst_read(busfd, 0xF7, 8);

	temp_msb = sensor_data[3] << 12;
	temp_lsb = sensor_data[4] << 4;
	temp_xlsb = sensor_data[5] >> 4;

	/*
	press_msb = sensor_data[0];
	press_lsb = sensor_data[1];
	press_xlsb = sensor_data[2];
	*/

	humidity_msb = sensor_data[6] << 8;
	humidity_lsb = sensor_data[7];

	temperature = comp_temp(temp_msb, temp_lsb, temp_xlsb, calib);
	humidity = comp_humidity(humidity_msb, humidity_lsb, calib);

	new_temp = (float)temperature / 100;

	new_humidity = (float)humidity / 1000;

	printf("Temperature: %3.2f ----- Humidity: %3.2f \n", new_temp, new_humidity);

	return 0;

}

