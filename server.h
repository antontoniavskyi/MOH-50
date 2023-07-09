AsyncWebServer server(8080);

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void webServerFunctions(){
  server.on("/mineInfo", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/html", mineInfo);
    Serial.println("=======================================");
    Serial.println("RECEIVED REQUEST");
    Serial.println("=======================================");
  });

  /*server.on("/boom", HTTP_GET, [](AsyncWebServerRequest * request) {
    // GET input1 value on <ESP_IP>/boom?mineID=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessageMineID = request->getParam(PARAM_INPUT_1)->value();
      inputMessageState = request->getParam(PARAM_INPUT_2)->value();
      sendBoomToMine = true;
      request->send(200, "text/plain", "SEND BOOM TO MINE");
    }
    else {
      inputMessageMineID = "No message sent";
      inputMessageState = "No message sent";
      request->send(200, "text/plain", "PLEASE INSERT MINE ID");
    }
  });*/



  server.on("/info", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/html", information);
  });

  server.on("/post", HTTP_POST, [](AsyncWebServerRequest * request) {
    String message;
    if (request->hasParam(PARAM_INPUT_1, true) && request->hasParam(PARAM_INPUT_2, true)) {
      inputMessageMineID = request->getParam(PARAM_INPUT_1, true)->value();
      inputMessageState = request->getParam(PARAM_INPUT_2, true)->value();
      sendBoomToMine = true;
    } else {
      message = "No message sent";
    }
    request->send(200, "text/plain", "Send boom to " + inputMessageMineID);
  });

}
