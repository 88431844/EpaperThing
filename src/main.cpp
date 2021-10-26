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
#include "epd2in9.h"
#include "EPD_2in9.h"
#include <stdlib.h>
#include <Button2.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

Epd epd;
// !!!!!!!!!!! /Arduino/libraries/U8g2/src/clib/u8g2.h 去掉 #define U8G2_16BIT 注释，让2.9寸墨水屏显示区域变大成整屏
U8G2_IL3820_V2_296X128_1_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/14, /* data=*/13, /* cs=*/15, /* dc=*/4, /* reset=*/2); // ePaper Display, lesser flickering and faster speed, enable 16 bit mode for this display!

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

String cityCode = "101010900";
//北京丰台区 101010900
//北京房山区 101011200

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 12

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;

String ds18b20Temp = "0.0";

//Todoist 配置
String projectId;
String authorization;

//函数声明
void handleCLEAR();
void clearDis();
void initNTP();
void webInit();
void myMDNSinit();
void returnRoot();
void wifiStatus(String myStatus, bool needClear);
void configModeCallback(WiFiManager *myWiFiManager);
void updateDisplay(String todo);
String changeWeek(int weekSum);
String getParam(String name);
void handler(Button2 &btn);
void getCityCode();
void getCityWeater();
void weaterData(String *cityDZ, String *dataSK, String *dataFC);
void printTemperature(DeviceAddress deviceAddress);
void printAddress(DeviceAddress deviceAddress);
void getTemp();

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

  button.setClickHandler(handler);
  button.setLongClickHandler(handler);
  button.setDoubleClickHandler(handler);
  button.setTripleClickHandler(handler);

  // // locate devices on the bus
  // Serial.print("Locating devices...");
  // sensors.begin();
  // Serial.print("Found ");
  // Serial.print(sensors.getDeviceCount(), DEC);
  // Serial.println(" devices.");

  // // report parasite power requirements
  // Serial.print("Parasite power is: ");
  // if (sensors.isParasitePowerMode())
  //   Serial.println("ON");
  // else
  //   Serial.println("OFF");
  // if (!sensors.getAddress(insideThermometer, 0))
  //   Serial.println("Unable to find address for Device 0");
  // // show the addresses we found on the bus
  // Serial.print("Device 0 Address: ");
  // printAddress(insideThermometer);
  // Serial.println();

  // // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  // sensors.setResolution(insideThermometer, 9);

  // Serial.print("Device 0 Resolution: ");
  // Serial.print(sensors.getResolution(insideThermometer), DEC);
  // Serial.println();
}

void loop()
{
  // delay(5000);
  // getCityWeater();

  // getTemp();

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

  // //每隔1小时，刷新一次屏幕
  // if (lastHour != thisHour)
  // {
  //   clearDis();
  //   lastHour = thisHour;
  // }
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

    // u8g2.drawFrame(0, 96, 294, 30);
    // u8g2.drawLine(141, 96, 141, 126);
    // u8g2.drawLine(64, 96, 64, 126);
    // u8g2.drawLine(98, 96, 98, 126);

    u8g2.setFont(u8g2_font_wqy16_t_gb2312b);
    u8g2.setCursor(2, 117);
    u8g2.print(myDate);

    u8g2.setCursor(67, 117);
    u8g2.print("周" + changeWeek(weekdays));

    u8g2.setFont(u8g2_font_wqy16_t_gb2312b);
    u8g2.setCursor(142, 117);
    u8g2.print(todo);
  } while (u8g2.nextPage());
  u8g2.sleepOn();
  Serial.println("u8g2.sleepOn().... ");
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
  todo = getParam("todo");
  Serial.println("todo:" + todo);
  handleCLEAR();
}
void handleCLEAR()
{
  clearDis();
  returnRoot();
  updateDisplay(todo);
}
void handleTODOIST()
{
  projectId = getParam("projectId");
  authorization = getParam("authorization");
  Serial.println("handleTODOIST projectId: " + projectId);
  Serial.println("handleTODOIST authorization: " + authorization);
  handleCLEAR();
}
String getParam(String name)
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

void returnRoot()
{
  esp8266_server.sendHeader("Location", "/"); // 跳转回页面根目录
  esp8266_server.send(303);                   // 发送Http相应代码303 跳转
}
void clearDis()
{
  // epd.Init(lut_full_update);
  // EPD_2IN9_FULL			0
  // EPD_2IN9_PART			1
  //   EPD_2IN9_Init(EPD_2IN9_FULL);
  // EPD_2IN9_Init(EPD_2IN9_PART);
  // EPD_2IN9_Clear();

  epd.ClearFrameMemory(0xFF); // bit set = white, bit reset = black
  epd.DisplayFrame();
  epd.ClearFrameMemory(0xFF); // bit set = white, bit reset = black
  epd.DisplayFrame();

  delay(500);

  // epd.ClearFrameMemory(0x00); // bit set = white, bit reset = black
  // epd.DisplayFrame();
  // epd.ClearFrameMemory(0x00); // bit set = white, bit reset = black
  // epd.DisplayFrame();

  // delay(500);

  // u8g2.begin();
  // u8g2.enableUTF8Print();
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
void handler(Button2 &btn)
{
  switch (btn.getClickType())
  {
  case SINGLE_CLICK:
    Serial.print("single ");
    clearDis();
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
// 发送HTTP请求并且将服务器响应通过串口输出
void getCityCode()
{
  String URL = "http://wgeo.weather.com.cn/ip/?_=" + String(now());
  //创建 HTTPClient 对象
  HTTPClient httpClient;

  //配置请求地址。此处也可以不使用端口号和PATH而单纯的
  httpClient.begin(URL);

  //设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://www.weather.com.cn/");

  //启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.print("Send GET request to URL: ");
  Serial.println(URL);

  //如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK)
  {
    String str = httpClient.getString();

    int aa = str.indexOf("id=");
    if (aa > -1)
    {
      //cityCode = str.substring(aa+4,aa+4+9).toInt();
      cityCode = str.substring(aa + 4, aa + 4 + 9);
      Serial.println(cityCode);
      getCityWeater();
    }
    else
    {
      Serial.println("获取城市代码失败");
    }
  }
  else
  {
    Serial.println("请求城市代码错误：");
    Serial.println(httpCode);
  }

  //关闭ESP8266与服务器连接
  httpClient.end();
}

// 获取城市天气
void getCityWeater()
{
  String URL = "http://d1.weather.com.cn/weather_index/" + cityCode + ".html?_=" + String(now());
  //创建 HTTPClient 对象
  HTTPClient httpClient;

  httpClient.begin(URL);

  //设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://www.weather.com.cn/");

  //启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.println("正在获取天气数据");
  Serial.println(URL);

  //如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK)
  {

    String str = httpClient.getString();
    int indexStart = str.indexOf("weatherinfo\":");
    int indexEnd = str.indexOf("};var alarmDZ");

    String jsonCityDZ = str.substring(indexStart + 13, indexEnd);
    Serial.println(jsonCityDZ);

    indexStart = str.indexOf("dataSK =");
    indexEnd = str.indexOf(";var dataZS");
    String jsonDataSK = str.substring(indexStart + 8, indexEnd);
    Serial.println(jsonDataSK);

    indexStart = str.indexOf("\"f\":[");
    indexEnd = str.indexOf(",{\"fa");
    String jsonFC = str.substring(indexStart + 5, indexEnd);
    Serial.println(jsonFC);

    weaterData(&jsonCityDZ, &jsonDataSK, &jsonFC);
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
void weaterData(String *cityDZ, String *dataSK, String *dataFC)
{

  DynamicJsonDocument doc(512);
  deserializeJson(doc, *dataSK);
  JsonObject sk = doc.as<JsonObject>();

  String temp = sk["temp"].as<String>();
  Serial.println("温度：" + temp);

  //PM2.5空气指数
  String aqiTxt = "优";
  int pm25V = sk["aqi"];
  if (pm25V > 200)
  {
    aqiTxt = "重度";
  }
  else if (pm25V > 150)
  {
    aqiTxt = "中度";
  }
  else if (pm25V > 100)
  {
    aqiTxt = "轻度";
  }
  else if (pm25V > 50)
  {
    aqiTxt = "良";
  }
  Serial.println("PM2.5空气指数：" + aqiTxt);

  //实时天气
  String realWeather = sk["weather"].as<String>();
  Serial.println("实时天气：" + realWeather);

  deserializeJson(doc, *dataFC);
  JsonObject fc = doc.as<JsonObject>();
  String lowTemp = fc["fd"].as<String>();
  Serial.println("最低温度：" + lowTemp);
  String highTemp = fc["fc"].as<String>();
  Serial.println("最高温度：" + highTemp);
}
// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  // method 1 - slower
  //Serial.print("Temp C: ");
  //Serial.print(sensors.getTempC(deviceAddress));
  //Serial.print(" Temp F: ");
  //Serial.print(sensors.getTempF(deviceAddress)); // Makes a second call to getTempC and then converts to Fahrenheit

  // method 2 - faster
  float tempC = sensors.getTempC(deviceAddress);
  if (tempC == DEVICE_DISCONNECTED_C)
  {
    Serial.println("Error: Could not read temperature data");
    return;
  }
  Serial.print("Temp C: ");
  ds18b20Temp = String(tempC);
  Serial.print(ds18b20Temp);
}
// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16)
      Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
void getTemp()
{
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");

  // It responds almost immediately. Let's print out the data
  printTemperature(insideThermometer); // Use a simple function to print out the data
}