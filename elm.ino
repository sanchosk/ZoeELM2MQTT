// send command to SerialBT
// set the "awaiting response" to true
// set the next step
void sendCommand() {
  if (!SerialBT.connected(1)) {
    webSocket.broadcastTXT("BT dongle not connected, unable to send");
    mqtt.publish("elm327report/status", "BT dongle not connected, unable to send");
    reconnectBT();
  }

  webSocket.broadcastTXT("ELM Sending command: " + commandToSend);
  snprintf (msg, MSG_BUFFER_SIZE, "%s", commandToSend);
  mqtt.publish("elm327report/debug/elmSend", msg);

  SerialBT.println(commandToSend);
  commandToSend = "";
  lastSend = millis();
  rNextRun = currentMillis + rInt;
}

void checkResponse() {
  String elmResponse = "";
  if (SerialBT.available()) {
    while (SerialBT.available()) {
      char c = SerialBT.read();
      if (c > 13) elmResponse += char(c);
    }
  }

  webSocket.broadcastTXT("ELM serialBT: " + elmResponse);
  snprintf (msg, MSG_BUFFER_SIZE, "%s", elmResponse);
  mqtt.publish("elm327report/debug/elmResponse", msg);

  if (commandResponsePart) {
    // indexOf works only for characters, so I need to make a temp string
    String matchResponse = elmResponse;
    matchResponse.replace(commandResponse, "");
    if (elmResponse.length() > matchResponse.length()) {
      webSocket.broadcastTXT("Response OK");
      mqtt.publish("elm327report/debug/elmResponseEval", "Response OK");
      lastResponseOK = 1;
    } else {
      webSocket.broadcastTXT("Response NOT OK");
      mqtt.publish("elm327report/debug/elmResponseEval", "Response NOT OK");
      lastResponseOK = 0;
    }
  } else {
    if (elmResponse.equalsIgnoreCase(commandResponse)) {
      webSocket.broadcastTXT("Response OK");
      mqtt.publish("elm327report/debug/elmResponseEval", "Response OK");
      lastResponseOK = 1;
    } else {
      webSocket.broadcastTXT("Response NOT OK");
      mqtt.publish("elm327report/debug/elmResponseEval", "Response NOT OK");
      lastResponseOK = 0;
    }
  }

  if ((handShakeStep == 20) && (lastResponseOK == 1)) {
    // we have SoC response, let's process it
    // 056220020FF6AAAA - 05 622002 - header, AAAA - footer
    // 0FF6 - value in HEX, to be multiplied by 0.02 for %SoC
    String SoC = elmResponse;
    SoC.replace("05622002", "");
    SoC = SoC.substring(0, 4);
    
    unsigned long SoCl = 0;
    char SoCc[5];
    SoC.toCharArray(SoCc, 5);
    SoCl = strtol(SoCc, NULL, 16);
    float SoCf = 0.02 * SoCl;
        
    webSocket.broadcastTXT("SoC: " + String(SoCf, 2));
    snprintf (msg, MSG_BUFFER_SIZE, "%3.2f", SoCf);
    mqtt_reconnect();
    mqtt.publish("elm327report/result/SoC", msg, true);

    // report successful, continue with next one
    
    handShakeStep = 101;
    lastResponseOK = 1;

  }

  if ((handShakeStep == 220) && (lastResponseOK == 1)) {
    // we have SoC response, let's process it
    // 0562320C0DC2AAAA - 05 62320C - header, AAAA - footer
    // 0DC2 - value in HEX, to be multiplied by 0.005 for Available Energy in kWh
    String AvailableEnergy = elmResponse;
    AvailableEnergy.replace("0562320C", "");
    AvailableEnergy = AvailableEnergy.substring(0, 4);
    
    unsigned long AvailableEnergyl = 0;
    char AvailableEnergyc[5];
    AvailableEnergy.toCharArray(AvailableEnergyc, 5);
    AvailableEnergyl = strtol(AvailableEnergyc, NULL, 16);
    float AvailableEnergyf = 0.005 * AvailableEnergyl;
        
    webSocket.broadcastTXT("AvailableEnergy: " + String(AvailableEnergyf, 2));
    mqtt_reconnect();
    snprintf (msg, MSG_BUFFER_SIZE, "%3.2f", AvailableEnergyf);
    mqtt.publish("elm327report/result/AvailableEnergy", msg, true);

  }

  if ((handShakeStep == 120) && (lastResponseOK == 1)) {
    // we have SoC response, let's process it
    // 056234510C0DC2AAAA - 05 623451 - header, AAAA - footer
    // 0DC2 - value in HEX, to be multiplied by 1 for Available Energy in kWh
    String AvailableRange = elmResponse;
    AvailableRange.replace("05623451", "");
    AvailableRange = AvailableRange.substring(0, 4);
    
    unsigned long AvailableRangel = 0;
    char AvailableRangec[5];
    AvailableRange.toCharArray(AvailableRangec, 5);
    AvailableRangel = strtol(AvailableRangec, NULL, 16);

    if (AvailableRangel < 300) {
      webSocket.broadcastTXT("AvailableRange: " + String(AvailableRangel));
      mqtt_reconnect();
      snprintf (msg, MSG_BUFFER_SIZE, "%d", AvailableRangel);
      mqtt.publish("elm327report/result/AvailableRange", msg, true);
    } else {
      webSocket.broadcastTXT("AvailableRange over limit: " + String(AvailableRangel));
      mqtt_reconnect();
      snprintf (msg, MSG_BUFFER_SIZE, "%d", AvailableRangel);
      mqtt.publish("elm327report/debug/AvailableRangeError", msg);
    }

    // report successful, continue with next one
    
    handShakeStep = 201;
    lastResponseOK = 1;

  }
}

void reconnectBT() {
  Serial.println("Attempt to reconnect ELM");
  webSocket.broadcastTXT("Attempt to reconnect ELM");
  mqtt.publish("elm327report/status", "Attempt to reconnect ELM");
  if (!SerialBT.connect(dongle_name))
  {
    Serial.println("Couldn't reconnect to OBD");
    webSocket.broadcastTXT("Couldn't reconnect to OBD");
    mqtt.publish("elm327report/status", "Couldn't reconnect to OBD");
  }
  if (!SerialBT.isClosed() && SerialBT.connected(1)) {
    webSocket.broadcastTXT("BT dongle reconnect successful");
    mqtt.publish("elm327report/status", "BT dongle reconnected");
  }
  commandToSend = "";
  commandResponse = "";
  handShakeStep = 0;
}



void handShake() {
  switch (handShakeStep) {
    case 1:
      commandToSend = "atws";
      commandResponse = "ELM327";
      commandResponsePart = true;
      break;
    case 2:
      commandToSend = "ate0";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 3:
      commandToSend = "ats0";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 4:
      commandToSend = "ath0";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 5:
      commandToSend = "atl0";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 6:
      commandToSend = "atal";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    // KarelSvo/CanZE
    case 7:
      commandToSend = "atcaf0";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 8:
      commandToSend = "atcfc1";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 9:
      commandToSend = "AT SH 7E4";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 10:
      commandToSend = "AT FC SH 7E4";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 11:
      commandToSend = "AT FC SD 30 00 00";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 12:
      commandToSend = "AT FC SM 1";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 13:
      commandToSend = "AT ST FF";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 14:
      commandToSend = "AT AT 0";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 15:
      commandToSend = "AT SP 6";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 16:
      commandToSend = "AT AT 1";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 17:
      commandToSend = "AT CRA 7EC";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 18:
      commandToSend = "AT FC SH 7e4";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 19:
      // this is the actual code for SoC
      commandToSend = "03 222002";
      commandResponse = "05622002";
      commandResponsePart = true;
      break;

    // restart sequence for available power

    
    case 201:
      commandToSend = "atws";
      commandResponse = ">";
      commandResponsePart = true;
      break;
    case 202:
      commandToSend = "ate0";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 203:
      commandToSend = "ats0";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 204:
      commandToSend = "ath0";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 205:
      commandToSend = "atl0";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 206:
      commandToSend = "atal";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    // KarelSvo/CanZE
    case 207:
      commandToSend = "atcaf0";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 208:
      commandToSend = "atcfc1";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 209:
      commandToSend = "AT SH 7E4";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 210:
      commandToSend = "AT FC SH 7E4";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 211:
      commandToSend = "AT FC SD 30 00 00";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 212:
      commandToSend = "AT FC SM 1";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 213:
      commandToSend = "AT ST FF";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 214:
      commandToSend = "AT AT 0";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 215:
      commandToSend = "AT SP 6";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 216:
      commandToSend = "AT AT 1";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 217:
      commandToSend = "AT CRA 7EC";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 218:
      commandToSend = "AT FC SH 7e4";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 219:
      // this is the actual code for Available Power in kWh
      commandToSend = "03 22320C";
      commandResponse = "0562320C";
      commandResponsePart = true;
      break;

      

    // restart sequence for range estimate

    
    case 101:
      commandToSend = "atws";
      commandResponse = ">";
      commandResponsePart = true;
      break;
    case 102:
      commandToSend = "ate0";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 103:
      commandToSend = "ats0";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 104:
      commandToSend = "ath0";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 105:
      commandToSend = "atl0";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 106:
      commandToSend = "atal";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    // KarelSvo/CanZE
    case 107:
      commandToSend = "atcaf0";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 108:
      commandToSend = "atcfc1";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 109:
      commandToSend = "AT SH 7E4";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 110:
      commandToSend = "AT FC SH 7E4";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 111:
      commandToSend = "AT FC SD 30 00 00";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 112:
      commandToSend = "AT FC SM 1";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 113:
      commandToSend = "AT ST FF";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 114:
      commandToSend = "AT AT 0";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 115:
      commandToSend = "AT SP 6";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 116:
      commandToSend = "AT AT 1";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 117:
      commandToSend = "AT CRA 7EC";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 118:
      commandToSend = "AT FC SH 7e4";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 119:
      // this is the actual code for Available range in km
      commandToSend = "03 223451";
      commandResponse = "05623451";
      commandResponsePart = true;
      break;

  }
}
