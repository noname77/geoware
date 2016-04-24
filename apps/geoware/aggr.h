#ifndef AGGR_H
#define AGGR_H

#include "geoware.h"

#define NONE 	0
#define MAXIMUM 1
#define AVERAGE 2

reading_owned maximum(sid_t sID, uint8_t num);
reading_owned average(sid_t sID, uint8_t num);

#endif