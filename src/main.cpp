#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <time.h>

// Include your custom headers
#include "utils/utils.h"
#include "api/api.h"

// Web server on port 80
ESP8266WebServer server(80);

// Timer variables
unsigned long lastPublishTime = 0;
const unsigned long publishInterval = 30000;

unsigned long lastReconnectAttempt = 5000;
const unsigned long reconnectInterval = 10000;

unsigned long lastWifiRetryAttempt = 0;
const unsigned long wifiRetryInterval = 30000;


void startAccessPoint() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("Guardian_GMS_Sensor", "");
  Serial.print("Access point IP: ");
  Serial.println(WiFi.softAPIP());
  digitalWrite(SHELLY_BUILTIN_LED, LOW);
}

void synchronizeTime() {
  Serial.println("Synchronizing system time...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
      Serial.println("Waiting for time synchronization...");
      delay(1000);
  }

  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "Current time: %A, %B %d %Y %H:%M:%S", &timeinfo);
  Serial.println(timeStr);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }
  Serial.println("\nFirmware Started");
  

  EEPROM.begin(512);

  pinMode(SHELLY_BUILTIN_LED, OUTPUT);
  digitalWrite(SHELLY_BUILTIN_LED, HIGH); 
  initRelay();


  if (checkForWifiAndUser()) {
      if (connectToWiFi(String(storedConfig.ssid), String(storedConfig.password))) {
          Serial.println("Connected to WiFi successfully.");
          digitalWrite(SHELLY_BUILTIN_LED, HIGH);
      } else {
          Serial.println("WiFi connection failed, starting Access Point...");
          startAccessPoint();
      }
  } else {
      Serial.println("No Credentials Found, Starting Access Point...");
      startAccessPoint();
  }

  if (WiFi.status() == WL_CONNECTED) {
      synchronizeTime();
  } else {
      Serial.println("WiFi not connected. Unable to synchronize time.");
  }

  setupApiRoutes(server);
  server.begin();
  Serial.println("Web Server Started");

  if (doesUserExist && WiFi.status() == WL_CONNECTED) {
      if (connectToMQTT()) {
          Serial.println("MQTT Connected Successfully.");
      } else {
          Serial.println("Failed to Connect to MQTT Broker.");
      }
  }

  // Initialize I²C
}



void loop() {
  server.handleClient();
  unsigned long currentMillis = millis();

  // WiFi Reconnect
  if (doesUserExist && (WiFi.status() != WL_CONNECTED)) {
      if (currentMillis - lastWifiRetryAttempt > wifiRetryInterval) {
          lastWifiRetryAttempt = currentMillis;
          Serial.println("Attempting to reconnect to WiFi...");
          if (connectToWiFi(String(storedConfig.ssid), String(storedConfig.password))) {
              Serial.println("Reconnected to WiFi successfully.");
              // digitalWrite(SHELLY_BUILTIN_LED, HIGH);
              synchronizeTime();

              if (!mqttClient.connected()) {
                  if (connectToMQTT()) {
                      Serial.println("MQTT Connected Successfully after WiFi reconnect.");
                  } else {
                      Serial.println("Failed to Connect to MQTT Broker after WiFi reconnect.");
                  }
              }
          } else {
              Serial.println("WiFi reconnect attempt failed.");
          }
      }
  }

  // MQTT Reconnect
  if (doesUserExist && (WiFi.status() == WL_CONNECTED)) {
      if (!mqttClient.connected()) {
          if (currentMillis - lastReconnectAttempt > reconnectInterval) {
              lastReconnectAttempt = currentMillis;
              Serial.println("Reconnecting to MQTT Broker...");
              if (connectToMQTT()) {
                  Serial.println("Reconnected to MQTT Broker.");
              } else {
                  Serial.println("Failed to connect to broker");
              }
          }
      }
      mqttClient.loop();

      static unsigned long lastAttempt = 0;
      unsigned long now = millis();
      if (now - lastAttempt >= 60000) { // Every 5 seconds
          lastAttempt = now;
          publishMessage(mqtt_publish_topic, "Hello from ESP8266");
          // readShellyHTData();
      }
      delay(100);

    
  }
  
}
