/**
 * Original implementation : https://github.com/merryhime/gba-multiboot/blob/master/sender/sender.c 
 */
#include <SPI.h>

#define READY 0x01
#define REQUEST_FILE_CHUNK 0x02
#define TRANSMISSION_ERROR 0x03
#define TRANSMISSION_SUCCESS 0x04

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(57600);
  Serial.setTimeout(-1);
  // SPI Settings
  SPI.begin();
  SPI.beginTransaction(SPISettings(256000, MSBFIRST, SPI_MODE3));
  Serial.write(READY);
  gba_transmit_rom();
  //detect_mb_type();
  //detect_gba_test();
}

// SPI Functions:
void spi_write32(uint32_t data) {
  uint8_t buf_write[4];
  buf_write[0] = (data >> 24) & 0xFF; 
  buf_write[1] = (data >> 16) & 0xFF; 
  buf_write[2] = (data >> 8) & 0xFF; 
  buf_write[3] = data & 0xFF;
  SPI.transfer(buf_write);
  delayMicroseconds(100);
}

uint32_t spi_rw32(uint32_t data) {
  uint32_t buf_read[4];
  buf_read[0] = SPI.transfer((data >> 24) & 0xFF);
  buf_read[1] = SPI.transfer((data >> 16) & 0xFF);
  buf_read[2] = SPI.transfer((data >> 8) & 0xFF);
  buf_read[3] = SPI.transfer(data & 0xFF);
  delayMicroseconds(100);
  return (buf_read[0] << 24) | (buf_read[1] << 16) | (buf_read[2] << 8) | buf_read[3];
}

void spi_wait32(uint32_t data, uint32_t expected) {
  uint32_t buf;
  do {
    buf = spi_rw32(data);
  } while(buf != expected);
}

// Serial Functions:
uint32_t serial_read32() {
  uint8_t receive[4];
  uint32_t result = 0;
  Serial.readBytes(receive, 4);
  result = *(uint32_t*)&receive;
  return result;
}

uint8_t serial_read8() {
  uint8_t read;
  Serial.readBytes(&read, 1);
  return read;
}

void gba_transmit_rom() {
  uint32_t token, crc_a, crc_b, crc_c, seed;
  uint32_t chunk_size = 0;
  uint32_t fsend = 0xC0;
  uint32_t length = 0;
  uint8_t header[0xC0];

  // Receive ROM length
  length = serial_read32();

  // Receive header
  for(int i = 0; i < 0xC0; i++) {
    header[i] = serial_read8();
  }
  
  // Waiting for GBA
  Serial.println("Looking for GBA...");
  spi_wait32(0x00006202, 0x72026202);
  Serial.println("GBA Found!");

  //Serial.print("File size received: ");
  //Serial.println(length);

  //Sendig header
  Serial.println("Sending Header");
  (void*)spi_rw32(0x00006102);
  uint16_t *header16 = (uint16_t*)header;
  for(int i = 0; i < 0xC0; i+=2) {
    (void*)spi_rw32(header16[i / 2]);
  }
  (void*)spi_rw32(0x00006200);

  // Getting encryption and crc seeds
  Serial.println("Getting seeds.");
  (void*)spi_rw32(0x00006202);
  (void*)spi_rw32(0x000063D1);

  token = spi_rw32(0x000063D1);
  crc_a = (token >> 16) & 0xFF;
  seed = 0xFFFF00D1 | (crc_a << 8);
  crc_a = (crc_a + 0xF) & 0xFF;

  (void*)spi_rw32(0x00006400 | crc_a);

  uint32_t fsize = length + 0xF;
  fsize &= ~0xF;

  token = spi_rw32((fsize - 0x190) / 4);
  /*if((seed_b >> 24) != 0x73) {
    Serial.println("Failed Handshake");
  }*/
  crc_b = (token >> 16) & 0xFF;
  crc_c = 0xC387;

  // Sending file
  delayMicroseconds(5000);

  Serial.write(REQUEST_FILE_CHUNK);

  uint32_t chunk, tmp;
  while(fsend < fsize) {
    // If we have already proceeded a chunk, then we request the next part
    if(chunk_size == 32){
      //Serial.println("Request Chunk");
      Serial.write(REQUEST_FILE_CHUNK);
      chunk_size = 0;
    }

    // Receive 4 bytes of data
    chunk = serial_read32();
    chunk_size += 4;

    // CRC
    tmp = chunk;
    for(int i = 0; i < 32; i++) {
      uint32_t bit = (crc_c ^ tmp) & 1;
      crc_c = (crc_c >> 1) ^ (bit ? 0xC37B : 0);
      tmp >>= 1;
    }

    // Encrypt
    seed = seed * 0x6F646573 + 1;
    chunk = seed ^ chunk ^ (0xFE000000 - fsend) ^ 0x43202F2F;

    // Send
    uint32_t check = spi_rw32(chunk) >> 16;
    if(check != (fsend & 0xFFFF)) {
      Serial.write(TRANSMISSION_ERROR);
    }
    fsend += 4;
  }
  uint32_t final = 0xFFFF0000 | (crc_b << 8) | crc_a;
  for(int i = 0; i < 32; i++) {
    uint32_t bit = (crc_c ^ final) & 1;
    crc_c = (crc_c >> 1) ^ (bit ? 0xC37B : 0);
    final >>= 1;
  }

  // Final Checksum
  Serial.println("Waiting for checksum...");
  //(void*)WriteSPI32NoDebug(0x00000065);
  (void*)spi_rw32(0x00000065);
  spi_wait32(0x000000065, 0x00750065);
  (void*)spi_rw32(0x00000066);
  uint32_t crc_gba = spi_rw32(crc_c & 0xFFFF) >> 16;
  delayMicroseconds(100);
  if(crc_gba == crc_c) {
    Serial.write(TRANSMISSION_SUCCESS);
  }
}


// Function to test GBA recognition
void detect_gba_test() {
  Serial.println("Waiting for GBA...");
  for(;;) {
    uint32_t token = spi_rw32(0x6202);
    Serial.println(token, HEX);
    if((token >> 16) == 0x7202) {
      break;
    }
  }
  Serial.println("GBA Found!");
}

void loop() {
  // put your main code here, to run repeatedly:
}
