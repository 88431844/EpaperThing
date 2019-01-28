/**
 *  @filename   :   epd2in9-demo.ino
 *  @brief      :   2.9inch e-paper display demo
 *  @author     :   Yehui from Waveshare
 *
 *  Copyright (C) Waveshare     September 9 2017
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <SPI.h>
#include "epd2in9.h"
#include "epdpaint.h"
#include "imagedata.h"
#include <Time.h>
#include <Timezone.h>
#include "NTP.h"
//#define WIFI_SSID "Mobvoi-Guest"       // 使用时请修改为当前你的 wifi ssid
//#define WIFI_PASS "mobvoiguest"   // 使用时请修改为当前你的 wifi 密码

#define WIFI_SSID "HiWiFi_301"       // 使用时请修改为当前你的 wifi ssid
#define WIFI_PASS "13520391363"   // 使用时请修改为当前你的 wifi 密码

//#define WIFI_SSID "luck max"       // 使用时请修改为当前你的 wifi ssid
//#define WIFI_PASS "402050821"   // 使用时请修改为当前你的 wifi 密码
// 北京时间时区
#define STD_TIMEZONE_OFFSET +8    // Standard Time offset (-7 is mountain time)
// Define daylight savings time rules for the China
TimeChangeRule mySTD = {"", First,  Sun, Jan, 0, STD_TIMEZONE_OFFSET * 60};
Timezone myTZ(mySTD, mySTD);

WiFiClient client;

#define COLORED     0
#define UNCOLORED   1
// Previous seconds value
time_t previousSecond = 0;
/**
  * Due to RAM not enough in Arduino UNO, a frame buffer is not allowed.
  * In this case, a smaller image buffer is allocated and you have to 
  * update a partial display several times.
  * 1 byte = 8 pixels, therefore you have to set 8*N pixels at a time.
  */
unsigned char image[1024];
Paint paint(image, 0, 0);    // width should be the multiple of 8 
Epd epd;
unsigned long time_start_ms;
unsigned long time_now_s;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  if (epd.Init(lut_full_update) != 0) {
      Serial.print("e-Paper init failed");
      return;
  }
initNTP(WIFI_SSID, WIFI_PASS);
  /** 
   *  there are 2 memory areas embedded in the e-paper display
   *  and once the display is refreshed, the memory area will be auto-toggled,
   *  i.e. the next action of SetFrameMemory will set the other memory area
   *  therefore you have to clear the frame memory twice.
   */
  epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
  epd.DisplayFrame();
  epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
  epd.DisplayFrame();

//  paint.SetRotate(ROTATE_0);
//  paint.SetWidth(128);
//  paint.SetHeight(24);
//
//  /* For simplicity, the arguments are explicit numerical coordinates */
//  paint.Clear(COLORED);
//  paint.DrawStringAt(0, 4, "Hello world!", &Font16, UNCOLORED);
//  epd.SetFrameMemory(paint.GetImage(), 0, 10, paint.GetWidth(), paint.GetHeight());
//  
//  paint.Clear(UNCOLORED);
//  paint.DrawStringAt(0, 4, "e-Paper Demo", &Font16, COLORED);
//  epd.SetFrameMemory(paint.GetImage(), 0, 30, paint.GetWidth(), paint.GetHeight());
//
//  paint.SetWidth(64);
//  paint.SetHeight(64);
//  
//  paint.Clear(UNCOLORED);
//  paint.DrawRectangle(0, 0, 40, 50, COLORED);
//  paint.DrawLine(0, 0, 40, 50, COLORED);
//  paint.DrawLine(40, 0, 0, 50, COLORED);
//  epd.SetFrameMemory(paint.GetImage(), 16, 60, paint.GetWidth(), paint.GetHeight());
//
//  paint.Clear(UNCOLORED);
//  paint.DrawCircle(32, 32, 30, COLORED);
//  epd.SetFrameMemory(paint.GetImage(), 72, 60, paint.GetWidth(), paint.GetHeight());
//
//  paint.Clear(UNCOLORED);
//  paint.DrawFilledRectangle(0, 0, 40, 50, COLORED);
//  epd.SetFrameMemory(paint.GetImage(), 16, 130, paint.GetWidth(), paint.GetHeight());
//
//  paint.Clear(UNCOLORED);
//  paint.DrawFilledCircle(32, 32, 30, COLORED);
//  epd.SetFrameMemory(paint.GetImage(), 72, 130, paint.GetWidth(), paint.GetHeight());
//  epd.DisplayFrame();
//
//  delay(2000);

  if (epd.Init(lut_partial_update) != 0) {
      Serial.print("e-Paper init failed");
      return;
  }

  /** 
   *  there are 2 memory areas embedded in the e-paper display
   *  and once the display is refreshed, the memory area will be auto-toggled,
   *  i.e. the next action of SetFrameMemory will set the other memory area
   *  therefore you have to set the frame memory and refresh the display twice.
   */
//  epd.SetFrameMemory(IMAGE_DATA);
//  epd.DisplayFrame();
//  epd.SetFrameMemory(IMAGE_DATA);
//  epd.DisplayFrame();

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
  int weekdays=   weekday(localTime);
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
  String h = "";
  String m = "";
  String s = "";
  String w = dayStr(weekdays);
  if(hours < 10 ){
    h = "0"+String(hours);
  }else{
    h = String(hours);
  }
    if(minutes < 10 ){
    m = "0"+String(minutes);
  }
  else{
    m = String(minutes);
  }
    if(seconds < 10 ){
    s = "0"+String(seconds);
  }else{
    s = String(seconds);
  }
  String mytime = h + " : " + m ;
  String mydate = String(years) + "/" + months + "/" + days + "" + w;
  Serial.println("!!!: " + mytime);
  Serial.println("!!!: " + mydate);

//
  paint.SetWidth(32);
  paint.SetHeight(188);
  paint.SetRotate(ROTATE_90);
//

  paint.Clear(UNCOLORED);
char __mytime[sizeof(mytime)];
    mytime.toCharArray(__mytime, sizeof(__mytime));
  paint.DrawStringAt(0, 4,__mytime , &Font24, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 80, 72, paint.GetWidth(), paint.GetHeight());

 paint.Clear(UNCOLORED);
char __mydate[sizeof(mydate)];
    mydate.toCharArray(__mydate, sizeof(__mydate));
  paint.DrawStringAt(0, 4, __mydate, &Font24, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 0, 50, paint.GetWidth(), paint.GetHeight());


  

//  paint.DrawStringAt(0, 4, "e-Paper Demo", &Font16, COLORED);
//  epd.SetFrameMemory(paint.GetImage(), 0, 30, paint.GetWidth(), paint.GetHeight());
  
  epd.DisplayFrame();
//
  delay(500);
}
