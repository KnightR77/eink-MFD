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
#include "config.h" //edit from config.template.h

const char* serverIP1 = "192.168.1.159";
const int serverPort1 = 5000;
// float VRAMU1 = 0;
// float VRAM1 = 0;
// int VLOAD1 = 0;
// int VTMEP1 = 0;
bool VGOOD1 = 0;

String jsonbuffer = "";

String weather = "Invalid";
float tmptr = -99;

int tmphour = 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -18000, 60000); // Eastern Time Zone UTC-5 (seconds)

GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16);
GxEPD_Class display(io, /*RST=*/ 16, /*BUSY=*/ 4);

const char* weekDays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void getCurrWeather(){
  StaticJsonDocument<200> weatherinfo;
  // DeserializationError error = deserializeJson(weatherinfo, jsonbuffer.c_str());
  String url = "http://api.openweathermap.org/data/2.5/weather?lat=" + latitude + "&lon=" + longitude + "&appid=" + apik;
  WiFiClient client;
  HTTPClient http;
  http.begin(client, url.c_str());
  int httpresp = http.GET();
  String pld;
  if (httpresp > 0){
    Serial.println("Got weather info!");
    pld = http.getString();
    jsonbuffer = pld.c_str();
  }
  else{
    Serial.println("Failed to get weather");
    jsonbuffer = "";
  }
  http.end();
  if (jsonbuffer == ""){
    weather = "Invalid";
    tmptr = -99;
  }
  else {
      deserializeJson(weatherinfo, jsonbuffer);
      const char * tmpw = weatherinfo["weather"]["main"];
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

void drawTimeDate(String timeStr, String dateStr, String weekStr, String temp, String vram1, String vtemp1, String vload1, String vmodel1, bool vgood1) {
    display.setRotation(1);
    display.setTextColor(GxEPD_BLACK);
    display.fillScreen(GxEPD_WHITE);

    display.setFont(&UbuntuNerdFontPropo_Light24pt);

    display.setCursor(230, 20);
    display.print(temp);
    // display.print("-15.0'C");
    display.setFont(&UbuntuNerdFontPropo_Light18pt);
    if (vgood1){
      display.setCursor(190, 40);
      display.print(vmodel1);
      display.setCursor(210, 56);
      display.print(vram1);
      display.setCursor(210, 72);
      display.print(vtemp1);
      display.setCursor(260, 72);
      display.print(vload1);

    } else{
      display.setCursor(185, 90);
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
}

void loop() {

    timeClient.update();
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime(&epochTime);
    epochTime += isDST(ptm) ? 3600 : 0; // Adjust for daylight saving time
    ptm = gmtime(&epochTime);

    char dateBuffer[20];
    snprintf(dateBuffer, sizeof(dateBuffer), "%s-%02d-%d", months[ptm->tm_mon], ptm->tm_mday, ptm->tm_year + 1900);
    String dateStr = String(dateBuffer);
    
    char timeBuffer[10];
    snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d", ptm->tm_hour, ptm->tm_min);
    String formattedTime = String(timeBuffer);
    
    String weekDay = weekDays[ptm->tm_wday];
  
    char tempBuffer[10];
    snprintf(tempBuffer, sizeof(tempBuffer), "%.1f'C", tmptr);
    String formattedTemp = String(tempBuffer);
    String vram1 = "";
    String vtemp1 = "";
    String vload1 = "";
    String vmodel1 = "";
    VGOOD1 = 0;
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = "http://" + String(serverIP1) + ":" + String(serverPort1) + "/gpu-stats";
        http.begin(url);

        int httpResponseCode = http.GET();
        if (httpResponseCode == 200) {
            String payload = http.getString();
            Serial.println("Received GPU Data: " + payload);

            // Parse JSON response
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (!error) {
                Serial.println(doc["gpu_model"].as<String>());
                Serial.print("GPU Temp: "); Serial.println(doc["temperature"].as<int>());
                Serial.print("Memory Used: "); Serial.println(doc["memory_used"].as<int>());
                Serial.print("Total Memory: "); Serial.println(doc["memory_total"].as<int>());
                Serial.print("GPU Usage: "); Serial.println(doc["gpu_usage"].as<int>());

                char vram1buf[20];
                char vtemp1buf[10];
                char vload1buf[10];
                snprintf(vram1buf, sizeof(vram1buf), "%.1f/%.1fGB", doc["memory_used"].as<float>(), doc["memory_total"].as<float>());
                snprintf(vtemp1buf, sizeof(vtemp1buf), "%d'C", doc["temperature"].as<int>());
                snprintf(vload1buf, sizeof(vload1buf), "%d%%", doc["gpu_usage"].as<int>());
                vram1 = String(vram1buf);
                vtemp1 = String(vtemp1buf);
                vload1 = String(vload1buf);
                vmodel1 = String(doc["gpu_model"].as<String>());
                VGOOD1 = 1;
            }
        } else {
            Serial.print("HTTP Request Failed, Code: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    } else {
        Serial.println("WiFi not connected!");
    }

    drawTimeDate(formattedTime, dateStr, weekDay, formattedTemp, vram1, vtemp1, vload1, vmodel1, VGOOD1);
    delay(60000);

    if (tmphour != ptm->tm_hour){
      getCurrWeather();
      tmphour = ptm->tm_hour;
    }
}
