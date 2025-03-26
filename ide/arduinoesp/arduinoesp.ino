#include <DHT.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

// DHT sensor configuration
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Soil moisture sensor configuration
#define SOIL_MOISTURE_PIN A0

// ESP8266 WiFi module configuration
#define ESP_RX 10  // Connect ESP8266 TX to Arduino RX (pin 10)
#define ESP_TX 11  // Connect ESP8266 RX to Arduino TX (pin 11)
SoftwareSerial esp8266(ESP_RX, ESP_TX);

// Variables to store sensor readings
float temperature;
float humidity;
int soilMoisture;

// Timing variables
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 30000; // 30 seconds between updates

// Firebase configuration
const char* FIREBASE_HOST = "iitm-8d97f.firebaseio.com";
const char* FIREBASE_AUTH = "jItkuriEnnRnE2NBFeKX8prunQsTXBdvYhiC0IGx";
const char* WIFI_SSID = "Technician";
const char* WIFI_PASSWORD = "1010101010";

// Debug LED
#define DEBUG_LED 13

void setup() {
  // Initialize Serial communication
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  // Initialize debug LED
  pinMode(DEBUG_LED, OUTPUT);
  digitalWrite(DEBUG_LED, HIGH);
  
  // Initialize ESP8266
  esp8266.begin(115200);
  delay(1000); // Give ESP8266 time to start
  
  // Initialize DHT sensor
  dht.begin();
  
  // Initialize ESP8266
  initESP8266();
  
  digitalWrite(DEBUG_LED, LOW);
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Only update Firebase every UPDATE_INTERVAL milliseconds
  if (currentMillis - lastUpdate >= UPDATE_INTERVAL) {
    digitalWrite(DEBUG_LED, HIGH); // Turn on LED during reading
    
    // Read temperature and humidity from DHT sensor
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    
    // Read soil moisture
    soilMoisture = analogRead(SOIL_MOISTURE_PIN);
    
    // Check if any reads failed and exit early (to try again)
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!");
      digitalWrite(DEBUG_LED, LOW);
      return;
    }
    
    // Print sensor readings to Serial Monitor
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");
    
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println("%");
    
    Serial.print("Soil Moisture: ");
    Serial.println(soilMoisture);
    
    // Send data to Firebase
    sendToFirebase();
    
    lastUpdate = currentMillis;
    digitalWrite(DEBUG_LED, LOW);
  }
  
  // Small delay to prevent overwhelming the system
  delay(100);
}

void initESP8266() {
  Serial.println("Initializing ESP8266...");
  String cmd;  // Declare cmd variable at the start of the function
  
  // Reset ESP8266
  sendCommand("AT+RST", 2000);
  
  // Set ESP8266 to Station mode
  sendCommand("AT+CWMODE=1", 1000);
  
  // Connect to WiFi
  cmd = "AT+CWJAP=\"" + String(WIFI_SSID) + "\",\"" + String(WIFI_PASSWORD) + "\"";
  String response = sendCommand(cmd, 5000);
  
  if (response.indexOf("OK") != -1) {
    Serial.println("WiFi Connected Successfully");
  } else {
    Serial.println("WiFi Connection Failed");
  }
  
  // Test connection to Firebase
  cmd = "AT+CIPSTART=\"TCP\",\"" + String(FIREBASE_HOST) + "\",80";
  response = sendCommand(cmd, 2000);
  
  if (response.indexOf("OK") != -1) {
    Serial.println("Firebase Connection Test Successful");
  } else {
    Serial.println("Firebase Connection Test Failed");
  }
  
  Serial.println("ESP8266 initialization complete");
}

void sendToFirebase() {
  Serial.println("Preparing to send data to Firebase...");
  String cmd;  // Declare cmd variable at the start of the function
  
  // Create JSON payload
  StaticJsonDocument<200> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["soilMoisture"] = soilMoisture;
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Prepare HTTP request
  cmd = "AT+CIPSTART=\"TCP\",\"" + String(FIREBASE_HOST) + "\",80";
  String response = sendCommand(cmd, 2000);
  
  if (response.indexOf("OK") == -1) {
    Serial.println("Failed to establish TCP connection");
    return;
  }
  
  // Send HTTP POST request with authentication
  String httpRequest = "POST /sensorData.json?auth=" + String(FIREBASE_AUTH) + " HTTP/1.1\r\n";
  httpRequest += "Host: " + String(FIREBASE_HOST) + "\r\n";
  httpRequest += "Content-Type: application/json\r\n";
  httpRequest += "Content-Length: " + String(jsonString.length()) + "\r\n";
  httpRequest += "Connection: close\r\n\r\n";
  httpRequest += jsonString;
  
  cmd = "AT+CIPSEND=" + String(httpRequest.length());
  response = sendCommand(cmd, 1000);
  
  if (response.indexOf(">") != -1) {
    response = sendCommand(httpRequest, 2000);
    if (response.indexOf("200 OK") != -1) {
      Serial.println("Data sent to Firebase successfully");
    } else {
      Serial.println("Failed to send data to Firebase");
      Serial.println("Response: " + response);
    }
  } else {
    Serial.println("Failed to prepare data transmission");
  }
}

String sendCommand(String command, const int timeout) {
  String response = "";
  
  esp8266.println(command);
  long int time = millis();
  
  while((time+timeout) > millis()) {
    while(esp8266.available()) {
      char c = esp8266.read();
      response += c;
    }
  }
  
  Serial.println("Command: " + command);
  Serial.println("Response: " + response);
  return response;
}