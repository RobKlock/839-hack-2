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

//Bluetooth
#include "BluetoothSerial.h"
#include "driver/i2s.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "bt_app_core.h"
#include "bt_app_av.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

#include <HTTPClient.h>
#include <SPI.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "arduino_secrets.h"

//ADC
#include <Arduino.h>
#include "ADCSampler.h"
#include "base64.h"
//#include <analogWrite.h>

//ADC

// 839 Hack 2 Main Driver
// This is where everything should happen, integrate things here instead of their own files :)
// Make a local file called arduino_secrets.h to test on your home wifi/whatever

// Gabe Selzer, Rob Klock, Samarth Mathur, Ethan Brown, Sunny Shen
// macos note: change upload speed to 115200 to avoid errors

#define PIN_LED 12
#define pin_led_button 33
#define TEMP_LED_COLD 0
#define TEMP_LED_WARM 2
#define trigPin 13
#define echoPin 14
#define BUTTON_PIN 5
#define SPEAKER_BUTTON_PIN 19
#define LABEL_PIN 18
#define MAX_DISTANCE 700
#define DISTANCE_THRESHOLD 10 // in centimeters

// server URLs
#define DATA_SERVER_URL "http://3.138.135.239:3000/data"
#define SETTINGS_SERVER_URL "http://3.138.135.239:3000/settings"
#define AUDIO_SERVER_URL "http://3.138.135.239:3001/upload"
//ADC start
#define ADC_SERVER_URL "http://3.138.135.239:3000/speech_output"
#define AUDIO_POST_INDICATOR_PIN 15
// bluetooth
#define CONFIG_I2S_LRCK_PIN 25
#define CONFIG_I2S_BCK_PIN  26
#define CONFIG_I2S_DATA_PIN 22
BluetoothSerial SerialBT;

volatile int user_in_bed=0;
volatile int is_speaker_on=0;
WiFiClient *wifiClientADC = NULL;
HTTPClient *httpClientADC = NULL;
ADCSampler *adcSampler = NULL;

// i2s config for using the internal ADC
i2s_config_t adcI2SConfig = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_LSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

  // i2s pins
i2s_pin_config_t i2sPins = {
    .bck_io_num = GPIO_NUM_32,
    .ws_io_num = GPIO_NUM_25,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = GPIO_NUM_33};

// i2s config for bluetooth
i2s_config_t btI2SConfig = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_MSB,
    .intr_alloc_flags = 0,
    .dma_buf_count = 6,
    .dma_buf_len = 60,
    .tx_desc_auto_clear = true};

i2s_pin_config_t btI2SPins = {
    .bck_io_num = CONFIG_I2S_BCK_PIN,
    .ws_io_num = CONFIG_I2S_LRCK_PIN,
    .data_out_num = CONFIG_I2S_DATA_PIN,
    .data_in_num = -1};

// how many samples to read at once
const int SAMPLE_SIZE = 16384;

// send data to a remote address
void sendData(WiFiClient *wifiClient, HTTPClient *httpClient, const char *url, const char *samples, size_t count)
{
  StaticJsonDocument<200> json_doc;
  //send them off to the server
  digitalWrite(AUDIO_POST_INDICATOR_PIN, HIGH);
  httpClient->begin(url);
  int httpCode = httpClient->GET();
  // Serial.print("Get Code: ");
  // Serial.println(httpCode);
  
  // Request JSON body:
  //   {
  //   "queryInput": {
  //     "audioConfig": {
  //       "languageCode": "en-US"
  //     }
  //   },
  //   "inputAudio": "AUDIO_BASE64_STRING"
  // }
  httpClient->addHeader("content-type", "application/json");
  JsonObject obj = json_doc.to<JsonObject>();
  obj["count"] = count;
  obj["queryInput"]["audioConfig"]["languageCode"] = "en-US";
  obj["inputAudio"] = samples;

  String json;
  serializeJson(json_doc, json);
  Serial.print("HTTP JSON: ");
  Serial.println(json);

  httpClient->POST(json);
  httpClient->end();
  digitalWrite(AUDIO_POST_INDICATOR_PIN, LOW);
  // digitalWrite(AUDIO_POST_INDICATOR_PIN, HIGH);
  // delay(1000);
  // digitalWrite(AUDIO_POST_INDICATOR_PIN,LOW);
}

// Task to write samples from ADC to our server
void adcWriterTask(void *param)
{
  // Serial.println("adcWriterTask Start");
  I2SSampler *sampler = (I2SSampler *)param;
  int16_t *samples = (int16_t *)malloc(sizeof(uint16_t) * SAMPLE_SIZE);
  if (!samples)
  {
    Serial.println("Failed to allocate memory for samples");
    return;
  }
  while (true)
  {
    if (user_in_bed){
    Serial.println("User in Bed");
    int samples_read = sampler->read(samples, SAMPLE_SIZE);
    
    Serial.print("audio sample read: ");Serial.print(samples_read);
    // base64 encode it
    std::string base64_text = base64_encode((unsigned char const*)samples, samples_read * sizeof(uint16_t));
    // Serial.println(base64_text.c_str());
    Serial.println("Send audio data");
    sendData(wifiClientADC, httpClientADC, ADC_SERVER_URL, base64_text.c_str(), samples_read * sizeof(uint16_t));
    }
  }
}
//ADC end

// speaker settings
void speakerTask(int op)
{
  if(op == 1) {
    if(is_speaker_on == 0) {
      adcSampler->stop();
      vTaskDelay(pdMS_TO_TICKS(1000));
      Serial.println("Play audio");
      i2s_driver_install(I2S_NUM_0, &btI2SConfig, 0, NULL);
      i2s_set_pin(I2S_NUM_0, &btI2SPins);
      is_speaker_on = 1;
      bt_app_task_start_up();
      /* initialize A2DP sink */
      esp_a2d_register_callback(&bt_app_a2d_cb);
      esp_a2d_sink_register_data_callback(bt_app_a2d_data_cb);
      esp_a2d_sink_init();
      /* initialize AVRCP controller */
      esp_avrc_ct_init();
      esp_avrc_ct_register_callback(bt_app_rc_ct_cb);
      /* set discoverable and connectable mode, wait to be connected */
      esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
      Serial.println("This ESP32 can be connected via bluetooth");
    }
  } else {
    if(is_speaker_on == 1) {
      Serial.println("Stop playing audio");
      i2s_stop(I2S_NUM_0);
      i2s_driver_uninstall(I2S_NUM_0);
      is_speaker_on = 0;
      vTaskDelay(pdMS_TO_TICKS(1000));
      adcSampler->start();
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

unsigned long currentTime;
unsigned long startTime;
unsigned long savePeriod = 3000; // ms
static const unsigned long REFRESH_INTERVAL = 1500; // ms

// Arbitrary values to compare to 
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

  //pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(SPEAKER_BUTTON_PIN, INPUT);  
  Serial.begin(115200);
  SerialBT.begin("ESP32");
  Serial.println("BT Init seccess!");
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }

  // speakerTask(1);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Can't Connect");
  }
  Serial.println("Wifi Connected");
  calibrate();

  //ADC start
  wifiClientADC = new WiFiClient();
  httpClientADC = new HTTPClient();
  pinMode(AUDIO_POST_INDICATOR_PIN,OUTPUT);
  // input from analog microphones such as the MAX9814 or MAX4466
  // internal analog to digital converter sampling using i2s
  // create our samplers
  adcSampler = new ADCSampler(ADC_UNIT_1, ADC1_CHANNEL_7, adcI2SConfig);

  // set up the adc sample writer task
  TaskHandle_t adcWriterTaskHandle;
  adcSampler->start();
  xTaskCreatePinnedToCore(adcWriterTask, "ADC Writer Task", 4096, adcSampler, 1, &adcWriterTaskHandle, 1);
  //ADC end
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

/*Samarth: Hopefully this piece of code does the right inference
input: array of [weights,bias] and data features
returns 1: for true; 0 for false; -1 for something went wrong
*/
void calibrate(){
  get_parameters();
  float old_lr1 = log_reg_w1;
  float old_lr2 = log_reg_w2;
  
  while (!(log_reg_w1 == old_lr1)){ 
    static unsigned long lastRefreshTime = 0;
    if(millis() - lastRefreshTime >= REFRESH_INTERVAL){
      StaticJsonDocument<200> json_doc;
      HTTPClient http;
      http.begin(DATA_SERVER_URL);
      http.addHeader("Content-Type", "application/json"); 
      float sonar_distance = getSonar();
      float button_state = digitalRead(BUTTON_PIN);
      JsonArray sonar = json_doc.createNestedArray("sonar");  
      JsonArray bed_sensor = json_doc.createNestedArray("bed_sensor");
      
      lastRefreshTime += REFRESH_INTERVAL;
      
      sonar.add(sonar_distance);
      bed_sensor.add(button_state);
      
      String json;
      serializeJson(json_doc, json);
      http.POST(json);  
      
    }
    // Update params
    get_parameters();
  }
  
  Serial.println("Calibrated");
  Serial.print("Old w1: ");
  Serial.println(old_lr1);
  Serial.print("New w1: ");
  Serial.println(log_reg_w1);
}

int inference(float w1, float w2, float b, float sonar, float button_state){
  const double e=2.71828;
  float threshold=0.2; //default threshold

  double logit=0;
  logit = (w1 * sonar) + (w2*button_state) + b;
  logit = 1/(1+pow(e, -logit));
  
  if (logit>threshold){
    return 1;
  }
  else{
    return 0;
  }
}


void loop() {
  Serial.print("SPEAKER_BUTTON_PIN: ");
  Serial.println(digitalRead(SPEAKER_BUTTON_PIN));
  if(digitalRead(SPEAKER_BUTTON_PIN) == 0) { 
    vTaskDelay(pdMS_TO_TICKS(100));
    speakerTask(1);
  } else {
    vTaskDelay(pdMS_TO_TICKS(100));
    speakerTask(0);
  }
  
  StaticJsonDocument<200> json_doc;
  HTTPClient http;
  http.begin(DATA_SERVER_URL);
  http.addHeader("Content-Type", "application/json"); 
  float sonar_distance = getSonar();
  float button_state = digitalRead(BUTTON_PIN);
  static unsigned long lastRefreshTime = 0;
  if(millis() - lastRefreshTime >= REFRESH_INTERVAL){
    get_parameters();
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
    
    user_in_bed = inference(log_reg_w1, log_reg_w2, log_reg_b, sonar_distance, button_state);
    Serial.print("user_in_bed: ");
    Serial.println(user_in_bed);
    // if the object is closer than DISTANCE_THRESHOLD cm OR button is pressed, turn off the LED; otherwise, turn on the LED
    // Sleep context criteria
    // 0 is waking, 1 is sleeping
    // Infer 0 is awake 
    // Sleep context criteria
    if(user_in_bed == 1) {
      sleep_context();
     
    }
    // infer val is 0
    else {
      wake_context();
    }
  
  }
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
  // Serial.println("Waking Context..."); // Serial monitor debugging
  
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

void get_parameters() {
  char* serverName = SETTINGS_SERVER_URL;
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  int httpResponseCode = http.GET();
  String payload = "{}"; 
  if (httpResponseCode>0) {
    // Serial.print("HTTP Response code: ");
    // Serial.println(httpResponseCode);
    
    payload = http.getString();
    // Serial.println(payload);
    
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
  if (w1 != log_reg_w1 and w2 != log_reg_w2 and bias != log_reg_b){
    log_reg_w1 = w1;
    log_reg_w2 = w2;
    log_reg_b = bias; 
  // Print values.

    Serial.print("w1: ");
    Serial.println(w1);
    Serial.print("w2: ");
    Serial.println(w2);
    Serial.print("bias: ");
    Serial.println(bias);
  }
  
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

}