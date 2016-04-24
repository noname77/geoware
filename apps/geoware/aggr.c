#include <float.h>

#include "geoware.h"
#include "aggr.h"

reading_owned maximum(sid_t sID, uint8_t num) {
  reading_owned rmax;
  mapping_t *mapping;
  uint8_t i;

  mapping = get_mapping(get_subscription(sID)->type);

  switch(mapping->r) {
    case UINT8:
      rmax.value.ui8 = 0;
      break;
    case UINT16:
      rmax.value.ui16 = 0;
      break;
    case FLOAT:
      rmax.value.fl = -FLT_MAX;
      break;
  }

  for (i = 0; i < num; i++)
  {
    reading_owned tmp = get_reading_sid(sID);
    // extra check that there are enough readings buffered
    // but it should already be handled correctly elswere
    if(tmp.value.fl == FLT_MAX) {
      break;
    }

    switch(mapping->r) {
      case UINT8:
        if(tmp.value.ui8 > rmax.value.ui8) {
          rmax.value = tmp.value;
        }
        break;
      case UINT16:
        if(tmp.value.ui16 > rmax.value.ui16) {
          rmax.value = tmp.value;
        }
        break;
      case FLOAT:
        if(tmp.value.fl > rmax.value.fl) {
          rmax.value = tmp.value;
        }
        break;
    }
  }

  return rmax;
}

reading_owned average(sid_t sID, uint8_t num) {
  reading_owned avg;
  mapping_t *mapping;
  uint8_t i;

  mapping = get_mapping(get_subscription(sID)->type);

  switch(mapping->r) {
    case UINT8:
      avg.value.ui8 = 0;
    case UINT16:
      avg.value.ui16 = 0;
    case UINT32:
      avg.value.ui32 = 0;
    case FLOAT:
      avg.value.fl = 0;
      break;
  }

  for (i = 0; i < num; i++)
  {
    reading_owned tmp = get_reading_sid(sID);

    // extra check that there are enough readings buffered
    // but it should already be handled correctly elswere
    if(tmp.value.fl == FLT_MAX) {
      break;
    }

    switch(mapping->r) {
      case UINT8:
        avg.value.ui32 += tmp.value.ui8;
        break;
      case UINT16:
        avg.value.ui32 += tmp.value.ui16;
        break;
      case FLOAT:
        avg.value.fl += tmp.value.fl;
        break;
    }
  }

  switch(mapping->r) {
    case UINT8:
      avg.value.ui8 = (uint8_t) (avg.value.ui32 / i);
      break;
    case UINT16:
      avg.value.ui16 = (uint16_t) (avg.value.ui32 / i);
      break;
    case FLOAT:
      avg.value.fl /= i;
      break;
  }

  return avg;
}