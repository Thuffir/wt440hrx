#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
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

typedef struct {
  uint8_t houseCode;
  uint8_t channel;
  uint8_t status;
  uint8_t humidity;
  uint8_t tempInteger;
  uint8_t tempFraction;
  uint8_t sequneceNr;
  uint8_t checksum;
} WT400hDataType;

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
  static const uint8_t preamble[] = {1, 1, 0, 0};
  BitType bitInfo;
  WT400hDataType data = { 0 };
  uint32_t prevTimeStamp = 0, bitLength;
  uint8_t bitNr;

  // Data is 36 bits long
  for(bitNr = 0; bitNr < 36;) {
    // Read Bit
    if(read(pipefd[0], &bitInfo, sizeof(bitInfo)) != sizeof(bitInfo)) {
      printf("read()");
      exit(EXIT_FAILURE);
    }

    // Get Bit Length
    bitLength = bitInfo.timeStamp - prevTimeStamp;
    prevTimeStamp = bitInfo.timeStamp;
    // Each bit must follow in a bit length sequence
    if(bitNr > 0) {
      // Check bit Length
      if((bitLength < BIT_LENGTH_THRES_LOW) || (bitLength > BIT_LENGTH_THRES_HIGH)) {
//        printf("Wrong bitlength %u at bit %u\n", bitLength, bitNr);
        bitNr = 0;
        memset(&data, 0, sizeof(data));
        continue;
      }
    }

    // Preamble [0 .. 3]
    if(bitNr <= 3) {
      if(bitInfo.bit != preamble[bitNr]) {
//        printf("Wrong preamble %u at bit %u\n", bitInfo.bit, bitNr);
        bitNr = 0;
        memset(&data, 0, sizeof(data));
        continue;
      }
    }
    // Housecode [4 .. 7]
    else if((bitNr >= 4) && (bitNr <= 7)) {
      data.houseCode = (data.houseCode << 1) | bitInfo.bit;
    }
    // Channel [8 .. 9]
    else if((bitNr >= 8) && (bitNr <= 9)) {
      data.channel = (data.channel << 1) | bitInfo.bit;
    }
    // Status [10 .. 11]
    else if((bitNr >= 10) && (bitNr <= 11)) {
      data.status = (data.status << 1) | bitInfo.bit;
    }
    // Humidity [12 .. 19]
    else if((bitNr >= 12) && (bitNr <= 19)) {
      data.humidity = (data.humidity << 1) | bitInfo.bit;
    }
    // Temperature (Integer part) [20 .. 27]
    else if((bitNr >= 20) && (bitNr <= 27)) {
      data.tempInteger = (data.tempInteger << 1) | bitInfo.bit;
    }
    // Temperature (Fractional part) [28 .. 31]
    else if((bitNr >= 28) && (bitNr <= 31)) {
      data.tempFraction = (data.tempFraction << 1) | bitInfo.bit;
    }
    // Message Sequence [32 .. 33]
    else if((bitNr >= 32) && (bitNr <= 33)) {
      data.sequneceNr = (data.sequneceNr << 1) | bitInfo.bit;
    }
    // Checksum [34 .. 35]
    else if((bitNr >= 34) && (bitNr <= 35)) {
      data.checksum = (data.checksum << 1) | bitInfo.bit;
    }

    // Increment bit pointer
    bitNr++;
  }

  printf("House code : %u\n" \
         "Channel    : %u\n" \
         "Status     : %u\n" \
         "Humidity   : %u\n" \
         "Temperature: %.1f\n" \
         "Sequnce Nr.: %u\n" \
         "Checksum   : %u\n\n", \
         data.houseCode,
         data.channel,
         data.status,
         data.humidity,
         ((double)data.tempInteger - (double)50) + ((double)data.tempFraction / (double)16),
         data.sequneceNr,
         data.checksum);
}

int main(void)
{

  Init();

  while(1) {
    RxData();
  }

  return 0;
}
