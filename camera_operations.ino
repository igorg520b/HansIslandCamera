void TriggerShutterIfNeeded()
{
    // see how much time elapsed since last exchange session
  unsigned long elapsed = millis() - lastShutter;

  // trigger when the time comes
  if(elapsed >= shutterInterval) {
    TriggerShutterNow();
  } else {
    unsigned long timeUntilTrigger = shutterInterval - elapsed;
    // if short period of time remains before camera trigger, then just wait
    if(timeUntilTrigger < 200) {
      delay(timeUntilTrigger);
      TriggerShutterNow();
    }
  }
}

void TriggerShutterNow()
{
  lastShutter = millis();
  photoCounter++;
#ifdef DEBUG
  Serial.print("triggering shutter ");
  Serial.print(modemError);
  Serial.print(" ");
  Serial.println(photoCounter);
#endif
  digitalWrite(SHUTTER_TRIGGER_PIN, LOW);
  delay(200);
  digitalWrite(SHUTTER_TRIGGER_PIN, HIGH);
}
