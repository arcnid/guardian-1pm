#include "utils.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <PubSubClient.h>


#define SHELLY_BUILTIN_LED 0

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];


Config storedConfig;

uint32_t calculateChecksum(const uint8_t* data, size_t length) {
  uint32_t checksum = 0;
  for (size_t i = 0; i < length; ++i) {
    checksum += data[i];
  }
  return checksum;
}

bool saveUserAndWifiCreds(const String& ssid, const String& password, const String& uuid, const String& deviceId) {
  // Clear the Config struct
  memset(&storedConfig, 0, sizeof(Config));

  // Copy SSID, password, and UUID into the struct
  ssid.toCharArray(storedConfig.ssid, MAX_SSID_LENGTH);
  password.toCharArray(storedConfig.password, MAX_PASSWORD_LENGTH);
  uuid.toCharArray(storedConfig.uuid, UUID_LENGTH);
  deviceId.toCharArray(storedConfig.deviceId, DEVICEID_LENGTH);

  // Calculate checksum excluding the checksum field itself
  storedConfig.checksum = 0; // Reset checksum before calculation
  storedConfig.checksum = calculateChecksum(reinterpret_cast<uint8_t*>(&storedConfig), sizeof(Config) - sizeof(uint32_t));

  // Write the Config struct to EEPROM
  for (size_t i = 0; i < sizeof(Config); ++i) {
    EEPROM.write(i, *((uint8_t*)&storedConfig + i));
  }

  // Commit changes to EEPROM
  if (EEPROM.commit()) {
    Serial.println("Configuration saved to EEPROM successfully.");
    return true;
  } else {
    Serial.println("Failed to commit EEPROM changes.");
    return false;
  }
}

bool checkForWifiAndUser() {
  // Read the Config struct from EEPROM
  for (size_t i = 0; i < sizeof(Config); ++i) {
    *((uint8_t*)&storedConfig + i) = EEPROM.read(i);
  }

  // Calculate checksum of the read data
  uint32_t calculatedChecksum = calculateChecksum(reinterpret_cast<uint8_t*>(&storedConfig), sizeof(Config) - sizeof(uint32_t));

  // Verify checksum
  if (storedConfig.checksum == calculatedChecksum) {
    // Check if all required fields are non-empty
    if (strlen(storedConfig.ssid) == 0 || strlen(storedConfig.password) == 0 ||
        strlen(storedConfig.uuid) == 0 || strlen(storedConfig.deviceId) == 0) {
      Serial.println("Configuration found but one or more fields are empty.");
      return false;
    }

    Serial.println("Valid configuration found in EEPROM.");
    Serial.printf("SSID: %s\n", storedConfig.ssid);
    Serial.printf("UUID: %s\n", storedConfig.uuid);
    Serial.printf("DeviceID: %s\n", storedConfig.deviceId);

    return true;
  } else {
    Serial.println("Invalid or no configuration found in EEPROM.");
    return false;
  }
}

void sendResponse(ESP8266WebServer &server, int statusCode, const String &content) {
  // Debug output
  Serial.printf("[DEBUG] Sending response with status code %d, content: %s\n", statusCode, content.c_str());

  // Set headers for CORS
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");

  // Send the response
  server.send(statusCode, "application/json", content);
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

    // Disconnect any previous connection
    WiFi.disconnect();
    delay(100);  // Give it some time to reset

    // Start WiFi connection
    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait until connected or timeout (15 seconds)
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
        delay(500);
        Serial.print(".");
        Serial.printf(" Current WiFi.status(): %d\n", WiFi.status());
    }

    // Check connection status
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
        return true;  // Connection successful
    } else {
        Serial.println("\nFailed to connect to WiFi.");
        Serial.printf("WiFi.status(): %d\n", WiFi.status());
        return false;  // Connection failed
    }
}

void clearEEPROM() {
  EEPROM.begin(512); // Adjust size to match your EEPROM size
  for (int i = 0; i < 512; i++) {
    EEPROM.write(i, 0xFF); // Clear each byte (0xFF is the default "erased" value)
  }
  EEPROM.commit();
  Serial.println("EEPROM cleared.");
}

