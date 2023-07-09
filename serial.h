#ifndef SERIAL_H
#define SERIAL_H
#include <stdint.h>
#include <unistd.h>
#include <string.h>
// Function to send data (8 bit unsigned integer) over the serial port
void serial_write(int arduino, uint8_t* data, int length);
// Function to read data from the serial port
void serial_read(int arduino, uint8_t* data, int length);
// Function to send data (32 bit unsigned integer) over the serial port
void serial_write32(int arduino, uint32_t* data, int length);
#endif