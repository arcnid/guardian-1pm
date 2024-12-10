#include "api.h"
#include "../utils/utils.h"
#include <ArduinoJson.h> // Include ArduinoJson library

#define SHELLY_BUILTIN_LED 0

// Wi-Fi scan results
String scanResults;
bool scanCompleted = false;




void setupApiRoutes(ESP8266WebServer &server) {
    server.on("/", HTTP_GET, [&server]() {
        Serial.println("Hit /");
        sendResponse(server, 200, "{\"status\":\"success\"}");
    });

    server.on("/led/on", HTTP_GET, [&server]() {
        digitalWrite(SHELLY_BUILTIN_LED, LOW);
        sendResponse(server, 200, "{\"status\":\"LED turned on\"}");
    });

    server.on("/led/off", HTTP_GET, [&server]() {
        digitalWrite(SHELLY_BUILTIN_LED, HIGH);
        sendResponse(server, 200, "{\"status\":\"LED turned off\"}");
    });

    server.on("/scanWifi", HTTP_GET, [&server]() {
        Serial.println("Starting Wi-Fi scan...");
        
        int n = WiFi.scanNetworks();
        if (n == -1) {
            sendResponse(server, 500, "{\"error\":\"Scan failed\"}");
            return;
        }

        String jsonResponse = "{\"networks\":[";
        for (int i = 0; i < n; i++) {
            jsonResponse += "{\"ssid\":\"" + WiFi.SSID(i) +
                            "\",\"rssi\":" + String(WiFi.RSSI(i)) +
                            ",\"channel\":" + String(WiFi.channel(i)) + "}";
            if (i < n - 1) jsonResponse += ",";
        }
        jsonResponse += "]}";

        sendResponse(server, 200, jsonResponse);
        Serial.println("Wi-Fi scan response sent");
    });

    server.on("/connect", HTTP_POST, [&server]() {
        if (server.args() == 0) {
            sendResponse(server, 400, "{\"status\":\"Error\", \"message\":\"Invalid request\"}");
            return;
        }

        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));

        if (error) {
            sendResponse(server, 400, "{\"status\":\"Error\", \"message\":\"Invalid JSON payload\"}");
            return;
        }

        String ssid = doc["ssid"] | "";
        String password = doc["password"] | "";

        if (!ssid.isEmpty() && !password.isEmpty()) {
            if (connect(ssid, password)) {
                WiFi.softAPdisconnect(true);
                sendResponse(server, 200, "{\"status\":\"Success\", \"message\":\"Connected to Wi-Fi\"}");
            } else {
                sendResponse(server, 400, "{\"status\":\"Error\", \"message\":\"Failed to connect to Wi-Fi\"}");
            }
        } else {
            sendResponse(server, 400, "{\"status\":\"Error\", \"message\":\"Missing 'ssid' or 'password'\"}");
        }
    });

    server.onNotFound([&server]() {
        sendResponse(server, 404, "{\"status\":\"Error\", \"message\":\"Not Found\"}");
    });
}
