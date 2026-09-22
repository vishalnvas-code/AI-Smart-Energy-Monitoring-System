from pydantic import BaseModel

class SensorDataRequest(BaseModel):
    device_id: str = "esp32_01"
    voltage: float
    current: float
    power: float
    relay_1: int
    relay_2: int
    battery_level: float
    hour_of_day: int

class PredictionResponse(BaseModel):
    device_id: str
    load_type: int
    load_type_label: str
    is_anomaly: bool
    anomaly_confidence: float
    predicted_power_next: float
    timestamp: str
