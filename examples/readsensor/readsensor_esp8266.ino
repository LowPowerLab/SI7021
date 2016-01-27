#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <SI7021.h>

const char* ssid = "SSID";
const char* password = "PASSWORD";

#define SDA 0 // GPIO0 on ESP-01 module
#define SCL 2 // GPIO2 on ESP-01 module

ESP8266WiFiMulti wifiMulti;
ESP8266WebServer server(80);
SI7021 sensor;

void handle_root() {
  int temperature = sensor.getCelsiusHundredths();
  int humidity = sensor.getHumidityPercent();
  server.send(200, "text/plain", String() +   F("{\"temperature:\": ") + String(temperature) +  F(", \"humidity:\": ") + String(humidity) + F("}"));
}

void setup() {
  WiFi.mode(WIFI_STA); // Wifi-client
  wifiMulti.addAP(ssid,password);
  
  wifiMulti.run();
  server.on("/", handle_root);
  sensor.begin(SDA,SCL);
}


void loop() {
  if (wifiMulti.run() == WL_CONNECTED) {
    server.handleClient();
  }
}

