#include <Wire.h>

#define OV7670_I2C_ADDRESS 0x21
///////// Main Program //////////////
void setup() {
  Wire.begin();
  Serial.begin(9600);
  
  // TODO: READ KEY REGISTERS
  Serial.println("Before Setting");
  read_key_registers();
  
  delay(100);
  
  // TODO: WRITE KEY REGISTERS
  set_color_matrix();

  Serial.println("After Setting");
  read_key_registers();
}

void loop(){
 }


///////// Function Definition //////////////
void read_key_registers(){
  /*TODO: DEFINE THIS FUNCTION*/

  Serial.print("11=");
  Serial.println(read_register_value(0x11),HEX);
  Serial.print("1E=");
  Serial.println(read_register_value(0x1E),HEX);
  Serial.print("0C=");
  Serial.println(read_register_value(0x0C),HEX);
  Serial.print("12=");
  Serial.println(read_register_value(0x12),HEX);
  Serial.print("40=");
  Serial.println(read_register_value(0x40),HEX);
  Serial.print("42=");
  Serial.println(read_register_value(0x42),HEX);

  Serial.println();

  Serial.print("4f=");
  Serial.println(read_register_value(0x4f),HEX);
  Serial.print("50=");
  Serial.println(read_register_value(0x50),HEX);
  Serial.print("51=");
  Serial.println(read_register_value(0x51),HEX);
  Serial.print("52=");
  Serial.println(read_register_value(0x52),HEX);
  Serial.print("53=");
  Serial.println(read_register_value(0x53),HEX);
  Serial.print("54=");
  Serial.println(read_register_value(0x54),HEX);
  Serial.print("56=");
  Serial.println(read_register_value(0x56),HEX);
  Serial.print("58=");
  Serial.println(read_register_value(0x58),HEX);
  Serial.print("59=");
  Serial.println(read_register_value(0x59),HEX);
  Serial.print("5a=");
  Serial.println(read_register_value(0x5a),HEX);
  Serial.print("5b=");
  Serial.println(read_register_value(0x5b),HEX);
  Serial.print("5c=");
  Serial.println(read_register_value(0x5c),HEX);
  Serial.print("5d=");
  Serial.println(read_register_value(0x5d),HEX);
  Serial.print("5e=");
  Serial.println(read_register_value(0x5e),HEX);
  Serial.print("69=");
  Serial.println(read_register_value(0x69),HEX);
  Serial.print("6a=");
  Serial.println(read_register_value(0x6a),HEX);
  Serial.print("6b=");
  Serial.println(read_register_value(0x6b),HEX);
  Serial.print("6c=");
  Serial.println(read_register_value(0x6c),HEX);
  Serial.print("6d=");
  Serial.println(read_register_value(0x6d),HEX);
  Serial.print("6e=");
  Serial.println(read_register_value(0x6e),HEX);
  Serial.print("6f=");
  Serial.println(read_register_value(0x6f),HEX);
  Serial.print("b0=");
  Serial.println(read_register_value(0xb0),HEX);
  
  
  
}

byte read_register_value(int register_address){
  byte data = 0;
  Wire.beginTransmission(OV7670_I2C_ADDRESS);
  Wire.write(register_address);
  Wire.endTransmission();
  Wire.requestFrom(OV7670_I2C_ADDRESS,1);
  while(Wire.available()<1);
  data = Wire.read();
  return data;
}

String OV7670_write(int start, const byte *pData, int size){
    int n,error;
    Wire.beginTransmission(OV7670_I2C_ADDRESS);
    n = Wire.write(start);
    if(n != 1){
      return "I2C ERROR WRITING START ADDRESS";   
    }
    n = Wire.write(pData, size);
    if(n != size){
      return "I2C ERROR WRITING DATA";
    }
    error = Wire.endTransmission(true);
    if(error != 0){
      return String(error);
    }
    return "no errors :)";
 }

String OV7670_write_register(int reg_address, byte data){
  return OV7670_write(reg_address, &data, 1);
 }

void set_color_matrix(){

//    OV7670_write_register(0x11, 0xC0);
//    OV7670_write_register(0x1E, 0x30);
//    OV7670_write_register(0x0C, 0x08);
//    OV7670_write_register(0x12, 0x0C);
//    OV7670_write_register(0x40, 0xF0);
//    OV7670_write_register(0x42, 0x0C);

    OV7670_write_register(0x11, 0xC0);
    OV7670_write_register(0x1E, 0x30);
    OV7670_write_register(0x0C, 0x08);
    OV7670_write_register(0x12, 0x0C);
    OV7670_write_register(0x40, 0xD0);
    OV7670_write_register(0x42, read_register_value(0x42)&0xF7);
 //   OV7670_write_register(0x42, 0x00);
    
  // OV7670_write_register(0x8C, 0x03);

  
    OV7670_write_register(0x4f, 0x80);
    OV7670_write_register(0x50, 0x80);
    OV7670_write_register(0x51, 0x00);
    OV7670_write_register(0x52, 0x22);
    OV7670_write_register(0x53, 0x5e);
    OV7670_write_register(0x54, 0x80);
    OV7670_write_register(0x56, 0x40);
    OV7670_write_register(0x58, 0x9e);
    OV7670_write_register(0x59, 0x88);
    OV7670_write_register(0x5a, 0x88);
    OV7670_write_register(0x5b, 0x44);
    OV7670_write_register(0x5c, 0x67);
    OV7670_write_register(0x5d, 0x49);
    OV7670_write_register(0x5e, 0x0e);
    OV7670_write_register(0x69, 0x00);
    OV7670_write_register(0x6a, 0x40);
    OV7670_write_register(0x6b, 0x0a);
    OV7670_write_register(0x6c, 0x0a);
    OV7670_write_register(0x6d, 0x55);
    OV7670_write_register(0x6e, 0x11);
    OV7670_write_register(0x6f, 0x9f);
    OV7670_write_register(0xb0, 0x84);
}
