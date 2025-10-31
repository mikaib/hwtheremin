#include <util/delay.h>
#include <avr/interrupt.h>
#include <config.h>
#include <util.h>
#include <segment.h>
#include <lcd.h>
#include <ping.h>
#include <filter.h>
#include <tone.h>
#include <volume.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <twi.h>
#include <hd44780pcf8574.h>

// calculates the output frequency based on a given distance
float calculate_frequency(float dist) {
  return FREQ_MAX - ((FREQ_MAX - FREQ_MIN) * (dist > DIST_MAX ? DIST_MAX : dist)) / DIST_MAX;
}

// main loop
void loop() {
  bool has_new_measurement = read_distance();
  if (has_new_measurement) {
    float dist = get_read_distance();
    push_filter_value(dist);

    float dist_filtered = get_filtered_distance();
    float freq = calculate_frequency(dist_filtered);
    float vol = get_volume();

    adjust_tone(freq, vol);
    update_twi_display(dist_filtered, freq);
  }
}

// entry point
int main() {
  init_twi_display(); // NOTE: already calls TWI_Init();
  init_distance_sensor();
  init_buzzer();
  init_potentiometer();
  init_buttons();
  sei();

  update_segment_display(1);
  set_filter_size(1);
  clear_filter();

  for (;;) {
    loop();
  }

  return 0;
}