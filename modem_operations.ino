uint8_t rxbuffer[50]; // Incoming message buffer
char message[50];     // Outgoing message buffer

void CallHomeNow()
{
  int parseResult = 0;
  lastCallHome = myRTC.get();
  
  int status = modem.begin(); // Wake up modem
  if (status != ISBD_SUCCESS)
  {
    modemError = true;
    return;
  } 
  else modemError = false;

  // (1) Report the current parameters, number of photos and the RTC sensor temperature.
  // (2) There may be several incoming messages in the queue. Discard all but the most recent.
  // If there are no incoming messages, terminate.
  // (3) Parse the most recent message and go to step 1.
  bool doneCommunicating = false; // Done when there are no incoming messages
  do
  {
    // Create the outbound message
    if(parseResult == 0) snprintf(message, sizeof(message), "%lu %lu %lu %d", shutterInterval, callHomeInterval, photoCounter, myRTC.temperature()/4);
    else snprintf(message, sizeof(message), "ok");

    // Initial sending and receiving
    size_t rxBufferSize = sizeof(rxbuffer);
    memset((void*)rxbuffer,0, rxBufferSize); // Clear the received buffer 
    rxBufferSize--; // The last byte is reserved for zero to terminate the string
    status = modem.sendReceiveSBDText(message, rxbuffer, rxBufferSize);
    if (status != ISBD_SUCCESS)
    {
      modemError = true;
      return;
    }
    ResetWD();  // Reset watchdog
    
    if(rxBufferSize)
    {
      // There is an inbound message
      rxbuffer[rxBufferSize] = 0; // Terminate the received message with zero
      
      // Check if there are more messages in queue
      while(modem.getWaitingMessageCount() > 0) 
      {
        ResetWD();  // Reset watchdog

        // Discard all queued messages except the last one
        rxBufferSize = sizeof(rxbuffer);
        memset((void*)rxbuffer,0, rxBufferSize); 
        rxBufferSize--;
        status = modem.sendReceiveSBDText(NULL, rxbuffer, rxBufferSize);
        if (status != ISBD_SUCCESS)
        {
          modemError = true;
          return;
        }
        ResetWD();  // Reset watchdog
      }

      // At this point the last message should be in rxBuffer, and rxBufferSize should be >0
      if(!rxBufferSize)
      {
        // If the last message was not received, some sort of error occurred
        modemError = true;
        return;
      } 
      else 
      {
        rxbuffer[rxBufferSize] = 0; // Terminate the incoming message with zero
      }
      
      // Process message
      parseResult = ParseIncomingMessage((char*)rxbuffer);
    } else doneCommunicating = true; // There are no messages in the queue

  } while(!doneCommunicating);

  modem.sleep(); // Put modem to sleep
  lastCallHome = myRTC.get(); // Update the last time of message exchange
}

int ParseIncomingMessage(char *msg) 
{
  // Check if it's a 'go' message
  if(msg[0] == 'g' && msg[1] == 'o')
  {
    TriggerShutterNow();
    return 1; // indicate that a 'go' message is received
  }
  
  // Parse the message of the form "12345 12345" where the first number is the 
  // shutter interval and the second number is the call-home interval
  unsigned long tmp = (unsigned long)atol(msg); // Try to convert the first number

  // Updating the interval also causes the reset of the reference time
  // Update only if the new value is different from the current one
  if(shutterInterval != tmp) 
  { 
    shutterInterval = tmp;
    intervalChanged = true; // Instruct SetNextAlarm() to restart the countdown from the current moment
  }

  // Find the first space in the string
  // Important: if the incoming message starts with space, e.g. " 12345",
  // then the first nubmer will be used both for shutter interval and for call home interval
  char *firstOccurrence = strchr(msg,(int)' ');
  if(firstOccurrence) 
  {
    // If there is space in the string, try to parse the string that comes after that symbol
    firstOccurrence++;
    tmp = (unsigned long)atol(firstOccurrence);
    
    // Communication with home is a critical function
    // Short periods will drain the battery, and long periods will cut off communication
    // Update only if the received value is within a reasonable interval
    if(tmp >= MIN_CALL_HOME_INTERVAL && tmp <= MAX_CALL_HOME_INTERVAL) callHomeInterval = tmp;
  }
  return 0; // normal message is processed
}
