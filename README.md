# 电子墨水屏项目

## 采用微雪2.9英寸黑白双色墨水屏，以及ESP8266 E12驱动

### 已经实现：
- 时间
- 日期
- 星期
- wifiManager自动配网
- 温度
- 湿度
- 提醒事项
- mDNS(通过EPaperThing.local可以访问esp启动的webServer页面)
### 计划实现：
- 日历
- 天气

### arduino环境配置
- arduino IDE中文件-》首选项-》附加开发版管理器网址 添加：http://arduino.esp8266.com/stable/package_esp8266com_index.json
- 工具-》开发版管理器-》搜esp8266 下载

### 需要引入arduino库
- name : 'Time' ,"authors" : "Michael Margolis" ,link : "https://github.com/PaulStoffregen/Time"
- name : "Timezone" ,"authors" : "Jack Christensen" ,link : "https://github.com/JChristensen/Timezone"
- name : "WifiManager" ,"" : "" ,link : "https://github.com/tzapu/WiFiManager"
- name : "DHTesp" ,"authors" : "beegee-tokyo" ,link : "https://github.com/beegee-tokyo/DHTesp"
- name : "U8g2" ,"authors" : "oliver" ,link : "https://github.com/olikraus/u8g2"
### 串口驱动
- CP210x USB to UART Bridge VCP Drivers ,link: "https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers"

![avatar](https://github.com/88431844/ePaperThing/blob/master/EpaperThingImg.JPG)
