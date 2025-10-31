#include <filter.h>
#include <util.h>
#include <segment.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

uint8_t g_filter_size = 0;
uint8_t g_filter_capacity = 0;
filter_entry_t* g_filter_arr = NULL;
filter_selector_state_t g_button_state = NONE;
filter_selector_state_t g_last_button_state = NONE;

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
  int center = MIN(g_filter_size / 2, g_filter_size - 1); // center would be: [0, X], [0, X, 0], [0, X, 0, 0], [0, 0, X, 0, 0]

  memcpy(sorted, g_filter_arr, sizeof(filter_entry_t) * g_filter_size);
  qsort(sorted, g_filter_size, sizeof(filter_entry_t), filter_compare);

  return sorted[center].value;
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

    // increment ages
    for (int idx = 0; idx < g_filter_size; idx++) {
        g_filter_arr[idx].age++;
    }

    // push to end until full
    if (g_filter_size < g_filter_capacity) {
        g_filter_arr[g_filter_size++] = (filter_entry_t){ 0, dist_sample };
        return;
    }

    // find oldest
    int oldest_idx = 0;
    int oldest_age = g_filter_arr[0].age;
    for (int idx = 1; idx < g_filter_size; idx++) {
        if (g_filter_arr[idx].age > oldest_age) {
            oldest_age = g_filter_arr[idx].age;
            oldest_idx = idx;
        }
    }

    // overwrite oldest
    g_filter_arr[oldest_idx].age = 0;
    g_filter_arr[oldest_idx].value = dist_sample;
}

// initializes the buttons for the filter config
void init_buttons() {
  DDRD &= ~((1 << PD4) | (1 << PD5)); // PD4, PD5 input
  PORTD |= (1 << PD4) | (1 << PD5); // enable pull-up
  PCICR |= (1 << PCIE2); // pin change interrupt 2 enable
  PCMSK2 |= (1 << PCINT20) | (1 << PCINT21); // enable PCINT20 and PCINT21
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