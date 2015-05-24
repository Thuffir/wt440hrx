USE_PIGPIOD = false

ifeq ($(USE_PIGPIOD), true)
GPIOLIB = pigpiod_if
GPIODEFINE = -DUSE_PIGPIOD
else
GPIOLIB = pigpio
GPIODEFINE =
endif

TARGET = wt440hrx
CC = gcc
CFLAGS = -O3 -Wall -fomit-frame-pointer $(GPIODEFINE)
LIBS = -l$(GPIOLIB) -lpthread -lrt
LFLAGS = -s

INSTALLDIR = /opt/fhem
ifeq ($(USE_PIGPIOD), true)
INSTALL = sudo install -m 755 -o fhem -g dialout
else
INSTALL = sudo install -m 755 -o root -g root
endif

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(LFLAGS) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)

install: $(TARGET)
	$(INSTALL) wt440h2fhem.sh $(INSTALLDIR)
	$(INSTALL) -s wt440hrx $(INSTALLDIR)
	