#ifndef UTILS_H
#define UTILS_H

#include <ESP8266WebServer.h>
#include <PubSubClient.h>




#define MAX_SSID_LENGTH 32
#define MAX_PASSWORD_LENGTH 64
#define UUID_LENGTH 36
#define DEVICEID_LENGTH 36

struct Config {
  char ssid[MAX_SSID_LENGTH];
  char password[MAX_PASSWORD_LENGTH];
  char uuid[UUID_LENGTH];
  char deviceId[DEVICEID_LENGTH];
  uint32_t checksum; // For data integrity
};

extern const int SHELLY_BUILTIN_LED;
extern Config storedConfig;

//utility functions go here
void sendResponse(ESP8266WebServer &server, int statusCode, const String &content);
void flashLED();
bool connect(String ssid, String password);
bool saveUserAndWifiCreds(const String& ssid, const String& password, const String& uuid, const String& deviceId);
bool checkForWifiAndUser();
uint32_t calculateChecksum(const uint8_t* data, size_t length);
void clearEEPROM();

#endif