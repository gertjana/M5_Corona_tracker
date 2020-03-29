#include <M5Stack.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "ArduinoJson.h"
#include "secrets.h"

#define NR_COUNTRIES 5
const int REFRESH_RATE = 60; //seconds
const String countries[NR_COUNTRIES]  = {"gb","us","it", "es", "cn"};

int cnt_http = 0;
bool refresh_http = true;
int confirmed = 0;
int recovered = 0;
int deaths = 0;
String scope = "World";
String path = "/totals";
int country_index = 0;
StaticJsonDocument<300> doc;



void setup(void) {
  Serial.begin(115200);
  while(!Serial);    // time to get serial running
  Serial.println("Corona Tracker");
    
  M5.begin(true, false, true); //lcd + serial enabled, sd card disabled
  M5.Power.begin();
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected with IP address: ");
  Serial.println(WiFi.localIP());
}

void update_display() {
  M5.Lcd.fillScreen(TFT_BLACK); 
  M5.Lcd.setCursor(0, 0, 2);

  M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
  M5.Lcd.setTextFont(4);
  M5.Lcd.println(String("Corona Tracker - ") + scope);

  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
  M5.Lcd.println("confirmed");
  M5.Lcd.setTextColor(TFT_YELLOW,TFT_BLACK);
  M5.Lcd.setTextFont(7);
  M5.Lcd.println(confirmed);
  
  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
  M5.Lcd.println("recovered");
  M5.Lcd.setTextColor(TFT_GREEN,TFT_BLACK);
  M5.Lcd.setTextFont(7);
  M5.Lcd.println(recovered);
  
  M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
  M5.Lcd.setTextFont(2);
  M5.Lcd.println("deaths");
  M5.Lcd.setTextColor(TFT_RED,TFT_BLACK);
  M5.Lcd.setTextFont(7);
  M5.Lcd.println(deaths);
}

void parse(String payload) {
  DeserializationError error = deserializeJson(doc, payload);
  
  confirmed = doc[0]["confirmed"];
  recovered = doc[0]["recovered"];
  deaths = doc[0]["deaths"];
}

void loop() {
  M5.update();
  
  if (M5.BtnA.read()) {
    scope = "World";
    path = "/totals";
    refresh_http = true;
  } else if (M5.BtnB.read()) {
    scope = "nl";
    path = "/country/code?code=nl";
    refresh_http = true;
  } else if (M5.BtnC.read()) {
    scope = String(countries[country_index]);
    path = String("/country/code?code=") + countries[country_index];
    country_index += 1;
    if (country_index >= NR_COUNTRIES) {
      country_index = 0;
    }
    refresh_http = true;
  }

  cnt_http += 1;
  
  if (cnt_http > 10*REFRESH_RATE) {
    cnt_http = 0;
    refresh_http = true;
  }

  if (refresh_http) {
    refresh_http = false;
  
    HTTPClient http;
    http.begin(String("https://") + host + path);
    http.addHeader("X-rapidapi-host", host);
    http.addHeader("X-rapidapi-key", apikey);
  
    int httpCode = http.GET();
   
    if (httpCode > 0) {
      String payload = http.getString();
      parse(payload);
    } else {
      Serial.println("Error on HTTP request");
    }
    
    update_display();

    http.end(); //Free the resources
  }
    
  delay(100); //milliseconds
}
