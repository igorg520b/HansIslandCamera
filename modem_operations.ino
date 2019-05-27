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
    Serial.print("Error: modem.begin() returned ");
    Serial.println(status);
    #endif
    return;
  } else modemError = false;

  bool doneCommunicating = false; // done when there are no incoming messages
  do
  {
    // create outbound message
#ifdef DEBUG
    Serial.print("invoking snprintf; sizeof(message)=");
    Serial.println(sizeof(message));
#endif
    snprintf(message, sizeof(message), "%lu %lu %lu", shutterInterval, callHomeInterval, photoCounter);

    // first send/receive
    size_t rxBufferSize = sizeof(rxbuffer);
#ifdef DEBUG
    Serial.print("rxBufferSize = ");
    Serial.println(rxBufferSize);
    status = modem.getSignalQuality(signalQuality);
    if(status != ISBD_SUCCESS)
    {
      Serial.println("Error: signal quality could not be obtained");
    }
    else
    {
      Serial.print("signal quality ");
      Serial.println(signalQuality);
    }
    Serial.println("invoking sendReceiveSBDText");
#endif

    status = modem.sendReceiveSBDText(message, rxbuffer, rxBufferSize);
    if (status != ISBD_SUCCESS)
    {
      modemError = true;
#ifdef DEBUG
      Serial.print("Error: sendReceiveSBDText returned ");
      Serial.println(status);
#endif
      return;
    }
#ifdef DEBUG
    else Serial.println("communication succeeded");
#endif

    if(rxBufferSize)
    {
#ifdef DEBUG
          Serial.print("incoming message; rxBufferSize=");
          Serial.println(rxBufferSize);
#endif
      // we have an inbound message
      // check if there are newer messages
      while(modem.getWaitingMessageCount() > 0) {

#ifdef DEBUG
      Serial.print("waiting message count = ");
      Serial.println(modem.getWaitingMessageCount());
#endif
        // discard messages except for the newest one
        rxBufferSize = sizeof(rxbuffer);
#ifdef DEBUG
        Serial.print("rxBufferSize (inside while) = ");
        Serial.println(rxBufferSize);
#endif
        status = modem.sendReceiveSBDText(NULL, rxbuffer, rxBufferSize);
        if (status != ISBD_SUCCESS)
        {
          modemError = true;
#ifdef DEBUG
          Serial.print("Error: sendReceiveSBDText (inside while) ");
          Serial.println(status);
#endif
          return;
        }
      }
      if(!rxBufferSize)
      {
         // not supposed to happen
#ifdef DEBUG
        Serial.println("Error: rxBufferSize != 0 after pulling all messages");
#endif
        modemError = true;
        return;
      }
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
#ifdef DEBUG
  Serial.print("parsing incoming message:");
  Serial.println(msg);
#endif
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
