#include <avr/delay.h>
#include <stdint.h>
#include <stdlib.h>
#include <twi.h>
#include <hd44780pcf8574.h>

// config
#define SEGMENT_DISPLAY_ADDR 0x21
#define I2C_DISPLAY_ADDR     0x27

// charmap (int -> pin states)
const uint8_t SEGMENT_CHARMAP[16] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};

// enum to represent the state of the 2 buttons
typedef enum filter_selector_state {
  NONE,
  UP,
  DOWN
} filter_selector_state_t;

// enum to represent the state of the ping sensor
typedef enum ping_sensor_state {
  START_PULSE,
  STOP_PULSE,
  WAIT
} ping_sensor_state_t;

// global variables
float g_volume = 0.0;
float g_distance_cm = 0.0;
uint64_t g_ping_start_time = 0.0;
ping_sensor_state_t g_ping_sensor_state = START_PULSE;

// get the volume as a scalar (0.0 - 1.0)
float get_volume() {
  return g_volume; 
}

// set the volume scalar (0.0 - 1.0)
void set_volume(float scalar) {
  g_volume = scalar;
}

// reads out the pot meter (0.0 - 1.0)s
float read_potentiometer() {
  return 0.0; // TODO: impl, perhaps GetVolume is reading the potentiometer and SetVolume is for the sound modulation state? Currently this feels like violating DRY principles.
}

// signal sensor to read the distance
void read_distance() {
  g_distance_cm = 0.0; // TODO: impl
}

// read the value of the distance sensor in cm
float get_read_distance() {
  return g_distance_cm;
}

// get the filtered distance in cm
float get_filtered_distance() {
  return 0.0; // TODO: impl
}

// calculates the output frequency based on a given distance
float calculate_frequency(float dist) {
  return g_distance_cm; // TODO: impl (mapping of dist->freq)
}

// adjusts the tone using given values
void adjust_tone(float freq, float vol) {
  // TODO: impl
}

// initializes the TWI display
void init_twi_display() {
  HD44780_PCF8574_Init(I2C_DISPLAY_ADDR); // already calls TWI_Init();
  HD44780_PCF8574_DisplayClear(I2C_DISPLAY_ADDR);
  HD44780_PCF8574_DisplayOn(I2C_DISPLAY_ADDR);
  HD44780_PCF8574_PositionXY(I2C_DISPLAY_ADDR, 0, 0);
  HD44780_PCF8574_DrawString(I2C_DISPLAY_ADDR, "Dist (cm): 0.0");
  HD44780_PCF8574_PositionXY(I2C_DISPLAY_ADDR, 0, 1);
  HD44780_PCF8574_DrawString(I2C_DISPLAY_ADDR, "Freq (hz): 0");
}

// updates the values of the I2C LCD (note: distance and frequency will be rounded)
void update_twi_display(float dist, float freq) {
  char buf[6]; // room for 5 chars

  // distance
  HD44780_PCF8574_PositionXY(I2C_DISPLAY_ADDR, 11, 0);
  dtostrf(dist, 3, 1, buf); // printf doesn't contain support for %f, luckily AVR has "dtostrf" in stdlib.h
  HD44780_PCF8574_DrawString(I2C_DISPLAY_ADDR, buf);

  // frequency
  HD44780_PCF8574_PositionXY(I2C_DISPLAY_ADDR, 11, 1);
  snprintf(buf, sizeof(buf), "%i", (int)freq);
  HD44780_PCF8574_DrawString(I2C_DISPLAY_ADDR, buf);
}

// updates the value on the filter strength display (0-15) in hexadecimal notation
// references used: https://github.com/Matiasus/HD44780_PCF8574/blob/master/lib/hd44780pcf8574.c
void update_segment_display(uint8_t filter) {
  TWI_MT_Start();
  TWI_Transmit_SLAW(SEGMENT_DISPLAY_ADDR);
  TWI_Transmit_Byte(~SEGMENT_CHARMAP[filter % 16]); // inverted (CA vs CC display)
  TWI_Stop();
}

// reads the buttons for the filter selection
filter_selector_state_t read_buttons() {
  return NONE; // TODO: impl
}

// sets the size of the filter
void set_filter_size(int size) {
  // TODO: impl
}

int main() {
  init_twi_display(); // NOTE: already calls TWI_Init();

  // TODO: remove test code
  for(;;) {
    read_distance();
    update_twi_display(g_distance_cm, 0);
  }

  return 0;
}