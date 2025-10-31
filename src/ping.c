#include <ping.h>
#include <config.h>
#include <util.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>

float g_distance_cm = 0.0;
bool g_ping_sensor_barrier = false;
uint16_t g_ping_sensor_start = 0;
uint16_t g_ping_sensor_pulse = 0;
ping_sensor_state_t g_ping_sensor_state = START_PULSE;

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
