/**
 * ESP8266 daily task library.
 * 
 * Uses multiple deepSleeps with the WiFi radio off to sleep for one day,
 * waking up once every 24 hours with WiFi enabled to do some work.
 * Supports adjusting the sleep time based on an HTTP response Date header
 * to compensate for drify from the ESP clock inaccuracy.
 *
 * The ESP deepSleep time is an uint32 of microseconds so a maximum of 4,294,967,295, 
 * about 71 minutes per deepSleep. So to sleep for one day requires sleeping for one 
 * hour 24 times and keep track of where its up to in RTC memory. The WiFi radio is 
 * switched off for all but the 24th wakeup to save power.
 * 
 * The Harringay Maker Space
 * License: Apache License v2
 **/
#include "EspDailyTask.h"
extern "C" {
  #include "user_interface.h" // this is for the RTC memory read/write functions
}

const unsigned long ONE_SECOND = 1000 * 1000;   
const unsigned long ONE_HOUR = 60 * 60 * ONE_SECOND; 

boolean firstTime = false;
unsigned long thisSleepTime; 

typedef struct {
  byte markerFlag;
  byte counter;
  unsigned long sleepTime;
} rtcStore __attribute__((aligned(4))); // not sure if the aligned stuff is necessary, some posts suggest it is for RTC memory?

rtcStore rtcMem;

// Desired wake up time, in minutes, eg 4:35pm = (16 * 60) + 30
int wakeUpTime;

ESPDailyTask::ESPDailyTask(int mins){
   wakeUpTime = mins;
}

/**
 * The deepSleep time is an uint32 of microseconds so a maximum of 4,294,967,295, about 71 minutes. 
 * So to publish a reading once per day needs to sleep for one hour 24 times and keep track of  
 * where its up to in RTC memory. The WiFi radio is switched off for all but the 24th wakeup 
 * to save power
 */
void ESPDailyTask::sleep1Day() {
  system_rtc_mem_read(65, &rtcMem, sizeof(rtcMem));
  printRtcMem();

  // 126 in [0] is a marker to detect first use   
  if (rtcMem.markerFlag != 126) {
     rtcMem.markerFlag = 126;
     rtcMem.counter = 0;
     rtcMem.sleepTime = ONE_HOUR;
     firstTime = true;
  } else {
     rtcMem.counter += 1;
     firstTime = false;
  }
  if (rtcMem.counter > 24) {
     rtcMem.counter = 0;
  }

  thisSleepTime = rtcMem.sleepTime;
  
  if (rtcMem.counter == 0) {
    return; // a day is up, go do some work
  } 

  if (rtcMem.counter == 24) {
     backToSleep(true); // next wake up has WiFi on
  } else {
     backToSleep();
  }
}

void ESPDailyTask::backToSleep() {
  backToSleep(false);
}

void ESPDailyTask::backToSleep(boolean wifiOn) {
  printRtcMem();
  system_rtc_mem_write(65, &rtcMem, sizeof(rtcMem));
  Serial.print("*** Up time: ");
  Serial.print(millis());
  if (wifiOn) { 
     Serial.println(", waking up... ");
     ESP.deepSleep(1, WAKE_RF_DEFAULT);
  } else { 
     Serial.print(", deep sleeping for ");
     Serial.print(thisSleepTime);
     Serial.print(" microseconds with WiFi on ");
     Serial.print(wifiOn);
     Serial.println("...");
     ESP.deepSleep(thisSleepTime, WAKE_RF_DISABLED); 
  }
}

void ESPDailyTask::printRtcMem() {
  Serial.print("rtc marker: ");
  Serial.print(rtcMem.markerFlag);
  Serial.print(", counter: ");
  Serial.print(rtcMem.counter);
  Serial.print(", sleepTime: ");
  Serial.print(rtcMem.sleepTime);
  Serial.print(", thisSleepTime: ");
  Serial.print(thisSleepTime);
  Serial.print(", firstTime: ");
  Serial.print(firstTime);
  Serial.println();
}

/* The sleep time varies as the wakeup and connect time varies and the ESP clock is a little slow 
 *  so get the current time from an HTTP response Date Header - "Thu, 15 Oct 2015 08:57:03 GMT" 
 *  and adjust the sleep time based on the drift from desired wake up time.
 */
void ESPDailyTask::timeAdjustFromDateHeader(WiFiClient client) {
  while(client.available()){
    if (client.read() == '\n') {    
      if (client.read() == 'D') {    
        if (client.read() == 'a') {    
          if (client.read() == 't') {    
            if (client.read() == 'e') {    
              if (client.read() == ':') {    
                client.read();
                // skip over the date bit - "Thu, 15 Oct 2015"
                client.readStringUntil(' ');client.readStringUntil(' ');client.readStringUntil(' ');client.readStringUntil(' ');
                int hours = client.parseInt();
                int minutes = client.parseInt();
                if (hours == 0 && minutes == 0) return; // likely something went wrong
                Serial.print("Current time ");Serial.print(hours);Serial.print(":");Serial.println(minutes);
                int currentMins = (hours * 60) + minutes;
                processCurrentTime(currentMins);
                return;
              }
            }
          }
        }
      }
    }
  }
}

void ESPDailyTask::processCurrentTime(int currentMins) {
    if (firstTime) {
       if (currentMins > wakeUpTime) {
          // if its past the wakeUpTime need to sleep for less than 24 hours
          int tx = (24*60) - currentMins + wakeUpTime;
          thisSleepTime = (tx % 60) * 60000000;
          rtcMem.counter = tx / 60;
       } else {
          // if its ealier than the wakeUpTime need to sleep for less than 24 hours
          int tx = wakeUpTime - currentMins;
          thisSleepTime = (tx % 60) * 60000000;
          rtcMem.counter = 24 - (tx / 60) - 1;
       }
       Serial.print("First time: thisSleepTime ");Serial.print(thisSleepTime);Serial.print(", currentCounter: ");Serial.println(rtcMem.counter);
    } else {
      int drift = currentMins - wakeUpTime;
      rtcMem.sleepTime -= (drift * 60000000) / 24;
      thisSleepTime = rtcMem.sleepTime;
      Serial.print("Drift adjust: drift=");Serial.print(drift);Serial.print(", new sleepTime=");Serial.println(rtcMem.sleepTime);
    }    
}
