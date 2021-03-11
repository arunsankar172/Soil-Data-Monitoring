#include <SDI12.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <espnow.h>
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define s_pin D5
#define DATA_PIN D1
#define ONE_WIRE_BUS D6

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
SoftwareSerial gsm(D3, D2);
SDI12 mySDI12(DATA_PIN);

String myCommand = "aD0!";
float board1X;
float board2X;
float board3X;
float incomingTemp;
String success;
//2    48:3F:DA:41:42:4D
//3   10:52:1C:EC:DF:A7
uint8_t broadcastAddress3[] = {0x10, 0x52, 0x1C, 0xEC, 0xDF, 0xA7};
uint8_t broadcastAddress2[] = {0x48, 0x3F, 0xDA, 0x41, 0x42, 0x4D};
//uint8_t broadcastAddress2[] = {0x48, 0x3F, 0xDA, 0x41, 0x42, 0x4D};

String b1;

typedef struct struct_message {
    int id;
    char data[50];

} struct_message;

struct_message sleepstatus;
struct_message myData;
struct_message board1;
struct_message board2;
struct_message board3;

struct_message boardsStruct[3] = {board1, board2, board3};

void setup() {
//  pinMode(D6, INPUT_PULLUP);
  pinMode(s_pin, OUTPUT);
  digitalWrite(s_pin, HIGH);
  delay(500);  
  Serial.begin(115200);
  gsm.begin(9600); 
  sensors.begin();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  mySDI12.begin();
  delay(500);
  mySDI12.sendCommand("aC!");
  while (mySDI12.available()) {  // write the response to the screen
    Serial.write(mySDI12.read());
  }
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(OnDataSent);
  esp_now_add_peer(broadcastAddress3, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_add_peer(broadcastAddress2, ESP_NOW_ROLE_COMBO, 1, NULL, 0); 
  esp_now_register_recv_cb(OnDataRecv);
}

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}

void OnDataRecv(uint8_t * mac_addr, uint8_t *incomingData, uint8_t len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  Serial.println();
  memcpy(&myData, incomingData, sizeof(myData));;
  b1=String(myData.data);
}

void simsend(String url){
  url="http://65.0.5.138/insert.php?data="+url;
  Serial.println("Uploading");
  gsm.println("AT");  
  ShowSerialData();
  gsm.println("AT");  
  ShowSerialData();
  gsm.println("AT+SAPBR=3,1,\"APN\",\"airtelgprs.com\""); 
  delay(700);
  ShowSerialData(); 
  gsm.println("AT+SAPBR=1,1");
  delay(1000);
  ShowSerialData();
  gsm.println("AT+HTTPINIT");  
  ShowSerialData();
  gsm.println("AT+HTTPPARA=\"CID\",1");
  delay(100);
  ShowSerialData(); 
  gsm.println("AT+HTTPPARA=\"URL\",\""+url+"\"");
  delay(5000);
  ShowSerialData();
  gsm.println("AT+HTTPACTION=0");
  delay(5000);
  ShowSerialData();
  gsm.println("AT+HTTPREAD=0,10");
  delay(5000);
  ShowSerialData();
}
void ShowSerialData(){
  while(gsm.available()!=0)
  Serial.write(gsm.read());
  delay(500); 
}

float ds18b20_read(){
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

//char * tdr315h_read(){
//  String tdr;
//  mySDI12.sendCommand(myCommand);
//  delay(500);
//  while (mySDI12.available()) {
//    tdr=mySDI12.readString();
//  }
//  char *t;
//  tdr.toCharArray(t,tdr.length());
//  return t;
//}

int flag=0;
char tdr315h[30];

void loop() {
  mySDI12.sendCommand(myCommand);
  flag++;
  float ds18b20;
  char *a;
  int moist=0;
//  Readings
  ds18b20=ds18b20_read();
//  tdr315h=tdr315h_read();

  int i=0;
  while (mySDI12.available()) {
     tdr315h[i]=mySDI12.read();

     i++;
  }

  
//  Serial.println("\n");
//tdr315 read end
tdr315h[sizeof(tdr315h)-1]='\0';
String tdr=String(tdr315h);
tdr.trim();
  moist=analogRead(A0);
//  Encoding
  String data=String(ds18b20)+","+String(moist)+","+String(tdr)+"$"+b1+"$"+String(boardsStruct[2].data); 
  Serial.println(data);   
//  Sending
  if(flag==40){
    simsend(data);
  }
  delay(1000);
  Serial.println(flag);
  if(flag==50 ){
    digitalWrite(s_pin, LOW);  
    Serial.println("Gooing to Sleep");
    ESP.deepSleep(174e7);
  }
}
