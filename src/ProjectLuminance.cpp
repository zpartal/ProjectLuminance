#include "application.h" // Defines things such as D6, A1, HIGH/LOW, and OUTPUT/INPUT. Needed for .cpp files.
#include "RCSwitch.h"    // https://github.com/suda/RCSwitch

// Publishing lightlevels to particle cloud
bool publishingEnabled = true;

// Debug enabled
bool debug = true;

// 433 Mhz Transmitter Data Pin
int txDataPin = D6;

// Photoresistor Read Pin
int photoresistorPin = A1;

// Photoresistor "power" pin, used to guarantee a steady voltage to the photoresisotr for accuracy
int powerPin = A5;

// Photoresistor value average over last minute
int lightLevel = 0UL;

// Publish lightLevel interval (minutes)
int publishInterval = 10;

// Publish lightLevel array of size publishInterval
int publishLightLevels[10];

// Times
unsigned long prevSecTime = 0UL;
unsigned long prevMinTime = 0UL;
unsigned long prevPublishTime = 0UL;

// Create switch object
RCSwitch mySwitch = RCSwitch();
int pulseLength = 184; // mySwitch.getReceivedDelay()

// Functions for controlling a switch being on and off
int switchOn(String command);
int switchOff(String command);

void setup() {
  // Pin Setup
  pinMode(photoresistorPin,INPUT);
  pinMode(powerPin,OUTPUT);
  digitalWrite(powerPin,HIGH); // Turn on power pin, surprisingly important

  // Initialize lightLevel
  lightLevel = analogRead(photoresistorPin);

  // Initialize times
  prevPublishTime = prevMinTime = prevSecTime = millis();

  // Initialize publishLightLevels array
  for (int i = 0; i < publishInterval; ++i) publishLightLevels[i] = lightLevel;

  // Serial Setup (for debug)
  Serial.begin(9600);

  // RCSwitch library setup
  mySwitch.enableTransmit(txDataPin);
  mySwitch.setPulseLength(pulseLength);

  // Register web endpoints
  Spark.function("on", switchOn);                     // Register 'POST: /{device}/on' function
  Spark.function("off", switchOff);                   // Register 'POST: /{device}/off' function
  Spark.variable("lightLevel", &lightLevel, INT);     // Register 'GET: /{device}/lightLevel' variable
}

void loop() {
  unsigned long currTime = millis();

  // Every Second
  if (currTime-prevSecTime > 1000UL)
  {
    prevSecTime = currTime;

    // Read photoresistor every second and update lightLevel average
    int reading = analogRead(photoresistorPin);

    // Calculate approximate average over the last 64 seconds
    lightLevel -= (lightLevel >> 7); // lightlevel/64
    lightLevel += (reading >> 7);

    // DEBUG
    // Serial.println("Real Reading: " + String(reading) + "\tAverage Reading: " + String(lightLevel));
  }

  // LightLevel publishing
  if (publishingEnabled) {
    // Every minute
    if (currTime-prevMinTime > 60000UL) {
      prevMinTime = currTime;

      // Update index so it is circular
      int publishLightLevelIndex = currTime % publishInterval;

      // Put lightLevel in array every minute
      publishLightLevels[publishLightLevelIndex] = lightLevel;
    }

    // Every publish interval
    if(currTime-prevPublishTime > (publishInterval * 60000UL)) {
      prevPublishTime = currTime;

      // Calulate average
      int publishLightLevelTotal = 0;
      for (int i = 0; i < publishInterval; ++i) { publishLightLevelTotal += publishLightLevels[i]; }
      int publishLightLevelAvg = publishLightLevelTotal / publishInterval;

      // Publish lightLevel at regular intervals
      Spark.publish("lightLevel",String(publishLightLevelAvg));

      // DEBUG
      //Serial.println("Publishing: " + String(publishLightLevelAvg));
    }
  }
}

// switch_num on/off
// 1:349491/349500
// 2:349635/349644
// 3:349955/349964
// 4:351491/351500
// 5:357635/357644

int switchOn(String command) {
  if (command == "1")
    mySwitch.send(349491, 24);
  else if (command == "2")
    mySwitch.send(349635, 24);
  else { return -1; }
  return 1;
}

int switchOff(String command) {
  if (command == "1") {
    mySwitch.send(349500, 24);
  }
  else if (command == "2") {
    mySwitch.send(349644, 24);
  }
  else { return -1; }
  return 1;
}
