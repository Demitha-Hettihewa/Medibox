
#include <PubSubClient.h>
#include <WiFi.h>
#include "DHTesp.h"
#include <ESP32Servo.h>

#define LED_BUILTIN 2
#define LDR_PIN 32
#define SERVO_PIN 18
const int DHT_PIN = 15;

Servo servoMotor;
DHTesp dhtSensor;
WiFiClient espclient;
PubSubClient mqttClient(espclient);
char tempAr[6];
char humiAr[6];
String sInte;
int prev_angle = 0;

void setup() {
  Serial.begin(115200);
  setupWifi();
  setupMqtt();
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  servoMotor.attach(SERVO_PIN, 500, 2400);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  digitalWrite(LED_BUILTIN, LOW);

}

void loop() {
  if(!mqttClient.connected())
  {
    connectToBroker();
  }
  mqttClient.loop();
  
  
  updateTempHumi();
  readIntensity();
  Serial.println(tempAr);
  mqttClient.publish("Medibox Temp", tempAr);
  mqttClient.publish("Medibox Humi", humiAr);
  mqttClient.publish("Medibox Inte", sInte.c_str());
  delay(1000);
}


void setupWifi(){
  WiFi.begin("Wokwi-GUEST", "");
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
}


void setupMqtt()
{
  mqttClient.setServer("test.mosquitto.org", 1883);
  mqttClient.setCallback(receiveCallback);
}

void connectToBroker()
{
  while(!mqttClient.connected())
  {
    Serial.print("Attemping MQTT connection...");
    if(mqttClient.connect("ESP32-3242425"))
    {
      Serial.println("connected");
      mqttClient.subscribe("Medibox light");
      mqttClient.subscribe("Medibox motor");
      
    }
    else
    {
      Serial.print("failed ");
      Serial.print(mqttClient.state());
      delay(2000);
    }
  }
}

void updateTempHumi()
{
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  String(data.temperature, 2).toCharArray(tempAr, 6);
  String(data.humidity, 2).toCharArray(humiAr, 6);
}

void readIntensity()
{
  int LDR_reading = analogRead(LDR_PIN);
  float intensity = 1 - LDR_reading/4095.0;
  Serial.print("intensity: ");
  Serial.println(intensity);
  sInte = String(intensity);
}

void receiveCallback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Messeage arrive [");
  Serial.print(topic);
  Serial.print("]\n");

  char payloadCharAr[length];
  for(int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    payloadCharAr[i] = (char)payload[i];
  }
  if(strcmp(topic, "Medibox light")==0)
  {
    if(payloadCharAr[0] == '1')
    {
        digitalWrite(LED_BUILTIN, HIGH);
    }
    else
    {
        digitalWrite(LED_BUILTIN, LOW);
    }
  }
  else if(strcmp(topic, "Medibox motor")==0)
  {
    int motor_angle = (int) atof(payloadCharAr);
    int angle_difference = motor_angle-prev_angle;
    servoMotor.write(motor_angle);
    delay(100);
    Serial.print("motor angle: ");
    Serial.println(motor_angle);
  }

  
}
