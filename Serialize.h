/*
 *  Serialize.h
 *  File defining functions for serializing data types into buffers for serial transfer
 *  Author: Alex St. Clair
 *  September 2019
 *
 *  The functions are passed the buffer pointer, size of the buffer, and a pointer to
 *  a variable tracking the current index in the buffer. Note that the functions will
 *  increment the index variable automatically and only if bytes on the buffer are
 *  actually used.
 *
 *  The maximum buffer size supported is 65531 (ie. UINT16_MAX - 4)
 */

#include <stdint.h>

enum Endianness_t : uint8_t {
    SERIALIZE_BIG_ENDIAN,
    SERIALIZE_LITTLE_ENDIAN
};

// allow the user to choose an endianness (must be a variable, not macro since used in many projects)

extern Endianness_t endianness;

// --- Safely add data to the buffer, return success ---

bool BufferAddUInt8(uint8_t data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index);
bool BufferAddUInt16(uint16_t data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index);
bool BufferAddUInt32(uint32_t data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index);

bool BufferAddInt8(int8_t data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index);
bool BufferAddInt16(int16_t data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index);
bool BufferAddInt32(int32_t data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index);

bool BufferAddFloat(float data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index);

// --- Safely get data from a buffer, return success ---

bool BufferGetUInt8(uint8_t * data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index);
bool BufferGetUInt16(uint16_t * data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index);
bool BufferGetUInt32(uint32_t * data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index);

bool BufferGetInt8(int8_t * data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index);
bool BufferGetInt16(int16_t * data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index);
bool BufferGetInt32(int32_t * data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index);

bool BufferGetFloat(float * data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index);