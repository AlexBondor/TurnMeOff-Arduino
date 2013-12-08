#include <SPI.h>
#include <Wire.h>
#include <Ethernet.h>
#include <String.h>
#include <EEPROM.h>
#include "EEPROMAnything.h"

/************* ETHERNET DECLARATIONS HERE ***********/
char server[] = "192.168.1.100"; //turnmeoff.com
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
int port = 52011;
EthernetClient client;
/****************************************************/

/************* JSON DECLARATIONS HERE **************/
String jsonText = ""; // read from server
int jsonTextLength;
int deviceNo=4; // devices number
int connectedDevices = 2; //should be 0 at first power on; again.. just for testing purpose it's initialised with 2

String FIELD; // json parsed field
boolean READ_UPDATE; // flags for json parsing
boolean READ_DEVICE;
boolean READ_VALUE;


int update;
String masterCode;
String id[3]; //id[1] - always userID
String value[3];
/***************************************************/

void setup() 
{
  Wire.begin();
  Serial.begin(9600);
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) 
  {
    Serial.println("Failed to configure Ethernet using DHCP");
    for(;;);
  }
  
  masterCode = "FR43DF4"; // set masterCode at start-up
  id[1]="235918c7-c79f-47dc-b532-a0abe797e843"; //light switch id
  id[2]="235918c7-c79f-47dc-b532-a2abe797e943"; //temp sensor id
  
  //EEPROM.read(paired, 0); // just to know if arduino runs for the very first time
  //if(!paired)
  //addPendingMasterDevice(masterCode);// add masterCode in PendingMasterDevices table
  
  client.stop(); // be sure there's no connection before making one..
  id[0] = getUserId(masterCode);// get userId from server
  
  if(id[0] == "")// without user id nothing functions.. try again
    setup();
    
  Serial.print("User id is: "); // everything with Serial.print should dissapear at final form
                                // those are used just for a kind of debugging
  Serial.println(id[0]);
}

/* using the infinite loop arduino keeps collecting data
* 1.requests from server
* 2.sends to devices
* 3.requests from devices
* 4.sends to server
* 5.GOTO step 1.
*/
void loop() 
{
  Serial.println("request info from server..");
  int a = requestInformation_S();
  delay(500);
  
//  Serial.println("share info to devices.."); // commented because of hardware issues..
//  shareInformation_D();
//  delay(500);
  
  
  Serial.println("request info from devices..");
  int b = requestInformation_D();
  delay(500);
  
  Serial.println("share info to server..");
  shareInformation_S();
  delay(500);
  
  delay(1000);
  
  /* the sode below represents an alternative*/
  
//  if(requestInformation_S()) //  requests struct from server
//  {  
//    shareInformation_D();  // sends values to devices
//    delay(500);
//  }
//  
//  if(requestInformation_D()) // reads values from devices 
//  {
//    shareInformation_D(); // sends values to server
//    delay(500);
//  }
}

/*******************************/
/****** ETHERNET METHODS *******/
/*******************************/

/* Makes a HTTP request to the sever
* which results in adding Arduino's
* masterCode in PendingMasterDevices
* table from server.
*/
void addPendingMasterDevice(String masterCode)
{
  if (client.connect(server, 52011)) {
    // Make a HTTP request:
    Serial.println("connected");
    client.println("GET /Master/AddPendingMasterDevice?masterDeviceCode=" + masterCode);
  } 
  else {
    // if you didn't get a connection to the server try again..
    addPendingMasterDevice(masterCode);
  }
  client.stop();
}

/* Returns user id on http request
* the returned userId is the one of the user
* that has the same masterDeviceID (in Users table)
* as the masterCode sent as parameter
*/
String getUserId(String masterCode)
{
  Serial.println("fetching user id..");
  String userId = "";
  if (client.connect(server, 52011)) {
    // Make a HTTP request:
    Serial.println("connected");
    client.println("GET /Master/GetUserId?masterDeviceCode=" + masterCode);
    client.println("Connection: close");
    client.println();
  } 
  else {
    Serial.println("disconnected");
    client.stop();
    // if you didn't get a connection to the server:
    return "";
  }
  
  while(!client.available()); // wait for page to load
  Serial.println("client available..");
  if (client.available()) 
  {
    // if page loaded read json
    char c = client.read();
    while(c != ':')
    {
      c = client.read();
    }
    c = client.read();// read "
    c = client.read();// read first char of userId
    while(c != '"')
    {
      userId.concat(String(c));
      c = client.read();      
    }
    client.stop();// disconnect from server
  }
  else
  { 
    // if page didn't load disconnect from server
    client.stop();
  }
  return userId;
}

/* Method tries to connect to the server
* on SUCCES reads json from requested url and returns it as string
* on FAILURE return empty string
*/
String readJson(String userId)
{
  String json = "";
  if (client.connect(server, 52011)) // try to connect to server
  { 
    // if connected then
    // make a HTTP request:
    client.println("GET /Devices/GetStatus?userId=" + userId);
    client.println();
  } 
  else 
  {
    // if you didn't get a connection to the server
    // Serial.println("Connection failed");
    client.stop();
    return ""; // didn't read any json..
  }
  
  while(!client.available()); // wait for page to load
  if (client.available()) 
  {
    // if page loaded read json
    char c = client.read();
    if(c == '{')
      json.concat(String(c));
    while(c != '}')
    {
      c = client.read();
      json.concat(String(c));
    }
    client.stop();// disconnect from server
  }
  else
  { 
    // if page didn't load disconnect from server
    client.stop();
  }
  return json;
}

/* REQUEST FROM SERVER 
* Makes a HTTP request to the server
* in order to get the latest statuses
* of the devices available on the server
*/

int requestInformation_S()
{
  String jsonString = readJson(id[0]); // read json from web page
  
  Serial.println("json to interpret: ");
  Serial.println(jsonString);
  
  if(jsonString != "")
    interpretJson(jsonString); // interpretJson a.k.a. link the values read from servet
                               // to the ones declared globally * value[] *
  return 1;
}

/* REQUEST FROM DEVICES
* gets current statuses
* from all the devices
*/

int requestInformation_D()
{
  value[1] = readTemp();
  //value[0] = readSwitch(); // hardware issues.. broken pin for raeding value..
  return 1;
}

String readSwitch()
{
  Wire.requestFrom(3, 1);    //request 1 bytes from slave device #3
  String state="";
  while(Wire.available())    //slave may send less than requested
  {
    char c = Wire.read(); //receive a byte as character
    state += (String)c; //concat read char to temp String
  }
  return state;
}

String readTemp()
{
  Wire.requestFrom(2, 5);    //request 5 bytes from slave device #2
  String temp="";
  while(Wire.available())    //slave may send less than requested
  {
    char c = Wire.read(); //receive a byte as character
    temp += (String)c; //concat read char to temp String
  }
  return temp;
}

/* SHARE TO SERVER
*
*/

void shareInformation_S()
{ 
  //String not long enough to hold all the data..
  String urlString = "";
  urlString += "?update=1&id[0]=" + (String)id[0];
  //urlString+=(String)update;
  //urlString += "&id[1]=" + (String)id[1];
  
  //urlString += "&id[2]=" + (String)id[2];
  
  for(int i=0; i<connectedDevices; i++)
  {
    urlString+="&values[" + String(i) + "]=" + (String)value[i];
  }   
  
  Serial.println(urlString);
  
  if (client.connect(server, 52011)) {
    // Make a HTTP request:
    Serial.println("connected to share");
    client.println("GET /Devices/SetStatus" + urlString); //doesn't work.. computes urlString great but it won't access url
  } 
  else {
    // if you didn't get a connection to the server try again..
    shareInformation_S();
  }
  client.stop();
}

/* SHARE TO DEVICES
*
*/

void shareInformation_D()
{
  Wire.beginTransmission(3);
  
  char buffer[2];
  value[0].toCharArray(buffer,2);// hardcoded setting led status..
  Wire.write(buffer);
  
  Wire.endTransmission();
}

/*******************************/
/******** JSON METHODS *********/
/*******************************/

/* run through all the string(json from server)
* character by character and discover keywords
* (update/id/values).. then send them to interpretField(FIELD)
* to update the gobal values[] string and update int
*/
void interpretJson(String jsonString)
{
  int jsonStringLength = jsonString.length(); 
  for(int i=0; i<jsonStringLength; i++)
  {
    switch(jsonString[i])
    {
      case '{' : if(FIELD!=""){interpretField(FIELD);} FIELD=""; break; // send current word to be interpreted
      case '"' : if(FIELD!=""){interpretField(FIELD);} FIELD=""; break;
      case ':' : if(FIELD!=""){interpretField(FIELD);} FIELD=""; break;
      case ',' : if(FIELD!=""){interpretField(FIELD);} FIELD=""; break;
      case '}' : if(FIELD!=""){interpretField(FIELD);} FIELD=""; break;
      default : FIELD += jsonString[i]; // add char to new word
    }
  }
}

/* updates the global dates
* taking into account if keywords
* were already read from json..
* (update/device/value)
*/
void interpretField(String fieldText)
{
  if(READ_UPDATE && fieldText != "device") // if fieldText param is the update value
  {
    if(fieldText == "0")
      update = 0;
    else 
      update = 1;
  }
  else
  {
    if(READ_DEVICE && fieldText != "value") // if fieldText param is a deviceID
    {
      //ids are already known.. do nothing here..
    }
    else
    {
      if(READ_VALUE) // if fieldText param is a value of specific deviceID
      {
        value[deviceNo] = fieldText;
        deviceNo++;
      }
    }
  }
  
  /* set update/device/value flags
  * taking into account a specific
  * keyword
  */
  if(fieldText == "update") 
  {
    READ_UPDATE = true;
    READ_DEVICE = false;
    READ_VALUE = false;
  }
  else
  {
    if(fieldText == "device")
    {
      READ_UPDATE = false;
      READ_DEVICE = true;
      READ_VALUE = false;
    }
    else
    {
      if(fieldText == "value")
      {
        deviceNo = 0;
        READ_UPDATE = false;
        READ_DEVICE = false;
        READ_VALUE = true;
      }
    }
  }
}
