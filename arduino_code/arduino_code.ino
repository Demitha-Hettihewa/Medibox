
#include <PubSubClient.h>
#include <WiFi.h>

WiFiClient espclient;
PubSubClient mqttClient(espclient);
void setup() {
  Serial.begin(115200);
  setupWifi();
  setupMqtt();
}

void loop() {
  if(!mqttClient.connected())
  {
    connectToBroker();
  }
  mqttClient.loop();
  mqttClient.publish("ENTC-TEMP", "25.55");
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
}

void connectToBroker()
{
  while(!mqttClient.connected())
  {
    Serial.print("Attemping MQTT connection...");
    if(mqttClient.connect("ESP32-3242425"))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed ");
      Serial.print(mqttClient.state());
      delay(2000);
    }
  }
}