#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <stdint.h>
#include "serial.h"

#define READY 0x01
#define REQUEST_SIZE_HEADER 0x02
#define REQUEST_FILE_CHUNK 0x03
#define TRANSMISSION_ERROR 0x04
#define TRANSMISSION_SUCCESS 0x05

// Function to send the GBA/MB file
int send_gba_file(int arduino, FILE* gba_file, uint32_t length) {
  uint8_t header[0xC0]; // ROM Header of size 0xC0
  uint8_t gba_buf[32]; // Chunk of 32 bytes of data
  uint32_t bytes_read_total = 0xC0; // Total bytes read
  uint8_t ack = 0x06;
  for(;;) {
    uint8_t buffer[128] = {0};
    ssize_t bytesRead = read(arduino, buffer, sizeof(buffer));
    if(bytesRead > 0) {
      //printf("0x%02x\n", buffer[0]);
      switch(buffer[0]) {
        case READY:
          serial_write(arduino, &ack, sizeof(uint8_t));
          break;
        case REQUEST_SIZE_HEADER:
          printf("Sending ROM Size\n");
          serial_write32(arduino, &length, sizeof(length));
          printf("Sending Header...\n");
          fread(header, sizeof(uint8_t), 0xC0, gba_file);
          for(int i = 0; i < sizeof(header); i++) {
              serial_write(arduino, &header[i], sizeof(uint8_t));
          }
          break;
        case REQUEST_FILE_CHUNK:
          if(!feof(gba_file)) {
            uint32_t bytes_read = fread(gba_buf, 1, sizeof(gba_buf), gba_file);
            serial_write(arduino, gba_buf, bytes_read);
            bytes_read_total += bytes_read;
            printf("%d / %d\n", bytes_read_total, length);
          }
          break;
        case TRANSMISSION_ERROR:
          fprintf(stderr, "Transmission error");
          return -1;
        case TRANSMISSION_SUCCESS:
          printf("Multiboot done!\n");
          return 0;
        default:
          printf("Message: %s\n", buffer);
          memset(buffer, 0, sizeof(buffer));
          break;
      }
    }
  }
  return 0;
}


int main(int argc, char** const argv) {
  FILE* gba_file;
  struct termios arduino_config;
  // Check arguments
  if(argc != 3) {
          fprintf(stderr, "Usage: %s device_port gba_file_path\n", argv[0]);
          return 1;
  }
  // First, we try to open the file
  gba_file = fopen(argv[2], "rb");
  if(!gba_file) {
      fprintf(stderr, "Error opening the rom file\n");
      return 1;
  }

  // Then, we try to open the communication with the Arduino
  int arduino = open(argv[1], O_RDWR | O_NOCTTY | O_SYNC | O_NDELAY);
  if(arduino == -1) {
      fprintf(stderr, "Error opening Arduino serial port\n");
      return 1;
  }

  memset(&arduino_config, 0, sizeof(arduino_config));
  tcgetattr(arduino, &arduino_config);

  // Set baud rate to match the Arduino config
  cfsetispeed(&arduino_config, B115200);
  cfsetospeed(&arduino_config, B115200);

  // Set other serial port settings
  // 8N1
  arduino_config.c_cflag &= ~PARENB;
  arduino_config.c_cflag &= ~CSTOPB;
  arduino_config.c_cflag &= ~CSIZE;
  arduino_config.c_cflag |= CS8;


  arduino_config.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
  arduino_config.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl
  arduino_config.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  arduino_config.c_oflag &= ~OPOST;

  // See: http://unixwiz.net/techtips/termios-vmin-vtime.html
  arduino_config.c_cc[VMIN]  = 0;
  arduino_config.c_cc[VTIME] = 0;

  tcsetattr(arduino, TCSANOW, &arduino_config);
    
  // Get file size
  fseek(gba_file, 0L, SEEK_END); // Go to the end of the file
  long gba_file_size = ftell(gba_file); // Get position
  rewind(gba_file); // Rewind the file to the first byte

  printf("File size is: %ld \n", gba_file_size);

  if(send_gba_file(arduino, gba_file, gba_file_size) == -1) {
    return 1;
  }

  close(arduino);
  fclose(gba_file);
  return 0;
}