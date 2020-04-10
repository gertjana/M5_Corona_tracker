#include <M5Stack.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include <Preferences.h>

#define NR_COUNTRIES 6
#define TREND_LENGTH 8
#define TREND_FACTOR 16

const int REFRESH_HTTP_RATE = 60; //seconds
const int REFRESH_TOGGLE_RATE = 10; //seconds
const String countries[NR_COUNTRIES]  = {"nl", "gb","us","it", "es", "cn"};

Preferences prefs;

TFT_eSprite spr = TFT_eSprite(&M5.Lcd);

int cnt_http = 0;
int cnt_toggle = 0;
bool refresh_http = true;
bool refresh_display = true;
bool toggle = false;
bool http_error = false;

long confirmed = 0;
long recovered = 0;
long critical = 0;
long deaths = 0;

String scope = "WORLD";
String path = "/totals";
int country_index = 0;
StaticJsonDocument<300> doc;
int trend_int[TREND_LENGTH];

void setup(void) {
  Serial.begin(115200);
  while(!Serial);    // wait until we have serial running
  delay(100);
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
  cnt_toggle += 1;

  
  if (cnt_http > 10*REFRESH_HTTP_RATE) {
    cnt_http = 0;
    refresh_http = true;
  }

  if (cnt_toggle > 10*REFRESH_TOGGLE_RATE) {
    cnt_toggle = 0;
    refresh_display = true;
    toggle = !toggle;
  }

  if (refresh_http) {
    refresh_http = false;

    get_http();
  
    store_trend(confirmed);
  
    refresh_display = true;

  }
  
  if (refresh_display) {
    refresh_display = false;
    update_display();
  }

  delay(100); //milliseconds
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
  
  if (toggle) {
    M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
    M5.Lcd.setTextFont(2);
    M5.Lcd.println("deaths");
    M5.Lcd.setTextColor(TFT_RED,TFT_BLACK);
    M5.Lcd.setTextFont(7);
    M5.Lcd.println(deaths);
  } else {
    M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
    M5.Lcd.setTextFont(2);
    M5.Lcd.println("critical");
    M5.Lcd.setTextColor(TFT_ORANGE,TFT_BLACK);
    M5.Lcd.setTextFont(7);
    M5.Lcd.println(critical);
  }

  for (int i=0;i<TREND_LENGTH;i++) {
    int val = trend_int[i]/TREND_FACTOR;
    double log_val = val==0 ? 0 : log10(val)*30;
    int x = i*10+230;
    int y = 220-log_val;

    M5.Lcd.drawRect(x, y, 8, log_val, TFT_YELLOW);
    M5.Lcd.fillRect(x, y, 8, log_val, TFT_YELLOW);
  }
  M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
  M5.Lcd.drawLine(230,90,230,220, TFT_WHITE);
  M5.Lcd.drawLine(230,220,310,220, TFT_WHITE);

  M5.Lcd.setTextFont(2);

  M5.Lcd.setCursor(230, 222);
  M5.Lcd.println("new cases");
  
  M5.Lcd.setCursor(205,90);
  M5.Lcd.println("10k");

  M5.Lcd.setCursor(205,120);
  M5.Lcd.println(" 1k");

  M5.Lcd.setCursor(205,150);
  M5.Lcd.println("100");

  M5.Lcd.setCursor(205,180);
  M5.Lcd.println(" 10");

  M5.Lcd.setCursor(205,210);
  M5.Lcd.println("  0");

  draw_sprite(http_error);
}

void parse(String payload) {
  DeserializationError error = deserializeJson(doc, payload);
  
  confirmed = doc[0]["confirmed"];
  recovered = doc[0]["recovered"];
  critical = doc[0]["critical"];
  deaths = doc[0]["deaths"];
}

void read_buttons() {
  if (M5.BtnA.read()) {
    scope = "World";
    path = "/totals";
    country_index = 0;
    refresh_http = true;
  } else if (M5.BtnB.read()) {
    if (!scope.equalsIgnoreCase("World")) {
      country_index -= 1;
      if (country_index < 0) {
        country_index = NR_COUNTRIES-1;
      }
    }
    scope = String(countries[country_index]);
    scope.toUpperCase();
    path = String("/country/code?code=") + countries[country_index];
    refresh_http = true;
  } else if (M5.BtnC.read()) {
    if (!scope.equalsIgnoreCase("World")) {
      country_index += 1;
      if (country_index >= NR_COUNTRIES) {
        country_index = 0;
      }
    }
    scope = String(countries[country_index]);
    scope.toUpperCase();
    path = String("/country/code?code=") + countries[country_index];
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
      http_error = false;
    } else {
      http_error = true;
    }
    http.end();
}

void store_trend(int value) {
  char schema[sizeof(scope)];
  scope.toCharArray(schema, sizeof(scope));
  
  char trend[sizeof(schema)+6];
  strcpy(trend, schema);
  strcat(trend, "_trend");

  prefs.begin(schema);
  int prev_value = prefs.getLong(schema, 0);
    
  if (prev_value == value) {
    prefs.end(); // schema
    prefs.begin(trend);

    char trend_buffer[TREND_LENGTH*2];
    prefs.getBytes(trend, trend_buffer, TREND_LENGTH*2);    
    trend_to_integer(trend_buffer, trend_int);

    prefs.end(); // trend
    return;
  } 
  
  prefs.putLong(schema, value);

  int diff = value - prev_value;

  prefs.end(); // schema
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

  prefs.end(); // trend
}

void trend_to_integer(char t[], int trend[]) {
  for (int i=0;i<TREND_LENGTH;i++) {
    trend[i] = (t[i*2] << 8) + t[i*2+1];  
  }
}

void draw_sprite(bool http_error) {
  spr.createSprite(32, 32);
  spr.fillSprite(TFT_BLACK);
  
  if (http_error) {
    spr.drawLine(0,31,31,31, TFT_RED);
    spr.drawLine(31,31,15,0, TFT_RED);
    spr.drawLine(15,0,0,31, TFT_RED);
    spr.fillRect(14,25,3,3, TFT_RED);
    spr.fillRect(14,10,3,12, TFT_RED);    
  } else {
    spr.drawLine(4,15,12,23, TFT_GREEN);
    spr.drawLine(12,23,28,7, TFT_GREEN);
    spr.drawLine(4,14,12,22, TFT_GREEN);
    spr.drawLine(12,22,28,6, TFT_GREEN);
  }

  spr.pushSprite(288,0, TFT_BLACK);
}