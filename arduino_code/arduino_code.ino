
#include <PubSubClient.h>
#include <WiFi.h>
#include "DHTesp.h"

#define LED_BUILTIN 2
const int DHT_PIN = 15;
DHTesp dhtSensor;
WiFiClient espclient;
PubSubClient mqttClient(espclient);
char tempAr[6];

void setup() {
  Serial.begin(115200);
  setupWifi();
  setupMqtt();
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

}

void loop() {
  if(!mqttClient.connected())
  {
    connectToBroker();
  }
  mqttClient.loop();
  
  updateTemperature();
  Serial.println(tempAr);
  mqttClient.publish("ENTC-TEMP", tempAr);
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
      mqttClient.subscribe("ENTC-ON-OFF");
      
    }
    else
    {
      Serial.print("failed ");
      Serial.print(mqttClient.state());
      delay(2000);
    }
  }
}

void updateTemperature()
{
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  String(data.temperature, 2).toCharArray(tempAr, 6);
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

    if(strcmp(topic, "ENTC-ON-OFF")==0)
    {
      if(payloadCharAr[0] == '1'){
        digitalWrite(LED_BUILTIN, HIGH);
      }
      else
      {
        digitalWrite(LED_BUILTIN, LOW);
      }
    }

  }
}