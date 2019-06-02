#include <DS3232RTC.h>
#include "IridiumSBD.h"

// all time intervals are in seconds
#define SHUTTER_TRIGGER_PIN 6                  // shutter trigger pin
#define ROCKBLOCK_SLEEP_PIN 5                  // model sleep pin
#define LED_PIN 13
#define RTC_INT_PIN   12

// intervals
#define MAX_CALL_HOME_INTERVAL (7*24*3600) // one week
#define MIN_CALL_HOME_INTERVAL (15*60)     // 15 minutes
#define LENGTH_OF_DEEP_SLEEP 11 // upper bound of deep sleep duration

IridiumSBD modem(Serial1, ROCKBLOCK_SLEEP_PIN);
DS3232RTC myRTC(false); 

unsigned long shutterInterval = 365*24*3600;   // once a year
unsigned long callHomeInterval = 120*60;       // once in two hours
bool intervalChanged = true;               // if received a command to change the interval
unsigned long photoCounter = 0;            // count shutter triggers
bool modemError = false;                   // indicates that the modem's library returned error
time_t initializeTime, alarmTime;     // initial time at reset (does not change) and time of next alarm
time_t nowTime;
time_t lastCallHome, lastLEDSignal;
int temperatureReading;                    // RTC thermometer reading

void setup() {
  Serial1.begin(19200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SHUTTER_TRIGGER_PIN, OUTPUT); // for camera shutter
  pinMode(RTC_INT_PIN, INPUT_PULLUP);   // external RTC alarm signal

  // set up RTC
  myRTC.begin();
  myRTC.alarmInterrupt(ALARM_1, true);
  lastLEDSignal = initializeTime = myRTC.get();
  
  // watchdog and sleep timer
  initializeWDT();
  enableWatchdog();

  // modem
  modem.setPowerProfile(IridiumSBD::USB_POWER_PROFILE);
  CallHomeNow();
}

void loop() {
  resetWD();  // reset watchdog
  nowTime = myRTC.get(); // get current time from external RTC

  // trigger shutter if (1) alarm goes off or (2) missed the alarm or (3) interval changed
  if (!digitalRead(RTC_INT_PIN) || nowTime >= alarmTime || intervalChanged)
  {
    TriggerShutterNow();
    setNextAlarm();
  } else if((nowTime + LENGTH_OF_DEEP_SLEEP) < alarmTime) {
    sleepWell(); // deep sleep ~10 seconds
  } 

  // call home if needed
  if(nowTime >= (lastCallHome + callHomeInterval)) CallHomeNow();

  // indicate current status
  SignalLED();
}

void setNextAlarm() {
  myRTC.alarm(ALARM_1);

  // tentative time for the next alarm
  alarmTime += shutterInterval; 

  // if the interval was changed or we already missed the next alarm
  if(intervalChanged || alarmTime <= myRTC.get()) {
    // start with a new reference point
    alarmTime = myRTC.get() + shutterInterval;
    intervalChanged = false; // the chage was just processed
  }
  myRTC.setAlarm(ALM1_MATCH_DATE, second(alarmTime), minute(alarmTime), hour(alarmTime), day(alarmTime)); // Set alarm
}

bool ISBDCallback() {
  resetWD(); // reset watchdog

  // trigger shutter if needed
  nowTime = myRTC.get(); // get current time from external RTC
  // trigger shutter if (1) alarm goes off or (2) missed the alarm or (3) interval changed
  if (!digitalRead(RTC_INT_PIN) || nowTime >= alarmTime || intervalChanged)
  {
    TriggerShutterNow();
    setNextAlarm();
  }
   
  SignalLED();
  return true;
}

void TriggerShutterNow()
{
  photoCounter++;
  digitalWrite(SHUTTER_TRIGGER_PIN, LOW);
  delay(200); // this delay is specific to DigiSnap contorller
  digitalWrite(SHUTTER_TRIGGER_PIN, HIGH);
}

void SignalLED() {
  // one blink per ~11 seconds - normal operation
  // two blinks - modem error or no reception
  // three blinks - RTC error
  unsigned long blinkInterval = 11;
  unsigned long turnOffAfter = 3600;

  if(nowTime < (initializeTime + turnOffAfter) && nowTime > (lastLEDSignal + blinkInterval)) {
    blink();
    if(modemError) blink();
    lastLEDSignal = nowTime;
  }
}

void blink() {
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  delay(150);
  digitalWrite(LED_PIN, LOW);
}
