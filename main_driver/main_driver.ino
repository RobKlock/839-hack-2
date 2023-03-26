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
#include <ArduinoJson.h>
#include "arduino_secrets.h"

// 839 Hack 2 Main Driver
// This is where everything should happen, integrate things here instead of their own files :)
// Make a local file called arduino_secrets.h to test on your home wifi/whatever

// Gabe Selzer, Rob Klock, Samarth Mathur, Ethan Brown, Sunny Shen
// macos note: change upload speed to 115200 to avoid errors

#define PIN_LED 12
#define pin_led_button 33
#define TEMP_LED_COLD 0
#define TEMP_LED_WARM 15
#define trigPin 13
#define echoPin 14
#define BUTTON_PIN 5
#define LABEL_PIN 18
#define MAX_DISTANCE 700
#define DISTANCE_THRESHOLD 10 // in centimeters

unsigned long currentTime;
unsigned long startTime;
unsigned long savePeriod = 3000; // ms
static const unsigned long REFRESH_INTERVAL = 1500; // ms

float log_reg_w1;
float log_reg_w2;
float log_reg_b; 

int state = 2; // 0 = waking, 1 = sleeping, 2 = calibration/debugging

// HTTP Request 
HTTPClient http;
String url="https://<IPaddress>/testurl";

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

 // 
  //pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Can't Connect");
  }
  Serial.println("Wifi Connected");

  double weights[3] = {0,0,0};
  get_parameters(weights);
  
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
  StaticJsonDocument<200> json_doc;
  HTTPClient http;
  http.begin("http://3.138.135.239:3000/data");
  http.addHeader("Content-Type", "application/json"); 
  float sonar_distance = getSonar();
  float button_state = digitalRead(BUTTON_PIN);
  static unsigned long lastRefreshTime = 0;
  if(millis() - lastRefreshTime >= REFRESH_INTERVAL){
    JsonArray sonar = json_doc.createNestedArray("sonar");
    JsonArray bed_sensor = json_doc.createNestedArray("bed_sensor");
    
    lastRefreshTime += REFRESH_INTERVAL;
    sonar_distance = getSonar();
    button_state = digitalRead(BUTTON_PIN);
    
    sonar.add(sonar_distance);
    bed_sensor.add(button_state);
    
    String json;
    serializeJson(json_doc, json);
    http.POST(json);
  }

  // if the object is closer than DISTANCE_THRESHOLD cm OR button is pressed, turn off the LED; otherwise, turn on the LED
  // Sleep context criteria
  // if( sonar_distance <= DISTANCE_THRESHOLD && button_state==LOW ) {
  //   if(state==1){
  //     delay(500);f
  //   }
  //   if (state==0){ 
  //     sleep_context();
  //     state=1;
  //   }
  // }
  
  // else{
  //   wake_context();
  //   state=0;
  //   delay(500);
  // }
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

void get_parameters(double* weights){
  // HTTPClient http;
  httpGETRequest("http://3.138.135.239:3000/settings",weights);
  // Serial.println(sensorReadings);
}

void httpGETRequest(const char* serverName,double* weights) {
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  int httpResponseCode = http.GET();
  String payload = "{}"; 
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    
    payload = http.getString();
    Serial.println(payload);
    
    StaticJsonDocument<200> doc;
    // String json[] = 
    const int length = payload.length();
 
    // declaring character array (+1 for null terminator)
    char* char_array = new char[length + 1];
 
    // copying the contents of the
    // string to char array
    strcpy(char_array, payload.c_str());
    
    DeserializationError error = deserializeJson(doc, char_array);
    
    // Test if parsing succeeds.
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
    }

  // Fetch values.

  double w1 = doc["logistic_regression_weights"]["w"][0][0];
  double w2 = doc["logistic_regression_weights"]["w"][0][1];
  double bias = doc["logistic_regression_weights"]["b"][0];
  log_reg_w1 = w1;
  log_reg_w2 = w2;
  log_reg_b = bias; 
  // Print values.
  Serial.println(w1);
  Serial.println(w2);
  Serial.println(bias);
  //Populate parameters array with values
  weights[0]=w1;
  weights[1]=w2;
  weights[2]=bias;

  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

}

/*Hopefully this piece of code does the right inference
input:
  array of [weights,bias] and data features
  len=number of parameters
returns 1: for true; 0 for false; -1 for something went wrong
*/

int inference(double* weights,double* data, int len=3){

  const double e=2.71828;
  float threshold=0.5; //default threshold

  double logit=0;
  for (int i=0; i<len; i++){
    logit+=weights[i]*data[i];
  }
  logit=1/(1+pow(e, -logit));
  if (logit>0.5){
    return 1;
  }
  else{
    return 0;
  }
}