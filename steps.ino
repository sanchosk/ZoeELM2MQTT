const char* jsonArray = "[  "
"  { "
"    \"topic\" : \"elm327report/result/SoC\", "
"    \"data\" : [ "
"      { \"stepID\" : 1, \"send\" : \"atws\", \"response\" : \"ELM327\", \"part\" : 1 }, "
"      { \"stepID\" : 2, \"send\" : \"ate0\", \"response\" : \"OK>\", \"part\" : 1 }, "
"      { \"stepID\" : 3, \"send\" : \"ats0\", \"response\" : \"OK>\", \"part\" : 0 }, "
"      { \"stepID\" : 4, \"send\" : \"ath0\", \"response\" : \"OK>\", \"part\" : 0 }, "
"      { \"stepID\" : 5, \"send\" : \"atl0\", \"response\" : \"OK>\", \"part\" : 0 }, "
"      { \"stepID\" : 6, \"send\" : \"atal\", \"response\" : \"OK>\", \"part\" : 0 }, "
"      { \"stepID\" : 7, \"send\" : \"atcaf0\", \"response\" : \"OK>\", \"part\" : 0 }, "
"      { \"stepID\" : 8, \"send\" : \"atcfc1\", \"response\" : \"OK>\", \"part\" : 0 }, "
"      { \"stepID\" : 9, \"send\" : \"AT SH 7E4\", \"response\" : \"OK>\", \"part\" : 0 }, "
"      { \"stepID\" : 10, \"send\" : \"AT FC SH 7E4\", \"response\" : \"OK>\", \"part\" : 0 }, "
"      { \"stepID\" : 11, \"send\" : \"AT FC SD 30 00 00\", \"response\" : \"OK>\", \"part\" : 0 }, "
"      { \"stepID\" : 12, \"send\" : \"AT FC SM 1\", \"response\" : \"OK>\", \"part\" : 0 }, "
"      { \"stepID\" : 13, \"send\" : \"AT ST FF\", \"response\" : \"OK>\", \"part\" : 0 }, "
"      { \"stepID\" : 14, \"send\" : \"AT AT 0\", \"response\" : \"OK>\", \"part\" : 0 }, "
"      { \"stepID\" : 15, \"send\" : \"AT SP 6\", \"response\" : \"OK>\", \"part\" : 0 }, "
"      { \"stepID\" : 16, \"send\" : \"AT AT 1\", \"response\" : \"OK>\", \"part\" : 0 }, "
"      { \"stepID\" : 17, \"send\" : \"AT CRA 7EC\", \"response\" : \"OK>\", \"part\" : 0 }, "
"      { \"stepID\" : 18, \"send\" : \"AT FC SH 7e4\", \"response\" : \"OK>\", \"part\" : 0 }, "
"      { \"stepID\" : 19, \"send\" : \"03 222002\", \"response\" : \"05622002\", \"part\" : 1 } "
"    ] "
"  } "
"]";

String getJson() {
  JsonDocument jsonDoc;
  deserializeJson(jsonDoc, jsonArray);

  for (JsonObject rootObject : jsonDoc.as<JsonArray>()) {
    String topic = rootObject["topic"];
    webSocket.broadcastTXT("Topic: " + String(topic));
    for (JsonObject dataObject : rootObject["data"].as<JsonArray>()) {
      String toSend = dataObject["send"];
      String toReceive = dataObject["response"];
      int partial = dataObject["part"];
      webSocket.broadcastTXT("to send: " + String(toSend));
      webSocket.broadcastTXT("to receive: " + String(toReceive));
      webSocket.broadcastTXT("partial OK: " + String(partial));
    }
  }

  return("Success");
};