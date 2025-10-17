#include <stdint.h>

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

int main() {
  return 0;
}