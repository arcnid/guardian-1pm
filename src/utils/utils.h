#ifndef UTILS_H
#define UTILS_H

#include <ESPAsyncWebServer.h>

extern const int SHELLY_BUILTIN_LED;

//utility functions go here
void sendResponse(AsyncWebServerRequest *request, int statusCode, const String &content);
void flashLED();
bool connect(String ssid, String password);

#endif