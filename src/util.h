#ifndef UTIL_H
#define UTIL_H

#include <config.h>

#define US_TO_TICKS(us, prescaler)(int)(((us / 1000000.0) * 16000000.0) / prescaler)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define LIMIT(x, min, max) (MIN(MAX((x), (min)), (max)))
#define MAP_FREQUENCY(dist) FREQ_MAX - ((FREQ_MAX - FREQ_MIN) * (dist > DIST_MAX ? DIST_MAX : dist)) / DIST_MAX;

#endif // UTIL_H