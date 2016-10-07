/*
Copyright 2014 Marcus Sorensen <marcus@electron14.com>

This program is licensed, please check with the copyright holder for terms

Updated:
	* Jul 16, 2015: TomWS1: 
		eliminated Byte constants, 
		fixed 'sizeof' error in _command(), 
		added getTempAndRH() function to simplify calls for C & RH only
	* Aug 25, 2016: jlblancoc
		added an optional timeout to all read functions to avoid infinite locks upon sensor failure or disconnection.
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

#if defined(ARDUINO_ARCH_ESP8266)
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

int SI7021::getCelsiusHundredths(bool *timeout /*= NULL*/, int timeout_ms/*=0*/)
{
    byte tempbytes[2];
    if (!_command(TEMP_READ, tempbytes, timeout_ms)) {
		if (timeout) *timeout = true;
		return 0;
	}
	if (timeout) *timeout = false;
    long tempraw = (long)tempbytes[0] << 8 | tempbytes[1];
    return ((17572 * tempraw) >> 16) - 4685;
}

int SI7021::_getCelsiusPostHumidity(bool *timeout /*= NULL*/, int timeout_ms/*=0*/)
{
    byte tempbytes[2];
    if (!_command(POST_RH_TEMP_READ, tempbytes, timeout_ms)) {
		if (timeout) *timeout = true;
		return 0;
	}
	if (timeout) *timeout = false;
    long tempraw = (long)tempbytes[0] << 8 | tempbytes[1];
    return ((17572 * tempraw) >> 16) - 4685;
}


unsigned int SI7021::getHumidityPercent(bool *timeout /*= NULL*/, int timeout_ms/*=0*/)
{
    byte humbytes[2];
    if (!_command(RH_READ, humbytes, timeout_ms)) {
		if (timeout) *timeout = true;
		return 0;
	}
	if (timeout) *timeout = false;
    long humraw = (long)humbytes[0] << 8 | humbytes[1];
    return ((125 * humraw) >> 16) - 6;
}

unsigned int SI7021::getHumidityBasisPoints(bool *timeout /*= NULL*/, int timeout_ms/*=0*/)
{
    byte humbytes[2];
    if (!_command(RH_READ, humbytes, timeout_ms)) {
		if (timeout) *timeout = true;
		return 0;
	}
	if (timeout) *timeout = false;
    long humraw = (long)humbytes[0] << 8 | humbytes[1];
    return ((12500 * humraw) >> 16) - 600;
}

bool SI7021::_command(byte cmd, byte * buf, int timeout_ms  ) {
    _writeReg(&cmd, sizeof cmd);
#if defined(ARDUINO_ARCH_ESP8266)
    delay(25);
#endif
    return _readReg(buf, 2,timeout_ms);
}

void SI7021::_writeReg(byte * reg, int reglen) {
    Wire.beginTransmission(I2C_ADDR);
    for(int i = 0; i < reglen; i++) {
        reg += i;
        Wire.write(*reg); 
    }
    Wire.endTransmission();
}

bool SI7021::_readReg(byte * reg, int reglen, int timeout_ms) {
    Wire.requestFrom(I2C_ADDR, reglen);
    while(Wire.available() < reglen) {
		if (timeout_ms) 
		{
			if (!--timeout_ms)
				return false;
			delay(1);
		}
    }
    for(int i = 0; i < reglen; i++) { 
        reg[i] = Wire.read(); 
    }
    return true;
}

bool SI7021::getSerialBytes(byte * buf,int timeout_ms) {
	byte serial[8];
	_writeReg(SERIAL1_READ, sizeof SERIAL1_READ);
	if (!_readReg(serial, 8, timeout_ms))  return false;
	
	//Page23 - https://www.silabs.com/Support%20Documents%2FTechnicalDocs%2FSi7021-A20.pdf
	buf[0] = serial[0]; //SNA_3
	buf[1] = serial[2]; //SNA_2
	buf[2] = serial[4]; //SNA_1
	buf[3] = serial[6]; //SNA_0

	_writeReg(SERIAL2_READ, sizeof SERIAL2_READ);
	if (!_readReg(serial, 6)) return false;
	buf[4] = serial[0]; //SNB_3 - device ID byte
	buf[5] = serial[1]; //SNB_2
	buf[6] = serial[3]; //SNB_1
	buf[7] = serial[4]; //SNB_0
	return true;
}

uint8_t SI7021::getDeviceId(int timeout_ms /*= 0*/) {
	//0x0D=13=Si7013
	//0x14=20=Si7020
	//0x15=21=Si7021
	byte serial[8];
	getSerialBytes(serial);
	uint8_t id = serial[4];
	return id;
}


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
