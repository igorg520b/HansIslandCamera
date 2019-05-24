#include <IridiumSBD.h>
#include <Adafruit_SleepyDog.h>

#define TRACE                          // verbose output to serial
#define SHUTTER_TRIGGER 6              // shutter trigger pin
#define ROCKBLOCK_SLEEP 5              // model sleep pin
#define LED 13
#define MAX_CALL_HOME_INTERVAL 2880    // in minutes; call home at least once in 48 hours
#define MIN_CALL_HOME_INTERVAL 15      // in minutes; 

IridiumSBD modem(Serial1);

unsigned long shutterInterval = 10;    // interval between taking photos in seconds
unsigned long callHomeInterval = 20;   // interval between modem data exchanges in minutes(!)
unsigned long lastShutter = 0;  // in milliseconds
unsigned long lastCallHome = 0;        // in milliseconds
unsigned long photoCounter = 0;        // count shutter triggers 


void setup() {
  pinMode(LED, OUTPUT);
  pinMode(SHUTTER_TRIGGER, OUTPUT);
  pinMode(ROCKBLOCK_SLEEP, OUTPUT);
  CallHomeNow();
  TriggerShutterNow();
}

void loop() {
  TriggerShutterIfNeeded();
  CallHomeIfNeeded();
}
