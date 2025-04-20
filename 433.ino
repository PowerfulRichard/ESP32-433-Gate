#include <Arduino.h>
#include <WiFi.h>
#include <MQTTPubSubClient.h>
#include <ArduinoJson.h>
#include <RCSwitch.h>
#include "StringSplitter.h"

RCSwitch SYN470 = RCSwitch();//定义接收端
unsigned long i = 0; //注意，这里的数据类型，不能用int

const char* WIFI_SSID     = "WIFI";             //wifi名称
const char* WIFI_PASS     = "123";              //wifi密码
const char* WIFI_HOSTNAME = "ESP32C3_433Gate";  //设备名称

#define MQTT_HOST "192.168.2.2"                 //mqtt地址
#define MQTT_PORT 1883                          //mqtt端口
#define MQTT_USER "admin"                       //mqtt用户
#define MQTT_PASS "12345"                       //mqtt密码

#define SENSOR_NAME "temperature"                    //传感器名称


WiFiClient client;
MQTTPubSubClient mqtt;

void init_WIFI() {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(WIFI_HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASS, 0, NULL, true);
    WiFi.setAutoReconnect(true);
}

void connectToWifi(){
  Serial.println("[开始连接WIFI]");
  WiFi.disconnect();
  WiFi.reconnect();
  int t=0;
  while (WiFi.status() != WL_CONNECTED) {
      t++;
      Serial.print(".");
      delay(1000);
      if(t==60){
        Serial.print(WiFi.status());
        Serial.println("\n!!WIFI连接超时!!");
        break;
      }
  }
  if(WiFi.isConnected()){
    Serial.print("WIFI连接成功!IP为");
    Serial.println(WiFi.localIP());
  }
}

void connectToHost() {
    Serial.println("[连接MQTT服务器]");
    client.stop();   //断开以前连接的MQTT服务器
    int t=0;
    while (!client.connect(MQTT_HOST, MQTT_PORT)) {
      t++;
      Serial.print(".");
      delay(1000); // 可以考虑改为非阻塞方式
      if(t==60){
        Serial.println("\n!!MQTT服务器连接超时!!");
        break;
      }
    }
    Serial.println("MQTT服务器已连接!");
}

void connectToMQTT() {
    Serial.print("[连接MQTT]");
    mqtt.disconnect();
    while (!mqtt.connect("esp", MQTT_USER, MQTT_PASS)) {
        Serial.print(".");
        delay(1000); // 可以考虑改为非阻塞方式
        if (WiFi.status() != WL_CONNECTED) {
          Serial.print(WiFi.status());
          Serial.println("\n!!WiFi 未连接!!");
          connectToWifi();
        }
        else{
          if (client.connected() != 1) {
              Serial.println("\n!!WiFiClient 未连接!!");
              connectToHost(); // 重新连接主机
          }
        }
    }
    Serial.println("MQTT客户端已连接!");
}
//注册mqtt消息
void reg(int type) {
  StaticJsonDocument<512> doc;
  char str[MAX_OUTPUT_SIZE];

  switch (type) {
    case 1: { // 温度传感器
      doc["name"] = "temperature_sensor";
      doc["state_topic"] = "homeassistant/sensor/temperature_sensor/state";
      doc["device_class"] = "temperature";
      doc["unit_of_measurement"] = "°C";
      doc["unique_id"] = "temperature_sensor_001";
      doc["value_template"] = "{{ value_json.temp }}";
      doc["state_class"] = "measurement";
      doc["force_update"] = true;

      serializeJson(doc, str);
      mqtt.publish("homeassistant/sensor/temperature_sensor/config", str);
      break;
    }

    case 2: { // cuco v3 switch（合并 set 和 state）
      doc["name"] = "cuco v3 switch";
      doc["command_topic"] = "homeassistant/switch/cuco_v3_0c8a_switch/set";
      doc["state_topic"] = "homeassistant/switch/cuco_v3_0c8a_switch/set"; // 直接复用 set 作为 state
      doc["unique_id"] = "cuco_v3_0c8a_switch";
      doc["payload_on"] = "ON";
      doc["payload_off"] = "OFF";

      serializeJson(doc, str);
      mqtt.publish("homeassistant/switch/cuco_v3_0c8a_switch/config", str);
      break;
    }

    default:
      Serial.println("未知设备类型");
      break;
  }
}

// 处理 MQTT 指令
bool mqtt_msg(String m) {
  mqtt.loop();
  if (!mqtt.connected()) {
    connectToMQTT();
  }

  // 拆分指令（可为2段或3段）
  StringSplitter splitter(m, ',', 3);
  String typeStr = splitter.getItemAtIndex(0);
  String optStr = splitter.getItemAtIndex(1);
  String payloadStr = splitter.getItemAtIndex(2); // 可能是 JSON 或 "ON"/"OFF"

  int type = typeStr.toInt();
  int opt = optStr.toInt();

  if (opt == 9) {
    reg(type);
    return true;
  }

  switch (type) {
    case 1: { // 温度传感器（只能 opt=0）
      if (opt != 0) return false;
      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, payloadStr);
      if (error) {
        Serial.print("温度 JSON 解析失败: ");
        Serial.println(error.c_str());
        return false;
      }
      char str[MAX_OUTPUT_SIZE];
      serializeJson(doc, str);
      mqtt.publish("homeassistant/sensor/temperature_sensor/state", str);
      break;
    }
    case 2: { // 开关控制（opt=1 发送 ON/OFF）
      if (opt != 1) return false;
      // payloadStr 直接是 "ON" 或 "OFF"
      String topic = "homeassistant/switch/cuco_v3_0c8a_switch/set";
      mqtt.publish(topic.c_str(), payloadStr.c_str());
      break;
    }
    default:
      Serial.println("未知类型");
      return false;
  }
  return true;
}

void setup() {
  Serial.begin(115200);     // 初始化串口通信，波特率设置为115200
  mySwitch.enableReceive(0);
  init_WIFI();              // 初始化wifi
  mqtt.begin(client);
  connectToMQTT();
}

void loop() {
  // 读取数据
  if (mySwitch.available()) {
    output(mySwitch.getReceivedValue(), mySwitch.getReceivedBitlength(), mySwitch.getReceivedDelay(), mySwitch.getReceivedRawdata(),mySwitch.getReceivedProtocol());
    mySwitch.resetAvailable();
  }
  mqtt_msg()
  delay(300000);  // 等待5分钟再次读取
}
