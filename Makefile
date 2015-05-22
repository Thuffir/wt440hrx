TARGET = wt440hrx
LIBS = -lpigpiod_if -lpthread -lrt
CC = gcc
CFLAGS = -O3 -Wall -fomit-frame-pointer
LFLAGS = -s

INSTALL = sudo install -m 755 -o fhem -g dialout
INSTALLDIR = /opt/fhem

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
