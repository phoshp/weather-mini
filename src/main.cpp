#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <Wire.h>

// --- Wi-Fi Credentials ---
const char* ssid = "EmreS21FE";
const char* password = "joker123";

// --- OpenWeatherMap Settings ---
const int LOCATION_COUNT = 8;
String apiKey = "58af577ba7ae4bbdb00c89e9b9198399";
String locations[LOCATION_COUNT][2] = {
  {"Inegol", "TR"},
  {"Balikesir", "TR"},
  {"Istanbul", "TR"},
  {"Ankara", "TR"},
  {"London", "UK"},
  {"Tokyo", "JP"},
  {"New+York", "US"},
  {"Sydney", "AU"},
};
int currentLocation = 0;

U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

static const unsigned char image_ButtonLeft[] U8X8_PROGMEM = {0x08,0x0c,0x0e,0x0f,0x0e,0x0c,0x08};
static const unsigned char image_ButtonRight[] U8X8_PROGMEM = {0x01,0x03,0x07,0x0f,0x07,0x03,0x01};

void drawScreen(const char* city, const char* desc, float temp, int humidity) {
    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);
    u8g2.drawXBMP(68, 0, 4, 7, image_ButtonRight);
    u8g2.drawXBMP(0, -1, 4, 7, image_ButtonLeft);
    
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(20, 6, city);
    u8g2.drawStr(6, 14, desc);

    u8g2.setCursor(5, 23);
    u8g2.printf("temp: %.2f C", temp);

    u8g2.drawEllipse(60, 14, 4, 4);

    u8g2.setCursor(5, 33);
    u8g2.printf("humidity: %%%d", humidity);
}

void setup() {
  Serial.begin(115200);

  pinMode(8, OUTPUT);
  pinMode(GPIO_NUM_8, INPUT_PULLUP);
  pinMode(GPIO_NUM_9, INPUT_PULLUP);

  // Initialize I2C bus and OLED screen
  Wire.begin(5, 6);
  u8g2.begin();
  u8g2.setFont(u8g2_font_5x8_tr); // A small, readable font

  // Show a startup message
  u8g2.clearBuffer();
  u8g2.drawStr(0, 15, "Connecting...");
  u8g2.sendBuffer();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  int step = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    digitalWrite(8, HIGH);
    delay(50);
    digitalWrite(8, LOW);

    u8g2.clearBuffer();
    switch (step % 3) {
      case 0:
        u8g2.drawStr(0, 15, "Connecting.");
        break;
      case 1:
        u8g2.drawStr(0, 15, "Connecting..");
        break;
      case 2:
        u8g2.drawStr(0, 15, "Connecting...");
        break;
    } 
    u8g2.sendBuffer();
    step++;
  }

  Serial.println("\nWiFi Connected!");
  u8g2.clearBuffer();
  u8g2.drawStr(0, 15, "WiFi Connected!");
  u8g2.sendBuffer();
  delay(1000);
}

unsigned long api_call_time = 0;

void drawLoadingScreen(void) {
  u8g2.setFont(u8g2_font_5x8_tr); // A small, readable font

  u8g2.clearBuffer();
  u8g2.drawStr(0, 15, "Loading...");
  u8g2.sendBuffer();
}

void loop() {
  if (digitalRead(GPIO_NUM_9) == LOW) {
    if (currentLocation - 1 < 0) {
      currentLocation = LOCATION_COUNT - 1;
    } else {
      currentLocation -= 1;
    }
    drawLoadingScreen();
    delay(300);
  } else if (digitalRead(GPIO_NUM_8) == LOW) {
    currentLocation = (currentLocation + 1) % LOCATION_COUNT;
    drawLoadingScreen();
    delay(300);
  }

  unsigned long cmillis = millis();
  if ((cmillis - api_call_time) >= 3000 && WiFi.status() == WL_CONNECTED) {
    api_call_time = cmillis;
    HTTPClient http;
    
    String city = locations[currentLocation][0];
    String countryCode = locations[currentLocation][1];
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&appid=" + apiKey + "&units=metric";
    
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      String payload = http.getString();
      
      JsonDocument doc; 
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        float temp = doc["main"]["temp"];
        int humidity = doc["main"]["humidity"];
        const char* desc = doc["weather"][0]["main"];
        
        u8g2.firstPage();
        do {
            drawScreen(city.c_str(), desc, temp, humidity);
        } while (u8g2.nextPage());
        
        Serial.println("Weather updated successfully.");
      } else {
        Serial.println("JSON Parsing failed!");
      }
    } else {
      Serial.print("HTTP Request failed. Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }

  delay(10);
}
