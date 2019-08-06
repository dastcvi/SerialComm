/*  SerialComm.ino
 *  Author: Alex St. Clair
 *  Created: July 2019
 */
 
#include <SerialComm.h>

SerialComm ser(&Serial);

uint8_t temp_u8 = 0;
uint16_t temp_u16 = 0;
uint32_t temp_u32 = 0;
int8_t temp_i8 = 0;
int16_t temp_i16 = 0;
int32_t temp_i32 = 0;
float temp_float = 0.0f;
char temp_buffer[128] = {0};

uint8_t bin_rx[128] = {0};
char bin_tx[] = "Readable test binary";
uint16_t bin_tx_length = sizeof(bin_tx);

void setup()
{
  Serial.begin(115200);
  delay(2500);
  Serial.println("Ready for messages");

  ser.AssignBinaryRXBuffer(bin_rx, 128);
  ser.AssignBinaryTXBuffer((uint8_t *) bin_tx, bin_tx_length);
}

void loop()
{
  switch (ser.RX()) {
  case ASCII_MESSAGE:
    Serial.print("Received message: "); Serial.println(ser.ascii_rx.msg_id);
    //Serial.print("Buffer: "); Serial.println(ser.ascii_rx.buffer);
    
    // test cases
    switch (ser.ascii_rx.msg_id) {
    case 32:
      ser.TX_Ack(32,1);
      break;
    case 64:
      if (ser.Get_uint8(&temp_u8)) {
        Serial.print("u8: "); Serial.println(temp_u8);
      } else {
        Serial.println("u8: error");
      }
      if (ser.Get_uint16(&temp_u16)) {
        Serial.print("u16: "); Serial.println(temp_u16);
      } else {
        Serial.println("u16: error");
      }
      if (ser.Get_uint32(&temp_u32)) {
        Serial.print("u32: "); Serial.println(temp_u32);
      } else {
        Serial.println("u32: error");
      }
      if (ser.Get_float(&temp_float)) {
        Serial.print("float: "); Serial.println(temp_float);
      } else {
        Serial.println("float: error");
      }
      if (ser.Get_string(temp_buffer, 128)) {
        Serial.print("string: "); Serial.println(temp_buffer);
      } else {
        Serial.println("string: error");
      }
      break;
    case 66:
      if (ser.Get_int8(&temp_i8)) {
        Serial.print("i8: "); Serial.println(temp_i8);
      } else {
        Serial.println("i8: error");
      }
      if (ser.Get_int16(&temp_i16)) {
        Serial.print("i16: "); Serial.println(temp_i16);
      } else {
        Serial.println("i16: error");
      }
      if (ser.Get_int32(&temp_i32)) {
        Serial.print("i32: "); Serial.println(temp_i32);
      } else {
        Serial.println("i32: error");
      }
      break;
    case 16:
      if (!ser.Add_uint8(10)) {
        Serial.println("Error adding u8");
        break;
      }
      if (!ser.Add_uint16(1000)) {
        Serial.println("Error adding u16");
        break;
      }
      if (!ser.Add_uint32(100000)) {
        Serial.println("Error adding u32");
        break;
      }
      if (!ser.Add_int8(-10)) {
        Serial.println("Error adding i8");
        break;
      }
      if (!ser.Add_int16(-1000)) {
        Serial.println("Error adding i16");
        break;
      }
      if (!ser.Add_int32(-100000)) {
        Serial.println("Error adding i32");
        break;
      }
      if (!ser.Add_float(1.101)) {
        Serial.println("Error adding float");
        break;
      }
      if (!ser.Add_string("test string")) {
        Serial.println("Error adding string");
        break;
      }
      ser.TX_ASCII(17);
    default:
      break;
    }
    break;
  case ACK_MESSAGE:
    Serial.print("ACK/NAK for msg: "); Serial.println(ser.ack_id);
    Serial.print("Value: ");
    ser.ack_value ? Serial.println("ACK") : Serial.println("NAK");
    break;
  case BIN_MESSAGE:
    Serial.print("Binary message: "); Serial.println(ser.binary_rx.bin_id);
    Serial.print("Buffer: ");
    for (int i = 0; i < ser.binary_rx.bin_length; i++) {
      Serial.print((char) ser.binary_rx.bin_buffer[i]);
    }
    Serial.println();
    if (ser.binary_rx.bin_id == 16) {
      ser.binary_tx.bin_length = bin_tx_length;
      ser.TX_Bin();
    }
    break;
  case NO_MESSAGE:
  default:
    break;
  }
  delay(500);
}

