#include <Wire.h>

byte msb,lsb;
String temp;
int led=10;
int button=9;
boolean paired=false;

// Variables will change:
int ledState = LOW;             // ledState used to set the LED
long previousMillis = 0;        // will store last time LED was updated
long pairingTime = 0;

// the follow variables is a long because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long interval = 50;   
//executed when the device is powered on

void setup()
{
  //begin wire communication at wire address 2
  Wire.begin(2);
  //on wire request execute printTemp
  Wire.onRequest(printTemp);
  //Wire.onReceive(setLed);
  Serial.begin(9600);
  pinMode(led,OUTPUT);
  pinMode(button,INPUT_PULLUP);
}

void loop()
{
  //tried a pseudo-pairing here..
  //just for testing purpose..
  if(paired)
  {
    readTemp();
    delay(500);
  }
  else
  {
    int buttonState = digitalRead(button);
    
    if(buttonState == LOW) 
    {
      pairingTime=millis();
      while(!paired && millis() - pairingTime < 3000)
        blinkLed();
        paired = true;
    }
  }
}

//reads temperature from sensor and updates temp String
void readTemp()
{
  temp = "";
  
  Wire.requestFrom(0x4F,2);// request 2 bytes representing msb and lsb of the temperature
  msb = Wire.read();
  lsb = Wire.read();
  lsb = lsb >> 7; //lsb is 0 or 1 now 
  
  temp = (msb <= 0 ? "-" : "+") + String(abs(msb)) + "." + String(lsb == 1 ? 5 : 0);//make a string that represents the read temperature
}

// send temperature read from sensor to wire
// function executes whenever data is requested by master
// this function is registered as an event, see setup()
void printTemp()
{
  if(paired)
  {
    char buffer[6];
    temp.toCharArray(buffer,6);
    Wire.write(buffer);
  }
}

void blinkLed()
{
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis > interval) 
  {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;   

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW)
      ledState = HIGH;
    else
      ledState = LOW;

    // set the LED with the ledState of the variable:
    digitalWrite(led, ledState);
  }
}
