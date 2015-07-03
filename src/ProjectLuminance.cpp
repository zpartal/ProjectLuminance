#include "application.h" // Defines things such as D6, A1, HIGH/LOW, and OUTPUT/INPUT. Needed for .cpp files.
#include "RCSwitch.h"    // https://github.com/suda/RCSwitch

// Debug enabled
bool debug = true;

// 433 Mhz Transmitter Data Pin
int txData = D6;

// Photoresistor Read Pin
int photoresistor = A1;

// Photoresistor "power" pin, used to guarantee a steady voltage to the photoresisotr for accuracy
int power = A5;

// Photoresistor Value
int lightLevel;

// Previous time
unsigned long prevTime = 0UL;

// Publish lightLevel interval (5 minutes in milliseconds)
int publishInterval = 300000;

// Create switch object
RCSwitch mySwitch = RCSwitch();
int pulseLength = 184; // mySwitch.getReceivedDelay()

// Functions for controlling a switch being on and off
int switchOn(String command);
int switchOff(String command);

void setup() {
    // Pin Setup
    pinMode(photoresistor,INPUT);
    pinMode(power,OUTPUT);
    digitalWrite(power,HIGH); // Turn on power pin, surprisingly important

    // Initialize prevTime
    prevTime = millis();

    // Serial Setup (for debug)
    Serial.begin(9600);

    // RCSwitch library setup
    mySwitch.enableTransmit(txData);
    mySwitch.setPulseLength(pulseLength);

    // Register web endpoints
    Spark.function("on", switchOn);                     // Register 'POST: /{device}/on' function
    Spark.function("off", switchOff);                   // Register 'POST: /{device}/off' function
    Spark.variable("lightLevel", &lightLevel, INT);     // Register 'GET: /{device}/lightLevel' variable
}

void loop() {
    // Read photoresistor
    lightLevel = analogRead(photoresistor);
    // Serial.println(lightLevel);

    // Publish lightLevel at regular intervals
    unsigned long currTime = millis();
    if(currTime-prevTime > publishInterval) {
        prevTime = currTime;
        Spark.publish("lightLevel",String(lightLevel));
        // Serial.println(lightLevel);
    }
    // delay(500);
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



