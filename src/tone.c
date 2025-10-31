#include <tone.h>
#include <config.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

bool g_pwm_flag = true;

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

// adjusts the tone using given values, volume is normalized between 0 and 1
void adjust_tone(float freq, uint8_t vol) {
  // frequency
  uint32_t period = 16000000UL / (256UL * 2UL * freq);
  OCR0A = period > 255 ? 255 : (uint8_t) period;

  // volume
  OCR2B = vol;
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