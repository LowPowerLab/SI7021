/*
  Copyright 2014 Marcus Sorensen <marcus@electron14.com>

This program is licensed, please check with the copyright holder for terms

Updated: Jul 16, 2015: TomWS1: eliminated Byte constants, fixed 'sizeof' error in _command(), added getTempAndRH() function to simplify calls for C & RH only
*/
#ifndef si7021_h
#define si7021_h

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#ifdef __AVR_ATtiny85__
 #include "TinyWireM.h"
 #define Wire TinyWireM
#elif defined(ARDUINO_ARCH_ESP8266)
 #include <Wire.h>
#else
 #include <avr/pgmspace.h>
 #include <Wire.h>
#endif

typedef struct si7021_env {
    int celsiusHundredths;
    int fahrenheitHundredths;
    unsigned int humidityBasisPoints;
} si7021_env;

// same as above but without fahrenheit parameter and RH %
typedef struct si7021_thc {
    int celsiusHundredths;
    unsigned int humidityPercent;
} si7021_thc;

class SI7021
{
  public:
    SI7021();
#if defined(ARDUINO_ARCH_ESP8266)
    bool begin(int SDA, int SCL);
#else
    bool begin();
#endif
    bool sensorExists();
    int getFahrenheitHundredths();
    int getCelsiusHundredths(bool *timeout = NULL, int timeout_ms=0);
    unsigned int getHumidityPercent(bool *timeout = NULL, int timeout_ms=0);
    unsigned int getHumidityBasisPoints(bool *timeout = NULL, int timeout_ms=0);
    struct si7021_env getHumidityAndTemperature();
    struct si7021_thc getTempAndRH();
	/** Return false on timeout */
    bool getSerialBytes(byte * buf, int timeout_ms = 0);

	/*Return: 
	 * 0x0D=13=Si7013
	 * 0x14=20=Si7020
	 * 0x15=21=Si7021
	 */
    uint8_t getDeviceId(int timeout_ms = 0);
    void setHeater(bool on);
  private:
    bool _command(byte cmd, byte * buf, int timeout_ms = 0 );
    void _writeReg(byte * reg, int reglen);
	/** return true on success, false on timeout. If timeout_ms is 0, infinite wait. */
    bool _readReg(byte * reg, int reglen, int timeout_ms=0);
    int _getCelsiusPostHumidity(bool *timeout = NULL, int timeout_ms=0);
};

#endif

