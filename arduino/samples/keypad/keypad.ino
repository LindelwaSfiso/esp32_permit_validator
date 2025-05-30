#include <Keypad.h>
#include <LiquidCrystal_I2C.h>


LiquidCrystal_I2C lcd(0x27, 16, 2);

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
byte rowPins[ROWS] = {14, 13, 3, 38}; // R1, R2, R3, R4
// Connect the keypad columns to these ESP32 pins
byte colPins[COLS] = {7, 18, 12};     // C1, C2, C3

// Create the Keypad object
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 4x3 Keypad Test");

  // setup LCD
  lcd.begin();
  lcd.backlight();

  lcd.print("Keypad Ready");
  delay(1000);
}

void loop() {
  // Check for key press
  char key = keypad.getKey();

  // If a key is pressed, print it to serial monitor
  if (key) {
    Serial.print("Key Pressed: ");
    Serial.println(key);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Key: ");
    lcd.print(key);

    delay(2000);
  } else {
    // Optional: Display a default message when no key is pressed
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No Key Pressed");
  }

  delay(100); // Small delay to debounce the keypad

}
