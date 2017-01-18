/* 
 *  Bootstrap OTA for ESP8266 
 *  
 *  Starts up using WPS to auto configure Wifi network, if WPS unsuccessful then try WifiManager.
 *  Once connected to Wifi it runs Over-The-Air (OTA) to enable remote program uploads to add further function. 
 *  
 *  For WPS, press the WPS button on the WiFi Access Point and then within 2 minutes power on the ESP8266 running 
 *  this sketch and it should automaticaly set the SSID and password. 
 *  
 *  Author: Anthony Elder
 *  License: Apache License v2 
 *  https://github.com/HarringayMakerSpace/IoT/
 */
#include <ESP8266WiFi.h>

// these for WiFiManager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          

// these for OTA upport
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h> 

#include <Ticker.h>

#define LED_PIN 2 // gpio2 for ESP-12, gpio1 for ESP-01, gpio12 for Sonoff

const String THIS_NAME = String("ESP-") + ESP.getChipId();

int heartBeatInterval = 15000; // 15 seconds
Ticker ledBlinker;


void setup(void) {
    Serial.begin(115200); Serial.println();
    Serial.print(THIS_NAME); Serial.print(__DATE__);Serial.println(__TIME__);

    pinMode(LED_PIN, OUTPUT);
    ledBlinker.attach(0.2, ledBlink); // start with fast blink to show Wifi connecting

    initWifi();
    Serial.print("WiFi connected to "); Serial.print(WiFi.SSID()); Serial.print(", IP address: "); Serial.println(WiFi.localIP());

    ledBlinker.detach();
    digitalWrite(LED_PIN, LOW);

    initOTA();
}

unsigned long lastHeartBeat;

void loop() {
    ArduinoOTA.handle();

    if (millis() - lastHeartBeat > heartBeatInterval) {
      Serial.println("loop running");
      digitalWrite(LED_PIN, LOW);
      delay(50);
      digitalWrite(LED_PIN, HIGH);
      lastHeartBeat = millis();
    }
}

void initWifi() {
  WiFi.mode(WIFI_STA);
  wifi_station_set_auto_connect(true);  
  
  // wait 10 seconds to see if auto connect to previous network
  Serial.println("Connecting to previous Wifi network...");
  unsigned long startMs = millis();
  while (millis() - startMs < 10000) {
    if (WiFi.status() == WL_CONNECTED) {
       return;
    }
    delay(1);
  }

  Serial.println("Trying WPS connection...");
  bool wpsSuccess = WiFi.beginWPSConfig();
  if(wpsSuccess && WiFi.SSID().length() > 0) {
      // wait for connect...
      int startMs = millis();
      while (millis() - startMs < 10000) {
        if (WiFi.status() == WL_CONNECTED) {
          return;
        }
        delay(1);
      } 
  }

  // still no connection so try WifiManager
  ledBlinker.attach(0.8, ledBlink); // slow blink led to show in Wifi AP mode
  WiFiManager wifiManager;
  wifiManager.setTimeout(240);
  if(!wifiManager.autoConnect(THIS_NAME.c_str())) {
    Serial.println("*** failed to connect to Wifi. Restarting...");
    // restart and try connecting again (WiFi AP might have been down - power cut, etc)
    ESP.reset();
  } 
}

void initOTA() {
  ArduinoOTA.setHostname(THIS_NAME.c_str());

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void ledBlink() {
  digitalWrite(LED_PIN, ! digitalRead(LED_PIN));
}

