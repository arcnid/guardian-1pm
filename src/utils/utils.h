#ifndef UTILS_H
#define UTILS_H

#include <ESP8266WebServer.h>

extern const int SHELLY_BUILTIN_LED;

//utility functions go here
void sendResponse(ESP8266WebServer &server, int statusCode, const String &content);
void flashLED();
bool connect(String ssid, String password);

#endif