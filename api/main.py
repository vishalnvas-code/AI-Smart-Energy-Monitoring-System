from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from schemas import SensorDataRequest, PredictionResponse
from ml_service import ai_engine
import uvicorn

app = FastAPI(
    title="SmartEV AI API",
    description="AI-Enabled IoT Smart Energy Monitoring System",
    version="1.0.0"
)

# Allow CORS for Web Dashboard and ESP32
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/")
def root():
    return {
        "message": "🚀 SmartEV AI API is Running",
        "endpoints": {
            "predict": "/api/v1/predict (POST)",
            "health": "/api/v1/health (GET)"
        }
    }

@app.get("/api/v1/health")
def health_check():
    return {"status": "healthy", "ai_models_loaded": True}

@app.post("/api/v1/predict", response_model=PredictionResponse)
def get_ai_predictions(data: SensorDataRequest):
    try:
        # Convert Pydantic model to dict for the ML engine
        raw_data = data.dict()
        
        # Run AI Inference
        result = ai_engine.predict(raw_data)
        
        return result
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

if __name__ == "__main__":
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=True)
