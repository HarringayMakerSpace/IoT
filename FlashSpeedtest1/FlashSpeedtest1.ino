/*
 * Flash reading speed test
 *
 * Times how long to read data from flash. Upload with different settings -   
 * flash mode (DIO or QIO), flash speed (40MHz or 80 MHz) and CPU speed (80MHz or 160MHz)
 * to see if each mode makes much difference.
 * 
 * The Harringay Maker Space
 * License: Apache License v2
 */
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <FS.h>

char buff[32684];

void setup() {
   Serial.begin(115200); Serial.println();
   WiFi.mode(WIFI_OFF);  
   SPIFFS.begin();  
}

void loop() {
  Serial.print("read bytes...");
  File f = SPIFFS.open("/news.mp3", "r");
  if (!f) {
     Serial.println("file open failed");
     ESP.restart();
  }

  unsigned long startMs = millis();
  int x = f.readBytes(buff, sizeof(buff));
  int x1 = f.readBytes(buff, sizeof(buff));
  int x2 = f.readBytes(buff, sizeof(buff));
  int x3 = f.readBytes(buff, sizeof(buff));
  int x4 = f.readBytes(buff, sizeof(buff));
  unsigned long stopMs = millis();
  Serial.print("done: ");  Serial.println(stopMs - startMs);

  f.close();
  delay(3000);
}
