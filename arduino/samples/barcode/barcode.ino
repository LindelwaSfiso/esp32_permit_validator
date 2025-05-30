// No need to include <SoftwareSerial.h>

// Define the pins (These are often default for Serial2, confirm with your board's pinout)
#define RX_PIN 16
#define TX_PIN 17

void setup() {
  Serial.begin(115200); // For USB Serial Monitor
  // Initialize Serial2 with the scanner's baud rate and specified pins
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // <--- REPLACE 9600 with your scanner's actual baud rate
  Serial.println("ESP32 Ready. Waiting for barcode scans on Serial2...");
}

void loop() {
  if (Serial2.available()) {
    String receivedData = Serial2.readStringUntil('\n');
    Serial.print("Received from scanner: ");
    Serial.println(receivedData);
  }
}
