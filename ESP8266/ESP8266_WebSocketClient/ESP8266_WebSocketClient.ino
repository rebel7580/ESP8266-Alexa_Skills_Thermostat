/*
 * ESP8266-Alexa_Skills_Thermostat
 * Ron Boston 
 * 20 Aug 2017
 * 
 * Code based on the work of Nassir Malik. See his Youtube tutorials at https://www.youtube.com/channel/UCn8v7OzXk7IqRdKZdf14yjA
 *  and code at https://github.com/nassir-malik
 * TCP code used to talk to the HomeVision system is a modified version from Big Dan the Blogging Man
 *  at https://bigdanzblog.wordpress.com/2016/02/09/esp8266arduino-ide-communicating-with-tcp/  
 */
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <Hash.h>

// @@@@@@@@@@@@@@@ modify the next items for your environment
const char* ssid     = "xxxxxxxxxx";          //enter your ssid/ wi-fi(case sensitiv) router name - 2.4 Ghz only
const char* password = "xxxxxxxxxx";        // enter ssid password (case sensitiv)
char host[] = "bhathermstat.herokuapp.com"; // enter your Heroku domain name like "espiot.herokuapp.com" 
// host and port for TCP connection to HA system
const char  tcphost[] = "192.168.123.xxx";
const int   tcpport = xxxxxxx;
// @@@@@@@@@@@@@@@ 

int pingCount = 0;

const int BLUELED = 2;

int port = 80;
char path[] = "/ws"; 
ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
WiFiClient TCP;

DynamicJsonBuffer jsonBuffer;
String message;
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) { 


    switch(type) {
        case WStype_DISCONNECTED:
            //USE_SERIAL.printf("[WSc] Disconnected!\n");
           Serial.println("Disconnected! ");
            break;
            
        case WStype_CONNECTED:
            {
            Serial.println("Connected! ");
            // send message to server when Connected
            webSocket.sendTXT("Connected");
            }
            break;
            
        case WStype_TEXT:
            Serial.println("Got data");       
            processWebSocketRequest((char*)payload);
            break;
            
        case WStype_BIN:
            hexdump(payload, length);
            Serial.print("Got bin");
            // send data to server
            // webSocket.sendBIN(payload, length);
            break;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
	
    pinMode(BLUELED, OUTPUT);
    digitalWrite(BLUELED, HIGH);
	
    for(uint8_t t = 4; t > 0; t--) {
        delay(1000);
    }
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    
    //Serial.println(ssid);
    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(ssid, password);

    //WiFi.disconnect();
    while(WiFiMulti.run() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("Connected to wi-fi");
    webSocket.begin(host, port, path);
    webSocket.onEvent(webSocketEvent);
}

void loop() {
    webSocket.loop();
        delay(2000);
	// make sure after every 40 seconds send a ping to Heroku
	//so it does not terminate the websocket connection
	//This is to keep the conncetion alive between ESP and Heroku
	if (pingCount > 20) {
		pingCount = 0;
		webSocket.sendTXT("\"heartbeat\":\"keepalive\"");
	}
	else {
		pingCount += 1;
	}
}

void processWebSocketRequest(String data){
    String jsonResponse = "{\"version\": \"1.0\",\"sessionAttributes\": {},\"response\": {\"outputSpeech\": {\"type\": \"PlainText\",\"text\": \"<text>\"},\"shouldEndSession\": true}}";
    String inst[3];
    digitalWrite(BLUELED, LOW); // Turn on blue LED
	
    JsonObject& req = jsonBuffer.parseObject(data);
    String result = "";
    String instance = req["instance"];
    String setting = req["setting"];
    String state = req["state"];
    String query = req["query"];
    String message = "{\"event\": \"OK\"}";
    
    Serial.println("Data2-->" + data);
    Serial.println("State-->" + state);

    if (query == "?") { //if query then check state
        Serial.println("Received query!");
        int n = 1;
        if (instance == "mode") {
            inst[0] = "system";
        } else if (instance == "status") {
            inst[0] = "system";
            inst[1] = "setpoint";
            inst[2] = "temperature";
            n = 3;
        } else {
            inst[0] = instance;
        }
        for (int i=0; i<n; i++) {
            result += inst[i] + " is " + getNetIO(inst[i]);
            if (inst[i] != "system") {
                result += " degrees. ";
            } else {
                result += ". ";
            }
        }
        Serial.println(result);
        jsonResponse.replace("<text>", result );

    } else if (query == "cmd") { //if command then do
        Serial.println("Received command!");
        if (setting != "none") {
            result = setNetIO("setpoint", setting);            
        } else if (state != "none") {
            result = setNetIO(state, "");            
        }
        jsonResponse.replace("<text>", result);
    } else { //can not recognized the command
        Serial.println("Command is not recognized!");
        jsonResponse.replace("<text>", "Command is not recognized by Thermostat Alexa skill");
    }
    Serial.print("Sending response back");
    Serial.println(jsonResponse);
    // send message to server
    webSocket.sendTXT(jsonResponse);
    if(query == "cmd" || query == "?"){webSocket.sendTXT(jsonResponse);}
	
    digitalWrite(BLUELED, HIGH); // Turn off blue LED
}

String getNetIO(String val){
    String cmd = "get hvac state 0 ";
    if (val == "mode") {
      val = "system";
    }
    cmd += val;
    Serial.println(cmd);
    return(sendRequestNetio(cmd));
}

String setNetIO(String val, String num){
    String cmd = "action hvac ";
    cmd += val + " 0 " + num;
    Serial.println(cmd);
    return(sendRequestNetio(cmd));
}

String sendRequestNetio(String cmd){
    Serial.print("connecting to ");
    Serial.println(tcphost);
    Serial.print("Requesting cmd: ");
    Serial.println(cmd);
    char s[100];
    int i = 0;

    if (!TCP.connect(tcphost, tcpport)) {
        Serial.println("connection failed");
        return("Fail");
    }
    TCP.println(cmd);
    Serial.print("Sent cmd: ");  

    while (true) {
        while (TCP.available() == 0) {
            delay(10UL);
        }
        s[i] = TCP.read(); 
        delay(10UL);
        if (s[i] == '\n' || s[i] == '\r') {
            s[i] = '\0';
            break;
        }
        else {
            i = i + 1;
            if (i == sizeof(s)) {
                Serial.println("buffer overflow");
                return("buffer overflow");
            }
        }
    } // while

    Serial.println(s);
    return(s);
}
