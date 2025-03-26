#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include "esp_camera.h"

// Wi-Fi credentials
const char* ssid = "Airtel_sara_0906";            // Replace with your Wi-Fi network name
const char* password = "air53712";    // Replace with your Wi-Fi network password

// Firebase Storage settings
const String FIREBASE_HOST = "iitm-8d97f.appspot.com";  // Firebase storage URL
const String FIREBASE_API_KEY = "AIzaSyDCbfNAZeNHQEIfCqCaTwPa-FzO1EWzeIc"; // Your Firebase API Key

// Server settings for prediction endpoint
const String SERVER_URL = " 192.168.1.9:5000/predict"; // Replace with your Flask server IP and port

// Define camera pins for AI Thinker ESP32-CAM model
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

// Function to initialize the camera
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

  // Set high resolution and quality
  if(psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;  // High resolution (1600x1200)
    config.jpeg_quality = 10;            // High quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;  // Lower resolution for devices without PSRAM
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Cam init failed");
    return;
  }
}

// Function to capture image
camera_fb_t* captureImage() {
  camera_fb_t *fb = esp_camera_fb_get();  // Capture image
  if (!fb) {
    Serial.println("Capture failed");
    return nullptr;
  }
  return fb;
}

// Function to upload image to Firebase Storage
bool uploadToFirebase(camera_fb_t* fb, String &fileLocation) {
  HTTPClient http;

  // Firebase Storage URL (without the full URL, just the storage path)
  String firebaseUrl = "https://firebasestorage.googleapis.com/v0/b/" + FIREBASE_HOST + "/o/images%2Fcaptured.jpg?uploadType=media&name=images/captured.jpg";

  // Start HTTP request to Firebase
  http.begin(firebaseUrl);
  http.addHeader("Content-Type", "image/jpeg");  // Set content type for JPEG
  http.addHeader("Authorization", "Bearer " + FIREBASE_API_KEY); // Use API key in header

  // Send the image data and capture response code
  int httpResponseCode = http.POST(fb->buf, fb->len);

  // Check the response from Firebase
  if (httpResponseCode == 200) {
    Serial.println("Image uploaded");
    fileLocation = "images/captured.jpg";  // Only store the relative path
    http.end();
    return true;
  } else {
    // Print detailed error message
    Serial.printf("Upload failed %d\n", httpResponseCode);
    Serial.println("Error message: " + http.getString());
    http.end();
    return false;
  }
}

// Function to send acknowledgment to server with image location (relative path)
void sendAckToServer(const String &fileLocation) {
  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  // JSON payload with image location (relative path)
  String jsonPayload = "{\"file_location\":\"" + fileLocation + "\"}";
  
  // Send the payload to the server
  int httpResponseCode = http.POST(jsonPayload);

  // Check the response from server
  if (httpResponseCode == 200) {
    // Get prediction from server response
    String prediction = http.getString();
    Serial.println("Prediction:");
    Serial.println(prediction);
  } else {
    Serial.printf("Failed to get prediction from server. HTTP response code: %d\n", httpResponseCode);
  }

  http.end();
}

// Function to connect to Wi-Fi
void connectToWiFi() {
  WiFi.begin(ssid, password);
  int retry_count = 0;
  while (WiFi.status() != WL_CONNECTED && retry_count < 10) {
    delay(1000);
    Serial.println("Connecting WiFi");
    retry_count++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected");
  } else {
    Serial.println("Failed to connect to WiFi");
  }
}

// Setup function
void setup() {
  Serial.begin(115200);

  // Initialize camera
  initCamera();

  // Connect to Wi-Fi
  connectToWiFi();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Cannot proceed without WiFi connection");
    return;
  }

  // Capture image
  camera_fb_t *fb = captureImage();
  if (fb == nullptr) {
    Serial.println("Image capture failed");
    return;
  }

  // Upload the captured image to Firebase
  String fileLocation;
  if (uploadToFirebase(fb, fileLocation)) {
    Serial.println("Image uploaded to Firebase successfully");
    // Send acknowledgment to server with file location (relative path)
    sendAckToServer(fileLocation);
  } else {
    Serial.println("Image upload to Firebase failed");
  }

  // Return frame buffer
  esp_camera_fb_return(fb);
}

// Loop function (empty for this example)
void loop() {
  // No actions needed in the loop for this demo
}