#include <WiFi.h>
#include <HttpClient.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <Servo.h>
#include "SparkFunLSM6DSO.h"
#include "Wire.h"

const int RX_PIN = 15;
const int TX_PIN = 13;
const int RED_LED = 26;
const int GRN_LED = 27;
const int SRVO_PIN = 32;

LSM6DSO myIMU; // Default constructor is I2C, addr 0x6B
Servo servo;

// helper functions
String serverConnect(const char* hostname, int port, String path);
void stabalize();

char ssid[] = "ATTJvVQfYs";   // your network SSID (name)
char pass[] = "5n95evrc3xm4"; // your network password (use for WPA, or use as key for WEP)

// Name of the server we want to connect to
const char kHostname[] = "18.217.73.232";
// Path to download (this is the bit after the hostname in the URL
// that you want to download
// const char kPath[] = "/?var=20";
const int kPort = 5000;

// Number of milliseconds to wait without receiving any data before we give up
const int kNetworkTimeout = 30 * 1000;
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000;

// gps
TinyGPSPlus gps;
HardwareSerial gpsSerial(1); // use UART1

void setup()
{
  // visual display of in/out of bounds
  pinMode(RED_LED, OUTPUT);
  pinMode(GRN_LED, OUTPUT);

  Serial.begin(9600);
  
  // begin gps connection
  gpsSerial.begin(9600, SERIAL_8N1, 15); // GPS RX

  // connect to WIFI network
  delay(1000);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());

  // send initial coordinates to cloud server
  // wait for coordinates to come through from gps module
  while (!gps.location.isUpdated())
  {
    while (gpsSerial.available())
    {
      gps.encode(gpsSerial.read());
    }
  }
  float lat = gps.location.lat();
  // float lng = gps.location.lng();

  Serial.print("Initial lat: ");
  Serial.println(lat, 6);

  // send initial coord data to server for initialization
  String path = "/init?lat=" + String(lat, 6);
  String response = serverConnect(kHostname, kPort, path);
  Serial.print("Response: ");
  Serial.println(response);

  // connect IMU
  Wire.begin();
  delay(10);
  if (myIMU.begin())
    Serial.println("Ready.");
  else {
    Serial.println("Could not connect to IMU.");
    Serial.println("Freezing");
    while (1);
  }

  if (myIMU.initialize(BASIC_SETTINGS))
    Serial.println("Loaded Settings.");

  // connect servo
  servo.attach(SRVO_PIN);
  servo.write(90);
  
}

void loop() {

  // stabalize the plane using LSM6DS0 accelerometer
  stabalize();

  // parse data sent by gps module
  while (gpsSerial.available())
  {
    gps.encode(gpsSerial.read());
  }

  // make sure data is new
  if (gps.location.isUpdated())
  {
    float lat = gps.location.lat();
    float lng = gps.location.lng();

    Serial.print("Latitude: ");
    Serial.println(lat, 6);
    Serial.print("Longitude: ");
    Serial.println(lng, 6);

    delay(500);

    // connect to cloud server

    // create a url path with data from sensor
    String path = "/track";
    path += "?lat=" + String(lat, 6);
    path += "&lng=" + String(lng, 6);

    // send data to serverk and get response
    String response = serverConnect(kHostname, kPort, path);
    if (response == "") {
      Serial.println("NO RESPONSE");
    }
    else if (response == "OOB") {
      //turn on red led (out-of-bounds)
      digitalWrite(RED_LED, HIGH);
      digitalWrite(GRN_LED, LOW);
    }
    else {
      // turn on green led (in-bounds)
      digitalWrite(GRN_LED, HIGH);
      digitalWrite(RED_LED, LOW);
    }
  }

  delay(20);
}

String serverConnect(const char* hostname, int port, String path) {
  
  WiFiClient c;
  HttpClient http(c);

  String response = "";

  int err = http.get(hostname, port, path.c_str());
  if (err == 0)
  {
    Serial.println("Started request ok");

    err = http.responseStatusCode();
    if (err >= 0)
    {
      Serial.print("Got status code: ");
      Serial.println(err);

      http.skipResponseHeaders();

      while (http.available())
      {
        char c = http.read();
        response += c;
      }
      Serial.println("Got status code: ");
      Serial.println(response);
    }
    else
    {
      Serial.print("Response body: ");
      Serial.println(err);
    }
  }
  else
  {
    Serial.print("Connect failed: ");
    Serial.println(err);
  }
  http.stop();

  return response;
}

void stabalize() {
  // float ax = myIMU.readFloatAccelX();
  float yAx = myIMU.readFloatAccelY();
  float zAx = myIMU.readFloatAccelZ();

  // Calculate roll angle and convert from rad to deg
  float rollAngle = atan2(yAx, zAx) * 180 / PI;

  // 0 deg roll ==> 90 deg servo
  float servoAngle = (rollAngle + 90);

  // Keep servo angle between 60 and 120 deg
  if (servoAngle < 60) {
    servoAngle = 60;
  }
  if (servoAngle > 120) {
    servoAngle = 120;
  } 
  
  // give angle to servo as int
  servo.write((int)servoAngle);
}
