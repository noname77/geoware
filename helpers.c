#include <math.h>  /* For pow() */

#include "helpers.h"

// returns the fractional part of the floating point number as unsigned integer
// needed for printing since printf implementation in contiki does not support
// floating point numbers
unsigned decimals(float f) {
	return (unsigned)((f-floor(f))*1000);
}

// string to float implementation since atof is not present in contiki stdlib
// taken from http://stackoverflow.com/a/4392789
float stof(const char* s){
  float rez = 0, fact = 1;
  int point_seen;
  if (*s == '-'){
    s++;
    fact = -1;
  };
  for (point_seen = 0; *s; s++){
    if (*s == '.'){
      point_seen = 1; 
      continue;
    };
    int d = *s - '0';
    if (d >= 0 && d <= 9){
      if (point_seen) fact /= 10.0f;
      rez = rez * 10.0f + (float)d;
    };
  };
  return rez * fact;
};

// math.h pow not present, so simple, no checking
uint32_t powi(uint16_t x, uint16_t y) {
  uint32_t res = x;
  int i;

  if (y == 0) {
    res = 1;
  }
  else if (y > 1) {
    for(i = 1; i<y; i++) {
      res *= x;
    }
  }

  return res;
}
