/*
 *  Serialize.cpp
 *  File implementing functions for serializing data types into buffers for serial transfer
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

#include "Serialize.h"

#define NULL nullptr

// Default to big endian
Endianness_t endianness = SERIALIZE_BIG_ENDIAN;

// --- Safely add data to the buffer, return success ---

bool BufferAddUInt8(uint8_t data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index)
{
    // check for NULL pointers
    if (NULL == buffer || NULL == curr_index) return false;

    // ensure we don't overrun the buffer
    if (*curr_index + sizeof(uint8_t) > buffer_size) return false;

    buffer[(*curr_index)++] = data;

    return true;
}

bool BufferAddUInt16(uint16_t data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index)
{
    // check for NULL pointers
    if (NULL == buffer || NULL == curr_index) return false;

    // ensure we don't overrun the buffer
    if (*curr_index + sizeof(uint16_t) > buffer_size) return false;

    if (SERIALIZE_BIG_ENDIAN == endianness) {
        buffer[(*curr_index)++] = (data >> 8) & 0xFF;
        buffer[(*curr_index)++] = data & 0xFF;
    } else {
        buffer[(*curr_index)++] = data & 0xFF;
        buffer[(*curr_index)++] = data >> 8;
    }

    return true;
}

bool BufferAddUInt32(uint32_t data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index)
{
    // check for NULL pointers
    if (NULL == buffer || NULL == curr_index) return false;

    // ensure we don't overrun the buffer
    if (*curr_index + sizeof(uint32_t) > buffer_size) return false;

    if (SERIALIZE_BIG_ENDIAN == endianness) {
        buffer[(*curr_index)++] = (data >> 24) & 0xFF;
        buffer[(*curr_index)++] = (data >> 16) & 0xFF;
        buffer[(*curr_index)++] = (data >> 8) & 0xFF;
        buffer[(*curr_index)++] = data & 0xFF;
    } else {
        buffer[(*curr_index)++] = data & 0xFF;
        buffer[(*curr_index)++] = (data >> 8) & 0xFF;
        buffer[(*curr_index)++] = (data >> 16) & 0xFF;
        buffer[(*curr_index)++] = (data >> 24) & 0xFF;
    }

    return true;
}

bool BufferAddInt8(int8_t data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index)
{
    return BufferAddUInt8((uint8_t) data, buffer, buffer_size, curr_index);
}

bool BufferAddInt16(int16_t data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index)
{
    return BufferAddUInt16((int16_t) data, buffer, buffer_size, curr_index);
}

bool BufferAddInt32(int32_t data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index)
{
    return BufferAddUInt32((int32_t) data, buffer, buffer_size, curr_index);
}

bool BufferAddFloat(float data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index)
{
    uint32_t * temp = (uint32_t *) &data;
    return BufferAddUInt32(*temp, buffer, buffer_size, curr_index);
}


// --- Safely get data from a buffer, return success ---

bool BufferGetUInt8(uint8_t * data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index)
{
    // check for NULL pointers
    if (NULL == data || NULL == buffer || NULL == curr_index) return false;

    // ensure we don't overrun the buffer
    if (*curr_index + sizeof(uint8_t) > buffer_size) return false;

    *data = buffer[(*curr_index)++];

    return true;
}

bool BufferGetUInt16(uint16_t * data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index)
{
    // check for NULL pointers
    if (NULL == data || NULL == buffer || NULL == curr_index) return false;

    // ensure we don't overrun the buffer
    if (*curr_index + sizeof(uint16_t) > buffer_size) return false;

    *data = 0;

    if (SERIALIZE_BIG_ENDIAN == endianness) {
        *data |= ((uint16_t) buffer[(*curr_index)++]) << 8;
        *data |= ((uint16_t) buffer[(*curr_index)++]);
    } else {
        *data |= ((uint16_t) buffer[(*curr_index)++]);
        *data |= ((uint16_t) buffer[(*curr_index)++]) << 8;
    }

    return true;
}

bool BufferGetUInt32(uint32_t * data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index)
{
    // check for NULL pointers
    if (NULL == data || NULL == buffer || NULL == curr_index) return false;

    // ensure we don't overrun the buffer
    if (*curr_index + sizeof(uint32_t) > buffer_size) return false;

    *data = 0;

    if (SERIALIZE_BIG_ENDIAN == endianness) {
        *data |=  ((uint32_t) buffer[(*curr_index)++]) << 24;
        *data |= ((uint32_t) buffer[(*curr_index)++]) << 16;
        *data |= ((uint32_t) buffer[(*curr_index)++]) << 8;
        *data |= ((uint32_t) buffer[(*curr_index)++]);
    } else {
        *data |=  ((uint32_t) buffer[(*curr_index)++]);
        *data |= ((uint32_t) buffer[(*curr_index)++]) << 8;
        *data |= ((uint32_t) buffer[(*curr_index)++]) << 16;
        *data |= ((uint32_t) buffer[(*curr_index)++]) << 24;
    }

    return true;
}

bool BufferGetInt8(int8_t * data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index)
{
    uint8_t placeholder = 0;

    if (!BufferGetUInt8(&placeholder, buffer, buffer_size, curr_index)) return false;

    *data = (int8_t) placeholder;

    return true;
}

bool BufferGetInt16(int16_t * data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index)
{
    uint16_t placeholder = 0;

    if (!BufferGetUInt16(&placeholder, buffer, buffer_size, curr_index)) return false;

    *data = (int16_t) placeholder;

    return true;
}

bool BufferGetInt32(int32_t * data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index)
{
    uint32_t placeholder = 0;

    if (!BufferGetUInt32(&placeholder, buffer, buffer_size, curr_index)) return false;

    *data = (int32_t) placeholder;

    return true;
}

bool BufferGetFloat(float * data, uint8_t * buffer, uint16_t buffer_size, uint16_t * curr_index)
{
    uint32_t placeholder = 0;
    float * temp;

    if (!BufferGetUInt32(&placeholder, buffer, buffer_size, curr_index)) return false;

    // cast the integer into a temporary float pointer variable
    temp = (float *) &placeholder;
    *data = *temp;

    return true;
}
