# 🚀 Quick Start Guide - Hardware Setup

## What You Need to Do Now

### Step 1: Add the Button (5 minutes)

**Shopping List** (if you don't have):
- 1x Push button (tactile switch) - $0.50
- 1x 10kΩ resistor - $0.10
- 2x Jumper wires - $0.20

**Wiring**:
```
ESP32 Pin 12 ──┬── Button Terminal 1
               │
               └── 10kΩ Resistor ── 3.3V

Button Terminal 2 ── GND
```

**Visual Guide**:
```
        3.3V
         │
        ┌┴┐ 10kΩ
        └┬┘
         │
Pin 12 ──┼──── [Button] ──── GND
```

### Step 2: Upload New Code (10 minutes)

1. **Install WiFiManager Library**:
   - Open Arduino IDE
   - Tools → Manage Libraries
   - Search "WiFiManager"
   - Install "WiFiManager by tzapu"

2. **Upload Code**:
   - Open [`agri_wand_with_button.ino`](file:///home/kain/folders/arduino%20projects%20codes/agri-wand/agri_wand_with_button.ino)
   - Board: "AI Thinker ESP32-CAM"
   - Port: Your USB port
   - Click Upload

3. **First Boot WiFi Setup**:
   - ESP32 creates "Agri-Wand-Setup" WiFi
   - Connect with phone/laptop
   - Password: `admin123`
   - Browser opens automatically
   - Enter your WiFi credentials
   - Save

### Step 3: Start the Server (2 minutes)

**Terminal**:
```bash
cd /home/kain/folders/arduino\ projects\ codes/agri-wand
source venv/bin/activate
python3 fixed_ultra_server.py
```

Server starts on `http://0.0.0.0:5000`

### Step 4: Test It! (1 minute)

1. Point camera at a plant
2. **Press the button**
3. Watch the display:
   - "📸 Analyzing..."
   - LED flashes
   - Results in 3-5 seconds!

---

## What's Different Now?

### Before (Old Code):
- ❌ Automatic capture every 20 seconds
- ❌ No control over timing
- ❌ Hardcoded WiFi (must edit code)
- ❌ Sensor data not used by AI

### After (New Code):
- ✅ **Button-triggered** - you control when to analyze
- ✅ **WiFiManager** - easy WiFi setup via web interface
- ✅ **Sensor-AI integration** - smart recommendations
- ✅ **Enhanced display** - shows sensor insights
- ✅ **Production-ready** - cloud deployment files included

---

## Example Output

**When you press the button**:

**Display Screen 1** (Environmental Data):
```
🌡️ Environmental Data
💧 Soil: 45.2%
🌡️ Temp: 24.5°C
💨 Humidity: 68.3%
📶 WiFi: Connected
🔘 Press button to analyze
```

**Display Screen 3** (AI Results with Sensor Insights):
```
🤖 AI Results
Disease: Tomato - Leaf Mold
Confidence: 92.5%
Severity: Moderate to Severe

💊 Treatment:
Increase ventilation, reduce humidity...

🌱 Sensor Insights:
HIGH: High humidity (68.3%) promoting
fungal growth
Action: Improve ventilation immediately
```

---

## Next Steps (Optional)

### Cloud Deployment (30 minutes)
Make it work anywhere:
1. Create GitHub account
2. Push code to GitHub
3. Deploy to Railway.app (free)
4. Get public URL
5. Works on any WiFi network!

**See**: [walkthrough.md](file:///home/kain/.gemini/antigravity/brain/56b81a23-87d2-4369-8637-a2c1b38cde78/walkthrough.md) for detailed steps

### Hardware Improvements
- Add enclosure ($5-15)
- Add battery for portability ($10-15)
- Add status LEDs ($2-5)

---

## Troubleshooting

**Button doesn't work?**
- Check wiring: Pin 12 → Button → GND
- Check resistor: 10kΩ between Pin 12 and 3.3V
- Open Serial Monitor (115200 baud) - should see "🔘 Button pressed"

**WiFiManager not showing?**
- Look for "Agri-Wand-Setup" in WiFi list
- Password: `admin123`
- If not appearing, re-upload code

**Server errors?**
```bash
# Reinstall dependencies
source venv/bin/activate
pip install -r requirements.txt
```

**No AI results?**
- Check server is running
- Check ESP32 IP matches server IP
- Check firewall allows port 5000

---

**Ready to test? Just add the button and upload the code! 🚀**
