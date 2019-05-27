void CallHomeIfNeeded()
{
  // time elapsed since last call home
  unsigned long elapsed = millis() - lastCallHome;
  // if it's time, call home
  if(elapsed >= callHomeInterval) CallHomeNow();
}

uint8_t rxbuffer[50]; // incoming message
char message[50];     // outgoing message

void CallHomeNow()
{
  // wake up modem
    #ifdef DEBUG
    Serial.println("waking up modem");
    #endif
  int status = modem.begin();
  if (status != ISBD_SUCCESS)
  {
    modemError = true;
    #ifdef DEBUG
    Serial.print("modem.begin() returned ");
    Serial.println(status);
    #endif
    return;
  } else modemError = false;

    #ifdef DEBUG
  status = modem.getSignalQuality(signalQuality);
  if(status != ISBD_SUCCESS) {
    Serial.println("signal quality could not be obtained");
  }
    #endif

  bool doneCommunicating = false; // done when there are no incoming messages
  do
  {
    // create outbound message
    snprintf(message, sizeof(message), "%lu %lu %lu", shutterInterval, callHomeInterval, photoCounter);

    // first send/receive
    size_t rxBufferSize = sizeof(rxbuffer);
    #ifdef DEBUG
  int status = modem.getSignalQuality(signalQuality);
  if(status != ISBD_SUCCESS) {
    Serial.println("signal quality could not be obtained");
  } else {
    Serial.print("signal quality ");
    Serial.println(signalQuality);
    }
    Serial.println("sendReceiveSBDText");
    #endif
    status = modem.sendReceiveSBDText(message, rxbuffer, rxBufferSize);
    if (status != ISBD_SUCCESS) 
    { 
      modemError = true;
    #ifdef DEBUG
    Serial.print("sendReceiveSBDText returned ");
    Serial.println(status);
    #endif
             
      return; 
    } else 
    {
    #ifdef DEBUG
    Serial.println("communication succeeded");
    #endif
    }

    if(rxBufferSize)
    {
      // we have an inbound message
      // check if there are newer messages
      while(modem.getWaitingMessageCount() > 0) {
        // discard messages except for the newest one
        rxBufferSize = sizeof(rxbuffer);
        status = modem.sendReceiveSBDText(NULL, rxbuffer, rxBufferSize);
        if (status != ISBD_SUCCESS) { modemError = true; return; }
      }
      if(!rxBufferSize) { modemError = true; return; } // not supposed to happen
      // process message
      ParseIncomingMessage((char*)rxbuffer);
    } else doneCommunicating = true;

  } while(!doneCommunicating);
  // done
    #ifdef DEBUG
    Serial.println("modem sleep");
    #endif
  modem.sleep();
  lastCallHome = millis();
}


void ParseIncomingMessage(char *msg) {
  unsigned long tmp = (unsigned long)atol(msg);
  if(!tmp) return;

  shutterInterval = tmp;

  char *firstOccurrence = strchr(msg,(int)' ');
  if(firstOccurrence) {
    firstOccurrence++;
    tmp = (unsigned long)atol(firstOccurrence);

    if(tmp >= MIN_CALL_HOME_INTERVAL && tmp <= MAX_CALL_HOME_INTERVAL)
      callHomeInterval = tmp;
  }
}
