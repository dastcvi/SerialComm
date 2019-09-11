# SerialComm

A simple, robust protocol and class for inter-Arduino UART communication. See examples/SerialComm_Test.ino
for a test script that exercises functionality.

The library also provides generic functions for serializing variables onto a uint8_t buffer for use when
constructing binary messages to send over serial. See examples/Serialize_Test.ino for the test/example.

*Note that checksums are currently unimplemented*

## Message Types

Three message types are supported: ASCII with parameters, ACK/NAK, and binary.

### ASCII Message

The basic structure of an internal message is as follows:

```
#msg_id,param_1,param_2,...,param_n;checksum;
```

`msg_id`:   the command number range 0:255 (uint8_t) expressed in ASCII

`param_n`:  The nth type-ambiguous param associated with the command. The param can even be a string
containing whitespace, but cannot contain restricted chars. The params (including the leading comma
for each parameter) can take up anywhere from 0 - 127 chars.

`checksum`: the TBD method checksum to verify the message integrity (future work, optional)

Special characters that can't be used in the param_n sections:
`,` or `;` or `#` or `?` or `!`

Note that a newline character at the end of a message (`\n`) isn't expected, but can be handled and is
generated for each TX message for ease of reading on a terminal.

In the case of a message with no parameters, the commas are omitted entirely:

```
#msg_id;checksum;
```

### ACK/NAK Message

The structure of an ACK/NAK message is as follows:

```
?msg_id,ack/nak;checksum;
```

`msg_id`:   the command number range 0:255 (uint8_t) expressed in ASCII

`ack/nak`:  the ACK or NAK expressed in an ascii '0' or '1'

`checksum`: the TBD method checksum to verify the message integrity (future work, optional)

### Binary message

The structure of a binary message is as follows:

```
!bin_id,length;bin;checksum;
```

`bin_id`:   type of binary data (ASCII)

`length`:   length of binary data (ASCII)

`bin`:      binary section

`checksum`: the TBD method checksum to verify the message integrity (future work, optional)

## Serialize Functions

The functions for serializing variables onto a uint8_t binary buffer are located in the Serialize.cpp and
Serialize.h files.

The functions for adding a variable to the buffer all take the data, buffer pointer,
size of the buffer, and a pointer to a variable tracking the current index to place bytes in the buffer.
The functions check for null pointers and ensure that the buffer won't be overwritten before serializing the
data onto the buffer. In the case of an error, the function returns false without writing to the buffer or
incrementing the index variable. Otherwise, the function increments the index variable according to the bytes
written.

The functions for reading variables from the buffer have the same interface and safety considerations, except
that the result variable must be passed as a pointer. It won't be written to unless all safety checks are passed.

*Note that the maximum buffer size supported is 65531 (which is UINT16_MAX - 4)*

## Description of provided software

The software core, SerialComm, doesn't maintain message types and is agnostic of the command IDs. It
simply provides a method for determining the message type, and reading it accordingly. This is all done
by calling the `SerialComm.RX()` function, which returns the message type (`SerialMessage_t`).

For an ASCII message, this entails parsing out the command id, verifying the checksum, and separating out a
string containing only the `,param_1,param_2,...,param_n` message (if present). Note that a leading comma
is included for each parameter. The core also provides standard, safe functions for parsing out and
generating individual params of different types from and for the message string. The message, its id, buffer,
and more are available in the `ascii_rx` struct of type `ASCII_MSG_t`.

For an ACK/NAK message, the member variable `ack_id` contains the id being ack'ed, and the `ack_value` contains
the ACK/NAK value as a boolean.

In order to accomodate projects that need large binary messages, the binary TX and RX buffers must be
allocated outside of the object and then "attached" using the `AssignBinaryRXBuffer()` and
`AssignBinaryTXBuffer()` functions. These buffers are located in the `binary_rx` and `binary_tx` structs along with the size of the buffer, number of objects in it, and an ID for the binary type. Calling `TX_Bin()` sends the TX buffer, and when an RX message is received, the message is written to the RX buffer
(unless the message is too large).

## ASCII Message Usage

*For an example implementation, look at* `examples/Example_Interface`

For a board to use this protocol, it should implement files on top of the core to enumerate command ID's
and to provide functions for parsing and generating each command (using the provided functions). The
core will provide the router that parses out the ID, checks the checksum, and places the message in a
statically-allocated buffer for the instruments to parse.

For example, the protocol for two boards that need to communicate with eachother could be implemented in
one class that inherits from SerialComm. The header file could look like the following:

```C++
#include "SerialComm.h"

enum InternalMessages_t : uint8_t {
    NO_MESSAGE = 0, // zero reserved as default
    MESSAGE1 = 1,
    // and so on...
};

class InternalMessaging : SerialComm {
public:
    bool TX_Message1(uint8_t param);
    bool RX_Message1(uint8_t * param);
};
```

The `bool TX_Message1(uint8_t param)` function could be written like this:

```C++
bool InternalMessaging::TX_Message1(uint8_t param)
{
    if (!Add_uint8(param)) return false;
    // if there were subsequent parameters, they would go here
    return TX_ASCII(MESSAGE1);
}
```

In order for RX to work, the main file would need to call the `RX()` function from the instantiated
`InternalMessaging` object at a regular interval, and parse based on the return value. If an ASCII message
is returned with `ascii_rx.msg_id == MESSAGE1`, a `bool RX_Message1(uint8_t * param)` function like the
following could be called.

```C++
bool InternalMessaging::RX_Message1(uint8_t * param)
{
    if (!Get_uint8(param)) return false;
    // if there were subsequent parameters, they would go here
    return true;
}
```

## Binary Usage

The interface for binary messages is comparably simpler than for ASCII messages, but the software provides
fewer utilites for using binary messages. The user must generate and parse the binary section, but the
software handles the transmission over UART in its entirety. A benefit to needing to assign TX and RX
buffers to the class is that buffers of up to 65356 (uint16 max) bytes can be used for either, and multiple
buffers could be used if desired.

Say in the case from the ASCII Message Usage section the inheriting class had two global binary buffers,
`uint8_t bin1[16]` and `uint8_t bin2[32]` to transmit. The following two functions would transmit the
buffers:

```C++
enum BinMessages_t : uint8_t {
    NO_BIN = 0,
    BIN1 = 1,
    BIN2 = 2,
};

uint8_t bin1[16];
uint8_t bin2[32];

bool InternalMessaging::TX_Bin1(uint16_t bytes_in_buffer)
{
    AssignBinaryTXBuffer(bin1, 16, bytes_in_buffer); // bytes_in_buffer exists in case buffer isn't full
    TX_Bin(BIN1);
}

bool InternalMessaging::TX_Bin2(uint16_t bytes_in_buffer)
{
    AssignBinaryTXBuffer(bin2, 32, bytes_in_buffer); // bytes_in_buffer exists in case buffer isn't full
    TX_Bin(BIN2);
}
```

Receiving messages works similarly. In some cases, it makes sense to just keep one generic RX buffer
assigned to the class, but if the user wants to read different message types into different locations (or subsequent messages into subsequent arrays), then the user can reassign the RX buffer to do so.

## Aside on Arduino's internal serial buffering

This protocol and class is specifically designed for use on the Teensy 3.6 Arduino-compatible MCU board,
where it is easy to adjust the size of the serial driver's internal buffers. This way, the user program
doesn't need to worry about over-running the internal buffer with large messages or adding buffering
to the software. Thus, this class can just use the `Serial.available()` and `Serial.read()` functions
on the internal software buffers.

(https://forum.pjrc.com/threads/49470-Changing-hardware-serial-buffer-size?p=167006&viewfull=1#post167006)
