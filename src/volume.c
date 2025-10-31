#include <volume.h>
#include <avr/io.h>
#include <avr/interrupt.h>

uint8_t g_volume = 0;

// initializes the pot meter
void init_potentiometer() {
  ADMUX = (1 << REFS0) | (1 << ADLAR); // 8bit, AVcc ref
  ADCSRA = (1 << ADEN) | (1 << ADSC) | (1 << ADATE) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // active, auto trigger enable bit, freerunning, enable interrupt, prescaler 128
  ADCSRB = 0x00; // freerunning
}

// get the volume as a scalar (0.0 - 1.0)
uint8_t get_volume() {
  return g_volume;
}

// ISR for the potentiometer
ISR(ADC_vect) {
  g_volume = ADCH;
}