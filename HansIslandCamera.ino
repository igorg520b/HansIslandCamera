// Libraries
#include <DS3232RTC.h>
#include "IridiumSBD.h"

// All time intervals are in seconds
#define ROCKBLOCK_SLEEP_PIN   5                 // Modem sleep pin
#define SHUTTER_TRIGGER_PIN   6                 // Shutter trigger pin
#define RTC_INT_PIN           12                // External RTC interrupt pin

// Intervals
#define MAX_CALL_HOME_INTERVAL (7UL*24UL*3600UL)  // One week
#define MIN_CALL_HOME_INTERVAL (5*60)             // 5 minutes
#define LENGTH_OF_DEEP_SLEEP (11)                 // Upper bound of deep sleep duration

// Object instantiations
IridiumSBD modem(Serial1, ROCKBLOCK_SLEEP_PIN);
DS3232RTC myRTC(false); 

// Global variable delcarations
bool            intervalChanged             = true;           // If received a command to change the interval
bool            modemError                  = false;          // Indicates that the modem's library returned error
unsigned long   shutterInterval             = 0;              // Off by default
unsigned long   callHomeInterval            = 5UL*3600UL;     // Message exchange default interval
unsigned long   photoCounter                = 0;              // Count shutter triggers
time_t          initializeTime, alarmTime=0;                  // Initial time at reset (does not change) and time of next alarm
time_t          nowTime;
time_t          lastCallHome, lastLEDSignal;

void setup() 
{
  Serial1.begin(19200);                     // RockBLOCK serial communications *** Can this be added to modem_operations.ino with a Serial1.end()? ***

//  pinMode(LED_BUILTIN, OUTPUT);             // ***LED_BUILTIN is the default way to control the LED on pin 13***
  pinMode(SHUTTER_TRIGGER_PIN, OUTPUT);     // Camera shutter
  pinMode(RTC_INT_PIN, INPUT_PULLUP);       // External RTC alarm signal

//  digitalWrite(LED_BUILTIN, LOW);           // Disable LED 
  digitalWrite(SHUTTER_TRIGGER_PIN, HIGH);  // Set trigger to its rest state

  // Set up the external RTC
  myRTC.begin();
  // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
  myRTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
  myRTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
  myRTC.alarm(ALARM_1);
  myRTC.alarm(ALARM_2);
  myRTC.alarmInterrupt(ALARM_1, false);
  myRTC.alarmInterrupt(ALARM_2, false);
  myRTC.squareWave(SQWAVE_NONE);
  lastLEDSignal = initializeTime = myRTC.get();
  
  // Watchdog and sleep timer
  InitializeWDT();
  EnableWatchdog();

  TriggerShutterNow();

  // Modem
  modem.setPowerProfile(IridiumSBD::USB_POWER_PROFILE);
  CallHomeNow();
}

void loop() 
{
  ResetWD();              // Reset watchdog
  nowTime = myRTC.get();  // Get current time from external RTC

  // Trigger shutter if (1) alarm goes off or (2) missed the alarm or (3) interval changed
  if (shutterInterval > 0 && (!digitalRead(RTC_INT_PIN) || nowTime >= alarmTime || intervalChanged))
  {
    TriggerShutterNow();
    SetNextAlarm();
  } 
  else if(shutterInterval == 0 || (nowTime + LENGTH_OF_DEEP_SLEEP) < alarmTime) 
  {
    // Comment out the next line to disable deep sleep 
    SleepWell();  // Deep sleep ~10 seconds
    ResetWD();    // Reset watchdog
  } 

  // Call home if needed
  if(nowTime >= (lastCallHome + callHomeInterval)) CallHomeNow();

//  SignalLED(false);
  ResetWD();  // Reset watchdog
}

void SetNextAlarm() 
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
  myRTC.setAlarm(ALM1_MATCH_DAY, second(alarmTime), minute(alarmTime), hour(alarmTime), day(alarmTime)); // Set alarm
}

bool ISBDCallback() 
{
  ResetWD(); // Reset watchdog

  // Trigger shutter if needed
  nowTime = myRTC.get(); // Get current time from external RTC

  // Trigger shutter if (1) alarm goes off or (2) missed the alarm or (3) interval changed
  if (shutterInterval > 0 && (!digitalRead(RTC_INT_PIN) || nowTime >= alarmTime || intervalChanged))
  {
    TriggerShutterNow();
    SetNextAlarm();
  }
   
//  SignalLED(true); // Signal that the modem is operating (rapid blinking)
  ResetWD();  // Reset watchdog
  return true; // This callback must return true for modem to continue
}

void TriggerShutterNow()
{
  photoCounter++;
  digitalWrite(SHUTTER_TRIGGER_PIN, LOW);
//  digitalWrite(LED_BUILTIN, HIGH);
  delay(200); // This delay is specific to DigiSnap contorller
  digitalWrite(SHUTTER_TRIGGER_PIN, HIGH);
//  digitalWrite(LED_BUILTIN, LOW);
}

/*
void SignalLED(bool modemActive) 
{
  // Blinking interval ~10 seconds  - Normal/sleep
  // Interval ~1 second             - Active modem
  // Steady off                     - Modem error or past one hour since start
  unsigned long blinkInterval = 10;
  unsigned long turnOffAfter = 3600;

  if(nowTime < (initializeTime + turnOffAfter) && !modemError &&
  (nowTime > (lastLEDSignal + blinkInterval) || 
  (modemActive && nowTime > (lastLEDSignal + 1)))) 
  {
    Blink();
    lastLEDSignal = nowTime;
  } 
}
*/

void Blink() 
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  pinMode(LED_BUILTIN, INPUT); // ***Set pin to INPUT when not in use***
}
