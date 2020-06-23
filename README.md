# Esp8266_monitor
NTP time+bilibili_fans monitor+weather station

 * 项目：ESP8266_monitor 
 * 硬件：适用于NodeMCU ESP8266 + ssd1306 128*32 
 * 功能：连接WiFi后获取指定用户的哔哩哔哩实时粉丝数+WIFI网络授时时钟+知心天气 
 * 作者：Nemo_ssc  bilibili UID:12772522  
 * 图像：BMP文件夹存放所有用到的位图文件，可用pctolcd2002取模 
 * 字模选项：阴码 逐行式 逆向 十六进制数 自定义格式 段前缀{ 段后缀} 数据前缀0x 数据后缀, 行后缀, 
 * 日期：2020/06/20
 
   
需要用到的部分网址：

1.ArduinoJson部分

https://arduinojson.org/v6/assistant/

https://blog.csdn.net/dpjcn1990/article/details/92831612

https://blog.csdn.net/dpjcn1990/article/details/92831648

2.u8g2部分

https://blog.csdn.net/dpjcn1990/article/details/92831760

3.心知天气

https://www.seniverse.com/


需要用到的材料清单

1. NodeMcu D1 Mini

2. 0.91' OLED 12832

3. CD42 5V2A充放电一体模块

4. microusb 母座

5. 300mah 3.7v锂电池

6. 30#硅胶线若干、ABS板、铜柱



感谢以下B站UP提供的代码支持：

1. 会飞的阿卡林 UID751219 BV1x441187QF BV1eJ411H7LN

https://github.com/flyAkari/bilibili_fans_monitor

https://github.com/flyAkari/ESP8266_Network_Clock

2. Hans叫泽涵 UID15481541 BV1LE411n7XR

https://github.com/HansCallZH/bilibili_fans_clock_ESP8266

3. 小年轻只爱她 UID389851522 BV1N7411g7Ps BV157411L7Xf

https://gitee.com/young_people_only_love_her/My_ESP8266/tree/master/%E5%BF%83%E7%9F%A5%E5
