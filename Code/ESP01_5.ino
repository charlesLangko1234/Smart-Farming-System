/***************************************************** 
* ArduFarmBot Light - Remote controling a plantation
* 
* ThingSpeak ID Channels: 
*   Status (Actuators and Sensors): 999999
*   Actuator1: 1741804 (Pompa air)
*   Actuator2: 1741806 (Pompa Pupuk)
* 
* Sensors:
*   DHT (2-wire Air Temperature/Humidity digital sensor)  ==> Pin D11
*   LDR (Light Dependent Resistor - Analog Sensor)        ==> Pin A1
*   LM394/YL70 (Soil Humidity Analog Sensor)  
* 
* Actuators:
*   Actuator1         ==> Pin 10 (RED LED   ==> pump)
*   Actuator2         ==> Pin 12 (GREEN LED ==> lamp)
* 
* FREEZE_LED:       ==> Pin 13 (ESP-01 Freezing and Comm errors)
* HW RESET          ==> Pin 08
*     
* 
*****************************************************/

// Thingspeak  
String statusChWriteKey = "LTSQGBI07YK6HRWZ";  // Status Channel id: 385184

String canalID1 = "1741804"; // Enter your Actuator1 Channel ID here
String canalID2 = "1741806"; // Enter your Actuator1 Channel ID here
String canalID3 = "1744440";

#include <SoftwareSerial.h>
SoftwareSerial EspSerial(6, 7); // Rx,  Tx

// HW Pins
#define FREEZE_LED 10
#define HARDWARE_RESET 8

//DHT
#include "DHT.h"
#include <stdlib.h>
int pinoDHT = 2;
int tipoDHT =  DHT11;
DHT dht(pinoDHT, tipoDHT); 
int airTemp = 0;
int airHum = 0;

// Soil humidity
#define soilHum1PIN A0
#define soilHum2PIN A1
int soilHum1 = 0;
int soilHum2 = 0;

// Variables to be used with timers
long writeTimingSeconds = 17; // ==> Define Sample time in seconds to send data
long readTimingSeconds = 10; // ==> Define Sample time in seconds to receive data
long startReadTiming = 0;
long elapsedReadTime = 0;
long startWriteTiming = 0;
long elapsedWriteTime = 0;

//Relays
#define ACTUATOR1 13 // RED LED   ==> Pump
#define ACTUATOR2 12 // GREEN LED ==> Lamp
boolean pumpAir = 0; 
boolean pumpPupuk = 0;
boolean arah = 0;//0->kiri; 1->kanan

//Servo
#include <Servo.h> 
Servo servo; 

//OLED
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

Adafruit_SSD1306 lcd(128, 64, &Wire, 4);

#define btnUp   7
#define btnOk   6
#define btnDown 5

boolean statusBtnUp   = false;
boolean statusBtnOk   = false;
boolean statusBtnDown = false;

boolean statusAkhirBtnUp   = false;
boolean statusAkhirBtnOk   = false;
boolean statusAkhirBtnDown = false;

boolean UP   = false;
boolean OK   = false;
boolean DOWN = false;

int halaman  = 1;
int menuItem = 1;

int spare = 0;
boolean error;

void setup()
{
  Serial.begin(9600);

  lcd.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  lcd.clearDisplay();

  pinMode(btnUp,   INPUT_PULLUP);
  pinMode(btnOk,   INPUT_PULLUP);
  pinMode(btnDown, INPUT_PULLUP);
  
  pinMode(ACTUATOR1,OUTPUT);
  pinMode(ACTUATOR2,OUTPUT);
  pinMode(FREEZE_LED,OUTPUT);
  pinMode(HARDWARE_RESET,OUTPUT);

  digitalWrite(ACTUATOR1, HIGH); //o módulo relé é ativo em LOW
  digitalWrite(ACTUATOR2, HIGH); //o módulo relé é ativo em LOW
  digitalWrite(FREEZE_LED, LOW);
  digitalWrite(HARDWARE_RESET, HIGH);

  servo.attach(9);
  servo.write(90);
  dht.begin();

  EspSerial.begin(9600); // Comunicacao com Modulo WiFi
  EspHardwareReset(); //Reset do Modulo WiFi
  startReadTiming = millis(); // starting the "program clock"
  startWriteTiming = millis(); // starting the "program clock"

  connectWiFi();

  lcd.setTextSize(1);
  lcd.setTextColor(WHITE);
  lcd.setCursor(0, 10);
  
  lcd.println("UMN ROBOTIC\nCLUB");
  lcd.display();
  delay(2000); 
}

void loop()
{
  start: //label 
  error=0;
  
  
  elapsedWriteTime = millis()-startWriteTiming; 
  elapsedReadTime = millis()-startReadTiming; 

  if (elapsedReadTime > (readTimingSeconds*1000)) 
  {
    ESPcheck();//executar antes de qualquer leitura ou gravação
    int command = readThingSpeak(canalID1); 
    if (command != 9) pumpAir = command; 
    delay (1000);
    command = readThingSpeak(canalID2); 
    if (command != 9) pumpPupuk = command; 
    delay (1000);
    command = readThingSpeak(canalID3); 
    if (command != 9) arah = command; 

    takeActions();
    startReadTiming = millis();   
  }
  
  if (elapsedWriteTime > (writeTimingSeconds*1000)) 
  {
    ESPcheck();//executar antes de qualquer leitura ou gravação
    readSensors();
    writeThingSpeak();
    startWriteTiming = millis();   
  }
  
  if (error==1) //Resend if transmission is not completed 
  {       
    Serial.println(" <<<< ERROR >>>>");
    digitalWrite(FREEZE_LED, HIGH);
    delay (2000);  
    goto start; //go to label "start"
  }
  //halaman 4 belum diatur untuk dihilangkan
  tampil();

  statusBtnUp   = digitalRead(btnUp);
  statusBtnOk   = digitalRead(btnOk);
  statusBtnDown = digitalRead(btnDown);

  saatUpDitekan();
  saatOkDitekan();
  saatDownDitekan();

  //  untuk button up
  if (UP && halaman == 1) {
    UP = false;
    menuItem --;
    if (menuItem < 1)menuItem = 3;
  }
  //untuk button down
  if (DOWN && halaman == 1) {
    DOWN = false;
    menuItem ++;
    if (menuItem > 3)menuItem = 1;
  }

  //  untuk button ok
  if (OK) {
    OK = false;
    if (halaman == 1 && menuItem == 1) {
      halaman = 2;
    } else if (halaman == 1 && menuItem == 2) {
      halaman = 3;
    } else if (halaman == 1 && menuItem == 3) {
      halaman = 4;
    } else if (halaman == 1 && menuItem == 4) {
      halaman = 5;
    } else if (halaman != 1){
      halaman = 1;
    } 
  }  
  delay(100);
}

/********* Read Sensors value *************/
void readSensors(void)
{
  airTemp = dht.readTemperature();
  airHum = dht.readHumidity();
             
  soilHum2 = map(analogRead(soilHum2PIN), 1023, 0, 0, 100);
  soilHum1 = map(analogRead(soilHum1PIN), 1023, 0, 0, 100); 
}

/********* Take actions based on ThingSpeak Commands *************/
void takeActions(void)
{
  Serial.print("PumpAir: ");
  Serial.println(pumpAir);
  Serial.print("PumpPupuk: ");
  Serial.println(pumpPupuk);
  Serial.print("arah: ");
  Serial.println(arah);
  if (pumpAir == 1) digitalWrite(ACTUATOR1, HIGH);
  else digitalWrite(ACTUATOR1, LOW);
  if (pumpPupuk == 1) digitalWrite(ACTUATOR2, HIGH);
  else digitalWrite(ACTUATOR2, LOW);
  if (arah == 1) servo.write(180);
  else servo.write(0);
}

/********* Read Actuators command from ThingSpeak *************/
int readThingSpeak(String channelID)
{
  startThingSpeakCmd();
  int command;
  // preparacao da string GET
  String getStr = "GET /channels/";
  getStr += channelID;
  getStr +="/fields/1/last";
  getStr += "\r\n";

  String messageDown = sendThingSpeakGetCmd(getStr);
  if (messageDown[5] == 49)
  {
    command = messageDown[7]-48; 
    Serial.print("Command received: ");
    Serial.println(command);
  }
  else command = 9;
  return command;
}

/********* Conexao com TCP com Thingspeak *******/
void writeThingSpeak(void)
{

  startThingSpeakCmd();

  // preparacao da string GET
  String getStr = "GET /update?api_key=";
  getStr += statusChWriteKey;
  getStr +="&field1=";
  getStr +="&field1=";
  getStr += String(pumpAir);
  getStr +="&field2=";
  getStr += String(pumpPupuk);
  getStr +="&field3=";
  getStr += String(airTemp);
  getStr +="&field4=";
  getStr += String(airHum);
  getStr +="&field5=";
  getStr += String(soilHum1);
  getStr +="&field6=";
  getStr += String(soilHum2);
  getStr +="&field7=";
  getStr += String(spare);
  getStr +="&field8=";
  getStr += String(arah);
  getStr += "\r\n\r\n";

  sendThingSpeakGetCmd(getStr); 
}

/********* Echo Command *************/
boolean echoFind(String keyword)
{
 byte current_char = 0;
 byte keyword_length = keyword.length();
 long deadline = millis() + 5000; // Tempo de espera 5000ms
 while(millis() < deadline){
  if (EspSerial.available()){
    char ch = EspSerial.read();
    Serial.write(ch);
    if (ch == keyword[current_char])
      if (++current_char == keyword_length){
       Serial.println();
       return true;
    }
   }
  }
 return false; // Tempo de espera esgotado
}

/********* Reset ESP *************/
void EspHardwareReset(void)
{
  Serial.println("Reseting......."); 
  digitalWrite(HARDWARE_RESET, LOW); 
  delay(500);
  digitalWrite(HARDWARE_RESET, HIGH);
  delay(8000);//Tempo necessário para começar a ler 
  Serial.println("RESET"); 
}

/********* Check ESP *************/
boolean ESPcheck(void)
{
  EspSerial.println("AT"); // Send "AT+" command to module
   
  if (echoFind("OK")) 
  {
    //Serial.println("ESP ok");
    digitalWrite(FREEZE_LED, LOW);
    return true; 
  }
  else //Freeze ou Busy
  {
    Serial.println("ESP Freeze *********************************************************************");
    digitalWrite(FREEZE_LED, HIGH);
    EspHardwareReset();
    return false;  
  }
}

/********* Start communication with ThingSpeak*************/
void startThingSpeakCmd(void)
{
  EspSerial.flush();//limpa o buffer antes de começar a gravar
  
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += "184.106.153.149"; // Endereco IP de api.thingspeak.com
  cmd += "\",80";
  EspSerial.println(cmd);
  Serial.print("enviado ==> Start cmd: ");
  Serial.println(cmd);

  if(EspSerial.find("Error"))
  {
    Serial.println("AT+CIPSTART error");
    return;
  }
}

/********* send a GET cmd to ThingSpeak *************/
String sendThingSpeakGetCmd(String getStr)
{
  String cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());
  EspSerial.println(cmd);
  Serial.print("enviado ==> lenght cmd: ");
  Serial.println(cmd);

  if(EspSerial.find((char *)">"))
  {
    EspSerial.print(getStr);
    Serial.print("enviado ==> getStr: ");
    Serial.println(getStr);
    delay(600);//tempo para processar o GET, sem este delay apresenta busy no próximo comando

    String messageBody = "";
    while (EspSerial.available()) 
    {
      String line = EspSerial.readStringUntil('\n');
      if (line.length() == 1) 
      { //actual content starts after empty line (that has length 1)
        messageBody = EspSerial.readStringUntil('\n');
        delay(50);
      }
    }
    Serial.print("MessageBody received: ");
    Serial.println(messageBody);
    return messageBody;
  }
  else
  {
    EspSerial.println("AT+CIPCLOSE");     // alert user
    Serial.println("ESP8266 CIPSEND ERROR: RESENDING"); //Resend...
    spare = spare + 1;
    error=1;
    return "error";
  } 
}

/********* Connect Wifi **************/
void connectWiFi(void)
{
  sendData("AT+RST\r\n", 2000, 0); // reset
  sendData("AT+CWJAP=\"Redmi\",\"qwertyuiop\"\r\n", 2000, 0); //Connect network
  delay(3000);
  sendData("AT+CWMODE=1\r\n", 1000, 0);
  sendData("AT+CIFSR\r\n", 1000, 0); // Show IP Adress
  Serial.println("8266 Connected");
}

String sendData(String command, const int timeout, boolean debug)
{
  String response = "";
  EspSerial.print(command);
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (EspSerial.available())
    {
      // The esp has data so display its output to the serial window
      char c = EspSerial.read(); // read the next character.
      response += c;
    }
  }
  if (debug)
  {
    Serial.print(response);
  }
  return response;
}
/*************OLED***********************/
//--------------------------------------------------------------------------------
void saatUpDitekan() {
  if (statusBtnUp != statusAkhirBtnUp) {
    if (statusBtnUp == 0) {
      UP = true;
    }
    delay(50);
  }
  statusAkhirBtnUp = statusBtnUp;
}

void saatOkDitekan() {
  if (statusBtnOk != statusAkhirBtnOk) {
    if (statusBtnOk == 0) {
      OK = true;
    }
    delay(50);
  }
  statusAkhirBtnOk = statusBtnOk;
}

void saatDownDitekan() {
  if (statusBtnDown != statusAkhirBtnDown) {
    if (statusBtnDown == 0) {
      DOWN = true;
    }
    delay(50);
  }
  statusAkhirBtnDown = statusBtnDown;
}


//----------------------------------------------------------------------------

//semua yang tampil di lcd ada di fungsi ini
void tampil() {
  if (halaman == 1) {
    lcd.clearDisplay();
    lcd.setTextSize(1);
    lcd.setTextColor(WHITE);
    lcd.setCursor(30, 0);
    lcd.print("DAFTAR MENU");

    if (menuItem == 1) {
      lcd.setCursor(2, 17);
      lcd.setTextColor(WHITE);
      lcd.print("> Cek kondisi tanaman");
    } else {
      lcd.setCursor(2, 17);
      lcd.setTextColor(WHITE);
      lcd.print("  Cek kondisi tanaman");
    }

    if (menuItem == 2) {
      lcd.setCursor(2, 27);
      lcd.setTextColor(WHITE);
      lcd.print("> Siram tanaman");
    } else {
      lcd.setCursor(2, 27);
      lcd.setTextColor(WHITE);
      lcd.print("  Siram tanaman");
    }

    if (menuItem == 3) {
      lcd.setCursor(2, 37);
      lcd.setTextColor(WHITE);
      lcd.print("> Beri pupuk");
    } else {
      lcd.setCursor(2, 37);
      lcd.setTextColor(WHITE);
      lcd.print("  Beri pupuk");
    }
    lcd.display();

  } else if (halaman == 2) {
    lcd.clearDisplay();
    drawThermometer(37, 4);

    lcd.setTextSize(1);
    lcd.setTextColor(WHITE);
    lcd.setCursor(1,14);
    lcd.println("TEMP:");
    lcd.setCursor(4, 25);
    lcd.print(airTemp);
    lcd.print("oC");

    drawHumidity(107, 14);
    lcd.setTextSize(1);
    lcd.setTextColor(WHITE);
    lcd.setCursor(65,5);
    lcd.println("SOIL:");
    lcd.setCursor(65, 15);
    lcd.print(soilHum1); //humidity tanah
    lcd.print("%");
    
    lcd.setCursor(65,35);
    lcd.println("AIR:");
    lcd.setCursor(65, 45);
    lcd.print(airHum); //humidity udara
    lcd.print("%");
    
    lcd.setTextSize(0.5);
    lcd.setCursor(0, 56);
    lcd.print("Tekan ok utk kembali."); //humidity udara
    lcd.display();

  } else if (halaman == 3) {
    unsigned long startTime = millis();
    unsigned long currentTime = millis();
    lcd.clearDisplay();
    lcd.setTextSize(1);
    lcd.setTextColor(WHITE);
    lcd.setCursor(0, 0);
    lcd.print("Proses penyiraman\nsedang berlangsung.\n\nJangan tekan\ntombol apapun!");
    lcd.display();
    
    while(currentTime - startTime < 5000){ //tunggu hingga penyiraman selesai
      currentTime = millis();

    }

    //masih jadi pr
    lcd.clearDisplay();
    while(OK == false)
    {
      lcd.setCursor(0, 0);
      lcd.setTextSize(1);
      lcd.setTextColor(WHITE);
      lcd.print("Tekan OK untuk\nkembali...");
      lcd.display();
      statusBtnOk   = digitalRead(btnOk);
      saatOkDitekan();
    }

    if(OK){
      OK = false;
      halaman = 1;
    }

  } else if (halaman == 4) {
    unsigned long startTime = millis();
    unsigned long currentTime = millis();
    lcd.clearDisplay();
    lcd.setTextSize(1);
    lcd.setTextColor(WHITE);
    lcd.setCursor(0, 0);
    lcd.print("Pemberian pupuk\nsedang berlangsung.");
    lcd.print("\n\nJangan tekan\ntombol apapun!");
    lcd.display();
    
    while(currentTime - startTime < 5000){ //tunggu hingga pemupukan selesai
      currentTime = millis();
    }

    //oemberian pupuk

    lcd.clearDisplay();
    while(OK == false)
    {
      lcd.setCursor(0, 0);
      lcd.setTextSize(1);
      lcd.setTextColor(WHITE);
      lcd.print("Tekan OK untuk\nkembali...");
      lcd.display();
      statusBtnOk   = digitalRead(btnOk);
      saatOkDitekan();
    }

    if(OK){
      OK = false;
      halaman = 1;
    }
  } 
}

void drawThermometer(int x_start, int y_start)
{//x = 90, y = 14 (37,9)
  lcd.drawLine(x_start, y_start+1, x_start, y_start+32, WHITE);
  lcd.drawLine(x_start+10, y_start+1, x_start+10, y_start+32, WHITE);
  lcd.drawCircle(x_start+5, y_start+31+9, 9, WHITE);
  lcd.fillCircle(x_start+5, y_start+31+9, 5, WHITE);
  //fillRect(upperLeftX, upperLeftY, width, height, WHITE) (40, 30)
  lcd.fillRect(x_start+3, y_start+21, 5, 16, WHITE);
  lcd.drawPixel(x_start+1, y_start+32, BLACK);
  lcd.drawPixel(x_start+2, y_start+32, BLACK);
  lcd.drawPixel(x_start+8, y_start+32, BLACK);
  lcd.drawPixel(x_start+9, y_start+32, BLACK);
  lcd.drawCircle(x_start+5, y_start+1, 5, WHITE);

  lcd.drawLine(x_start+3,y_start+21, x_start+3, y_start+1, WHITE);
  lcd.drawLine(x_start+3+4,y_start+21, x_start+3+4, y_start+1, WHITE);
  
  for(int i = 0; i <= 2; i++)
  {
    lcd.drawPixel(x_start+4+i, y_start, WHITE);
  }

  for(int i = -1; i <= 1; i++)
  {
    lcd.drawPixel(x_start+5+i, y_start+6, BLACK);
  }
  lcd.drawPixel(x_start+5-3, y_start+5, BLACK);
  lcd.drawPixel(x_start+5+3, y_start+5, BLACK);
  lcd.drawPixel(x_start+5-4, y_start+4, BLACK);
  lcd.drawPixel(x_start+5+4, y_start+4, BLACK);
}

void drawHumidity(int x_start, int y_start)
{//(107, 16)
  lcd.fillCircle(x_start, y_start+22, 11, WHITE);
  lcd.drawLine(x_start, y_start, x_start-9, y_start+15, WHITE);
  lcd.fillTriangle(x_start, y_start-1, x_start+8, y_start+14, x_start-9, y_start+15, WHITE);
  //(103,29)
  lcd.fillRect(x_start-4, y_start+13, 2, 2, BLACK);
  lcd.fillRect(x_start-6, y_start+17, 2, 8, BLACK);
  lcd.fillRect(x_start-4, y_start+25, 2, 2, BLACK);
  //(101, 33)

  //(108,32)
  lcd.drawCircle(x_start+1, y_start+17, 1, BLACK);
  lcd.drawCircle(x_start+6, y_start+21, 1, BLACK);//(113,36)
  lcd.drawLine(x_start+5, y_start+16, x_start+1, y_start+22, BLACK);//112,31 and 108,38

}
