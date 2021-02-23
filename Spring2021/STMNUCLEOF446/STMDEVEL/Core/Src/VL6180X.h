#ifndef VL6180X_H
#define VL6180X_H

#include "main.h"

// Function definitions from VL6180X.c
uint8_t VL6180X_read_reg(uint16_t reg_addr);
uint8_t VL6180X_single_range_measurement();
void VL6180X_init();
void VL6180X_write_reg(uint16_t reg_addr, uint8_t data_to_write);
void VL6180X_test_write(uint16_t reg_to_test);

/* Defined constants for device address and registers needed for range measurements */
//VL6180X  default address
static const int VL6180X = 0x52;

//ID Register
static const uint16_t ID_REG = 0x000;

//#define VL6180X	((uint16_t)(0x29))
//#define ID_REG	((uint16_t)(0x000))

#endif
