#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <DHT.h>

// WiFi Credentials
#define WIFI_SSID "Airtel_sara_0906"
#define WIFI_PASSWORD "air53712"

// Firebase Credentials
#define FIREBASE_HOST "bit-896e2-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "https://bit-896e2-default-rtdb.firebaseio.com" // Replace with your actual database secret

// Sensor Pins
#define DHTPIN D4
#define DHTTYPE DHT22
#define SOIL_MOISTURE_PIN A0

// Firebase Objects
FirebaseConfig config;
FirebaseAuth auth;
FirebaseData firebaseData;

// DHT Sensor
DHT dht(DHTPIN, DHTTYPE);

void setup() {
    Serial.begin(115200);
    Serial.println("Starting...");

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("Connected to WiFi");

    // Firebase Setup
    config.host = FIREBASE_HOST;
    config.signer.tokens.legacy_token = FIREBASE_AUTH;  // Use legacy token for authentication
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    firebaseData.setResponseSize(128);  // Reduce buffer size for lower RAM usage

    dht.begin();
    Serial.println("DHT sensor initialized.");
}

void loop() {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 10000) {  // Update every 10 seconds
        lastUpdate = millis();
        
        float temperature = dht.readTemperature();
        float humidity = dht.readHumidity();
        int soilMoisture = analogRead(SOIL_MOISTURE_PIN);

        // Check if the readings are valid
        if (!isnan(temperature) && !isnan(humidity)) {
            Serial.println("Readings are valid.");
            // Send data to Firebase
            if (Firebase.setFloat(firebaseData, "/temp", temperature)) {
                Serial.println("Temperature sent to Firebase: " + String(temperature));
            } else {
                Serial.println("Failed to send temperature: " + firebaseData.errorReason());
            }

            if (Firebase.setFloat(firebaseData, "/hum", humidity)) {
                Serial.println("Humidity sent to Firebase: " + String(humidity));
            } else {
                Serial.println("Failed to send humidity: " + firebaseData.errorReason());
            }

            if (Firebase.setInt(firebaseData, "/soil", soilMoisture)) {
                Serial.println("Soil moisture sent to Firebase: " + String(soilMoisture));
            } else {
                Serial.println("Failed to send soil moisture: " + firebaseData.errorReason());
            }
        } else {
            Serial.println("Failed to read from DHT sensor!");
        }
    }
}