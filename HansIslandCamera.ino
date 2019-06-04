// Libraries
#include <DS3232RTC.h>
#include "IridiumSBD.h"

// All time intervals are in seconds
#define ROCKBLOCK_SLEEP_PIN   5                 // Modem sleep pin
#define SHUTTER_TRIGGER_PIN   6                 // Shutter trigger pin
#define RTC_INT_PIN           12                // External RTC interrupt pin

// Intervals
#define MAX_CALL_HOME_INTERVAL (7UL*24UL*3600UL)  // One week
#define MIN_CALL_HOME_INTERVAL (15*60)            // 15 minutes
#define LENGTH_OF_DEEP_SLEEP (11)                 // Upper bound of deep sleep duration

// Object instantiations
IridiumSBD modem(Serial1, ROCKBLOCK_SLEEP_PIN);
DS3232RTC myRTC(false); 

// Global variable delcarations
bool            intervalChanged             = true;           // If received a command to change the interval
bool            modemError                  = false;          // Indicates that the modem's library returned error
unsigned long   shutterInterval             = 24UL*3600UL;    // Once a day
unsigned long   callHomeInterval            = 120*60;          // Message exchange default interval
unsigned long   photoCounter                = 0;              // Count shutter triggers
time_t          initializeTime, alarmTime;                    // Initial time at reset (does not change) and time of next alarm
time_t          nowTime;
time_t          lastCallHome, lastLEDSignal;

void setup() 
{
  Serial1.begin(19200);                     // RockBLOCK serial communications *** Can this be added to modem_operations.ino with a Serial1.end()? ***

  pinMode(LED_BUILTIN, OUTPUT);             // ***LED_BUILTIN is the default way to control the LED on pin 13***
  pinMode(SHUTTER_TRIGGER_PIN, OUTPUT);     // Camera shutter
  pinMode(RTC_INT_PIN, INPUT_PULLUP);       // External RTC alarm signal

  digitalWrite(LED_BUILTIN, LOW);           // Disable LED 
  digitalWrite(SHUTTER_TRIGGER_PIN, HIGH);  // Set trigger to its rest state

  // Set up the external RTC
  myRTC.begin();
  myRTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);  // Initialize the alarms to known values
  myRTC.alarm(ALARM_1);                         // Clear the alarm flags
  myRTC.alarmInterrupt(ALARM_1, false);         // Clear the alarm interrupt flags ***If this is set to true you could have a false trigger before you set the alarm***
  myRTC.squareWave(SQWAVE_NONE);                // Ensure RTC alarm is not set to SQW
  lastLEDSignal = initializeTime = myRTC.get();
  
  // Watchdog and sleep timer
  initializeWDT();
  enableWatchdog();

  // Modem
  modem.setPowerProfile(IridiumSBD::USB_POWER_PROFILE);
  CallHomeNow();
}

void loop() 
{
  resetWD();              // Reset watchdog
  nowTime = myRTC.get();  // Get current time from external RTC

  // Trigger shutter if (1) alarm goes off or (2) missed the alarm or (3) interval changed
  if (!digitalRead(RTC_INT_PIN) || nowTime >= alarmTime || intervalChanged)
  {
    TriggerShutterNow();
    setNextAlarm();
  } else if((nowTime + LENGTH_OF_DEEP_SLEEP) < alarmTime) {
    sleepWell();  // Deep sleep ~10 seconds
    resetWD();    // Reset watchdog
  } 

  // Call home if needed
  if(nowTime >= (lastCallHome + callHomeInterval)) CallHomeNow();

  // Indicate current status
  SignalLED(false);
}

void setNextAlarm() 
{
  myRTC.alarm(ALARM_1); // Ensure alarm 1 interrupt flag is cleared

  // Tentative time for the next alarm
  alarmTime += shutterInterval; 

  // If the interval was changed or we already missed the next alarm
  if(intervalChanged || alarmTime <= myRTC.get()) 
  {
    // Start with a new reference point
    alarmTime = myRTC.get() + shutterInterval;
    intervalChanged = false; // the chage was just processed
  }
  myRTC.setAlarm(ALM1_MATCH_DATE, second(alarmTime), minute(alarmTime), hour(alarmTime), day(alarmTime)); // Set alarm
}

bool ISBDCallback() 
{
  resetWD(); // Reset watchdog

  // Trigger shutter if needed
  nowTime = myRTC.get(); // Get current time from external RTC

  // Trigger shutter if (1) alarm goes off or (2) missed the alarm or (3) interval changed
  if (!digitalRead(RTC_INT_PIN) || nowTime >= alarmTime || intervalChanged)
  {
    TriggerShutterNow();
    setNextAlarm();
  }
   
  SignalLED(true); // Signal that the modem is operating (rapid blinking)
  return true; // This callback must return true for modem to continue
}

void TriggerShutterNow()
{
  photoCounter++;
  digitalWrite(SHUTTER_TRIGGER_PIN, LOW);
  delay(200); // This delay is specific to DigiSnap contorller
  digitalWrite(SHUTTER_TRIGGER_PIN, HIGH);
}

void SignalLED(bool modemActive) {
  // Blinking interval ~10 seconds  - Normal/sleep
  // Interval ~1 second             - Active modem
  // Steady off                     - Modem error or past one hour since start
  unsigned long blinkInterval = 10;
  unsigned long turnOffAfter = 3600;

  if(nowTime < (initializeTime + turnOffAfter) && 
  (nowTime > (lastLEDSignal + blinkInterval) || 
  (modemActive && nowTime > (lastLEDSignal + 1)))) 
  {
    if(!modemError) blink();
    lastLEDSignal = nowTime;
  } 
}

void blink() 
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  pinMode(LED_BUILTIN, INPUT); // ***Set pin to INPUT when not in use***
  resetWD();  // Reset watchdog
}
