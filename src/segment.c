#include <segment.h>
#include <config.h>
#include <stdint.h>
#include <twi.h>

// charmap (int -> pin states)
const uint8_t SEGMENT_CHARMAP[16] = {
  0x3F,
  0x06,
  0x5B,
  0x4F,
  0x66,
  0x6D,
  0x7D,
  0x07,
  0x7F,
  0x6F,
  0x77,
  0x7C,
  0x39,
  0x5E,
  0x79,
  0x71
};

// updates the value on the filter strength display (0-15) in hexadecimal notation
// references used: https://github.com/Matiasus/HD44780_PCF8574/blob/master/lib/hd44780pcf8574.c
void update_segment_display(uint8_t filter) {
  TWI_MT_Start();
  TWI_Transmit_SLAW(SEGMENT_DISPLAY_ADDR);
  TWI_Transmit_Byte(~SEGMENT_CHARMAP[filter % 16]); // inverted (CA vs CC display)
  TWI_Stop();
}