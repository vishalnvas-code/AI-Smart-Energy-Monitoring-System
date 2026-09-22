import paho.mqtt.client as mqtt
import requests
import json
import sys

# ==========================================
# CONFIGURATION
# ==========================================
MQTT_BROKER = "10.33.192.245"
MQTT_PORT = 1883
TOPIC_TELEMETRY = "smartev/telemetry"
TOPIC_COMMAND = "smartev/command"
API_URL = "http://127.0.0.1:8000/api/v1/predict"

# ==========================================
# MQTT CALLBACKS
# ==========================================
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ Bridge connected to MQTT Broker!")
        client.subscribe(TOPIC_TELEMETRY)
        print(f"👂 Listening for ESP32 data on topic: {TOPIC_TELEMETRY}")
    else:
        print(f"❌ Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    try:
        # 1. Parse incoming ESP32 data
        payload = json.loads(msg.payload.decode())
        print(f"\n📥 [ESP32] V={payload['voltage']}V | I={payload['current']}A | P={payload['power']}W | Bat={payload['battery_level']}%")

        # 2. Send to FastAPI for AI Prediction
        response = requests.post(API_URL, json=payload, timeout=2)
        if response.status_code == 200:
            ai_result = response.json()

            # 3. Display AI Insights
            print(f"🧠 [AI] Load: {ai_result['load_type_label']} | Next Power: {ai_result['predicted_power_next']}W")

            # 4. ANOMALY DETECTION -> AUTO TRIP RELAYS
            if ai_result['is_anomaly']:
                print(f"🚨 [AI] ANOMALY DETECTED! (Confidence: {ai_result['anomaly_confidence']})")
                print("⚡ [ACTION] Tripping relays to protect the system...")

                cmd1 = json.dumps({"relay": 1, "action": "off"})
                cmd2 = json.dumps({"relay": 2, "action": "off"})
                client.publish(TOPIC_COMMAND, cmd1)
                client.publish(TOPIC_COMMAND, cmd2)
            else:
                print(f"✅ [AI] System Normal.")
        else:
            print(f"❌ API Error: {response.status_code}")

    except Exception as e:
        print(f"⚠️ Bridge Error: {e}")

# ==========================================
# MAIN
# ==========================================
if __name__ == "__main__":
    print("🚀 Starting MQTT-to-AI Bridge...")
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_forever()
    except KeyboardInterrupt:
        print("\n🛑 Bridge stopped.")
        sys.exit(0)
