#include <ESP8266WiFi.h>
//#include "ESP8266WiFi.h"
#include <math.h>
#include <EEPROM.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ESP8266httpUpdate.h>

extern "C" {
#include "user_interface.h"
}


HTTPClient http;
ESP8266WebServer server(80);

bool sendIp = true;
bool isPaired = false;

int sendIpCounter = 0;

//int connectionLed = 5;
int relayPin = 4;
int sizepass;
int sizess;

int relayState = 0;

int redLed = 14;
int greenLed = 12;
int blueLed = 13;


int defaultButton = 5;
int relaySwitchButton = 15;

String  ssidStr="";
String passStr="";
String host ="termoservlet-env.eu-central-1.elasticbeanstalk.com";


String transmitterIpStr="";
String localIpStr="";
String gatewayStr="";

char hostString[16] = {0};

String transmitterHostStr="";

String pairingCode = "";

const char* ssid;


String wifiList[5];

String networks="";

int rssiList[5];
int arraySize = 0;

int biggestRssi = -200;
int biggestRssiIndex= 0;
int connectionTryCounter = 0;

unsigned long activeMillis = 0;


IPAddress senderNetIp;
IPAddress receiverNetIp;
int addr =0;

String senderNetIpStr;


void parseBytes(const char* str, char sep, byte* bytes, int maxBytes, int base) {
    for (int i = 0; i < maxBytes; i++) {
        bytes[i] = strtoul(str, NULL, base);  // Convert byte
        str = strchr(str, sep);               // Find next separator
        if (str == NULL || *str == '\0') {
            break;                            // No more separators, exit
        }
        str++;                                // Point to next character after separator
    }
}

String ipToString(IPAddress ip){

    byte ip1 = ip[0];
    byte ip2 = ip[1];
    byte ip3 = ip[2];
    byte ip4 = ip[3];
    
    String ipStr = String(ip1)+'.'+String(ip2)+'.'+String(ip3)+'.'+String(ip4);
    return ipStr;
    
}

void writeEeprom(const char data[] , int lng){

  for(int i =0  ; i< lng;i++){

    EEPROM.write(addr, data[i]);
    addr++;
  }
  
  EEPROM.write(addr, 255);
  addr++;

  EEPROM.commit();
  
}




String readEeprom(int adr){

  String var;
  byte val;
  char charVal ;
  
  int i = 0;
  while(1){

    val = EEPROM.read(adr);

    if(val != 255){

      charVal = val;
      Serial.println(charVal);
      var += charVal;
      i++;
      adr++;
    }
    else{
      
      break;
    }
  }

  Serial.println(var);

  return var;
  
}

  void resetDefault(){

    Serial.println("reset command has arrived");
    addr = 0;
    for(int i =0  ; i< 512;i++){

    EEPROM.write(addr, 255);
   // Serial.println(data[i]);
    addr++;
  }

    EEPROM.commit();

     server.send(200 , "text/html" , "1");


    Serial.println("esp restart");
    ESP.restart();  
  
     
  }

  void checkUpdate(){


     Serial.println("check for update");
  
      t_httpUpdate_return ret = ESPhttpUpdate.update(host , 80 , "/Download/receiver?pairingCode="+pairingCode, "");

    switch(ret) {
      
    case HTTP_UPDATE_FAILED:
        Serial.println("[update] Update failed.");
        break;
    case HTTP_UPDATE_NO_UPDATES:
        Serial.println("[update] Update no Update.");
        break;
    case HTTP_UPDATE_OK:
        
        Serial.println("[update] Update ok."); // may not called we reboot the ESP
        ESP.restart();
        break;
      }
    
  }
  

  void checkFirmware(){

        Serial.println("check firmware command has arrived");

        digitalWrite( redLed , HIGH);
        digitalWrite( greenLed , LOW);
        digitalWrite( blueLed , HIGH);

        if(server.hasArg("pairingCode")){

          pairingCode = server.arg("pairingCode");

          checkUpdate();
        }

       
  
     
  }

  void searchTransmitterHost(String transmitterHost){


        if (!MDNS.begin(hostString)) {
          Serial.println("Error setting up MDNS responder!");
        }
        
        Serial.println("mDNS responder started");
        MDNS.addService("esp", "tcp", 8080); // Announce esp tcp service on port 8080

      delay(1000);

     Serial.println("Sending mDNS query");
  int n = MDNS.queryService("esp", "tcp"); // Send out query for esp tcp services
  Serial.println("mDNS query done");
  if (n == 0) {
    Serial.println("no services found");
  }
  else {
    Serial.print(n);
    Serial.println(" service(s) found");
    for (int i = 0; i < n; ++i) {
      // Print details for each service found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(MDNS.hostname(i));
      Serial.print(" (");
      Serial.print(MDNS.IP(i));
      Serial.print(":");
      Serial.print(MDNS.port(i));
      Serial.println(")");

      transmitterHost.toLowerCase();

      if(MDNS.hostname(i) == transmitterHost){

      transmitterIpStr =  ipToString(MDNS.IP(i));
      
      }

      
    }
  }
  }


  void defaultButtonInterrupt(){


    if(digitalRead(defaultButton) == LOW){

    Serial.println("girdi 1");
    //delay(50);
          unsigned long buttonMillis = millis();

          while(1){

          Serial.println("girdi 2");
         // delay(50);
            if(digitalRead(defaultButton) == LOW){

              if((millis() - buttonMillis) > 3000){

                //smartconfig

                resetDefault();

                Serial.println("3 saniye butona basıldı");

                return;

              }
            }else{

              return;
            }
          }
          
        
      }

      
  }

  void relaySwitchInterrupt(){


      if(digitalRead(relaySwitchButton) == HIGH){

    Serial.println("girdi 1");
    //delay(50);
          unsigned long buttonMillis = millis();

          while(1){

          Serial.println("girdi 3");
         // delay(50);
            if(digitalRead(relaySwitchButton) == HIGH){

              if((millis() - buttonMillis) > 3000){

                if(relayState == 1){

                  relayState = 0;
                  
                }else{

                  relayState = 1;
                }

                

                digitalWrite(relayPin , relayState);

                  if(relayState == 1){

                    digitalWrite(redLed , HIGH);
                    digitalWrite(greenLed , LOW);
                    digitalWrite(blueLed , LOW);
        
                  }else if(relayState == 0){

                    digitalWrite(redLed , LOW);
                    digitalWrite(greenLed , HIGH);
                    digitalWrite(blueLed , HIGH);
  
                }

                Serial.println("röle konum degistirdi");

                return;

              }
            }else{

              return;
            }
          }
          
        
      }
    
  }

void handleScan(){

     

      server.send(200 , "text/html" , networks);

     

}

void check(){

  Serial.println("verici check kontrol");
  server.send(200 , "text/html" , "OK");

  activeMillis = millis();

  sendIp = false;
}



int sendHostToTransmitter(String hostStr){



    String url = "http://"+transmitterIpStr+"/receiverHostCheck?receiverHost="+hostStr;
  Serial.println(url);

  http.begin(url);
  int httpCode = http.GET();
  Serial.println(httpCode);

  if(httpCode>0){

    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    if(httpCode == 200){

    String payload = http.getString();

    int responseCode = payload.toInt();

    if(responseCode == 1){

      Serial.println("pairingCompleted");

      //ssid yi eeproma kaydet

      sendIp = false;

      
      
      isPaired = true;

      
    }
   
    
      
    }
  }

  http.end();

  return httpCode;
  
} 

int checkInternet(){

  String url = "http://termoservlet-env.eu-central-1.elasticbeanstalk.com/Register/checkInternet?1";
  int result = 0;
  Serial.println(url);
  http.begin(url);
  int httpCode = http.GET();
  Serial.println(httpCode);

  if(httpCode>0){

     Serial.printf("[HTTP] GET... code: %d\n", httpCode);

     

        result = 1;
      

      
     
  }

  http.end();

  return result;
}

bool checkTransmitter(){

  String url = "http://"+transmitterIpStr+"/checkActive";

  Serial.println(url);
  http.begin(url);
  int httpCode = http.GET();
  Serial.println(httpCode);

  bool value = false;

  if(httpCode>0){

     Serial.printf("[HTTP] GET... code: %d\n", httpCode);

     if(httpCode == 200){

      String payload = http.getString();

      Serial.println(payload);
      
      int state  = payload.toInt();
      
      if(state == 1){

        value = true;
      }
      
      
     }
  }
    http.end();

    return value;
 
}



void askRelayState(String transmitterIpStr){

  String url = "http://"+transmitterIpStr+"/askRelayState";
  
  Serial.println(url);
  http.begin(url);
  int httpCode = http.GET();
  Serial.println(httpCode);

  if(httpCode>0){

     Serial.printf("[HTTP] GET... code: %d\n", httpCode);

     if(httpCode == 200){

      String payload = http.getString();

      Serial.println(payload);
      
      relayState = payload.toInt();
      
      digitalWrite(relayPin , relayState);

      if(relayState == 1){

        digitalWrite(redLed , HIGH);
        digitalWrite(greenLed , LOW);
        digitalWrite(blueLed , LOW);
        
      }else if(relayState == 0){

        digitalWrite(redLed , LOW);
        digitalWrite(greenLed , HIGH);
        digitalWrite(blueLed , HIGH);
  
      }

      sendIp = false;
      
      
     }
  }
    http.end();
}



void updateReceiver(String localIpStr , String pairingCode){


  
  String url = "http://"+host+"/Register/updateReceiver?receiverIp="+localIpStr+"&pairingCode="+pairingCode;
  Serial.println(url);
  http.begin(url);
  int httpCode = http.GET();
  Serial.println(httpCode);

  if(httpCode>0){

     Serial.printf("[HTTP] GET... code: %d\n", httpCode);

     if(httpCode == 200){

      const char* localIpCh = localIpStr.c_str();

      addr= 150;

      writeEeprom(localIpCh , localIpStr.length());

      String payload = http.getString();

      Serial.println(payload);

      const char* json = payload.c_str();

      StaticJsonBuffer<200> jsonBuffer;

      JsonObject& root = jsonBuffer.parseObject(json);

      if (!root.success()) {
    Serial.println("parseObject() failed");
    
  }
      
      //String recIp;
      int result = root["result"];

      

      if(localIpStr.length() > 0){

        //sendIpToTransmitter(localIpStr);
        
      }else {

        //localIpStr = readEeprom(150);

        //sendIpToTransmitter(localIpStr);
        
      }


      
     }
  }
    http.end();
 
 }





void checkHostFromServer(String hostStr){


  
  String url = "http://"+host+"/Register/receiver?receiverHost="+hostStr;
  Serial.println(url);
  Serial.print("[HTTP] begin...\n");
  http.begin(url);   // HTTP

      int control = 5;
      control = checkInternet();

      if(control == 0){
        digitalWrite( redLed , LOW);
        digitalWrite( greenLed , LOW);
        digitalWrite( blueLed , HIGH);
      }
      else{
        digitalWrite( redLed , HIGH);
        digitalWrite( greenLed , HIGH);
        digitalWrite( blueLed , LOW);
      }
      Serial.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
        // file found at server
          String payload = http.getString();
          Serial.println(payload);
        


  /*
  http.begin(url);
  int httpCode = http.GET();
  Serial.println(httpCode);

  if(httpCode>0){

     Serial.printf("[HTTP] GET... code: %d\n", httpCode);

     if(httpCode == 200){

      Serial.println("http code OK!");
      int control = 5;
      control = checkInternet();

      if(control == 0){
        digitalWrite( redLed , LOW);
        digitalWrite( greenLed , LOW);
        digitalWrite( blueLed , HIGH);
      }
      else{
        digitalWrite( redLed , HIGH);
        digitalWrite( greenLed , LOW);
        digitalWrite( blueLed , LOW);
      }

      String payload = http.getString();

      Serial.println("Json payload a yazıldı!");
      */
      const char* json = payload.c_str();

      Serial.printf("json C formatı: ", json);
      
      Serial.printf("gelen json: ", json);
      Serial.println("");

      StaticJsonBuffer<200> jsonBuffer;

      JsonObject& root = jsonBuffer.parseObject(json);

      if (!root.success()) {
    Serial.println("parseObject() failed");
    
  }

      
      //String recIp;
      const char* trans = root["transmitterHost"];

      transmitterHostStr = String(trans);

      const char* pairCh = root["pairingCode"];

      pairingCode = String(pairCh);

      addr = 300;
      
      writeEeprom(pairCh , pairingCode.length());

      if(transmitterHostStr.length()>0){

        
        
      Serial.print("Verici host:");
      Serial.println(transmitterHostStr);

      addr= 250;

      writeEeprom(trans , transmitterHostStr.length());

      searchTransmitterHost(transmitterHostStr);

      /*if(transmitterIpStr.length() > 0){

       sendHostToTransmitter(String(hostString));

       sendIp = false;
       
      } */

      

      
        
      }
      else{

       // server.on("/isConnect",isConnect);
       server.on("/transmitterHostCheck" , handleTransmitterHost);
        
      }
      

      
      

      
      

  
    http.end();
          
      }
 }
  


   void handleTransmitterHost(){


    if(server.hasArg("transmitterHost")){

      
      transmitterHostStr = server.arg("transmitterHost");

      Serial.print("transmitterHost: ");
      Serial.println(transmitterHostStr);
       
      
      addr = 250;
      
      const char* transHostCh = transmitterHostStr.c_str();
      writeEeprom(transHostCh , transmitterHostStr.length());

      searchTransmitterHost(transmitterHostStr);


      if(transmitterIpStr.length() >  0){

      isPaired = true;

      sendIp = false;
        
      }
      
      server.send(200 , "text/html" , "1");
      
  
    
  }
    

    
   }


   void afterSmartConfig(){

  

   digitalWrite(redLed , LOW);
       digitalWrite(greenLed , LOW);
       digitalWrite(blueLed , LOW);

       

        sprintf(hostString, "ESP_%06X", ESP.getChipId());
        Serial.print("Hostname: ");
        Serial.println(hostString);
        WiFi.hostname(hostString);

        if (!MDNS.begin(hostString)) {
          Serial.println("Error setting up MDNS responder!");
        }
        
        Serial.println("mDNS responder started");
        MDNS.addService("esp", "tcp", 8080); // Announce esp tcp service on port 8080


       IPAddress localIp = WiFi.localIP();   

        localIpStr = ipToString(localIp);

       

       const char* localIpCh = localIpStr.c_str();

       //addr= 150;

       //writeEeprom(localIpCh , localIpStr.length());

       ssidStr = WiFi.SSID();

       passStr = WiFi.psk();

       //IPAddress gateway = WiFi.gatewayIP();

       //String gatewayStr = ipToString(gateway);

       const char* ssidCh = ssidStr.c_str();

       const char* passCh = passStr.c_str();

       //const char* gatewayCh = gatewayStr.c_str();

       addr = 0;

       writeEeprom(ssidCh , ssidStr.length()); 

       Serial.println("SSID eeproma yazıldı");

       addr=50;

       writeEeprom(passCh , passStr.length()); 

       Serial.println("Password eeproma yazıldı");

       //addr= 200;

       //writeEeprom(gatewayCh, gatewayStr.length()); 

               
       checkHostFromServer(String(hostString));

        /*if(transmitterIpStr.length() > 0){

        askRelayState(transmitterIpStr);
  
        } */

      

//deviceMode

     



}



  void handleNotFound(){
    
  String message = "File Not Found\n\n";
  
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}




void setRelay(){


  if(server.hasArg("relayState")){

    String rl = server.arg("relayState");

     relayState = rl.toInt();

     activeMillis = millis();
     
    if(relayState==0){

     
      server.send(200 , "text/html" , "0");
    }
    else{

      
      server.send(200 , "text/html" , "1");
    }

     digitalWrite(relayPin , relayState);

     if(relayState == 1){

        digitalWrite(redLed , HIGH);
        digitalWrite(greenLed , LOW);
        digitalWrite(blueLed , LOW);
        
      }else if(relayState == 0){

        digitalWrite(redLed , LOW);
        digitalWrite(greenLed , HIGH);
        digitalWrite(blueLed , HIGH);
  
      }
  }
}


void reconfigure(){


    int control = 0 ;

    byte index = 0;

    while(control == 0){

    IPAddress ip = WiFi.localIP();  

    IPAddress gateway = WiFi.gatewayIP();

    IPAddress subnet = IPAddress(255,255,255,0);    
        
    IPAddress newIp = IPAddress(ip[0],ip[1],ip[2],200+index);
      
     WiFi.disconnect();

     delay(500);

     const char* ssid = ssidStr.c_str();
     const char* password = passStr.c_str();

     Serial.print("ssid:");
     Serial.println(ssidStr);
    
     Serial.print("size of ssid:");
    Serial.println(ssidStr.length());
    
    Serial.print("password:");
    Serial.println(passStr);

    Serial.print("size of password:");
    Serial.println(passStr.length());


   
   


  int connectIndex = 0;
     
  WiFi.begin(ssid,password);
  
  WiFi.config(newIp , gateway , subnet);
  
  
  while (WiFi.status() != WL_CONNECTED)
  {
  connectIndex++;  
  
  delay(200);
  
  Serial.print(".");
 
  delay(200);

  if(connectIndex > 300){
    
    break;
    
    }
  
      }

      control = checkInternet();

      if(control == 0){

        index++;
      }
      
      
    }
    
    
          
       IPAddress localIp = WiFi.localIP();   

       localIpStr = ipToString(localIp);

       Serial.print("new Ip address:");

       Serial.println(localIpStr);                     

       const char* localIpCh = localIpStr.c_str();

       addr= 150;

       writeEeprom(localIpCh , localIpStr.length());

       IPAddress gateway = WiFi.gatewayIP();

       String gatewayStr = ipToString(gateway);       

       const char* gatewayCh = gatewayStr.c_str();       

       addr= 200;

       writeEeprom(gatewayCh, gatewayStr.length());  

       

       //updateTransmitter(localIpStr , pairingCode);

       if (!MDNS.begin(hostString)) {
          Serial.println("Error setting up MDNS responder!");
        }
        
        Serial.println("mDNS responder started");
        MDNS.addService("esp", "tcp", 8080); // Announce esp tcp service on port 8080

        sendHostToTransmitter(String(hostString));    

       //updateReceiver(localIpStr , pairingCode);

      

    //Bağlı olduğundan emin olunmalı
  }



void reconnect(){

    const char* ssid = ssidStr.c_str();
     const char* password = passStr.c_str();

     Serial.print("ssid:");
    Serial.println(ssidStr);
    
     Serial.print("size of ssid:");
    Serial.println(ssidStr.length());
    
    Serial.print("password:");
    Serial.println(passStr);

    Serial.print("size of password:");
    Serial.println(passStr.length());


    

    int connectIndex = 0;
            
           WiFi.begin(ssid,password);
           if(localIpStr.length() > 0 && gatewayStr.length() > 0){

    const char* localIpChr = localIpStr.c_str();
    const char* gatewayChr = gatewayStr.c_str();

    byte ip[4];

    parseBytes(localIpChr , '.' , ip , 4 , 10);

    IPAddress localIp = IPAddress(ip[0],ip[1],ip[2],ip[3]);

    Serial.println(localIp);
    

    parseBytes(gatewayChr , '.' , ip , 4 , 10);

    IPAddress gateway = IPAddress(ip[0],ip[1],ip[2],ip[3]);

    Serial.println(gateway);

    IPAddress subnet = IPAddress(255,255,255,0);

    Serial.println(subnet);

    WiFi.config(localIp , gateway , subnet);
    
  }
  
         while (WiFi.status() != WL_CONNECTED)
         {
            connectIndex++;
  
            
 
  
            delay(200);
  
            Serial.print(".");

            

            delay(200);

            if(connectIndex > 300){
    
              break;
            }
            
  
          }
          if(WiFi.status() == WL_CONNECTED){

            Serial.println("reconnected");
          }
  }




void isConnect(){
//ip adresleri kontrol edilerek düzenlenecek
  //String chipId;
  

      if(server.hasArg("pairingCode")){

      
       pairingCode = server.arg("pairingCode");

       const char* pairingCh = pairingCode.c_str();

       addr = 300;   

       writeEeprom(pairingCh , pairingCode.length());

          

       server.send(200 , "text/html" ,"1" );
      
       Serial.println("eslesme basarili");

       sendIp = false;
    
      }else{

        server.send(200 , "text/html" ,"1" );
      }

      
    
 // server.send(200 , "text/html" ,"" );
  
}


void handleRoot() {
  
  digitalWrite(13, 1);
  server.send(200, "text/plain", "hello from esp8266!");
  digitalWrite(13, 0);
  
}


void setup(){
  // put your setup code here, to run once:

  Serial.begin(115200);
  EEPROM.begin(512);

  //networks = scan();


  pinMode(relayPin , OUTPUT);

  pinMode(defaultButton , INPUT_PULLUP);

  attachInterrupt(defaultButton, defaultButtonInterrupt, CHANGE);
  
 
  pinMode(relaySwitchButton , INPUT);

  attachInterrupt(relaySwitchButton, relaySwitchInterrupt , CHANGE);
  
  
 //WiFi.disconnect();
 
  WiFi.softAPdisconnect();
  
  WiFi.mode(WIFI_STA);
  
 // wifi_set_phy_mode(PHY_MODE_11N);
  
  //String softApSsidStr = readEeprom(200);

  
  
 

  //pinMode(connectionLed, OUTPUT);
 

  pinMode(redLed , OUTPUT );
  pinMode(greenLed , OUTPUT );
  pinMode(blueLed , OUTPUT );


  digitalWrite(redLed , HIGH );
  digitalWrite(greenLed , HIGH);
  digitalWrite(blueLed , HIGH);

  
  //digitalWrite(connectionLed , LOW);
 

  

  ssidStr = readEeprom(0);
  Serial.println(ssidStr);
  passStr = readEeprom(50);
  Serial.println(passStr);
 

  localIpStr = readEeprom(150);
  Serial.println(localIpStr);
  gatewayStr = readEeprom(200);
  Serial.println(gatewayStr);

  transmitterHostStr = readEeprom(250);
  Serial.println(transmitterHostStr);

  pairingCode = readEeprom(300);
  Serial.println(pairingCode);

  if(ssidStr.length() > 0 && passStr.length() > 0 && transmitterHostStr.length() > 0 ){

    Serial.println("alici wifi deneme");
     const char* ssid = ssidStr.c_str();
     const char* password = passStr.c_str();

     Serial.print("ssid:");
    Serial.println(ssidStr);
    
     Serial.print("size of ssid:");
    Serial.println(ssidStr.length());
    
    Serial.print("password:");
    Serial.println(passStr);

    Serial.print("size of password:");
    Serial.println(passStr.length());


    

    sprintf(hostString, "ESP_%06X", ESP.getChipId());
    Serial.print("Hostname: ");
    Serial.println(hostString);
    WiFi.hostname(hostString);

    
     int connectIndex = 0;
  WiFi.begin(ssid,password);

  if(localIpStr.length() > 0 && gatewayStr.length() > 0){

    const char* localIpChr = localIpStr.c_str();
    const char* gatewayChr = gatewayStr.c_str();

    byte ip[4];

    parseBytes(localIpChr , '.' , ip , 4 , 10);

    IPAddress localIp = IPAddress(ip[0],ip[1],ip[2],ip[3]);

    Serial.println(localIp);
    

    parseBytes(gatewayChr , '.' , ip , 4 , 10);

    IPAddress gateway = IPAddress(ip[0],ip[1],ip[2],ip[3]);

    Serial.println(gateway);

    IPAddress subnet = IPAddress(255,255,255,0);

    Serial.println(subnet);

    WiFi.config(localIp , gateway , subnet);
  }
  
  
  while (WiFi.status() != WL_CONNECTED)
{
  connectIndex++;
  
  //digitalWrite(connectionLed , HIGH);

  digitalWrite(redLed , LOW);
  digitalWrite(greenLed , LOW);
  digitalWrite(blueLed , LOW);
  
  delay(200);
  
  Serial.print(".");

  //digitalWrite(connectionLed , LOW);

  digitalWrite(redLed , HIGH);
  digitalWrite(greenLed , HIGH);
  digitalWrite(blueLed , HIGH);

  delay(200);

  if(connectIndex > 300){
    
    break;
  }
  
}

 if(WiFi.status() != WL_CONNECTED){

   //digitalWrite(connectionLed , LOW);

  //Rgb Led Beyaz Olacak
  //smartConfig e girecek


  digitalWrite(redLed , HIGH);
  digitalWrite(greenLed , HIGH);
  digitalWrite(blueLed , HIGH);

    int tryCounter = 0;
    Serial.println("smartconfig");
while(WiFi.status() != WL_CONNECTED) {
    WiFi.beginSmartConfig();
       while(1){
           delay(1000);
           if(WiFi.smartConfigDone()){
             Serial.println("SmartConfig Success");

              digitalWrite(redLed ,  !digitalRead(redLed));
             digitalWrite(greenLed , !digitalRead(greenLed));
             digitalWrite(blueLed , !digitalRead(blueLed));

             tryCounter++; 
              
             if( tryCounter > 20){

              WiFi.stopSmartConfig();
              Serial.println("smartconfig again");
              tryCounter =  0;

                digitalWrite(redLed , LOW);
                digitalWrite(greenLed , LOW);
                digitalWrite(blueLed , LOW);
              
             }
             
             break;
           }
       }

}

       delay(1000);       

       afterSmartConfig();



  
  
  //server.on("/wifi", handleWifi);
    
  }
  else{

    delay(10);

  digitalWrite(redLed , LOW);
  digitalWrite(greenLed , LOW);
  digitalWrite(blueLed , LOW);


  if (!MDNS.begin(hostString)) {
          Serial.println("Error setting up MDNS responder!");
        }
        
        Serial.println("mDNS responder started");
        MDNS.addService("esp", "tcp", 8080); // Announce esp tcp service on port 8080

  
  searchTransmitterHost(transmitterHostStr);

  if(transmitterIpStr.length() > 0){

    Serial.print("transmitter ip: ");
    Serial.println(transmitterIpStr);
  }
  
  

  //Serial.println("EEproma kaydedildi");

 //IPAddress vericiIP(ip1,ip2,ip3,ip4);
     
  
  WiFi.printDiag(Serial);

if(transmitterIpStr.length() > 0){


  askRelayState(transmitterIpStr);
  
}
  

  
  }
    
  }
  else{

    

    //smartConfig

    
  digitalWrite(redLed , HIGH);
  digitalWrite(greenLed , HIGH);
  digitalWrite(blueLed , HIGH);


    int tryCounter = 0 ;
    Serial.println("smartconfig");
while(WiFi.status() != WL_CONNECTED) {
    WiFi.beginSmartConfig();
       while(1){
           delay(1000);
           if(WiFi.smartConfigDone()){
             Serial.println("SmartConfig Success");

             digitalWrite(redLed ,  !digitalRead(redLed));
             digitalWrite(greenLed , !digitalRead(greenLed));
             digitalWrite(blueLed , !digitalRead(blueLed));

             tryCounter++; 
              
             if( tryCounter > 20){

              WiFi.stopSmartConfig();
              Serial.println("smartconfig again");
              tryCounter =  0;

                digitalWrite(redLed , LOW);
                digitalWrite(greenLed , LOW);
                digitalWrite(blueLed , LOW);
              
             }
             
             break;
           }
       }

}

       delay(1000);       

       
       afterSmartConfig();       

  
  }
 
   
  //server.on("/", handleRoot);
  
 // server.on("/receiverIp" , handleReceiverIp);

   Serial.println("HTTP server started");
  

  checkInternet();  

  checkUpdate();

  server.on("/", handleRoot);
  server.on("/setRelay" , setRelay);
  server.on("/isConnect",isConnect);
  server.on("/scanNetworks",handleScan);
  server.on("/check",check);
  server.on("/transmitterHostCheck" , handleTransmitterHost);
  server.on("/reset" , resetDefault);
  server.on("/checkFirmware" , checkFirmware);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Server started");

 

  

}

void loop(void) {
  
  // put your main code here, to run repeatedly:

server.handleClient();
delay(50);

if(sendIp){

  if(transmitterIpStr.length() > 0){

    /*if(sendIpCounter > 4){

      searchTransmitterHost(transmitterHostStr);
      
    }else{

      int returnValue = sendHostToTransmitter(String(hostString));

      delay(100);

    if(returnValue < 0){

      sendIpCounter++;
    }
    
    } */

    
  }else{

    if(transmitterHostStr.length() > 0){

      searchTransmitterHost(transmitterHostStr);
      
    }else{

      checkHostFromServer(String(hostString));
      
    }
    
  }  
  
  delay(500);
  
}else{

  if(( millis() - activeMillis ) > 125000){

  //bir sıkıntı var

  activeMillis = millis();

    if(WiFi.status() != WL_CONNECTED){

        reconnect();
          
         }else{


         bool transCheck = checkTransmitter();

          if(!transCheck){

            //İnternet bağlantısı kontrol edilmeli

            int check = 0;

          for(int i = 0 ;  i < 3 ; i++ ){

           check =  checkInternet();
           
          }

          if(check == 0){

            //IP cakisması vardır
           
            reconfigure();
          
          }
          }

          
         }
  
}
}



   /* if(digitalRead(defaultButton) == LOW){

    Serial.println("girdi 1");
    delay(50);
          unsigned long buttonMillis = millis();

          while(1){

          Serial.println("girdi 2");
          delay(50);
            if(digitalRead(defaultButton) == LOW){

              if((millis() - buttonMillis) > 3000){

                //smartconfig

                resetDefault();

                Serial.println("3 saniye butona basıldı");

                return;

              }
            }else{

              return;
            }
          }
          
        
      } */ 
 server.handleClient();


  
}
