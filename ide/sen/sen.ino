#include <DHT.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>

// WiFi Credentials
#define WIFI_SSID "Airtel_sara_0906"  
#define WIFI_PASSWORD "air53712"

// Firebase Credentials
#define API_KEY "AIzaSyDCbfNAZeNHQEIfCqCaTwPa-FzO1EWzeIc"
#define DATABASE_URL "https://iitm-8d97f-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Firebase Objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// DHT Sensor
#define DHTPIN D2  
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Soil Moisture Sensor
#define SOIL_MOISTURE_PIN A0  

// Timer
unsigned long sendDataPrevMillis = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\nStarting...");

  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    attempt++;
    if (attempt > 20) {  // Restart ESP8266 if WiFi fails
      Serial.println("\nWiFi Connection Failed! Restarting...");
      ESP.restart();
    }
  }

  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Firebase Setup
  Serial.println("Initializing Firebase...");
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase Initialized!");

  // Initialize Sensors
  dht.begin();
  pinMode(SOIL_MOISTURE_PIN, INPUT);
}

void loop() {
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 3000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    Serial.println("\nReading Sensor Data...");

    // Read Sensor Data
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    int soilMoisture = analogRead(SOIL_MOISTURE_PIN);

    // Check if DHT Sensor is working
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("⚠️ DHT Sensor Error! Check wiring.");
      return;  // Skip sending to Firebase if DHT fails
    }

    // Print Sensor Data
    Serial.print("Humidity: "); Serial.print(humidity); Serial.println("%");
    Serial.print("Temperature: "); Serial.print(temperature); Serial.println("°C");
    Serial.print("Soil Moisture: "); Serial.println(soilMoisture);

    // Send Data to Firebase
    bool success = true;
    
    success &= Firebase.RTDB.setFloat(&fbdo, "/humidity", humidity);
    success &= Firebase.RTDB.setFloat(&fbdo, "/temperature", temperature);
    success &= Firebase.RTDB.setInt(&fbdo, "/moisture", soilMoisture);

    if (success) {
      Serial.println("✅ Data Sent Successfully!");
    } else {
      Serial.println("❌ Firebase Send Failed! Error: " + fbdo.errorReason());
    }

    // Monitor Free Heap Memory
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());
  }
}
