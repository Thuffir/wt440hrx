/**********************************************************************************************************************/
/** Circular buffer header file
 ***********************************************************************************************************************
 *
 * @file circular.h
 * @ingroup circular_m
 *
 **********************************************************************************************************************/

/**********************************************************************************************************************/
/** Circular buffers
 ***********************************************************************************************************************
 *
 * These routines implement a generic circular buffer. The buffers are realized with a start index and an end index.
 * The start index is always showing the oldest byte in the buffer, and the end index is showing one byte after the
 * newest byte (the next free space).
 *
 * @note
 * The functions are interrupt and thread safe.
 *
 * @attention
 * Buffer overruns are not checked!
 *
 * @defgroup circular_m Circular Buffer
 *
 **********************************************************************************************************************/

// Protection against circular inclusion
#ifndef CIRCULAR_H
#define CIRCULAR_H

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include <stdint.h>

/**********************************************************************************************************************/
/** Circular Buffer Data Structure
 ***********************************************************************************************************************
 *
 * This structure holds all the important state information of a circular buffer
 *
 * @attention
 * The structure members should be never manipulated directly!
 *
 * @ingroup circular_m
 *
 **********************************************************************************************************************/
typedef struct {
  /// Pointer to the memory buffer
  uint8_t *buffer;
  /// Size of the memory buffer
  uint16_t size;
  /// The beginning position of the buffer
  uint16_t start;
  /// The number of elements actually in the buffer
  uint16_t end;
} CIRCULAR_DATA;

/// Definition for end of buffer value
/// @ingroup circular_m
#define CIRCULAR_EOB    ((uint16_t)-1)

/***********************************************************************************************************************
 * Function prototypes
 **********************************************************************************************************************/
void CircularInitialize(CIRCULAR_DATA *, uint8_t *, uint16_t);
void CircularPutByte(CIRCULAR_DATA *, uint8_t );
uint16_t CircularGetByte(CIRCULAR_DATA *);
uint16_t CircularGetNumberOfBytes(CIRCULAR_DATA *);

#endif // CIRCULAR_H
