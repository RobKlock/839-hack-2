// 839 Hack 2 Main Driver
// This is where everything should happen, integrate things here instead of their own files :)
// Make a local file called arduino_secrets.h to test on your home wifi/whatever

// Gabe Selzer, Rob Klock, Samarth Mathur, Ethan Brown, Sunny Shen
#include <SPI.h>
#include <WiFi.h>
#include "arduino_secrets.h"

#define PIN_LED 12
#define trigPin 13
#define echoPin 14
#define BUTTON_PIN 5
#define MAX_DISTANCE 700
float timeOut = MAX_DISTANCE * 60;
int soundVelocity = 340;

const char* ssid = SECRET_SSID; // Fill in real value in arduino_secrets.h
const char* password = SECRET_PASSWORD;
const char* host = "maker.ifttt.com";
const char* trigger = SECRET_TRIGGER;
const char* apiKey = SECRET_APIKEY;


void setup() {
  // put your setup code here, to run once:
  pinMode(PIN_LED, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  //pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT);
  Serial.begin(115200);
}
float getSonar() {
	unsigned long pingTime;
	float distance;
	// make trigPin output high level lasting for 10us to trigger HC_SR04
	digitalWrite(trigPin, HIGH);
	delayMicroseconds(10);
	digitalWrite(trigPin, LOW);
	// Wait HC-SR04 returning to the high level and measure out this waiting time
	pingTime = pulseIn(echoPin, HIGH, timeOut);
	// calculate the distance according to the time
	distance = (float)pingTime * soundVelocity / 2 / 10000;
	return distance; // return the distance value
}

void loop() {
  // put your main code here, to run repeatedly:
  float d;
  delay(100);
  float sonar_distance = getSonar();
  float button_state = digitalRead(BUTTON_PIN);
  Serial.println(digitalRead(BUTTON_PIN));
  // if the object is closer than 20 cm OR button is pressed, turn off the LED; otherwise, turn on the LED
  if( sonar_distance <= 20 || button_state==LOW ) {
    digitalWrite(PIN_LED, LOW); // turn off the light
    Serial.println("pushed"); // Serial monitor debugging
  }
  else 
    digitalWrite(PIN_LED, HIGH); // turn on the light
  }

