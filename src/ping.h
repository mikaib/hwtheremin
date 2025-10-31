#ifndef PING_H
#define PING_H

#include <stdbool.h>

// enum to represent the state of the ping sensor
typedef enum ping_sensor_state {
  START_PULSE,
  STOP_PULSE,
  WAIT
}
ping_sensor_state_t;

bool read_distance();
float get_read_distance();
void init_distance_sensor();

#endif // PING_H