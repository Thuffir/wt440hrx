#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pigpio.h>

#define INPUT_PIN                    5
#define BIT_LENGTH                2000
#define BIT_LENGTH_TOLERANCE       200

#define BIT_LENGTH_THRES_LOW      (BIT_LENGTH - BIT_LENGTH_TOLERANCE)
#define BIT_LENGTH_THRES_HIGH     (BIT_LENGTH + BIT_LENGTH_TOLERANCE)
#define HALFBIT_LENGTH_THRES_LOW  (BIT_LENGTH_THRES_LOW  / 2)
#define HALFBIT_LENGTH_THRES_HIGH (BIT_LENGTH_THRES_HIGH / 2)

int pipefd[2];

typedef struct {
  uint8_t bit;
  uint32_t timeStamp;
} BitType;

void RxAlert(int gpio, int level, uint32_t timeStamp)
{
  static enum {
    Init,
    BitStartReceived,
    HalfBitReceived
  } state = Init;
  static uint32_t lastTimeStamp = 0;
  uint32_t bitLength;
  BitType bitInfo;

  if(gpio != INPUT_PIN) {
    return;
  }

  bitLength = timeStamp - lastTimeStamp;

  switch(state) {
    case Init: {
      state = BitStartReceived;
    }
    break;

    case BitStartReceived: {
      if((bitLength >= BIT_LENGTH_THRES_LOW) && (bitLength <= BIT_LENGTH_THRES_HIGH)) {
        // Zero received
        bitInfo.bit = 0;
        bitInfo.timeStamp = timeStamp;
        if(write(pipefd[1], &bitInfo, sizeof(bitInfo)) != sizeof(bitInfo)) {
          perror("write()");
          exit(EXIT_FAILURE);
        }
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
        bitInfo.bit = 1;
        bitInfo.timeStamp = timeStamp;
        if(write(pipefd[1], &bitInfo, sizeof(bitInfo)) != sizeof(bitInfo)) {
          perror("write()");
          exit(EXIT_FAILURE);
        }
      }
      state = BitStartReceived;
    }
    break;

    default: {
      state = Init;
    }
    break;
  }

  lastTimeStamp = timeStamp;
}

void Init(void)
{
  if(pipe(pipefd) == -1) {
    perror("pipe()");
    exit(EXIT_FAILURE);
  }

  if(gpioCfgClock(10, PI_CLOCK_PCM, 0)) {
    perror("gpioCfgClock()");
    exit(EXIT_FAILURE);
  }

  if(gpioInitialise() < 0)
  {
    perror("gpioInitialise()");
    exit(EXIT_FAILURE);
  }

  if(gpioSetAlertFunc(INPUT_PIN, RxAlert)) {
    perror("gpioSetAlertFunc()");
    exit(EXIT_FAILURE);
  }
}

void RxData(void)
{
  BitType bitInfo;

  while(1) {
    if(read(pipefd[0], &bitInfo, sizeof(bitInfo)) != sizeof(bitInfo)) {
      printf("read()");
      exit(EXIT_FAILURE);
    }
    printf("%u (%u)\n", bitInfo.bit, bitInfo.timeStamp);
  }
}

int main(void)
{

  Init();
  RxData();

  return 0;
}
