#include <Adafruit_SleepyDog.h>
#include <IridiumSBD.h>

// all time intervals are in milliseconds for consistency
#define DEBUG
#define SHUTTER_TRIGGER_PIN 6                  // shutter trigger pin
#define ROCKBLOCK_SLEEP_PIN 5                  // model sleep pin
#define LED_PIN 13
#define MAX_CALL_HOME_INTERVAL (48*60*60000)
#define MIN_CALL_HOME_INTERVAL (15*60000)
#define WATCHDOG_TIMEOUT 30000 // 16-bit signed int

IridiumSBD modem(Serial1, ROCKBLOCK_SLEEP_PIN);

unsigned long shutterInterval = 10000;
unsigned long callHomeInterval = 1200000;
unsigned long lastShutter = 0;             // in milliseconds
unsigned long lastCallHome = 0;            // in milliseconds
unsigned long photoCounter = 0;            // count shutter triggers
bool modemError = false;                   // indicates that the modem is not working as expected
int signalQuality = -1;

void setup() {
//  Watchdog.enable(WATCHDOG_TIMEOUT);
#ifdef DEBUG  
  delay(3000);
#endif


  Serial1.begin(19200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SHUTTER_TRIGGER_PIN, OUTPUT);
  modem.setPowerProfile(IridiumSBD::USB_POWER_PROFILE);

  TriggerShutterNow();
  CallHomeNow();
}

void loop() {
//  Watchdog.reset();
  TriggerShutterIfNeeded();
  CallHomeIfNeeded();
  BlinkLED();
}

bool ISBDCallback() {
//  Watchdog.reset();
  TriggerShutterIfNeeded();
  BlinkLED();
  return true;
}

void BlinkLED() {
  if(modemError) digitalWrite(LED_PIN, (millis()/100 % 2) ? HIGH : LOW); // fast blink
  else digitalWrite(LED_PIN, (millis()/1000 % 2) ? HIGH : LOW); // slow blink
}
