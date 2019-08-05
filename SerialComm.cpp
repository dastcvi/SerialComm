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

void SerialComm::TX_ASCII()
{
    rx_stream->print(ASCII_DELIMITER);
    rx_stream->print(ascii_tx.msg_id);
    if (ascii_tx.msg_length > 0) { // todo: ensure always updated
        rx_stream->print(ascii_tx.buffer);
    }
    rx_stream->println(';');
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
        if (';' == rx_char) return true;
        if (',' == rx_char) ascii_rx.num_params++;

        // add character to the buffer
        ascii_rx.buffer[ascii_rx.msg_length++] = rx_char;
    }

    return false;
}

bool SerialComm::Read_Ack(uint32_t timeout)
{
    return false;
}

bool SerialComm::Read_Bin(uint32_t timeout)
{
    return false;
}

void SerialComm::ResetRX()
{
    ascii_rx.msg_id = 0;
    ascii_rx.msg_length = 0;
    ascii_rx.num_params = 0;
    ascii_rx.buffer[0] = '\0';
}