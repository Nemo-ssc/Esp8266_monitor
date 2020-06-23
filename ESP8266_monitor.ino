/**********************************************************************
 * 项目：ESP8266_monitor
 * 硬件：适用于NodeMCU ESP8266 + ssd1306 128*32
 * 功能：连接WiFi后获取指定用户的哔哩哔哩实时粉丝数+WIFI网络授时时钟+知心天气
 * 作者：Nemo_ssc  bilibili UID:12772522
 * 日期：2020/06/20
 **********************************************************************/
//----------------------调用库头文件---------------------
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <math.h>
#include "DHT.h"
#define DHTTYPE DHT11
#define DHTPIN 2

DHT dht(DHTPIN, DHTTYPE);

U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);

//---------------wifi连接/API用户名密码信息----------------
const char *ssid = "********";          //WiFi名
const char *password = "********";  //WiFi密码

String biliuid = "********";         //bilibili UID

const char* HOST = "http://api.seniverse.com";
const char* APIKEY = "********";        //API KEY 知心天气私钥
const char* CITY = "********";  //城市全拼
const char* LANGUAGE = "zh-Hans";//zh-Hans 简体中文 
//-------------------------------------------------------

//-----------------------天气参数-------------------------
typedef struct
{
    String date_m;
    String date_d;
    int code_day;
    int high;
    int low;
    int wind_direction_degree;
    int wind_scale;
    int humidity;
}weather_date; //天气信息的结构体

weather_date day0,day1,day2;
//-------------------------------------------------------

//-----------------------定义常量-------------------------
int Click = 0;  //轻触开关引脚
int beep = 13;  //蜂鸣器引脚
const unsigned long HTTP_TIMEOUT = 5000;  //http访问请求
WiFiClient client;
HTTPClient http;

String response_bff;  //json返回粉丝数
String response_bfv;  //json返回观看数、点赞数
int follower = 0;  //粉丝数
long viewer = 0;  //浏览量
long likes = 0;  //点赞数
const int slaveSelect = 5;
const int scanLimit = 7;

static const char ntpServerName[] = "ntp1.aliyun.com"; //NTP服务器，阿里云
const int timeZone = 8;                                //时区，北京时间为+8
WiFiUDP Udp;
unsigned int localPort = 8888; // 用于侦听UDP数据包的本地端口
time_t getNtpTime();
boolean isNTPConnected = false;

String response_ws;  //json返回天气信息
String GetUrl;  //心知天气url请求
//------------------------------------------------------------------

//---------------------------位图信息--------------------------------
//24*24小电视
const unsigned char bilibilitv_24u[] U8X8_PROGMEM = {
0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x30, 0x00, 0x01, 0xe0, 0x80, 0x01,
0x80, 0xc3, 0x00, 0x00, 0xef, 0x00, 0xff, 0xff, 0xff, 0x03, 0x00, 0xc0, 0xf9, 0xff, 0xdf, 0x09, 0x00, 0xd0, 0x09, 0x00, 0xd0, 0x89, 0xc1,
0xd1, 0xe9, 0x81, 0xd3, 0x69, 0x00, 0xd6, 0x09, 0x91, 0xd0, 0x09, 0xdb, 0xd0, 0x09, 0x7e, 0xd0, 0x0d, 0x00, 0xd0, 0x4d, 0x89, 0xdb, 0xfb,
0xff, 0xdf, 0x03, 0x00, 0xc0, 0xff, 0xff, 0xff, 0x78, 0x00, 0x1e, 0x30, 0x00, 0x0c};
//128*32小电视bilibili                                                    
const unsigned char bilibilitv_12832[] U8X8_PROGMEM ={
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x1C,0x00,0x0E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x1C,0x00,0x0E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x7C,0xC0,0x0F,0x00,0x1E,0x00,0x00,0x00,0xE0,0x03,0x00,0x00,0x00,0x00,
0x00,0x00,0xF0,0xE0,0x03,0x80,0x1F,0x00,0x00,0x00,0xF8,0x03,0x00,0x00,0x00,0x00,
0x00,0xF8,0xFF,0xFF,0xFF,0x83,0x1F,0x00,0x00,0x00,0xF8,0x03,0x00,0x00,0x00,0x00,
0x00,0xFC,0xFF,0xFF,0xFF,0x8F,0x1F,0x00,0x80,0x07,0xF8,0x03,0x00,0x78,0x00,0x00,
0x00,0xFE,0xFF,0xFF,0xFF,0x9F,0x1F,0x00,0x80,0x0F,0xF8,0x03,0x00,0xF8,0x00,0x00,
0x00,0xFE,0xFF,0xFF,0xFF,0x9F,0x1F,0x00,0x80,0x0F,0xF8,0x03,0x00,0xF8,0x00,0x00,
0x00,0x3F,0x00,0x00,0x80,0x9F,0x1F,0x00,0x80,0x0F,0xF0,0x03,0x00,0xF0,0x00,0x00,
0x00,0x1F,0x00,0x00,0x00,0xBE,0x1F,0x00,0x80,0x0F,0xF0,0x03,0x00,0xF0,0x00,0x00,
0x00,0x1F,0x00,0x00,0x00,0xBE,0x1F,0x00,0x00,0x0F,0xF0,0x03,0x00,0xF0,0x00,0x00,
0x00,0x1F,0x00,0x00,0x00,0xBE,0x1F,0x00,0x7C,0xEF,0xF3,0x03,0xC0,0xF7,0x7E,0x00,
0x00,0x1F,0x7F,0xC0,0x1F,0x3E,0x1F,0x00,0x7C,0xEF,0xF3,0x03,0xC0,0xF7,0x7E,0x00,
0x00,0xDF,0xFF,0xC0,0x3F,0x3E,0x1F,0x00,0x7C,0xEF,0xE3,0x03,0xC0,0xF7,0x78,0x00,
0x00,0x1F,0x1F,0x00,0x3E,0x3E,0x1F,0x00,0x18,0xEF,0xE3,0x03,0x80,0xE0,0x78,0x00,
0x00,0x1F,0x00,0x00,0x00,0x3E,0x1F,0x00,0x78,0x1F,0xE0,0x03,0x80,0xEF,0x00,0x00,
0x00,0x1F,0x00,0x00,0x00,0x3E,0x1E,0x00,0x78,0xFE,0xE3,0x03,0x80,0xEF,0x79,0x00,
0x00,0x1F,0xC0,0x4C,0x00,0x3E,0x3E,0x00,0xF8,0xFE,0xE3,0x03,0x80,0xEF,0x79,0x00,
0x00,0x1F,0xC0,0xFF,0x00,0x3E,0xFE,0x1F,0xF0,0xDE,0xE3,0xFF,0x87,0xEF,0xF9,0x00,
0x00,0x1F,0xC0,0xFF,0x00,0x3E,0xFE,0xFF,0xF0,0xDE,0xE3,0xFF,0x9F,0xEF,0xF9,0x00,
0x00,0x1F,0x80,0x3B,0x00,0x3E,0xFE,0xFF,0xF7,0xDE,0xE3,0xFF,0x7F,0xEF,0xF9,0x00,
0x00,0x1F,0x00,0x00,0x00,0x3E,0xFE,0xFF,0xFF,0xDE,0xE3,0xFF,0xFF,0xCF,0xF9,0x00,
0x00,0x3F,0x00,0x00,0x80,0x1F,0xFC,0xF8,0xEF,0xDF,0xC3,0x3F,0xFF,0xDF,0xF9,0x00,
0x00,0xFE,0xFF,0xFF,0xFF,0x1F,0xFC,0xFD,0xEF,0xDD,0xC3,0xBF,0xFF,0xDE,0xF1,0x00,
0x00,0xFE,0xFF,0xFF,0xFF,0x1F,0xFC,0xFF,0xEF,0xDD,0xC3,0xFF,0xFF,0xDE,0xF1,0x00,
0x00,0xFC,0xFF,0xFF,0xFF,0x0F,0xFC,0xFF,0xE3,0xDD,0xC3,0xFF,0x3F,0xDE,0xF1,0x00,
0x00,0xFC,0xFF,0xFF,0xFF,0x0F,0xFC,0xFF,0xE3,0xDD,0xC3,0xFF,0x3F,0xDE,0xF1,0x00,
0x00,0x00,0x0F,0x00,0x3E,0x00,0xFC,0x1F,0x00,0x00,0xC0,0xFF,0x01,0x00,0x00,0x00,
0x00,0x00,0x0F,0x00,0x3E,0x00,0xFC,0x1F,0x00,0x00,0xC0,0xFF,0x01,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};       

//下面是天气图标和对应心知天气的天气代码
//Sunny = 0 晴
const unsigned char Sunny[] U8X8_PROGMEM ={
0x00,0xC0,0x00,0x00,0x00,0xC0,0x00,0x00,0x00,0xC0,0x00,0x00,0x00,0xC0,0x00,0x00,
0x30,0xC0,0x00,0x03,0x70,0x00,0x80,0x03,0xE0,0x00,0xC0,0x01,0xC0,0xE0,0xC1,0x00,
0x00,0xF8,0x07,0x00,0x00,0xFE,0x1F,0x00,0x00,0xFE,0x1F,0x00,0x00,0xFF,0x3F,0x00,
0x00,0xFF,0x3F,0x00,0x80,0xFF,0x7F,0x00,0x8F,0xFF,0x7F,0x3C,0x8F,0xFF,0x7F,0x3C,
0x80,0xFF,0x7F,0x00,0x80,0xFF,0x7F,0x00,0x00,0xFF,0x3F,0x00,0x00,0xFE,0x1F,0x00,
0x00,0xFE,0x1F,0x00,0x00,0xFC,0x0F,0x00,0xC0,0xF0,0xC3,0x00,0xE0,0x00,0xC0,0x01,
0x70,0x00,0x80,0x03,0x30,0x00,0x00,0x03,0x00,0xC0,0x00,0x00,0x00,0xC0,0x00,0x00,
0x00,0xC0,0x00,0x00,0x00,0xC0,0x00,0x00                                            
};
//Cloudy = 4 多云
const unsigned char Cloudy[] U8X8_PROGMEM ={
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0E,0x00,
0x00,0x80,0x3F,0x00,0x00,0xE6,0xFF,0x00,0x00,0xFF,0xFF,0x00,0x80,0xFF,0xFF,0x00,
0x80,0xFF,0xFF,0x01,0xC0,0xFF,0xFF,0x01,0xC0,0xFF,0xFF,0x03,0xE0,0xFF,0xFF,0x07,
0xF0,0xFF,0xFF,0x0F,0xF8,0xFF,0x07,0x0F,0xF8,0xFF,0x71,0x0F,0xF8,0xFF,0xFE,0x0E,
0xF8,0x7F,0xFE,0x08,0xF0,0x7F,0xFF,0x07,0xE0,0x1F,0xFF,0x0F,0xC0,0xEF,0xFF,0x1F,
0x80,0xE7,0xFF,0x1F,0x00,0xF0,0xFF,0x3F,0x1C,0xF0,0xFF,0x3F,0x3C,0xF0,0xFF,0x1F,
0x7E,0xF0,0xFF,0x1F,0x7F,0xE0,0xFF,0x0F,0x3E,0xC0,0xFF,0x07,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00                                            
};
//Overcast = 9 阴
const unsigned char Overcast[] U8X8_PROGMEM ={
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x80,0x1F,0x00,0x00,0xE0,0x7F,0x00,0x00,0xF3,0xFF,0x00,
0xC0,0xFF,0xFF,0x01,0xE0,0xFF,0xFF,0x01,0xF0,0xFF,0xFF,0x01,0xF0,0xFF,0xFF,0x03,
0xF0,0xFF,0xFF,0x03,0xF8,0xFF,0xFF,0x07,0xFC,0xFF,0xFF,0x0F,0xFE,0xFF,0xFF,0x1F,
0xFF,0xFF,0xFF,0x1F,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,
0xFF,0xFF,0xFF,0x1F,0xFE,0xFF,0xFF,0x1F,0xFE,0xFF,0xFF,0x0F,0xF8,0xFF,0xFF,0x07,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00                                      
};
//Shower = 10 阵雨
const unsigned char Shower[] U8X8_PROGMEM ={
0x00,0x00,0x70,0x00,0x00,0x00,0xFC,0x03,0x00,0x00,0xFE,0x07,0x00,0x00,0xF8,0x0F,
0x00,0xC0,0xE3,0x1F,0x00,0xF0,0xCF,0x1F,0xC0,0xF9,0x9F,0x3F,0xE0,0xFF,0xBF,0x3F,
0xF0,0xFF,0x3F,0x3F,0xF0,0xFF,0x7F,0x1F,0xF0,0xFF,0x7F,0x1E,0xF8,0xFF,0xFF,0x1C,
0xFC,0xFF,0xFF,0x09,0xFE,0xFF,0xFF,0x03,0xFE,0xFF,0xFF,0x03,0xFF,0xFF,0xFF,0x07,
0xFF,0xFF,0xFF,0x07,0xFF,0xFF,0xFF,0x07,0xFF,0xFF,0xFF,0x03,0xFE,0xFF,0xFF,0x03,
0xFC,0xFF,0xFF,0x01,0xF0,0xFF,0x7F,0x00,0x00,0x00,0x00,0x00,0x40,0x40,0x20,0x00,
0x60,0x60,0x30,0x00,0x60,0x60,0x30,0x00,0x30,0x30,0x18,0x00,0x30,0x30,0x18,0x00,
0x18,0x18,0x0C,0x00,0x08,0x08,0x04,0x00                                           
};
//Thundershower = 11 雷阵雨
const unsigned char Thundershower[] U8X8_PROGMEM ={
0x00,0x00,0x0E,0x00,0x00,0xC0,0x3F,0x00,0x00,0xE0,0xFF,0x00,0xC0,0xFF,0xFF,0x01,
0xE0,0xFF,0xFF,0x01,0xF0,0xFF,0xFF,0x01,0xF0,0xFF,0xFF,0x03,0xF0,0xFF,0xFF,0x03,
0xF0,0xFF,0xFF,0x03,0xF8,0xFF,0xFF,0x07,0xFC,0xFF,0xFF,0x0F,0xFE,0xFF,0xFF,0x1F,
0xFF,0xFF,0xFF,0x1F,0xFF,0xFF,0xFF,0x1F,0xFF,0xFF,0xFF,0x1F,0xFF,0xFF,0xFF,0x1F,
0xFE,0xFF,0xFF,0x1F,0xFE,0xFF,0xFF,0x0F,0xFC,0xFF,0xFF,0x07,0xF0,0xFF,0xFF,0x01,
0x00,0xF0,0x01,0x00,0x60,0xF0,0xC1,0x00,0x60,0xF0,0xC0,0x00,0x60,0x78,0xC0,0x00,
0x30,0xFC,0x60,0x00,0x30,0x70,0x60,0x00,0x10,0x30,0x20,0x00,0x10,0x18,0x20,0x00,
0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00                                           
};
//Rain_L = 13 小雨
const unsigned char Rain_L[] U8X8_PROGMEM ={
0x00,0x00,0x00,0x00,0x00,0x00,0x0E,0x00,0x00,0xC0,0x3F,0x00,0x00,0xE0,0xFF,0x00,
0xC0,0xFF,0xFF,0x01,0xE0,0xFF,0xFF,0x01,0xF0,0xFF,0xFF,0x01,0xF0,0xFF,0xFF,0x03,
0xF0,0xFF,0xFF,0x03,0xF0,0xFF,0xFF,0x03,0xF8,0xFF,0xFF,0x0F,0xFC,0xFF,0xFF,0x0F,
0xFE,0xFF,0xFF,0x1F,0xFF,0xFF,0xFF,0x1F,0xFF,0xFF,0xFF,0x1F,0xFF,0xFF,0xFF,0x1F,
0xFF,0xFF,0xFF,0x1F,0xFE,0xFF,0xFF,0x1F,0xFE,0xFF,0xFF,0x0F,0xFC,0xFF,0xFF,0x07,
0xE0,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x60,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x10,0x00,0x00,
0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00                                            
};
//Rain_M = 14 中雨
const unsigned char Rain_M[] U8X8_PROGMEM ={
0x00,0x00,0x00,0x00,0x00,0x00,0x0E,0x00,0x00,0xC0,0x3F,0x00,0x00,0xE0,0xFF,0x00,
0xC0,0xFF,0xFF,0x01,0xE0,0xFF,0xFF,0x01,0xF0,0xFF,0xFF,0x01,0xF0,0xFF,0xFF,0x03,
0xF0,0xFF,0xFF,0x03,0xF0,0xFF,0xFF,0x03,0xF8,0xFF,0xFF,0x0F,0xFC,0xFF,0xFF,0x0F,
0xFE,0xFF,0xFF,0x1F,0xFF,0xFF,0xFF,0x1F,0xFF,0xFF,0xFF,0x1F,0xFF,0xFF,0xFF,0x1F,
0xFF,0xFF,0xFF,0x1F,0xFE,0xFF,0xFF,0x1F,0xFE,0xFF,0xFF,0x0F,0xFC,0xFF,0xFF,0x07,
0xE0,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x0C,0x0C,0x00,0x00,0x04,0x04,0x00,0x00,0x06,0x06,0x00,0x00,0x02,0x02,0x00,
0x00,0x03,0x03,0x00,0x00,0x00,0x00,0x00                                   
};
//Rain_H = 15 大雨
const unsigned char Rain_H[] U8X8_PROGMEM ={
0x00,0x00,0x00,0x00,0x00,0x00,0x0E,0x00,0x00,0xC0,0x3F,0x00,0x00,0xE0,0xFF,0x00,
0xC0,0xFF,0xFF,0x01,0xE0,0xFF,0xFF,0x01,0xF0,0xFF,0xFF,0x01,0xF0,0xFF,0xFF,0x03,
0xF0,0xFF,0xFF,0x03,0xF0,0xFF,0xFF,0x03,0xF8,0xFF,0xFF,0x0F,0xFC,0xFF,0xFF,0x0F,
0xFE,0xFF,0xFF,0x1F,0xFF,0xFF,0xFF,0x1F,0xFF,0xFF,0xFF,0x1F,0xFF,0xFF,0xFF,0x1F,
0xFF,0xFF,0xFF,0x1F,0xFE,0xFF,0xFF,0x1F,0xFE,0xFF,0xFF,0x0F,0xFC,0xFF,0xFF,0x07,
0xE0,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x60,0x60,0x60,0x00,0x20,0x20,0x20,0x00,0x30,0x30,0x30,0x00,0x10,0x10,0x10,0x00,
0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00                                            
};
//Foggy = 30 雾
const unsigned char Foggy[] U8X8_PROGMEM ={
0x00,0x00,0x00,0x00,0x00,0x1E,0x00,0x00,0xC0,0xFF,0x00,0x00,0xF0,0xFF,0x01,0x00,
0xF8,0xFF,0x03,0x00,0xFC,0xFF,0x07,0x00,0xFC,0xFF,0x0F,0x00,0xFE,0xFF,0xFF,0x01,
0xFE,0xFF,0xFF,0x07,0xFE,0xFF,0xFF,0x0F,0xFE,0xFF,0xFF,0x0F,0xFE,0xFF,0xFF,0x1F,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFE,0xFF,0xFF,0x1F,
0xFE,0xFF,0xFF,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0xFE,0xFF,0xFF,0x1F,0xFE,0xFF,0xFF,0x1F,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFE,0xFF,0xFF,0x1F,
0xFE,0xFF,0xFF,0x1F,0x00,0x00,0x00,0x00                                            
};
//Haze = 31 霾
const unsigned char Haze[] U8X8_PROGMEM ={
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0E,0xE0,0x00,0x1C,0x0E,0xE0,0x00,0x1C,
0x0E,0xE0,0x00,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0xF0,0x03,0xF0,0x01,0xF8,0x0F,0xFC,0x07,0x1C,0x1E,0x0E,0x0E,
0x0E,0x38,0x07,0x1C,0x06,0x30,0x03,0x18,0x07,0xF0,0x03,0x38,0x03,0xE0,0x01,0x30,
0x03,0xE0,0x01,0x30,0x07,0x30,0x03,0x38,0x06,0x38,0x07,0x18,0x0E,0x1C,0x0E,0x1C,
0x1C,0x0F,0x1C,0x0F,0xF8,0x07,0xF8,0x03,0xF0,0x03,0xF0,0x01,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0E,0xE0,0x00,0x1C,0x0E,0xE0,0x00,0x1C,
0x0E,0xE0,0x00,0x1C,0x00,0x00,0x00,0x00                                            
};

//浏览和点赞的图标
//viewer
const unsigned char viewer_bmp[] U8X8_PROGMEM ={
0x00,0x00,0x00,0x00,0x80,0x00,0x8C,0x18,0x98,0x0C,0xF0,0x07,0x38,0x0E,0x0C,0x18,
0xC6,0x31,0x63,0x63,0x23,0x62,0x60,0x03,0xC0,0x01,0x00,0x00,0x00,0x00  
};
//likes
const unsigned char likes_bmp[] U8X8_PROGMEM ={
0x00,0x00,0x00,0x00,0xC0,0x00,0xC0,0x00,0xE0,0x00,0xE0,0x00,0xF0,0x7F,0xF7,0x7F,
0xF7,0x7F,0xF7,0x7F,0xF7,0x3F,0xF7,0x3F,0xF7,0x1F,0xF7,0x1F,0x00,0x00    
};
//------------------------------------------------------------------
//-------------------------中断执行函数-------------------------------
//------------------------------------------------------------------
ICACHE_RAM_ATTR void onChange()
{
    delay(100);  //延时消抖
    if(digitalRead(15) == HIGH)
    {
        Click ++;
        Serial.println("Key Down");
    }
    
}
//------------------------------------------------------------------
//---------------------------setup函数-------------------------------
//------------------------------------------------------------------
void setup()
{
    attachInterrupt(15, onChange, RISING);  //中断控制 引脚为内部GPIO口号
    pinMode(beep, OUTPUT);
    u8g2.begin();  //启用u8g2库
    u8g2.clearBuffer();
    u8g2.drawXBMP( 0 , 0 , 128 , 32 , bilibilitv_12832 );  //开机画面
    u8g2.sendBuffer();
    Serial.begin(115200);  //串口通讯波特率
    while (!Serial)
        continue;
    Serial.println("bilibili fans monitor, version v1.2");
    Serial.print("Connecting WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);  //连接WIFI
    while (WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    Serial.println("Starting UDP");
    Udp.begin(localPort);
    Serial.print("Local port: ");
    Serial.println(Udp.localPort());
    Serial.println("waiting for sync");
    setSyncProvider(getNtpTime);
    setSyncInterval(300); //每300秒同步一次时间
    isNTPConnected = true;

    dht.begin();
}
//------------------------------------------------------------------
//--------------------bilibili_fans_follower------------------------
//------------------------------------------------------------------
bool getJson()  //同时获得粉丝数和浏览量的json数据
{
    bool r = false;
    http.setTimeout(HTTP_TIMEOUT);
    http.begin("http://api.bilibili.com/x/relation/stat?vmid=" + biliuid);
    int httpCode = http.GET();
    if (httpCode > 0){
        if (httpCode == HTTP_CODE_OK){
            response_bff = http.getString();
            Serial.println(response_bff);
            r = true;
        }
    }else{
        Serial.printf("[HTTP] GET JSON failed, error: %s\n", http.errorToString(httpCode).c_str());
        //errorCode(0x2);
        r = false;;
    }
    http.end();
    //return t;

    //view
    http.begin("http://api.bilibili.com/x/space/upstat?mid=" + biliuid);
    int httpCode_v = http.GET();
    if (httpCode_v > 0){
        if (httpCode_v == HTTP_CODE_OK){
            response_bfv = http.getString();
            //Serial.println(response_bfv);
            r = true;
        }
    }else{
        Serial.printf("[HTTP] GET JSON failed, error: %s\n", http.errorToString(httpCode_v).c_str());
        r = false;
    }
    http.end();

    return r;
}

bool parseJson_bff(String json)   //对粉丝数的json解析
{
    const size_t capacity = JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 70;//分配内存大小
    DynamicJsonDocument doc(capacity);  //解析Json
    deserializeJson(doc, json);

    int code = doc["code"];  //返回json
    const char *message = doc["message"];

    if (code != 0){          //根据code判断返回数据状态，并输出错误信息代码
        Serial.print("[API]Code:");
        Serial.print(code);
        Serial.print(" Message:");
        Serial.println(message);
        //errorCode(0x3);
        return false;
    }

    JsonObject data = doc["data"];  //Json对象
    unsigned long data_mid = data["mid"];
    int data_follower = data["follower"];
    if (data_mid == 0){
        Serial.println("[JSON] FORMAT ERROR");
        //errorCode(0x4);
        return false;
    }
    Serial.print("UID: ");
    Serial.print(data_mid);
    Serial.print(" follower: ");
    Serial.println(data_follower);

    follower = data_follower;
    return true;
}

void displayNumber_f(int number)  //oled输出粉丝数
{
    u8g2.setFont(u8g2_font_freedoomr25_tn); // choose a suitable font
    u8g2.setCursor(40, 30);
    u8g2.print(number);
    u8g2.sendBuffer();          // transfer internal memory to the display
}

void displayBmp_f()  //输出图片
{
    u8g2.clearBuffer();
    u8g2.drawXBMP( 4 , 4 , 24 , 24 , bilibilitv_24u );
}

void bilibili_fans_follower()  //获取json数据并解析成功
{
    if (getJson()){
        if (parseJson_bff(response_bff)){
            displayBmp_f();
            displayNumber_f(follower);
            delay(10);
        }
    }
    else{
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_luBIS08_tr); 
        u8g2.drawStr(20,20,"Get Data .");
        u8g2.sendBuffer();
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_luBIS08_tr); 
        u8g2.drawStr(20,20,"Get Data ..");
        u8g2.sendBuffer();
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_luBIS08_tr); 
        u8g2.drawStr(20,20,"Get Data ...");
        u8g2.sendBuffer();
    }
}

//------------------------------------------------------------------
//--------------------bilibili_fans_viewer--------------------------
//------------------------------------------------------------------

bool parseJson_bfv(String json)  //对浏览量的json解析
{
    const size_t capacity = 2*JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4) + 70;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, json);
 
    int code = doc["code"]; // 0
    const char* message = doc["message"]; // "0"
    int ttl = doc["ttl"]; // 1

    JsonObject data = doc["data"];

    long data_archive_view = data["archive"]["view"]; // 2974603
    int data_article_view = data["article"]["view"]; // 1218
    long data_likes = data["likes"];
    viewer = data_archive_view + data_article_view;
    likes = data_likes;
}

void displayBmp_v()  //输出图片
{
    u8g2.clearBuffer();
    u8g2.drawXBMP( 4 , 4 , 24 , 24 , bilibilitv_24u );  
    u8g2.drawXBMP( 100 , 1 , 15 , 15 , viewer_bmp );
    u8g2.drawXBMP( 100 , 17 , 15 , 15 , likes_bmp );
}

void displayNumber_v(int vw,int lk)  //oled输出浏览量和点赞数
{
    u8g2.setFont(u8g2_font_freedoomr10_tu); // choose a suitable font
    u8g2.setCursor(40, 15);
    u8g2.print(vw);
    u8g2.setCursor(40, 31);
    u8g2.print(lk);
    u8g2.sendBuffer();
}

void bilibili_fans_viewer()   //获取json数据并解析成功
{    
    if (getJson()){
        if (parseJson_bfv(response_bfv)){
            displayBmp_v();
            displayNumber_v(viewer,likes);
            delay(10);
        }
    } 
}


//------------------------------------------------------------------
//-----------------------NTP_clock----------------------------------
//------------------------------------------------------------------
const int NTP_PACKET_SIZE = 48;     // NTP时间在消息的前48个字节里
byte packetBuffer[NTP_PACKET_SIZE]; // 输入输出包的缓冲区

time_t getNtpTime()
{
  IPAddress ntpServerIP;          // NTP服务器的地址

  while (Udp.parsePacket() > 0);  // 丢弃以前接收的任何数据包
  Serial.println("Transmit NTP Request");
  // 从池中获取随机服务器
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500)
  {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE)
    {
      Serial.println("Receive NTP Response");
      isNTPConnected = true;
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // 将数据包读取到缓冲区
      unsigned long secsSince1900;
      // 将从位置40开始的四个字节转换为长整型，只取前32位整数部分
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      Serial.println(secsSince1900);
      Serial.println(secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-("); //无NTP响应
  isNTPConnected = false;
  return 0; //如果未得到时间则返回0
}


// 向给定地址的时间服务器发送NTP请求
void sendNTPpacket(IPAddress& address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  Udp.beginPacket(address, 123); //NTP需要使用的UDP端口号为123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
void NTP_CLOCK()  //oled输出时分秒，没有十位的补0
{
    int hours, minutes, seconds;
    hours = hour();
    minutes = minute();
    seconds = second();
    Serial.printf("%d:%d:%d\n", hours, minutes, seconds);
    String currentTime_hm = "";
    String currentTime_s = "";
    if (hours < 10)
        currentTime_hm += 0;
    currentTime_hm += hours;
    currentTime_hm += ":";
    if (minutes < 10)
        currentTime_hm += 0;
    currentTime_hm += minutes;
    //currentTime += ":";
    if (seconds < 10)
        currentTime_s += 0;
    currentTime_s += seconds;
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_freedoomr25_tn); // choose a suitable font
    u8g2.setCursor(0, 30);
    u8g2.print(currentTime_hm);  // write something to the internal memory
    u8g2.setFont(u8g2_font_freedoomr10_tu); // choose a suitable font
    u8g2.setCursor(90, 30);
    u8g2.print(currentTime_s);  // write something to the internal memory
    u8g2.setFont(u8g2_font_freedoomr10_tu); // choose a suitable font
    u8g2.setCursor(90, 15);
    u8g2.print("UTC+8");
    u8g2.sendBuffer();
}
void NTP_DAY()  //oled输出年月日+星期
{
    int years, months, days, weekdays;
    years = year();
    months = month();
    days = day();
    weekdays = weekday();
    Serial.printf("%d/%d/%d Weekday:%d\n", years, months, days, weekdays);
    String currentDay = "";
    if (months < 10)
      currentDay += 0;
    currentDay += months;
    currentDay += "/";
    if (days < 10)
      currentDay += 0;
    currentDay += days;
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB12_tr);
    u8g2.setCursor(0, 15);
    u8g2.print(years);
    u8g2.setCursor(0, 31);
    u8g2.print(currentDay);
    u8g2.setFont(u8g2_font_logisoso24_tr);
    u8g2.setCursor(60, 31);
    if (weekdays == 1)
      u8g2.print("SUN");
    else if (weekdays == 2)
      u8g2.print("MON");
    else if (weekdays == 3)
      u8g2.print("TUS");
    else if (weekdays == 4)
      u8g2.print("WED");
    else if (weekdays == 5)
      u8g2.print("THU");
    else if (weekdays == 6)
      u8g2.print("FRI");
    else if (weekdays == 7)
      u8g2.print("SAT");
    u8g2.sendBuffer();
}

//------------------------------------------------------------------
//------------------------weather_station---------------------------
//----------------------------api test------------------------------
//https://api.seniverse.com/v3/weather/now.json?key=SuQDmnIsehmcwH7cx&location=beijing&language=zh-Hans&unit=c
//https://api.seniverse.com/v3/weather/daily.json?key=SuQDmnIsehmcwH7cx&location=beijing&language=zh-Hans&unit=c&start=0&days=5
bool getJson_ws()  //构建并发出获取天气数据的请求，获得json数据
{
    bool s = false;
    GetUrl = String(HOST) + "/v3/weather/daily.json?key=";
    GetUrl += APIKEY;
    GetUrl += "&location=";
    GetUrl += CITY;
    GetUrl += "&language=";
    GetUrl += LANGUAGE;
    GetUrl += "&unit=c&start=0&days=3";
    http.setTimeout(HTTP_TIMEOUT);
    http.begin(GetUrl);
    int httpCode = http.GET();
    if (httpCode > 0){
        if (httpCode == HTTP_CODE_OK){
            response_ws = http.getString();
            //Serial.println(response_ws);
            s = true;
        }
    }else{
        Serial.printf("[HTTP] GET JSON failed, error: %s\n", http.errorToString(httpCode).c_str());
        s = false;
    }
    http.end();
    return s;
}

bool parseJson_ws(String json)  //对心知天气的json数据解析，采用arduinojson v6库
{
    const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(3) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 3*JSON_OBJECT_SIZE(14) + 810;
    DynamicJsonDocument doc(capacity);  //根据json数据结构计算所需要的内存大小
    deserializeJson(doc, json);  //反序列化json数据
    JsonObject results_0 = doc["results"][0];
    JsonArray results_0_daily = results_0["daily"];

    JsonObject results_0_daily_0 = results_0_daily[0];  //今天的天气数据
    const char* results_0_daily_0_date = results_0_daily_0["date"]; // "2020-06-20"
    const char* results_0_daily_0_text_day = results_0_daily_0["text_day"]; // "多云"
    const char* results_0_daily_0_code_day = results_0_daily_0["code_day"]; // "4"
    const char* results_0_daily_0_text_night = results_0_daily_0["text_night"]; // "多云"
    const char* results_0_daily_0_code_night = results_0_daily_0["code_night"]; // "4"
    const char* results_0_daily_0_high = results_0_daily_0["high"]; // "28"
    const char* results_0_daily_0_low = results_0_daily_0["low"]; // "20"
    const char* results_0_daily_0_rainfall = results_0_daily_0["rainfall"]; // "0.0"
    const char* results_0_daily_0_precip = results_0_daily_0["precip"]; // ""
    const char* results_0_daily_0_wind_direction = results_0_daily_0["wind_direction"]; // "东"
    const char* results_0_daily_0_wind_direction_degree = results_0_daily_0["wind_direction_degree"]; // "90"
    const char* results_0_daily_0_wind_speed = results_0_daily_0["wind_speed"]; // "16.20"
    const char* results_0_daily_0_wind_scale = results_0_daily_0["wind_scale"]; // "3"
    const char* results_0_daily_0_humidity = results_0_daily_0["humidity"]; // "72"

    JsonObject results_0_daily_1 = results_0_daily[1];  //明天的天气数据
    const char* results_0_daily_1_date = results_0_daily_1["date"]; // "2020-06-21"
    const char* results_0_daily_1_text_day = results_0_daily_1["text_day"]; // "多云"
    const char* results_0_daily_1_code_day = results_0_daily_1["code_day"]; // "4"
    const char* results_0_daily_1_text_night = results_0_daily_1["text_night"]; // "阴"
    const char* results_0_daily_1_code_night = results_0_daily_1["code_night"]; // "9"
    const char* results_0_daily_1_high = results_0_daily_1["high"]; // "27"
    const char* results_0_daily_1_low = results_0_daily_1["low"]; // "20"
    const char* results_0_daily_1_rainfall = results_0_daily_1["rainfall"]; // "0.0"
    const char* results_0_daily_1_precip = results_0_daily_1["precip"]; // ""
    const char* results_0_daily_1_wind_direction = results_0_daily_1["wind_direction"]; // "东南"
    const char* results_0_daily_1_wind_direction_degree = results_0_daily_1["wind_direction_degree"]; // "135"
    const char* results_0_daily_1_wind_speed = results_0_daily_1["wind_speed"]; // "16.20"
    const char* results_0_daily_1_wind_scale = results_0_daily_1["wind_scale"]; // "3"
    const char* results_0_daily_1_humidity = results_0_daily_1["humidity"]; // "68"

    JsonObject results_0_daily_2 = results_0_daily[2];  //后天的天气数据
    const char* results_0_daily_2_date = results_0_daily_2["date"]; // "2020-06-22"
    const char* results_0_daily_2_text_day = results_0_daily_2["text_day"]; // "中雨"
    const char* results_0_daily_2_code_day = results_0_daily_2["code_day"]; // "14"
    const char* results_0_daily_2_text_night = results_0_daily_2["text_night"]; // "小雨"
    const char* results_0_daily_2_code_night = results_0_daily_2["code_night"]; // "13"
    const char* results_0_daily_2_high = results_0_daily_2["high"]; // "26"
    const char* results_0_daily_2_low = results_0_daily_2["low"]; // "23"
    const char* results_0_daily_2_rainfall = results_0_daily_2["rainfall"]; // "10.0"
    const char* results_0_daily_2_precip = results_0_daily_2["precip"]; // ""
    const char* results_0_daily_2_wind_direction = results_0_daily_2["wind_direction"]; // "东南"
    const char* results_0_daily_2_wind_direction_degree = results_0_daily_2["wind_direction_degree"]; // "127"
    const char* results_0_daily_2_wind_speed = results_0_daily_2["wind_speed"]; // "25.20"
    const char* results_0_daily_2_wind_scale = results_0_daily_2["wind_scale"]; // "4"
    const char* results_0_daily_2_humidity = results_0_daily_2["humidity"]; // "71"
    
    const char* results_0_last_update = results_0["last_update"];  //数据更新时间
    
    String date0 = results_0_daily_0_date;  //将日期取出处理
    String date1 = results_0_daily_1_date;  
    String date2 = results_0_daily_2_date; 
    
    day0.date_m = date0.substring(5, 7);  //日期字符串切片
    day0.date_d = date0.substring(8, 10);
    day0.code_day = atoi(results_0_daily_0_code_day);//获取今天天气信息
    day0.high = atoi(results_0_daily_0_high);  //最高温度
    day0.low = atoi(results_0_daily_0_low);  //最低温度
    day0.wind_direction_degree = atoi(results_0_daily_0_wind_direction_degree);  //风向
    day0.wind_scale = atoi(results_0_daily_0_wind_scale);  //风力等级
    day0.humidity = atoi(results_0_daily_0_humidity);  //湿度
    
    day1.date_m = date1.substring(5, 7);
    day1.date_d = date1.substring(8, 10);
    day1.code_day = atoi(results_0_daily_1_code_day);//获取明天天气信息
    day1.high = atoi(results_0_daily_1_high);
    day1.low = atoi(results_0_daily_1_low);
    day1.wind_direction_degree = atoi(results_0_daily_1_wind_direction_degree);
    day1.wind_scale = atoi(results_0_daily_1_wind_scale);
    day1.humidity = atoi(results_0_daily_1_humidity);

    day2.date_m = date2.substring(5, 7);
    day2.date_d = date2.substring(8, 10);
    day2.code_day = atoi(results_0_daily_2_code_day);//获取后天天气信息
    day2.high = atoi(results_0_daily_2_high);
    day2.low = atoi(results_0_daily_2_low);
    day2.wind_direction_degree = atoi(results_0_daily_2_wind_direction_degree);
    day2.wind_scale = atoi(results_0_daily_2_wind_scale);
    day2.humidity = atoi(results_0_daily_2_humidity);

    return true;
}

void display_today()  //oled显示今天的天气信息
{   
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_lubB08_tr); 
    u8g2.setCursor(34, 15);
    u8g2.print(day0.low);
    u8g2.setCursor(49, 15);
    u8g2.print("-");
    u8g2.setCursor(54, 15);
    u8g2.print(day0.high);
    u8g2.setCursor(68, 15);
    u8g2.print("C");

    u8g2.drawLine(87, 5, 87, 31);

    u8g2.setCursor(94, 15);
    u8g2.print(day0.humidity);
    u8g2.setCursor(110, 15);
    u8g2.print("%");
    //u8g2.setCursor(80, 31);
    //u8g2.print(day0.wind_direction_degree);
    
    String Direction;  //将风向角度简化为风向文字信息
    if (day0.wind_direction_degree == 0)
    {
        Direction = "N";
    }
    else if (day0.wind_direction_degree == 90)
    {
        Direction = "E";
    }
    else if (day0.wind_direction_degree == 180)
    {
        Direction = "S";
    }
    else if (day0.wind_direction_degree == 270)
    {
        Direction = "W";
    }
    else if ((day0.wind_direction_degree>0) && (day0.wind_direction_degree<90))
    {
        Direction = "EN";
    }
    else if ((day0.wind_direction_degree>90) && (day0.wind_direction_degree<180))
    {
        Direction = "ES";
    }
    else if ((day0.wind_direction_degree>180) && (day0.wind_direction_degree<270))
    {
        Direction = "WS";
    }
    else if ((day0.wind_direction_degree>270) && (day0.wind_direction_degree<360))
    {
        Direction = "WN";
    }
    u8g2.setCursor(94, 31);
    u8g2.print(Direction);

    u8g2.setFont(u8g2_font_tenstamps_mn);
    u8g2.setCursor(110, 31);
    u8g2.print(day0.wind_scale);

    
    switch(day0.code_day)  //将天气代码转化为天气图标和字符显示
    {
        case 0:
            u8g2.drawXBMP( 1 , 1 , 30 , 30 , Sunny );
            u8g2.setFont(u8g2_font_luBIS08_tr); 
            u8g2.drawStr(33,30,"Sunny");
            break;
        case 4:
            u8g2.drawXBMP( 1 , 1 , 30 , 30 , Cloudy );
            u8g2.setFont(u8g2_font_luBIS08_tr); 
            u8g2.drawStr(33,30,"Cloudy");
            break;
        case 9:
            u8g2.drawXBMP( 1 , 1 , 30 , 30 , Overcast );
            u8g2.setFont(u8g2_font_luBIS08_tr); 
            u8g2.drawStr(33,30,"Overcast");
            break;
        case 10:
            u8g2.drawXBMP( 1 , 1 , 30 , 30 , Shower );
            u8g2.setFont(u8g2_font_luBIS08_tr); 
            u8g2.drawStr(33,30,"Shower");
            break;
        case 11:
            u8g2.drawXBMP( 1 , 1 , 30 , 30 , Thundershower );
            u8g2.setFont(u8g2_font_luBIS08_tr); 
            u8g2.drawStr(33,30,"Thunder");
            break;
        case 13:
            u8g2.drawXBMP( 1 , 1 , 30 , 30 , Rain_L );
            u8g2.setFont(u8g2_font_luBIS08_tr); 
            u8g2.drawStr(33,30,"L-Rain");
            break;
        case 14:
            u8g2.drawXBMP( 1 , 1 , 30 , 30 , Rain_M );
            u8g2.setFont(u8g2_font_luBIS08_tr); 
            u8g2.drawStr(33,30,"M-Rain");
            break;
        case 15:
            u8g2.drawXBMP( 1 , 1 , 30 , 30 , Rain_H );
            u8g2.setFont(u8g2_font_luBIS08_tr); 
            u8g2.drawStr(33,30,"H-Rain");
            break;
        case 30:
            u8g2.drawXBMP( 1 , 1 , 30 , 30 , Foggy );
            u8g2.setFont(u8g2_font_luBIS08_tr); 
            u8g2.drawStr(33,30,"Foggy");
            break;
        case 31:
            u8g2.drawXBMP( 1 , 1 , 30 , 30 , Haze );
            u8g2.setFont(u8g2_font_luBIS08_tr); 
            u8g2.drawStr(33,30,"Haze");
            break;    
    }
    
    u8g2.sendBuffer();
    Serial.println(day0.code_day);
}

void weather_station0()  //获取解析json，显示今天天气
{   
    if (getJson_ws()){
        if (parseJson_ws(response_ws)){
            display_today();
        }
    
    }
}

void weather_station1()  //并列显示三天日期和最高/低温度信息
{
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_lubB08_tr); 
    u8g2.setCursor(7, 15);
    u8g2.print(day0.date_m);
    u8g2.setCursor(21, 15);
    u8g2.print("/");
    u8g2.setCursor(29, 15);
    u8g2.print(day0.date_d);
    
    u8g2.setCursor(7, 31);
    u8g2.print(day0.low);
    u8g2.setCursor(22, 31);
    u8g2.print("-");
    u8g2.setCursor(27, 31);
    u8g2.print(day0.high);

    u8g2.drawLine(45, 5, 45, 31);
    
    u8g2.setCursor(49, 15);
    u8g2.print(day1.date_m);
    u8g2.setCursor(63, 15);
    u8g2.print("/");
    u8g2.setCursor(71, 15);
    u8g2.print(day1.date_d);

    u8g2.setCursor(49, 31);
    u8g2.print(day1.low);
    u8g2.setCursor(64, 31);
    u8g2.print("-");
    u8g2.setCursor(69, 31);
    u8g2.print(day1.high);

    u8g2.drawLine(87, 5, 87, 31);

    u8g2.setCursor(91, 15);
    u8g2.print(day2.date_m);
    u8g2.setCursor(105, 15);
    u8g2.print("/");
    u8g2.setCursor(113, 15);
    u8g2.print(day2.date_d);
    
    u8g2.setCursor(91, 31);
    u8g2.print(day2.low);
    u8g2.setCursor(106, 31);
    u8g2.print("-");
    u8g2.setCursor(111, 31);
    u8g2.print(day2.high);  
      
    u8g2.sendBuffer();
}

void weather_station2()  //并列显示三天天气图标
{   
    u8g2.clearBuffer();
    switch(day0.code_day)
    {
        case 0:
            u8g2.drawXBMP( 7 , 1 , 30 , 30 , Sunny  );
            break;
        case 4:
            u8g2.drawXBMP( 7 , 1 , 30 , 30 , Cloudy );
            break;
        case 9:
            u8g2.drawXBMP( 7 , 1 , 30 , 30 , Overcast );
            break;
        case 10:
            u8g2.drawXBMP( 7 , 1 , 30 , 30 , Shower );
            break;
        case 11:
            u8g2.drawXBMP( 7 , 1 , 30 , 30 , Thundershower );
            break;
        case 13:
            u8g2.drawXBMP( 7 , 1 , 30 , 30 , Rain_L );
            break;
        case 14:
            u8g2.drawXBMP( 7 , 1 , 30 , 30 , Rain_M );
            break;
        case 15:
            u8g2.drawXBMP( 7 , 1 , 30 , 30 , Rain_H );
            break;
        case 30:
            u8g2.drawXBMP( 7 , 1 , 30 , 30 , Foggy );
            break;
        case 31:
            u8g2.drawXBMP( 7 , 1 , 30 , 30 , Haze );
            break;
    }  
    switch(day1.code_day)
    {
        case 0:
            u8g2.drawXBMP( 49 , 1 , 30 , 30 , Sunny  );
            break;
        case 4:
            u8g2.drawXBMP( 49 , 1 , 30 , 30 , Cloudy );
            break;
        case 9:
            u8g2.drawXBMP( 49 , 1 , 30 , 30 , Overcast );
            break;
        case 10:
            u8g2.drawXBMP( 49 , 1 , 30 , 30 , Shower );
            break;
        case 11:
            u8g2.drawXBMP( 49 , 1 , 30 , 30 , Thundershower );
            break;
        case 13:
            u8g2.drawXBMP( 49 , 1 , 30 , 30 , Rain_L );
            break;
        case 14:
            u8g2.drawXBMP( 49 , 1 , 30 , 30 , Rain_M );
            break;
        case 15:
            u8g2.drawXBMP( 49 , 1 , 30 , 30 , Rain_H );
            break;
        case 30:
            u8g2.drawXBMP( 49 , 1 , 30 , 30 , Foggy );
            break;
        case 31:
            u8g2.drawXBMP( 49 , 1 , 30 , 30 , Haze );
            break;
    }
    switch(day2.code_day)
    {
        case 0:
            u8g2.drawXBMP( 91 , 1 , 30 , 30 , Sunny  );
            break;
        case 4:
            u8g2.drawXBMP( 91 , 1 , 30 , 30 , Cloudy );
            break;
        case 9:
            u8g2.drawXBMP( 91 , 1 , 30 , 30 , Overcast );
            break;
        case 10:
            u8g2.drawXBMP( 91 , 1 , 30 , 30 , Shower );
            break;
        case 11:
            u8g2.drawXBMP( 91 , 1 , 30 , 30 , Thundershower );
            break;
        case 13:
            u8g2.drawXBMP( 91 , 1 , 30 , 30 , Rain_L );
            break;
        case 14:
            u8g2.drawXBMP( 91 , 1 , 30 , 30 , Rain_M );
            break;
        case 15:
            u8g2.drawXBMP( 91 , 1 , 30 , 30 , Rain_H );
            break;
        case 30:
            u8g2.drawXBMP( 91 , 1 , 30 , 30 , Foggy );
            break;
        case 31:
            u8g2.drawXBMP( 91 , 1 , 30 , 30 , Haze );
            break;
    }
    u8g2.sendBuffer();

}

//------------------------------------------------------------------
//-------------------------DHT11_SENSOR-----------------------------
//------------------------------------------------------------------
void dht11()  //DHT11传感器，精度不行已弃用
{
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    //float hif = dht.computeHeatIndex(f, h);
    // Compute heat index in Celsius (isFahreheit = false)
    //float hic = dht.computeHeatIndex(t, h, false);
    //Serial.println(h);
    //Serial.println(t);
    char Humidity[25];
    char Temperature[25];
    itoa(h, Humidity, 10);
    itoa(t, Temperature, 10);
    
          // clear the internal memory
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB12_tr); // choose a suitable font
    u8g2.drawStr(0,15,"Hum:");  // write something to the internal memory
    u8g2.drawStr(0,30,"Tem:");  // write something to the internal memory
    u8g2.setCursor(55, 15);
    u8g2.print(h);
    u8g2.setCursor(55, 30);
    u8g2.print(t);
    u8g2.drawStr(105,15,"%");  // write something to the internal memory
    u8g2.drawStr(105,30,"C");
    //u8g2.drawStr(30,15,Humidity+"%");  // write something to the internal memory
    //u8g2.drawStr(30,30,Temperature+"°C");  // write something to the internal memory
    u8g2.sendBuffer();
    delay(500);
}
//------------------------------------------------------------------
//---------------------------loop函数--------------------------------
//------------------------------------------------------------------
void loop()  //主循环，判断按键次数切换运行程序和显示
{
    int menu = Click%7;
    if (WiFi.status() == WL_CONNECTED){
        switch(menu){
            case 0:
                NTP_CLOCK();
                break;
            case 1:
                NTP_DAY();
                break;
            case 2:
                weather_station0();
                break;
            case 3:
                weather_station1();
                break;
            case 4:
                weather_station2();
                break;
            case 5:
                bilibili_fans_follower();
                break;
            case 6:
                bilibili_fans_viewer();
                break;                    
            case 7:
                dht11();
                break;
        }
        
        Serial.println(Click);
    }
    else{
        Serial.println("[WiFi] Waiting to reconnect...");  //未联网串口输出
    }
}
