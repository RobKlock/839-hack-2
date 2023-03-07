// macos note: change upload speed to 115200 to avoid errors
#define pin_led 12
#define pin_led_button 33
#define trigpin 13
#define echopin 14
#define button_pin 5
#define max_distance 700
float timeout = max_distance * 60;
int soundvelocity = 340;

void setup() {
  // put your setup code here, to run once:
  pinmode(pin_led, output);
  pinmode(pin_led_button, output);
  pinmode(trigpin, output);
  pinmode(echopin, input);
  //pinmode(button_pin, input_pullup);
  pinmode(button_pin, input);
  serial.begin(115200);
}
float getsonar() {
	unsigned long pingtime;
	float distance;
	// make trigpin output high level lasting for 10us to trigger hc_sr04
	digitalwrite(trigpin, high);
	delaymicroseconds(10);
	digitalwrite(trigpin, low);
	// wait hc-sr04 returning to the high level and measure out this waiting time
	pingtime = pulsein(echopin, high, timeout);
	// calculate the distance according to the time
	distance = (float)pingtime * soundvelocity / 2 / 10000;
	return distance; // return the distance value
}

void loop() {
  // put your main code here, to run repeatedly:
  float d;
  delay(100);
  float sonar_distance = getsonar();
  float button_state = digitalread(button_pin);
  serial.println(digitalread(button_pin));
  // if the object is closer than 20 cm or button is pressed, turn off the led; otherwise, turn on the led
  if( sonar_distance <= 50) {
    digitalwrite(pin_led, high); // turn off the light
    serial.println("pushed"); // serial monitor debugging
  }
  else 
    digitalwrite(pin_led, low); // turn on the light

  if(button_state==low ) {
    digitalwrite(pin_led_button, high); // turn off the light
    serial.println("pushed"); // serial monitor debugging
  }
  else 
    digitalwrite(pin_led_button, low); // turn on the light
}