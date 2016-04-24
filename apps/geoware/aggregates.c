#include "geoware.h"

MEMB_GLOBAL(aggrs_memb, struct aggregate, MAX_AGGREGATES);
LIST_GLOBAL(aggregate_list);

void aggregates_init() {
  memb_init(&aggrs_memb);
  list_init(aggregate_list);
}

aggr_mapping_t*
get_aggregate(sid_t sID)
{
  struct aggregate *a;
  aggr_t type = get_subscription(sID)->aggr_type;

  for(a = list_head(aggregate_list); a != NULL; a = list_item_next(a)) {
    /* We break out of the loop if the sID in quesiton matches current
       subscription. */
    if(a->mapping->type == type) {
      return a->mapping;
    }
  }

  return NULL;
}