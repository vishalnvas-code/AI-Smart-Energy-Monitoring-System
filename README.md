# AI-Smart-Energy-Monitoring-System
# ⚡ AI-Enabled IoT Smart Energy Monitoring & Intelligent Load Management

<div align="center">

![Python](https://img.shields.io/badge/Python-3.10+-blue?logo=python)
![ESP32](https://img.shields.io/badge/ESP32-Dev_Board-red?logo=espressif)
![FastAPI](https://img.shields.io/badge/FastAPI-0.103+-009688?logo=fastapi)
![Streamlit](https://img.shields.io/badge/Streamlit-1.31+-FF4B4B?logo=streamlit)
![MQTT](https://img.shields.io/badge/MQTT-Mosquitto-660066?logo=mqtt)
![ML](https://img.shields.io/badge/ML-scikit--learn%20|%20XGBoost-orange)

**An intelligent, self-sustaining energy monitoring system that combines IoT, Embedded Systems, and AI/ML to monitor, predict, and protect electrical loads in real-time.**

[Features](#-key-features) • [Architecture](#-system-architecture) • [Installation](#-installation--setup) • [Usage](#-how-to-run) • [AI Models](#-ai-models--intelligence) • [Troubleshooting](#-troubleshooting)

</div>

---

## 📖 Overview

This project is a **full-stack AI-IoT Smart Energy Monitoring System** built around an **ESP32 microcontroller** powered by a **solar panel, battery, and buck booster** setup (off-grid edge node). The system continuously measures voltage, current, and power consumption of connected electrical loads, transmits data via **MQTT**, and uses **three trained Machine Learning models** to:

1. 🧠 **Identify** which appliance is running (Random Forest)
2. 🛡️ **Detect** electrical faults and anomalies in real-time (Isolation Forest)
3. 📈 **Predict** future power consumption and battery timelines (XGBoost)

A beautiful, dark-themed **Streamlit dashboard** visualizes everything in real-time, while the AI engine provides smart recommendations and can auto-trip relays to protect the hardware from electrical faults.

> 💡 **Short Pitch:** *"Our project is an AI-enabled IoT smart energy monitoring and load management system using ESP32. It measures voltage, current, and power consumption of electrical loads and provides real-time monitoring and remote relay control through a web dashboard. The collected electrical data is used to train an AI/ML model to identify load patterns, detect abnormal power consumption, and predict energy behavior, enabling intelligent and efficient power management."*

---

## ✨ Key Features

### 🔌 Hardware & IoT
- ✅ ESP32-based edge node with voltage & current sensors (ACS712 / ZMPT101B)
- ✅ 2-channel relay control (Discharge / Charge)
- ✅ Solar-powered with battery & buck booster (off-grid capable)
- ✅ MQTT-based real-time telemetry (Eclipse Mosquitto broker)
- ✅ Bidirectional communication (sensor data ↑ / relay commands ↓)

### 🤖 AI/ML Intelligence
- ✅ **Load Identification:** Random Forest Classifier (Idle / Light / Heavy / Both)
- ✅ **Anomaly Detection:** Isolation Forest (flags short circuits, sensor glitches, overloads)
- ✅ **Power Prediction:** XGBoost Regressor (forecasts next-step power consumption)
- ✅ **Smart Recommendations Engine:** Rule-based AI layer combining ML outputs with physical constraints (e.g., *"Solar Peak detected! Turn ON Charge Relay"*)
- ✅ **Predictive Battery Timeline:** Calculates hours until full charge or drain based on AI predictions
- ✅ **Anomaly Risk Gauge:** Visual 0–100% system health indicator

### 🖥️ Dashboard & UI
- ✅ Beautiful dark-themed **Streamlit** dashboard with custom CSS
- ✅ Real-time Plotly charts (Power bars + Current line overlay)
- ✅ Physical relay control buttons (actually click the hardware relays!)
- ✅ Live AI insight cards (Load ID, Predicted Power, System Health)
- ✅ Thread-safe data handling (Queue) for seamless background MQTT processing

### 🛡️ Safety & Automation
- ✅ **Auto-Trip Feature:** AI detects anomaly → instantly sends MQTT command → relays click OFF
- ✅ Protects solar/battery system from real electrical faults and thermal runaway

---

## 🏗️ System Architecture

```text
┌──────────────────────────────────────────────────────────────────┐
│                        HARDWARE (EDGE)                            │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐      │
│  │  Solar   │──▶│  Buck    │──▶│ Battery  │──▶│  ESP32   │      │
│  │  Panel   │   │ Booster  │   │   Pack   │   │ + Sensors│      │
│  └──────────┘   └──────────┘   └──────────┘   └────┬─────┘      │
│                                                     │            │
│                          ┌──────────────────────────┤            │
│                          │ 2-Channel Relay (Loads)  │            │
│                          └──────────────────────────┘            │
└──────────────────────────────────┬───────────────────────────────┘
                                   │ MQTT (smartev/telemetry)
                                   ▼
                    ┌──────────────────────────────┐
                    │   Mosquitto MQTT Broker      │
                    │       (Port 1883)            │
                    └──────────────┬───────────────┘
                                   │
              ┌────────────────────┼────────────────────┐
              ▼                    ▼                    ▼
   ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
   │   Streamlit UI   │  │    FastAPI AI    │  │   MQTT Bridge    │
   │  (Dashboard)     │  │    Backend       │  │   (Optional)     │
   │  :8501           │  │    :8000         │  │                  │
   └──────────────────┘  └────────┬─────────┘  └──────────────────┘
                                  │
                    ┌─────────────┴─────────────┐
                    │   AI Models (.pkl)        │
                    │  • Random Forest          │
                    │  • Isolation Forest       │
                    │  • XGBoost Regressor      │
                    └───────────────────────────┘
