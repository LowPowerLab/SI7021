/*
Copyright 2014 Marcus Sorensen <marcus@electron14.com>

This program is licensed, please check with the copyright holder for terms

Updated: Jul 16, 2015: TomWS1: 
        eliminated Byte constants, 
        fixed 'sizeof' error in _command(), 
        added getTempAndRH() function to simplify calls for C & RH only
*/
#include "Arduino.h"
#include "SI7021.h"
#include <Wire.h>

#define I2C_ADDR 0x40

// I2C commands
#define RH_READ             0xE5 
#define TEMP_READ           0xE3 
#define POST_RH_TEMP_READ   0xE0 
#define RESET               0xFE 
#define USER1_READ          0xE7 
#define USER1_WRITE         0xE6 

// compound commands
byte SERIAL1_READ[]      ={ 0xFA, 0x0F };
byte SERIAL2_READ[]      ={ 0xFC, 0xC9 };

bool _si_exists = false;

SI7021::SI7021() {
}

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
bool SI7021::begin(int SDA, int SCL) {
    Wire.begin(SDA,SCL);
#else
bool SI7021::begin() {
    Wire.begin();
#endif
    Wire.beginTransmission(I2C_ADDR);
    if (Wire.endTransmission() == 0) {
        _si_exists = true;
    }
    return _si_exists;
}

bool SI7021::sensorExists() {
    return _si_exists;
}

int SI7021::getFahrenheitHundredths() {
    int c = getCelsiusHundredths();
    return (1.8 * c) + 3200;
}

int SI7021::getCelsiusHundredths() {
    byte tempbytes[2];
    _command(TEMP_READ, tempbytes);
    long tempraw = (long)tempbytes[0] << 8 | tempbytes[1];
    return ((17572 * tempraw) >> 16) - 4685;
}

int SI7021::_getCelsiusPostHumidity() {
    byte tempbytes[2];
    _command(POST_RH_TEMP_READ, tempbytes);
    long tempraw = (long)tempbytes[0] << 8 | tempbytes[1];
    return ((17572 * tempraw) >> 16) - 4685;
}


unsigned int SI7021::getHumidityPercent() {
    byte humbytes[2];
    _command(RH_READ, humbytes);
    long humraw = (long)humbytes[0] << 8 | humbytes[1];
    return ((125 * humraw) >> 16) - 6;
}

unsigned int SI7021::getHumidityBasisPoints() {
    byte humbytes[2];
    _command(RH_READ, humbytes);
    long humraw = (long)humbytes[0] << 8 | humbytes[1];
    return ((12500 * humraw) >> 16) - 600;
}

void SI7021::_command(byte cmd, byte * buf ) {
    _writeReg(&cmd, sizeof cmd);
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    delay(25);
#endif
    _readReg(buf, 2);
}

void SI7021::_writeReg(byte * reg, int reglen) {
    Wire.beginTransmission(I2C_ADDR);
    for(int i = 0; i < reglen; i++) {
        reg += i;
        Wire.write(*reg); 
    }
    Wire.endTransmission();
}

int SI7021::_readReg(byte * reg, int reglen) {
    Wire.requestFrom(I2C_ADDR, reglen);
    //while(Wire.available() < reglen); //remove redundant loop-wait per https://github.com/LowPowerLab/SI7021/issues/12
    for(int i = 0; i < reglen; i++) { 
        reg[i] = Wire.read(); 
    }
    return 1;
}

int SI7021::getSerialBytes(byte * buf) {
  byte serial[8];
  _writeReg(SERIAL1_READ, sizeof SERIAL1_READ);
  _readReg(serial, 8);
  
  //Page23 - https://www.silabs.com/Support%20Documents%2FTechnicalDocs%2FSi7021-A20.pdf
  buf[0] = serial[0]; //SNA_3
  buf[1] = serial[2]; //SNA_2
  buf[2] = serial[4]; //SNA_1
  buf[3] = serial[6]; //SNA_0

  _writeReg(SERIAL2_READ, sizeof SERIAL2_READ);
  _readReg(serial, 6);
  buf[4] = serial[0]; //SNB_3 - device ID byte
  buf[5] = serial[1]; //SNB_2
  buf[6] = serial[3]; //SNB_1
  buf[7] = serial[4]; //SNB_0
  return 1;
}

int SI7021::getDeviceId() {
  //0x0D=13=Si7013
  //0x14=20=Si7020
  //0x15=21=Si7021
  byte serial[8];
  getSerialBytes(serial);
  int id = serial[4];
  return id;
}

// 0x00 = 14 bit temp, 12 bit RH (default)
// 0x01 = 12 bit temp, 8 bit RH
// 0x80 = 13 bit temp, 10 bit RH
// 0x81 = 11 bit temp, 11 bit RH
void SI7021::setPrecision(byte setting) {
    byte reg = USER1_READ;
    _writeReg(&reg, 1);
    _readReg(&reg, 1);

    reg = (reg & 0x7E) | (setting & 0x81);
    byte userwrite[] = {USER1_WRITE, reg};
    _writeReg(userwrite, sizeof userwrite);
}

// NOTE on setHeater() function:
// Depending if 'on' parameter it sets it to:
// - 0x3A (0011_1010 - POR default) for OFF
// - 0x3E (0011_1110) for ON
// This resets the resolution bits to 00 in both cases - which is maximum resolution (12bit RH and 14bit Temp)
// For User1 register usage see p26 (section 6) in DSheet https://www.silabs.com/documents/public/data-sheets/Si7021-A20.pdf
void SI7021::setHeater(bool on) {
    byte userbyte;
    if (on) {
        userbyte = 0x3E;
    } else {
        userbyte = 0x3A;
    }
    byte userwrite[] = {USER1_WRITE, userbyte};
    _writeReg(userwrite, sizeof userwrite);
}

// get humidity, then get temperature reading from humidity measurement
struct si7021_env SI7021::getHumidityAndTemperature() {
    si7021_env ret = {0, 0, 0};
    ret.humidityBasisPoints      = getHumidityBasisPoints();
    ret.celsiusHundredths        = _getCelsiusPostHumidity();
    ret.fahrenheitHundredths     = (1.8 * ret.celsiusHundredths) + 3200;
    return ret;
}

// get temperature (C only) and RH Percent
struct si7021_thc SI7021::getTempAndRH()
{
    si7021_thc ret;
    
    ret.humidityPercent   = getHumidityPercent();
    ret.celsiusHundredths = _getCelsiusPostHumidity();
    return ret;

}
