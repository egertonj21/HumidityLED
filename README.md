# HumidityLED
Quick bit of code for an ESP32C with DHT22 sensor &amp; RGB LED 

DHT22 data  connects to D10 on the esp32 (I used xiao seeed), LED red connector goes to 9 and green to 8, didn't bother using the blue for this project.

If humidity is below 50 the LED lights green, 50-60 orange, and over 60 red.

I intend to add some additional code to connect this to wifi and send data via MQTT to a database and webapp.
