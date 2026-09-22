#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// Wi-Fi credentials
const char* ssid = "IOT";
const char* password = "012345678";

// Pin Definitions
#define RELAY1_PIN 26
#define RELAY2_PIN 27
#define VOLTAGE_PIN 34  // ADC pin for voltage sensor
#define CURRENT_PIN 35  // ADC pin for current sensor

// Calibration constants
const float VOLTAGE_REFERENCE = 3.3;    // ESP32 ADC reference voltage
const float VOLTAGE_DIVIDER_RATIO = 5.0; // Adjust based on your voltage divider
const float CURRENT_OFFSET = 2.5;        // ACS712 midpoint (VCC/2)
const float CURRENT_SENSITIVITY = 0.185; // ACS712 5A: 185mV/A, 20A: 100mV/A, 30A: 66mV/A

// ADC resolution (12-bit = 0-4095)
const int ADC_RESOLUTION = 4095;

// Global variables
WebServer server(80);
bool relay1State = false;
bool relay2State = false;
float voltage = 0.0;
float current = 0.0;
float power = 0.0;

// HTML Webpage - Colorful Design
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Relay Control</title>
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }

    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 50%, #f093fb 100%);
      min-height: 100vh;
      padding: 20px;
      animation: gradientShift 10s ease infinite;
      background-size: 200% 200%;
    }

    @keyframes gradientShift {
      0% { background-position: 0% 50%; }
      50% { background-position: 100% 50%; }
      100% { background-position: 0% 50%; }
    }

    .container {
      max-width: 900px;
      margin: 0 auto;
      background: rgba(255, 255, 255, 0.95);
      padding: 40px;
      border-radius: 20px;
      box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
      backdrop-filter: blur(10px);
    }

    .header {
      text-align: center;
      margin-bottom: 40px;
    }

    h1 {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      background-clip: text;
      font-size: 48px;
      margin-bottom: 10px;
      font-weight: 700;
    }

    .subtitle {
      color: #666;
      font-size: 16px;
      font-weight: 300;
    }

    .relay-section {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
      gap: 30px;
      margin: 40px 0;
    }

    .relay-card {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      padding: 35px;
      border-radius: 15px;
      text-align: center;
      color: white;
      box-shadow: 0 10px 30px rgba(102, 126, 234, 0.4);
      transition: all 0.3s ease;
      position: relative;
      overflow: hidden;
    }

    .relay-card:hover {
      transform: translateY(-5px);
      box-shadow: 0 15px 40px rgba(102, 126, 234, 0.6);
    }

    .relay-card.relay-2 {
      background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
      box-shadow: 0 10px 30px rgba(245, 87, 108, 0.4);
    }

    .relay-card.relay-2:hover {
      box-shadow: 0 15px 40px rgba(245, 87, 108, 0.6);
    }

    .relay-card h2 {
      margin: 0 0 20px 0;
      font-size: 28px;
      font-weight: 600;
    }

    .relay-icon {
      font-size: 48px;
      margin-bottom: 15px;
      animation: pulse 2s ease-in-out infinite;
    }

    @keyframes pulse {
      0%, 100% { transform: scale(1); }
      50% { transform: scale(1.1); }
    }

    .status {
      display: inline-block;
      padding: 12px 35px;
      border-radius: 30px;
      margin: 20px 0;
      font-weight: bold;
      font-size: 18px;
      background: rgba(255, 255, 255, 0.2);
      color: white;
      border: 2px solid rgba(255, 255, 255, 0.3);
      transition: all 0.3s ease;
    }

    .status-on {
      background: rgba(76, 175, 80, 0.3);
      border-color: rgba(76, 175, 80, 0.8);
      box-shadow: 0 0 15px rgba(76, 175, 80, 0.5);
    }

    .status-off {
      background: rgba(244, 67, 54, 0.3);
      border-color: rgba(244, 67, 54, 0.8);
      box-shadow: 0 0 15px rgba(244, 67, 54, 0.5);
    }

    .btn-group {
      display: flex;
      gap: 15px;
      justify-content: center;
      flex-wrap: wrap;
      margin-top: 20px;
    }

    .btn {
      padding: 14px 35px;
      font-size: 16px;
      font-weight: 600;
      border: none;
      border-radius: 10px;
      cursor: pointer;
      transition: all 0.3s ease;
      text-transform: uppercase;
      letter-spacing: 1px;
      min-width: 110px;
      box-shadow: 0 5px 15px rgba(0, 0, 0, 0.2);
      color: white;
    }

    .btn-on {
      background: linear-gradient(135deg, #4CAF50 0%, #66BB6A 100%);
    }

    .btn-on:hover {
      transform: translateY(-2px);
      box-shadow: 0 8px 20px rgba(76, 175, 80, 0.4);
    }

    .btn-on:active {
      transform: translateY(0);
      box-shadow: 0 3px 10px rgba(76, 175, 80, 0.3);
    }

    .btn-off {
      background: linear-gradient(135deg, #f44336 0%, #ef5350 100%);
    }

    .btn-off:hover {
      transform: translateY(-2px);
      box-shadow: 0 8px 20px rgba(244, 67, 54, 0.4);
    }

    .btn-off:active {
      transform: translateY(0);
      box-shadow: 0 3px 10px rgba(244, 67, 54, 0.3);
    }

    .sensor-data {
      margin-top: 40px;
      padding: 30px;
      background: linear-gradient(135deg, rgba(102, 126, 234, 0.1) 0%, rgba(245, 87, 108, 0.1) 100%);
      border-radius: 15px;
      border: 2px solid rgba(102, 126, 234, 0.2);
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
      gap: 20px;
    }

    .sensor-item {
      text-align: center;
      padding: 20px;
      background: white;
      border-radius: 12px;
      box-shadow: 0 5px 15px rgba(0, 0, 0, 0.1);
      transition: all 0.3s ease;
    }

    .sensor-item:hover {
      transform: translateY(-3px);
      box-shadow: 0 8px 20px rgba(0, 0, 0, 0.15);
    }

    .sensor-item .label {
      font-size: 14px;
      color: #999;
      text-transform: uppercase;
      letter-spacing: 1px;
      font-weight: 600;
      margin-bottom: 10px;
    }

    .sensor-item .value {
      font-size: 26px;
      font-weight: bold;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      background-clip: text;
    }

    .sensor-item .icon {
      font-size: 32px;
      margin-bottom: 8px;
    }

    .footer {
      text-align: center;
      margin-top: 30px;
      color: #999;
      font-size: 12px;
    }

    @media (max-width: 600px) {
      .container {
        padding: 20px;
      }

      h1 {
        font-size: 32px;
      }

      .relay-card {
        padding: 25px;
      }

      .relay-card h2 {
        font-size: 22px;
      }

      .btn-group {
        flex-direction: column;
        align-items: center;
      }

      .btn {
        width: 100%;
        max-width: 200px;
      }

      .sensor-data {
        grid-template-columns: 1fr;
      }

      .relay-icon {
        font-size: 36px;
      }
    }

    .wifi-status {
      text-align: center;
      padding: 10px 20px;
      background: linear-gradient(135deg, #4CAF50 0%, #66BB6A 100%);
      color: white;
      border-radius: 10px;
      margin-bottom: 20px;
      font-size: 14px;
      font-weight: 600;
    }

    .wifi-status::before {
      content: " ";
      animation: blink 1s infinite;
    }

    @keyframes blink {
      0%, 50%, 100% { opacity: 1; }
      25%, 75% { opacity: 0.3; }
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="wifi-status">Connected & Ready</div>

    <div class="header">
      <h1>SmartEV Charging and DisCharging Station </h1>
      <p class="subtitle">V2G Grid Stabilization</p>
    </div>

    <div class="relay-section">
      <!-- Relay 1 -->
      <div class="relay-card">
        <div class="relay-icon"></div>
        <h2>DisCharging</h2>
        <div id="status1" class="status status-off">OFF</div>
        <div class="btn-group">
          <button class="btn btn-on" onclick="controlRelay(1, 'on')">ON</button>
          <button class="btn btn-off" onclick="controlRelay(1, 'off')">OFF</button>
        </div>
      </div>

      <!-- Relay 2 -->
      <div class="relay-card relay-2">
        <div class="relay-icon"></div>
        <h2>Charging</h2>
        <div id="status2" class="status status-off">OFF</div>
        <div class="btn-group">
          <button class="btn btn-on" onclick="controlRelay(2, 'on')">ON</button>
          <button class="btn btn-off" onclick="controlRelay(2, 'off')">OFF</button>
        </div>
      </div>
    </div>

    <!-- Sensor Data Display -->
    <div class="sensor-data">
      <div class="sensor-item">
        <div class="icon"></div>
        <div class="label">Voltage</div>
        <div class="value" id="voltage">0.00 V</div>
      </div>
      <div class="sensor-item">
        <div class="icon"></div>
        <div class="label">Current</div>
        <div class="value" id="current">0.00 A</div>
      </div>
      <div class="sensor-item">
        <div class="icon"></div>
        <div class="label">Power</div>
        <div class="value" id="power">0.00 W</div>
      </div>
    </div>

    <div class="footer">
      <p>Last updated: <span id="timestamp">Just now</span></p>
    </div>
  </div>

  <script>
    function controlRelay(relay, action) {
      fetch('/relay?relay=' + relay + '&action=' + action)
        .then(response => response.json())
        .then(data => {
          updateRelayStatus(relay, data.state);
          updateData();
        })
        .catch(error => console.error('Error:', error));
    }

    function updateRelayStatus(relay, state) {
      const statusElement = document.getElementById('status' + relay);
      statusElement.textContent = state ? 'ON' : 'OFF';
      statusElement.className = 'status ' + (state ? 'status-on' : 'status-off');
    }

    function updateData() {
      fetch('/readings')
        .then(response => response.json())
        .then(data => {
          document.getElementById('voltage').textContent = data.voltage.toFixed(2) + ' V';
          document.getElementById('current').textContent = data.current.toFixed(3) + ' A';
          document.getElementById('power').textContent = data.power.toFixed(2) + ' W';
          updateTimestamp();
        })
        .catch(error => console.error('Error:', error));
    }

    function updateTimestamp() {
      const now = new Date();
      const timeStr = now.toLocaleTimeString();
      document.getElementById('timestamp').textContent = timeStr;
    }

    // Auto-refresh every 2 seconds
    setInterval(updateData, 2000);
    updateData();
  </script>
</body>
</html>
)rawliteral";

// Function to read voltage
float readVoltage() {
  int adcValue = analogRead(VOLTAGE_PIN);
  float voltageRaw = (adcValue / (float)ADC_RESOLUTION) * VOLTAGE_REFERENCE;
  float voltageActual = voltageRaw * VOLTAGE_DIVIDER_RATIO;
  return voltageActual;
}

// Function to read current
float readCurrent() {
  int adcValue = analogRead(CURRENT_PIN);
  float voltageRaw = (adcValue / (float)ADC_RESOLUTION) * VOLTAGE_REFERENCE;
  float currentValue = (voltageRaw - CURRENT_OFFSET) / CURRENT_SENSITIVITY;
  return currentValue;
}

// Handle root webpage
void handleRoot() {
  server.send(200, "text/html", index_html);
}

// Handle relay control - FIXED LOGIC
void handleRelay() {
  String relay = server.arg("relay");
  String action = server.arg("action");

  bool state = false;

  if (relay == "1") {
    if (action == "on") {
      digitalWrite(RELAY1_PIN, LOW);   // CHANGED: LOW turns relay ON (active LOW)
      relay1State = true;
      state = true;
    } else if (action == "off") {
      digitalWrite(RELAY1_PIN, HIGH);  // CHANGED: HIGH turns relay OFF (active LOW)
      relay1State = false;
      state = false;
    }

    StaticJsonDocument<100> jsonDoc;
    jsonDoc["state"] = state;
    String response;
    serializeJson(jsonDoc, response);
    server.send(200, "application/json", response);

  } else if (relay == "2") {
    if (action == "on") {
      digitalWrite(RELAY2_PIN, LOW);   // CHANGED: LOW turns relay ON (active LOW)
      relay2State = true;
      state = true;
    } else if (action == "off") {
      digitalWrite(RELAY2_PIN, HIGH);  // CHANGED: HIGH turns relay OFF (active LOW)
      relay2State = false;
      state = false;
    }

    StaticJsonDocument<100> jsonDoc;
    jsonDoc["state"] = state;
    String response;
    serializeJson(jsonDoc, response);
    server.send(200, "application/json", response);
  }
}

// Handle sensor readings
void handleReadings() {
  voltage = readVoltage();
  current = readCurrent();
  power = voltage * current;

  // Filter out negative values for current
  if (current < 0.01) current = 0.0;

  StaticJsonDocument<200> jsonDoc;
  jsonDoc["voltage"] = voltage;
  jsonDoc["current"] = current;
  jsonDoc["power"] = power;

  String response;
  serializeJson(jsonDoc, response);
  server.send(200, "application/json", response);
}

void setup() {
  Serial.begin(115200);

  // Initialize pins
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(VOLTAGE_PIN, INPUT);
  pinMode(CURRENT_PIN, INPUT);

  // Set initial relay states - RELAYS OFF (HIGH for active LOW relays)
  digitalWrite(RELAY1_PIN, HIGH);   // CHANGED: HIGH = OFF for active LOW relays
  digitalWrite(RELAY2_PIN, HIGH);   // CHANGED: HIGH = OFF for active LOW relays

  // Connect to Wi-Fi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Configure ADC for better accuracy
  analogReadResolution(12);
  analogSetWidth(12);

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/relay", handleRelay);
  server.on("/readings", handleReadings);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  // Update readings periodically
  static unsigned long lastReadTime = 0;
  if (millis() - lastReadTime > 1000) {
    voltage = readVoltage();
    current = readCurrent();
    power = voltage * current;
    if (current < 0.01) current = 0.0;
    lastReadTime = millis();
  }
}