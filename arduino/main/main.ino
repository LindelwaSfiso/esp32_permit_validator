#include "OV7670.h"

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include "BMP.h"

#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

#define ssid1        "HUAWEI-E456"
#define password1    "51B7QLRH371"

#define ssid2         "Lindelwa"
#define password2     "123456789"

// camera configurations
const int SIOD = 21; //SDA
const int SIOC = 22; //SCL

const int VSYNC = 34;
const int HREF = 35;

const int XCLK = 32;
const int PCLK = 33;

const int D0 = 27;
const int D1 = 17;
const int D2 = 16;
const int D3 = 15;
const int D4 = 14;
const int D5 = 13;
const int D6 = 12;
const int D7 = 4;


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
byte rowPins[ROWS] = {19, 18, 5, 3};   // R1, R2, R3, R4
// Connect the keypad columns to these ESP32 pins
byte colPins[COLS] = {26, 25, 23};     // C1, C2, C3

// Create the Keypad object
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


OV7670 *camera;

WiFiMulti wifiMulti;
WiFiServer server(80);

String serverName = "http://192.168.8.101:8000";
String uploadServerPath = serverName + "/api/upload/";
String manualVerificationPath = serverName + "/api/manual-verification/";

WiFiClient client;

unsigned char bmpHeader[BMP::headerSize];


LiquidCrystal_I2C lcd(0x27, 16, 2); // initialize lcd


const unsigned long LONG_PRESS_THRESHOLD = 1000; // 1000 milliseconds (1 second)

void setup()
{
  Serial.begin(115200);

  // setup LCD
  lcd.begin();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting......");

  wifiMulti.addAP(ssid2, password2);
  wifiMulti.addAP(ssid1, password1);

  Serial.println("Connecting Wifi...........");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }


  camera = new OV7670(OV7670::Mode::QQQVGA_RGB565, SIOD, SIOC, VSYNC, HREF, XCLK, PCLK, D0, D1, D2, D3, D4, D5, D6, D7);
  BMP::construct16BitHeader(bmpHeader, camera->xres, camera->yres);

  server.begin();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected WiFi.");
  lcd.setCursor(0, 1);
  lcd.print("");

  keypad.setHoldTime(2000);

  delay(1000);
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
      // the user has pressed the enter key, therefore capture and send the image for processing
      captureAndSendPhoto();
    }
    else if (key == '*') {
      // user has entered manual mode, entering barcode manually
      sendManualVerification();
    }

  }

  // camera->oneFrame();
  // serve();

  delay(100);

}


void captureAndSendPhoto() {
  Serial.println("User has pressed # (enter), begin to send image to the server. Automatic mode....\n\n");
  Serial.println("Connecting to server: " + serverName);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Capturing image.");
  lcd.setCursor(0, 1);
  lcd.print("Please wait.....");

  if (WiFi.status() == WL_CONNECTED) {

    // try to capture image and process it to memory
    /// use the camera and try to get one frame from it
    camera->oneFrame();

    WiFiClient client;
    HTTPClient http;

    const char* filename = "capture.bmp";
    const char* formFieldName = "capture";
    const char* boundary = "POLICE";

    http.begin(client, uploadServerPath.c_str());

    String contentType = "multipart/form-data; boundary=" + String(boundary);
    http.addHeader("Content-Type", contentType.c_str());

    String multipartPayload = "--" + String(boundary) + "\r\n";
    multipartPayload += "Content-Disposition: form-data; name=\"" + String(formFieldName) + "\"; filename=\"" + String(filename) + "\"\r\n";
    multipartPayload += "Content-Type: image/bmp\r\n\r\n";

    uint8_t* frameData = camera->frame;
    size_t frameDataSize = camera->xres * camera->yres * 2;

    // Calculate the total payload size
    size_t payloadSize = multipartPayload.length() + BMP::headerSize + frameDataSize + ("\r\n--" + String(boundary) + "--\r\n").length();


    // Create a buffer to hold the entire payload
    uint8_t* payloadBuffer = (uint8_t*)malloc(payloadSize);
    if (!payloadBuffer) {
      Serial.println("Memory allocation failed!");
      return;
    }

    // Copy the multipart headers into the buffer
    memcpy(payloadBuffer, multipartPayload.c_str(), multipartPayload.length());

    // Copy the BMP header into the buffer
    memcpy(payloadBuffer + multipartPayload.length(), bmpHeader, BMP::headerSize);

    // Copy the frame data into the buffer
    memcpy(payloadBuffer + multipartPayload.length() + BMP::headerSize, frameData, frameDataSize);

    // Append the closing boundary
    String closingBoundary = "\r\n--" + String(boundary) + "--\r\n";
    memcpy(payloadBuffer + multipartPayload.length() + BMP::headerSize + frameDataSize, closingBoundary.c_str(), closingBoundary.length());

    int statusCode = http.POST(payloadBuffer, payloadSize);

    if (statusCode == 200) {
      // successfully send image to server
      Serial.println("SENT IMAGE TO SERVER: " + statusCode);
      Serial.println();
      String payload = http.getString();
      proccessServerResponse(payload);
      delay(3000);
    }

    else {
      // can't connect to internet, timeout
      Serial.print("Timeout, cannot connect...: ");
      Serial.println(statusCode);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Timeout.......");
      lcd.setCursor(0, 1);
      lcd.print("CAN'T CONNECT.");

      delay(3000);
    }

    http.end();
    free(payloadBuffer); // Free the allocated memory

  } else {
    Serial.println("Not CONNECTED TO WIFI!!!!");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Your WiFi has   ");
    lcd.setCursor(0, 1);
    lcd.print("Disconnected....");

    delay(3000);
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
          break; // Exit loop when '#' is pressed
        }
        else if (key == '*') {
          if (manualBarcode.length() == 0) {
            // if the user previously cleared, exit now

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Exiting manual.");
            lcd.setCursor(0, 1);
            lcd.print("barcode input..");
            Serial.println("Exiting manual barcode input...");
            delay(2000);

            printDefaults();
            return; // the user has cancelled manual input mode.
          }

          manualBarcode = "";
          Serial.println("\n\nBarcode cleared..");
          Serial.println("Enter code via keypad (press #: to finish), (*: exit)");

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Barcode cleared");
          lcd.setCursor(0, 1);
          lcd.print("#: Done *: Exit");
        }
        else {
          if (key != '#')
            manualBarcode += key;

          Serial.print(key);

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("->");
          lcd.print(manualBarcode);
          lcd.setCursor(0, 1);
          lcd.print("#: Done *: Clear");
        }

      }
      delay(50); // Debounce
    }

    Serial.println("Connecting to server: " + serverName);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sending barcode.");
    lcd.setCursor(0, 1);
    lcd.print("Please wait.....");
    delay(1000);


    http.begin(client, manualVerificationPath.c_str());
    // configure http, set content-type to JSON
    http.addHeader("Content-Type", "application/json");


    // Construct HTTP Request Payload as JSON
    // Server expecting this format => {"barcode": "212313131"}
    String httpRequestData = "{\"barcode\": \"";
    httpRequestData.concat(manualBarcode);
    httpRequestData.concat("\"}");


    // Send HTTP POST request
    int statusCode = http.POST(httpRequestData);


    if (statusCode == 200) {
      // successfully send barcode to server
      Serial.println("SUCCESS SENT CODE VIA KEYAD: " + statusCode);
      Serial.println();
      String payload = http.getString();
      proccessServerResponse(payload);
      delay(3000);
    }

    else if (statusCode == 403) {
      // failed, barcode has expired
      Serial.println("Tax has expired: " + statusCode);
      Serial.println();
      String payload = http.getString();
      proccessServerResponse(payload);
      delay(3000);
    }

    else if (statusCode == 404) {
      // barcode not found on our database
      Serial.println("Barcode not found on our database: " + statusCode);
      Serial.println();
      String payload = http.getString();
      proccessServerResponse(payload);
      delay(3000);
    }

    else {
      // can't connect to internet, timeout
      Serial.print("Cant connect, connection timeout:  ");
      Serial.println(statusCode);


      // can't connect to internet, timeout
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("TImeout........");
      lcd.setCursor(0, 1);
      lcd.print("Can't connect.");

      delay(3000);
    }

    // Free resources
    http.end();

  } else {
    Serial.println("Not CONNECTED WIFI!!!!\n");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Your WiFi has   ");
    lcd.setCursor(0, 1);
    lcd.print("Disconnected....");

    delay(3000);
  }


}


void proccessServerResponse(String payload) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {

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

    delay(3000);
  }

  printDefaults();
}


void printDefaults() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("#:Capture & Send");
  lcd.setCursor(0, 1);
  lcd.print("*:Manual Mode...");

  Serial.println();
  Serial.println("#:Capture & Send");
  Serial.println("*:Manual Mode...\n");
}


void serve()
{
  WiFiClient client = server.available();
  if (client)
  {
    Serial.println("New Client.");
    String currentLine = "";
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        Serial.write(c);
        if (c == '\n')
        {
          if (currentLine.length() == 0)
          {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head>");
            client.print(
              "<style>body{margin: 0; height: 100vh; width: 100%}\nimg{height: 50%; width: 50%}</style>"
              "<div style=\"height: 100%; width: 100%; display: flex; justify-content: center; align-items: center;\">"
              "<img id='camera-feed' src='/camera' style=\"height: 50%; width: 50%;\" />"
              "</div>"
              "<script type=\"text/javascript\">"
              "window.addEventListener('load', function () {"
              "setInterval(function() {"
              "var myImageElement = document.getElementById('camera-feed');"
              "myImageElement.src = '/camera?rand=' + Math.random();"
              "}, 2000);"
              "})"
              "</script>"
            );
            client.println();
            break;
          }
          else
          {
            currentLine = "";
          }
        }
        else if (c != '\r')
        {
          currentLine += c;
        }

        if (currentLine.endsWith("GET /camera"))
        {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:image/jpeg");
          client.println();

          client.write(bmpHeader, BMP::headerSize);
          client.write(camera->frame, camera->xres * camera->yres * 2);
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}




//void captureAndSendPhoto1() {
//  Serial.println("User has pressed * (enter), begin to send image to the server. Automatic mode....\n\n");
//
//  String getAll;
//  String getBody;
//
//  Serial.println("Connecting to server: " + serverName);
//
//  if (client.connect(serverName.c_str(), serverPort)) {
//    Serial.println("Connection successful!");
//    Serial.println();
//
//    String head = "--POLICE\r\nContent-Disposition: form-data; name=\"capture\"; filename=\"esp32.bmp\"\r\nContent-Type: image/bmp\r\n\r\n";
//    String tail = "\r\n--POLICE--\r\n";
//
//    uint32_t imageLen = BMP::headerSize;
//    uint32_t extraLen = head.length() + tail.length();
//    uint32_t totalLen = imageLen + extraLen;
//
//    client.println("POST " + uploadServerPath + " HTTP/1.1");
//    client.println("Host: " + serverName);
//    client.println("Content-Length: " + String(totalLen));
//    client.println("Content-Type: multipart/form-data; boundary==POLICE");
//    client.println();
//    client.print(head);
//
//    client.write(bmpHeader, BMP::headerSize);
//    client.write(camera->frame, camera->xres * camera->yres * 2);
//
//    client.print(tail);
//
//    int timoutTimer = 6000;
//    long startTimer = millis();
//    boolean state = false;
//
//    while ((startTimer + timoutTimer) > millis()) {
//      Serial.print(".");
//      delay(100);
//
//      while (client.available()) {
//        char c = client.read();
//        if (c == '\n') {
//          if (getAll.length() == 0) {
//            state = true;
//          }
//          getAll = "";
//        } else if (c != '\r') {
//          getAll += String(c);
//        }
//        if (state == true) {
//          getBody += String(c);
//        }
//        startTimer = millis();
//      }
//      if (getBody.length() > 0) {
//        break;
//      }
//    }
//    Serial.println();
//    client.stop();
//    Serial.println("SEVER RESPONSE: ");
//    Serial.println(getBody);
//
//  } else {
//    getBody = "Connection to " + serverName + " failed.";
//    Serial.println(getBody);
//    Serial.println();
//  }
//}
