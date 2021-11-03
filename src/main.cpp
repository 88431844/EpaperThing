#include <Arduino.h>

#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#include "NTP.h"
#include <DNSServer.h>
#include "WiFiManager.h" //https://github.com/tzapu/WiFiManager
#include <SPI.h>
#include <Time.h>
#include <Timezone.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <ESP8266WebServer.h> //  ESP8266WebServer库·
#include <ESP8266mDNS.h>
#include <stdlib.h>
#include <Button2.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
// !!!!!!!!!!! /Arduino/libraries/U8g2/src/clib/u8g2.h 去掉 #define U8G2_16BIT 注释，让2.9寸墨水屏显示区域变大成整屏
U8G2_IL3820_V2_296X128_1_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/14, /* data=*/13, /* cs=*/15, /* dc=*/0, /* reset=*/2); // ePaper Display, lesser flickering and faster speed, enable 16 bit mode for this display!

// 北京时间时区
#define STD_TIMEZONE_OFFSET +8
TimeChangeRule mySTD = {"", First, Sun, Jan, 0, STD_TIMEZONE_OFFSET * 60};
Timezone myTZ(mySTD, mySTD);
time_t previousMinute = 0;
int lastHour = 0;

ESP8266WebServer esp8266_server(80);

String todo = " 生命不息 折腾不止";

#define BUTTON_PIN 0
Button2 button = Button2(BUTTON_PIN);

//屏幕局部更新次数
int updateTimes = 0;
//5分钟全局刷新屏幕一次
int clearDisMin = 5;

//Todoist 配置
String projectId;
String authorization;

//电池使用时间 单位：分钟
int batLife = 0;

//函数声明
void handleCLEAR();
void clearDis();
void initNTP();
void webInit();
void myMDNSinit();
void returnWebRoot();
void wifiStatus(String myStatus, bool needClear);
void configModeCallback(WiFiManager *myWiFiManager);
void updateDisplay(String todo);
String changeWeek(int weekSum);
String getWebParam(String name);
void buttonHandler(Button2 &btn);
void getBatNow();

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  u8g2.begin();
  u8g2.enableUTF8Print();
  wifiStatus("WiFi连接中...", true);

  WiFiManager wifiManager;

  wifiManager.setAPCallback(configModeCallback);
  if (!wifiManager.autoConnect("EPaperThing"))
  {
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

  button.setClickHandler(buttonHandler);
  button.setLongClickHandler(buttonHandler);
  button.setDoubleClickHandler(buttonHandler);
  button.setTripleClickHandler(buttonHandler);
}

void loop()
{
  button.loop();
  MDNS.update();
  esp8266_server.handleClient(); // 处理http服务器访问

  //  Update the display only if time has changed
  if (timeStatus() != timeNotSet)
  {
    if (minute() != previousMinute)
    {
      previousMinute = minute();
      // Update the display
      Serial.println("-----------------");
      Serial.println("updateDisplay start...");
      updateDisplay(todo);
      Serial.println("updateDisplay end...");
    }
  }
}

void configModeCallback(WiFiManager *myWiFiManager)
{
  wifiStatus("请检查WiFi连接后重启", false);
}

void updateDisplay(String todo)
{

  TimeChangeRule *tcr; // Pointer to the time change rule

  // Read the current UTC time from the NTP provider
  time_t utc = now();

  // Convert to local time taking DST into consideration
  time_t localTime = myTZ.toLocal(utc, &tcr);

  // Map time to pixel pos78itions
  int weekdays = weekday(localTime);
  int days = day(localTime);
  int months = month(localTime);
  int years = year(localTime);
  int seconds = second(localTime);
  int minutes = minute(localTime);
  int hours = hour(localTime); //12 hour format use : hourFormat12(localTime)  isPM()/isAM()
  int thisHour = hours;

  //每隔X分钟全局刷新屏幕
  updateTimes++;
  if (updateTimes == clearDisMin)
  {
    clearDis();
    Serial.println("all display clear !!!");
    updateTimes = 0;
  }

  /////process time///////
  String h = "";
  String m = "";
  String s = "";
  if (hours < 10)
  {
    h = "0" + String(hours);
  }
  else
  {
    h = String(hours);
  }
  if (minutes < 10)
  {
    m = "0" + String(minutes);
  }
  else
  {
    m = String(minutes);
  }
  if (seconds < 10)
  {
    s = "0" + String(seconds);
  }
  else
  {
    s = String(seconds);
  }
  /////process date///////
  String mm = "";
  String dd = "";
  String w = dayStr(weekdays);

  if (months < 10)
  {
    mm = "0" + String(months);
  }
  else
  {
    mm = String(months);
  }

  if (days < 10)
  {
    dd = "0" + String(days);
  }
  else
  {
    dd = String(days);
  }
  String myTime = h + ":" + m;
  String myDate = mm + "月" + dd + "日";
  String myWeek = w.substring(0, 3);

  Serial.println("myTime: " + myTime);
  Serial.println("myDate: " + myDate);
  Serial.println("myWeek: " + myWeek);

  //////process display////////
  u8g2.sleepOff();
  Serial.println("u8g2.sleepOff().... ");
  u8g2.firstPage();
  do
  {
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
    // u8g2.print(String(todo));
    u8g2.print(String(batLife));

  } while (u8g2.nextPage());
  u8g2.sleepOn();
  Serial.println("u8g2.sleepOn().... ");
  batLife++;
}
void wifiStatus(String myStatus, bool needClear)
{
  u8g2.firstPage();
  do
  {
    u8g2.setFont(u8g2_font_wqy16_t_gb2312b);
    u8g2.setCursor(80, 70);
    u8g2.print(myStatus);
  } while (u8g2.nextPage());

  if (needClear)
  {
    clearDis();
  }
}
String changeWeek(int weekSum)
{

  if (1 == weekSum)
  {
    return "日";
  }
  if (7 == weekSum)
  {
    return "六";
  }
  if (6 == weekSum)
  {
    return "五";
  }
  if (5 == weekSum)
  {
    return "四";
  }
  if (4 == weekSum)
  {
    return "三";
  }
  if (3 == weekSum)
  {
    return "二";
  }
  if (2 == weekSum)
  {
    return "一";
  }
}

void handleRoot()
{ //处理网站根目录“/”的访问请求
  String content = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'/></head><body><center><form action='TODO'method='POST'><br/>提醒事项：<input type='text'name='todo'value=''/><br/><br/><br/><input type='submit'value='更新提醒事项'/></form><form action='CLEAR'method='POST'><br/><br/><input type='submit'value='清屏'/></form><form action='TODOIST'method='POST'><br/><br/>项目ID：<input type='text'name='projectId'value=''/><br/>authorization：<input type='text'name='authorization'value=''/><input type='submit'value='提交'/></form></center></body></html>";
  esp8266_server.send(200, "text/html", content); // NodeMCU将调用此函数。访问成功返回200状态码，返回信息类型text/html
}

// 设置处理404情况的函数'handleNotFound'
void handleNotFound()
{                                                           // 当浏览器请求的网络资源无法在服务器找到时，返回404状态码，和提升信息类型和内容
  esp8266_server.send(404, "text/plain", "404: Not found"); // NodeMCU将调用此函数。
}

void handleTODO()
{
  todo = getWebParam("todo");
  Serial.println("todo:" + todo);
  handleCLEAR();
}
void handleCLEAR()
{
  clearDis();
  returnWebRoot();
  updateDisplay(todo);
}
void handleTODOIST()
{
  projectId = getWebParam("projectId");
  authorization = getWebParam("authorization");
  Serial.println("handleTODOIST projectId: " + projectId);
  Serial.println("handleTODOIST authorization: " + authorization);
  handleCLEAR();
}
String getWebParam(String name)
{
  //read parameter from server, for customhmtl input
  String value;
  if (esp8266_server.hasArg(name))
  {
    value = esp8266_server.arg(name);
  }
  return value;
}

void webInit()
{

  //--------"启动网络服务功能"程序部分开始--------
  esp8266_server.begin();
  esp8266_server.on("/", HTTP_GET, handleRoot);            //访问网站的根目录 处理GET请求 执行handleRoot函数
  esp8266_server.on("/TODO", HTTP_POST, handleTODO);       //设置处理墨水屏提醒文字请求函数  处理POST请求
  esp8266_server.on("/CLEAR", HTTP_POST, handleCLEAR);     //设置处理墨水屏清理请求函数  处理POST请求
  esp8266_server.on("/TODOIST", HTTP_POST, handleTODOIST); //设置todoist的项目ID和authorization  处理POST请求

  esp8266_server.onNotFound(handleNotFound); //当请求服务器找不到时，执行handleNotFound函数
  //--------"启动网络服务功能"程序部分结束--------
  Serial.println("HTTP esp8266_server started"); //  告知用户ESP8266网络服务功能已经启动
}

void returnWebRoot()
{
  esp8266_server.sendHeader("Location", "/"); // 跳转回页面根目录
  esp8266_server.send(303);                   // 发送Http相应代码303 跳转
}
void clearDis()
{
  u8g2.clear();
  delay(500);
  u8g2.clearDisplay();
}
void myMDNSinit()
{
  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "EPaperThing.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin("EPaperThing"))
  {
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
}
void buttonHandler(Button2 &btn)
{
  switch (btn.getClickType())
  {
  case SINGLE_CLICK:
    Serial.print("single ");
    break;
  case DOUBLE_CLICK:
    Serial.print("double ");
    break;
  case TRIPLE_CLICK:
    Serial.print("triple ");
    break;
  case LONG_CLICK:
    Serial.print("long");
    break;
  }
  Serial.print("click");
  Serial.print(" (");
  Serial.print(btn.getNumberOfClicks());
  Serial.println(")");
}

// 获取城市天气
void getCityWeater()
{

  String URL = "http://restapi.amap.com/v3/weather/weatherInfo?city=130606&key=";
  //创建 HTTPClient 对象
  HTTPClient httpClient;

  httpClient.begin(URL);

  //启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.println("正在获取天气数据");
  Serial.println(URL);

  //如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK)
  {

    String str = httpClient.getString();
    Serial.println("ret:" + str);
    Serial.println("获取成功");
  }
  else
  {
    Serial.println("请求城市天气错误：");
    Serial.print(httpCode);
  }

  //关闭ESP8266与服务器连接
  httpClient.end();
}
