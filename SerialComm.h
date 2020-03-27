/*
 * SerialComm.h
 * Author:  Alex St. Clair
 * Created: August 2019
 *
 * This file declares an Arduino library (C++ class) that implements a simple, robust
 * serial (UART) protocol for inter-Arduino messaging.
 *
 * This class doesn't define specific messages, so any project using the protocol must
 * implement message definitions on top of this class.
 */

#ifndef SERIALCOMM_H
#define SERIALCOMM_H

#include "Arduino.h"
#include <stdint.h>

#define ASCII_DELIMITER    '#'
#define ACK_DELIMITER      '?'
#define BIN_DELIMITER      '!'
#define STRING_DELIMITER   '"'

#define READ_TIMEOUT       100 // milliseconds

#define ASCII_BUFFER_SIZE  128
#define STRING_BUFFER_SIZE 128

enum SerialMessage_t {
    NO_MESSAGE,
    ASCII_MESSAGE,
    ACK_MESSAGE,
    BIN_MESSAGE,
    STRING_MESSAGE
};

struct ASCII_MSG_t {
    uint8_t msg_id;
    uint8_t num_params;
    uint8_t buffer_index;
    bool checksum_valid;
    char buffer[ASCII_BUFFER_SIZE];
};

struct STRING_MSG_t {
    uint8_t str_id;
    uint16_t str_length;
    bool checksum_valid;
    char buffer[STRING_BUFFER_SIZE];
};

struct BIN_MSG_t {
    uint8_t bin_id;
    uint16_t bin_length;
    uint16_t buffer_size;
    bool checksum_valid;
    uint8_t * bin_buffer;
};

class SerialComm {
public:
    SerialComm(Stream * stream_in);
    ~SerialComm() { };

    // To allow user to change to the USB serial port for testing or debug
    void UpdatePort(Stream * stream_in);

    // Attach pre-allocated buffers for binary messaging
    void AssignBinaryRXBuffer(uint8_t * buffer, uint16_t size);
    void AssignBinaryTXBuffer(uint8_t * buffer, uint16_t size, uint16_t num_bytes);

    // Receive interface
    SerialMessage_t RX();

    // Transmit interface
    void TX_ASCII();
    void TX_ASCII(uint8_t msg_id);
    void TX_Ack(uint8_t msg_id, bool ack_val);
    bool TX_Bin();
    bool TX_Bin(uint8_t bin_id);
    void TX_String(uint8_t str_id, const char * msg);

    // ASCII RX buffer interface
    bool Get_uint8(uint8_t * ret_val);
    bool Get_uint16(uint16_t * ret_val);
    bool Get_uint32(uint32_t * ret_val);
    bool Get_int8(int8_t * ret_val);
    bool Get_int16(int16_t * ret_val);
    bool Get_int32(int32_t * ret_val);
    bool Get_float(float * ret_val);

    // ASCII TX buffer interface
    bool Add_uint8(uint8_t val);
    bool Add_uint16(uint16_t val);
    bool Add_uint32(uint32_t val);
    bool Add_int8(int8_t val);
    bool Add_int16(int16_t val);
    bool Add_int32(int32_t val);
    bool Add_float(float val);

    // ASCII messages with buffers
    ASCII_MSG_t ascii_rx = {0};
    ASCII_MSG_t ascii_tx = {0};

    // Binary messages with buffers
    BIN_MSG_t binary_rx = {0};
    BIN_MSG_t binary_tx = {0};

    // String messages with buffers
    STRING_MSG_t string_rx = {0};
    STRING_MSG_t string_tx = {0};

    // Last ACK/NAK
    uint8_t ack_id = 0;
    bool ack_value = false;
    bool ack_checksum = false;

private:
    // Receive message parsing
    bool Read_ASCII(uint32_t timeout);
    bool Read_Ack(uint32_t timeout);
    bool Read_Bin(uint32_t timeout);
    bool Read_String(uint32_t timeout);

    // reset RX/TX internal state
    void ResetRX();
    void ResetTX();

    // deal with safely reading characters and updating the checksum
    bool GetNextChar(char * new_char);
    bool ReadSpecificChar(uint32_t timeout, char specific_char);

    // deal with safely writing characters and updating the checksum
    void WriteBinByte(uint8_t new_byte);
    void WriteChar(char new_char);
    void WriteASCIIu8(uint8_t new_u8);
    void WriteASCIIu16(uint16_t new_u16);

    // read or write the checksum (concatenate a and b into a uint16_t)
    bool ReadChecksum(uint32_t timeout);
    void WriteChecksum();

    // checksum calculation and values
    void UpdateChecksum(uint8_t new_byte);
    void ResetChecksum();
    uint8_t check_a = 0;
    uint8_t check_b = 0;

    // Serial port
    Stream * serial_stream;

};

#endif /* SERIALCOMM_H */