#include <M5Stack.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "ArduinoJson.h"
#include "secrets.h"
#include <Preferences.h>

#define NR_COUNTRIES 5
#define TREND_LENGTH 8
const int REFRESH_RATE = 10; //seconds
const String countries[NR_COUNTRIES]  = {"gb","us","it", "es", "cn"};

Preferences prefs;

int cnt_http = 0;
bool refresh_http = true;
int confirmed = 0;
int recovered = 0;
int deaths = 0;
String scope = "World";
String path = "/totals";
int country_index = 0;
StaticJsonDocument<300> doc;
int trend_int[TREND_LENGTH];
int TREND_FACTOR = 8;

void setup(void) {
  Serial.begin(115200);
  while(!Serial);    // wait until we have serial running
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


void loop() {
  M5.update();

  read_buttons();
  
  cnt_http += 1;
  
  if (cnt_http > 10*REFRESH_RATE) {
    cnt_http = 0;
    refresh_http = true;
  }

  if (refresh_http) {
    refresh_http = false;

    get_http();
  
    store_trend(confirmed);
  
    update_display();

  }
    
  delay(100); //milliseconds
}

void update_display() {
  M5.Lcd.fillScreen(TFT_BLACK); 
  M5.Lcd.setCursor(0, 0, 2);

  M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
  M5.Lcd.setTextFont(4);
  scope.toUpperCase();
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

  if (scope.equalsIgnoreCase("World")) {
    for (int i=0;i<TREND_LENGTH;i++) {
      int val = trend_int[i]/TREND_FACTOR;
      if (val > 150) {val = 150;}
      int x = i*10+230;
      int w = 8;
      int y = 220-val;
      int h = val;
      M5.Lcd.drawRect(x, y, w, h, TFT_ORANGE);
      M5.Lcd.fillRect(x, y, w, h, TFT_ORANGE);
    }
    M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
    M5.Lcd.drawLine(230,70,230,220, TFT_WHITE);
    M5.Lcd.drawLine(230,220,310,220, TFT_WHITE);

    M5.Lcd.setTextFont(2);

    M5.Lcd.setCursor(230, 222);
    M5.Lcd.println("new cases");
    
    M5.Lcd.setCursor(205,85);
    M5.Lcd.println(" 1k");

    M5.Lcd.setCursor(205,210);
    M5.Lcd.println(" 0");
  }
}

void parse(String payload) {
  DeserializationError error = deserializeJson(doc, payload);
  
  confirmed = doc[0]["confirmed"];
  recovered = doc[0]["recovered"];
  deaths = doc[0]["deaths"];
}

void read_buttons() {
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
}

void get_http() {
 HTTPClient http;
    http.begin(String("https://") + host + path);
    http.addHeader("Accept", "application/json");
    http.addHeader("X-rapidapi-host", host);
    http.addHeader("X-rapidapi-key", apikey);
  
    int httpCode = http.GET();
   
    if (httpCode > 0) {
      String payload = http.getString();
      parse(payload);
    } else {
      Serial.println("Error on HTTP request");
    }
    http.end();
}

void store_trend(int value) {
  Serial.println(scope);

  char schema[sizeof(scope)];
  scope.toCharArray(schema, sizeof(scope));
  
  char trend[sizeof(schema)+6];
  strcpy(trend, schema);
  strcat(trend, "_trend");

  prefs.begin(schema);
  int prev_value = prefs.getInt(schema, 0);
    
  if (prev_value == value) {
    Serial.println("No change");

    prefs.end();
    prefs.begin(trend);

    char trend_buffer[TREND_LENGTH*2];
    prefs.getBytes(trend, trend_buffer, TREND_LENGTH*2);
    trend_to_integer(trend_buffer, trend_int);
    
    prefs.end();
    return;
  } 
  
  prefs.putInt(schema, value);

  int diff = value - prev_value;

  prefs.end();
  prefs.begin(trend);

  char trend_buffer[TREND_LENGTH*2];
  prefs.getBytes(trend, trend_buffer, TREND_LENGTH*2);
  
  for (int i=0;i<(TREND_LENGTH*2-2); i++) {
    trend_buffer[i] = trend_buffer[i+2];
  }
  trend_buffer[TREND_LENGTH*2-2] = (diff >> 8) & 0xFF;
  trend_buffer[TREND_LENGTH*2-1] = diff & 0xFF;
  
  trend_to_integer(trend_buffer, trend_int);

  prefs.putBytes(trend, &trend_buffer, TREND_LENGTH*2);

  prefs.end();
}

void trend_to_integer(char t[], int trend[]) {
  //print_array("Trend", t, TREND_LENGTH*2);  
  for (int i=0;i<TREND_LENGTH;i++) {
    trend[i] = (t[i*2] << 8) + t[i*2+1];  
  }
  //print_array_int("Trend", trend, TREND_LENGTH*2);  
}

// void print_array(char name[], char data[], int len) {
//   Serial.print(name);
//   Serial.print(": ");
//   for (int i=0; i<len; i++) {
//     if (data[i]<0x10) {Serial.print("0");}
//     Serial.print(data[i],HEX);
//     Serial.print(" ");
//   }
//   Serial.println("");
// }

// void print_array_int(char name[], int a[], int len) {
//   int l = len/2;
//   Serial.print(name);
//   Serial.print(": ");
//   for (int i=0; i<l; i++) {
//     Serial.print(a[i]);
//     if (i != l-1) { Serial.print(", "); }
//   }
//   Serial.println("");  
// }
