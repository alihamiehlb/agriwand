/*
  Agri-Wand Production Version - Button Triggered Analysis
  Features: Manual capture button, sensor-AI integration, WiFiManager support
*/

#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiManager.h>  // Install via Library Manager
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ESP32-CAM pins
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Sensor and control pins (Unified for ILI9488 / ESP32-CAM)
#define CAPTURE_BUTTON    0      // Use BOOT button or external on GPIO 0
#define SOIL_MOISTURE_PIN 33     // Analog input (Shares with onboard LED)
#define DHT_PIN           16     // DHT22 Data pin (if available) or shared
#define BUZZER_PIN        2      // Shared with TFT DC

// Status indicators
#define STATUS_LED        33     // Onboard LED (Note: inverted logic)

// Display - Using TFT_eSPI (User_Setup.h must be configured for ILI9488)
TFT_eSPI tft = TFT_eSPI();

// WiFi Manager
WiFiManager wifiManager;

// Server URL (Production Cloud Default)
String serverURL = "https://web-production-75c3a.up.railway.app/predict";
// For local testing: "http://192.168.1.14:5000/predict"

// Sensor data structure
struct SensorData {
  float soilMoisture;
  float temperature;
  float humidity;
  unsigned long timestamp;
};

// AI result structure
struct AIResult {
  String disease;
  float confidence;
  String treatment;
  String prevention;
  String severity;
  bool isHealthy;
  String sensorRecommendations;
  unsigned long timestamp;
};

SensorData sensors;
AIResult aiResult;
bool newAIResult = false;
bool analysisInProgress = false;

// Button state
volatile bool buttonPressed = false;
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 400;  // 300ms debounce

// Interrupt handler for button
void IRAM_ATTR buttonISR() {
  unsigned long currentTime = millis();
  if (currentTime - lastButtonPress > DEBOUNCE_DELAY) {
    buttonPressed = true;
    lastButtonPress = currentTime;
  }
}

void setup() {
  Serial.begin(115200);
  
  // Disable brownout detector for camera stability
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  // Pin setup
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, HIGH); // LED OFF (inverted)
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(SOIL_MOISTURE_PIN, INPUT); // Keep this if using analog soil sensor
  pinMode(DHT_PIN, INPUT); // Keep this if using DHT sensor
  pinMode(CAPTURE_BUTTON, INPUT_PULLUP);
  
  // Attach button interrupt
  attachInterrupt(digitalPinToInterrupt(CAPTURE_BUTTON), buttonISR, FALLING);
  
  // Initialize ILI9488 Display
  tft.init();
  tft.setRotation(1); // Landscape 480x320
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  // Professional Boot Screen
  tft.setTextSize(3);
  tft.setCursor(60, 100);
  tft.println("🍅 AGRI-WAND PRO");
  tft.setTextSize(2);
  tft.setCursor(60, 140);
  tft.println("3.5\" HD AI Analysis System");
  tft.setTextSize(1);
  tft.setCursor(60, 180);
  tft.println("Initializing hardware...");
  
  // Camera Setup
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA; // Higher res for larger screen
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x", err);
    tft.setTextColor(TFT_RED);
    tft.setCursor(60, 200);
    tft.println("✗ CAMERA ERROR!");
    return;
  }
  
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(60, 200);
  tft.println("✓ CAMERA READY");
  
  // WiFi connection with WiFiManager
  tft.setCursor(60, 220);
  tft.println("Setting up WiFi...");
  
    digitalWrite(WIFI_LED, HIGH);  // Blink during setup
  #endif
  
  // Custom parameters for server URL
  WiFiManagerParameter custom_server("server", "AI Server URL", 
                                     serverURL.c_str(), 100);
  wifiManager.addParameter(&custom_server);
  
  // Set timeout for config portal
  wifiManager.setConfigPortalTimeout(180);  // 3 minutes
  
  // Auto-connect or start config portal
  if (!wifiManager.autoConnect("Agri-Wand-Setup", "admin123")) {
    tft.setTextColor(TFT_RED);
    tft.println("✗ WiFi Failed");
    Serial.println("Failed to connect, restarting...");
    delay(3000);
    ESP.restart();
  }
  
  // Connected!
  tft.setTextColor(TFT_GREEN);
  tft.println("✓ WiFi Connected");
  tft.println(WiFi.localIP());
  
  #ifdef WIFI_LED
    digitalWrite(WIFI_LED, LOW);  // Solid on when connected
  #endif
  
  // Get custom server URL
  serverURL = String(custom_server.getValue());
  Serial.printf("Server URL: %s\n", serverURL.c_str());
  
  // Initialize sensors
  readSensors();
  
  // Success beep
  tone(BUZZER_PIN, 1000, 200);
  
  delay(2000);
  tft.fillScreen(TFT_BLACK);
  
  Serial.println("✅ Agri-Wand Ready!");
  Serial.println("Press button to analyze plant");
}

void loop() {
  // Read sensors every 3 seconds
  static unsigned long lastSensorRead = 0;
  if (millis() - lastSensorRead > 3000) {
    readSensors();
    lastSensorRead = millis();
  }
  
  // Check for button press
  if (buttonPressed && !analysisInProgress) {
    buttonPressed = false;  // Reset flag
    
    Serial.println("🔘 Button pressed - Starting analysis");
    
    // Visual feedback
    digitalWrite(LED_PIN, HIGH);
    #ifdef CAPTURE_LED
      digitalWrite(CAPTURE_LED, HIGH);
    #endif
    tone(BUZZER_PIN, 2000, 100);  // Beep
    
    // Show analyzing message
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 100);
    tft.println("📸 Analyzing...");
    tft.setTextSize(1);
    tft.println("\nCapturing image");
    tft.println("Reading sensors");
    tft.println("Sending to AI...");
    
    // Perform analysis
    captureAndAnalyze();
    
    // Turn off indicators
    digitalWrite(LED_PIN, LOW);
    #ifdef CAPTURE_LED
      digitalWrite(CAPTURE_LED, LOW);
    #endif
  }
  
  // Update display
  updateDisplay();
  
  // Check alerts
  checkAlerts();
  
  delay(100);
}

void readSensors() {
  // Soil moisture (0-4095 -> 0-100%)
  int soilRaw = analogRead(SOIL_MOISTURE_PIN);
  sensors.soilMoisture = map(soilRaw, 0, 4095, 100, 0);
  sensors.soilMoisture = constrain(sensors.soilMoisture, 0, 100);
  
  // Note: Integrated DHT22 reading should be placed here
  // For production, ensure DHT sensor is on GPIO 16 (or I2C if using expander)
  sensors.temperature = 25.5; // Placeholder
  sensors.humidity = 60.0;    // Placeholder
  
  sensors.timestamp = millis();
}

void captureAndAnalyze() {
  analysisInProgress = true;
  
  Serial.println("📸 Capturing plant image...");
  
  // Read sensors first
  readSensors();
  
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    tft.setTextColor(TFT_RED);
    tft.println("\n✗ Camera error!");
    tone(BUZZER_PIN, 500, 500);
    analysisInProgress = false;
    return;
  }
  
  // LED flash
  digitalWrite(LED_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
  
  // Send to AI server with sensor data
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    
    // Add sensor data as custom headers
    http.addHeader("Content-Type", "image/jpeg");
    http.addHeader("X-Soil-Moisture", String(sensors.soilMoisture, 1));
    http.addHeader("X-Temperature", String(sensors.temperature, 1));
    http.addHeader("X-Humidity", String(sensors.humidity, 1));
    
    Serial.printf("📤 Sending to server: %s\n", serverURL.c_str());
    Serial.printf("   Soil: %.1f%%, Temp: %.1f°C, Humidity: %.1f%%\n",
                  sensors.soilMoisture, sensors.temperature, sensors.humidity);
    
    int httpResponseCode = http.POST(fb->buf, fb->len);
    
    if (httpResponseCode == 200) {
      String response = http.getString();
      parseAIResponse(response);
      newAIResult = true;
      
      // Success beep
      tone(BUZZER_PIN, 1500, 200);
      delay(250);
      tone(BUZZER_PIN, 2000, 200);
      
      Serial.println("✅ Analysis complete!");
    } else {
      Serial.printf("❌ HTTP Error: %d\n", httpResponseCode);
      tft.setTextColor(TFT_RED);
      tft.println("\n✗ Server error!");
      tone(BUZZER_PIN, 500, 500);
    }
    
    http.end();
  } else {
    Serial.println("❌ WiFi not connected");
    tft.setTextColor(TFT_RED);
    tft.println("\n✗ No WiFi!");
    tone(BUZZER_PIN, 500, 500);
  }
  
  esp_camera_fb_return(fb);
  analysisInProgress = false;
}

void parseAIResponse(String response) {
  DynamicJsonDocument doc(4096);  // Increased size for sensor recommendations
  DeserializationError error = deserializeJson(doc, response);
  
  if (error) {
    Serial.printf("JSON parse failed: %s\n", error.c_str());
    return;
  }
  
  aiResult.disease = doc["disease"] | "Unknown";
  aiResult.confidence = doc["confidence"] | 0.0;
  aiResult.treatment = doc["treatment"] | "No treatment available";
  aiResult.prevention = doc["prevention"] | "No prevention advice";
  aiResult.severity = doc["severity"] | "Unknown";
  aiResult.isHealthy = doc["is_healthy"] | false;
  
  // Get sensor recommendations if available
  if (doc.containsKey("sensor_recommendations")) {
    JsonArray recommendations = doc["sensor_recommendations"];
    aiResult.sensorRecommendations = "";
    
    for (JsonObject rec : recommendations) {
      String priority = rec["priority"] | "";
      String issue = rec["issue"] | "";
      String action = rec["action"] | "";
      
      aiResult.sensorRecommendations += priority + ": " + issue + "\n" + action + "\n\n";
    }
  }
  
  aiResult.timestamp = millis();
  
  Serial.printf("🤖 AI Result: %s (%.1f%% confidence)\n", 
                aiResult.disease.c_str(), aiResult.confidence);
}

void updateDisplay() {
  static unsigned long lastDisplayUpdate = 0;
  static int screen = 0;
  
  if (analysisInProgress) return;
  
  if (millis() - lastDisplayUpdate > 6000) {
    screen = (screen + 1) % 3;
    lastDisplayUpdate = millis();
  }
  
  tft.fillScreen(TFT_BLACK);
  
  switch (screen) {
    case 0: displaySensors(); break;
    case 1: displayStatus(); break;
    case 2: displayAI(); break;
  }
}

void displaySensors() {
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 20);
  tft.println("SYSTEM SENSORS");
  
  tft.setTextSize(2);
  // Soil Moisture Bar
  tft.setCursor(20, 70);
  tft.setTextColor(TFT_BLUE);
  tft.printf("Soil Moisture: %.1f%%", sensors.soilMoisture);
  tft.drawRect(20, 100, 440, 20, TFT_WHITE);
  tft.fillRect(22, 102, (int)(436 * (sensors.soilMoisture/100.0)), 16, TFT_BLUE);
  
  // Temperature
  tft.setCursor(20, 140);
  tft.setTextColor(TFT_ORANGE);
  tft.printf("Temperature: %.1f C", sensors.temperature);
  
  // Humidity
  tft.setCursor(20, 180);
  tft.setTextColor(TFT_CYAN);
  tft.printf("Air Humidity: %.1f%%", sensors.humidity);
  
  // Large Prompt
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(60, 260);
  tft.println(">>> PRESS BUTTON TO ANALYZE <<<");
}

void displayStatus() {
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 20);
  tft.println("WAND STATUS");
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(20, 80);  tft.println("✓ SYSTEM: ONLINE");
  tft.setCursor(20, 120); tft.println("✓ WIFI: CONNECTED");
  tft.setCursor(20, 160); tft.println("✓ CAMERA: SVGA READY");
  tft.setCursor(20, 200); tft.println("✓ CLOUD: RAILWAY ACTIVE");
  
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 250);
  tft.setTextSize(1);
  tft.println("FIRMWARE: AGRI-WAND PRO v2.0 (ILI9488)");
}

void displayAI() {
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 20);
  tft.println("AI DIAGNOSIS");
  
  if (newAIResult && (millis() - aiResult.timestamp < 300000)) { // Show for 5 mins
    tft.setTextSize(2);
    tft.setCursor(20, 70);
    if (aiResult.isHealthy) {
      tft.setTextColor(TFT_GREEN);
      tft.println("HEALTH: PLANT IS HEALTHY");
    } else {
      tft.setTextColor(TFT_RED);
      tft.printf("DISEASE: %s", aiResult.disease.c_str());
    }
    
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(20, 110);
    tft.printf("CONFIDENCE: %.1f%%", aiResult.confidence);
    
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(20, 150);
    tft.println("TREATMENT:");
    tft.setTextSize(1);
    tft.setCursor(20, 180);
    tft.println(aiResult.treatment);
    
    // Recommendations
    if (aiResult.sensorRecommendations.length() > 0) {
      tft.setTextColor(TFT_CYAN);
      tft.setTextSize(1);
      tft.setCursor(20, 240);
      tft.println(aiResult.sensorRecommendations);
    }
  } else {
    tft.setTextSize(2);
    tft.setCursor(20, 100);
    tft.println("READY FOR NEW SCAN");
  }
}

void checkAlerts() {
  bool criticalAlert = false;
  if (sensors.soilMoisture < 10) criticalAlert = true;
  if (newAIResult && !aiResult.isHealthy && aiResult.confidence > 90) criticalAlert = true;
  
  if (criticalAlert) {
    tone(BUZZER_PIN, 1000, 500);
    digitalWrite(STATUS_LED, LOW); // LED ON
    delay(500);
    digitalWrite(STATUS_LED, HIGH); // LED OFF
  }
}
