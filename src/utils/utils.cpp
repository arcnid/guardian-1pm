#include "utils.h"
#include <Arduino.h>

#define SHELLY_BUILTIN_LED 0

void sendResponse(AsyncWebServerRequest *request, int statusCode, const String &content) {
    AsyncWebServerResponse *response = request->beginResponse(statusCode, "application/json", content);
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    request->send(response);
}

// Function to flash the LED when connected
void flashLED() {
  digitalWrite(SHELLY_BUILTIN_LED, LOW);  // Turn off the LED
  delay(500);
  digitalWrite(SHELLY_BUILTIN_LED, HIGH);  // Turn on the LED
  delay(500);
}


bool connect(String ssid, String password) {
  Serial.println("Attempting to connect to WiFi...");

  // Turn on the LED (solid, indicating connection attempt)
  digitalWrite(SHELLY_BUILTIN_LED, HIGH);

  // Start WiFi connection
  WiFi.begin(ssid.c_str(), password.c_str());

  // Wait until connected or timeout (10 seconds)
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  // Check connection status
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    return true;  // Connection successful
  } else {
    Serial.println("\nFailed to connect to WiFi.");
    return false;  // Connection failed
  }
}