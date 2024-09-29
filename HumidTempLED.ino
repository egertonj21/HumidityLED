#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define DHTPIN 10        // Pin where the DHT22 is connected
#define DHTTYPE DHT22    // DHT 22 (AM2302)

// Wi-Fi credentials
const char* ssid = "YOUR_SSID";              // Replace with your Wi-Fi SSID
const char* password = "YOUR_PASSWORD";       // Replace with your Wi-Fi password

// MQTT broker details
const char* mqtt_server = "YOUR_MQTT_BROKER_IP";  // Replace with your MQTT broker IP
const char* publish_topic = "sensor/data";   // Topic to publish temperature and humidity data

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// MQTT Client
WiFiClient espClient;
PubSubClient client(espClient);

// LED Pins
#define RED_LED_PIN 9    // Red LED for high humidity
#define GREEN_LED_PIN 8   // Green LED for low humidity

// Timing variables
unsigned long lastPublishTime = 0;  // Time for last publish
const unsigned long publishInterval = 3600000;  // 1 hour in milliseconds
const unsigned long printInterval = 10000;        // 10 seconds in milliseconds

void setup() {
    Serial.begin(115200);
    dht.begin();

    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Set MQTT server
    client.setServer(mqtt_server, 1883);
}

void loop() {
    // Check if MQTT client is connected
    if (!client.connected()) {
        reconnect();
    }
    client.loop();  // Keep the connection alive

    // Read temperature and humidity
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Check if readings failed
    if (isnan(h) || isnan(t)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    // Print temperature and humidity to Serial Monitor every 10 seconds
    static unsigned long lastPrintTime = 0;
    if (millis() - lastPrintTime >= printInterval) {
        Serial.print("Humidity: ");
        Serial.print(h);
        Serial.print(" %\t");
        Serial.print("Temperature: ");
        Serial.print(t);
        Serial.println(" °C");
        lastPrintTime = millis();
    }

    // Control LEDs based on humidity
    if (h < 50) {
        // Low humidity - Turn on green LED
        digitalWrite(GREEN_LED_PIN, HIGH);
        digitalWrite(RED_LED_PIN, LOW);
    } else if (h >= 50 && h <= 60) {
        // Moderate humidity - Turn on both red and green for orange
        digitalWrite(GREEN_LED_PIN, HIGH);
        digitalWrite(RED_LED_PIN, HIGH);
    } else {
        // High humidity - Turn on red LED
        digitalWrite(RED_LED_PIN, HIGH);
        digitalWrite(GREEN_LED_PIN, LOW);
    }

    // Publish temperature and humidity to MQTT topic every hour
    if (millis() - lastPublishTime >= publishInterval) {
        char payload[64];
        sprintf(payload, "Temp: %.2f, Humidity: %.2f", t, h);
        client.publish(publish_topic, payload);
        Serial.println("Published: " + String(payload));
        lastPublishTime = millis();
    }
}

void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect("ESP32Client")) {
            Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}
