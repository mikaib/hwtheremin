#include <avr/interrupt.h>
#include <config.h>
#include <util.h>
#include <segment.h>
#include <lcd.h>
#include <ping.h>
#include <filter.h>
#include <tone.h>
#include <volume.h>

// handle a measurement
void handle_measurement() {
  float dist = get_read_distance();
  push_filter_value(dist);

  float dist_filtered = get_filtered_distance();
  float freq = MAP_FREQUENCY(dist_filtered); // maps distance -> frequency according to values in config.h
  float vol = get_volume();

  adjust_tone(freq, vol);
  update_twi_display(dist_filtered, freq);
}

// main loop
void loop() {
  bool has_new_measurement = read_distance();

  if (has_new_measurement) {
    handle_measurement();  
  }
}

// entry point
#ifndef UNIT_TEST // disable main() when testing is enabled
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
#endif