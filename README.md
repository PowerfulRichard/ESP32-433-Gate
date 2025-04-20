# ESP32-433-Gate
> 用于[ESP32 433MHz 智能家居接收器](https://oshwhub.com/a1768800804/esp32radiogate)的软件部分
## 运行环境
* Arduino for ESP32
* 安装`MQTTPubSubClient`,`ArduinoJson`,`RCSwitch`,`StringSplitter`库
* 自定义发送数据可参考*示例指令*
## 配置文件

```cpp
const char* WIFI_SSID     = "WIFI";             //wifi名称
const char* WIFI_PASS     = "123";              //wifi密码
const char* WIFI_HOSTNAME = "ESP32C3_433Gate";  //设备名称

#define MQTT_HOST "192.168.2.2"                 //mqtt地址
#define MQTT_PORT 1883                          //mqtt端口
#define MQTT_USER "admin"                       //mqtt用户
#define MQTT_PASS "12345"                       //mqtt密码
```

## 发送说明：

| 字段 | 取值 | 用途 |
|---|---|---|
| `type` | 1 | 温度传感器（发布 JSON 数据） |
| `type` | 2 | 控制开关（只需 ON 或 OFF 指令） |
| `opt` | 9 | 注册设备（发布 `/config`） |
| `opt` | 0 | 发布传感器状态（JSON） |
| `opt` | 1 | 控制开关（直接字符串 ON 或 OFF） |

## 示例指令格式

#### 注册温度传感器
```text
1,9,{}
```

#### 发送温度值
```text
1,0,{"temp":25.3}
```

#### 注册开关
```text
2,9,{}
```

#### 开启开关
```text
2,1,ON
```

#### 关闭开关
```text
2,1,OFF
```
