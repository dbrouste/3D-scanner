//Board DOIT ESP32 DEVKIT v1

#include <WiFi.h>
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

 //Declare pin functions on RedBoard
#define stp 32
#define dir 33
#define MS1 34
#define MS2 35
#define EN  25

//****Stepper driver Microstep Resolution************
// MS1  MS2 Microstep Resolution
// 0    0   Full Step (2 Phase)
// 1    0   Half Step
// 0    1   Quarter Step
// 1    1   Eigth Step

//Declare variables for functions
// Stepper function
char user_input;
//int y;
//int state;
float StepperMinDegree = 1.8; // pas mimimum du moteur en degree
int StepperAngleDiv = 8; //1 2 4 ou 8
int Angle;
//int Vitesse;
int Steps = 20; // Nombre de pose par tour
int attente = 1000; // Attente avant photo

// Sony function
volatile int counter;
const char* ssid     = "DIRECT-CeE0:ILCE-7RM2";
const char* password = "9E8EqQDV";     // your WPA2 password
const char* host = "192.168.122.1";   // fixed IP of camera
const int httpPort = 8080;
char JSON_1[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"getVersions\",\"params\":[]}";
char JSON_2[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"startRecMode\",\"params\":[]}";
char JSON_3[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"startBulbShooting\",\"params\":[]}";
char JSON_4[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"stopBulbShooting\",\"params\":[]}";
char JSON_5[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"actTakePicture\",\"params\":[]}";
//char JSON_6[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"actHalfPressShutter\",\"params\":[]}";
//char JSON_7[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"cancelHalfPressShutter\",\"params\":[]}";
//char JSON_8[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"setSelfTimer\",\"params\":[2]}";
//char JSON_9[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"setFNumber\",\"params\":[\"5.6\"]}";
//char JSON_10[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"setShutterSpeed\",\"params\":[\"1/200\"]}";
//char JSON_11[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"setIsoSpeedRate\",\"params\":[]}";
//char JSON_6[]="{\"method\":\"getEvent\",\"params\":[true],\"id\":1,\"version\":\"1.0\"}";
//char JSON_3[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"startLiveview\",\"params\":[]}";
//char JSON_4[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"stopLiveview\",\"params\":[]}";
WiFiClient client;

unsigned long lastmillis;

void setup() {
  pinMode(stp, OUTPUT);
  pinMode(dir, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  pinMode(EN, OUTPUT);
  resetEDPins(); //Set step, direction, microstep and enable pins to default states
  Serial.begin(115200); //Open Serial connection for debugging
  SerialBT.begin("ESP32test"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
  Serial.println("Open remote control app on your camera!");
  delay(10000);
  CameraConnection();
  SerialBT.println("Begin motor control");
  SerialBT.println();
  //Print function list for user selection
  SerialBT.println("Enter number for control option:");
  SerialBT.println("1. Definir angle");
  SerialBT.println("2. Lancer le scan.");
//  SerialBT.println("3. Turn at 1/8th microstep mode.");
//  SerialBT.println("4. Step forward and reverse directions.");
  SerialBT.println();
}

 //Main loop
void loop() {
  while(SerialBT.available()){
      user_input = SerialBT.read(); //Read user input and trigger appropriate function
      if (user_input =='1') {
         SerialBT.println("Entrer nombre de pas");
         delay(5000);
//         while(!SerialBT.available());
         Steps = SerialBT.read();
         SerialBT.println(Steps);
      }
      else if(user_input =='2')
      {
        RotationComplete();
      }
//      else if(user_input =='3')
//      {
//        SmallStepMode();
//      }
//      else if(user_input =='4')
//      {
//        ForwardBackwardStep();
//      }
      else
      {
        SerialBT.println("Invalid option entered.");
        SerialBT.println(user_input);
      }
      resetEDPins();
  }
}

void CameraConnection()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {   // wait for WiFi connection
    delay(500);
    SerialBT.print(".");
  }
  SerialBT.println("");
  SerialBT.println("WiFi connected");
  SerialBT.println("IP address: ");
  SerialBT.println(WiFi.localIP());
  httpPost(JSON_1);  // initial connect to camera
  httpPost(JSON_2); // startRecMode
  //httpPost(JSON_3);  //startLiveview  - in this mode change camera settings  (skip to speedup operation)
}

void httpPost(char* jString) {
  SerialBT.print("connecting to ");
  SerialBT.println(host);
  if (!client.connect(host, httpPort)) {
    SerialBT.println("connection failed");
    return;
  }
  else {
    SerialBT.print("connected to ");
    SerialBT.print(host);
    SerialBT.print(":");
    SerialBT.println(httpPort);
  }
  // We now create a URI for the request
  String url = "/sony/camera/";
 
  SerialBT.print("Requesting URL: ");
  SerialBT.println(url);
 
  // This will send the request to the server
  client.print(String("POST " + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n"));
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(strlen(jString));
  // End of headers
  client.println();
  // Request body
  client.println(jString);
  SerialBT.println("wait for data");
  lastmillis = millis();
  while (!client.available() && millis() - lastmillis < 8000) {} // wait 8s max for answer
 
  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    SerialBT.print(line);
  }
  SerialBT.println();
  SerialBT.println("----closing connection----");
  SerialBT.println();
  client.stop();
}

void DeclenchementPhoto()
{
  httpPost(JSON_5);  //actTakePicture
}

//Resolution moteur
void ResolutionMoteur(int Resolution)
{
  if (Resolution = 8) {
    digitalWrite(MS1, HIGH); //Pull MS1, and MS2 high to set logic to 1/8th microstep resolution
    digitalWrite(MS2, HIGH);
    }
  else if (Resolution = 4) {
    digitalWrite(MS1, LOW);
    digitalWrite(MS2, HIGH);
    }
  else if (Resolution = 2) {
    digitalWrite(MS1, HIGH);
    digitalWrite(MS2, LOW); 
    } 
  else if (Resolution = 1) {
    digitalWrite(MS1, LOW);
    digitalWrite(MS2, LOW);
    }
}

//Default angle mode function
void AngleForward()
{
  SerialBT.println("Moving Angle forward");
  digitalWrite(dir, LOW); //Pull direction pin low to move "forward"
  TournerAngle();
}

//Default angle mode function
void AngleBackward()
{
  SerialBT.println("Moving Angle forward");
  digitalWrite(dir, HIGH); //Pull direction pin low to move "backward"
  TournerAngle();
  SerialBT.println("Enter new option");
  SerialBT.println();
}

//Default angle mode function
void TournerAngle()
{
  ResolutionMoteur(StepperAngleDiv);
  int Pas = (360/Steps)/(StepperMinDegree/StepperAngleDiv);
  int x;
  for(x= 0; x<Pas; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(stp,HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
}

void RotationComplete()
{
  SerialBT.println("C'est parti!");
  digitalWrite(EN, LOW); //Pull enable pin low to allow motor control
  int y;
  for(y=0;y<Steps;y++)
  {
    SerialBT.println("On declenche!");
    delay(attente);
    DeclenchementPhoto();
    SerialBT.println("On avance!");
    SerialBT.println(y);
    SerialBT.println(Steps);
    AngleForward();

  }
  delay(attente);
  DeclenchementPhoto();
  SerialBT.println("Enter new option");
  SerialBT.println();
}

//Reset Easy Driver pins to default states
void resetEDPins()
{
  digitalWrite(stp, LOW);
  digitalWrite(dir, LOW);
  digitalWrite(MS1, LOW);
  digitalWrite(MS2, LOW);
  digitalWrite(EN, HIGH);
}
