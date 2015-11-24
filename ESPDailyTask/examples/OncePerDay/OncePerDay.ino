/*
 * This sketch shows an example of using the ESPDailyTask library
 * 
 * The Harringay Maker Space
 * License: Apache License v2
 * 
 */
#include <ESP8266WiFi.h>
#include <ESPDailyTask.h>

ESPDailyTask dailyTask(16*60); // Hour to do the task - 16 for 4pm

void setup() {
  dailyTask.sleep1Day();

  // put your daily code here...


  // and back to sleep once daily code is done  
  dailyTask.backToSleep();
}

void loop() {
  // sleeping so wont get here
}

