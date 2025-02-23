#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// Wi-Fi credentials
const char* ssid = "A32";
const char* password = "aryan2663";

// Server URL for GPS data
const char* serverUrl = "http://192.168.241.211:8080/gps";

// TinyGPS++ object for GPS
TinyGPSPlus gps;

// UART2 for GPS (RX=16, TX=17)
HardwareSerial mySerial(2);

// Interval for sending GPS data (30 seconds)
const unsigned long interval = 30000;
unsigned long lastSendTime = 0;

// Unique MAC address key
String uniqueKey;

void setup() {
  Serial.begin(115200);

  // GPS setup
  mySerial.begin(9600, SERIAL_8N1, 16, 17);

  // Wi-Fi setup
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");

  // Get MAC address
  uint8_t mac[6];
  WiFi.macAddress(mac);
  uniqueKey = String(mac[0], HEX) + String(mac[1], HEX) + String(mac[2], HEX) + String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
  uniqueKey.toUpperCase();
  Serial.println("Unique Key (MAC Address): " + uniqueKey);
}

void loop() {
  // GPS data processing
  bool gpsDataReceived = false;
  while (mySerial.available() > 0) {
    char c = mySerial.read();
    gps.encode(c);
    if (gps.location.isUpdated()) {
      gpsDataReceived = true;
    }
  }

  if (gpsDataReceived) {
    Serial.print("GPS Data: ");
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Altitude: ");
    Serial.println(gps.altitude.meters());
    Serial.print("Speed: ");
    Serial.println(gps.speed.kmph());
    Serial.print("Satellites: ");
    Serial.println(gps.satellites.value());
    Serial.print("HDOP: ");
    Serial.println(gps.hdop.value());
  } else {
    Serial.println("No GPS data available or waiting for GPS fix...");
  }

  // Send GPS data at intervals
  if (millis() - lastSendTime >= interval) {
    lastSendTime = millis();

    // Prepare JSON data
    StaticJsonDocument<200> doc;
    if (gps.location.isValid()) {
      doc["latitude"] = gps.location.lat();
      doc["longitude"] = gps.location.lng();
      doc["altitude"] = gps.altitude.meters();
      doc["speed"] = gps.speed.kmph();
      doc["satellites"] = gps.satellites.value();
      doc["hdop"] = gps.hdop.value();
      doc["unique_key"] = uniqueKey;
    } else {
      doc["error"] = "Waiting for GPS fix...";
    }

    String jsonString;
    serializeJson(doc, jsonString);

    // Send data to server
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/json");
      int httpCode = http.POST(jsonString);
      if (httpCode == 200) {
        String payload = http.getString();
        Serial.println("Response from server: " + payload);
      } else {
        Serial.print("Error in HTTP request, Code: ");
        Serial.println(httpCode);
      }
      http.end();
    } else {
      Serial.println("Wi-Fi disconnected. Unable to send data.");
    }
  }
}
