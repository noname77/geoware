#include "lib/random.h"

#include "fake_sensors.h"
#include "helpers.h"

// in celcius
float get_temperature() {
  static float initial = 20.0;
  static uint8_t counter = 0;

  // every 5 readings add +/- 1.0 to the baseline
  if ((++counter%5) == 0) {
  	initial += ((float) (random_rand() % 21)) / 10 - 1.0;
  }

  // return the baseline + (0.0-0.5)
  return initial + ((float) (random_rand()%6)) / 10;
}

// in lux
uint16_t get_light() {
  static uint16_t initial = 50;
  static uint8_t counter = 0;

  // initialize to 5 - night, 50 - room, 500 - office/sunrise, 5000 - cloudy day, 50000 - direct sunlight
  if (++counter == 1)
  {
  	initial = (uint16_t) (5 * powi(10, (random_rand() % 5)));
  }
  else if (counter%5 == 0) {
  	initial += (random_rand() % 3) * (initial/10);
  }

  if (counter == 100) counter = 0;

  return initial + random_rand()%(initial/10);
}

// in %
uint8_t get_humidity() {
  uint8_t h = 50 + random_rand()%10;
  return h;
}