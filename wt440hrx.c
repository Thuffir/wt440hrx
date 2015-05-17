#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <wiringPi.h>

#define INPUT_PIN                    5
#define BIT_LENGTH                2000
#define BIT_LENGTH_TOLERANCE       200

#define BIT_LENGTH_THRES_LOW      (BIT_LENGTH - BIT_LENGTH_TOLERANCE)
#define BIT_LENGTH_THRES_HIGH     (BIT_LENGTH + BIT_LENGTH_TOLERANCE)
#define HALFBIT_LENGTH_THRES_LOW  (BIT_LENGTH_THRES_LOW  / 2)
#define HALFBIT_LENGTH_THRES_HIGH (BIT_LENGTH_THRES_HIGH / 2)

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

  // printf("%d -> ", state);
  
  switch(state) {
    case Init: {
      state = BitStartReceived;
    }
    break;

    case BitStartReceived: {
      if((bitLength >= BIT_LENGTH_THRES_LOW) && (bitLength <= BIT_LENGTH_THRES_HIGH)) {
        // Zero received
        printf("0\n");
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
        printf("1\n");
      }
      state = BitStartReceived;
    }
    break;

    default: {
      state = Init;
    }
    break;
  }
  // printf("%d\n", state);
}

int main(void)
{
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
    sleep(1);
  }

  return 0;
}
