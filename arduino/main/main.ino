/**
Topic: Barcode scanner for traffic cops - checking if driver has valid permits
*/


#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//

// CONFIG NOTES:
// BOARD: AI THINKER ESP32-CAM


#define CAMERA_MODEL_AI_THINKER  // Has PSRAM


#include "camera_pins.h"


const char* ssid = "HUAWEI-E456";
const char* password = "51B7QLRH371";


String serverName = "http://192.168.8.102:8000";
String serverPath = "/api/upload/";

const int timerInterval = 3000;    // time between each HTTP server upload, 3s
unsigned long previousMillis = 0;  // last time captured image was sent

const int serverPort = 443;  // http use 80

WiFiClient client;


#define BUZZER 14

void startCameraServer();


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t* s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  // begin connetion to the WiFi, using credentials
  WiFi.begin(ssid, password);

  Serial.println("Connecting.");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  delay(200);
}

void loop() {
  // put your main code here, to run repeatedly:

  if ((millis() - previousMillis) >= timerInterval) {
    captureAndSendPhoto();
    previousMillis = millis();
  }

  delay(1000);
}



String captureAndSendPhoto() {
  String getAll;
  String getBody;

  camera_fb_t* fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    Serial.println();
    delay(100);
    // ESP.restart();
  }

  Serial.println("Connecting to server: " + serverName);

  // for http: client.connect(serverName.c_str())
  if (client.connect(serverName.c_str(), serverPort)) {
    Serial.println("Connection successful!");
    Serial.println();

    String head = "--HomeSecurity\r\nContent-Disposition: form-data; name=\"capture\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--HomeSecurity--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    client.println("POST " + serverPath + " HTTP/1.1");
    client.println("Host: " + serverName);
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=HomeSecurity");
    client.println();
    client.print(head);

    uint8_t* fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n = n + 1024) {
      if (n + 1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      } else if (fbLen % 1024 > 0) {
        size_t remainder = fbLen % 1024;
        client.write(fbBuf, remainder);
      }
    }
    client.print(tail);

    esp_camera_fb_return(fb);

    int timoutTimer = 6000;
    long startTimer = millis();
    boolean state = false;

    while ((startTimer + timoutTimer) > millis()) {
      Serial.print(".");
      delay(100);

      while (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (getAll.length() == 0) { state = true; }
          getAll = "";
        } else if (c != '\r') {
          getAll += String(c);
        }
        if (state == true) { getBody += String(c); }
        startTimer = millis();
      }
      if (getBody.length() > 0) { break; }
    }


    Serial.println();
    client.stop();
    Serial.println("SEVER RESPONSE: ");
    Serial.println(getBody);

  } else {
    getBody = "Connection to " + serverName + " failed.";
    Serial.println(getBody);
    Serial.println();
  }

  return getBody;
}