#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoOTA.h>
#include <WebSocketsServer.h>
#include <WiFiUdp.h>
#include <EEPROM.h>

#include <PubSubClient.h>
#include "BluetoothSerial.h"

#include "secrets.h"

BluetoothSerial SerialBT;

WiFiClient espClient;
PubSubClient mqtt(espClient);

#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

char* host = "EML327gate";

HTTPClient http;

const char* mqttTopicReport = "elm327report/report";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// main look variables
unsigned long currentMillis = 0UL;    // main time-counting var
unsigned long rNextRun = 0UL;         // Next time display should start
unsigned long rInt = 30000UL;         // Interval for reconnect
unsigned long lastSend = 0UL;         // last sent time in millis
String commandToSend = "";            // command to send using ELM327
String commandResponse = "";          // response to expect
bool commandResponsePart = false;     // compare also partial response
int handShakeStep = 0;                // handshake stage
bool lastResponseOK = 0;              // check if last response did match

void setup() {
  // starting debug serial port
  Serial.begin(115200);

  // connect to wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setHostname(host);

  if (String(WiFi.SSID()) != String(ssid)) {
    WiFi.begin(ssid, password);
  }

  int counter = 0;
  while ((WiFi.status() != WL_CONNECTED) & (counter < 30)) {
    delay(250);
    Serial.println("Wifi status: " + String(WiFi.status()));
    counter++;
    yield();
  }

  Serial.println("Wifi status: " + String(WiFi.status()));

  WiFi.setHostname(host);

  // wifi is up, starting OTA, web and websocket servers
  startOTA();
  startServer();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // only for debug - print IP address
  IPAddress ip;
  ip = WiFi.localIP();
  Serial.println("IP address: " + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]));


  // set MQTT server and callback
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(mqttCallback);
  mqtt_reconnect();

  // starting the BT serial to connect to ELM327
  SerialBT.setPin(dongle_pin);
  SerialBT.setTimeout(2000);
  SerialBT.begin("esp32HUD", true);

  if (SerialBT.isClosed() || !SerialBT.connect(dongle_name))
  {
    Serial.println("Couldn't connect to OBD scanner");
    webSocket.broadcastTXT("Couldn't connect to OBD scanner");
    mqtt.publish(mqttTopicReport, "Couldn't connect to OBD scanner");
  } else {
    Serial.println("Connected to ELM327");
    webSocket.broadcastTXT("Connected to ELM327");
    mqtt.publish(mqttTopicReport, "Connected to OBD");
  }

  Serial.println("Setup done OK");
}

void loop() {
  currentMillis = millis();

  if ((SerialBT.available() > 0) && (currentMillis - lastSend > 250)) { //150 before
    checkResponse();
  }

  if (commandToSend.length() > 0) sendCommand();

  if ((currentMillis - lastSend > 150) & (lastResponseOK) & (handShakeStep > 0)) { //50 before
    lastResponseOK = 0;
    handShake();
    handShakeStep++;
  }

  if (currentMillis > rNextRun) {
    Serial.println("Reporting");

    // first, check wifi
    if (WiFi.status() != WL_CONNECTED) {
      // we are disconnected, let's reconnect.
      WiFi.reconnect();
    }
    if (!mqtt.connected()) mqtt_reconnect();

    if (!SerialBT.isClosed() && SerialBT.connected(1)) {
      webSocket.broadcastTXT("Reporting - connected");
      mqtt.publish("elm327report/heartbeat", "OBD connected");
      
      // running the queries to ELM
      handShakeStep = 1;
      lastResponseOK = 1;
      
    } else {
      webSocket.broadcastTXT("Reporting - NOT connected :(");
      mqtt.publish("elm327report/heartbeat", "OBD disconnected");
      reconnectBT();
    }
    rNextRun = currentMillis + rInt;
  }

  // only verify internet services if connected
  ArduinoOTA.handle();
  server.handleClient();
  webSocket.loop();
  yield();
}
