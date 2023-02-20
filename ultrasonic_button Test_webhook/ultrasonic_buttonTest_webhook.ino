#include <WiFi.h>
#include <HTTPClient.h>

#define PIN_LED 2
#define trigPin 13
#define echoPin 14
#define BUTTON_PIN 16
#define MAX_DISTANCE 700
float timeOut = MAX_DISTANCE * 60;
int soundVelocity = 340;

const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";
const char* host = "maker.ifttt.com";
const char* trigger = "REPLACE_WITH_PATH/TO/TRIGGER/";
const char* apiKey = "REPLACE_WITH_YOUR_IFTTT_API_KEY";

void setup() {
  // put your setup code here, to run once:
  pinMode(PIN_LED, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(115200);

  WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
}

void loop() {
  // put your main code here, to run repeatedly:
  float d;
  delay(100);
  sonar_distance = getSonar();
  button_state = digitalRead(BUTTON_PIN);
  // if the object is closer than 20 cm OR button is presses, turn off the LED; otherwise, turn on the LED
  if(sonar_distance <= 20 || button_state==LOW ) {
    digitalWrite(PIN_LED, HIGH); // turn off the light
      WiFiClient client;
      HTTPClient http;

      const int httpPort = 80;
      if (!client.connect(host, httpPort)) {
        return;
      }

      String url = host + trigger + apiKey;
      http.begin(client, url);

      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      String httpRequestData = "api_key=" + apiKey;           
      int httpResponseCode = http.POST(httpRequestData);
      http.end();

  }
  else digitalWrite(PIN_LED, LOW); // turn on the light
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