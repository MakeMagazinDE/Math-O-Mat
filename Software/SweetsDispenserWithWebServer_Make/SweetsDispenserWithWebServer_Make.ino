// ---------------------------------------------------------------------------------------
//
// Code for Math-o-Mat (M&Ms dispenser). The code generates a Webpage with Websocket connection. 
// Random numbers are generated for a Math-quiz. Correct answers will enable the stepper-motor to dispense M&Ms from the storage.
// The code, written by Bernd Heisterkamp is based on the esp-websocket tutorial written by mo thunderz (https://www.youtube.com/watch?v=15X0WvGaVg8)
//
// For installation, the following libraries need to be installed:
// Websockets by Markus Sattler (can be tricky to find -> search for "Arduino Websockets"
// Stepper motor driver library A4988.h -> https://github.com/laurb9/StepperDriver/blob/master/src/A4988.h
// Arduino Json Library ArduinoJson.h -> Search libraries for ArduinoJson by Benoit Blanchon
// Library to connect to multiple Wifi-networks WifiMulti.h -> https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFiMulti.h

//
// ---------------------------------------------------------------------------------------

#include <WiFi.h>                 // needed to connect to WiFi
#include <WiFiMulti.h>            // allows definition of multiple wifi networks

#include <WebServer.h>            // needed to create a simple webserver
#include <WebSocketsServer.h>     // needed for instant communication between client and server through Websockets
#include <ArduinoJson.h>          // needed to convert and de-convert data into json format for easy web-socket communication
#include "A4988.h"                // needed to smoothly control the stepper motor with acceleration and decelleration

#define MOTOR_STEPS 400           // Defines the full-steps per resolution for the stepper-motor
#define MOTOR_ACCEL 2000          // Acceleration and deceleration values are always in FULL steps / s^2
#define MOTOR_DECEL 2000
#define MICROSTEPS 16             // Microstepping mode. 

// The following definitions define the IO-ports to control the stepper-driver
#define DIR 2                     // Stepper motor direction
#define STEP 4                    // Stepper motor stepping pulse
#define ENABLE 17                 // Pin to enable stepper-motor, low = active (!)
#define MS1 14                    // Microstepping mode switch 1
#define MS2 16                    // Microstepping mode switch 2
#define MS3 27                    // Microstepping mode switch 3

#define STEPS_EINELINSE -1281     // Steps to dispense a single M&M

A4988 stepper(MOTOR_STEPS, DIR, STEP, ENABLE, MS1, MS2, MS3);
int RPM = 150;              // Target RPM for cruise speed
bool bolNewGame = false;    // 
WiFiMulti wifiMulti;

// The String below "webpage" contains the complete HTML code that is sent to the client whenever someone connects to the webserver.
// It has been compressed using https://www.textfixer.com/html/compress-html-compression.php
// Make sure the HTML-code does not contain any double-quotes
String webpage = "<p>&nbsp;</p><p>&nbsp;</p><h1>Train your Brain !</h1><table><thead><tr><td><h4>No1</h4></td><td><h4>...</h4></td><td><h4>&nbsp;No2</h4></td><td><h4>=</h4></td><td><h4></h4></td><td><h4>Result</h4></td></tr></thead><tbody><tr><td style='text-align: center;'><p><strong><span id='sum1'>-</span></strong></p></td><td style='text-align: center;'><p><strong>+</strong></p></td><td style='text-align: center;'><p><strong><span id='sum2'>-</span></strong></p></td><td style='text-align: center;'><p><strong>=</strong></p></td><td><td><input type='number' name='lsgAdd' id='lsgAdd' max='9999' min='-9999' step='1'></td></td></tr><tr><td style='text-align: center;'><p><strong><span id='sub1'>-</span></strong></p></td><td style='text-align: center;'><p><strong>-</strong></p></td><td style='text-align: center;'><p><strong><span id='sub2'>-</span></strong></p></td><td style='text-align: center;'><p><strong>=</strong></p></td><td><td><input type='number' name='lsgSub' id='lsgSub' max='9999' min='-9999' step='1'></td></td></tr><tr><td style='text-align: center;'><p><strong><span id='prod1'>-</span></strong></p></td><td style='text-align: center;'><p><strong>*</strong></p></td><td style='text-align: center;'><p><strong><span id='prod2'>-</span></strong></p></td><td style='text-align: center;'><p><strong>=</strong></p></td><td><td><input type='number' name='lsgProd' id='lsgProd' max='9999' min='-9999' step='1'></td></td></tr><tr><td style='text-align: center;'><p><strong><span id='div1'>-</span></strong></p></td><td style='text-align: center;'><p><strong>/</strong></p></td><td style='text-align: center;'><p><strong><span id='div2'>-</span></strong></p></td><td style='text-align: center;'><p><strong>=</strong></p></td><td><td><input type='number' name='lsgDiv' id='lsgDiv' max='9999' min='-9999' step='1'></td></td></tr></tbody></table><p><button id='BTN_CHECK' type='button'>Check result </button></p><p><button id='BTN_SEND_BACK' type='button' hidden>Continue </button></p><script> var Socket; var obj; var CorrectAnswers; document.getElementById('BTN_CHECK').addEventListener('click', button_check_result); document.getElementById('BTN_SEND_BACK').addEventListener('click', button_send_back); function init() { Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); Socket.onmessage = function(event) { processCommand(event); }; } function button_check_result() {CorrectAnswers = 0;document.getElementById('lsgAdd').setAttribute('disabled', '');document.getElementById('lsgSub').setAttribute('disabled', ''); document.getElementById('lsgProd').setAttribute('disabled', '');document.getElementById('lsgDiv').setAttribute('disabled', '');if (document.getElementById('lsgAdd').value == obj.LSGadd){CorrectAnswers += 1;document.getElementById('lsgAdd').style.color = 'green';} else {document.getElementById('lsgAdd').style.color = 'red';}if (document.getElementById('lsgSub').value == obj.LSGsub){CorrectAnswers += 1;document.getElementById('lsgSub').style.color = 'green';} else {document.getElementById('lsgSub').style.color = 'red';}if (document.getElementById('lsgProd').value == obj.LSGprod){CorrectAnswers += 1;document.getElementById('lsgProd').style.color = 'green';} else {document.getElementById('lsgProd').style.color = 'red';}if (document.getElementById('lsgDiv').value == obj.LSGdiv){CorrectAnswers += 1;document.getElementById('lsgDiv').style.color = 'green';} else {document.getElementById('lsgDiv').style.color = 'red';} console.log(CorrectAnswers);document.getElementById('BTN_CHECK').setAttribute('hidden', 'hidden');document.getElementById('BTN_SEND_BACK').removeAttribute('hidden'); } function button_send_back() { var loesungen = {count: CorrectAnswers,};ResetCounter = 0;document.getElementById('lsgAdd').value = '';document.getElementById('lsgSub').value = '';document.getElementById('lsgProd').value = '';document.getElementById('lsgDiv').value = '';document.getElementById('lsgAdd').style.color = 'black';document.getElementById('lsgSub').style.color = 'black';document.getElementById('lsgProd').style.color = 'black';document.getElementById('lsgDiv').style.color = 'black'; console.log(JSON.stringify(loesungen)); Socket.send(JSON.stringify(loesungen));document.getElementById('BTN_CHECK').removeAttribute('hidden');document.getElementById('BTN_SEND_BACK').setAttribute('hidden', 'hidden'); } function processCommand(event) {obj = JSON.parse(event.data); document.getElementById('sum1').innerHTML = obj.sum1; document.getElementById('sum2').innerHTML = obj.sum2; document.getElementById('sub1').innerHTML = obj.sub1; document.getElementById('sub2').innerHTML = obj.sub2; document.getElementById('prod1').innerHTML = obj.prod1; document.getElementById('prod2').innerHTML = obj.prod2; document.getElementById('div1').innerHTML = obj.div1; document.getElementById('div2').innerHTML = obj.div2;document.getElementById('lsgAdd').removeAttribute('disabled', '');document.getElementById('lsgSub').removeAttribute('disabled', ''); document.getElementById('lsgProd').removeAttribute('disabled', '');document.getElementById('lsgDiv').removeAttribute('disabled', ''); console.log(obj.sum1); console.log(obj.sum2); } window.onload = function(event) { init(); }</script></html>";

StaticJsonDocument<250> doc_tx;                       // Data send to WebPage
StaticJsonDocument<250> doc_rx;                       // Data received from WebPage

// Initialization of webserver and websocket
WebServer server(80);                                 // the server uses port 80 (standard port for webpages
WebSocketsServer webSocket = WebSocketsServer(81);    // the websocket uses port 81 (standard port for websockets

void setup() {
  Serial.begin(115200);                               // init serial port for debugging           
  unsigned long currentMillis = millis(); // call millis  and Get snapshot of time
  stepper.begin(RPM, MICROSTEPS);
  // Activate enable-pin on stepper-driver
  stepper.setEnableActiveState(LOW);
  /*
   * Set LINEAR_SPEED (accelerated) profile.
   */
  stepper.setSpeedProfile(stepper.LINEAR_SPEED, MOTOR_ACCEL, MOTOR_DECEL);
  stepper.disable();                                  // Disable stepper, it is only activated when dispensing M&Ms
  randomSeed(analogRead(0));                          // Assure truely random numbers
 
  // Add a line for each Wifi-network you'd like to connect to (e.g. Home, Fablab, Mobile-phone hotspot, etc.)
  wifiMulti.addAP("SSID-Network1", "Password1");  
  wifiMulti.addAP("SSID-Network2", "Password2");  
  wifiMulti.addAP("SSID-Network3", "Password3");  
  wifiMulti.addAP("SSID-Network4", "Password4");  
  Serial.println("Establishing connection to WiFi ");           // print SSID to the serial interface for debugging

  while (wifiMulti.run() != WL_CONNECTED) {             // wait until WiFi is connected
    delay(1000);
    Serial.print(".");
  }
  Serial.print("Connected to network with IP address: ");
  Serial.println(WiFi.localIP());                     // show IP address that the ESP32 has received from router
  
  server.on("/", []() {                               // define here what the webserver needs to do
    server.send(200, "text\html", webpage);           //    -> it needs to send out the HTML string "webpage" to the client
  });
  server.begin();                                     // start server
  webSocket.begin();                                  // start websocket
  webSocket.onEvent(webSocketEvent);                  // define a callback function 
                                                      // -> what does the ESP32 need to do when an event from the websocket is received? 
                                                      // -> run function "webSocketEvent()"
}

void loop() {
  server.handleClient();                              // Needed for the webserver to handle all clients
  if (bolNewGame == true) {
    newNumbers();
  }
  webSocket.loop();                                   // Update function for the webSockets 
}

void TurnWheel(int count){
  Serial.println("Correct Answers = " + String(count));
  stepper.enable();
  if (count == 4) {
    // With 4 correct answers dispens 2 M&Ms
    stepper.rotate(2 * STEPS_EINELINSE);
  } else if (count == 3) {
    // With 3 correct answers dispens 1 M&M
    stepper.rotate(1 * STEPS_EINELINSE);
  } 
  else {
    // Mocking the user with a short left-right rotation
    stepper.rotate(-0.5 * STEPS_EINELINSE);
    stepper.rotate(0.5 * STEPS_EINELINSE);
  }
  stepper.disable(); // Disable the stepper to save power.
}

void newNumbers(){
  bolNewGame = false;
  JsonObject object = doc_tx.to<JsonObject>(); //Define a JSON-object which is later filled with the new quiz-numbers
  String jsonString = "";

  // Generate random numbers for Math-quiz calculations and calculate correct solutions
  // Change random upper and lower range to make calculations more or less difficult
  long div1 = random(30,70);
  long div2 = div1;
  while (div1/div2 < 3){ 
    div2 = random(4,50);
  }
  div1 = div1 - (div1 % div2); 
  long sum1 = random(-100,100);   
  long sum2 = random(-100,100);
  long sub1 = random(-100,100);
  long sub2 = random(-100,100);
  long prod1 = random(3,20);
  long prod2 = random(3,50);
  long LSGadd = sum1 + sum2;
  long LSGsub = sub1 - sub2;
  long LSGprod = prod1 * prod2;
  long LSGdiv = div1 / div2;
  // Add generated numbers and solutions to JSON-object
  object ["sum1"] = sum1;
  object ["sum2"] = sum2;
  object ["LSGadd"] = LSGadd;
  object ["sub1"] = sub1;
  object ["sub2"] = sub2;
  object ["LSGsub"] = LSGsub;
  object ["prod1"] = prod1;
  object ["prod2"] = prod2;
  object ["LSGprod"] = LSGprod;
  object ["div1"] = div1;
  object ["div2"] = div2;
  object ["LSGdiv"] = LSGdiv;
  serializeJson(doc_tx, jsonString);
  Serial.println(jsonString);
  webSocket.broadcastTXT(jsonString); //Transfer numbers to web-page

}
void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {      // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
  switch (type) {                                     // switch on the type of information sent
    case WStype_DISCONNECTED:                         // if a client is disconnected, then type == WStype_DISCONNECTED
      Serial.println("Client " + String(num) + " disconnected");
      break;
    case WStype_CONNECTED:                            // if a client is connected, then type == WStype_CONNECTED
      Serial.println("Client " + String(num) + " connected");
      newNumbers(); // As soon as a client connects, new numbers are generated for the quiz.
      break;
    case WStype_TEXT:                                 // if a client has sent data, then type == WStype_TEXT
      DeserializationError error = deserializeJson(doc_rx, payload);
      if(error) {
        Serial.print("Deserialization failed");
        return;
      }
      else {
          const int g_count = doc_rx["count"];        // Count contains the number of correct answers (should be 4)
          TurnWheel(g_count);
          bolNewGame = true;
      }
      Serial.println("");
      break;
  }

}
