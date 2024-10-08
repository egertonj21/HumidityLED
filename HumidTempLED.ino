#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define DHTPIN 10        // Pin where the DHT22 is connected
#define DHTTYPE DHT22    // DHT 22 (AM2302)

// Wi-Fi credentials
const char* ssid = "wifiSSD";              // Replace with your Wi-Fi SSID
const char* password = "WifiPass";       // Replace with your Wi-Fi password

// MQTT broker details
const char* mqtt_server = "MQTTIP";  // Replace with your MQTT broker IP
const char* publish_topic = "sensor/data";   // Topic to publish temperature and humidity data
const char* ack_topic = "sensor/ack";        // Topic to receive acknowledgment for the published data
const char* request_topic = "sensor/request"; // Topic to subscribe to for sensor data requests

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
const unsigned long ackTimeout = 5000;            // Timeout waiting for ack (5 seconds)

// State variables
bool ledControlEnabled = true;  // Flag to control whether LED logic is active
bool ackReceived = false;       // Flag to track if ACK was received

// Sensor data to resend
char lastPayload[64];

// Timing for ACK
unsigned long lastSendTime = 0;

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

    // Set MQTT server and callback function
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    // Ensure MQTT connection
    reconnect();
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

    // Control LEDs based on humidity if LED control is enabled
    if (ledControlEnabled) {
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
    } else {
        // LED control is disabled, so turn off both LEDs
        digitalWrite(RED_LED_PIN, LOW);
        digitalWrite(GREEN_LED_PIN, LOW);
    }

    // Publish temperature and humidity to MQTT topic every hour or if ACK is not received
    if (millis() - lastPublishTime >= publishInterval || !ackReceived) {
        if (millis() - lastSendTime > ackTimeout && !ackReceived) {
            sprintf(lastPayload, "Temp: %.2f, Humidity: %.2f", t, h);
            client.publish(publish_topic, lastPayload);
            Serial.println("Published: " + String(lastPayload));
            lastSendTime = millis();
            ackReceived = false; // Expect ACK
        }
    }
}

// MQTT callback function to handle messages
void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("Message arrived on topic: ");
    Serial.println(topic);
    Serial.print("Message: ");
    Serial.println(message);

    // Handle ACK for sensor data
    if (String(topic) == ack_topic) {
        if (message == "ACK") {
            ackReceived = true;  // Acknowledgment received, stop resending
            Serial.println("ACK received, stopping resend");
        }
    }

    // Handle requests for sensor data
    if (String(topic) == request_topic) {
        if (message == "GET_DATA") {
            float h = dht.readHumidity();
            float t = dht.readTemperature();
            char response[64];
            sprintf(response, "Temp: %.2f, Humidity: %.2f", t, h);
            client.publish("sensor/response", response);
            Serial.println("Responded with sensor data: " + String(response));
        }
    }

    // Handle LED control (turn on/off LED logic)
    if (String(topic) == "led/control") {
        if (message == "LED_CONTROL_ON") {
            ledControlEnabled = true;
            Serial.println("LED control enabled");
        } else if (message == "LED_CONTROL_OFF") {
            ledControlEnabled = false;
            Serial.println("LED control disabled");
        }
    }
}

void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect("ESP32Client")) {
            Serial.println("connected");
            // Subscribe to the LED control, ack, and request topics
            client.subscribe("led/control");
            client.subscribe(ack_topic);
            client.subscribe(request_topic);
            Serial.println("Subscribed to necessary topics");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}
