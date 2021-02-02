#ifndef bme_h
#define bme_h

#define BME280_CONCAT_BYTES(msb, lsb) (((uint16_t)msb << 8) | (uint16_t)lsb)

struct bme280_calib_data
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

#endif
