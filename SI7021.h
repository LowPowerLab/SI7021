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
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
 #include <Wire.h>
#else
 #if (defined(__AVR__)) || defined(ARDUINO_ARCH_NRF5)
  #include <avr/pgmspace.h>
 #else
  #include <pgmspace.h>
 #endif
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
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    bool begin(int SDA, int SCL);
#else
    bool begin();
#endif
    bool sensorExists();
    int getFahrenheitHundredths();
    int getCelsiusHundredths();
    unsigned int getHumidityPercent();
    unsigned int getHumidityBasisPoints();
    struct si7021_env getHumidityAndTemperature();
    struct si7021_thc getTempAndRH();
    int getSerialBytes(byte * buf);
    int getDeviceId();
    void setPrecision(byte setting);
    void setHeater(bool on);
  private:
    void _command(byte cmd, byte * buf );
    void _writeReg(byte * reg, int reglen);
    int _readReg(byte * reg, int reglen);
    int _getCelsiusPostHumidity();
};

#endif

