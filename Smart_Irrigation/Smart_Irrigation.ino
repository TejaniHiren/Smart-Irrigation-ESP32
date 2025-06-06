
#include <WiFi.h>
#include <EEPROM.h>
#include <Ticker.h>

#define RELAY_PIN           12      
#define BUTTON_PIN          14

#define TIMER_INTERVAL_SEC  1       // 1-second interval
#define SAVE_INTERVAL       300     // 5 minutes = 300 seconds
#define ON_DURATION         30      // Relay ON time in seconds
#define TRIGGER_INTERVAL    86400   // 24 hours = 86400 seconds

Ticker secondTicker;

volatile unsigned long secondsCounter = 0;
bool triggerActive = false;
unsigned long relayStartTime = 0;
unsigned long lastSaveTime = 0;
unsigned long localCounter;

const int EEPROM_SIZE = 512;        // Define EEPROM size
const int ADDRESS = 0;              // Starting EEPROM address

const unsigned long threshold = 3000;
unsigned long highStartTime = 0;
bool isTiming = false;
bool actionTaken = false;
int pinState;

void tickEverySecond() {
  secondsCounter++;
  // Serial.print("Tick: ");
  // Serial.println(secondsCounter);
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);        // Initialize EEPROM
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  digitalWrite(RELAY_PIN, LOW);  // Relay off

  EEPROM.get(ADDRESS, secondsCounter);

  Serial.print("Restored seconds: ");
  Serial.println(secondsCounter);
  if(secondsCounter == (unsigned long)0xFFFFFFFF)
  {
      secondsCounter = 0;
      EEPROM.put(ADDRESS, secondsCounter); 
      EEPROM.commit();  // IMPORTANT: Required on ESP8266 to save changes
  }
  Serial.println("Last ON time: " + String(secondsCounter));

  // Setup timer interrupt
  secondTicker.attach(1, tickEverySecond); // Call every 1 second
}

void loop() {
  pinState = digitalRead(BUTTON_PIN);
  if (pinState == HIGH) {
    if (!isTiming) 
    {
      // Start timing when HIGH is first detected
      highStartTime = millis();
      isTiming = true;
    }
    if (millis() - highStartTime >= threshold && !actionTaken) 
    {
      Serial.println("Pin HIGH for 5 seconds. Taking action...");
      digitalWrite(RELAY_PIN, HIGH);
      delay(1000);
      digitalWrite(RELAY_PIN, LOW);
      secondsCounter = 0;
      EEPROM.put(ADDRESS, secondsCounter); 
      EEPROM.commit();
      actionTaken = true;  // Prevent repeating the action
    }

  } else {
    // Reset if pin goes LOW before threshold is reached
    isTiming = false;
    actionTaken = false;
  }


  localCounter = secondsCounter;

  // Save to EEPROM every 5 minutes
  if (localCounter - lastSaveTime >= SAVE_INTERVAL) 
  {
    lastSaveTime = localCounter;

    EEPROM.put(ADDRESS, localCounter); 
    EEPROM.commit();
    Serial.print("Saved seconds: ");
    Serial.println(localCounter);
  }

  // Trigger GPIO12 every 24 hours for 30 sec
  if (localCounter >= TRIGGER_INTERVAL && !triggerActive) {
    Serial.println("Trigger ON");
    digitalWrite(RELAY_PIN, HIGH);
    triggerActive = true;
    relayStartTime = millis();
  }

  // Turn off after 30 seconds
  if (triggerActive && millis() - relayStartTime >= ON_DURATION * 1000) {
    Serial.println("Trigger OFF");
    Serial.print("Time at Trigger OFF");
    Serial.println(localCounter);
    digitalWrite(RELAY_PIN, LOW);

    // Reset counter
    secondsCounter = 0;

    EEPROM.put(ADDRESS, secondsCounter); 
    EEPROM.commit();

    triggerActive = false;
    lastSaveTime = 0;
  }
  delay(100);  // Small delay to avoid CPU hog
}
