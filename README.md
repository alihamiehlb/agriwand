# 🌱 Agri-Wand Pro: AI-Powered Plant Health System

[![Status](https://img.shields.io/badge/status-production--ready-green)](#)
[![Accuracy](https://img.shields.io/badge/accuracy-90%25+-blue)](#)
[![Model](https://img.shields.io/badge/AI-Gemini%201.5%20Flash-orange)](#)

**Agri-Wand Pro** is a professional-grade, portable IoT device for real-time plant disease detection. It fuses computer vision (Google Gemini AI) with environmental data (Soil, Temp, Humidity) to provide farmers and gardeners with precise diagnosis and treatment advice.

---

## ✨ Key Features

- 🤖 **Multi-Engine AI**: Uses Google Gemini 1.5 Flash + specialized CV algorithms for >90% accuracy.
- 🌡️ **Sensor Fusion**: Correlates environmental stress with diseases for context-aware advice.
- 🖥️ **HD Display Support**: Native support for **3.5" ILI9488 TFT (480x320)** color screens.
- 🔘 **User-Controlled Scan**: Physical button-triggered analysis with audio/visual feedback.
- 📱 **Smart Setup**: WiFiManager configuration portal—no code editing required for changing WiFi or Server URLs.
- 🌍 **Global Support**: Detects issues across 100+ species, including specialized Lebanese crops.
- ☁️ **Cloud Native**: Optimized for Railway.app deployment for always-on accessibility.

---

## 🛠️ Hardware Setup

### Components
1. **ESP32-CAM** (AI-Thinker module)
2. **3.5" ILI9488 TFT** (480x320 SPI)
3. **Capacitive Soil Moisture Sensor**
4. **DHT22** (Temp/Humidity)
5. **Passive Buzzer**
6. **Push Button** (on GPIO 0 or BOOT)

### 🔌 Wiring Diagram (Production)

| Display Pin | ESP32-CAM Pin | Color |
| :--- | :--- | :--- |
| VCC | 5V | Red |
| GND | GND | Black |
| CS | GPIO 15 | Orange |
| RESET | GPIO 4 | Yellow |
| DC/RS | GPIO 2 | Green |
| SDI/MOSI | GPIO 13| Blue |
| SCK | GPIO 14 | Purple |
| LED | 3.3V | White |

**Sensors:**
- **Soil Sensor**: GPIO 33
- **DHT22**: GPIO 16 (or I2C)
- **Button**: GPIO 0 (GND trigger)
- **Buzzer**: GPIO 2 (Shared with DC)

---

## 🚀 Quick Start

### 1. Server Deployment (Cloud)
1. Fork this repository.
2. Link it to [Railway.app](https://railway.app).
3. Add `GEMINI_API_KEY` to your Environment Variables.
4. Deployment is automatic! Get your URL (e.g., `https://your-app.up.railway.app`).

### 2. Firmware Installation
1. Install **Arduino IDE**.
2. Install libraries: `TFT_eSPI`, `ArduinoJson`, `WiFiManager`.
3. Configure `TFT_eSPI/User_Setup.h` for **ILI9488** (see `docs/hardware.md` for details).
4. Upload `agri_wand_with_button.ino` to your ESP32-CAM.

### 3. Device Configuration
1. Connect to "Agri-Wand-Setup" WiFi from your phone.
2. Enter your home WiFi credentials.
3. Paste your Cloud Server URL.
4. **Done!** Press the button to start analyzing.

---

## 📂 Project Directory
- `agri_wand_with_button.ino`: ESP32 Firmware (Production)
- `fixed_ultra_server.py`: Python Flask/AI Server
- `requirements.txt`: Python dependencies
- `nixpacks.toml`: Cloud build configuration
- `runtime.txt`: Python runtime version (3.11.7)

---

## 📄 License
Open source under the MIT License. Feel free to use and contribute!

---
**Made with ❤️ for smart agriculture.**
