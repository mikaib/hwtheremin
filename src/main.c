#include <util/delay.h>
#include <avr/interrupt.h>
#include <config.h>
#include <util.h>
#include <segment.h>
#include <lcd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <twi.h>
#include <hd44780pcf8574.h>

// enum to represent the state of the 2 buttons
typedef enum filter_selector_state {
  NONE,
  UP,
  DOWN
}
filter_selector_state_t;

// enum to represent the state of the ping sensor
typedef enum ping_sensor_state {
  START_PULSE,
  STOP_PULSE,
  WAIT
}
ping_sensor_state_t;

// struct for the ds
typedef struct filter_entry {
  uint8_t age;
  float value;
} filter_entry_t;

// global variables
float g_distance_cm = 0.0;
bool g_pwm_flag = true;
bool g_ping_sensor_barrier = false;
uint8_t g_volume = 0;
uint8_t g_filter_size = 0;
uint8_t g_filter_capacity = 0;
filter_entry_t* g_filter_arr = NULL;
uint16_t g_ping_sensor_start = 0;
uint16_t g_ping_sensor_pulse = 0;
ping_sensor_state_t g_ping_sensor_state = START_PULSE;
filter_selector_state_t g_button_state = NONE;
filter_selector_state_t g_last_button_state = NONE;

// get the volume as a scalar (0.0 - 1.0)
uint8_t get_volume() {
  return g_volume;
}

// signal sensor to read the distance
bool read_distance() {
  bool has_new_measurement = false;

  switch (g_ping_sensor_state) {
  case START_PULSE: {
    PORTB |= (1 << TRIGGER_PIN);
    g_ping_sensor_start = TCNT1;
    g_ping_sensor_state = STOP_PULSE;
    break;
  }

  case STOP_PULSE: {
    uint16_t diff = TCNT1 - g_ping_sensor_start;
    if (diff >= US_TO_TICKS(10, 8)) {
      PORTB &= ~(1 << TRIGGER_PIN);
      g_ping_sensor_state = WAIT;
    }
    break;
  }

  case WAIT: {
    if (g_ping_sensor_barrier) {
      has_new_measurement = true;
      g_ping_sensor_barrier = false;
      g_ping_sensor_state = START_PULSE;
      g_distance_cm = g_ping_sensor_pulse / 58.0;
    }
    break;
  }
  }

  return has_new_measurement;
}

// read the value of the distance sensor in cm
float get_read_distance() {
  return g_distance_cm;
}

// compare function for qsort
int filter_compare(const void* a, const void* b) {
  float a_val = ((filter_entry_t*)a)->value;
  float b_val = ((filter_entry_t*)b)->value;

  if (a_val < b_val) return -1;
  if (a_val > b_val) return 1;

  return 0;
}

// get the filtered distance in cm
float get_filtered_distance() {
  if (g_filter_arr == NULL) {
    return 0.0;
  }

  filter_entry_t sorted[g_filter_size];
  int center = MIN((g_filter_size / 2) + 1, g_filter_size - 1);

  memcpy(sorted, g_filter_arr, sizeof(filter_entry_t) * g_filter_size);
  qsort(sorted, g_filter_size, sizeof(filter_entry_t), filter_compare);

  return g_filter_arr[center].value;
}

// sets the size of the filter
bool set_filter_size(int size) {
  if (g_filter_arr != NULL) {
    filter_entry_t* resized = realloc(g_filter_arr, sizeof(filter_entry_t) * size);
    if (resized == NULL) {
      return false;
    }

    g_filter_arr = resized;
    g_filter_capacity = size;
    g_filter_size = MIN(g_filter_size, size);
    return true;
  }

  g_filter_arr = malloc(sizeof(filter_entry_t) * size);
  if (g_filter_arr == NULL) {
    return false;
  }

  g_filter_capacity = size;
  g_filter_size = 0;
  return true;
}

// clear the filter
void clear_filter() {
  if (g_filter_arr == NULL) {
    return;
  }

  memset(g_filter_arr, 0, sizeof(filter_entry_t) * g_filter_capacity); // NOTE: 0 for both float and uint8_t is valid
  g_filter_size = 0;
}

// push a value to the filter, shifts to the left if full
void push_filter_value(float dist_sample) {
  // ensure arr is valid
  if (g_filter_arr == NULL) {
    return;
  }

  // push to end
  if (g_filter_size < g_filter_capacity) {
    g_filter_arr[g_filter_size++] = (filter_entry_t){ 0, dist_sample };
    return;
  }

  // increment ages
  for (int idx = 0; idx < g_filter_size; idx++) {
    g_filter_arr[idx].age++;
  }

  // find olders
  int oldest_age = 0;
  int oldest_idx = -1;
  for (int idx = 0; idx < g_filter_size; idx++) {
    if (g_filter_arr[idx].age > oldest_age) {
      oldest_age = g_filter_arr[idx].age;
      oldest_idx = idx;
    }
  }

  // set new value
  g_filter_arr[oldest_idx].age = 0;
  g_filter_arr[oldest_idx].value = dist_sample;
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

// initializes the distance sensor
void init_distance_sensor() {
  DDRB |= (1 << TRIGGER_PIN);
  TCCR1A = 0x00; // normal mode
  TCCR1B = (1 << ICES1) | (1 << CS11); // rising edge, prescaler 8
  TIMSK1 = (1 << ICIE1); // input capture interrupt (TIMER1_CAPT_vect)
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

// initializes the buttons for the filter config
void init_buttons() {
  DDRD &= ~((1 << PD4) | (1 << PD5)); // PD4, PD5 input
  PORTD |= (1 << PD4) | (1 << PD5); // enable pull-up
  PCICR |= (1 << PCIE2); // pin change interrupt 2 enable
  PCMSK2 |= (1 << PCINT20) | (1 << PCINT21); // enable PCINT20 and PCINT21
}

// ISR for timer1 (ultrasonic sensor)
ISR(TIMER1_CAPT_vect) {
  if (TCCR1B & (1 << ICES1)) {
    TCCR1B &= ~(1 << ICES1);
    TCNT1 = 0;
  } else {
    g_ping_sensor_pulse = TCNT1;
    g_ping_sensor_barrier = true;
    TCCR1B |= (1 << ICES1);
  }
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

// ISR for the buttons
ISR(PCINT2_vect) {
  if (!(PIND & (1 << PD4))) {
    if (g_button_state != DOWN) {
      set_filter_size(LIMIT(g_filter_capacity - 2, 1, 15));
      update_segment_display(g_filter_capacity);
    }
    
    g_button_state = DOWN;
    _delay_ms(50);
    return;
  }
  
  if (!(PIND & (1 << PD5))) {
    if (g_button_state != UP) {
      set_filter_size(LIMIT(g_filter_capacity + 2, 1, 15));
      update_segment_display(g_filter_capacity);
    }

    g_button_state = UP;
    _delay_ms(50);
    return;
  }
  
  g_button_state = NONE;
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