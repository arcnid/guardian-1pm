#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <PubSubClient.h>


#include <api/api.h>
#include <utils/utils.h>

// Web server
ESP8266WebServer server(80);



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
  WiFi.mode(WIFI_AP_STA);
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

  EEPROM.begin(512);


  clearEEPROM();

  

  // Set the LED pin to output mode
  pinMode(SHELLY_BUILTIN_LED, OUTPUT);

  // Call the connect function

  if(checkForWifiAndUser()){
    //attempt to connect using EEPROm credentials
    if(connect(storedConfig.ssid, storedConfig.password)){
      Serial.println("Connected to WiFi successfully.");
      digitalWrite(SHELLY_BUILTIN_LED, HIGH);
    } else{
      startAccessPoint();
    }
  } else{
    Serial.println("No Creds Found, Wifi Started...");
    startAccessPoint();
  }

  setupApiRoutes(server);
 
  server.begin();
  Serial.println("Web Server Started");
  
}

void loop() {
 

  server.handleClient();  
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






