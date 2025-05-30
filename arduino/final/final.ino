// MTHANDENI
// B.ENG

// ===========================
// Imports
// ===========================
#include <WiFi.h>
#include <HTTPClient.h>

#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// ===========================
// Enter your WiFi credentials
// ===========================
const char* ssid = "Lindelwa";
const char* password = "123456789";

// ===========================
// Keypad configuration
// ===========================
const byte ROWS = 4;  // Number of rows
const byte COLS = 3;  // Number of columns

// Define the keymap
char keys[ROWS][COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};

// Connect the keypad rows to these ESP32 pins
byte rowPins[ROWS] = { 12, 14, 27, 26 };  // R1, R2, R3, R4
// Connect the keypad columns to these ESP32 pins
byte colPins[COLS] = { 25, 33, 32 };  // C1, C2, C3

// Create the Keypad object
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


// Letters/symbols assigned to each key (in order of taps)
const char* keyChars[ROWS][COLS] = {
  { "1!", "2ABC", "3DEF" },      // 1: 1 → ! | 2: A → B → C | 3: D → E → F
  { "4GHI", "5JKL", "6MNO" },    // 4: G → H → I | etc.
  { "7PQRS", "8TUV", "9WXYZ" },  // 7: P → Q → R → S | 9: W → X → Y → Z
  { "*", "0 ", "#" }             // *: * | 0: SPACE | #: #
};

// Variables for multi-tap detection
unsigned long lastPressTime = 0;
char lastKey = '\0';
byte tapCount = 0;



// ===========================
// I2C LCD configuration
// ===========================
LiquidCrystal_I2C lcd(0x27, 16, 2);


// ===========================
// Server details
// ===========================
String serverName = "http://192.168.8.101:8000";
String verifyServerPath = serverName + "/api/verify/";


// ===========================
// Barcode module setup
// ===========================
#define RX_PIN 16
#define TX_PIN 17

#define SCANNER_READ_TIMEOUT_MS 5000

// use this command to send a scan command to the barcode scanner
const byte SCAN_COMMAND[] = { 0x7E, 0x00, 0x08, 0x01, 0x00, 0x02, 0x01, 0xAB, 0xCD };
const size_t SCAN_COMMAND_LEN = sizeof(SCAN_COMMAND);

const byte SCAN_RESPONSE_EXPECTED[] = { 0x02, 0x00, 0x00, 0x01, 0x00, 0x33, 0x31 };
const size_t SCAN_RESPONSE_LEN = sizeof(SCAN_RESPONSE_EXPECTED);


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  // setup LCD
  lcd.begin();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi......");

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected: ");
  Serial.print(WiFi.localIP());
  Serial.println("");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected WiFi.");
  lcd.setCursor(0, 1);
  lcd.print(String(ssid));

  keypad.setHoldTime(2000);

  delay(3000);
  printDefaults();
}


void loop() {

  // Check for key press
  char key = keypad.getKey();

  // If a key is pressed, check ->
  // - if # is pressed capture image and send for processing
  // - if long any key enter manual barcode input
  if (key) {
    Serial.print("Key Pressed: ");
    Serial.println(key);

    if (key == '#') {
      // the user has pressed the enter key, therefore capture and send the barcode for processing
      captureAndSendBarcode();
    } else if (key == '*') {
      // user has entered manual mode, entering barcode manually
      sendManualVerification();
    }
  }

  delay(100);
}



void printDefaults() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("#:Scan & Send...");
  lcd.setCursor(0, 1);
  lcd.print("*:Manual Mode...");

  Serial.println();
  Serial.println("#:Scan & Send...");
  Serial.println("*:Manual Mode...\n");
}

void captureAndSendBarcode() {
  // ==================================
  // Remote Upload Server configuration
  // ==================================
  Serial.println("User has entered automatic mode, will scan and capture barcode..\n");

  WiFiClient client;
  HTTPClient http;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scanning.......");
  lcd.setCursor(0, 1);
  lcd.print("Please wait.....");

  if (!sendCommandAndVerifyResponse(SCAN_COMMAND, SCAN_COMMAND_LEN, SCAN_RESPONSE_EXPECTED, SCAN_RESPONSE_LEN, 1000)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Scan CMD FAILED!!");
    lcd.setCursor(0, 1);
    lcd.print("reboot device....");
    delay(3000);
    printDefaults();  // Go back to default display
    return;           // Exit if scanner command fails
  }


  String scannedBarcode = getBarcode();

  if (scannedBarcode.length() > 0) {


    if (WiFi.status() == WL_CONNECTED) {

      http.begin(client, verifyServerPath.c_str());
      // configure http, set content-type to JSON
      http.addHeader("Content-Type", "application/json");

      // Construct HTTP Request Payload as JSON
      // Server expecting this format => {"barcode": "ASD3412BH"}
      String httpRequestData = "{\"barcode\": \"";
      httpRequestData.concat(scannedBarcode);
      httpRequestData.concat("\"}");


      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Verifying tax....");
      lcd.setCursor(0, 1);
      lcd.print("Please wait.....");
      delay(1500);


      // Send HTTP POST request
      int statusCode = http.POST(httpRequestData);


      // this is a very wrong way of handling apis, the server always return 200, haha!!
      if (statusCode == 200) {
        // successfully send barcode to server
        String payload = http.getString();
        proccessServerResponse(payload);
      }

      else if (statusCode == 403) {
        // failed, barcode has expired
        String payload = http.getString();
        proccessServerResponse(payload);
      }

      else if (statusCode == 404) {
        // barcode not found on our database
        String payload = http.getString();
        proccessServerResponse(payload);
      }

      else {
        // can't connect to internet, timeout
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Timeout........");
        lcd.setCursor(0, 1);
        lcd.print("Can't connect.");

        delay(3000);
        printDefaults();
      }
      http.end();

    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Your WiFi has   ");
      lcd.setCursor(0, 1);
      lcd.print("Disconnected....");

      delay(3000);
      printDefaults();
    }

  } else {
    // No barcode was scanned within the timeout after commanding
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No Barcode Read!");
    lcd.setCursor(0, 1);
    lcd.print("...Try again...");

    delay(3000);
    printDefaults();
  }
}



void sendManualVerification() {
  // function to send manual verification to the server
  // simple send an http post to the /manual-verification endpoint with the driver's barcode tag
  Serial.println("User has entered manual mode, begin to process keypad to the server\n");

  if (WiFi.status() == WL_CONNECTED) {

    WiFiClient client;
    HTTPClient http;

    // capture keypad press
    String manualBarcode = "";
    char key;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter code......");
    lcd.setCursor(0, 1);
    lcd.print("#: Done *: Clear");

    Serial.println("Enter code via keypad (#: finish), (*: clear):");

    while (true) {
      key = keypad.getKey();
      if (key) {
        if (key == '#' && manualBarcode.length() > 0) {
          Serial.println("\n\nDone...");
          // user has confirmed barcode, therefore procceed and process the request to the server
          break;  // Exit loop when '#' is pressed
        } else if (key == '*') {
          if (manualBarcode.length() == 0) {
            // if the user previously cleared, exit now
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Exiting manual.");
            lcd.setCursor(0, 1);
            lcd.print("barcode input..");
            delay(2000);

            printDefaults();
            return;  // the user has cancelled manual input mode.
          }

          // attempt to backspace
          manualBarcode.remove(manualBarcode.length() - 1);

          if (manualBarcode.length() == 0) {
            // show exit command
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Barcode cleared");
            lcd.setCursor(0, 1);
            lcd.print("        *: Exit");

          } else {
            // else print the remaining barcode after clearing
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("->");
            lcd.print(manualBarcode);
            lcd.setCursor(0, 1);
            lcd.print("#: Done *: Clear");
          }

        } else {
          if (key != '#') {
            // this is where we process the keyad input
            // we try to adapt 4x3 keypad to accept alphanumeric values
            if (processMultiTap(key) != '\0') manualBarcode += key;
          }

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("->");
          lcd.print(manualBarcode);
          lcd.setCursor(0, 1);
          lcd.print("#: Done *: Clear");
        }
      }
      delay(50);  // Debounce
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Verifying tax....");
    lcd.setCursor(0, 1);
    lcd.print("Please wait.....");
    delay(1500);


    http.begin(client, verifyServerPath.c_str());
    // configure http, set content-type to JSON
    http.addHeader("Content-Type", "application/json");


    // Construct HTTP Request Payload as JSON
    // Server expecting this format => {"barcode": "ASD3412BH"}
    String httpRequestData = "{\"barcode\": \"";
    httpRequestData.concat(manualBarcode);
    httpRequestData.concat("\"}");


    // Send HTTP POST request
    int statusCode = http.POST(httpRequestData);


    if (statusCode == 200) {
      // successfully send barcode to server
      String payload = http.getString();
      proccessServerResponse(payload);
    }

    else if (statusCode == 403) {
      // failed, barcode has expired
      String payload = http.getString();
      proccessServerResponse(payload);
    }

    else if (statusCode == 404) {
      // barcode not found on our database
      String payload = http.getString();
      proccessServerResponse(payload);
    }

    else {
      // can't connect to internet, timeout
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Timeout........");
      lcd.setCursor(0, 1);
      lcd.print("Can't connect.");

      delay(3000);
      printDefaults();
    }

    // Free resources
    http.end();

  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Your WiFi has   ");
    lcd.setCursor(0, 1);
    lcd.print("Disconnected....");

    delay(3000);
    printDefaults();
  }
}


void proccessServerResponse(String payload) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Failed to parse");
    lcd.setCursor(0, 1);
    lcd.print("server data....");

    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());

  } else {

    const char* lcd1 = doc["lcd1"];
    const char* lcd2 = doc["lcd2"];
    const char* details = doc["details"];

    Serial.print("Details:  ");
    Serial.println(details);
    Serial.print("LCD1: ");
    Serial.println(lcd1);
    Serial.print("LCD2: ");
    Serial.println(lcd2);
    Serial.println("\n");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(lcd1);
    lcd.setCursor(0, 1);
    lcd.print(lcd2);
  }

  delay(3000);
  printDefaults();
}


String getBarcode() {
  unsigned long startTime = millis();  // Record the start time

  // Loop while there's no data AND we haven't exceeded the timeout
  while (!Serial2.available() && (millis() - startTime < SCANNER_READ_TIMEOUT_MS)) {
    // Optional: Add a small delay to prevent busy-waiting
    delay(50);  // Adjust this if you need more responsiveness or less CPU usage
  }

  if (Serial2.available()) {
    String receivedData = Serial2.readStringUntil('\n');
    Serial.print("Received from scanner: ");
    Serial.println(receivedData);
    return receivedData;
  } else {
    // If the timeout occurred and no data was received
    Serial.println("Barcode scan timed out. No data received from scanner.");
    return "";  // Return an empty string or handle the timeout case as needed
  }
}


// Function to send a command to the scanner and verify its response
bool sendCommandAndVerifyResponse(
  const byte* command, size_t commandLen,
  const byte* expectedResponse, size_t expectedLen,
  unsigned long timeout) {

  // Clear any old data in the serial buffer before sending command
  while (Serial2.available()) {
    Serial2.read();
  }

  Serial.print("Sending command to scanner: ");
  for (size_t i = 0; i < commandLen; i++) {
    Serial.printf("%02X ", command[i]);
    Serial2.write(command[i]);  // Send byte by byte
  }
  Serial.println();

  unsigned long startTime = millis();
  byte receivedBytes[expectedLen];
  size_t receivedCount = 0;

  while (millis() - startTime < timeout) {
    if (Serial2.available()) {
      receivedBytes[receivedCount] = Serial2.read();
      Serial.printf("Received byte: %02X (expected %02X)\n", receivedBytes[receivedCount], expectedResponse[receivedCount]);  // Debugging received bytes

      // Check if the received byte matches the expected byte at the current position
      if (receivedBytes[receivedCount] == expectedResponse[receivedCount]) {
        receivedCount++;
        if (receivedCount == expectedLen) {
          Serial.println("Scanner response matched!");
          return true;  // Command successful, response received and matched
        }
      } else {
        // Mismatch: reset count and start looking for the response from the beginning
        Serial.println("Scanner response mismatch, resetting buffer.");
        receivedCount = 0;  // Reset to look for the start of the response again
        // Important: If the mismatch happens early, you might need to consume the rest of the current
        // invalid sequence or implement a more robust state machine for complex responses.
        // For a fixed, short response, resetting is usually fine.
      }
    }
    delay(1);  // Small delay to prevent busy-waiting
  }
  Serial.println("Scanner command response timed out or mismatch.");
  return false;  // Timeout or mismatch
}


char processMultiTap(char key) {
  unsigned long currentTime = millis();
  unsigned long timeDiff = currentTime - lastPressTime;

  char selectedChar = '\0';

  // Reset tap count if a new key is pressed or timeout (500ms)
  if (key != lastKey || timeDiff > 500) {
    tapCount = 0;
    lastKey = key;
  }

  // Find the key's position in the keymap
  for (byte i = 0; i < ROWS; i++) {
    for (byte j = 0; j < COLS; j++) {
      if (keys[i][j] == key) {
        tapCount++;
        byte maxTaps = strlen(keyChars[i][j]);

        // Wrap around if exceeding max taps
        if (tapCount >= maxTaps) tapCount = 0;

        // Get the character from the multi-tap sequence
        selectedChar = keyChars[i][j][tapCount];
        Serial.print("Pressed: ");
        Serial.println(selectedChar);
        break;
      }
    }
  }

  lastPressTime = currentTime;


  return selectedChar;
}
