#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <Spacebrew.h>

const char* ssid     = "wifi network";
const char* password = "wifi password";

char host[] = "sandbox.spacebrew.cc";
char clientName[] = "esp8266";
char description[] = "";

ESP8266WiFiMulti wifi;
Spacebrew sb;

// Update sine wave 3 times per second
unsigned int  waveUpdateRate = 1000.0 / 3.;
unsigned long waveTimer;

// Declare handlers for various Spacebrew events
// These get defined below!
void onOpen();
void onClose();
void onError(const char* message);
void onBooleanMessage(const char *name, bool value);
void onStringMessage(const char *name, const char* message);
void onRangeMessage(const char *name, int value);

void setup() {
  Serial.begin(115200);

  // Set up built-in LEDs for output
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(0, LOW);
  digitalWrite(2, LOW);

  // Connect to wifi network
  Serial.println("");
  wifi.addAP(ssid, password);
  while (wifi.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Bind Spacebrew connection event handlers
  sb.onOpen(onOpen);
  sb.onClose(onClose);
  sb.onError(onError);

  // Bind Spacebrew message event handlers
  sb.onBooleanMessage(onBooleanMessage);
  sb.onStringMessage(onStringMessage);
  sb.onRangeMessage(onRangeMessage);

//  sb.onBooleanMessage(std::bind(&onBooleanMessage, std::placeholders::_1, std::placeholders::_2));
//  sb.onStringMessage(std::bind(&onStringMessage, std::placeholders::_1, std::placeholders::_2));
//  sb.onRangeMessage(std::bind(&onRangeMessage, std::placeholders::_1, std::placeholders::_2));

  // Register publishers and subscribers

  // Messages received on the Parrot subscribers will be
  // sent back on the Parrot publishers
  sb.addSubscribe("Parrot String", SB_STRING);
  sb.addSubscribe("Parrot Range", SB_RANGE);
  sb.addSubscribe("Parrot Boolean", SB_BOOLEAN);

  sb.addPublish("Parrot String", SB_STRING);
  sb.addPublish("Parrot Range", SB_RANGE);
  sb.addPublish("Parrot Boolean", SB_BOOLEAN);

  // Send a sine wave
  sb.addPublish("Sine Wave", SB_RANGE);

  // Control the built-in LEDs
  sb.addSubscribe("Red LED", SB_BOOLEAN);
  sb.addSubscribe("Blue LED", SB_BOOLEAN);

  // Connect to the spacebrew server
  sb.connect(host, clientName, description);

}

void loop() {
  sb.monitor();

  unsigned long now = millis();
  if (now - waveTimer > waveUpdateRate) {
    waveTimer = now;

    // sin() generates values between -1 to 1
    float sine = sin(millis() / 1000.0);

    // change those values to be between 0 - 1023
    sine = ((sine + 1) / 2) * 1023;

    sb.send("Sine Wave", (int) sine);
  }

  delay(1);
}

void onBooleanMessage(const char *name, bool value) {
  Serial.print("bool: ");
  Serial.print(name);
  Serial.print(" :: ");
  Serial.println(value);

  //repeat back whatever was sent
  if (strcmp(name, "Parrot Boolean") == 0) {
    sb.send("Parrot Boolean", value);
  }

  //turn the LEDs on and off based on the incoming boolean
  if (strcmp(name, "Red LED") == 0) {
    digitalWrite(0, value ? HIGH : LOW);
  }
  if (strcmp(name, "Blue LED") == 0) {
    digitalWrite(2, value ? HIGH : LOW);
  }
}

void onStringMessage(const char *name, const char* message) {
  Serial.print("string: ");
  Serial.print(name);
  Serial.print(" :: ");
  Serial.println(message);

  //repeat back whatever was sent
  if (strcmp(name, "Parrot String") == 0) {
    sb.send("Parrot String", message);
  }
}

void onRangeMessage(const char *name, int value) {
  Serial.print("range: ");
  Serial.print(name);
  Serial.print(" :: ");
  Serial.println(value);

  //repeat back whatever was sent
  if (strcmp(name, "Parrot Range") == 0) {
    sb.send("Parrot Range", value);
  }
}

void onOpen() {
  //send a message when we get connected!
  //sb.send("Parrot String", "Hello Spacebrew");
}

void onClose() {

}

void onError(const char* message) {
  Serial.print("Received error! ");
  Serial.println(message);
}
