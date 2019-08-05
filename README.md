# SerialComm

A simple, robust protocol and class for inter-Arduino UART communication. See examples/SerialComm_Test.ino
for a test script that exercises functionality.

*Note that checksums are currently unimplemented*

*Note that binary is currently unimplemented*

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

Binary messages are in progress.

## Usage

For a board to use this protocol, it should implement files on top of the core to enumerate command ID's
and to provide functions for parsing and generating each command (using the provided functions). The
core will provide the router that parses out the ID, checks the checksum, and places the message in a
statically-allocated buffer for the instruments to parse.

For example, the protocol for two boards that need to communicate with eachother could be implemented in
one class that inherits from SerialComm. The header file could look like the following:

```C++
#include "SerialComm.h"

enum InternalMessages_t : uint8_t {
    NO_MESSAGE = 0, // necessary
    MESSAGE1 = 1,
    // and so on...
};

class InternalMessaging : SerialComm(&Serial1) {
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
}
```

## Aside on Arduino's internal serial buffering

This protocol and class is specifically designed for use on the Teensy 3.6 Arduino-compatible MCU board,
where it is easy to adjust the size of the serial driver's internal buffers. This way, the user program
doesn't need to worry about over-running the internal buffer with large messages or adding buffering
to the software. Thus, this class can just use the `Serial.available()` and `Serial.read()` functions
on the internal software buffers.

(https://forum.pjrc.com/threads/49470-Changing-hardware-serial-buffer-size?p=167006&viewfull=1#post167006)
