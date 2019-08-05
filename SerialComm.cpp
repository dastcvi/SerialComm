/*
 * SerialComm.cpp
 * Author:  Alex St. Clair
 * Created: August 2019
 *  
 * This file implements an Arduino library (C++ class) that implements a simple, robust
 * serial (UART) protocol for inter-Arduino messaging.
 * 
 * This class doesn't define specific messages, so any project using the protocol must
 * implement message definitions on top of this class.
 */

#include "SerialComm.h"

SerialComm::SerialComm(Stream * rxstream)
{
    rx_stream = rxstream;
}

// -------------------------- RX --------------------------

SerialMessage_t SerialComm::RX()
{
    ResetRX();

    if (!rx_stream->available()) return NO_MESSAGE;

    uint32_t timeout = millis() + READ_TIMEOUT;
    char rx_char = '\0';
    
    while (timeout > millis() && -1 != (rx_char = rx_stream->read())) {
        switch (rx_char) {
        case ASCII_DELIMITER:
            if (Read_ASCII(timeout)) {
                return ASCII_MESSAGE;
            } else {
                return NO_MESSAGE;
            }
        case ACK_DELIMITER:
            if (Read_Ack(timeout)) {
                return ACK_MESSAGE;
            } else {
                return NO_MESSAGE;
            }
        case BIN_DELIMITER:
            if (Read_Bin(timeout)) {
                return BIN_MESSAGE;
            } else {
                return NO_MESSAGE;
            }
        default:
            break;
        }
    }

    return NO_MESSAGE;
}

void SerialComm::ResetRX()
{
    ascii_rx.msg_id = 0;
    ascii_rx.num_params = 0;
    ascii_rx.buffer_index = 0;
    ascii_rx.buffer[0] = '\0';
}

void SerialComm::ResetTX()
{
    ascii_tx.msg_id = 0;
    ascii_tx.num_params = 0;
    ascii_tx.buffer_index = 0;
    ascii_tx.buffer[0] = '\0';
}

bool SerialComm::Read_ASCII(uint32_t timeout)
{
    char id_buffer[4] = {0}; // uint8 up to 3 chars long
    char rx_char = '\0';
    unsigned int temp = 0;

    // read the message id
    while (timeout > millis() && temp < 3) {
        // check for delimiters
        rx_char = rx_stream->peek();
        if (rx_char == ',' || rx_char == ';') break;

        // add to the id buffer
        id_buffer[temp++] = rx_stream->read();
    }

    // if the next char isn't a delimiter, there's been an error
    rx_char = rx_stream->peek();
    if (rx_char != ',' && rx_char != ';') return false;

    // convert the message id
    if (1 != sscanf(id_buffer, "%u", &temp)) return false;
    if (temp > 255) return false;
    ascii_rx.msg_id = (uint8_t) temp;

    // read the parameters into the buffer
    while (timeout > millis() && -1 != (rx_char = rx_stream->read())) {
        // check for special characters
        if (';' == rx_char) {
            ascii_rx.buffer[ascii_rx.buffer_index] = '\0'; // null terminate
            ascii_rx.buffer_index = 0; // reset index to zero
            return true;
        }
        if (',' == rx_char) ascii_rx.num_params++;

        // add character to the buffer
        ascii_rx.buffer[ascii_rx.buffer_index++] = rx_char;
    }

    return false;
}

bool SerialComm::Read_Ack(uint32_t timeout)
{
    char id_buffer[4] = {0}; // uint8 up to 3 chars long
    char rx_char = '\0';
    unsigned int temp = 0;

    // read the message id
    while (timeout > millis() && temp < 3) {
        // check for delimiters
        rx_char = rx_stream->peek();
        if (rx_char == ',') break;

        // add to the id buffer
        id_buffer[temp++] = rx_stream->read();
    }

    // if the next char isn't a comma, there's been an error
    rx_char = rx_stream->read();
    if (rx_char != ',') return false;

    // convert the message id
    if (1 != sscanf(id_buffer, "%u", &temp)) return false;
    if (temp > 255) return false;
    ack_id = (uint8_t) temp;

    // read the ack value
    rx_char = rx_stream->read();
    if (';' != rx_stream->peek()) return false; // verify trailing ';' first
    if ('0' == rx_char) {
        ack_value = false;
    } else if ('1' == rx_char) {
        ack_value = true;
    } else {
        return false;
    }

    rx_stream->read(); // read trailing ';'

    return true;
}

bool SerialComm::Read_Bin(uint32_t timeout)
{
    return false;
}

// -------------------------- TX --------------------------

void SerialComm::TX_ASCII(uint8_t msg_id)
{
    rx_stream->print(ASCII_DELIMITER);
    rx_stream->print(msg_id);
    if (ascii_tx.buffer_index > 0) { // todo: ensure always updated
        rx_stream->print(ascii_tx.buffer);
    }
    rx_stream->println(';');
    ResetTX();
}

void SerialComm::TX_Ack(uint8_t msg_id, bool ack_val)
{
    rx_stream->print(ACK_DELIMITER);
    rx_stream->print(msg_id);
    rx_stream->print(',');
    ack_val ? rx_stream->print('1') : rx_stream->print('0');
    rx_stream->println(';');
}

void SerialComm::TX_Bin()
{
    rx_stream->print(BIN_DELIMITER);
    rx_stream->println("0,0;;"); // empty message
    // todo: finish implementing binary
}

// -------------------- Buffer Parsing --------------------

bool SerialComm::Get_uint8(uint8_t * ret_val)
{
    char int_buffer[4] = {0};
    uint8_t max_index = ascii_rx.buffer_index + 3; // uint8_t means 3 chars max
    unsigned int temp = 0;

    if (',' != ascii_rx.buffer[ascii_rx.buffer_index++]) return false; // always a leading comma

    while (ascii_rx.buffer_index <= max_index) {
        if (',' == ascii_rx.buffer[ascii_rx.buffer_index] || '\0' == ascii_rx.buffer[ascii_rx.buffer_index]) {
            break;
        }

        int_buffer[temp++] = ascii_rx.buffer[ascii_rx.buffer_index++];
    }

    // ensure next char is ',' or '\0'
    if (',' != ascii_rx.buffer[ascii_rx.buffer_index] && '\0' != ascii_rx.buffer[ascii_rx.buffer_index]) return false;

    // convert the message id
    if (1 != sscanf(int_buffer, "%u", &temp)) return false;
    if (temp > 255) return false;
    *ret_val = (uint8_t) temp;

    return true;
}

bool SerialComm::Get_uint16(uint16_t * ret_val)
{
    char int_buffer[6] = {0};
    uint8_t max_index = ascii_rx.buffer_index + 5; // uint16_t means 5 chars max
    unsigned int temp = 0;

    if (',' != ascii_rx.buffer[ascii_rx.buffer_index++]) return false; // always a leading comma

    while (ascii_rx.buffer_index <= max_index) {
        if (',' == ascii_rx.buffer[ascii_rx.buffer_index] || '\0' == ascii_rx.buffer[ascii_rx.buffer_index]) {
            break;
        }

        int_buffer[temp++] = ascii_rx.buffer[ascii_rx.buffer_index++];
    }

    // ensure next char is ',' or '\0'
    if (',' != ascii_rx.buffer[ascii_rx.buffer_index] && '\0' != ascii_rx.buffer[ascii_rx.buffer_index]) return false;

    // convert the message id
    if (1 != sscanf(int_buffer, "%u", &temp)) return false;
    if (temp > 65535) return false;
    *ret_val = (uint16_t) temp;

    return true;
}

bool SerialComm::Get_uint32(uint32_t * ret_val)
{
    char int_buffer[11] = {0};
    uint8_t max_index = ascii_rx.buffer_index + 10; // uint32_t means 10 chars max
    unsigned int temp = 0;

    if (',' != ascii_rx.buffer[ascii_rx.buffer_index++]) return false; // always a leading comma

    while (ascii_rx.buffer_index <= max_index) {
        if (',' == ascii_rx.buffer[ascii_rx.buffer_index] || '\0' == ascii_rx.buffer[ascii_rx.buffer_index]) {
            break;
        }

        int_buffer[temp++] = ascii_rx.buffer[ascii_rx.buffer_index++];
    }

    // ensure next char is ',' or '\0'
    if (',' != ascii_rx.buffer[ascii_rx.buffer_index] && '\0' != ascii_rx.buffer[ascii_rx.buffer_index]) return false;

    // convert the message id
    if (1 != sscanf(int_buffer, "%u", &temp)) return false;
    *ret_val = temp;

    return true;
}

bool SerialComm::Get_float(float * ret_val)
{
    char int_buffer[16] = {0};
    uint8_t max_index = ascii_rx.buffer_index + 15; // 15 chars max
    unsigned int temp = 0;
    float temp_float = 0.0f;

    if (',' != ascii_rx.buffer[ascii_rx.buffer_index++]) return false; // always a leading comma

    while (ascii_rx.buffer_index <= max_index) {
        if (',' == ascii_rx.buffer[ascii_rx.buffer_index] || '\0' == ascii_rx.buffer[ascii_rx.buffer_index]) {
            break;
        }

        int_buffer[temp++] = ascii_rx.buffer[ascii_rx.buffer_index++];
    }

    // ensure next char is ',' or '\0'
    if (',' != ascii_rx.buffer[ascii_rx.buffer_index] && '\0' != ascii_rx.buffer[ascii_rx.buffer_index]) return false;

    // convert the message id
    if (1 != sscanf(int_buffer, "%f", &temp_float)) return false;
    *ret_val = temp_float;

    return true;
}

bool SerialComm::Get_string(char * buffer, uint8_t length)
{
    uint8_t buffer_index = 0;

    // leave room for null termination
    uint16_t max_index = ascii_rx.buffer_index + length - 1;
    if (max_index > 127) max_index = 127;

    if (',' != ascii_rx.buffer[ascii_rx.buffer_index++]) return false; // always a leading comma

    while (ascii_rx.buffer_index <= max_index) {
        if (',' == ascii_rx.buffer[ascii_rx.buffer_index] || '\0' == ascii_rx.buffer[ascii_rx.buffer_index]) {
            break;
        }

        buffer[buffer_index++] = ascii_rx.buffer[ascii_rx.buffer_index++];
    }

    // ensure next char is ',' or '\0'
    if (',' != ascii_rx.buffer[ascii_rx.buffer_index] && '\0' != ascii_rx.buffer[ascii_rx.buffer_index]) return false;

    // null terminate the buffer
    buffer[buffer_index] = '\0';

    return true;
}

// -------------------- Buffer Addition -------------------

bool SerialComm::Add_uint8(uint8_t val)
{
    return Add_uint32((uint32_t) val);
}

bool SerialComm::Add_uint16(uint16_t val)
{
    return Add_uint32((uint32_t) val);
}

bool SerialComm::Add_uint32(uint32_t val)
{
    uint8_t buffer_remaining = 128 - ascii_tx.buffer_index;
    int num_written = 0;

    // snprintf will return the number of chars it could write, but won't write more than buffer_remaining
    // note leading comma!
    num_written = snprintf(ascii_tx.buffer + ascii_tx.buffer_index, buffer_remaining, ",%u", (unsigned int) val);
    
    // make sure the write was valid and not too large
    if (num_written < 1 || num_written >= buffer_remaining) {
        ResetTX();
        return false;
    }

    ascii_tx.buffer_index += num_written;

    return true;
}

bool SerialComm::Add_float(float val)
{
    uint8_t buffer_remaining = 128 - ascii_tx.buffer_index;
    int num_written = 0;

    // snprintf will return the number of chars it could write, but won't write more than buffer_remaining
    // note leading comma!
    num_written = snprintf(ascii_tx.buffer + ascii_tx.buffer_index, buffer_remaining, ",%f", val);
    
    // make sure the write was valid and not too large
    if (num_written < 1 || num_written >= buffer_remaining) {
        ResetTX();
        return false;
    }

    ascii_tx.buffer_index += num_written;

    return true;
}

bool SerialComm::Add_string(const char * buffer)
{
    uint8_t buffer_remaining = 128 - ascii_tx.buffer_index;
    int num_written = 0;

    // snprintf will return the number of chars it could write, but won't write more than buffer_remaining
    // note leading comma!
    num_written = snprintf(ascii_tx.buffer + ascii_tx.buffer_index, buffer_remaining, ",%s", buffer);
    
    // make sure the write was valid and not too large
    if (num_written < 1 || num_written >= buffer_remaining) {
        ResetTX();
        return false;
    }

    ascii_tx.buffer_index += num_written;

    return true;
}
