#ifndef SRC_BME280_H_
#define SRC_BME280_H_

#include "main.h"

#define BME280_CONCAT_BYTES(msb, lsb) (((uint16_t)msb << 8) | (uint16_t)lsb)
#define TEMPERATURE	0
#define HUMIDITY	1
#define SAMPLE_READY	0x08

// By defualt, BME280 I2C address is 0x77, but because it is right aligned, shift one place to make left aligned
#define BME280	(0x77 << 1)

struct BME280_calib_data
{
	uint16_t dig_t1;

	int16_t dig_t2;

	int16_t dig_t3;

	uint16_t dig_p1;

	int16_t dig_p2;

	int16_t dig_p3;

	int16_t dig_p4;

	int16_t dig_p5;

	int16_t dig_p6;

	int16_t dig_p7;

	int16_t dig_p8;

	int16_t dig_p9;

	uint8_t dig_h1;

	int16_t dig_h2;

	uint8_t dig_h3;

	int16_t dig_h4;

	int16_t dig_h5;

	int8_t dig_h6;

	int32_t t_fine;

};

void BME280_get_calib_data(struct BME280_calib_data *calib);

uint8_t BME280_read_reg(uint8_t addr);
int8_t BME280_write_reg(uint8_t addr, uint8_t data);
uint8_t * BME280_burst_read(uint8_t addr, uint8_t number_of_bytes);

void BME280_init();

int32_t BME280_comp_temp(uint32_t temp_msb, uint32_t temp_lsb, uint32_t temp_xlsb, struct BME280_calib_data *calib);
uint32_t BME280_comp_humidity(uint32_t humidity_msb, uint32_t humidity_lsb, struct BME280_calib_data *calib);

float * BME280_read_data(struct BME280_calib_data *calib);

#endif /* SRC_BME280_H_ */
