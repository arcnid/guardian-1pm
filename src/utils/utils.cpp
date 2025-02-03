// utils.cpp
#include "utils.h"
#include "secrets.h" 
#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <string.h> 
#include <stdlib.h>
#include <cmath>
#include <ArduinoJson.h>

// Global Variables
Config storedConfig;
#define RELAY_CONTROL 15
bool relayState = false; // true = ON, false = OFF


String getPubTopic(){
  String userId = getUserId();
  String deviceId = getDeviceId();

  Serial.println("Getting pub topic String");
  Serial.println(userId);


  String topic = "/toDaemon/" + userId + "/" + deviceId;

  Serial.println(topic);

  return topic;

}

String getSubTopic(){
  String userId = getUserId();
  String deviceId = getDeviceId();

  String topic = "/toDevice/" + userId + "/" + deviceId;

  Serial.println("Getting sub topic String");
  Serial.println(userId);

  Serial.println(topic);


  return topic;

}



const char* mqtt_publish_topic    = getPubTopic().c_str();
const char* mqtt_subscribe_topic  = getSubTopic().c_str();

// Initialize MQTT Client with a plain WiFiClient
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Define the global state variable
bool doesUserExist = false;

// Calculate Checksum for Data Integrity
uint32_t calculateChecksum(const uint8_t* data, size_t length) {
  uint32_t checksum = 0;
  for (size_t i = 0; i < length; ++i) {
    checksum += data[i];
  }
  return checksum;
}

// Save User and WiFi Credentials to EEPROM
bool saveUserAndWifiCreds(const String& ssid, const String& password, const String& uuid, const String& deviceId) {
  // Clear the Config struct
  memset(&storedConfig, 0, sizeof(Config));

  // Copy SSID, password, UUID, and DeviceID into the struct
  ssid.toCharArray(storedConfig.ssid, MAX_SSID_LENGTH);
  password.toCharArray(storedConfig.password, MAX_PASSWORD_LENGTH);
  uuid.toCharArray(storedConfig.uuid, UUID_LENGTH);
  deviceId.toCharArray(storedConfig.deviceId, DEVICEID_LENGTH);

  // Ensure null-termination (just to be safe)
  storedConfig.ssid[MAX_SSID_LENGTH - 1] = '\0';
  storedConfig.password[MAX_PASSWORD_LENGTH - 1] = '\0';
  storedConfig.uuid[UUID_LENGTH - 1] = '\0';
  storedConfig.deviceId[DEVICEID_LENGTH - 1] = '\0';

  // ✅ Verify each field after copying
  if (String(storedConfig.ssid) != ssid) {
    Serial.println("[ERROR] SSID mismatch after copying to storedConfig.");
    return false;
  }

  if (String(storedConfig.password) != password) {
    Serial.println("[ERROR] Password mismatch after copying to storedConfig.");
    return false;
  }

  if (String(storedConfig.uuid) != uuid) {
    Serial.println("[ERROR] UUID mismatch after copying to storedConfig.");
    return false;
  }

  if (String(storedConfig.deviceId) != deviceId) {
    Serial.println("[ERROR] Device ID mismatch after copying to storedConfig.");
    return false;
  }

  Serial.println("[SUCCESS] All fields verified successfully.");

  // Calculate checksum excluding the checksum field itself
  storedConfig.checksum = 0;
  storedConfig.checksum = calculateChecksum(reinterpret_cast<uint8_t*>(&storedConfig), sizeof(Config) - sizeof(uint32_t));

  // Write the Config struct to EEPROM
  for (size_t i = 0; i < sizeof(Config); ++i) {
      EEPROM.write(i, *((uint8_t*)&storedConfig + i));
  }

  // Commit changes to EEPROM
  if (EEPROM.commit()) {
      Serial.println("[EEPROM] Configuration saved successfully.");
      return true;
  } else {
      Serial.println("[EEPROM] Failed to commit changes.");
      return false;
  }
}

// Check for WiFi and User Configuration in EEPROM
bool checkForWifiAndUser() {
  // Read the Config struct from EEPROM
  for (size_t i = 0; i < sizeof(Config); ++i) {
    *((uint8_t*)&storedConfig + i) = EEPROM.read(i);
  }

  // Ensure null-termination for UUID and DeviceID
  storedConfig.uuid[UUID_LENGTH - 1] = '\0';
  storedConfig.deviceId[DEVICEID_LENGTH - 1] = '\0';
  storedConfig.ssid[MAX_SSID_LENGTH - 1] = '\0';
  storedConfig.password[MAX_PASSWORD_LENGTH - 1] = '\0';

  // Calculate checksum of the read data
  uint32_t calculatedChecksum = calculateChecksum(reinterpret_cast<uint8_t*>(&storedConfig), sizeof(Config) - sizeof(uint32_t));

  // Verify checksum
  if (storedConfig.checksum == calculatedChecksum) {
    // Check if all required fields are non-empty
    if (strlen(storedConfig.ssid) == 0 || strlen(storedConfig.password) == 0 ||
      strlen(storedConfig.uuid) == 0 || strlen(storedConfig.deviceId) == 0) {
      Serial.println("[EEPROM] Configuration found, but one or more fields are empty.");
      doesUserExist = false;
      return false;
    }

    Serial.println("[EEPROM] Valid configuration found.");
    Serial.printf("SSID: %s\n", storedConfig.ssid);
    Serial.printf("UUID: %s, Length: %d\n", storedConfig.uuid, strlen(storedConfig.uuid));
    Serial.printf("DeviceID: %s, Length: %d\n", storedConfig.deviceId, strlen(storedConfig.deviceId));

    doesUserExist = true;
    return true;
  } else {
    Serial.println("[EEPROM] Invalid or no configuration found.");
    doesUserExist = false;
    return false;
  }
}

// Send HTTP Response with CORS Headers
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

// Flash LED (Example Function)
void flashLED() {
  digitalWrite(SHELLY_BUILTIN_LED, LOW);  // Turn LED on
  delay(500);
  digitalWrite(SHELLY_BUILTIN_LED, HIGH); // Turn LED off
  delay(500);
}

// Connect to WiFi with Given Credentials
bool connectToWiFi(const String& ssid, const String& password) {
  Serial.println("Attempting to connect to WiFi...");

  // Turn on the LED (solid, indicating connection attempt)
  digitalWrite(SHELLY_BUILTIN_LED, LOW);

  // Disconnect any previous connection
  WiFi.disconnect();
  delay(100);

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
    digitalWrite(SHELLY_BUILTIN_LED, HIGH);

    doesUserExist = true;
    return true;
  } else {
    Serial.println("\nFailed to connect to WiFi.");
    Serial.printf("WiFi.status(): %d\n", WiFi.status());

    doesUserExist = false;
    return false;
  }
}

// Clear EEPROM (Use with Caution)
void clearEEPROM() {
  EEPROM.begin(512);
  for (int i = 0; i < 512; i++) {
      EEPROM.write(i, 0xFF);
  }
  EEPROM.commit();
  Serial.println("EEPROM cleared.");
  ESP.restart();
}

void restart(){
  ESP.restart();
}

// MQTT Callback Function to Handle Incoming Messages
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  // Convert payload to a string
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message: ");
  Serial.println(message);

  // Parse the JSON message
  StaticJsonDocument<200> doc; // Adjust buffer size as needed

  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    return;
  }

  // Check for the "power" key in JSON
  if (doc.containsKey("power")) {
    bool powerState = doc["power"];
    
    if (powerState) {
      turnRelayOn();
      Serial.println("Relay turned ON via MQTT");
    } else {
      turnRelayOff();
      Serial.println("Relay turned OFF via MQTT");
    }

    publishRelayStateupdate("state_change");
  } else {
    Serial.println("JSON does not contain 'power' key.");
  }
  if(doc.containsKey("command")){
    String command = doc["command"];
    Serial.println("commmand recieved");

    if(command == "restart"){
      restart();
    } 
    else if (command == "clearEEPROM"){
      clearEEPROM();
    }

  }
}

// Connect to MQTT Broker
bool connectToMQTT() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  int randomSessionId = random(1, 201); // Random number from 1 to 200

  String clientId = "ESP8266Client-" + String(WiFi.macAddress()) + "-" + String(storedConfig.deviceId) + "-" + String(randomSessionId);
  String pubTopic = getPubTopic().c_str();
  String subTopic = getSubTopic().c_str();

  bool connected = mqttClient.connect(clientId.c_str());

  Serial.println("about to connect to mqtt broker with this topic");
  Serial.println(mqtt_subscribe_topic);

  if (connected) {
    Serial.println("Connected to MQTT Broker.");
    mqttClient.subscribe(getSubTopic().c_str());
    Serial.print("Subscribed to topic: ");
    Serial.println(getSubTopic().c_str());

    publishRelayStateupdate("init");

    // mqttClient.publish(topic.c_str(), "ESP8266 Connected");
    return true;
  } else {
    Serial.print("Failed to connect to MQTT Broker, state: ");
    Serial.println(mqttClient.state());
    return false;
  }
}

// Publish Message to MQTT Broker
void publishMessage(const char* topic, const char* message) {

  if (mqttClient.publish(topic, message)) {
    Serial.print("Message published to topic ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(message);
  } else {
    Serial.print("Failed to publish message to topic ");
    Serial.println(topic);
  }
}

String getUserId() {
  // Temporary storage for the Config struct
  Config tempConfig;

  // Read data from EEPROM into tempConfig
  for (size_t i = 0; i < sizeof(Config); ++i) {
      *((uint8_t*)&tempConfig + i) = EEPROM.read(i);
  }

  // Ensure null-termination for UUID
  tempConfig.uuid[UUID_LENGTH - 1] = '\0';

  // Calculate checksum
  uint32_t calculatedChecksum = calculateChecksum(reinterpret_cast<uint8_t*>(&tempConfig), sizeof(Config) - sizeof(uint32_t));

  // Verify checksum
  if (tempConfig.checksum == calculatedChecksum) {
    if (strlen(tempConfig.uuid) == 0) {
        Serial.println("[EEPROM] UUID field is empty.");
        return String(""); // Return empty string if UUID is empty
    }

    return String(tempConfig.uuid);
  }

  Serial.println("[EEPROM] Invalid configuration or checksum mismatch while retrieving UUID.");
  return String(""); // Return empty string if checksum fails
}


String getDeviceId() {
  // Temporary storage for the Config struct
  Config tempConfig;

  // Read data from EEPROM into tempConfig
  for (size_t i = 0; i < sizeof(Config); ++i) {
    *((uint8_t*)&tempConfig + i) = EEPROM.read(i);
  }

  // Ensure null-termination for DeviceID
  tempConfig.deviceId[DEVICEID_LENGTH - 1] = '\0';

  // Calculate checksum
  uint32_t calculatedChecksum = calculateChecksum(reinterpret_cast<uint8_t*>(&tempConfig), sizeof(Config) - sizeof(uint32_t));

  // Verify checksum
  if (tempConfig.checksum == calculatedChecksum) {
    if (strlen(tempConfig.deviceId) == 0) {
      Serial.println("[EEPROM] DeviceID field is empty.");
      return String(""); 
    }

    return String(tempConfig.deviceId);
  }

  Serial.println("[EEPROM] Invalid configuration or checksum mismatch while retrieving DeviceID.");
  return String(""); 
}

void initRelay() {
  pinMode(RELAY_CONTROL, OUTPUT);

  // Read last state from EEPROM
  relayState = LOW; // Default to OFF
  // Apply the last state to the relay
  digitalWrite(RELAY_CONTROL, LOW);
}

void toggleRelay() {
  // Toggle state
  relayState = !relayState;

  // Apply state to GPIO4
  digitalWrite(RELAY_CONTROL, relayState ? HIGH : LOW);

  // Persist to EEPROM if required
  
}

void turnRelayOn() {
  relayState = true;
  digitalWrite(RELAY_CONTROL, HIGH);
 
}

void turnRelayOff() {
  relayState = false;
  digitalWrite(RELAY_CONTROL, LOW);
 
}

void publishRelayStateupdate(const char* updateType){
 DynamicJsonDocument doc(256);
  doc["relay_update"] = true;
  doc["update_type"] = updateType;  // "init", "state_change", or "heartbeat"
  doc["relay_state"] = relayState;  // current state: true or false
  doc["device_type"] = "relay";
  doc["status"] = "online";
  // Optionally add a timestamp (here using millis(), or use an NTP timestamp if available)
  doc["timestamp"] = millis();

  char buffer[256];
  serializeJson(doc, buffer, sizeof(buffer));
  
  // Use the dynamic topic based on stored user/device IDs.
  publishMessage(getPubTopic().c_str(), buffer);
}