#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "VL6180X.h"
#include "SerialDebug.h"
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

void VL6180X_test_write(uint16_t reg_to_test)
{
	unsigned char result;
	char debug[50];

	result = VL6180X_read_reg(reg_to_test);
	sprintf(debug, "Reg value before: %X --- Reg ADDR: %X\r\n", result, reg_to_test);
	DebugLog(debug);

	VL6180X_write_reg(reg_to_test, 0xFF);
	result = VL6180X_read_reg(reg_to_test);
	sprintf(debug, "Reg value now: %X --- Reg ADDR: %X\r\n", result, reg_to_test);
	DebugLog(debug);

	return;

}


uint8_t VL6180X_single_range_measurement()
{
	uint8_t range;
	char range_status;

	VL6180X_write_reg(0x018, 0x01);

	range_status = VL6180X_read_reg(0x04f);
	range_status = range_status & 0x07;

	while (range_status != 0x04)
	{
		range_status = VL6180X_read_reg(0x04f);
		range_status = range_status & 0x07;
	}

	range = VL6180X_read_reg(0x062);


	VL6180X_write_reg(0x015, 0x07);
	return range;
}


uint8_t VL6180X_read_reg(uint16_t reg_addr)
{

	unsigned char reg_buf[2];
	HAL_StatusTypeDef bytes;
	unsigned char data_read[1];

	reg_buf[0] = (reg_addr >> 8) & 0xFF;
	reg_buf[1] = (reg_addr >> 0) & 0xFF;

	bytes = HAL_I2C_Master_Transmit(&hi2c1, VL6180X, reg_buf, 2, HAL_MAX_DELAY);

	if (bytes != HAL_OK)
	{
		DebugLog("Error in Read Reg---Write\r\n");
		exit(-1);
	}

	bytes = HAL_I2C_Master_Receive(&hi2c1, VL6180X, data_read, 1, HAL_MAX_DELAY);

	if (bytes != HAL_OK)
	{
		DebugLog("Error in Read Reg---Read\r\n");
		exit(-1);
	}

	return data_read[0];

}

void VL6180X_write_reg(uint16_t reg_addr, uint8_t data_to_write)
{
	unsigned char reg_buf[3];
	int bytes;

	reg_buf[0] = (reg_addr >> 8) & 0xFF;
	reg_buf[1] = (reg_addr >> 0) & 0xFF;
	reg_buf[2] = data_to_write;

	bytes = HAL_I2C_Master_Transmit(&hi2c1, VL6180X, reg_buf, 3, HAL_MAX_DELAY);

	if (bytes != HAL_OK)
	{

		DebugLog("Error reading from I2C device\r\n");
		exit(-1);

	}

	return;
}

void VL6180X_init()
{
	char reset;

	// STM Recommended register values. No additional description given. VL6180X consistency improved with these settings.
	VL6180X_write_reg(0x0207, 0x01);
	VL6180X_write_reg(0x0208, 0x01);
	VL6180X_write_reg(0x0096, 0x00);
	VL6180X_write_reg(0x0097, 0xfd);
	VL6180X_write_reg(0x00e3, 0x00);
	VL6180X_write_reg(0x00e4, 0x04);
	VL6180X_write_reg(0x00e5, 0x02);
	VL6180X_write_reg(0x00e6, 0x01);
	VL6180X_write_reg(0x00e7, 0x03);
	VL6180X_write_reg(0x00f5, 0x02);
	VL6180X_write_reg(0x00d9, 0x05);
	VL6180X_write_reg(0x00db, 0xce);
	VL6180X_write_reg(0x00dc, 0x03);
	VL6180X_write_reg(0x00dd, 0xf8);
	VL6180X_write_reg(0x009f, 0x00);
	VL6180X_write_reg(0x00a3, 0x3c);
	VL6180X_write_reg(0x00b7, 0x00);
	VL6180X_write_reg(0x00bb, 0x3c);
	VL6180X_write_reg(0x00b2, 0x09);
	VL6180X_write_reg(0x00ca, 0x09);
	VL6180X_write_reg(0x0198, 0x01);
	VL6180X_write_reg(0x01b0, 0x17);
	VL6180X_write_reg(0x01ad, 0x00);
	VL6180X_write_reg(0x00ff, 0x05);
	VL6180X_write_reg(0x0100, 0x05);
	VL6180X_write_reg(0x0199, 0x05);
	VL6180X_write_reg(0x01a6, 0x1b);
	VL6180X_write_reg(0x01ac, 0x3e);
	VL6180X_write_reg(0x01a7, 0x1f);
	VL6180X_write_reg(0x0030, 0x00);

	VL6180X_write_reg(0x0011, 0x10); // Enables polling for "New Sample Ready" instead of constant sample
	VL6180X_write_reg(0x010a, 0x30); // Sets averaging period of device to value recommended by STM community
	VL6180X_write_reg(0x003f, 0x46); // Sets light gain. Useful for light sensor if implemented in project
	VL6180X_write_reg(0x0031, 0xff); // Sets number of measurements before auto cal to half max
	VL6180X_write_reg(0x0040, 0x63); // ALS integration time to 100ms
	VL6180X_write_reg(0x002e, 0x01); // Perform temperature calibration
	VL6180X_write_reg(0x001b, 0x09); // Set default ranging inter-measurement time to 100ms. Reduces noise
	VL6180X_write_reg(0x003e, 0x31); // Set default ALS inter measurement to 500 ms. Again, reduces noise
	VL6180X_write_reg(0x0014, 0x24); // Configured interrupt on "New Sample Ready" condition. Useful for interrupts if implemented


	reset = VL6180X_read_reg(0x016);

	if (reset == 1)
	{
		VL6180X_write_reg(0x016, 0x00);
	}

	return;
}
