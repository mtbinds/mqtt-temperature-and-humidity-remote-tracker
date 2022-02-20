#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define LED 4
#define ONE_WIRE_BUS 2

//Dallas temperature
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire); 


// WiFi connection
const char WIFI_SSID[] = "arduino";     // your network SSID (name)
const char WIFI_PASS[] = "123456789"; // your network password (use for WPA, or use as key for WEP)
byte mac[6];
int status = WL_IDLE_STATUS;

// MQTT connection 
#define MQTT_BROKER "broker.emqx.io"
#define MQTT_PORT 1883
#define MQTT_USERNAME "admin"
#define MQTT_PASSWORD "Taoualit2016$"

// MQTT topics
#define TOPIC_SENSOR_TEMP_DATA   "/sensor/temp/data"
#define TOPIC_SENSOR_TEMP_STATUS "/sensor/temp/status"
#define TOPIC_SENSOR_TEMP_LED    "/sensor/temp/led"

WiFiClient httpClient;
PubSubClient mqttClient(httpClient);

unsigned long lastConnectionTime = 0;               // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 15L * 1000L;  // delay between updates, in milliseconds

void setup() {
  setupSerialPorts();
  setup_WiFi();
  
  // set server
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);

  // callback
  mqttClient.setCallback(callback);

  // initialise DHT
  sensors.begin();

  pinMode(LED, OUTPUT);

  /*
   * Feather M0 Analog Input Resolution
   * https://forums.adafruit.com/viewtopic.php?f=57&t=103719&p=519042
   */
  analogReadResolution(12);
}

void setupSerialPorts() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  
  //while (!Serial) {
  //  ; // wait for serial port to connect. Needed for native USB port only
  //}
}

void setup_WiFi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void printWifiStatus() {
  Serial.println();
  
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  WiFi.macAddress(mac);
  Serial.print("MAC Address: ");
  Serial.print(mac[5],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.println(mac[0],HEX);
  
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

  Serial.print("WiFi firmware version: ");
  //Serial.println(WiFi.firmwareVersion());

  Serial.println();
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
    
  } else {
    mqttClient.loop();
  }

  collectData();
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    String clientId = "DHTSensor-";  
    clientId += getMacAddress(mac);
    
    if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      mqttClient.subscribe(TOPIC_SENSOR_TEMP_LED);
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

String getMacAddress(byte mac[]) {
  String s;

  for (int i = 5; i >=0; i--) {
    char buf[3];
    sprintf(buf, "%2X", mac[i]);
    s += buf;
  }
  
  return s;
}

void collectData() {
  if ((millis() - lastConnectionTime) > postingInterval) {
    String data = "";
    String deviceStatus = "";

    Serial.print("Data = ");
    data = readData();
    Serial.print(data);
    Serial.println("");

    if (data.length() > 0) {
      sendData(TOPIC_SENSOR_TEMP_DATA, data);
    }

    Serial.print("Status = ");
    //deviceStatus = readStatus();
    //Serial.print(deviceStatus);
    Serial.println("");

    if (deviceStatus.length() > 0) {
      sendData(TOPIC_SENSOR_TEMP_STATUS, deviceStatus);
    }
  }
}

String readData() {
  String data = "";
  
  // Read temperature as Celsius (the default)
  
  sensors.requestTemperatures();
  float t=sensors.getTempCByIndex(0);

  // Check if any reads failed and exit early (to try again).
  if (isnan(t)) {
    Serial.println(F("Failed to read from Temperature sensor!"));
    return data;
  }
  
  
    
  data = "{\"temperature\": %temperatureC}";
  //data.replace("%humidity", String(h));
  data.replace("%temperatureC", String(t));
  //data.replace("%temperatureF", String(f));
  //data.replace("%heat_indexC", String(hic));
  //data.replace("%heat_indexF", String(hif));
    
  return data;
}


void sendData(String topic, String payload) {
  if (payload.length() < 1) {
    Serial.println("No data to send!");
    return;
  }
    
  mqttClient.publish(topic.c_str(), payload.c_str());

  // Note the time that the connection was made
  lastConnectionTime = millis();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  for (int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
  }
  
  Serial.println();

  if (strcmp(topic, TOPIC_SENSOR_TEMP_LED) == 0) {
    int value = atoi((char*) payload);
    Serial.print("Value: ");
    Serial.println(value);

    if (value > 0) {
      digitalWrite(LED, HIGH);

    } else {
      digitalWrite(LED, LOW);
    }
  }
}
