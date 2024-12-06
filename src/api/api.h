#ifndef API_H
#define API_H

#include <ESPAsyncWebServer.h>
#include "../utils/utils.h"
#include <ArduinoJson.h>

//utility functions go here
void setupApiRoutes(AsyncWebServer &server);

#endif