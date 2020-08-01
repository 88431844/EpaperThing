
//needed for library
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include "NTP.h"
#include <DNSServer.h>
#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager
#include <SPI.h>
#include <Time.h>
#include <Timezone.h>
#include "epd2in9.h"
#include "epdpaint.h"
#include "DHTesp.h"
#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_IL3820_V2_296X128_1_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 14, /* data=*/ 13, /* cs=*/ 15, /* dc=*/ 4, /* reset=*/ 5);  // ePaper Display, lesser flickering and faster speed, enable 16 bit mode for this display!


#define COLORED     0
#define UNCOLORED   1

Epd epd;
unsigned char image[1024];
Paint paint(image, 0, 0);    // width should be the multiple of 8
int charSize = 50;

// 北京时间时区
#define STD_TIMEZONE_OFFSET +8
TimeChangeRule mySTD = {"", First,  Sun, Jan, 0, STD_TIMEZONE_OFFSET * 60};
Timezone myTZ(mySTD, mySTD);
time_t previousMinute = 0;

DHTesp dht;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  if (epd.Init(lut_full_update) != 0) {
    Serial.print("e-Paper init failed");
    return;
  }
  if (epd.Init(lut_partial_update) != 0) {
    Serial.print("e-Paper init failed");
    return;
  }

  /**
      there are 2 memory areas embedded in the e-paper display
      and once the display is refreshed, the memory area will be auto-toggled,
      i.e. the next action of SetFrameMemory will set the other memory area
      therefore you have to clear the frame memory twice.
  */
  epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
  epd.DisplayFrame();
  epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
  epd.DisplayFrame();

  wifiStatus("wifi connecting ..");

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("ePaperThing")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...");

  wifiStatus("wifi connected !!");

  initNTP();

  dht.setup(2, DHTesp::DHT11); // Connect DHT sensor to GPIO 2(D4)


  u8g2.begin();
  u8g2.enableUTF8Print();
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

  // forceClearEpaper();
}

void configModeCallback (WiFiManager *myWiFiManager) {
  wifiStatus("check wifi connect");
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

  /////process temp///////
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  String humidityStr = String(humidity);
  String temperatureStr = String(temperature);

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
void clearEpaper() {
  //清除屏幕残影
  paint.Clear(UNCOLORED);
  delay(1000);
  paint.Clear(COLORED);
}
void forceClearEpaper() {
  epd.ClearFrameMemory(0xFF);
  epd.DisplayFrame();
  delay(1000);
  epd.ClearFrameMemory(0x00);
  epd.DisplayFrame();
}
void wifiStatus(String status) {
  clearEpaper();
  paint.SetWidth(32);
  paint.SetHeight(250);
  paint.SetRotate(ROTATE_90);
  paint.Clear(UNCOLORED);
  char __status[100];
  status.toCharArray(__status, 100);
  paint.DrawStringAt(0, 4, __status, &Font20, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 40, 0, paint.GetWidth(), paint.GetHeight());
  epd.DisplayFrame();
  delay(1000);
}
String changeWeek(int weekSum){
  if(7 == weekSum){
    return "日";
  }
    if(6 == weekSum){
    return "六";
  }
    if(5 == weekSum){
    return "五";
  }
    if(4 == weekSum){
    return "四";
  }
    if(3 == weekSum){
    return "三";
  }
    if(2 == weekSum){
    return "二";
  }
    if(1 == weekSum){
    return "一";
  }
}
