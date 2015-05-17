/**********************************************************************************************************************/
/** Circular buffer source file
 ***********************************************************************************************************************
 *
 * @file circular.c
 * @ingroup circular_m
 *
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "circular.h"

/**********************************************************************************************************************/
/** Initialize a given circular buffer
 ***********************************************************************************************************************
 *
 * This function initializes a given circular buffer structure.
 *
 * @note
 * This function can be also used to reinitialize the structure (reset the buffer). To do so, just pass the buffer and
 * size parameters from the structure to the function.
 *
 * @ingroup circular_m
 *
 **********************************************************************************************************************/
void CircularInitialize(
  /// Pointer to the data structure
  CIRCULAR_DATA *data,
  /// Pointer to memory for the circular buffer
  uint8_t *buffer,
  /// Size of the circular buffer
  uint16_t size)
{
  // Initialize the buffer pointer
  data->buffer = buffer;
  // Set the buffer size
  data->size = size;
  // Reset the buffer pointers
  data->start = 0;
  data->end = 0;
}

/**********************************************************************************************************************/
/** Put one byte into the circular buffer
 ***********************************************************************************************************************
 *
 * This function puts one byte into the circular buffer.
 *
 * @ingroup circular_m
 *
 **********************************************************************************************************************/
void CircularPutByte(
  /// Pointer to the data structure
  CIRCULAR_DATA *data,
  /// Byte to put into the buffer
  uint8_t byte)
{
  // Put the byte into the buffer
  data->buffer[data->end] = byte;

  // Increment the end index
  data->end = (data->end + 1) % data->size;
}

/**********************************************************************************************************************/
/** Get one byte from the circular buffer
 ***********************************************************************************************************************
 *
 * This function returns the next byte from the circular buffer. If no more bytes are available it returns
 * #CIRCULAR_EOB.
 *
 * @return
 * The next available byte in the buffer or #CIRCULAR_EOB if no more bytes are available.
 *
 * @ingroup circular_m
 *
 **********************************************************************************************************************/
uint16_t CircularGetByte(
  /// Pointer to the data structure
  CIRCULAR_DATA *data)
{
  // We will use this variable for the return value
  uint16_t byte;

  // Check if we actually have something in the buffer
  if(data->start != data->end) {
    // Get the next byte from the buffer
    byte = data->buffer[data->start];

    // Increment the start position index
    data->start = (data->start + 1) % data->size;
  }
  else {
    // We have nothing in the buffer, signal it with a special return value
    byte = CIRCULAR_EOB;
  }

  // Return the next byte or the EOB marker
  return byte;
}

/**********************************************************************************************************************/
/** Get the number of bytes in the circular buffer
 ***********************************************************************************************************************
 *
 * This function returns the number of bytes residing in the circular buffer.
 *
 * @return
 * The number of bytes in the buffer.
 *
 * @ingroup circular_m
 *
 **********************************************************************************************************************/
uint16_t CircularGetNumberOfBytes(
  /// Pointer to the data structure
  CIRCULAR_DATA *data)
{
  // Return the number of bytes in buffer
  return data->end - data->start;
}
