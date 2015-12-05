#include "application.h" // Defines things such as D6, A1, HIGH/LOW, and OUTPUT/INPUT. Needed for .cpp files.
#include "RCSwitch.h"    // https://github.com/suda/RCSwitch
#include "plotly_particle.h"

#define NUM_TRACES 1
char *streaming_tokens[NUM_TRACES] = {"l5natiwog5"};
plotly graph = plotly("zpartal", "j6ejuqezsz", streaming_tokens, "ProjectLuminance", NUM_TRACES);

// Publishing lightlevels to particle cloud
bool publishingEnabled = true;

// Debug enabled
bool debug = true;

// 433 Mhz Transmitter Data Pin
const int txDataPin = D6;

// Photoresistor Read Pin
const int photoresistorPin = A1;

// Photoresistor "power" pin, used to guarantee a steady voltage to the photoresisotr for accuracy
const int powerPin = A5;

// Number of wireless switches
const int numSwitches = 5;

// On/Off codes
const int onCodes[5]  = { 349491, 349635, 349955, 351491, 357635 };
const int offCodes[5] = { 349500, 349644, 349964, 351500, 357644 };

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
const int pulseLength = 184; // mySwitch.getReceivedDelay()

// Functions for controlling a switch being on and off
// command can either be a single swich, or a list of up to numSwitches switches seperated by commas
int switchOn(String command);
int switchOff(String command);

// Switch implementation
int doSwitch(String& command, const int* codeList);

int argumentList[5];
int tokenzieArguments(String& arguments);

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
  Serial.begin(115200);

  // RCSwitch library setup
  mySwitch.enableTransmit(txDataPin);
  mySwitch.setPulseLength(pulseLength);

  // Register web endpoints
  Particle.function("on", switchOn);                     // Register 'POST: /{device}/on' function
  Particle.function("off", switchOff);                   // Register 'POST: /{device}/off' function
  Particle.variable("lightLevel", &lightLevel, INT);     // Register 'GET: /{device}/lightLevel' variable

  while (!WiFi.ready()) {};
  Serial.println("Wifi Ready...");

  // Plotly
  graph.world_readable = false;
  graph.timezone = "US/Central";
  graph.fileopt = "extend"; // Remove this if you want the graph to be overwritten on initialization
  // graph.log_level = 0;

  graph.init();
  graph.openStream();
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

    // Serial.println("Light Level: " + String(lightLevel));
  }

  // LightLevel publishing
  if (publishingEnabled) {
    // Every minute
    if (currTime-prevMinTime > 60000UL) {
      prevMinTime = currTime;

      // Update index so it is circles around the array
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
      Particle.publish("lightLevel",String(publishLightLevelAvg));
      graph.plot(millis(), lightLevel, streaming_tokens[0]);
    }
  }
}

int switchOn(String command) {
  return doSwitch(command, onCodes);
}

int switchOff(String command) {
  return doSwitch(command, offCodes);
}

// Implementation of switch command
int doSwitch(String& command, const int* codeList) {
  if(tokenzieArguments(command)) {
    // Loop through argmueent list switching every switch that is not -1 or greater than numSwitches
    for (int i = 0; i < numSwitches; ++i) {
      if (argumentList[i] > 0 && argumentList[i] <= numSwitches) {
        mySwitch.send(codeList[argumentList[i] - 1],24);
        delay(100);
        Serial.println(argumentList[i]);
      }
    }
    return 1;
  }
  return 0;
}

int tokenzieArguments(String& arguments) {
  if (arguments.length()) {
    // Reset argumentList
    for (int i = 0; i < numSwitches; ++i) argumentList[i] = -1;

    // Tokenize list of arguments
    if(arguments.indexOf(",") != -1) {
      int numToks = 0;
      char inputStr[64];
      arguments.toCharArray(inputStr,64);
      char *p;
      p = strtok(inputStr,",");
      while (p != NULL && numToks < (numSwitches - 1)) {
        argumentList[numToks] = atoi(p);
        p = strtok (NULL, ",");
        numToks++;
      }
    }

    // Single argument case
    else {
      argumentList[0] = arguments.toInt();
    }
    return 1;
  }
  return 0;
}
