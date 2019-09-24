/*  SerialComm.ino
 *  Author: Alex St. Clair
 *  Created: July 2019
 */
 
#include <Serialize.h>

// use unique bytes for the test variables

uint8_t u8_in = 0xAC;
uint8_t u8_out = 0;

uint16_t u16_in = 0xB5CA;
uint16_t u16_out = 0;

uint32_t u32_in = 0xB3CB951F;
uint32_t u32_out = 0;

int8_t i8_in = -0x5C;
int8_t i8_out = 0;

int16_t i16_in = -0x6AC4;
int16_t i16_out = 0;

int32_t i32_in = -0x1FC893D2;
int32_t i32_out = 0;

float float_in = 12.035f;
float float_out = 0;

uint8_t test_buffer[128] = {0};
uint16_t curr_index = 0;

// test result variables
bool size_test = true;
bool input_test = true;
bool output_test = true;

void setup()
{
  Serial.begin(115200);
  delay(2500);

  size_test &= !BufferAddUInt8(u8_in, test_buffer, 0, &curr_index);
  size_test &= !BufferAddUInt16(u16_in, test_buffer, 0, &curr_index);
  size_test &= !BufferAddUInt32(u32_in, test_buffer, 0, &curr_index);
  size_test &= !BufferAddInt8(i8_in, test_buffer, 0, &curr_index);
  size_test &= !BufferAddInt16(i16_in, test_buffer, 0, &curr_index);
  size_test &= !BufferAddInt32(i32_in, test_buffer, 0, &curr_index);
  size_test &= !BufferAddFloat(float_in, test_buffer, 0, &curr_index);
  size_test &= (0 == test_buffer[0]);
  curr_index = 128;
  size_test &= !BufferAddUInt8(u8_in, test_buffer, 128, &curr_index);
  curr_index = 127;
  size_test &= !BufferAddUInt16(u16_in, test_buffer, 128, &curr_index);
  curr_index = 125;
  size_test &= !BufferAddUInt32(u32_in, test_buffer, 128, &curr_index);
  curr_index = 128;
  size_test &= !BufferAddInt8(i8_in, test_buffer, 128, &curr_index);
  curr_index = 127;
  size_test &= !BufferAddInt16(i16_in, test_buffer, 128, &curr_index);
  curr_index = 125;
  size_test &= !BufferAddInt32(i32_in, test_buffer, 128, &curr_index);
  curr_index = 125;
  size_test &= !BufferAddFloat(float_in, test_buffer, 128, &curr_index);
  size_test &= (0 == test_buffer[0]);

  if (size_test) {
    Serial.println("Passed size test");
  } else {
    Serial.println("FAILED size test");
  }

  curr_index = 0;
  input_test &= BufferAddUInt8(u8_in, test_buffer, 128, &curr_index);
  input_test &= BufferAddUInt16(u16_in, test_buffer, 128, &curr_index);
  input_test &= BufferAddUInt32(u32_in, test_buffer, 128, &curr_index);
  input_test &= BufferAddInt8(i8_in, test_buffer, 128, &curr_index);
  input_test &= BufferAddInt16(i16_in, test_buffer, 128, &curr_index);
  input_test &= BufferAddInt32(i32_in, test_buffer, 128, &curr_index);
  input_test &= BufferAddFloat(float_in, test_buffer, 128, &curr_index);

  if (input_test) {
    Serial.println("Passed input test  (big endian)");
  } else {
    Serial.println("FAILED input test  (big endian)");
  }

  curr_index = 0;

  output_test &= BufferGetUInt8(&u8_out, test_buffer, 128, &curr_index);
  output_test &= BufferGetUInt16(&u16_out, test_buffer, 128, &curr_index);
  output_test &= BufferGetUInt32(&u32_out, test_buffer, 128, &curr_index);
  output_test &= BufferGetInt8(&i8_out, test_buffer, 128, &curr_index);
  output_test &= BufferGetInt16(&i16_out, test_buffer, 128, &curr_index);
  output_test &= BufferGetInt32(&i32_out, test_buffer, 128, &curr_index);
  output_test &= BufferGetFloat(&float_out, test_buffer, 128, &curr_index);
  output_test &= (u8_in == u8_out);
  output_test &= (u16_in == u16_out);
  output_test &= (u32_in == u32_out);
  output_test &= (i8_in == i8_out);
  output_test &= (i16_in == i16_out);
  output_test &= (i32_in == i32_out);
  output_test &= (float_in == float_out);

  if (output_test) {
    Serial.println("Passed output test (big endian)");
  } else {
    Serial.println("FAILED output test (big endian)");
  }

  // switch to little endian
  endianness = SERIALIZE_LITTLE_ENDIAN;

  curr_index = 0;
  input_test = true;
  input_test &= BufferAddUInt8(u8_in, test_buffer, 128, &curr_index);
  input_test &= BufferAddUInt16(u16_in, test_buffer, 128, &curr_index);
  input_test &= BufferAddUInt32(u32_in, test_buffer, 128, &curr_index);
  input_test &= BufferAddInt8(i8_in, test_buffer, 128, &curr_index);
  input_test &= BufferAddInt16(i16_in, test_buffer, 128, &curr_index);
  input_test &= BufferAddInt32(i32_in, test_buffer, 128, &curr_index);
  input_test &= BufferAddFloat(float_in, test_buffer, 128, &curr_index);

  if (input_test) {
    Serial.println("Passed input test  (little endian)");
  } else {
    Serial.println("FAILED input test  (little endian)");
  }

  curr_index = 0;
  output_test = true;
  output_test &= BufferGetUInt8(&u8_out, test_buffer, 128, &curr_index);
  output_test &= BufferGetUInt16(&u16_out, test_buffer, 128, &curr_index);
  output_test &= BufferGetUInt32(&u32_out, test_buffer, 128, &curr_index);
  output_test &= BufferGetInt8(&i8_out, test_buffer, 128, &curr_index);
  output_test &= BufferGetInt16(&i16_out, test_buffer, 128, &curr_index);
  output_test &= BufferGetInt32(&i32_out, test_buffer, 128, &curr_index);
  output_test &= BufferGetFloat(&float_out, test_buffer, 128, &curr_index);
  output_test &= (u8_in == u8_out);
  output_test &= (u16_in == u16_out);
  output_test &= (u32_in == u32_out);
  output_test &= (i8_in == i8_out);
  output_test &= (i16_in == i16_out);
  output_test &= (i32_in == i32_out);
  output_test &= (float_in == float_out);

  if (output_test) {
    Serial.println("Passed output test (little endian)");
  } else {
    Serial.println("FAILED output test (little endian)");
  }

  Serial.println("Conclusion of tests");
}

void loop()
{
  delay(500);
}

