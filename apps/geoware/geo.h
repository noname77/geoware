#ifndef GEO_H
#define GEO_H

#define EPSILON 0.05

typedef struct {
  float x;
  float y;
} pos_t;

void print_pos(pos_t pos);
float distance(pos_t a, pos_t b);
int pos_cmp(pos_t a, pos_t b);

#endif