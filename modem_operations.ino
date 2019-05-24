unsigned int processedMessages = 0; // number of processed messages before response is sent
int lastMessageParseResult = 0;     // result code for the last message, equals to the number of reset parameters 

void CallHomeIfNeeded() 
{
  // see how much time elapsed since last exchange session
  unsigned long now = millis();
  unsigned long interval = now - lastCallHome;
  unsigned long callHomeIntervalMilliseconds = callHomeInterval * 60000;

  // communicate when the time comes
  if(interval >= callHomeIntervalMilliseconds) {
    CallHomeNow();
  }
}

void CallHomeNow() 
{
    lastCallHome = millis();
  // exchange messages

  // if successful:
}

void parseMessage(char *msg) {
#ifdef TRACE
  Serial.print("parsing message: ");
  Serial.println(msg);
#endif // TRACE

  lastMessageParseResult = 0;
  unsigned long tmp = atoi(msg);
  if(!tmp) return;
  
  shutterInterval = tmp;
  lastMessageParseResult = 1;
 
  char *firstOccurrence = strchr(msg,(int)' ');
  if(firstOccurrence) {
    firstOccurrence++;
    tmp = atoi(firstOccurrence);

    if(tmp >= MIN_CALL_HOME_INTERVAL && tmp <= MAX_CALL_HOME_INTERVAL) {
      callHomeInterval = tmp;
      lastMessageParseResult = 2;
    }
  }
}
