/*
 Testing ESP-Now 
 See https://espressif.com/en/products/software/esp-now/overview

 ESP-Now enables fast lightwieght connectionless communication between ESP8266's.

 So for example a remote sensor node could send a reading using ESP-Now in just a few 
 hundred milliseconds and deepSleep the rest of the time and so get increased battery 
 life compared to the node using a regular WiFi connection which could take 10 times 
 as long to connect to WiFi and send a sensor reading.
 
 ESP-Now has the concept of controllers and slaves. AFAICT the controller is the remote
 sensor node and the slave is the always on "gateway" node that listens for sensor node readings. 

 *** Note: to compile ESP-Now (with arduino/esp8266 release 2.3.0) need to edit 
 * ~/Library/Arduino15/packages/esp8266/hardware/esp8266/2.1.0/platform.txt 
 * Search "compiler.c.elf.libs", and append "-lespnow" at the end of the line. 
 * See: http://www.esp8266.com/viewtopic.php?p=44161#p44161 
 ***

 **** This skecth is the slave/gateway node ****

 Ant Elder
 License: Apache License v2
*/
#include <ESP8266WiFi.h>
extern "C" {
  #include <espnow.h>
}

const char* ssid = "BTHub5-72W5";
const char* password = "xxxxxxxxxx";

#define WIFI_CHANNEL 1

// keep in sync with sensor struct
struct SENSOR_DATA {
    float temp;
    float hum;
    float t;
    float pressure;
};

void setup() {
  Serial.begin(115200); Serial.println();

  initWifi();

  if (esp_now_init()!=0) {
    Serial.println("*** ESP_Now init failed");
    ESP.restart();
  }

  Serial.print("This node AP mac: "); Serial.print(WiFi.softAPmacAddress());
  Serial.print(", STA mac: "); Serial.println(WiFi.macAddress());

  // Note: When ESP8266 is in soft-AP+station mode, this will communicate through station interface
  // if it is in slave role, and communicate through soft-AP interface if it is in controller role,
  // so you need to make sure the remote nodes use the correct MAC address being used by this gateway. 
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);

  esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {
    Serial.print("recv_cb, from mac: ");
    char macString[50] = {0};
    sprintf(macString, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print(macString);

    getReading(data, len);
  });
}

void loop() {
}

void getReading(uint8_t *data, uint8_t len) {
  SENSOR_DATA tmp;
  memcpy(&tmp, data, sizeof(tmp));

  Serial.print(", parsed data, t="); Serial.println(tmp.t);
}

void initWifi() {

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("MyGateway", "12345678", WIFI_CHANNEL, 1);

  Serial.print("Connecting to "); Serial.print(ssid);
  if (strcmp (WiFi.SSID().c_str(), ssid) != 0) {
      WiFi.begin(ssid, password);
  }
 
  int retries = 20; // 10 seconds
  while ((WiFi.status() != WL_CONNECTED) && (retries-- > 0)) {
     delay(500);
     Serial.print(".");
  }
  Serial.println("");
  if (retries < 1) {
     Serial.print("*** WiFi connection failed");  
     ESP.restart();
  }

  Serial.print("WiFi connected, IP address: "); Serial.println(WiFi.localIP());
}

