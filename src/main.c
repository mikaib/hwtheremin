#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <twi.h>
#include <hd44780pcf8574.h>

// constant macros
#define US_TO_TICKS(us, prescaler) (int)(((us / 1000000.0) * 16000000.0) / prescaler)

// config
#define SEGMENT_DISPLAY_ADDR 0x21
#define I2C_DISPLAY_ADDR     0x27
#define ECHO_PIN             PB0
#define TRIGGER_PIN          PB1
#define FREQ_MIN             230.0
#define FREQ_MAX             1400.0
#define DIST_MAX             65.0

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
bool g_ping_sensor_barrier = false;
uint16_t g_ping_sensor_start = 0;
uint16_t g_ping_sensor_pulse = 0;
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

// initializes the distance sensor
void init_distance_sensor() {
  DDRB |= (1 << TRIGGER_PIN);
  TCCR1A = 0x00; // normal mode
  TCCR1B = (1 << ICES1) | (1 << CS11); // rising edge, prescaler 8 
  TIMSK1 = (1 << ICIE1); // input capture interrupt (TIMER1_CAPT_vect)
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

// main loop
void loop() {
  bool has_new_measurement = read_distance();
  if (has_new_measurement) {
      float dist = get_read_distance();
      float freq = calculate_frequency(dist);

      update_twi_display(dist, freq);
  }
}

// entry point
int main() {
  init_twi_display(); // NOTE: already calls TWI_Init();
  init_distance_sensor();
  sei();

  for(;;) {
    loop();
  }

  return 0;
}