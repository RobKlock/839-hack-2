#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiGeneric.h>
#include <WiFiMulti.h>
#include <WiFiSTA.h>
#include <WiFiScan.h>
#include <WiFiServer.h>
#include <WiFiType.h>
#include <WiFiUdp.h>


#include <HTTPClient.h>
#include <SPI.h>
#include <WiFi.h>
#include "arduino_secrets.h"

// 839 Hack 2 Main Driver
// This is where everything should happen, integrate things here instead of their own files :)
// Make a local file called arduino_secrets.h to test on your home wifi/whatever

// Gabe Selzer, Rob Klock, Samarth Mathur, Ethan Brown, Sunny Shen

#define PIN_LED 12
#define TEMP_LED_COLD 0
#define TEMP_LED_WARM 15
#define trigPin 13
#define echoPin 14
#define BUTTON_PIN 5
#define MAX_DISTANCE 700
#define DISTANCE_THRESHOLD 10 //in centimeters
int state = 0; // 0 = waking, 1 = sleeping

float timeOut = MAX_DISTANCE * 60;
int soundVelocity = 340;

const char* ssid = SECRET_SSID; // Fill in real value in arduino_secrets.h
const char* password = SECRET_PASSWORD;
// Note: It takes a minute or two to connect. Personal phone hotspot works best
const char* host = "maker.ifttt.com";
char* trigger = SECRET_TRIGGER;
char* apiKey = SECRET_APIKEY;

WiFiClient client;
void send_webhook();
void sleep_context();
void wake_context();


void setup() {
  // put your setup code here, to run once:
  pinMode(PIN_LED, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  //pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT);
  Serial.begin(115200);

  WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Can't Connect");
    }
    Serial.println("Wifi Connected");
     
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
  float sonar_distance = getSonar();
  float button_state = digitalRead(BUTTON_PIN);
  
  // if the object is closer than DISTANCE_THRESHOLD cm OR button is pressed, turn off the LED; otherwise, turn on the LED
  // Sleep context criteria
  if( sonar_distance <= DISTANCE_THRESHOLD && button_state==LOW ) {
    if(state==1){
      delay(500);
    }
    if (state==0){ 
      sleep_context();
      state=1;
    }
  }
  else 
    wake_context();
    state=0;
    delay(500);
  }

void sleep_context(){
  // IF 
  // Pressure sensor is on
  // Sonar sensor is on 
  // THEN  
  // Play spotify
  trigger_spotify_playback();

  // Turn off room light
  digitalWrite(PIN_LED, LOW); // turn off the green LED
  
  // Debug
  // Serial.println("pushed"); // Serial monitor debugging
  
  // Cool down temperature (increase the blue light, decrease the red light)
  int i = 0;
  while (i<=100){
    analogWrite(TEMP_LED_COLD, i);
    analogWrite(TEMP_LED_WARM, 100-i);
    delay(50);
    i = i+5;    
  }
}

void wake_context(){
  // Pause spotify? For the sake of the demo
  pause_spotify_playback();
  
  // Turn on room light
  digitalWrite(PIN_LED, HIGH); // turn off the green LED
  
  // Debug
  Serial.println("Waking Context..."); // Serial monitor debugging
  
  // Warm up temperature (decrease the blue light, increase the red light)
  int i = 0;
  while (i<=100){
    analogWrite(TEMP_LED_COLD, 100-i);
    analogWrite(TEMP_LED_WARM, i);
    delay(50);
    i = i+5;    
  }
  
}

void pause_spotify_playback(){
  HTTPClient http;
  String url = PAUSE_TRIGGER;
  // Note that this triggers Rob's computer
  http.begin(url);
  Serial.println("Waking: Spotify Pause");
  int httpResponseCode = http.GET();   
}
void trigger_spotify_playback(){
    HTTPClient http;
    String url = SECRET_TRIGGER;
    // Note that this triggers Rob's computer
    http.begin(url);
    Serial.println("Sleeping: Spotify Play");
    int httpResponseCode = http.GET();   
}