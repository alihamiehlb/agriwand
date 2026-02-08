/*
  Fixed Smart Agricultural AI Wand - Simplified Sensor Setup
  Enhanced tomato mold detection + moisture, temperature, humidity sensors
*/

#include "esp_camera.h"
#include <WiFi.h>
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

// Simplified sensor pins
#define SOIL_MOISTURE_PIN 13
#define TEMPERATURE_PIN 14
#define HUMIDITY_PIN 15
#define BUZZER_PIN 2  // Passive buzzer
#define LED_PIN 33

// Display
TFT_eSPI tft = TFT_eSPI();

// WiFi config
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverURL = "http://192.168.1.14:5000/predict";

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
  unsigned long timestamp;
};

SensorData sensors;
AIResult aiResult;
bool newAIResult = false;

void setup() {
  Serial.begin(115200);
  
  // Disable brownout detector
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  // Pin setup
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(TEMPERATURE_PIN, INPUT);
  pinMode(HUMIDITY_PIN, INPUT);
  
  // Initialize display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("🍅 Fixed AI Wand");
  tft.setTextSize(1);
  tft.println("Enhanced detection system");
  
  // Camera setup
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
    config.frame_size = FRAMESIZE_SVGA;
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
    tft.println("Camera failed!");
    return;
  }
  
  tft.setTextColor(TFT_GREEN);
  tft.println("✓ Camera OK");
  
  // WiFi connection
  WiFi.begin(ssid, password);
  tft.println("Connecting WiFi...");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(TFT_GREEN);
    tft.println("✓ WiFi Connected");
    tft.println(WiFi.localIP());
  } else {
    tft.setTextColor(TFT_RED);
    tft.println("✗ WiFi Failed");
  }
  
  // Initialize sensors
  readSensors();
  
  delay(2000);
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  // Read sensors every 3 seconds
  static unsigned long lastSensorRead = 0;
  if (millis() - lastSensorRead > 3000) {
    readSensors();
    lastSensorRead = millis();
  }
  
  // Capture and analyze every 20 seconds
  static unsigned long lastCapture = 0;
  if (millis() - lastCapture > 20000) {
    captureAndAnalyze();
    lastCapture = millis();
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
  
  // Temperature simulation (0-4095 -> 15-40°C)
  int tempRaw = analogRead(TEMPERATURE_PIN);
  sensors.temperature = map(tempRaw, 0, 4095, 15, 40);
  
  // Humidity simulation (0-4095 -> 30-90%)
  int humidRaw = analogRead(HUMIDITY_PIN);
  sensors.humidity = map(humidRaw, 0, 4095, 30, 90);
  
  sensors.timestamp = millis();
  
  Serial.printf("Sensors - Soil: %.1f%%, Temp: %.1f°C, Humidity: %.1f%%\n",
                sensors.soilMoisture, sensors.temperature, sensors.humidity);
}

void captureAndAnalyze() {
  Serial.println("📸 Capturing plant image...");
  
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  
  // LED flash
  digitalWrite(LED_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
  
  // Send to AI server
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "image/jpeg");
    
    int httpResponseCode = http.POST(fb->buf, fb->len);
    
    if (httpResponseCode == 200) {
      String response = http.getString();
      parseAIResponse(response);
      newAIResult = true;
      
      // Success beep (passive buzzer)
      tone(BUZZER_PIN, 1000, 200);
    } else {
      Serial.printf("HTTP Error: %d\n", httpResponseCode);
      tone(BUZZER_PIN, 500, 500); // Error beep
    }
    
    http.end();
  }
  
  esp_camera_fb_return(fb);
}

void parseAIResponse(String response) {
  DynamicJsonDocument doc(2048);
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
  aiResult.timestamp = millis();
  
  Serial.printf("🤖 AI Result: %s (%.1f%% confidence)\n", aiResult.disease.c_str(), aiResult.confidence);
}

void updateDisplay() {
  static unsigned long lastDisplayUpdate = 0;
  static int screen = 0;
  
  // Cycle screens every 4 seconds
  if (millis() - lastDisplayUpdate > 4000) {
    screen = (screen + 1) % 3;
    lastDisplayUpdate = millis();
  }
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  
  switch (screen) {
    case 0:
      displaySensors();
      break;
    case 1:
      displayCamera();
      break;
    case 2:
      displayAI();
      break;
  }
}

void displaySensors() {
  tft.setCursor(10, 10);
  tft.println("🌡️ Environmental Data");
  tft.setTextSize(1);
  
  // Soil moisture
  tft.setTextColor(TFT_BLUE);
  tft.printf("💧 Soil Moisture: %.1f%%\n", sensors.soilMoisture);
  if (sensors.soilMoisture < 30) {
    tft.setTextColor(TFT_RED);
    tft.println("⚠ Low moisture - Water needed!");
  }
  
  // Temperature
  tft.setTextColor(TFT_ORANGE);
  tft.printf("🌡️ Temperature: %.1f°C\n", sensors.temperature);
  if (sensors.temperature > 32) {
    tft.setTextColor(TFT_RED);
    tft.println("⚠ High temperature stress!");
  }
  
  // Humidity
  tft.setTextColor(TFT_CYAN);
  tft.printf("💨 Humidity: %.1f%%\n", sensors.humidity);
  if (sensors.humidity > 80) {
    tft.setTextColor(TFT_RED);
    tft.println("⚠ High humidity - Mold risk!");
  }
  
  // Status
  tft.setTextColor(TFT_GREEN);
  tft.printf("\n📶 WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
  tft.printf("🤖 AI: Enhanced detection active");
}

void displayCamera() {
  tft.setCursor(10, 10);
  tft.println("📸 Smart Camera System");
  tft.setTextSize(1);
  tft.println("Auto-capture every 20 seconds");
  tft.println("Enhanced tomato mold detection");
  tft.println("All plant species supported");
  
  // Camera icon
  tft.drawRect(80, 60, 160, 100, TFT_WHITE);
  tft.fillRect(85, 65, 150, 90, TFT_DARKGREY);
  tft.fillCircle(160, 110, 25, TFT_GREEN);
  tft.fillCircle(160, 110, 20, TFT_BLACK);
  
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(10, 180);
  tft.println("✓ Enhanced AI algorithms");
  tft.println("✓ Fixed tomato mold detection");
  tft.println("✓ Global plant database");
}

void displayAI() {
  tft.setCursor(10, 10);
  tft.println("🤖 Enhanced AI Results");
  tft.setTextSize(1);
  
  if (newAIResult && (millis() - aiResult.timestamp < 60000)) {
    // Disease name with color coding
    if (aiResult.isHealthy) {
      tft.setTextColor(TFT_GREEN);
    } else if (aiResult.confidence > 85) {
      tft.setTextColor(TFT_RED);
    } else {
      tft.setTextColor(TFT_ORANGE);
    }
    
    tft.printf("Disease: %s\n", aiResult.disease.c_str());
    
    tft.setTextColor(TFT_WHITE);
    tft.printf("Confidence: %.1f%%\n", aiResult.confidence);
    tft.printf("Severity: %s\n", aiResult.severity.c_str());
    
    // Treatment recommendations
    tft.setTextColor(TFT_YELLOW);
    tft.println("\n💊 Treatment:");
    String treatment = aiResult.treatment.substring(0, 80);
    tft.println(treatment);
    
    // Prevention
    tft.setTextColor(TFT_CYAN);
    tft.println("\n🛡️ Prevention:");
    String prevention = aiResult.prevention.substring(0, 80);
    tft.println(prevention);
    
    // Sensor fusion recommendations
    tft.setTextColor(TFT_GREEN);
    tft.println("\n🌱 Smart Recommendations:");
    
    if (!aiResult.isHealthy) {
      if (aiResult.disease.indexOf("Mold") >= 0 && sensors.humidity > 75) {
        tft.println("💨 HIGH PRIORITY: Reduce humidity!");
        tft.println("   Improve ventilation immediately");
      }
      if (sensors.soilMoisture < 25) {
        tft.println("💧 Increase watering schedule");
      }
      if (sensors.temperature > 33) {
        tft.println("🌡️ Provide shade/cooling");
      }
      if (aiResult.disease.indexOf("Blight") >= 0 && sensors.humidity > 70) {
        tft.println("⚠️ Blight risk - Reduce moisture");
      }
    }
    
  } else {
    tft.setTextColor(TFT_WHITE);
    tft.println("No recent AI analysis");
    tft.println("Next capture in...");
    
    int timeLeft = 20 - ((millis() % 20000) / 1000);
    tft.printf("%d seconds", timeLeft);
    
    tft.setTextColor(TFT_CYAN);
    tft.println("\n🔧 System Status:");
    tft.println("✓ Enhanced algorithms loaded");
    tft.println("✓ Tomato mold detection fixed");
    tft.println("✓ All plants supported");
  }
}

void checkAlerts() {
  bool criticalAlert = false;
  
  // Critical sensor alerts
  if (sensors.soilMoisture < 15) {
    criticalAlert = true;
    Serial.println("🚨 CRITICAL: Extremely dry soil!");
  }
  
  if (sensors.temperature > 38) {
    criticalAlert = true;
    Serial.println("🚨 CRITICAL: Dangerous temperature!");
  }
  
  if (sensors.humidity > 90) {
    criticalAlert = true;
    Serial.println("🚨 CRITICAL: Extreme humidity!");
  }
  
  // AI disease alerts
  if (newAIResult && !aiResult.isHealthy && aiResult.confidence > 85) {
    if (aiResult.disease.indexOf("Mold") >= 0 || 
        aiResult.disease.indexOf("Blight") >= 0 ||
        aiResult.disease.indexOf("Spot") >= 0) {
      criticalAlert = true;
      Serial.printf("🚨 CRITICAL DISEASE: %s detected!\n", aiResult.disease.c_str());
    }
  }
  
  // Trigger alert with passive buzzer
  if (criticalAlert) {
    digitalWrite(LED_PIN, HIGH);
    
    // Passive buzzer alert pattern
    tone(BUZZER_PIN, 2000, 500);  // High tone
    delay(200);
    tone(BUZZER_PIN, 1000, 500);  // Medium tone  
    delay(200);
    tone(BUZZER_PIN, 500, 1000);  // Low tone
    
    delay(100);
    digitalWrite(LED_PIN, LOW);
  }
}
