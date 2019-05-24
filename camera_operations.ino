

void TriggerShutterIfNeeded() 
{
    // see how much time elapsed since last exchange session
  unsigned long now = millis();
  unsigned long interval = now - lastShutter;
  unsigned long shutterIntervalMilliseconds = shutterInterval * 1000;

  // communicate when the time comes
  if(interval >= shutterIntervalMilliseconds) {
    TriggerShutterNow();
  }
}

void TriggerShutterNow() 
{
  lastShutter = millis();
  digitalWrite(SHUTTER_TRIGGER, LOW); 
  delay(500);
  digitalWrite(SHUTTER_TRIGGER, HIGH);
  photoCounter++;
}
