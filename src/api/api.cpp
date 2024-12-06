#include "api.h"
#include "../utils/utils.h"
#include <ArduinoJson.h> // Include ArduinoJson library

#define SHELLY_BUILTIN_LED 0

void setupApiRoutes(AsyncWebServer &server) {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        sendResponse(request, 200, "{\"status\":\"success\"}");
    });

    server.on("/led/on", HTTP_GET, [](AsyncWebServerRequest *request) {
        digitalWrite(SHELLY_BUILTIN_LED, LOW);
        sendResponse(request, 200, "{\"status\":\"LED turned on\"}");
    });

    server.on("/led/off", HTTP_GET, [](AsyncWebServerRequest *request) {
        digitalWrite(SHELLY_BUILTIN_LED, HIGH);
        sendResponse(request, 200, "{\"status\":\"LED turned off\"}");
    });

    server.on("/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<256> doc; // Allocate memory for JSON parsing
        String body;

        // Check if the request has a body
        if (request->hasParam("plain", true)) {
            body = request->getParam("plain", true)->value();
        } else {
            sendResponse(request, 400, "{\"status\":\"Error\", \"message\":\"No JSON payload received\"}");
            return;
        }

        // Parse the JSON payload
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            sendResponse(request, 400, "{\"status\":\"Error\", \"message\":\"Invalid JSON payload\"}");
            return;
        }

        // Extract the "ssid" and "password" fields
        String ssid = doc["ssid"] | "";
        String password = doc["password"] | "";

        // Validate the extracted values
        if (!ssid.isEmpty() && !password.isEmpty()) {
            if (connect(ssid, password)) {
                WiFi.softAPdisconnect(true);
                sendResponse(request, 200, "{\"status\":\"Success\", \"message\":\"Connected to Wi-Fi\"}");
            } else {
                sendResponse(request, 400, "{\"status\":\"Error\", \"message\":\"Failed to connect to Wi-Fi\"}");
            }
        } else {
            sendResponse(request, 400, "{\"status\":\"Error\", \"message\":\"Missing 'ssid' or 'password'\"}");
        }
    });
}
