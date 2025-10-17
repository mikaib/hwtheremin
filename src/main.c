#include <stdint.h>

// enum to represent the state of the 2 buttons
typedef enum filter_selector_state {
  NONE,
  UP,
  DOWN
} filter_selector_state_t;

// global variables
float g_volume = 0.0;
float g_distance_cm = 0.0;

// get the volume as a scalar (0.0 - 1.0)
float get_volume() {
  return g_volume; 
}

// set the volume scalar (0.0 - 1.0)
void set_volume(float scalar) {
  g_volume = scalar;
}

// reads out the pot meter (0.0 - 1.0)
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

// updates the values of the I2C LCD (note: distance and frequency will be rounded)
void update_twi_display(float dist, float freq) {
  // TODO: impl
}

// updates the value on the filter strength display (0-15) in hexadecimal notation
void update_segment_display(uint8_t filter) {
  // TODO: impl
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
  return 0;
}