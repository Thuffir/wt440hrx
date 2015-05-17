#include <stdio.h>
#include <stdlib.h>
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

int pipefd[2];
typedef struct {
  uint8_t bit;
  uint32_t bitLength;
} BitType;

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

  BitType bitInfo;

  switch(state) {
    case Init: {
      state = BitStartReceived;
    }
    break;

    case BitStartReceived: {
      if((bitLength >= BIT_LENGTH_THRES_LOW) && (bitLength <= BIT_LENGTH_THRES_HIGH)) {
        // Zero received
	bitInfo.bit = 0;
	bitInfo.bitLength = bitLength;
	if(write(pipefd[1], &bitInfo, sizeof(bitInfo)) != sizeof(bitInfo)) {
	  perror("write() failed!");
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
	bitInfo.bitLength = bitLength;
	if(write(pipefd[1], &bitInfo, sizeof(bitInfo)) != sizeof(bitInfo)) {
	  perror("write() failed!");
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
}

int main(void)
{
  BitType bitInfo;

  if(pipe(pipefd) == -1) {
    perror("pipe() failed!");
    exit(EXIT_FAILURE);
  }

  if(wiringPiSetupGpio()) {
    perror("wiringPiSetupGpio() failed!");
    exit(EXIT_FAILURE);
  }

  if(wiringPiISR(INPUT_PIN, INT_EDGE_BOTH, &RxInterrupt))
  {
    perror("wiringPiISR() failed!");
    exit(EXIT_FAILURE);
  }

  if(piHiPri(99)) {
    printf("piHiPri() failed!\n");
    return 1;
  }

  while(1) {
    if(read(pipefd[0], &bitInfo, sizeof(bitInfo)) != sizeof(bitInfo)) {
      printf("read() failed!\n");
      return 1;
    }
    printf("%u (%u)\n", bitInfo.bit, bitInfo.bitLength);
  }

  return 0;
}
