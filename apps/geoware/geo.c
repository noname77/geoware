#include "net/rime.h"

#include <stdio.h> /* For printf() */
#include <math.h>  /* For sqrt() */

#include "geo.h"
#include "helpers.h"

void print_pos(pos_t pos) {
  printf("x - "PRINTFLOAT" y - "PRINTFLOAT"\n", \
  	(long)pos.x, decimals(pos.x), (long)pos.y, decimals(pos.y));
}

float distance(pos_t a, pos_t b) {
	float diff_x = a.x - b.x;
	float diff_y = a.y - b.y;

	return sqrt(diff_x*diff_x + diff_y*diff_y);
}

int pos_cmp(pos_t a, pos_t b) {
	return (a.x == b.x) && (a.y == b.y);
}