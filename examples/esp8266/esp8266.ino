#include <ESP8266WiFi.h>
#include <WebSocketClient.h>
#include <Spacebrew.h>

const char* ssid     = "wifi network";
const char* password = "wifi password";

char host[] = "sandbox.spacebrew.cc";
char clientName[] = "esp8266";
char description[] = "";

WiFiClient wifiClient;
Spacebrew spacebrewConnection;

// Declare handlers for various Spacebrew events
// These get defined below!
void onOpen();
void onClose();
void onError(char* message);
void onBooleanMessage(char *name, bool value);
void onStringMessage(char *name, char* message);
void onRangeMessage(char *name, int value);

void setup() {
  Serial.begin(115200);

  //
  // Connect to wifi network
  //
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Bind Spacebrew connection event handlers
  spacebrewConnection.onOpen(onOpen);
  spacebrewConnection.onClose(onClose);
  spacebrewConnection.onError(onError);

  // Bind Spacebrew message event handlers
  spacebrewConnection.onBooleanMessage(onBooleanMessage);
  spacebrewConnection.onStringMessage(onStringMessage);
  spacebrewConnection.onRangeMessage(onRangeMessage);
  
  // Register publishers and subscribers
  spacebrewConnection.addPublish("Analog", SB_RANGE);
  spacebrewConnection.addPublish("Button", SB_BOOLEAN);
  spacebrewConnection.addPublish("Parrot", SB_STRING);
  spacebrewConnection.addSubscribe("Blink LED", SB_BOOLEAN);
  spacebrewConnection.addSubscribe("Fade LED", SB_RANGE);
  spacebrewConnection.addSubscribe("Parrot", SB_STRING);
  
  // Connect to the spacebrew server
  spacebrewConnection.connect(&wifiClient, host, clientName, description);
  
//  pinMode(digitalLedPin, OUTPUT);
//  pinMode(analogLedPin, OUTPUT);
//  pinMode(buttonPin, INPUT);

}

void loop() {
  // Put your main code here, to run repeatedly:
  spacebrewConnection.monitor();
}

void onBooleanMessage(char *name, bool value){
  //turn the 'digital' LED on and off based on the incoming boolean
//  digitalWrite(digitalLedPin, value ? HIGH : LOW);
  Serial.print("bool: ");
  Serial.print(name);
  Serial.print(" :: ");
  Serial.println(value);
}

void onStringMessage(char *name, char* message){
  //repeat back whatever was sent
  spacebrewConnection.send("Parrot", message);

  Serial.print("string: ");
  Serial.print(name);
  Serial.print(" :: ");
  Serial.println(message);
}

void onRangeMessage(char *name, int value){
  //use the range input to control the brightness of the 'analog' LED
//  analogWrite(analogLedPin, map(value, 0, 1024, 0, 255));
  Serial.print("range: ");
  Serial.print(name);
  Serial.print(" :: ");
  Serial.println(value);
}

void onOpen(){
  Serial.println("Spacebrew opened");
  //send a message when we get connected!
  spacebrewConnection.send("Parrot", "Hello Spacebrew");
}

void onClose(){
  Serial.println("Spacebrew closed");
  //turn everything off if we get disconnected
//  analogWrite(analogLedPin, 0);
//  digitalWrite(digitalLedPin, LOW);

}

void onError(char* message){
}

