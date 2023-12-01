void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String sTopic = "";
  String sPayload = "";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    sPayload += payload[i];
  }
  Serial.println();
  webSocket.broadcastTXT("MQTT input topic: " + String(topic));
  webSocket.broadcastTXT("MQTT input payload: " + sPayload);
}


void mqtt_reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqtt.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqtt.publish("elm327report/startup", "ELM327 esp32 started");
      // ... and resubscribe
      mqtt.subscribe("elm327report/input");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 1 seconds");
      // Wait 1 second before retrying
      delay(1000);
    }
  }
}
