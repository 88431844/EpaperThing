#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager
#include <SPI.h>
#include <Time.h>
#include <Timezone.h>
#include "epd2in9.h"
#include "epdpaint.h"
#include "NTP.h"

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
time_t previousSecond = 0;

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
  Serial.println("connected...yeey :)");


  initNTP();

}

void loop() {
  //  Update the display only if time has changed
  if (timeStatus() != timeNotSet) {
    if (second() != previousSecond) {
      previousSecond = second();
      // Update the display
      updateDisplay();
    }
  }
  delay(500);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  paint.SetWidth(32);
  paint.SetHeight(250);
  paint.SetRotate(ROTATE_90);
  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 4, "check wifi connect !!", &Font20, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 40, 0, paint.GetWidth(), paint.GetHeight());
  epd.DisplayFrame();
  delay(500);
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
  String myDate = String(years) + "-" + mm + "-" + dd;
  String myWeek = w.substring(0, 3);
  Serial.println("-----------------");
  Serial.println("myTime: " + myTime);
  Serial.println("myDate: " + myDate);
  Serial.println("myWeek: " + myWeek);

  paint.SetWidth(32);
  paint.SetHeight(250);
  paint.SetRotate(ROTATE_90);

  paint.Clear(UNCOLORED);
  char __myTime[charSize];
  myTime.toCharArray(__myTime, charSize);
  paint.DrawStringAt(0, 4, __myTime , &Font24, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 100, 0, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  char __myWeek[charSize];
  myWeek.toCharArray(__myWeek, charSize);
  paint.DrawStringAt(0, 4, __myWeek, &Font24, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 100, 110, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  char __myDate[charSize];
  myDate.toCharArray(__myDate, charSize);
  paint.DrawStringAt(0, 4, __myDate, &Font24, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 70, 0, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  String first =  "bad artists copy";
  char __first[100];
  first.toCharArray(__first, 100);
  paint.DrawStringAt(0, 4, __first, &Font20, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 35, 0, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  String second =  "good artists steal";
  char __second[100];
  second.toCharArray(__second, 100);
  paint.DrawStringAt(0, 4, __second, &Font20, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 0, 0, paint.GetWidth(), paint.GetHeight());

  epd.DisplayFrame();
}
void clearEpaper(){
  //清除屏幕残影
  epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
  epd.DisplayFrame();
  delay(1000);
  epd.ClearFrameMemory(0x00);   // bit set = white, bit reset = black
  epd.DisplayFrame();
}
