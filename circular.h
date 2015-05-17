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
  unsigned char *buffer;
  /// Size of the memory buffer
  unsigned int size;
  /// The beginning position of the buffer
  unsigned int start;
  /// The number of elements actually in the buffer
  unsigned int end;
} CIRCULAR_DATA;

/// Definition for end of buffer value
/// @ingroup circular_m
#define CIRCULAR_EOB    (-1)

/***********************************************************************************************************************
 * Function prototypes
 **********************************************************************************************************************/
void CircularInitialize(CIRCULAR_DATA *, unsigned char *, unsigned int);
void CircularPutByte(CIRCULAR_DATA *, unsigned char);
int CircularGetByte(CIRCULAR_DATA *);
unsigned int CircularGetNumberOfBytes(CIRCULAR_DATA *);

#endif // CIRCULAR_H
