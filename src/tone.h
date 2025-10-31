#ifndef TONE_H
#define TONE_H

#include <stdint.h>

void init_buzzer();
void adjust_tone(float freq, uint8_t vol);

#endif // TONE_H