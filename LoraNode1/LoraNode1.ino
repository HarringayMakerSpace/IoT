/* ESP8266 with RFM95W/SX1276 Example
 *  
 * Uses deepSleep and publishes Vcc every 30 seconds.
 *
 * Tested using:
 *  - ESP8266/Arduino 2.2.0
 *  - Arduino LMIC 1.5.0+arduino-1
 *  - Arduino IDE 1.6.8 
 *
 * Wiring connections:
 * ESP8266  - SX1276
 *  GND        GND
 *  VCC        VCC
 *  GPIO15     NSS
 *  GPIO13     MOSI
 *  GPIO12     MIS0
 *  GPIO14     SCK
 *  GPIO5      DIO0
 *  GPIO4      DIO1
 *
 * Also on the ESP connect GPIO16-RST for deepSleep wakeup
 *  
 * Spare pins for other things:
 *  GPIO0 - must be high to boot
 *  GPIO1 - Serial TX, will output some garbage at bootup 
 *  GPIO2 - must be high to boot
 *  GPIO3 - Serial RX
 *  ADC
 *  On an ESP12e also:
 *  GPIO10 - Can be used if sketch flashed with DIO mode
 *  GPIO9 - Can be used if you have a DIO ESP such as a "real" NodeMCU 1.0
 *  
 * Author: Ant Elder
 * License: Apache License v2
 */
#include <ESP8266WiFi.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <EEPROM.h>

ADC_MODE(ADC_VCC); // enables reading the ESP8266 supply voltage

#define SLEEP_SECONDS 30

const lmic_pinmap lmic_pins = {
    .nss = 15,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LMIC_UNUSED_PIN,
    .dio = {5, 4, LMIC_UNUSED_PIN},
};

// Create these with ttnctl or the TTN Dashboard at https://staging.thethingsnetwork.org/
// LoRaWAN end-device address
static const u4_t DEVADDR = 0x762C6511;
// LoRaWAN NwkSKey, network session key
static const PROGMEM u1_t NWKSKEY[16] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 };
// LoRaWAN AppSKey, application session key
static const u1_t PROGMEM APPSKEY[16] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0xD1, 0x11, 0x11, 0x11, 0x11, 0x11 };
                                                                                                                  
volatile boolean dio0Triggered;

// The LORAWAN session state is saved in EEPROM
// Here just the frame sequence number is saved as its ABP with a Single Channel Gateway,
// See http://forum.thethingsnetwork.org/t/new-backend-how-to-connect/1983/91 
#define INITED_FLAG 0x12344321 // <<<< Change this to have the frame sequence restart from zero  
typedef struct {
  uint32 inited;
  u4_t seqnoUp;
} LORA_STATE;
LORA_STATE loraState;

void setup() {
  Serial.begin(115200); Serial.println();

  EEPROM.begin(sizeof(loraState));
  EEPROM.get(0, loraState);
  if (loraState.inited != INITED_FLAG) {
     loraState.inited = INITED_FLAG;
     loraState.seqnoUp = 0;
  }

  lmicInit();
  // the EV_TXCOMPLETE event takes ages to happen, but DIO0 high indicates TX complete 
  // See https://github.com/matthijskooijman/arduino-lmic/issues/16
  attachInterrupt(digitalPinToInterrupt(lmic_pins.dio[0]), dio0ISR, RISING);

  LMIC.seqnoUp = loraState.seqnoUp;

  String vcc = String(ESP.getVcc()); // Needs ESP SDK 1.5.3, see https://github.com/esp8266/Arduino/issues/1961    
  do_send(vcc);
}


// loop runs until the TX Complete event puts the ESP to sleep
void loop() {
  os_runloop_once();
  if (dio0Triggered) goToSleep();
}

void dio0ISR() {
   dio0Triggered = true;  
}

void do_send(String msg) {
   Serial.print("Sending msg at "); Serial.print(millis());
   Serial.print(": "); Serial.print(msg); Serial.println();

    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
      Serial.println("OP_TXRXPEND, not sending");
      return;
    }

    Serial.print("Sending on channel: "); Serial.println(LMIC.txChnl);

    dio0Triggered = false;
    LMIC_setTxData2(1, (xref2u1_t)msg.c_str(), msg.length(), 0);
}

void onEvent (ev_t ev) {
    Serial.print("on event: "); Serial.println(ev);
    switch(ev) {
      case EV_TXCOMPLETE:
          Serial.print("Event EV_TXCOMPLETE at "); Serial.println(millis());
          if(LMIC.dataLen) { // has data been received in rx slot after tx
                Serial.print(F("Data Received: "));
                Serial.write(LMIC.frame+LMIC.dataBeg, LMIC.dataLen);
                Serial.println();
           }
          goToSleep();          
          break;
       default:
          break;
    }
}

void goToSleep() {

  loraState.seqnoUp = LMIC.seqnoUp;
  EEPROM.put(0, loraState);
  EEPROM.end();
  Serial.print("loraState.seqnoUp = "); Serial.println(loraState.seqnoUp);    

  Serial.print("Up time "); Serial.print(millis());
  Serial.print(", going to sleep for "); Serial.print(SLEEP_SECONDS); Serial.println(" secs");
  ESP.deepSleep(SLEEP_SECONDS*1000000, RF_DISABLED);  
}

void lmicInit() {
  Serial.print("LMIC Init started at "); Serial.print(millis()); Serial.print("...");
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Set static session parameters. Instead of dynamically establishing a session
    // by joining the network, precomputed session parameters are be provided.
    #ifdef PROGMEM
    // On AVR, these values are stored in flash and only copied to RAM
    // once. Copy them to a temporary buffer here, LMIC_setSession will
    // copy them into a buffer of its own again.
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
    #else
    // If not running an AVR with PROGMEM, just use the arrays directly 
    LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    // Set up the channels used by the Things Network, which corresponds
    // to the defaults of most gateways. Without this, only three base
    // channels from the LoRaWAN specification are used, which certainly
    // works, so it is good for debugging, but can overload those
    // frequencies, so be sure to configure the full frequency range of
    // your network here (unless your network autoconfigures them).
    // Setting up channels should happen after LMIC_setSession, as that
    // configures the minimal channel set.
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
    // TTN defines an additional channel at 869.525Mhz using SF9 for class B
    // devices' ping slots. LMIC does not have an easy way to define set this
    // frequency and support for class B is spotty and untested, so this
    // frequency is not configured here.

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // Set data rate and transmit power (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(DR_SF7, 20);

  // only use channel 0 for the single channel gateway
  LMIC_disableChannel(1);
  LMIC_disableChannel(2);
  LMIC_disableChannel(3);
  LMIC_disableChannel(4);
  LMIC_disableChannel(5);
  LMIC_disableChannel(6);
  LMIC_disableChannel(7);
  LMIC_disableChannel(8);

  Serial.print("done "); Serial.println(millis());
}

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }
