
#include <SPI.h>
#include <Ethernet.h>

// assign a MAC address for the ethernet controller.
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
char device_id_string[4];
byte device_id_pos;
byte device_id;

char device_value_string[4];
byte device_value_pos;
byte device_value;

char testString[] = "{\"device123\"=0, \"device15\"=32}";
byte testStringIndex;

// for manual configuration:
//IPAddress ip(192,168,1,122);

// fill in your Domain Name Server address here:
//IPAddress myDns(78, 96, 7, 88);

// initialize the library instance:
EthernetClient client;

char serverName[] = "graph.facebook.com";
char inChar;

unsigned long lastAttemptTime = 0;          // last time you connected to the server, in milliseconds
const unsigned long requestInterval = 2*1000;  // delay between updates, in milliseconds
String currentLine = "";            // string to hold the text from server
String tweet = "";                  // string to hold the tweet
boolean readingTweet = false;       // if you're currently reading the tweet
int likes;
byte led = 9;
byte skipChar;


byte state;
#define READING_HEADER       1
#define READING_DEVICE_ID    2
#define READ_DEVICE_ID       3
#define READING_VALUE        4


void setup() {
  // start serial port:
  Serial.begin(9600);
  // give the ethernet module time to boot up:
  delay(1000);
  currentLine.reserve(256);
  pinMode(led, OUTPUT); 
  
  // attempt a DHCP connection:
  Serial.println("Attempting to get an IP address using DHCP:");
  if (!Ethernet.begin(mac)) {
    // if DHCP fails, start with a hard-coded address:
    Serial.println("failed to get an IP address using DHCP"); //, trying manually
    //Ethernet.begin(mac, ip);
  }
  Serial.print("My address:");
  Serial.println(Ethernet.localIP());
  testStringIndex = 0;

  connectToServer();  
}







void loop() {
  
  
  if (client.connected()) {
    
    if (client.available()) {
      // read incoming bytes:
      //inChar = client.read();
      skipChar = false;
      inChar = testString[testStringIndex++];
      if (testStringIndex > sizeof(testString)){
       testStringIndex = 0; 
      }
      delay(100);
      
      //strip spaces
      if (inChar == ' '){
        skipChar = true;
      }
      if (!skipChar){
        Serial.print(inChar);
  
        // add incoming byte to end of line:
        currentLine += inChar; 
      }

      // if you get a newline, clear the line:
      if (inChar == '\n') {
        currentLine = "";
      } 
            /*
      // if you're currently reading the bytes of a tweet,
      // add them to the tweet String:
      if (readingTweet) {
        if (inChar != ',') {
          tweet += inChar;
        } 
        else {
          // if you got a "<" character,
          // you've reached the end of the tweet:
          readingTweet = false;
          likes = atoi(tweet.c_str());
          if (likes == 263)
          {
            digitalWrite(led, HIGH);
          } else {
            digitalWrite(led, LOW);
          }
          Serial.println(likes);   
          // close the connection to the server:
          client.stop(); 
        }
      }
   */   
      // if the current line ends with  <  "device   >, it will be followed by a device ID
      if (!skipChar && state == READING_HEADER && currentLine.endsWith("\"device")) {
        // "deviceXXXXX - start reading XXXX
        state = READING_DEVICE_ID;
        device_id_string[0] = 0;
        device_id_pos = 0;
        currentLine = "";
        skipChar = true;
      }
      
      if (!skipChar && state == READING_DEVICE_ID){
        if (inChar == '"'){
          state = READ_DEVICE_ID;
          device_id_string[device_id_pos] = 0;
          device_id = atoi(device_id_string);
        } else
        {
          device_id_string[device_id_pos++] = inChar;
        }
      }
        
      if (!skipChar && state == READ_DEVICE_ID){
        if (inChar == '='){
          state = READING_VALUE;
          device_value_string[0] = 0;
          device_value_pos = 0;
          skipChar = true;
        }
      }
      
      
      if (!skipChar && state == READING_VALUE){
        if (inChar == ',' || inChar == '}'){
          state = READING_HEADER;
          device_value_string[device_value_pos] = 0;
          device_value = atoi(device_value_string);
          
          Serial.println();
          Serial.println();
          Serial.print(device_id_string);
          Serial.print(" ");
          Serial.println(device_value_string);
          Serial.println();
          Serial.println();
          Serial.print(device_id);
          Serial.print(" ");
          Serial.println(device_value);
        } else
        {
          device_value_string[device_value_pos++] = inChar;
        }
      }      

    }  
  }
  else if (millis() - lastAttemptTime > requestInterval) {
    // if you're not connected, and two minutes have passed since
    // your last connection, then attempt to connect again:
    connectToServer();
  }
}
  
void connectToServer() {
  // attempt to connect, and wait a millisecond:
  Serial.println("connecting to server...");
  if (client.connect(serverName, 80)) {
    Serial.println("making HTTP request...");
    // make HTTP GET request to twitter:
    client.println("GET /276047202430945?fields=likes HTTP/1.1");
    client.println("HOST: graph.facebook.com");
    client.println("Connection: close");
    client.println();
    state = READING_HEADER;
  }  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    Serial.println("disconnecting.");
    client.stop();
  }
  // note the time of this connect attempt:
  lastAttemptTime = millis();
}  
  
