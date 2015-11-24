/**
 * ESP8266 daily task library.
 * 
 * The Harringay Maker Space
 * License: Apache License v2
 **/

#ifndef _ESP_DAILY_TASK_H
#define _ESP_DAILY_TASK_H

#include <ESP8266WiFi.h>

class ESPDailyTask {

  public:
    ESPDailyTask(int wakeUpMinutes);

    void sleep1Day();  
    void backToSleep();  
    void timeAdjustFromDateHeader(WiFiClient client);

  private:
    void backToSleep(boolean wifiOn);
    void printRtcMem();
    void processCurrentTime(int time);
};

#endif // _ESP_DAILY_TASK_H

