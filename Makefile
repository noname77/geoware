# for <math.h>
TARGET_LIBFILES += -lm
LDFLAGS += -v
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"


CONTIKI_PROJECT = geoware

PROJECT_SOURCEFILES = helpers.c commands.c geo.c fake_sensors.c
PROJECT_SOURCEFILES += app.c

APPS = serial-shell

all: $(CONTIKI_PROJECT)

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

