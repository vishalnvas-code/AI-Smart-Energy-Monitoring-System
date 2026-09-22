#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ==========================================
// 1. NETWORK & MQTT CONFIGURATION
// ==========================================
const char* ssid = "IOT";
const char* password = "012345678";

// ⚠️ CRITICAL: Changed from 110 to 10. Verify this matches your PC's exact IPv4 Address!
const char* mqtt_server = "10.12.59.245"; 
const int mqtt_port = 1883;
const char* mqtt_user = ""; // Leave blank if no auth
const char* mqtt_pass = "";

// MQTT Topics
const char* topic_telemetry = "smartev/telemetry"; // ESP32 publishes sensor data here
const char* topic_command = "smartev/command";     // ESP32 subscribes to relay commands here

// ==========================================
// 2. HARDWARE PINS & CONSTANTS
// ==========================================
#define RELAY1_PIN 26
#define RELAY2_PIN 27
#define VOLTAGE_PIN 34
#define CURRENT_PIN 35

const float VOLTAGE_REFERENCE = 3.3;
const float VOLTAGE_DIVIDER_RATIO = 5.0;
const float CURRENT_OFFSET = 2.5;
const float CURRENT_SENSITIVITY = 0.185;
const int ADC_RESOLUTION = 4095;

// ==========================================
// 3. GLOBAL VARIABLES
// ==========================================
WiFiClient espClient;
PubSubClient client(espClient);
IPAddress serverIP; // Added for the ping test

bool relay1State = false;
bool relay2State = false;
float voltage = 0.0, current = 0.0, power = 0.0;
float batteryLevel = 100.0; // Simulated battery

unsigned long lastMsgTime = 0;
const long MSG_INTERVAL = 2000; // Send data every 2 seconds

// ==========================================
// 4. SENSOR FUNCTIONS
// ==========================================
float readVoltage() {
  int adc = analogRead(VOLTAGE_PIN);
  return ((adc / (float)ADC_RESOLUTION) * VOLTAGE_REFERENCE) * VOLTAGE_DIVIDER_RATIO;
}

float readCurrent() {
  int adc = analogRead(CURRENT_PIN);
  float vRaw = (adc / (float)ADC_RESOLUTION) * VOLTAGE_REFERENCE;
  float val = (vRaw - CURRENT_OFFSET) / CURRENT_SENSITIVITY;
  return val < 0.05 ? 0.0 : val; // Filter noise
}

void updateBatterySimulation() {
  if (power > 1000) batteryLevel -= 0.05;
  else if (power > 100) batteryLevel -= 0.01;
  else batteryLevel += 0.02; // Solar charging simulation
  
  if (batteryLevel > 100.0) batteryLevel = 100.0;
  if (batteryLevel < 0.0) batteryLevel = 0.0;
}

// ==========================================
// 5. MQTT CALLBACK (Receive Commands)
// ==========================================
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  
  Serial.print("MQTT Command Received: ");
  Serial.println(message);

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error) return;

  int relay = doc["relay"];
  String action = doc["action"].as<String>();

  if (relay == 1) {
    if (action == "on") { digitalWrite(RELAY1_PIN, LOW); relay1State = true; }
    else { digitalWrite(RELAY1_PIN, HIGH); relay1State = false; }
  } 
  else if (relay == 2) {
    if (action == "on") { digitalWrite(RELAY2_PIN, LOW); relay2State = true; }
    else { digitalWrite(RELAY2_PIN, HIGH); relay2State = false; }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32_SmartEV", mqtt_user, mqtt_pass)) {
      Serial.println("Connected!");
      client.subscribe(topic_command);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

// ==========================================
// 6. SETUP & LOOP
// ==========================================
void setup() {
  Serial.begin(115200);
  
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, HIGH); // OFF
  digitalWrite(RELAY2_PIN, HIGH); // OFF
  
  analogReadResolution(12);

  // 1. Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  Serial.println("\n✅ WiFi Connected. ESP32 IP: " + WiFi.localIP().toString());

  // 2. 🚨 DIAGNOSTIC PING TEST 🚨
  Serial.print("🔍 Pinging PC at ");
  Serial.print(mqtt_server);
  Serial.print("... ");
  
  if (WiFi.hostByName(mqtt_server, serverIP)) {
    Serial.println("PC is REACHABLE! (Resolved to: " + serverIP.toString() + ")");
  } else {
    Serial.println("❌ PC is BLOCKED or UNREACHABLE!");
    Serial.println("   ➡️ Action: Turn off Windows Firewall or check Router AP Isolation.");
  }

  // 3. Setup MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();
  if (now - lastMsgTime > MSG_INTERVAL) {
    lastMsgTime = now;

    // Read Sensors
    voltage = readVoltage();
    current = readCurrent();
    power = voltage * current;
    updateBatterySimulation();

    int hourOfDay = (millis() / 3600000) % 24; 

    // Create JSON Payload
    StaticJsonDocument<256> jsonDoc;
    jsonDoc["device_id"] = "esp32_01";
    jsonDoc["voltage"] = round(voltage * 100.0) / 100.0;
    jsonDoc["current"] = round(current * 1000.0) / 1000.0;
    jsonDoc["power"] = round(power * 100.0) / 100.0;
    jsonDoc["relay_1"] = relay1State ? 1 : 0;
    jsonDoc["relay_2"] = relay2State ? 1 : 0;
    jsonDoc["battery_level"] = round(batteryLevel * 10.0) / 10.0;
    jsonDoc["hour_of_day"] = hourOfDay;

    char jsonBuffer[256];
    serializeJson(jsonDoc, jsonBuffer);
    
    client.publish(topic_telemetry, jsonBuffer);
    Serial.println("📤 Published: " + String(jsonBuffer));
  }
}