#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include "NTP.h"
#include <DNSServer.h>
#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager
#include <SPI.h>
#include <Time.h>
#include <Timezone.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <ESP8266WebServer.h>   //  ESP8266WebServer库·
#include <ESP8266mDNS.h>
#include "DEV_Config.h"
#include "EPD.h"
#include <stdlib.h>

// !!!!!!!!!!! /Arduino/libraries/U8g2/src/clib/u8g2.h 去掉 #define U8G2_16BIT 注释，让2.9寸墨水屏显示区域变大成整屏
U8G2_IL3820_V2_296X128_1_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 14, /* data=*/ 13, /* cs=*/ 15, /* dc=*/ 4, /* reset=*/ 2);  // ePaper Display, lesser flickering and faster speed, enable 16 bit mode for this display!

// 北京时间时区
#define STD_TIMEZONE_OFFSET +8
TimeChangeRule mySTD = {"", First,  Sun, Jan, 0, STD_TIMEZONE_OFFSET * 60};
Timezone myTZ(mySTD, mySTD);
time_t previousMinute = 0;
int lastHour = 0;

ESP8266WebServer esp8266_server(80);

String todo = " 生命不息 折腾不止";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  u8g2.begin();
  u8g2.enableUTF8Print();
  wifiStatus("WiFi连接中...", true);

  WiFiManager wifiManager;

  wifiManager.setAPCallback(configModeCallback);
  if (!wifiManager.autoConnect("EPaperThing")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...");

  wifiStatus("WiFi连接成功！！！", true);

  initNTP();

  webInit();
  myMDNSinit();
}

void loop() {

  MDNS.update();
  esp8266_server.handleClient();     // 处理http服务器访问

  //  Update the display only if time has changed
  if (timeStatus() != timeNotSet) {
    if (minute() != previousMinute) {
      previousMinute = minute();
      // Update the display
      updateDisplay(todo);
    }
  }
  delay(500);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  wifiStatus("请检查WiFi连接后重启", false);
}

void updateDisplay(String todo) {

  TimeChangeRule *tcr;        // Pointer to the time change rule

  // Read the current UTC time from the NTP provider
  time_t utc = now();

  // Convert to local time taking DST into consideration
  time_t localTime = myTZ.toLocal(utc, &tcr);

  // Map time to pixel pos78itions
  int weekdays =   weekday(localTime);
  int days    =   day(localTime);
  int months  =   month(localTime);
  int years   =   year(localTime);
  int seconds =   second(localTime);
  int minutes =   minute(localTime);
  int hours   =   hour(localTime) ;   //12 hour format use : hourFormat12(localTime)  isPM()/isAM()
  int thisHour = hours;

  //每隔1小时，刷新一次屏幕
  if (lastHour != thisHour) {
    clearDis();
    lastHour = thisHour;
  }


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

  //////process display////////
  u8g2.firstPage();
  do {
    //u8g2_font_logisoso92_tn
    u8g2.setFont(u8g2_font_logisoso92_tn);
    char __myTime[sizeof(myTime)];
    myTime.toCharArray(__myTime, sizeof(__myTime));
    u8g2.drawStr(5, 95, __myTime);

    u8g2.setFont(u8g2_font_wqy16_t_gb2312b);
    u8g2.setCursor(2, 117);
    u8g2.print(myDate);

    u8g2.setCursor(67, 117);
    u8g2.print("周" + changeWeek(weekdays));


    u8g2.setFont(u8g2_font_wqy16_t_gb2312b);
    u8g2.setCursor(142, 117);
    u8g2.print(todo);

  } while ( u8g2.nextPage() );

}
void wifiStatus(String myStatus, bool needClear) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_wqy16_t_gb2312b);
    u8g2.setCursor(80, 70);
    u8g2.print(myStatus);
  } while ( u8g2.nextPage() );

  if (needClear) {
    clearDis();
  }
}
String changeWeek(int weekSum) {

  if (1 == weekSum) {
    return "日";
  }
  if (7 == weekSum) {
    return "六";
  }
  if (6 == weekSum) {
    return "五";
  }
  if (5 == weekSum) {
    return "四";
  }
  if (4 == weekSum) {
    return "三";
  }
  if (3 == weekSum) {
    return "二";
  }
  if (2 == weekSum) {
    return "一";
  }
}

void handleRoot() {   //处理网站根目录“/”的访问请求
  String content = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'></head><body><center><form action='TODO 'method='POST'><br>提醒事项：<input type='text'name='todo'value=''><br><br><br><input type='submit'value='更新提醒事项'></form><form action='CLEAR 'method='POST'><br><br><input type='submit'value='清屏'></form></center></body></html>";
  esp8266_server.send(200, "text/html", content);   // NodeMCU将调用此函数。访问成功返回200状态码，返回信息类型text/html
}

// 设置处理404情况的函数'handleNotFound'
void handleNotFound() { // 当浏览器请求的网络资源无法在服务器找到时，返回404状态码，和提升信息类型和内容
  esp8266_server.send(404, "text/plain", "404: Not found");   // NodeMCU将调用此函数。
}

void handleTODO() {
  todo = getParam("todo");
  Serial.println("todo:" + todo);
  handleCLEAR();
}
void handleCLEAR() {
  clearDis();
  returnRoot();
  updateDisplay(todo);
}
String getParam(String name) {
  //read parameter from server, for customhmtl input
  String value;
  if (esp8266_server.hasArg(name)) {
    value = esp8266_server.arg(name);
  }
  return value;
}

void webInit() {

  //--------"启动网络服务功能"程序部分开始--------
  esp8266_server.begin();
  esp8266_server.on("/", HTTP_GET, handleRoot); //访问网站的根目录 处理GET请求 执行handleRoot函数
  esp8266_server.on("/TODO", HTTP_POST, handleTODO); //设置处理墨水屏提醒文字请求函数  处理POST请求
  esp8266_server.on("/CLEAR", HTTP_POST, handleCLEAR); //设置处理墨水屏清理请求函数  处理POST请求
  esp8266_server.onNotFound(handleNotFound);  //当请求服务器找不到时，执行handleNotFound函数
  //--------"启动网络服务功能"程序部分结束--------
  Serial.println("HTTP esp8266_server started");//  告知用户ESP8266网络服务功能已经启动
}

void returnRoot() {
  esp8266_server.sendHeader("Location", "/");         // 跳转回页面根目录
  esp8266_server.send(303);                           // 发送Http相应代码303 跳转
}
void clearDis() {
  DEV_Module_Init();
  EPD_2IN9_Init(EPD_2IN9_FULL);
  EPD_2IN9_Clear();
  DEV_Delay_ms(500);

  u8g2.begin();
  u8g2.enableUTF8Print();
}
void myMDNSinit() {
  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "EPaperThing.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin("EPaperThing")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
}
