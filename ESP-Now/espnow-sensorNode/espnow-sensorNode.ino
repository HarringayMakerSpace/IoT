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

 **** This skecth is the controller/sensor node ****

 Ant Elder
 License: Apache License v2
*/
#include <ESP8266WiFi.h>
extern "C" {
  #include <espnow.h>
}

// this is the MAC Address of the remote ESP which this ESP sends its data too
uint8_t remoteMac[] = {0x5E, 0xCF, 0x7F, 0x5, 0xFD, 0xF0};

#define WIFI_CHANNEL 1
#define SLEEP_TIME 15e6
#define SEND_TIMEOUT 10000

// keep in sync with slave struct
struct SENSOR_DATA {
    float temp;
    float hum;
    float t;
    float pressure;
};

volatile boolean readingSent;

void setup() {
  Serial.begin(115200); Serial.println();

  WiFi.mode(WIFI_STA); // Station mode for sensor/controller node
  Serial.print("This node mac: "); Serial.println(WiFi.macAddress());

  if (esp_now_init()!=0) {
    Serial.println("*** ESP_Now init failed");
    ESP.restart();
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(remoteMac, ESP_NOW_ROLE_SLAVE, WIFI_CHANNEL, NULL, 0);

  esp_now_register_send_cb([](uint8_t* mac, uint8_t status) {
    Serial.print("send_cb, status = "); Serial.print(status); 
    Serial.print(", to mac: "); 
    char macString[50] = {0};
    sprintf(macString,"%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.println(macString);

    readingSent = true;
  });

  sendReading();
}

void loop() {
  if (readingSent || (millis() > SEND_TIMEOUT)) {
    Serial.print("Going to sleep, uptime: "); Serial.println(millis());  
    ESP.deepSleep(SLEEP_TIME, WAKE_RF_DEFAULT);
  }
}

void sendReading() {
    readingSent = false;
    SENSOR_DATA sd;
    sd.t = millis();
    Serial.print("sendReading, t="); Serial.println(sd.t);

    u8 bs[sizeof(sd)];
    memcpy(bs, &sd, sizeof(sd));
    esp_now_send(NULL, bs, sizeof(bs)); // NULL means send to all peers
}

