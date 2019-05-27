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
  Serial.print("*");
  if(photoCounter % 100==0)
  {
    Serial.print("\npictures taken: ");
    Serial.println(photoCounter);
  }
#endif
  digitalWrite(SHUTTER_TRIGGER_PIN, LOW);
  delay(200); // this delay is specific to DigiSnap contorller
  digitalWrite(SHUTTER_TRIGGER_PIN, HIGH);
}
