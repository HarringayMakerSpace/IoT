/*
 Testing ESP-Now 
 See https://espressif.com/en/products/software/esp-now/overview

 ESP-Now enables fast lightwieght connectionless communication between ESP8266's.

 So for example a remote sensor node could send a reading using ESP-Now in just a few 
 hundred milliseconds and deepSleep the rest of the time and so get increased battery 
 life compared to the node using a regular WiFi connection which could take 10 times 
 as long to connect to WiFi and send a sensor reading.
 
 ESP-Now has the concept of controllers and slaves. AFAICT the controller is the remote
 sensor node and the slave is the always on "gaeway" node that listens for sensor node readings. 

 **** This skecth is the slave/gateway node ****

 Ant Elder
 License: Apache License v2
*/
#include <ESP8266WiFi.h>
extern "C" {
  #include <espnow.h>
}

const char* ssid = "BTHub5-72W5";
const char* password = "46d38ec753";

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

  esp_now_set_self_role(ESP_NOW_ROLE_MAX);

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

  WiFi.mode(WIFI_AP);
  WiFi.softAP("MyGateway", "12345678", WIFI_CHANNEL, 1);
  Serial.print("This node AP mac: "); Serial.println(WiFi.softAPmacAddress());

// WIFI_AP_STA mode doesn;t seem to work and misses most ESP-Now messages
// See See http://bbs.espressif.com/viewtopic.php?f=7&t=2514&p=8285
return; 

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
//     goToSleep();
  }

  Serial.print("WiFi connected, IP address: "); Serial.println(WiFi.localIP());
}

