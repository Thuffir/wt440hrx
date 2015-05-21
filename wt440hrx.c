/***********************************************************************************************************************
 *
 * WT440H Receiver / Decoder for Raspberry Pi
 *
 * (C) 2015 Gergely Budai
 *
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the COPYING file for more details.
 *
 **********************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <pigpio.h>

// GPIO PIN
#define INPUT_PIN                    5
// Bit length in uS
#define BIT_LENGTH                2000
// +- Bit length tolerance in uS
#define BIT_LENGTH_TOLERANCE       200

// Calculated Thresholds for zeros and ones
#define BIT_LENGTH_THRES_LOW      (BIT_LENGTH - BIT_LENGTH_TOLERANCE)
#define BIT_LENGTH_THRES_HIGH     (BIT_LENGTH + BIT_LENGTH_TOLERANCE)
#define HALFBIT_LENGTH_THRES_LOW  (BIT_LENGTH_THRES_LOW  / 2)
#define HALFBIT_LENGTH_THRES_HIGH (BIT_LENGTH_THRES_HIGH / 2)

// Pipe for communication between main thread and alert thread.
int pipefd[2];

// Bit Value with Timestamp
typedef struct {
  uint8_t bit;
  uint32_t timeStamp;
} BitType;

// Decoded WT440H Message
typedef struct {
  uint8_t houseCode;
  uint8_t channel;
  uint8_t status;
  uint8_t humidity;
  uint8_t tempInteger;
  uint8_t tempFraction;
  uint8_t sequneceNr;
} WT440hDataType;

/***********************************************************************************************************************
 * Send bit to main Thread
 **********************************************************************************************************************/
void SendBit(uint8_t bit, uint32_t timeStamp)
{
  // Recognised Bit
  BitType bitInfo;

  // Fill bit infos
  bitInfo.bit = bit;
  bitInfo.timeStamp = timeStamp;
  // Send via pipe
  if(write(pipefd[1], &bitInfo, sizeof(bitInfo)) != sizeof(bitInfo)) {
    perror("write()");
    exit(EXIT_FAILURE);
  }
}

/***********************************************************************************************************************
 *
 * GPIO Level Change Alert Handler
 *
 * Biphase Mark Decoder
 *
 **********************************************************************************************************************/
void RxAlert(int gpio, int level, uint32_t timeStamp)
{
  // Internal State for bit recognition
  static enum {
    Init,
    BitStartReceived,
    HalfBitReceived
  } state = Init;
  // Last timestamp for bit length calculation
  static uint32_t lastTimeStamp = 0;
  // Bit Length
  uint32_t bitLength;

  // Filter out non-interesting GPIOs (we should not get any)
  if(gpio != INPUT_PIN) {
    return;
  }

  // Calculate bit length
  bitLength = timeStamp - lastTimeStamp;
  lastTimeStamp = timeStamp;

  // Bit recognition state machine
  switch(state) {
    // Init State, no start Timestamp yet
    case Init: {
      state = BitStartReceived;
    }
    break;

    // Start edge of a bit has been received
    case BitStartReceived: {
      // Check bit length
      if((bitLength >= BIT_LENGTH_THRES_LOW) && (bitLength <= BIT_LENGTH_THRES_HIGH)) {
        // Full bit length, Zero received
        SendBit(0, timeStamp);
      }
      else if((bitLength >= HALFBIT_LENGTH_THRES_LOW) && (bitLength <= HALFBIT_LENGTH_THRES_HIGH)) {
        // Half bit length, first half of a One received
        state = HalfBitReceived;
      }
    }
    break;

    // First half of a One received
    case HalfBitReceived: {
      // Check bit length
      if((bitLength >= HALFBIT_LENGTH_THRES_LOW) && (bitLength <= HALFBIT_LENGTH_THRES_HIGH)) {
        // Second half of a One received
        SendBit(1, timeStamp);
      }
      state = BitStartReceived;
    }
    break;

    // Invalid state (should not happen)
    default: {
      perror("Invalid state");
      exit(EXIT_FAILURE);
    }
    break;
  }
}

/***********************************************************************************************************************
 * Init functions
 **********************************************************************************************************************/
void Init(void)
{
  // Create Pipe
  if(pipe(pipefd) == -1) {
    perror("pipe()");
    exit(EXIT_FAILURE);
  }

  // Set smaple rate
  if(gpioCfgClock(10, PI_CLOCK_PCM, 0)) {
    perror("gpioCfgClock()");
    exit(EXIT_FAILURE);
  }

  // Initialise GPIO library
  if(gpioInitialise() < 0)
  {
    perror("gpioInitialise()");
    exit(EXIT_FAILURE);
  }

  // Set edge change alert function
  if(gpioSetAlertFunc(INPUT_PIN, RxAlert)) {
    perror("gpioSetAlertFunc()");
    exit(EXIT_FAILURE);
  }
}

/***********************************************************************************************************************
 * Decode received bits into a WT440H Message
 **********************************************************************************************************************/
WT440hDataType RxData(void)
{
  // Preamble bits
  static const uint8_t preamble[] = {1, 1, 0, 0};
  // Received bit info
  BitType bitInfo;
  // Decoded WT440H data
  WT440hDataType data = { 0 };
  // Previous bit timestamp and bit length
  uint32_t prevTimeStamp = 0, bitLength;
  // Bit number counter
  uint8_t bitNr;
  // Checksum
  uint8_t checksum = 0;

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
        // printf("Wrong bitlength %u at bit %u\n", bitLength, bitNr);
        bitNr = 0;
        checksum = 0;
        memset(&data, 0, sizeof(data));
        continue;
      }
    }

    // Preamble [0 .. 3]
    if(bitNr <= 3) {
      if(bitInfo.bit != preamble[bitNr]) {
        // printf("Wrong preamble %u at bit %u\n", bitInfo.bit, bitNr);
        bitNr = 0;
        checksum = 0;
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

    // Update checksum
    checksum ^= bitInfo.bit << (bitNr & 1);
    // and check checksum if appropriate
    if(bitNr == 35) {
      if(checksum != 0) {
        // printf("Checksum error\n");
        bitNr = 0;
        checksum = 0;
        memset(&data, 0, sizeof(data));
        continue;
      }
    }

    // Increment bit pointer
    bitNr++;
  }

  return data;
}

/***********************************************************************************************************************
 * Main
 **********************************************************************************************************************/
int main(void)
{
  // Decoded WT440H data
  WT440hDataType data;

  // Do init stuff
  Init();

  // Receive and decode messages
  while(1) {
    data = RxData();
    printf("%u %u %u %u %.1f %u\n", data.houseCode, data.channel + 1, data.status, data.humidity,
      ((double)data.tempInteger - (double)50) + ((double)data.tempFraction / (double)16), data.sequneceNr);
    fflush(stdout);
  }

  return 0;
}
