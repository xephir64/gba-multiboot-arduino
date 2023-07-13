# GBA Multiboot Arduino

Send .gba/.mb file to a GBA with an Arduino

## Building

First, make sure you have a C compiler (such as ```gcc``` or ```clang```) and ```make``` installed on your computer.

To build this project, just run make:

```sh
make
```

Then, upload the arduino source code to the arduino 

## Usage

```sh
./send_file_gba.out arduino_serial_interface gba_file
```

## Wiring

```
        _
  _____| |_____
 /             \
|  1    3    5  |
|               |
|  2    4    6  |
 ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
(Looking at the cable)

```

| GBA Pin | Arduino Uno Pin | Usage |
|---------|-----------------|-------|
| 2       | 12              | MISO  |
| 3       | 11              | MOSI  |
| 5       | 13              | SCLK  |
| 6       | GND             | GND   |

## Credits

merryhime : https://github.com/merryhime/gba-multiboot
