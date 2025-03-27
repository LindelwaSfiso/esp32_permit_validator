#include <Keypad.h>

// Keypad configuration
const byte ROWS = 4; // Number of rows
const byte COLS = 3; // Number of columns

// Define the keymap
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

// Connect the keypad rows to these ESP32 pins
byte rowPins[ROWS] = {19, 18, 5, 3}; // R1, R2, R3, R4
// Connect the keypad columns to these ESP32 pins
byte colPins[COLS] = {26, 25, 23};     // C1, C2, C3

// Create the Keypad object
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 4x3 Keypad Test");
}

void loop() {
  // Check for key press
  char key = keypad.getKey();

  // If a key is pressed, print it to serial monitor
  if (key) {
    Serial.print("Key Pressed: ");
    Serial.println(key);
  }
}
