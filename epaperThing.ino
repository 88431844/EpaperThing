#include <SPI.h>
#include "epd2in9.h"
#include "epdpaint.h"
#include <Time.h>
#include <Timezone.h>
#include "NTP.h"
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include "heweather.h"

// 北京时间时区
#define STD_TIMEZONE_OFFSET +8    // Standard Time offset (-7 is mountain time)
// Define daylight savings time rules for the China
TimeChangeRule mySTD = {"", First,  Sun, Jan, 0, STD_TIMEZONE_OFFSET * 60};
Timezone myTZ(mySTD, mySTD);

WiFiClient client;
heweatherclient heweather();

#define COLORED     0
#define UNCOLORED   1
// Previous seconds value
time_t previousSecond = 0;
/**
    Due to RAM not enough in Arduino UNO, a frame buffer is not allowed.
    In this case, a smaller image buffer is allocated and you have to
    update a partial display several times.
    1 byte = 8 pixels, therefore you have to set 8*N pixels at a time.
*/
unsigned char image[1024];
Paint paint(image, 0, 0);    // width should be the multiple of 8
Epd epd;
unsigned long time_start_ms;
unsigned long time_now_s;

int charSize = 50;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  if (epd.Init(lut_full_update) != 0) {
    Serial.print("e-Paper init failed");
    return;
  }

  WiFiManager wifiManager;
  wifiManager.autoConnect("ePaperThing");
  Serial.println("connected...yeey :)");

  initNTP();
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

  if (epd.Init(lut_partial_update) != 0) {
    Serial.print("e-Paper init failed");
    return;
  }
  time_start_ms = millis();
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
  Serial.println("");
  Serial.print("Current local time:");
  Serial.print(days);
  Serial.print("/");
  Serial.print(months);
  Serial.print("/");
  Serial.print(years);
  Serial.print(" - ");
  Serial.print(hours);
  Serial.print(":");
  Serial.print(minutes);
  Serial.print(":");
  Serial.print(seconds);
  Serial.print(" - ");
  Serial.print(dayStr(weekdays));

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
  String mytime = h + ":" + m ;
  String mydate = String(years) + "-" + mm + "-" + dd;
  Serial.println("mytime: " + mytime);
  Serial.println("mydate: " + mydate);

  paint.SetWidth(32);
  paint.SetHeight(264);
  paint.SetRotate(ROTATE_90);

  paint.Clear(UNCOLORED);
  char __mytime[charSize];
  mytime.toCharArray(__mytime, charSize);
  paint.DrawStringAt(0, 4, __mytime , &Font24, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 100, (296 - 90) / 2, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  char __mydate[charSize];
  mydate.toCharArray(__mydate, charSize);
  paint.DrawStringAt(0, 4, __mydate, &Font24, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 70, (296 - 160) / 2, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  char __myweek[charSize];
  w.toCharArray(__myweek, charSize);
  paint.DrawStringAt(0, 4, __myweek, &Font24, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 35, (296 - 125) / 2, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  String minTemp = "-11";
  String maxTemp = "-18";
  String temp =  "temp:" + minTemp + "/" + maxTemp;
   char __temp[50];
  temp.toCharArray(__temp, 50);
  paint.DrawStringAt(0, 4, __temp, &Font24, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 0, (296 - 125) / 2, paint.GetWidth(), paint.GetHeight());

  epd.DisplayFrame();
}
