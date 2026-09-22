import joblib
import numpy as np
import pandas as pd
from collections import deque
from datetime import datetime
import os

class AIPredictionEngine:
    def __init__(self, models_dir="../models"):
        print("🧠 Loading AI Models and Scalers...")
        
        # Load Models
        self.model_load = joblib.load(os.path.join(models_dir, "model_load_id_rf.pkl"))
        self.model_anomaly = joblib.load(os.path.join(models_dir, "model_anomaly_iso.pkl"))
        self.model_power = joblib.load(os.path.join(models_dir, "model_power_xgb.pkl"))
        
        # Load Scalers
        self.scaler_load = joblib.load(os.path.join(models_dir, "scaler_load.pkl"))
        self.scaler_anomaly = joblib.load(os.path.join(models_dir, "scaler_anomaly.pkl"))
        self.scaler_power = joblib.load(os.path.join(models_dir, "scaler_power.pkl"))
        
        # Sliding window for feature engineering (Window size = 5 to match training)
        self.history_buffer = deque(maxlen=5)
        
        self.load_labels = {0: "Idle", 1: "Light Load", 2: "Heavy Load", 3: "Both Loads"}
        print("✅ AI Models loaded successfully!")

    def _engineer_features(self, raw_data: dict) -> np.ndarray:
        """Calculates Deltas and Rolling features from raw ESP32 data."""
        self.history_buffer.append(raw_data)
        
        # 1. Calculate Deltas (Rate of Change)
        if len(self.history_buffer) >= 2:
            prev = self.history_buffer[-2]
            curr = self.history_buffer[-1]
            p_delta = curr['power'] - prev['power']
            c_delta = curr['current'] - prev['current']
            v_delta = curr['voltage'] - prev['voltage']
        else:
            p_delta = c_delta = v_delta = 0.0
            
        # 2. Calculate Rolling Statistics
        powers = [x['power'] for x in self.history_buffer]
        currents = [x['current'] for x in self.history_buffer]
        
        p_rolling_mean = np.mean(powers)
        c_rolling_std = np.std(currents) if len(currents) > 1 else 0.0
        
        # 3. Time Context
        is_daytime = 1 if 8 <= raw_data['hour_of_day'] <= 17 else 0
        
        # Construct the exact 13-feature array expected by the models
        features = [
            raw_data['voltage'], raw_data['current'], raw_data['power'],
            raw_data['relay_1'], raw_data['relay_2'], raw_data['battery_level'],
            raw_data['hour_of_day'], is_daytime,
            p_delta, c_delta, v_delta,
            p_rolling_mean, c_rolling_std
        ]
        
        return np.array(features).reshape(1, -1)

    def predict(self, data: dict) -> dict:
        # 1. Feature Engineering
        X_raw = self._engineer_features(data)
        
        # 2. Load Identification (Random Forest)
        X_load_scaled = self.scaler_load.transform(X_raw)
        load_pred = self.model_load.predict(X_load_scaled)[0]
        
        # 3. Anomaly Detection (Isolation Forest)
        X_anom_scaled = self.scaler_anomaly.transform(X_raw)
        # Isolation forest returns -1 for anomaly, 1 for normal
        anom_pred_raw = self.model_anomaly.predict(X_anom_scaled)[0]
        is_anomaly = True if anom_pred_raw == -1 else False
        # Get anomaly score (decision function)
        anom_score = self.model_anomaly.decision_function(X_anom_scaled)[0]
        
        # 4. Power Prediction (XGBoost)
        X_pow_scaled = self.scaler_power.transform(X_raw)
        power_pred = self.model_power.predict(X_pow_scaled)[0]
        
        return {
            "device_id": data.get("device_id", "esp32_01"),
            "load_type": int(load_pred),
            "load_type_label": self.load_labels.get(int(load_pred), "Unknown"),
            "is_anomaly": is_anomaly,
            "anomaly_confidence": round(float(anom_score), 4),
            "predicted_power_next": round(float(power_pred), 2),
            "timestamp": datetime.now().isoformat()
        }

# Initialize global engine instance
ai_engine = AIPredictionEngine()
