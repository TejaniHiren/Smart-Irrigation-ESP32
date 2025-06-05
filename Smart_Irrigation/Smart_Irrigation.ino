
#include <WiFi.h>
// #include <time.h>
#include <EEPROM.h>
#include <Ticker.h>

#define RELAY_PIN 12  // D4 on Lolin WEMOS Mini

#define TIMER_INTERVAL_SEC 1  // 1-second interval
#define SAVE_INTERVAL 300     // 5 minutes = 300 seconds
#define ON_DURATION 30        // Relay ON time in seconds
#define TRIGGER_INTERVAL 86400 // 24 hours = 86400 seconds

Ticker secondTicker;

volatile unsigned long secondsCounter = 0;
bool triggerActive = false;
unsigned long relayStartTime = 0;
unsigned long lastSaveTime = 0;
unsigned long localCounter;

const int EEPROM_SIZE = 512;        // Define EEPROM size
const int ADDRESS = 0;              // Starting EEPROM address

void tickEverySecond() {
  secondsCounter++;
  Serial.print("Tick: ");
  Serial.println(secondsCounter);
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);        // Initialize EEPROM
  pinMode(RELAY_PIN, OUTPUT);
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
  // timer = timerBegin(1000000);  // 80MHz / 80 = 1MHz (1 tick = 1us)
  // timerAttachInterrupt(timer, &onTimer);
  // timerAlarmEnable(timer);
}

void loop() {
  

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
