#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <GxEPD.h>
#include <GxDEPG0290BS/GxDEPG0290BS.h>
#include GxEPD_BitmapExamples
// #include "VWThesis_MIB_Regular36pt.h"
// #include "VWThesis_MIB_Regular48pt.h"
// #include "UbuntuSansMonoNerdFont_SemiBold48pt.h"
// #include "UbuntuSansMonoNerdFont_Regular36pt.h"
// #include "UbuntuSansMonoNerdFont_Regular48pt.h"
#include "UbuntuNerdFontPropo_Regular24pt.h"
#include "UbuntuNerdFontPropo_Light24pt.h"
#include "UbuntuNerdFontPropo_Light18pt.h"
#include "UbuntuNerdFontPropo_Regular32pt.h"
#include "UbuntuNerdFontPropo_Regular64pt.h"
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <ctime>
#include <HTTPClient.h>
#include "config.h"  //edit from config.template.h
#include <Timezone.h>

#define TZ_OFFSET 0
#define NTP_INTERVAL 60000

TimeChangeRule usEDT = { "EDT", Second, Sun, Mar, 2, -240 };  // Eastern Daylight Time = UTC - 4 hours
TimeChangeRule usEST = { "EST", First, Sun, Nov, 2, -300 };   // Eastern Standard Time = UTC - 5 hours
Timezone usET(usEDT, usEST);

const char* serverIP1 = "192.168.1.2";
const int serverPort1 = 5000;
float vram1 = 0;
float vramu1 = 0;
int vtemp1 = 0;
int vload1 = 0;
String vmodel1 = "";
bool VGOOD1 = 0;

const char* serverIP2 = "192.168.1.3";
const int serverPort2 = 5000;
float vram2 = 0;
float vramu2 = 0;
int vtemp2 = 0;
int vload2 = 0;
String vmodel2 = "";
bool VGOOD2 = 0;

String jsonbuffer = "";

String weather = "Invalid";
float tmptr = -99;

int tmphour = 0;
int tmpmin = 0;

unsigned long lastupdate = 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);  // Eastern Time Zone UTC-5 (seconds)

GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/17, /*RST=*/16);
GxEPD_Class display(io, /*RST=*/16, /*BUSY=*/4);

const char* weekDays[] = { "", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
const char* months[] = { "", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

void getCurrWeather() {
  StaticJsonDocument<200> weatherinfo;
  // DeserializationError error = deserializeJson(weatherinfo, jsonbuffer.c_str());
  String url = "http://api.openweathermap.org/data/2.5/weather?lat=" + latitude + "&lon=" + longitude + "&appid=" + apik;
  WiFiClient client;
  HTTPClient http;
  http.begin(client, url.c_str());
  int httpresp = http.GET();
  String pld;
  if (httpresp > 0) {
    Serial.println("Got weather info!");
    pld = http.getString();
    jsonbuffer = pld.c_str();
  } else {
    Serial.println("Failed to get weather");
    jsonbuffer = "";
  }
  http.end();
  if (jsonbuffer == "") {
    weather = "Invalid";
    tmptr = -99;
  } else {
    deserializeJson(weatherinfo, jsonbuffer);
    const char* tmpw = weatherinfo["weather"]["main"];
    weather = tmpw;
    tmptr = weatherinfo["main"]["temp"];
    tmptr -= 273.15;
  }
  Serial.println(weather);
  return;
};

bool isDST(struct tm* timeinfo) {
  int month = timeinfo->tm_mon + 1;
  int day = timeinfo->tm_mday;
  int wday = timeinfo->tm_wday;
  if (month < 3 || month > 11) return false;
  if (month > 3 && month < 11) return true;
  int previousSunday = day - wday;
  if (month == 3) return previousSunday >= 8;
  return previousSunday < 1;
}

void updateMonitor() {
  VGOOD1 = 0;
  VGOOD2 = 0;
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url1 = "http://" + String(serverIP1) + ":" + String(serverPort1) + "/gpu-stats";
    Serial.println(url1);
    http.begin(url1);

    int httpResponseCode1 = http.GET();
    if (httpResponseCode1 == 200) {
      String payload = http.getString();
      Serial.println("Received GPU1 Data: " + payload);

      // Parse JSON response
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (!error) {
        Serial.println(doc["gpu_model"].as<String>());
        Serial.print("GPU Temp: ");
        Serial.println(doc["temperature"].as<int>());
        Serial.print("Memory Used: ");
        Serial.println(doc["memory_used"].as<int>());
        Serial.print("Total Memory: ");
        Serial.println(doc["memory_total"].as<int>());
        Serial.print("GPU Usage: ");
        Serial.println(doc["gpu_usage"].as<int>());
        vramu1 = doc["memory_used"].as<float>();
        vram1 = doc["memory_total"].as<float>();
        vtemp1 = doc["temperature"].as<int>();
        vload1 = doc["gpu_usage"].as<int>();
        vmodel1 = String(doc["gpu_model"].as<String>());
        VGOOD1 = 1;
      }
    } else {
      Serial.print("HTTP Request1 Failed, Code: ");
      Serial.println(httpResponseCode1);
    }
    http.end();
    
    String url2 = "http://" + String(serverIP2) + ":" + String(serverPort2) + "/gpu-stats";
    Serial.println(url2);
    http.begin(url2);

    int httpResponseCode2 = http.GET();
    if (httpResponseCode2 == 200) {
      String payload2 = http.getString();
      Serial.println("Received GPU2 Data: " + payload2);

      // Parse JSON response
      StaticJsonDocument<200> doc2;
      DeserializationError error2 = deserializeJson(doc2, payload2);
      if (!error2) {
        Serial.println(doc2["gpu_model"].as<String>());
        Serial.print("GPU Temp: ");
        Serial.println(doc2["temperature"].as<int>());
        Serial.print("Memory Used: ");
        Serial.println(doc2["memory_used"].as<int>());
        Serial.print("Total Memory: ");
        Serial.println(doc2["memory_total"].as<int>());
        Serial.print("GPU Usage: ");
        Serial.println(doc2["gpu_usage"].as<int>());
        vramu2 = doc2["memory_used"].as<float>();
        vram2 = doc2["memory_total"].as<float>();
        vtemp2 = doc2["temperature"].as<int>();
        vload2 = doc2["gpu_usage"].as<int>();
        vmodel2 = String(doc2["gpu_model"].as<String>());
        VGOOD2 = 1;
      }
    } else {
      Serial.print("HTTP Request2 Failed, Code: ");
      Serial.println(httpResponseCode2);
    }
    http.end();

  } else {
    Serial.println("WiFi not connected!");
  }
}

void draw() {
  char dateBuffer[20];
  snprintf(dateBuffer, sizeof(dateBuffer), "%s-%02d-%d", months[month()], day(), year());
  String dateStr = String(dateBuffer);

  char timeBuffer[10];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d", hour(), minute());
  String timeStr = String(timeBuffer);

  String weekStr = weekDays[weekday()];

  char tempBuffer[10];
  snprintf(tempBuffer, sizeof(tempBuffer), "%.1f'C", tmptr);
  String temp = String(tempBuffer);

  char vram1buf[20];
  char vtemp1buf[10];
  char vload1buf[10];
  snprintf(vram1buf, sizeof(vram1buf), "%.1f/%.1fGB", vramu1, vram1);
  snprintf(vtemp1buf, sizeof(vtemp1buf), "%d'C", vtemp1);
  snprintf(vload1buf, sizeof(vload1buf), "%d%%", vload1);
  String vvram1 = String(vram1buf);
  String vvtemp1 = String(vtemp1buf);
  String vvload1 = String(vload1buf);

  char vram2buf[20];
  char vtemp2buf[10];
  char vload2buf[10];
  snprintf(vram2buf, sizeof(vram2buf), "%.1f/%.1fGB", vramu2, vram2);
  snprintf(vtemp2buf, sizeof(vtemp2buf), "%d'C", vtemp2);
  snprintf(vload2buf, sizeof(vload2buf), "%d%%", vload2);
  String vvram2 = String(vram2buf);
  String vvtemp2 = String(vtemp2buf);
  String vvload2 = String(vload2buf);
  // String vvmodel1 = String(doc["gpu_model"].as<String>());

  display.setRotation(3);
  display.setTextColor(GxEPD_BLACK);
  display.fillScreen(GxEPD_WHITE);

  display.setFont(&UbuntuNerdFontPropo_Light24pt);

  display.setCursor(230, 20);
  display.print(temp);
  // display.print("-15.0'C");
  display.setFont(&UbuntuNerdFontPropo_Light18pt);
  if (VGOOD1) {
    display.setCursor(190, 45);
    display.print(vmodel1);
    display.setCursor(210, 61);
    display.print(vvram1);
    display.setCursor(210, 77);
    display.print(vvtemp1);
    display.setCursor(255, 77);
    display.print(vvload1);

  } else {
    display.setCursor(190, 40);
    display.print("Error");
  }
  if (VGOOD2) {
    display.setCursor(190, 95);
    display.print(vmodel2);
    display.setCursor(210, 111);
    display.print(vvram2);
    display.setCursor(210, 127);
    display.print(vvtemp2);
    display.setCursor(255, 127);
    display.print(vvload2);

  } else {
    display.setCursor(190, 100);
    display.print("Error");
  }

  display.setFont(&UbuntuNerdFontPropo_Regular24pt);

  display.setCursor(5, 25);
  display.print(weekStr);
  // display.print("Wednesday");

  display.setFont(&UbuntuNerdFontPropo_Regular32pt);

  // display.setCursor(5, 128);
  // display.print("Date:");
  display.setCursor(5, 60);
  display.print(dateStr);
  // display.print("Sep-30-2025");

  display.setFont(&UbuntuNerdFontPropo_Regular64pt);

  // display.setCursor(5, 55);
  // display.print("Time:");
  display.setCursor(5, 120);
  display.print(timeStr);
  // display.print("23:59");

  display.drawLine(180, 10, 180, 120, GxEPD_BLACK);
  display.drawLine(181, 10, 181, 120, GxEPD_BLACK);
  // display.drawLine(187, 10, 187, 120, GxEPD_BLACK);

  display.update();
  display.powerDown();
}
void drawPartial() {
  char vram1buf[20];
  char vtemp1buf[10];
  char vload1buf[10];
  snprintf(vram1buf, sizeof(vram1buf), "%.1f/%.1fGB", vramu1, vram1);
  snprintf(vtemp1buf, sizeof(vtemp1buf), "%d'C", vtemp1);
  snprintf(vload1buf, sizeof(vload1buf), "%d%%", vload1);
  String vvram1 = String(vram1buf);
  String vvtemp1 = String(vtemp1buf);
  String vvload1 = String(vload1buf);
  char vram2buf[20];
  char vtemp2buf[10];
  char vload2buf[10];
  snprintf(vram2buf, sizeof(vram2buf), "%.1f/%.1fGB", vramu2, vram2);
  snprintf(vtemp2buf, sizeof(vtemp2buf), "%d'C", vtemp2);
  snprintf(vload2buf, sizeof(vload2buf), "%d%%", vload2);
  String vvram2 = String(vram2buf);
  String vvtemp2 = String(vtemp2buf);
  String vvload2 = String(vload2buf);
  display.setFont(&UbuntuNerdFontPropo_Light18pt);
  display.fillRect(190, 20, 100, 108, GxEPD_WHITE);
  if (VGOOD1) {
    display.setCursor(190, 45);
    display.print(vmodel1);
    display.setCursor(210, 61);
    display.print(vvram1);
    display.setCursor(210, 77);
    display.print(vvtemp1);
    display.setCursor(255, 77);
    display.print(vvload1);
  } else {
    display.setCursor(190, 50);
    display.print("Error");
  }
    if (VGOOD2) {
    display.setCursor(190, 95);
    display.print(vmodel2);
    display.setCursor(210, 111);
    display.print(vvram2);
    display.setCursor(210, 127);
    display.print(vvtemp2);
    display.setCursor(255, 127);
    display.print(vvload2);

  } else {
    display.setCursor(190, 100);
    display.print("Error");
  }
  display.updateWindow(190, 20, 100, 108, true);
  Serial.println("Partial");
}
void drawPartialTime(){
  char timeBuffer[10];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d", hour(), minute());
  String timeStr = String(timeBuffer);
  display.setFont(&UbuntuNerdFontPropo_Regular64pt);
  display.fillRect(5, 65, 150, 60, GxEPD_WHITE);
  display.setCursor(5, 120);
  display.print(timeStr);
  display.updateWindow(5, 65, 150, 60, true);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Connecting to WiFi...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  timeClient.begin();

  display.init(115200);
  display.fillScreen(GxEPD_WHITE);
  display.update();
  getCurrWeather();
  timeClient.update();
  setTime(usET.toLocal(timeClient.getEpochTime()));
}

void loop() {
  updateMonitor();

  if (tmphour != hour()) {
    timeClient.update();
    setTime(usET.toLocal(timeClient.getEpochTime()));
    getCurrWeather();
    draw();
    tmphour = hour();
  }

  if (tmpmin != minute()) {

    drawPartialTime();
    tmpmin = minute();
  }
  drawPartial();

  delay(1000);
}
