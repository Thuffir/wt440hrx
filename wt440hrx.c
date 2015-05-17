#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <wiringPi.h>
#include "circular.h"

#define INPUT_PIN                    5
#define BIT_LENGTH                2000
#define BIT_LENGTH_TOLERANCE       200

#define BIT_LENGTH_THRES_LOW      (BIT_LENGTH - BIT_LENGTH_TOLERANCE)
#define BIT_LENGTH_THRES_HIGH     (BIT_LENGTH + BIT_LENGTH_TOLERANCE)
#define HALFBIT_LENGTH_THRES_LOW  (BIT_LENGTH_THRES_LOW  / 2)
#define HALFBIT_LENGTH_THRES_HIGH (BIT_LENGTH_THRES_HIGH / 2)

#define BIT_BUFFER_SIZE          4096
unsigned char bitBuffer[BIT_BUFFER_SIZE];
CIRCULAR_DATA circBits;

void RxInterrupt(void)
{
  static uint32_t lastEdge = 0;
  static enum {
    Init,
    BitStartReceived,
    HalfBitReceived
  } state = Init;

  uint32_t currEdge = micros();
  uint32_t bitLength = currEdge - lastEdge;
  lastEdge = currEdge;

  switch(state) {
    case Init: {
      state = BitStartReceived;
    }
    break;

    case BitStartReceived: {
      if((bitLength >= BIT_LENGTH_THRES_LOW) && (bitLength <= BIT_LENGTH_THRES_HIGH)) {
        // Zero received
        CircularPutByte(&circBits, 0);
      }
      else if((bitLength >= HALFBIT_LENGTH_THRES_LOW) && (bitLength <= HALFBIT_LENGTH_THRES_HIGH)) {
        // First half of a One received
        state = HalfBitReceived;
      }
    }
    break;

    case HalfBitReceived: {
      if((bitLength >= HALFBIT_LENGTH_THRES_LOW) && (bitLength <= HALFBIT_LENGTH_THRES_HIGH)) {
        // Second half of a One received
        CircularPutByte(&circBits, 1);
      }
      state = BitStartReceived;
    }
    break;

    default: {
      state = Init;
    }
    break;
  }
}

int main(void)
{
  CircularInitialize(&circBits, bitBuffer, BIT_BUFFER_SIZE);

  if(wiringPiSetupGpio()) {
    printf("wiringPiSetupGpio() failed!\n");
    return 1;
  }

  if(wiringPiISR(INPUT_PIN, INT_EDGE_BOTH, &RxInterrupt))
  {
    printf("wiringPiISR() failed!\n");
    return 1;
  }

  if(piHiPri(99)) {
    printf("piHiPri() failed!\n");
    return 1;
  }

  while(1) {
    int bit;
    while((bit = CircularGetByte(&circBits)) != CIRCULAR_EOB) {
      printf("%d\n", bit);
    }
    sleep(1);
  }

  return 0;
}
