
#include "secrets.h"
#include <WebServer.h>
#include <WiFi.h>
#include <esp32cam.h>
#include <ESP32Servo.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
 
//const char* WIFI_SSID = "NewWlanIGC";
//const char* WIFI_PASS = "Secureit@most";
const char* WIFI_SSID = WIFI_SSID_SECRET;
const char* WIFI_PASS = WIFI_SSID_SECRET;
int8_t newMACAddress[] = {0x62, 0xB6, 0x66, 0x17, 0x0D, 0xF6};

#define PAN_PIN 14
#define TILT_PIN 15
#define LIGHT_PIN 4

// MQTT Broker
//const char *mqtt_broker = MQTT_BROKER_SECRET;
const char *mqtt_broker = "192.168.2.88";
const char *topic = "DUMSHOME/JARVIS/jarvisCam";
const char *subscribeTopic = "DUMSHOME/JARVIS/jarvisCam/#";
const char *topicIP = "DUMSHOME/JARVIS/jarvisCam/ip";
const char *topicMsg = "DUMSHOME/JARVIS/jarvisCam/msg";
//const char *mqtt_username = "openhabian";
//const char *mqtt_password = "openhabian";
const char *mqtt_username = MQTT_USERNAME_SECRET;
const char *mqtt_password = MQTT_PASSWORD_SECRE;
const int mqtt_port = 1883;

char temp[50];

Servo panServo;
Servo tiltServo;
 
WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);
 
 
static auto loRes = esp32cam::Resolution::find(320, 240);
static auto midRes = esp32cam::Resolution::find(350, 530);
static auto hiRes = esp32cam::Resolution::find(800, 600);
void serveJpg()
{
  auto frame = esp32cam::capture();
  if (frame == nullptr) {
    Serial.println("CAPTURE FAIL");
    server.send(503, "", "");
    return;
  }
  Serial.printf("CAPTURE OK %dx%d %db\n", frame->getWidth(), frame->getHeight(),
                static_cast<int>(frame->size()));
 
  server.setContentLength(frame->size());
  server.send(200, "image/jpeg");
  WiFiClient client = server.client();
  frame->writeTo(client);
}
 
void handleJpgLo()
{
  if (!esp32cam::Camera.changeResolution(loRes)) {
    Serial.println("SET-LO-RES FAIL");
  }
  serveJpg();
}
 
void handleJpgHi()
{
  if (!esp32cam::Camera.changeResolution(hiRes)) {
    Serial.println("SET-HI-RES FAIL");
  }
  serveJpg();
}
 
void handleJpgMid()
{
  if (!esp32cam::Camera.changeResolution(midRes)) {
    Serial.println("SET-MID-RES FAIL");
  }
  serveJpg();
}

void pantilt(int panmove, int tiltmove)
{
      Serial.print("Receiving data to pan : ");
      Serial.print(panmove);
      Serial.print(" and tilt : ");
      Serial.println(tiltmove);
      panServo.write(panmove);
      tiltServo.write(tiltmove);  
}
void lighton()
{
  digitalWrite(LIGHT_PIN, HIGH);
}
void lightoff()
{
    digitalWrite(LIGHT_PIN, LOW);
}
void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("JarvisCam", mqtt_username, mqtt_password))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(topicMsg, "Jarvis connected");
      // ... and resubscribe
      client.subscribe(subscribeTopic);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 1 seconds");
      // Wait 1 seconds before retrying
      delay(1000);
    }
  String ipaddress = WiFi.localIP().toString();
  ipaddress.toCharArray(temp, ipaddress.length() + 1);
  client.publish(topicIP, temp);

  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  String strPayload;
  
  payload[length] = '\0';
  Serial.print("Payload:-");
  Serial.print((char *)payload);
  Serial.println("-");
  if(strcmp(topic,"DUMSHOME/JARVIS/jarvisCam/light") == 0) {
    Serial.println("matched Topic light");
    Serial.println(topic);
    
    if(strcmp((char *)payload,"ON") == 0) {
    Serial.println("switching light ON");
    lighton();
   }
   if(strcmp((char *)payload,"OFF") == 0)  {
    Serial.println("switching light OFF");
    lightoff();
   }
  }
  if(strcmp(topic,"DUMSHOME/JARVIS/jarvisCam/direct") == 0) {
    Serial.println("matched Topic direct - need to extract data for pan and tilt ");
    Serial.println(topic);
    StaticJsonDocument<200> doc;
    deserializeJson(doc, payload);
    if((int)doc["pan"]>=0 && (int)doc["pan"]<180 && (int)doc["tilt"]>=0 && (int)doc["tilt"]<180)
      {
        Serial.println("valid pan tilt data received");
        Serial.print("pan : ");
        Serial.print((int)doc["pan"]);
        Serial.print(" - tilt : ");
        Serial.println((int)doc["tilt"]);
        pantilt((int)doc["pan"],(int)doc["tilt"]);
        
      }
      else
      {
        Serial.println("Invalid pan tilt data received");
      }
    }
}


void  setup(){  
  Serial.begin(115200);
  Serial.println();
  Serial.print("Setting up servo and light");
  panServo.attach(PAN_PIN);
  tiltServo.attach(TILT_PIN);
  pinMode(LIGHT_PIN, OUTPUT);
  Serial.print("Testing servo and light");
//flash light
  lighton();
  delay(1000);
  lightoff();
//test pan tilt
  pantilt(0,0);
  lighton();
  delay(1000);
  pantilt(180, 180);
  lightoff();
  delay(1000);
  pantilt(90, 90);
  lighton();
  delay(1000);
  lightoff();
  Serial.print("Setting up camera");

  {
    using namespace esp32cam;
    Config cfg;
    cfg.setPins(pins::AiThinker);
    cfg.setResolution(hiRes);
    cfg.setBufferCount(2);
    cfg.setJpeg(80);
 
    bool ok = Camera.begin(cfg);
    Serial.println(ok ? "CAMERA OK" : "CAMERA FAIL");
  }
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.print("http://");
  Serial.println(WiFi.localIP());
  Serial.println("  /cam-lo.jpg");
  Serial.println("  /cam-hi.jpg");
  Serial.println("  /cam-mid.jpg");
 
  server.on("/cam-lo.jpg", handleJpgLo);
  server.on("/cam-hi.jpg", handleJpgHi);
  server.on("/cam-mid.jpg", handleJpgMid);
 
  server.begin();
  // MQTT publish and subscribe
  Serial.print("Setting up MQTT connection");
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);

  while (!client.connected()) {
      if (client.connect("JarvisCam", mqtt_username, mqtt_password)) {
          Serial.println("Mqtt broker connected");
      } else {
          Serial.print("failed with state ");
          Serial.print(client.state());
          delay(2000);
      }
  
  client.publish(topic, "Hello from jarvis cam");
  client.subscribe(subscribeTopic);
  String ipaddress = WiFi.localIP().toString();
  ipaddress.toCharArray(temp, ipaddress.length() + 1);
  client.publish(topicIP, temp);
  }
}


 
void loop()
{
  if (!client.connected()) { reconnect(); }
    client.loop();
  server.handleClient();
}
