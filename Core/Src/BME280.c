#include <stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "main.h"

#include "bme280.h"
#include "SerialDebug.h"

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

int8_t BME280_write_reg(uint8_t reg_addr, uint8_t data)
{

	// BME280 Datasheet states to write to device over i2c, first send device address over line, then register address, then register data
	unsigned char reg_buf[2];

	HAL_StatusTypeDef bytes;

	reg_buf[0] = reg_addr;
	reg_buf[1] = data;

	bytes = HAL_I2C_Master_Transmit(&hi2c1, BME280, reg_buf, 2, HAL_MAX_DELAY);

	if (bytes != HAL_OK) {
		DebugLog("Error in BME280_write_reg\r\n");
		return -1;
	}

	return 0;

}

// Function reads one singular 8 bit register. Useful for getting ID of device
uint8_t BME280_read_reg(uint8_t reg_addr)
{
	// BME280 Datasheet states to write device address followed by 0 bit high (0xEE), followed by register address with 0 bit 1 (0xEF). Device then returns
	unsigned char reg_buf[1];

	uint8_t data_read[1];

	HAL_StatusTypeDef bytes;

	// reg_buf[0] = BME_280_ADDRESS;
	reg_buf[0] = reg_addr;

	// Write to the device what register we want read. By sending stop bit, device will go into read mode
	bytes = HAL_I2C_Master_Transmit(&hi2c1, BME280, reg_buf, 1, HAL_MAX_DELAY);

	if (bytes != HAL_OK) {
		DebugLog("Error writing to device registers!\r\n");
		return 0;
	}

	// Read back register dictated by the address we passed in
	bytes = HAL_I2C_Master_Receive(&hi2c1, BME280, data_read, 1, HAL_MAX_DELAY);

	// Return the data we read
	return data_read[0];

}

// Function reads bytes from addr to addr + bytes_to_read and returns data from registers
uint8_t * BME280_burst_read(uint8_t reg_addr, uint8_t bytes_to_read)
{
	// Reg buf holds address of where to begin reading
	unsigned char reg_buf[1];

	HAL_StatusTypeDef bytes;

	// Buffer to read in byte data
	uint8_t * data_read = malloc(bytes_to_read * sizeof(uint8_t));

	reg_buf[0] = reg_addr;

	bytes = HAL_I2C_Master_Transmit(&hi2c1, BME280, reg_buf, 1, HAL_MAX_DELAY);

	// If write failed
	if (bytes != HAL_OK) {
		DebugLog("Error writing register address!\r\n");
		return NULL;
	}

	bytes = HAL_I2C_Master_Receive(&hi2c1, BME280, data_read, bytes_to_read, HAL_MAX_DELAY);

	return data_read;
}

// Function taken from BOSCH VL6180X Driver.
void BME280_get_calib_data(struct BME280_calib_data *calib)
{
	// Get temp and pressure calibration data
	uint8_t * readout = BME280_burst_read(0x88, 26);

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

	readout = BME280_burst_read(0xE1, 7);

	calib->dig_h2 = (int16_t)BME280_CONCAT_BYTES(readout[1], readout[0]);
	calib->dig_h3 = readout[2];
	dig_h4_msb = (int16_t)(int8_t)readout[3] * 16;
	dig_h4_lsb = (int16_t)(readout[4] & 0x0F);
	calib->dig_h4 = dig_h4_msb | dig_h4_lsb;
	dig_h5_msb = (int16_t)(int8_t)readout[5] * 16;
	dig_h5_lsb = (int16_t)(readout[4] >> 4);
	calib->dig_h5 = dig_h5_msb | dig_h5_lsb;
	calib->dig_h6 = (int8_t)readout[6];

	// Clear heap of readout, we have data we need
	free(readout);

	return;


}


// Function taken from BOSCH VL6180X Driver.
int32_t BME280_comp_temp(uint32_t t_msb, uint32_t t_lsb, uint32_t t_xlsb, struct BME280_calib_data *calib)
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

uint32_t BME280_comp_humidity(uint32_t h_msb, uint32_t h_lsb, struct BME280_calib_data *calib)
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

void BME280_init()
{
	BME280_write_reg(0xF2, 0x7); // Writes to humidity oversampling register to oversample 16 times. This means to meet contract specs, simply average 2 measurements
	BME280_write_reg(0xF4, 0xE3); // Writes to Temperature and Pressure controller to oversample 16 times.
	BME280_write_reg(0xF5, 0x00); // Sets period of measurements to 0.5ms (f = 2kHz). Also turns off IIR for T/P sensor

	DebugLog("BME280 setup complete!\r\n");
	return;
}

float * BME280_read_data(struct BME280_calib_data *calib)
{
	uint8_t * sensor_data;

	char debug[100];

	uint32_t humidity_msb;
	uint32_t humidity_lsb;
	uint32_t temp_msb;
	uint32_t temp_lsb;
	uint32_t temp_xlsb;

	int32_t temperature;
	uint32_t humidity;



	float new_temp;
	float new_humidity;

	float * result = malloc(sizeof(float) * 2);

	// Wait until Sample Available flag has been set
	while((BME280_read_reg(0xF3) & SAMPLE_READY) != 0);


	sensor_data = BME280_burst_read(0xF7, 8);

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

	// Done with sensor data, free it from heap
	free(sensor_data);

	temperature = BME280_comp_temp(temp_msb, temp_lsb, temp_xlsb, calib);
	humidity = BME280_comp_humidity(humidity_msb, humidity_lsb, calib);

	new_temp = (float)temperature / 100;

	new_humidity = (float)humidity / 1000;

	//sprintf(debug, "HUMIDITY: %f --- TEMPERATURE: %f\r\n", new_humidity, new_temp);
	//DebugLog(debug);

	result[TEMPERATURE] = new_temp;
	result[HUMIDITY] = new_humidity;

	// printf("Temperature: %3.2f ----- Humidity: %3.2f --- TEMPERATURE: %f \n", new_temp, new_humidity);

	return result;

}
