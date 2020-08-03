
//needed for library
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include "NTP.h"
#include <DNSServer.h>
#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager
#include <SPI.h>
#include <Time.h>
#include <Timezone.h>
#include "DHTesp.h"
#include <Arduino.h>
#include <U8g2lib.h>
#include "OpenWeatherMapCurrent.h" //lib: ESP8266 Weather Station
#include "OpenWeatherMapForecast.h"

U8G2_IL3820_V2_296X128_1_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 14, /* data=*/ 13, /* cs=*/ 15, /* dc=*/ 4, /* reset=*/ 5);  // ePaper Display, lesser flickering and faster speed, enable 16 bit mode for this display!

// 北京时间时区
#define STD_TIMEZONE_OFFSET +8
TimeChangeRule mySTD = {"", First,  Sun, Jan, 0, STD_TIMEZONE_OFFSET * 60};
Timezone myTZ(mySTD, mySTD);
time_t previousMinute = 0;

DHTesp dht;

String OPEN_WEATHER_MAP_APP_ID = "5b254b901fb5d41b5eb4bf6c3dfe3894";
String OPEN_WEATHER_MAP_LOCATION_ID = "1816971";//保定；1816670 北京
// Pick a language code from this list:
// Arabic - ar, Bulgarian - bg, Catalan - ca, Czech - cz, German - de, Greek - el,
// English - en, Persian (Farsi) - fa, Finnish - fi, French - fr, Galician - gl,
// Croatian - hr, Hungarian - hu, Italian - it, Japanese - ja, Korean - kr,
// Latvian - la, Lithuanian - lt, Macedonian - mk, Dutch - nl, Polish - pl,
// Portuguese - pt, Romanian - ro, Russian - ru, Swedish - se, Slovak - sk,
// Slovenian - sl, Spanish - es, Turkish - tr, Ukrainian - ua, Vietnamese - vi,
// Chinese Simplified - zh_cn, Chinese Traditional - zh_tw.
String OPEN_WEATHER_MAP_LANGUAGE = "zh_cn";
const uint8_t MAX_FORECASTS = 4;
const boolean IS_METRIC = true;
OpenWeatherMapCurrent currentWeatherClient;
OpenWeatherMapCurrentData currentWeather;
OpenWeatherMapForecast forecastClient;
OpenWeatherMapForecastData forecasts[MAX_FORECASTS];
bool readyForWeatherUpdate = false;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  u8g2.begin();
  u8g2.enableUTF8Print();
  wifiStatus("WiFi连接中...");

  WiFiManager wifiManager;

  wifiManager.setAPCallback(configModeCallback);
  if (!wifiManager.autoConnect("ePaperThing")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...");

  wifiStatus("WiFi连接成功！！！");

  initNTP();

  dht.setup(2, DHTesp::DHT11); // Connect DHT sensor to GPIO 2(D4)

}

void loop() {
  //  Update the display only if time has changed
  if (timeStatus() != timeNotSet) {
    if (minute() != previousMinute) {
      previousMinute = minute();
      // Update the display
      updateDisplay();
    }
  }
  delay(500);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  wifiStatus("请检查WiFi连接后重启");
}

void updateDisplay(void) {

  TimeChangeRule *tcr;        // Pointer to the time change rule

  // Read the current UTC time from the NTP provider
  time_t utc = now();

  // Convert to local time taking DST into consideration
  time_t localTime = myTZ.toLocal(utc, &tcr);

  // Map time to pixel positions
  int weekdays =   weekday(localTime);
  int days    =   day(localTime);
  int months  =   month(localTime);
  int years   =   year(localTime);
  int seconds =   second(localTime);
  int minutes =   minute(localTime);
  int hours   =   hour(localTime) ;   //12 hour format use : hourFormat12(localTime)  isPM()/isAM()

  /////process time///////
  String h = "";
  String m = "";
  String s = "";
  if (hours < 10 ) {
    h = "0" + String(hours);
  } else {
    h = String(hours);
  }
  if (minutes < 10 ) {
    m = "0" + String(minutes);
  }
  else {
    m = String(minutes);
  }
  if (seconds < 10 ) {
    s = "0" + String(seconds);
  } else {
    s = String(seconds);
  }
  /////process date///////
  String mm = "";
  String dd = "";
  String w = dayStr(weekdays);

  if (months < 10 ) {
    mm = "0" + String(months);
  } else {
    mm = String(months);
  }
  
  if (days < 10 ) {
    dd = "0" + String(days);
  } else {
    dd = String(days);
  }
  String myTime = h + ":" + m ;
  String myDate = mm + "月" + dd + "日";
  String myWeek = w.substring(0, 3);
  Serial.println("-----------------");
  Serial.println("myTime: " + myTime);
  Serial.println("myDate: " + myDate);
  Serial.println("myWeek: " + myWeek);
  
  //////process weather////////
  currentWeatherClient.setMetric(IS_METRIC);
  currentWeatherClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  currentWeatherClient.updateCurrentById(&currentWeather, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);
  Serial.println("Updating forecasts...");
  forecastClient.setMetric(IS_METRIC);
  forecastClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  uint8_t allowedHours[] = {12};
  forecastClient.setAllowedHours(allowedHours, sizeof(allowedHours));
  forecastClient.updateForecastsById(forecasts, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID, MAX_FORECASTS);

  readyForWeatherUpdate = false;
  Serial.println("Done...");

  String myWeather = currentWeather.description;
  String myTemp = String(currentWeather.temp);
  Serial.println("myWeather：" + myWeather);
  Serial.println("myTemp：" + myTemp);
  
  /////process temp///////
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  String humidityStr = String(humidity);
  String temperatureStr = String(temperature);

  //////process display////////
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_logisoso78_tn);
    u8g2.setCursor(18, 83);
    u8g2.print(myTime);

    u8g2.setFont(u8g2_font_wqy16_t_gb2312b);
    u8g2.setCursor(0, 100);
    u8g2.print("温度：" + temperatureStr + " 湿度：" + humidityStr);

    u8g2.setFont(u8g2_font_wqy16_t_gb2312b);
    u8g2.setCursor(0, 120);
    u8g2.print(myDate + " 星期" + changeWeek(weekdays));
  } while ( u8g2.nextPage() );

}
void wifiStatus(String myStatus) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_wqy16_t_gb2312b);
    u8g2.setCursor(80, 70);
    u8g2.print(myStatus);
  } while ( u8g2.nextPage() );
}
String changeWeek(int weekSum){
  
  if(1 == weekSum){
    return "日";
  }
    if(7 == weekSum){
    return "六";
  }
    if(6 == weekSum){
    return "五";
  }
    if(5 == weekSum){
    return "四";
  }
    if(4 == weekSum){
    return "三";
  }
    if(3 == weekSum){
    return "二";
  }
    if(2 == weekSum){
    return "一";
  }
}
