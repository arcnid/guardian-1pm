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

        // Deduplicate networks by ssid
        // We'll keep a record of the first occurrence of each ssid
        // Using a simple approach: a vector to store processed ssids.
        // For efficiency, you could use a std::set<String>.
        std::vector<String> processedSSIDs;

        String jsonResponse = "{\"data\":[";
        bool first = true;

        for (int i = 0; i < n; i++) {
            String currentSSID = WiFi.SSID(i);

            // Check if we've seen this ssid before
            bool alreadySeen = false;
            for (const auto &seenSSID : processedSSIDs) {
                if (seenSSID == currentSSID) {
                    alreadySeen = true;
                    break;
                }
            }

            if (alreadySeen) {
                // Skip duplicates
                continue;
            }

            // Mark this ssid as seen
            processedSSIDs.push_back(currentSSID);

            // Add comma before next entry if not the first
            if (!first) {
                jsonResponse += ",";
            } else {
                first = false;
            }

            jsonResponse += "{\"ssid\":\"" + currentSSID +
                            "\",\"signal_level\":" + String(WiFi.RSSI(i)) +
                            ",\"channel\":" + String(WiFi.channel(i)) +
                            ",\"security\":\"";

            switch (WiFi.encryptionType(i)) {
                case ENC_TYPE_NONE:
                    jsonResponse += "Open";
                    break;
                case ENC_TYPE_WEP:
                    jsonResponse += "WEP";
                    break;
                case ENC_TYPE_TKIP:
                    jsonResponse += "WPA/PSK";
                    break;
                case ENC_TYPE_CCMP:
                    jsonResponse += "WPA2/PSK";
                    break;
                case ENC_TYPE_AUTO:
                    jsonResponse += "Auto";
                    break;
                default:
                    jsonResponse += "Unknown";
                    break;
            }

            jsonResponse += "\"}";
        }

        jsonResponse += "]}";

        sendResponse(server, 200, jsonResponse);
        Serial.println("Wi-Fi scan response sent");
    });

    server.on("/connect", HTTP_POST, [&server]() {
        // Debug: Print the number of arguments
        Serial.printf("Number of arguments: %d\n", server.args());

        // Check if the request has a body
        if (!server.hasArg("plain")) {
            sendResponse(server, 400, "{\"status\":\"Error\", \"message\":\"Missing JSON body\"}");
            Serial.println("Missing JSON body");
            return;
        }

        // Retrieve the raw body content
        String body = server.arg("plain");
        Serial.printf("Request Body: %s\n", body.c_str());

        // Parse JSON payload
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, body);

        if (error) {
            sendResponse(server, 400, "{\"status\":\"Error\", \"message\":\"Invalid JSON payload\"}");
            Serial.printf("JSON Deserialization Error: %s\n", error.c_str());
            return;
        }

        // Extract ssid and password
        String ssid = doc["ssid"] | "";
        String password = doc["password"] | "";
        String userId = doc["userId"] | "";
        String deviceId = doc["deviceId"] | "";
        String brokerAddress = doc["brokerAddress"] | "";

        Serial.printf("Extracted SSID: %s\n", ssid.c_str());
        Serial.printf("Extracted Password: %s\n", password.c_str());
        Serial.printf("Extracted UserID: %s\n", userId.c_str());

        // Validate extracted parameters
        if (ssid.isEmpty() || password.isEmpty()  || userId.isEmpty()) {
            sendResponse(server, 400, "{\"status\":\"Error\", \"message\":\"Missing 'ssid' or 'password'\"}");
            Serial.println("Missing 'ssid' or 'password'");
            return;
        }

        // Attempt to connect to Wi-Fi
        bool isConnected = connect(ssid, password);
        Serial.println("Just got a response back for isConnected");
        Serial.println(isConnected);

        // Based on connection result, send appropriate response
        if (isConnected) {
            // Disconnect AP if connected successfully
            //save wifi and user to EEPROM
            if(saveUserAndWifiCreds(ssid, password, userId, deviceId)){
                Serial.println("User and WIfi has been saved.");

                sendResponse(server, 200, "{\"status\":\"Success\", \"message\":\"Connected to Wi-Fi\", \"deviceType\": \"relay\"}");
                delay(1000);

                WiFi.softAPdisconnect(true);
                Serial.println(1);
                Serial.println("Access Point disconnected after successful connection");
            } else{
                Serial.println("Failed to save configuration to EEPROM");
                sendResponse(server, 500, "{\"status\":\"Error\", \"message\":\"Failed to save configuration\"}");
            
            }
            sendResponse(server, 200, "{\"status\":\"Success\", \"message\":\"Connected to Wi-Fi\"}");

            //initiate database intitialization handshake


            delay(1000);
            Serial.println(2);
            WiFi.softAPdisconnect(true);
            Serial.println("Access Point disconnected after successful connection");
    
            
        } else {
            Serial.println("Connection attempt failed");
            sendResponse(server, 400, "{\"status\":\"Error\", \"message\":\"Failed to connect to Wi-Fi\"}");
            // Optionally, keep the AP active for retrying
        }
    });




    // --------------------------
    // ADD CORS PRE-FLIGHT HANDLERS FOR OPTIONS REQUESTS
    // --------------------------

    server.on("/", HTTP_OPTIONS, [&server]() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        server.send(204);
    });

    server.on("/led/on", HTTP_OPTIONS, [&server]() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        server.send(204);
    });

    server.on("/led/off", HTTP_OPTIONS, [&server]() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        server.send(204);
    });

    server.on("/scanWifi", HTTP_OPTIONS, [&server]() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        server.send(204);
    });

    server.on("/connect", HTTP_OPTIONS, [&server]() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        server.send(204);
    });


    // Existing onNotFound handler (unchanged)
    server.onNotFound([&server]() {
        sendResponse(server, 404, "{\"status\":\"Error\", \"message\":\"Not Found\"}");
    });
}
