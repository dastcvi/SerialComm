# SerialComm

A simple, robust protocol and class for inter-Arduino UART communication.

*Note that checksums are currently unimplemented*

## ASCII Message

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

## ACK/NAK Message

The structure of an ACK/NAK message is as follows:

```
?msg_id,ack/nak;checksum;
```

`msg_id`:   the command number range 0:255 (uint8_t) expressed in ASCII

`ack/nak`:  the ACK or NAK expressed in an ascii '0' or '1'

`checksum`: the TBD method checksum to verify the message integrity (future work, optional)

## Binary message

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
simply provides a method for parsing out the command id, verifying the checksum, and separating out a
string containing only the `,param_1,param_2,...,param_n` message (if present). Note that a leading comma
is included for each parameter. The core also provides standard, safe functions for parsing out and 
generating individual params of different types from and for the message string.

In the case that that ACK/NAK response is required from a message, the protocol handles this by using a
different delimiter for the message start: `?`. This is followed only by the ACK/NAK and the checksum.

Binary messages use the `!` delimiter for the message start, and the user must pass the statically allocated
RX and TX buffers into the class when each object is instantiated. This way, if very large messages are
expected, the user can allocate a large enough buffer.

For a board to use this protocol, it should implement files on top of the core to enumerate command ID's
and to provide functions for parsing and generating each command (using the provided functions). The
core will provide the router that parses out the ID, checks the checksum, and places the message in a
statically-allocated buffer for the instruments to parse.

## Aside on Arduino's internal serial buffering

This protocol and class is specifically designed for use on the Teensy 3.6 Arduino-compatible MCU board,
where it is easy to adjust the size of the serial driver's internal buffers. This way, the user program
doesn't need to worry about over-running the internal buffer with large messages or adding buffering
to the software. Thus, this class can just use the `Serial.available()` and `Serial.read()` functions
on the internal software buffers.

(https://forum.pjrc.com/threads/49470-Changing-hardware-serial-buffer-size?p=167006&viewfull=1#post167006)
