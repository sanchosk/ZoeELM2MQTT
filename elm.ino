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
      mqtt.publish("elm327report/result/AvailableRange", msg, false);
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
    mqtt.publish("elm327report/result/AvailableEnergy", msg, false);

    // report successful, continue with next one
    
    handShakeStep = 401;
    lastResponseOK = 1;

  }

  if ((handShakeStep == 319) && (lastResponseOK == 1)) {
    // we have SoC response, let's process it
    // new code: 624B9B
    // 032220060662200601A497AA - 03 222006 - header, AA - footer
    // 01A497 - value in HEX for Odometer status
    // data for CAN responses from CanZE project in 
    // https://github.com/fesch/CanZE/tree/master/app/src/main/assets/ZOE
    // example data captured using canZE
    // again big credit to KarelSvo on github
    
    String odometer = elmResponse;
    odometer.replace("03222006", "");
    odometer = odometer.substring(8, 14);
    
    unsigned long odometerl = 0;
    char odometerc[7];
    odometer.toCharArray(odometerc, 7);
    odometerl = strtol(odometerc, NULL, 16);
        
    webSocket.broadcastTXT("Odometer: " + String(odometerl));
    mqtt_reconnect();
    snprintf (msg, MSG_BUFFER_SIZE, "%d", odometerl);
    mqtt.publish("elm327report/result/Odometer", msg, false);

    // report successful, continue with next one 
    // nothing to continue with
    handShakeStep = 301;
    lastResponseOK = 1;

  }

  if ((handShakeStep == 420) && (lastResponseOK == 1)) {
    // charging cable connected
    // response: 22339D0462339D00AAAAAA - no cable
    // response: 0462339D01AAAAAA - with cable
    // Take last 8 chars, remove AAAAAAA, if it includes 01 - we have cable, otherwise not
    // https://github.com/fesch/CanZE/tree/master/app/src/main/assets/ZOE
    // again big credit to KarelSvo on github
    
    String cablePresence = elmResponse;
    
    unsigned long cablePresenceL = 2;
    if (cablePresence.indexOf("01AAAA") > 0) cablePresenceL = 1;
    if (cablePresence.indexOf("00AAAA") > 0) cablePresenceL = 0;

    mqtt_reconnect();
    snprintf (msg, MSG_BUFFER_SIZE, "%d", cablePresenceL);
    mqtt.publish("elm327report/result/CablePresent", msg, false);

    // report successful, continue with next one 
//    nothing to continue with
//    handShakeStep = 301;
//    lastResponseOK = 1;

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
      commandToSend = "AT FC SH 7E4";
      commandResponse = "OK>";
      commandResponsePart = false;
      break;
    case 119:
      // this is the actual code for Available range in km
      commandToSend = "03 223451";
      commandResponse = "05623451";
      commandResponsePart = true;
      break;
      

      

    // restart sequence for odometer
    // huge thanks to GitHub user KarelSvo
    
    case 301:
      commandToSend = "atws";
      commandResponse = ">";
      commandResponsePart = true;
      break;
    case 302:
      commandToSend = "ate1";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 303:
      commandToSend = "ats0";
      commandResponse = "ats0OK>";
      commandResponsePart = false;
      break;
    case 304:
      commandToSend = "ath0";
      commandResponse = "ath0OK>";
      commandResponsePart = false;
      break;
    case 305:
      commandToSend = "atl0";
      commandResponse = "atl0OK>";
      commandResponsePart = false;
      break;
    case 306:
      commandToSend = "atcaf0";
      commandResponse = "atcaf0OK>";
      commandResponsePart = false;
      break;
    // KarelSvo/CanZE
    case 307:
      commandToSend = "atcfc1";
      commandResponse = "atcfc1OK>";
      commandResponsePart = false;
      break;
    case 308:
      commandToSend = "AT SH 7E4";
      commandResponse = "AT SH 7E4OK>";
      commandResponsePart = false;
      break;
    case 309:
      commandToSend = "AT FC SH 7E4";
      commandResponse = "AT FC SH 7E4OK>";
      commandResponsePart = false;
      break;
    case 310:
      commandToSend = "AT FC SD 30 00 00";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 311:
      commandToSend = "AT FC SM 1";
      commandResponse = "AT FC SM 1OK>";
      commandResponsePart = false;
      break;
    case 312:
      commandToSend = "atsp6";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 313:
      commandToSend = "AT ST FF";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 314:
      commandToSend = "AT AT 0";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 315:
      commandToSend = "AT SP 6";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 316:
      commandToSend = " AT AT 1";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 317:
      commandToSend = " AT CRA 7EC";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 318:
      commandToSend = "03222006";
      commandResponse = "03222006";
      commandResponsePart = true;
      break;


    // restart sequence for plug connected
    // huge thanks to GitHub user KarelSvo
    
    case 401:
      commandToSend = "atws";
      commandResponse = ">";
      commandResponsePart = true;
      break;
    case 402:
      commandToSend = "ate1";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 403:
      commandToSend = "ats0";
      commandResponse = "ats0OK>";
      commandResponsePart = false;
      break;
    case 404:
      commandToSend = "ath0";
      commandResponse = "ath0OK>";
      commandResponsePart = false;
      break;
    case 405:
      commandToSend = "atl0";
      commandResponse = "atl0OK>";
      commandResponsePart = false;
      break;
    case 406:
      commandToSend = "atcaf0";
      commandResponse = "atcaf0OK>";
      commandResponsePart = false;
      break;
    // KarelSvo/CanZE
    case 407:
      commandToSend = "atcfc1";
      commandResponse = "atcfc1OK>";
      commandResponsePart = false;
      break;
    case 408:
      commandToSend = "AT SH 7E4";
      commandResponse = "AT SH 7E4OK>";
      commandResponsePart = false;
      break;
    case 409:
      commandToSend = "AT FC SH 7E4";
      commandResponse = "AT FC SH 7E4OK>";
      commandResponsePart = false;
      break;
    case 410:
      commandToSend = "AT FC SD 30 00 00";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 411:
      commandToSend = "AT FC SM 1";
      commandResponse = "AT FC SM 1OK>";
      commandResponsePart = false;
      break;
    case 412:
      commandToSend = "atsp6";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 413:
      commandToSend = "AT ST FF";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 414:
      commandToSend = "AT AT 0";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 415:
      commandToSend = "AT SP 6";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 416:
      commandToSend = " AT AT 0";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 417:
      commandToSend = " AT CRA 7EC";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 418:
      commandToSend = " AT FC SH 7E4";
      commandResponse = "OK>";
      commandResponsePart = true;
      break;
    case 419:
      commandToSend = "0322339D";
      commandResponse = "0462339D";
      commandResponsePart = true;
      break;

  }
}
