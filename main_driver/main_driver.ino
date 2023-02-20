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
#define trigPin 13
#define echoPin 14
#define BUTTON_PIN 5
#define MAX_DISTANCE 700
#define DISTANCE_THRESHOLD 5 //in centimeters
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
  delay(100);
  float sonar_distance = getSonar();
  float button_state = digitalRead(BUTTON_PIN);
  // if the object is closer than DISTANCE_THRESHOLD cm OR button is pressed, turn off the LED; otherwise, turn on the LED
  if( sonar_distance <= DISTANCE_THRESHOLD || button_state==LOW ) {
    digitalWrite(PIN_LED, LOW); // turn off the light
    Serial.println("pushed"); // Serial monitor debugging

  
    HTTPClient http;
    String url = SECRET_TRIGGER;
    // Note that this triggers Rob's computer
    http.begin(url);
    Serial.println("sending Spotify Request");
    int httpResponseCode = http.GET();   
  }
  else 
    digitalWrite(PIN_LED, HIGH); // turn on the light
  }


void send_webhook(){
  Serial.println("Entered Send Webhook");
  // construct the JSON payload
  String jsonString = "";
  jsonString += "{\"value1\":\"";
  jsonString += "test";
  jsonString += "\"}";
  int jsonLength = jsonString.length();  
  String lenString = String(jsonLength);
  // connect to the Maker event server
  client.connect("maker.ifttt.com", 80);
  // construct the POST request
  String url = SECRET_TRIGGER;
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP32\r\n" +
               "Connection: close\r\n\r\n");
  delay(500);
  client.stop();
}