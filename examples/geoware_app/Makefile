# for <math.h>
TARGET_LIBFILES += -lm
LDFLAGS += -v
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"


CONTIKI_PROJECT = geoware_app

PROJECT_SOURCEFILES = fake_sensors.c aggr.c

APPS = geoware

all: $(CONTIKI_PROJECT)

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

