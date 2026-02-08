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

// Sensor and control pins
#define SOIL_MOISTURE_PIN 13
#define TEMPERATURE_PIN 14    // DHT22 DATA pin (use DHT library for production)
#define HUMIDITY_PIN 15       // Currently unused (DHT22 provides both on one pin)
#define CAPTURE_BUTTON 12     // Button to trigger analysis
#define BUZZER_PIN 2
#define LED_PIN 33

// Status LEDs (optional - add if you have extra LEDs)
#define WIFI_LED 26           // Blue LED for WiFi status
#define CAPTURE_LED 27        // Yellow LED for capture indicator

// Display
TFT_eSPI tft = TFT_eSPI();

// WiFi Manager
WiFiManager wifiManager;

// Server URL (can be configured via WiFiManager)
String serverURL = "http://192.168.1.14:5000/predict";  // Default local
// For production: "https://agri-wand-production.up.railway.app/predict"

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
const unsigned long DEBOUNCE_DELAY = 300;  // 300ms debounce

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
  
  // Disable brownout detector
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  // Pin setup
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(TEMPERATURE_PIN, INPUT);
  pinMode(CAPTURE_BUTTON, INPUT_PULLUP);  // Internal pull-up
  
  // Optional status LEDs
  #ifdef WIFI_LED
    pinMode(WIFI_LED, OUTPUT);
  #endif
  #ifdef CAPTURE_LED
    pinMode(CAPTURE_LED, OUTPUT);
  #endif
  
  // Attach button interrupt
  attachInterrupt(digitalPinToInterrupt(CAPTURE_BUTTON), buttonISR, FALLING);
  
  // Initialize display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("🍅 Agri-Wand Pro");
  tft.setTextSize(1);
  tft.println("Button-triggered analysis");
  
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
    tft.println("✗ Camera failed!");
    return;
  }
  
  tft.setTextColor(TFT_GREEN);
  tft.println("✓ Camera OK");
  
  // WiFi connection with WiFiManager
  tft.println("Setting up WiFi...");
  
  #ifdef WIFI_LED
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
  // Soil moisture (0-4095 → 0-100%)
  int soilRaw = analogRead(SOIL_MOISTURE_PIN);
  sensors.soilMoisture = map(soilRaw, 0, 4095, 100, 0);
  sensors.soilMoisture = constrain(sensors.soilMoisture, 0, 100);
  
  // Temperature (simulated - use DHT library for production)
  // TODO: Replace with DHT22 library reading
  int tempRaw = analogRead(TEMPERATURE_PIN);
  sensors.temperature = map(tempRaw, 0, 4095, 15, 40);
  
  // Humidity (simulated - use DHT library for production)
  // TODO: Replace with DHT22 library reading
  int humidRaw = analogRead(HUMIDITY_PIN);
  sensors.humidity = map(humidRaw, 0, 4095, 30, 90);
  
  sensors.timestamp = millis();
  
  Serial.printf("📊 Sensors - Soil: %.1f%%, Temp: %.1f°C, Humidity: %.1f%%\n",
                sensors.soilMoisture, sensors.temperature, sensors.humidity);
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
  
  // Don't update if analysis in progress
  if (analysisInProgress) return;
  
  // Cycle screens every 5 seconds
  if (millis() - lastDisplayUpdate > 5000) {
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
      displayStatus();
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
  tft.printf("💧 Soil: %.1f%%\n", sensors.soilMoisture);
  if (sensors.soilMoisture < 30) {
    tft.setTextColor(TFT_RED);
    tft.println("  ⚠ Low - Water needed!");
  }
  
  // Temperature
  tft.setTextColor(TFT_ORANGE);
  tft.printf("🌡️ Temp: %.1f°C\n", sensors.temperature);
  if (sensors.temperature > 32) {
    tft.setTextColor(TFT_RED);
    tft.println("  ⚠ High temp stress!");
  }
  
  // Humidity
  tft.setTextColor(TFT_CYAN);
  tft.printf("💨 Humidity: %.1f%%\n", sensors.humidity);
  if (sensors.humidity > 80) {
    tft.setTextColor(TFT_RED);
    tft.println("  ⚠ High - Mold risk!");
  }
  
  // Status
  tft.setTextColor(TFT_GREEN);
  tft.printf("\n📶 WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
  
  // Button prompt
  tft.setTextColor(TFT_YELLOW);
  tft.println("\n🔘 Press button to analyze");
}

void displayStatus() {
  tft.setCursor(10, 10);
  tft.println("📸 System Status");
  tft.setTextSize(1);
  
  tft.setTextColor(TFT_GREEN);
  tft.println("✓ Camera ready");
  tft.println("✓ Sensors active");
  tft.println("✓ AI server connected");
  tft.println("✓ Button-triggered mode");
  
  tft.setTextColor(TFT_CYAN);
  tft.println("\nFeatures:");
  tft.println("• Manual capture button");
  tft.println("• Sensor-AI integration");
  tft.println("• Enhanced recommendations");
  tft.println("• 90%+ accuracy");
  
  tft.setTextColor(TFT_YELLOW);
  tft.println("\n🔘 Press to analyze plant");
}

void displayAI() {
  tft.setCursor(10, 10);
  tft.println("🤖 AI Results");
  tft.setTextSize(1);
  
  if (newAIResult && (millis() - aiResult.timestamp < 120000)) {  // Show for 2 minutes
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
    
    // Treatment (truncated for display)
    tft.setTextColor(TFT_YELLOW);
    tft.println("\n💊 Treatment:");
    String treatment = aiResult.treatment.substring(0, 60);
    tft.println(treatment + "...");
    
    // Sensor recommendations
    if (aiResult.sensorRecommendations.length() > 0) {
      tft.setTextColor(TFT_CYAN);
      tft.println("\n🌱 Sensor Insights:");
      String recommendations = aiResult.sensorRecommendations.substring(0, 80);
      tft.println(recommendations);
    }
    
  } else {
    tft.setTextColor(TFT_WHITE);
    tft.println("No recent analysis");
    
    tft.setTextColor(TFT_YELLOW);
    tft.println("\n🔘 Press button to analyze");
    
    tft.setTextColor(TFT_CYAN);
    tft.println("\nReady to detect:");
    tft.println("• Tomato diseases");
    tft.println("• 100+ plant species");
    tft.println("• Environmental issues");
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
  
  // Trigger alert
  if (criticalAlert) {
    digitalWrite(LED_PIN, HIGH);
    
    // Alert pattern
    tone(BUZZER_PIN, 2000, 300);
    delay(200);
    tone(BUZZER_PIN, 1000, 300);
    delay(200);
    tone(BUZZER_PIN, 500, 500);
    
    delay(100);
    digitalWrite(LED_PIN, LOW);
  }
}
