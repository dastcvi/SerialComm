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

// -------------------- Initialization --------------------

SerialComm::SerialComm(Stream * stream_in)
{
    serial_stream = stream_in;

    // explicity set the pointers to NULL
    binary_rx.bin_buffer = NULL;
    binary_tx.bin_buffer = NULL;
}

void SerialComm::UpdatePort(Stream * stream_in)
{
    serial_stream = stream_in;
}

void SerialComm::AssignBinaryRXBuffer(uint8_t * buffer, uint16_t size)
{
    binary_rx.bin_buffer = buffer;
    binary_rx.buffer_size = size;
}

void SerialComm::AssignBinaryTXBuffer(uint8_t * buffer, uint16_t size, uint16_t num_bytes)
{
    binary_tx.bin_buffer = buffer;
    binary_tx.buffer_size = size;
    binary_tx.bin_length = num_bytes;
}

// ----------------------- Helpers ------------------------

inline bool SerialComm::GetNextChar(char * new_char)
{
    int read_ret = serial_stream->read();
    if (read_ret == -1) return false;
    *new_char = (char) read_ret;
    UpdateChecksum((uint8_t) *new_char);
    return true;
}

bool SerialComm::ReadSpecificChar(uint32_t timeout, char specific_char)
{
    char new_char;

    // wait until there's a character available
    while (millis() < timeout && !serial_stream->available());

    // verify that we get the expected char
    if (!GetNextChar(&new_char) || specific_char != new_char) return false;

    return true;
}

// -------------------------- RX --------------------------

SerialMessage_t SerialComm::RX()
{
    ResetRX();

    if (!serial_stream->available()) return NO_MESSAGE;

    uint32_t timeout = millis() + READ_TIMEOUT;
    char rx_char = '\0';

    ResetChecksum();
    while (timeout > millis() && GetNextChar(&rx_char)) {
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
            timeout += 900; // some binary messages take up to a second
            if (Read_Bin(timeout)) {
                return BIN_MESSAGE;
            } else {
                return NO_MESSAGE;
            }
        case STRING_DELIMITER:
            if (Read_String(timeout)) {
                return STRING_MESSAGE;
            } else {
                return NO_MESSAGE;
            }
        default:
            ResetChecksum();
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
    int read_ret = -1;

    // read the message id
    while (timeout > millis() && temp < 3) {
        // check for delimiters
        read_ret = serial_stream->peek();
        if (-1 == read_ret) continue;
        rx_char = (char) read_ret;
        if (rx_char == ',' || rx_char == ';') break;

        // add to the id buffer
        if (GetNextChar(&rx_char)) id_buffer[temp++] = rx_char;
    }

    // if the next char isn't a delimiter, there's been an error
    rx_char = serial_stream->peek();
    if (rx_char != ',' && rx_char != ';') return false;

    // convert the message id
    if (1 != sscanf(id_buffer, "%u", &temp)) return false;
    if (temp > 255) return false;
    ascii_rx.msg_id = (uint8_t) temp;

    // read the parameters into the buffer
    while (timeout > millis()) {
        if (!GetNextChar(&rx_char)) continue;

        // check for special characters
        if (';' == rx_char) {
            ascii_rx.buffer[ascii_rx.buffer_index] = '\0'; // null terminate
            ascii_rx.buffer_index = 0; // reset index to zero

            ascii_rx.checksum_valid = ReadChecksum(timeout);

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
    int read_ret = -1;

    // read the message id
    while (timeout > millis() && temp < 3) {
        // check for delimiters
        read_ret = serial_stream->peek();
        if (-1 == read_ret) continue;
        rx_char = (char) read_ret;
        if (rx_char == ',') break;

        // add to the id buffer
        if (GetNextChar(&rx_char)) id_buffer[temp++] = rx_char;
    }

    // if the next char isn't a comma, there's been an error
    if (!ReadSpecificChar(timeout, ',')) return false;

    // convert the message id
    if (1 != sscanf(id_buffer, "%u", &temp)) return false;
    if (temp > 255) return false;
    ack_id = (uint8_t) temp;

    // read the ack value
    while (millis() < timeout && !GetNextChar(&rx_char));
    if ('0' == rx_char) {
        ack_value = false;
    } else if ('1' == rx_char) {
        ack_value = true;
    } else {
        return false;
    }

    // the message should end with a semi-colon before the checksum
    if (!ReadSpecificChar(timeout, ';')) return false;

    ack_checksum = ReadChecksum(timeout);

    return true;
}

bool SerialComm::Read_Bin(uint32_t timeout)
{
    char id_buffer[4] = {0}; // uint8 up to 3 chars long
    char length_buffer[6] = {0};
    char rx_char = '\0';
    unsigned int temp = 0;
    int read_ret = -1;

    // ensure rx message struct is reset
    binary_rx.bin_length = 0;
    binary_rx.bin_id = 0;

    // ensure the destination buffer is valid
    if (binary_rx.bin_buffer == NULL) return false;

    // read the binary id
    while (timeout > millis() && temp < 3) {
        // check for delimiters
        read_ret = serial_stream->peek();
        if (-1 == read_ret) continue;
        rx_char = (char) read_ret;
        if (rx_char == ',') break;

        // add to the id buffer
        if (GetNextChar(&rx_char)) id_buffer[temp++] = rx_char;
    }

    // if timed out, flush the buffer and return error
    if (timeout <= millis()) {
        serial_stream->flush();
        return false;
    }

    // if the next char isn't a comma, there's been an error
    if (!ReadSpecificChar(timeout, ',')) return false;

    // convert the binary id
    if (1 != sscanf(id_buffer, "%u", &temp)) return false;
    if (temp > 255) return false;
    binary_rx.bin_id = (uint8_t) temp;

    // read the binary length
    temp = 0;
    while (timeout > millis() && temp < 5) {
        // check for delimiters
        read_ret = serial_stream->peek();
        if (-1 == read_ret) continue;
        rx_char = (char) read_ret;
        if (rx_char == ';') break;

        // add to the length buffer
        if (GetNextChar(&rx_char)) length_buffer[temp++] = rx_char;
    }

    // if timed out, flush the buffer and return error
    if (timeout <= millis()) {
        serial_stream->flush();
        return false;
    }

    // if the next char isn't a semicolon, there's been an error
    if (!ReadSpecificChar(timeout, ';')) return false;

    // convert the binary length
    if (1 != sscanf(length_buffer, "%u", &temp)) return false;
    if (temp > 65535) return false;
    binary_rx.bin_length = (uint16_t) temp;

    // ensure we won't overflow the buffer
    if (binary_rx.bin_length > binary_rx.buffer_size) {
        serial_stream->flush();
        return false;
    }

    // read the binary section
    temp = 0;
    while (timeout > millis() && temp < binary_rx.bin_length) {
        if (GetNextChar(&rx_char)) binary_rx.bin_buffer[temp++] = (uint8_t) rx_char;
    }

    // if timed out, flush the buffer and return error
    if (timeout <= millis()) {
        serial_stream->flush();
        return false;
    }

    // the message should end with a semi-colon before the checksum
    if (!ReadSpecificChar(timeout, ';')) return false;

    binary_rx.checksum_valid = ReadChecksum(timeout);

    return true;
}

bool SerialComm::Read_String(uint32_t timeout)
{
    char id_buffer[4] = {0}; // uint8 up to 3 chars long
    char length_buffer[6] = {0};
    char rx_char = '\0';
    unsigned int temp = 0;
    int read_ret = -1;

    // ensure rx message struct is reset
    string_rx.str_length = 0;
    string_rx.str_id = 0;

    // read the string id
    while (timeout > millis() && temp < 3) {
        // check for delimiters
        read_ret = serial_stream->peek();
        if (-1 == read_ret) continue;
        rx_char = (char) read_ret;
        if (rx_char == ',') break;

        // add to the id buffer
        if (GetNextChar(&rx_char)) id_buffer[temp++] = rx_char;
    }

    // if timed out, flush the buffer and return error
    if (timeout <= millis()) {
        serial_stream->flush();
        return false;
    }

    // if the next char isn't a comma, there's been an error
    if (!ReadSpecificChar(timeout, ',')) return false;

    // convert the string id
    if (1 != sscanf(id_buffer, "%u", &temp)) return false;
    if (temp > 255) return false;
    string_rx.str_id = (uint8_t) temp;

    // read the string length
    temp = 0;
    while (timeout > millis() && temp < 5) {
        // check for delimiters
        read_ret = serial_stream->peek();
        if (-1 == read_ret) continue;
        rx_char = (char) read_ret;
        if (rx_char == ';') break;

        // add to the length buffer
        if (GetNextChar(&rx_char)) length_buffer[temp++] = rx_char;
    }

    // if timed out, flush the buffer and return error
    if (timeout <= millis()) {
        serial_stream->flush();
        return false;
    }

    // if the next char isn't a semicolon, there's been an error
    if (!ReadSpecificChar(timeout, ';')) return false;

    // convert the string length
    if (1 != sscanf(length_buffer, "%u", &temp)) return false;
    if (temp > 65535) return false;
    string_rx.str_length = (uint16_t) temp;

    // read the string section
    temp = 0;
    while (timeout > millis() && temp < string_rx.str_length && temp < (STRING_BUFFER_SIZE-1)) {
        if (GetNextChar(&rx_char)) string_rx.buffer[temp++] = (uint8_t) rx_char;
    }

    // if timed out, flush the buffer and return error
    if (timeout <= millis()) {
        serial_stream->flush();
        return false;
    }

    // null-terminate the buffer
    string_rx.buffer[temp] = '\0';


    // the message should end with a semi-colon before the checksum
    if (!ReadSpecificChar(timeout, ';')) return false;

    string_rx.checksum_valid = ReadChecksum(timeout);

    return true;
}

// -------------------------- TX --------------------------

void SerialComm::TX_ASCII()
{
    TX_ASCII(ascii_tx.msg_id);
}

void SerialComm::TX_ASCII(uint8_t msg_id)
{
    ResetChecksum();
    WriteChar(ASCII_DELIMITER);
    WriteASCIIu8(msg_id);
    for (int i = 0; i < ascii_tx.buffer_index; i++) {
        WriteChar(ascii_tx.buffer[i]);
    }
    WriteChar(';');
    WriteChecksum();
    serial_stream->print('\n');
    ResetTX();
}

void SerialComm::TX_Ack(uint8_t msg_id, bool ack_val)
{
    ResetChecksum();
    WriteChar(ACK_DELIMITER);
    WriteASCIIu8(msg_id);
    WriteChar(',');
    ack_val ? WriteChar('1') : WriteChar('0');
    WriteChar(';');
    WriteChecksum();
    serial_stream->print('\n');
}

bool SerialComm::TX_Bin()
{
    return TX_Bin(binary_tx.bin_id);
}

bool SerialComm::TX_Bin(uint8_t bin_id)
{
    if (binary_tx.bin_buffer == NULL) return false;

    ResetChecksum();
    WriteChar(BIN_DELIMITER);
    WriteASCIIu8(bin_id);
    WriteChar(',');
    WriteASCIIu16(binary_tx.bin_length);
    WriteChar(';');
    for (int i = 0; i < binary_tx.bin_length; i++) {
        WriteBinByte(binary_tx.bin_buffer[i]);
    }
    WriteChar(';');
    WriteChecksum();
    serial_stream->print('\n');

    return true;
}

void SerialComm::TX_String()
{
    TX_String(string_tx.str_id);
}

void SerialComm::TX_String(uint8_t str_id)
{
    ResetChecksum();
    WriteChar(STRING_DELIMITER);
    WriteASCIIu8(str_id);
    WriteChar(',');
    WriteASCIIu8(string_tx.str_length);
    WriteChar(';');
    for (int i = 0; i < string_tx.str_length; i++) {
        WriteBinByte(string_tx.buffer[i]);
    }
    WriteChar(';');
    WriteChecksum();
    serial_stream->print('\n');
}

void SerialComm::TX_String(uint8_t str_id, const char * msg)
{
    Add_string(msg);
    TX_String(str_id);
}

// --------------------- TX Helpers -----------------------

inline void SerialComm::WriteBinByte(uint8_t new_byte)
{
    serial_stream->write(new_byte);
    UpdateChecksum(new_byte);
}

inline void SerialComm::WriteChar(char new_char)
{
    serial_stream->print(new_char);
    UpdateChecksum((uint8_t) new_char);
}

void SerialComm::WriteASCIIu8(uint8_t new_u8)
{
    char ubuffer[4] = {0};
    int num = snprintf(ubuffer, 4, "%u", new_u8);

    if (num < 1 || num > 3) return;

    for (int i = 0; i < num; i++) {
        WriteChar(ubuffer[i]);
    }
}

void SerialComm::WriteASCIIu16(uint16_t new_u16)
{
    char ubuffer[6] = {0};
    int num = snprintf(ubuffer, 6, "%u", new_u16);

    if (num < 1 || num > 5) return;

    for (int i = 0; i < num; i++) {
        WriteChar(ubuffer[i]);
    }
}

// ---------------------- Checksum ------------------------

inline void SerialComm::UpdateChecksum(uint8_t new_byte)
{
    check_a = check_a + new_byte;
    check_b = check_b + check_a;
}

inline void SerialComm::ResetChecksum()
{
    check_a = 0;
    check_b = 0;
}

bool SerialComm::ReadChecksum(uint32_t timeout)
{
    unsigned int temp = 0;
    int read_ret = -1;
    char rx_char = '\0';
    char checksum_buffer[6] = "";
    uint16_t checksum = 0;

    // read the binary length
    while (timeout > millis() && temp < 5) {
        // check for delimiters
        read_ret = serial_stream->peek();
        if (-1 == read_ret) continue;
        rx_char = (char) read_ret;
        if (rx_char == ';') break;

        rx_char = serial_stream->read();
        checksum_buffer[temp++] = rx_char;
    }

    if (';' != serial_stream->read()) return false;

    // convert the checksum
    if (1 != sscanf(checksum_buffer, "%u", &temp)) return false;
    if (temp > 65535) return false;
    checksum = (uint16_t) temp;

    return (checksum == (((uint16_t) check_a << 8) | (uint16_t) check_b));
}

void SerialComm::WriteChecksum()
{
    uint16_t combined_checksum = check_b;
    combined_checksum |= (uint16_t) check_a << 8;

    WriteASCIIu16(combined_checksum);
    WriteChar(';');
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

bool SerialComm::Get_int8(int8_t * ret_val)
{
    char int_buffer[5] = {0};
    uint8_t max_index = ascii_rx.buffer_index + 4; // int8_t means 4 chars max
    int temp = 0;

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
    if (1 != sscanf(int_buffer, "%d", &temp)) return false;
    if (temp > 127 || temp < -128) return false;
    *ret_val = (int8_t) temp;

    return true;
}

bool SerialComm::Get_int16(int16_t * ret_val)
{
    char int_buffer[7] = {0};
    uint8_t max_index = ascii_rx.buffer_index + 6; // int16_t means 6 chars max
    int temp = 0;

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
    if (1 != sscanf(int_buffer, "%d", &temp)) return false;
    if (temp > 32767 || temp < -32768) return false;
    *ret_val = (int16_t) temp;

    return true;
}

bool SerialComm::Get_int32(int32_t * ret_val)
{
    char int_buffer[12] = {0};
    uint8_t max_index = ascii_rx.buffer_index + 11; // int32_t means 11 chars max
    int temp = 0;

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
    if (1 != sscanf(int_buffer, "%d", &temp)) return false;
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
    uint8_t buffer_remaining = ASCII_BUFFER_SIZE - ascii_tx.buffer_index;
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

bool SerialComm::Add_int8(int8_t val)
{
    return Add_int32((int32_t) val);
}

bool SerialComm::Add_int16(int16_t val)
{
    return Add_int32((int32_t) val);
}

bool SerialComm::Add_int32(int32_t val)
{
    uint8_t buffer_remaining = ASCII_BUFFER_SIZE - ascii_tx.buffer_index;
    int num_written = 0;

    // snprintf will return the number of chars it could write, but won't write more than buffer_remaining
    // note leading comma!
    num_written = snprintf(ascii_tx.buffer + ascii_tx.buffer_index, buffer_remaining, ",%d", (int) val);

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
    uint8_t buffer_remaining = ASCII_BUFFER_SIZE - ascii_tx.buffer_index;
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

void SerialComm::Add_string(const char * msg)
{
    uint16_t length = 0;

    while ('\0' != msg[length] && length < (STRING_BUFFER_SIZE - 1)) {
        string_tx.buffer[length] = msg[length];
        length++;
    }

    string_tx.str_length = length;
}
