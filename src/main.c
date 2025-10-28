#include <util/delay.h>

#include <avr/interrupt.h>

#include <stdint.h>

#include <stdlib.h>

#include <stdbool.h>

#include <twi.h>

#include <hd44780pcf8574.h>

// constant macros
#define US_TO_TICKS(us, prescaler)(int)(((us / 1000000.0) * 16000000.0) / prescaler)

// config
#define SEGMENT_DISPLAY_ADDR 0x21
#define I2C_DISPLAY_ADDR 0x27
#define ECHO_PIN PB0
#define TRIGGER_PIN PB1
#define BUZZER_PIN PD3
#define FREQ_MIN 230.0
#define FREQ_MAX 1400.0
#define DIST_MAX 65.0

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

// global variables
float g_distance_cm = 0.0;
bool g_pwm_flag = true;
bool g_ping_sensor_barrier = false;
uint8_t g_volume = 0;
uint16_t g_ping_sensor_start = 0;
uint16_t g_ping_sensor_pulse = 0;
ping_sensor_state_t g_ping_sensor_state = START_PULSE;

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

// get the filtered distance in cm
float get_filtered_distance() {
  return 0.0; // TODO: impl
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

// updates the values of the I2C LCD (note: distance and frequency will be rounded)
void update_twi_display(float dist, float freq) {
  char buf[6]; // room for 5 chars

  // distance
  HD44780_PCF8574_PositionXY(I2C_DISPLAY_ADDR, 11, 0);
  dtostrf(dist, 3, 1, buf); // printf doesn't contain support for %f, luckily AVR has "dtostrf" in stdlib.h
  HD44780_PCF8574_DrawString(I2C_DISPLAY_ADDR, buf);
  HD44780_PCF8574_DrawString(I2C_DISPLAY_ADDR, "  "); // 230 is min, so 2 chars needed

  // frequency
  HD44780_PCF8574_PositionXY(I2C_DISPLAY_ADDR, 11, 1);
  snprintf(buf, sizeof(buf), "%i", (int) freq);
  HD44780_PCF8574_DrawString(I2C_DISPLAY_ADDR, buf);
  HD44780_PCF8574_DrawString(I2C_DISPLAY_ADDR, "  "); // 0.0 is min, so 2 chars needed
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

// ISR for the potentiometer
ISR(ADC_vect) {
  g_volume = ADCH;
}

// main loop
void loop() {
  bool has_new_measurement = read_distance();
  if (has_new_measurement) {
    float dist = get_read_distance();
    float freq = calculate_frequency(dist);
    float vol = get_volume();

    adjust_tone(freq, vol);
    update_twi_display(dist, freq);
  }
}

// entry point
int main() {
  init_twi_display(); // NOTE: already calls TWI_Init();
  init_distance_sensor();
  init_buzzer();
  init_potentiometer();
  sei();

  for (;;) {
    loop();
  }

  return 0;
}