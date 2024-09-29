#include <DHT.h>

#define DHTPIN 10        // Pin where the DHT22 is connected
#define DHTTYPE DHT22    // DHT 22 (AM2302)

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// LED Pins
#define RED_LED_PIN 9    // Red LED for high humidity
#define GREEN_LED_PIN 8   // Green LED for low humidity

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
}

void loop() {
  delay(2000); // Wait a few seconds between measurements

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" °C");

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
}
