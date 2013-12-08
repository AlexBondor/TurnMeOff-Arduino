#include<Wire.h>

// constants won't change. They're used here to 
// set pin numbers:
const int buttonPin = 6;    // the number of the pushbutton pin
const int ledPin = 5;      // the number of the LED pin
const int triac = 4;      // the number of transistor

boolean changed = false; // if led status has changed since the last time master read status

// Variables will change:
int ledState = HIGH;         // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup() {
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(triac, OUTPUT);
  Wire.begin(3); // device has address 3 on Wire
  Wire.onRequest(sendStatus); // on request from master execute sendStatus()
  Wire.onReceive(setLed);  // on receive from maste execute setLed()
  // set initial LED state
  digitalWrite(ledPin, ledState);
  digitalWrite(triac, HIGH);
}

void loop() {
  // read the state of the switch into a local variable:
  int reading = digitalRead(buttonPin);

  // check to see if you just pressed the button 
  // (i.e. the input went from LOW to HIGH),  and you've waited 
  // long enough since the last press to ignore any noise:  

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    changed = true;
    lastDebounceTime = millis();
  } 
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == HIGH) {
        ledState = !ledState;
      }
    }
  }
  
  // set the LED:
  digitalWrite(ledPin, ledState);
  if(ledState == HIGH)
  {
    //set the lightBulb
    digitalWrite(triac, LOW);
    digitalWrite(triac, HIGH);
  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;
  delay(1);
}

void setLed(int howMany)
{
  String ledStatus="";
  while(Wire.available()) // loop through all but the last
  {
    char c = Wire.read(); // receive byte as a character
    ledStatus+=(String)c;
  }
  if(!changed) // do not overwrite if led status changed
  {
    digitalWrite(ledPin, ledState);
    if(ledStatus=="HIGH")
    {
      digitalWrite(triac, LOW);
      digitalWrite(triac, HIGH);  
      delay(1);
      }
  }
}

void sendStatus()
{
  changed = false;
  if(ledState == HIGH)
  {
    Wire.write(1);
  }
  else
  {
    Wire.write(0);
  }
}

