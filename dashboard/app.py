import streamlit as st
import paho.mqtt.client as mqtt
import json
import queue
import time
import pandas as pd
import plotly.graph_objects as go
from plotly.subplots import make_subplots
from datetime import datetime

# ==========================================
# 1. CONFIGURATION & CONSTANTS
# ==========================================
MQTT_BROKER = "10.12.59.245" 
MQTT_PORT = 1883
TOPIC_TELEMETRY = "smartev/telemetry"
TOPIC_COMMAND = "smartev/command"
API_URL = "http://10.12.59.245:8000/api/v1/predict"

# Simulated Battery Capacity for Timeline Calculations (in Watt-Hours)
BATTERY_CAPACITY_WH = 5000 

# ==========================================
# 2. THREAD-SAFE QUEUE & SESSION STATE
# ==========================================
if 'data_queue' not in st.session_state:
    st.session_state.data_queue = queue.Queue(maxsize=100)
data_queue = st.session_state.data_queue

if 'live_data' not in st.session_state: st.session_state.live_data = None
if 'ai_result' not in st.session_state: st.session_state.ai_result = None
if 'relay1_state' not in st.session_state: st.session_state.relay1_state = False
if 'relay2_state' not in st.session_state: st.session_state.relay2_state = False
if 'mqtt_client' not in st.session_state: st.session_state.mqtt_client = None

# ==========================================
# 3. MQTT BACKGROUND THREAD
# ==========================================
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        client.subscribe(TOPIC_TELEMETRY, qos=1)
    else:
        print(f"❌ MQTT Connection failed with code {rc}")

def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode())
        if not data_queue.full():
            data_queue.put(payload)
    except Exception:
        pass

if st.session_state.mqtt_client is None:
    try:
        try:
            client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, client_id="streamlit_ai_v2")
        except AttributeError:
            client = mqtt.Client(client_id="streamlit_ai_v2")
        client.on_connect = on_connect
        client.on_message = on_message
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()
        st.session_state.mqtt_client = client
    except Exception as e:
        st.error(f"MQTT Error: {e}")

# Drain queue
if not data_queue.empty():
    try:
        payload = data_queue.get_nowait()
        st.session_state.live_data = payload
        try:
            response = requests.post(API_URL, json=payload, timeout=2)
            if response.status_code == 200:
                st.session_state.ai_result = response.json()
        except:
            pass
    except queue.Empty:
        pass

# ==========================================
# 4. RELAY CONTROL
# ==========================================
def send_relay_command(relay_num, action):
    client = st.session_state.get('mqtt_client')
    if client:
        cmd = json.dumps({"relay": relay_num, "action": action})
        client.publish(TOPIC_COMMAND, cmd, qos=1)
        if relay_num == 1: st.session_state.relay1_state = (action == "on")
        else: st.session_state.relay2_state = (action == "on")

# ==========================================
# 5. UI LAYOUT & STYLING
# ==========================================
st.set_page_config(page_title="SmartEV AI Dashboard", layout="wide", page_icon="⚡")

st.markdown("""
    <style>
    .stApp { background-color: #0E1117; }
    h1, h2, h3 { color: #FAFAFA; }
    .metric-card { background: linear-gradient(135deg, #1E1E2F 0%, #2A2A40 100%); padding: 20px; border-radius: 15px; border: 1px solid #333; text-align: center; box-shadow: 0 4px 15px rgba(0,0,0,0.5); }
    .metric-value { font-size: 32px; font-weight: bold; color: #00FFAE; margin: 10px 0; }
    .metric-label { font-size: 14px; color: #A0A0A0; text-transform: uppercase; letter-spacing: 1px; }
    .ai-card { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); padding: 20px; border-radius: 15px; text-align: center; color: white; }
    .ai-value { font-size: 24px; font-weight: bold; margin: 10px 0; }
    .insight-box { background: rgba(255,255,255,0.05); padding: 15px; border-radius: 10px; border-left: 4px solid #00FFAE; margin-bottom: 10px; }
    .anomaly-alert { background: linear-gradient(135deg, #ff416c 0%, #ff4b2b 100%); padding: 20px; border-radius: 15px; color: white; font-weight: bold; text-align: center; font-size: 18px; }
    </style>
""", unsafe_allow_html=True)

st.title("⚡ SmartEV AI Energy Dashboard")
st.caption("Live Hardware Monitoring & AI Control Center")

if st.session_state.get('mqtt_client'):
    st.success("✅ Connected to MQTT Broker")
else:
    st.error("❌ Disconnected from MQTT Broker")

st.markdown("---")

# --- SIDEBAR ---
with st.sidebar:
    st.header("🎛️ Physical Load Control")
    col1, col2 = st.columns(2)
    with col1:
        btn1 = "DISCHARGE ON" if not st.session_state.relay1_state else "DISCHARGE OFF"
        if st.button(btn1, use_container_width=True, type="primary" if not st.session_state.relay1_state else "secondary"):
            send_relay_command(1, "off" if st.session_state.relay1_state else "on")
    with col2:
        btn2 = "CHARGE ON" if not st.session_state.relay2_state else "CHARGE OFF"
        if st.button(btn2, use_container_width=True, type="primary" if not st.session_state.relay2_state else "secondary"):
            send_relay_command(2, "off" if st.session_state.relay2_state else "on")

# ==========================================
# 6. MAIN DASHBOARD AREA
# ==========================================
if st.session_state.live_data:
    d = st.session_state.live_data
    ai = st.session_state.ai_result or {}
    
    # ROW 1: RAW SENSOR METRICS
    col1, col2, col3, col4 = st.columns(4)
    with col1: st.markdown(f"""<div class='metric-card'><div class='metric-label'>Voltage</div><div class='metric-value'>{d.get('voltage', 0):.2f} V</div></div>""", unsafe_allow_html=True)
    with col2: st.markdown(f"""<div class='metric-card'><div class='metric-label'>Current</div><div class='metric-value'>{d.get('current', 0):.3f} A</div></div>""", unsafe_allow_html=True)
    with col3: st.markdown(f"""<div class='metric-card'><div class='metric-label'>Power</div><div class='metric-value'>{d.get('power', 0):.2f} W</div></div>""", unsafe_allow_html=True)
    with col4: st.markdown(f"""<div class='metric-card'><div class='metric-label'>Battery</div><div class='metric-value'>{d.get('battery_level', 0):.1f} %</div></div>""", unsafe_allow_html=True)

    st.markdown("<br>", unsafe_allow_html=True)

    # ROW 2: AI MODELS OUTPUT (The 3 AI Features)
    st.subheader("🧠 AI Intelligence Engine")
    ai_col1, ai_col2, ai_col3 = st.columns(3)

    # FEATURE 1: Load Identification & Prediction (Random Forest + XGBoost)
    with ai_col1:
        st.markdown(f"""<div class='ai-card'>
            <div class='metric-label'>AI Load ID (Random Forest)</div>
            <div class='ai-value'>{ai.get('load_type_label', 'Idle')}</div>
            <div style='font-size:14px; opacity:0.8;'>Next Power (XGBoost): {ai.get('predicted_power_next', 0):.1f} W</div>
        </div>""", unsafe_allow_html=True)

    # FEATURE 2: Anomaly Risk Gauge (Isolation Forest)
    with ai_col2:
        # Normalize anomaly score to a 0-100% risk gauge
        raw_score = ai.get('anomaly_confidence', 0)
        # Isolation forest: negative = anomaly. Let's map it to 0-100 risk.
        risk_percent = max(0, min(100, int(50 - (raw_score * 20)))) 
        if ai.get('is_anomaly', False): risk_percent = max(risk_percent, 85)
        
        st.markdown(f"""<div class='ai-card' style='background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%);'>
            <div class='metric-label'>System Health (Isolation Forest)</div>
            <div class='ai-value'>{100 - risk_percent}% Healthy</div>
        </div>""", unsafe_allow_html=True)
        st.progress(risk_percent / 100.0, text=f"️ Anomaly Risk: {risk_percent}%")

    # FEATURE 3: Predictive Battery Timeline (Using AI Power Prediction)
    with ai_col3:
        battery_pct = d.get('battery_level', 0)
        predicted_power = ai.get('predicted_power_next', d.get('power', 0))
        
        # Simple physics calculation for timeline
        current_wh = (battery_pct / 100.0) * BATTERY_CAPACITY_WH
        if predicted_power > 10: # Discharging
            hours_left = current_wh / predicted_power
            timeline_text = f"⏳ Drain in {hours_left:.1f} Hours"
            color = "#ff4b2b"
        elif d.get('relay_2', 0) == 1: # Charging
            charge_rate = 500 # Simulated charge rate in W
            hours_to_full = (BATTERY_CAPACITY_WH - current_wh) / charge_rate
            timeline_text = f"🔋 Full in {hours_to_full:.1f} Hours"
            color = "#00FFAE"
        else:
            timeline_text = "⏸️ System Idle"
            color = "#A0A0A0"

        st.markdown(f"""<div class='ai-card' style='background: {color}; color: black;'>
            <div class='metric-label' style='color:black;'>AI Battery Timeline</div>
            <div class='ai-value' style='font-size:20px;'>{timeline_text}</div>
        </div>""", unsafe_allow_html=True)

    st.markdown("---")

    # ROW 3: AI SMART RECOMMENDATIONS ENGINE
    st.subheader("💡 AI Smart Recommendations")
    
    hour = d.get('hour_of_day', 12)
    is_daytime = 8 <= hour <= 17
    load_type = ai.get('load_type_label', 'Idle')
    is_anomaly = ai.get('is_anomaly', False)
    
    recommendations = []
    
    if is_anomaly:
        recommendations.append(("🚨 CRITICAL FAULT DETECTED", "AI has detected an electrical anomaly. Relays should be tripped immediately to prevent hardware damage.", "#ff4b2b"))
    elif battery_pct < 20 and d.get('power', 0) > 500:
        recommendations.append((" Battery Critical", f"Battery is at {battery_pct:.1f}%. AI strongly recommends turning OFF the Discharge Relay to preserve system life.", "#f39c12"))
    elif is_daytime and d.get('relay_2', 0) == 0:
        recommendations.append(("☀️ Solar Peak Available", "It is daytime and the Charge Relay is OFF. AI recommends turning ON the Charge Relay to harvest solar energy.", "#2ecc71"))
    elif load_type == "Heavy Load" and d.get('power', 0) > 1200:
        recommendations.append(("⚡ High Consumption Alert", "Heavy load is drawing >1200W. Monitor battery drain closely. Consider switching to grid power if available.", "#e67e22"))
    else:
        recommendations.append(("✅ System Optimal", "All electrical parameters are within normal thermodynamic limits. AI is continuously monitoring.", "#3498db"))

    # Display Recommendations
    for title, desc, color in recommendations:
        st.markdown(f"""
        <div class='insight-box' style='border-left-color: {color};'>
            <h3 style='color: {color}; margin: 0 0 5px 0;'>{title}</h3>
            <p style='color: #ccc; margin: 0;'>{desc}</p>
        </div>
        """, unsafe_allow_html=True)

    st.markdown("---")

    # ROW 4: REAL-TIME CHARTS
    st.subheader(" Real-Time Telemetry")
    if 'history_df' not in st.session_state:
        st.session_state.history_df = pd.DataFrame(columns=['Time', 'Power', 'Current'])
    
    # Add to history
    new_row = pd.DataFrame([{
        'Time': datetime.now().strftime('%H:%M:%S'),
        'Power': d.get('power', 0),
        'Current': d.get('current', 0)
    }])
    st.session_state.history_df = pd.concat([st.session_state.history_df, new_row], ignore_index=True).tail(30)

    df_plot = st.session_state.history_df
    fig = make_subplots(specs=[[{"secondary_y": True}]])
    fig.add_trace(go.Bar(x=df_plot['Time'], y=df_plot['Power'], name="Power (W)", marker_color="#667eea"), secondary_y=False)
    fig.add_trace(go.Scatter(x=df_plot['Time'], y=df_plot['Current'] * 40, name="Current (A) x40", line=dict(color="#f093fb", width=3)), secondary_y=True)
    fig.update_layout(template="plotly_dark", height=400, legend=dict(orientation="h", yanchor="bottom", y=1.02, xanchor="right", x=1))
    st.plotly_chart(fig, use_container_width=True)

else:
    st.warning("⏳ Waiting for live data from ESP32...")

# Auto-refresh loop
time.sleep(2)
st.rerun()
