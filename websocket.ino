void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch (type) {
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        // send message to client
        webSocket.broadcastTXT("New connection from " + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]));
      }
      break;
    case WStype_TEXT:
      if (payload[0] == 'p') {
        webSocket.broadcastTXT("__ping__");
      } else if (payload[0] == 'r') {
        webSocket.broadcastTXT("Reset");
        ESP.restart();
      } else if (payload[0] == 'h') {
        webSocket.broadcastTXT("Help for ESP32 ELM MQTT translator\n"
                               " r - reset\n"
                               " d - display settings\n"
                               " c - send AT command\n"
                               " R - Response to expect\n"
                               " T - Response must be exactt\n"
                               " H - handshake\n"
                               );

      } else if (payload[0] == 'd') {
        webSocket.broadcastTXT("Wifi SSID: " + String(WiFi.SSID()));
        
        IPAddress ip;
        ip = WiFi.localIP();
        webSocket.broadcastTXT("IP address: " + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]));
        webSocket.broadcastTXT("MQTT: " + String(mqtt.connected()));
        webSocket.broadcastTXT("ELM: " + String(SerialBT.connected(1)));

      } else if (payload[0] == 'T') {
        commandResponsePart = !commandResponsePart;
        webSocket.broadcastTXT("Exact response toggle, new value " + String(commandResponsePart));
        handShake();
        
      } else if (payload[0] == 'H') {
        webSocket.broadcastTXT("Setting handshake step to 1");
        handShakeStep = 1;
        lastResponseOK = 1;
        
      } else if (payload[0] == 'I') {
        webSocket.broadcastTXT("Setting handshake step to 21");
        handShakeStep = 201;
        lastResponseOK = 1;
        
      } else if (payload[0] == 'J') {
        webSocket.broadcastTXT("Setting handshake step to 101");
        handShakeStep = 101;
        lastResponseOK = 1;
        
      } else if (payload[0] == 'c') {
        webSocket.broadcastTXT("Publishing to serial BT: ");
        webSocket.broadcastTXT(payload);
        // wiriting the AT command to the BT serial char-by-char, adding line end, too
        for (int i = 1; i < length; i++) {
          SerialBT.write(payload[i]);
        }
        SerialBT.write('\r');
        SerialBT.write('\n');

      } else if (payload[0] == 'R') {
        webSocket.broadcastTXT("Setting expected value from serial to : ");
        webSocket.broadcastTXT(payload);
        // wiriting the AT command to the BT serial char-by-char, adding line end, too
        commandResponse = (char*)payload;
        commandResponse = commandResponse.substring(1, 99);
        
      } else if (payload[0] == 'o') {
        webSocket.broadcastTXT("Attempt to convert: ");
        webSocket.broadcastTXT(payload);
        // wiriting the AT command to the BT serial char-by-char, adding line end, too
        unsigned long SoCl = 0;
        String SoC = (char*)payload;
        SoC = SoC.substring(1, 99);
        
        char SoCc[5];
        SoC.toCharArray(SoCc, 5);
        SoCl = strtol(SoCc, NULL, 16);
        
        float SoCf = 0.02 * SoCl;
            
        webSocket.broadcastTXT("SoC: " + String(SoCf, 2));
        snprintf (msg, MSG_BUFFER_SIZE, "%3.2f", SoCf);
        mqtt_reconnect();
        mqtt.publish("elm327report/soc", msg);        

      } else if (payload[0] != 'p') webSocket.broadcastTXT(payload);
      break;
      
    default:
      break;
  }
  yield();
}
