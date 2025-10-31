#ifndef FILTER_H
#define FILTER_H

#include <stdint.h>
#include <stdbool.h>

// enum to represent the state of the 2 buttons
typedef enum filter_selector_state {
  NONE,
  UP,
  DOWN
}
filter_selector_state_t;

// struct for the ds
typedef struct filter_entry {
  uint8_t age;
  float value;
} filter_entry_t;

int filter_compare(const void* a, const void* b);
float get_filtered_distance();
bool set_filter_size(int size);
void clear_filter();
void push_filter_value(float dist_sample);
void init_buttons();

#endif // FILTER_H