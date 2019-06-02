uint8_t rxbuffer[50]; // incoming message
char message[50];     // outgoing message

void CallHomeNow()
{
  // wake up modem
  int status = modem.begin();
  if (status != ISBD_SUCCESS)
  {
    modemError = true;
    return;
  } 
  else modemError = false;

  bool doneCommunicating = false; // done when there are no incoming messages
  do
  {
    // create outbound message
    snprintf(message, sizeof(message), "%lu %lu %lu %d", shutterInterval, callHomeInterval, photoCounter, myRTC.temperature()/4);

    // first send/receive
    size_t rxBufferSize = sizeof(rxbuffer);
    status = modem.sendReceiveSBDText(message, rxbuffer, rxBufferSize);
    if (status != ISBD_SUCCESS)
    {
      modemError = true;
      return;
    }

    if(rxBufferSize)
    {
      // we have an inbound message
      // check if there are newer messages
      while(modem.getWaitingMessageCount() > 0) 
      {
        // discard messages except for the newest one
        rxBufferSize = sizeof(rxbuffer);
        status = modem.sendReceiveSBDText(NULL, rxbuffer, rxBufferSize);
        if (status != ISBD_SUCCESS)
        {
          modemError = true;
          return;
        }
      }
      if(!rxBufferSize)
      {
         // not supposed to happen
        modemError = true;
        return;
      }
      // process message
      ParseIncomingMessage((char*)rxbuffer);
    } else doneCommunicating = true;

  } while(!doneCommunicating);
  // done
  modem.sleep();
  lastCallHome = myRTC.get();
}

void ParseIncomingMessage(char *msg) {
  unsigned long tmp = (unsigned long)atol(msg);
  if(!tmp) return; // atol failed
  if(shutterInterval != tmp) {
    shutterInterval = tmp;
    intervalChanged = true;
  }
  char *firstOccurrence = strchr(msg,(int)' ');
  if(firstOccurrence) {
    firstOccurrence++;
    tmp = (unsigned long)atol(firstOccurrence);
    if(tmp >= MIN_CALL_HOME_INTERVAL && tmp <= MAX_CALL_HOME_INTERVAL)
      callHomeInterval = tmp;
  }
}
