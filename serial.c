#include "serial.h"
// Function to send data (8 bit unsigned integer) over the serial port
void serial_write(int arduino, uint8_t* data, int length) {
  write(arduino, data, length);
  usleep(100); 
}

// Function to send data (32 bit unsigned integer) over the serial port
void serial_write32(int arduino, uint32_t* data, int length) {
  uint8_t send[4];
  memcpy(&send[0], data, 4);
  write(arduino, send, 4);
  usleep(100);
}

// Function to read data from the serial port
void serial_read(int arduino, uint8_t* data, int length) {
  read(arduino, data, length);
  usleep(100);
}
