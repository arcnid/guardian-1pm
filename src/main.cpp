#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <api/api.h>
#include <utils/utils.h>

// Web server
AsyncWebServer server(80);


// Pin definitions
#define SHELLY_BUILTIN_LED 0

// Function prototypes
void flashLED();
void printIP();

// Timer variables
unsigned long lastPrintTime = 0;  // Last time the IP was printed
const unsigned long printInterval = 5000;  // Interval to print the IP (in ms)

void startAccessPoint(){
  //start the AP
  WiFi.softAP("Guardian_GMS_Relay", "");


  //log out the ip that was assigned to the device
  Serial.print("Access point IP: ");
  Serial.println(WiFi.softAPIP());

  //turn light on
  digitalWrite(SHELLY_BUILTIN_LED, LOW);

}



void setup() {
  // Start the serial communication for debugging
  Serial.begin(115200);
  Serial.print("Firmware Started");
  

  // Set the LED pin to output mode
  pinMode(SHELLY_BUILTIN_LED, OUTPUT);

  // Call the connect function

  setupApiRoutes(server);
 

  

  server.begin();
  Serial.println("Web Server Started");


  startAccessPoint();
}

void loop() {
  // If connected, keep flashing the LED
  // if (WiFi.status() == WL_CONNECTED) {
    

  //   // Print the IP address at intervals
  //   if (millis() - lastPrintTime >= printInterval) {
  //     printIP();
  //     lastPrintTime = millis();  // Update the last print time
  //   }
  // } else{
  //   flashLED();
  // }


  //start the wifi fallback ap 



}

// Function to print the IP address
void printIP() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi.");
  }
}






