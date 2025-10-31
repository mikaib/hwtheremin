#include <util/delay.h>
#include <avr/interrupt.h>
#include <config.h>
#include <util.h>
#include <segment.h>
#include <lcd.h>
#include <ping.h>
#include <filter.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <twi.h>
#include <hd44780pcf8574.h>

// global variables
bool g_pwm_flag = true;
uint8_t g_volume = 0;

// get the volume as a scalar (0.0 - 1.0)
uint8_t get_volume() {
  return g_volume;
}

// calculates the output frequency based on a given distance
float calculate_frequency(float dist) {
  return FREQ_MAX - ((FREQ_MAX - FREQ_MIN) * (dist > DIST_MAX ? DIST_MAX : dist)) / DIST_MAX;
}

// adjusts the tone using given values, volume is normalized between 0 and 1
void adjust_tone(float freq, uint8_t vol) {
  // frequency
  uint32_t period = 16000000UL / (256UL * 2UL * freq);
  OCR0A = period > 255 ? 255 : (uint8_t) period;

  // volume
  OCR2B = vol;
}

// reads the buttons for the filter selection
filter_selector_state_t read_buttons() {
  return NONE; // TODO: impl
}

// initialize the buzzer
void init_buzzer() {
  // timer2 fast pwm, volume
  DDRD |= (1 << BUZZER_PIN); // buzzer pin output
  TCCR2A = (1 << WGM20) | (1 << WGM21) | (1 << COM2B1); // fast pwm mode, non-inverting, ORC2B duty cycle
  TCCR2B = (1 << CS20); // system clock (no prescaler)

  // timer0 ctc, frequency
  TCCR0A = (1 << WGM01); // CTC mode
  TCCR0B = (1 << CS02); // prescaler 256
  TIMSK0 = (1 << OCIE0A); // Compare Match A interrupt

  // set defaults
  adjust_tone(FREQ_MIN, 0);
}

// initializes the pot meter
void init_potentiometer() {
  ADMUX = (1 << REFS0) | (1 << ADLAR); // 8bit, AVcc ref
  ADCSRA = (1 << ADEN) | (1 << ADSC) | (1 << ADATE) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // active, auto trigger enable bit, freerunning, enable interrupt, prescaler 128
  ADCSRB = 0x00; // freerunning
}

// ISR for tone modulation
ISR(TIMER0_COMPA_vect) {
  if (g_pwm_flag) {
    TCCR2A |= (1 << COM2B1);
  } else {
    TCCR2A &= ~(1 << COM2B1);
  }
  g_pwm_flag = !g_pwm_flag;
}

// ISR for the potentiometer
ISR(ADC_vect) {
  g_volume = ADCH;
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