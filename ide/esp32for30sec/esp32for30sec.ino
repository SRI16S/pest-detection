#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include "esp_camera.h"

// Wi-Fi credentials
const char* ssid = "Airtel_sara_0906";            
const char* password = "air53712";

// Firebase Storage settings
const String FIREBASE_HOST = "iitm-8d97f.appspot.com";
const String FIREBASE_API_KEY = "AIzaSyDCbfNAZeNHQEIfCqCaTwPa-FzO1EWzeIc";

// Server settings for prediction endpoint
const String SERVER_URL = "192.168.1.9:5000/predict";

// Camera pins for AI Thinker ESP32-CAM
#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27
#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

void initCamera() {
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  if(psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera initialization failed");
  }
}

camera_fb_t* captureImage() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Image capture failed");
    return nullptr;
  }
  return fb;
}

bool uploadToFirebase(camera_fb_t* fb, String &fileLocation) {
  HTTPClient http;
  String firebaseUrl = "https://firebasestorage.googleapis.com/v0/b/" + FIREBASE_HOST + "/o/images%2Fcaptured.jpg?uploadType=media&name=images/captured.jpg";
  
  http.begin(firebaseUrl);
  http.addHeader("Content-Type", "image/jpeg");
  http.addHeader("Authorization", "Bearer " + FIREBASE_API_KEY);
  int httpResponseCode = http.POST(fb->buf, fb->len);
  
  if (httpResponseCode == 200) {
    Serial.println("Image uploaded");
    fileLocation = "images/captured.jpg";
    http.end();
    return true;
  }
  
  Serial.printf("Upload failed %d\n", httpResponseCode);
  Serial.println("Error message: " + http.getString());
  http.end();
  return false;
}

void sendAckToServer(const String &fileLocation) {
  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  String jsonPayload = "{\"file_location\":\"" + fileLocation + "\"}";
  int httpResponseCode = http.POST(jsonPayload);
  
  if (httpResponseCode == 200) {
    Serial.println("Prediction: " + http.getString());
  } else {
    Serial.printf("Failed to get prediction from server. HTTP response code: %d\n", httpResponseCode);
  }

  http.end();
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  int retry_count = 0;
  while (WiFi.status() != WL_CONNECTED && retry_count < 10) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    retry_count++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected");
  } else {
    Serial.println("Failed to connect to WiFi");
  }
}

void setup() {
  Serial.begin(115200);
  initCamera();
  connectToWiFi();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Cannot proceed without WiFi connection");
    return;
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconnecting to WiFi...");
    connectToWiFi();
  }
  
  Serial.println("Capturing image...");
  camera_fb_t *fb = captureImage();
  if (fb) {
    String fileLocation;
    if (uploadToFirebase(fb, fileLocation)) {
      Serial.println("Image uploaded successfully");
      sendAckToServer(fileLocation);
    } else {
      Serial.println("Image upload failed");
    }
    esp_camera_fb_return(fb);
  }
  
  Serial.println("Waiting for 30 seconds before next capture...");
  delay(30000);
}
