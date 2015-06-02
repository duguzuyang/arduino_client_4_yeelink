/*
 Yeelink sensor client example
 */

#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <math.h>
#include <dht.h> 
#define dht_dpin  A0
#define measurePin A2 

dht DHT;

byte buff[2];

// for yeelink api
#define APIKEY         "bfe34a96d4a264e0bbfee0e9c7151d3a" // replace your yeelink api key here
#define DEVICEID       21474 // replace your device ID
#define THERMID        37851 // replace your sensor ID
#define HUMIDID        37855 // replace your sensor ID

char lightPin = A1;
int ledPower = 2;    // 连接数字口2
char thermID[] = "37851";
char humidID[] = "37855";
char lightID[] = "37958";
char dustID[]  = "38078";
int i=4;

// assign a MAC address for the ethernet controller.
byte mac[] = { 0x00, 0x1D, 0x72, 0x82, 0x35, 0x9D};
// initialize the library instance:
EthernetClient client;
char server[] = "api.yeelink.net";   // name address for yeelink API
//IPAddress server(42,96,164,52);      // numeric IP for api.yeelink.net

unsigned long lastConnectionTime = 0;          // last time you connected to the server, in milliseconds
boolean lastConnected = false;                 // state of the connection last time through the main loop
const unsigned long postingInterval = 1*1000; // delay between 2 datapoints, 30s

void setup() {
  pinMode(ledPower,OUTPUT);
  Wire.begin();
  // start serial port:
  Serial.begin(115200);
  // start the Ethernet connection with DHCP:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    for(;;)
      ;
  }
  else {
    Serial.println("Ethernet configuration OK");
  }
}

void loop() {
  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  DHT.read11(dht_dpin);
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }
  
  // if there's no net connection, but there was one last time
  // through the loop, then stop the client:
  if (!client.connected() && lastConnected) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
  
  // if you're not connected, and ten seconds have passed since
  // your last connection, then connect again and send data:
  if(!client.connected() && (millis() - lastConnectionTime > postingInterval)) {
    // read sensor data, replace with your code
    int thermval = getthermval();
    int humidval = gethumidval();
    int lightval = getlightval(lightPin);
    int dustval  = averageDust();
    Serial.println("yeelink:");
    Serial.print(thermval);
    Serial.println(" C");
    Serial.print(humidval);
    Serial.println(" %");
    Serial.println(lightval);
    Serial.print(dustval);
    Serial.println(" ug/m3");
    //send data to server
    if(humidval <= 100 && humidval > 0 && dustval > 0){
      if(i % 4 == 0){
        sendData(thermval,thermID);
        Serial.println("Sending therm data");
        i++;
      }
      else if(i % 4 == 1){
        sendData(lightval,lightID);
        Serial.println("Sending light data");
        i++;
      }
      else if(i % 4 == 2){
        sendData(dustval,dustID);
        Serial.println("Sending dust data");
        i++;
      }
      else{
        sendData(humidval,humidID);
        Serial.println("Sending humid data");
        i -= 3;
      }
    }
  }
  // store the state of the connection for next time through
  // the loop:
  lastConnected = client.connected();
}

// this method makes a HTTP connection to the server:
void sendData(int Data,char sensorID[]) {
  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("connecting...");
    // send the HTTP PUT request:
    client.print("POST /v1.0/device/");
    client.print(DEVICEID);
    client.print("/sensor/");
    client.print(sensorID);
    client.print("/datapoints");
    client.println(" HTTP/1.1");
    client.println("Host: api.yeelink.net");
    client.print("Accept: *");
    client.print("/");
    client.println("*");
    client.print("U-ApiKey: ");
    client.println(APIKEY);
    client.print("Content-Length: ");

    // calculate the length of the sensor reading in bytes:
    // 8 bytes for {"value":} + number of digits of the data:
    int thisLength = 10 + getLength(Data);
    client.println(thisLength);
    
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Connection: close");
    client.println();

    // here's the actual content of the PUT request:
    client.print("{\"value\":");
    client.print(Data);
    client.println("}");
  } 
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
   // note the time that the connection was made or attempted:
  lastConnectionTime = millis();
}

// this method makes a HTTP connection to the server:
void sendhumidData(int humidData) {
  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("connecting...");
    // send the HTTP PUT request:
    client.print("POST /v1.0/device/");
    client.print(DEVICEID);
    client.print("/sensor/");
    client.print(37855);
    client.print("/datapoints");
    client.println(" HTTP/1.1");
    client.println("Host: api.yeelink.net");
    client.print("Accept: *");
    client.print("/");
    client.println("*");
    client.print("U-ApiKey: ");
    client.println(APIKEY);
    client.print("Content-Length: ");

    // calculate the length of the sensor reading in bytes:
    // 8 bytes for {"value":} + number of digits of the data:
    int thisLength = 10 + getLength(humidData);
    client.println(thisLength);
    
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Connection: close");
    client.println();

    // here's the actual content of the PUT request:
    client.print("{\"value\":");
    client.print(humidData);
    client.println("}");
  } 
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
   // note the time that the connection was made or attempted:
  lastConnectionTime = millis();
}


// This method calculates the number of digits in the
// sensor reading.  Since each digit of the ASCII decimal
// representation is a byte, the number of digits equals
// the number of bytes:
int getLength(int someValue) {
  // there's at least one byte:
  int digits = 1;
  // continually divide the value by ten, 
  // adding one to the digit count for each
  // time you divide, until you're at 0:
  int dividend = someValue /10;
  while (dividend > 0) {
    dividend = dividend /10;
    digits++;
  }
  // return the number of digits:
  return digits;
}

int getlightval(char pin) {
  //take average of 3 readings as the final value
  int val1 = 0, val2 = 0, val3 = 0, val = 0;
  val1 = analogRead(pin);
  val2 = analogRead(pin);
  val3 = analogRead(pin);
  val = (val1 + val2 + val3) / 3;
  return val;
}

int getthermval(){
  int val1 = 0, val2 = 0, val3 = 0, val = 0;
  val1 = DHT.temperature;
  val2 = DHT.temperature;
  val3 = DHT.temperature;
  val = (val1 + val2 + val3) / 3;
  return val;
}

int gethumidval(){
  int humidval = DHT.humidity;
  int val1 = 0, val2 = 0, val3 = 0, val = 0;
  val1 = DHT.humidity;
  val2 = DHT.humidity;
  val3 = DHT.humidity;
  val = (val1 + val2 + val3) / 3;
  return val;
}
  
float getdustval(){
  int samplingTime = 280;
  int deltaTime = 40;
  int sleepTime = 9680;
  
  float voMeasured = 0;
  float calcVoltage = 0;
  float dustDensity = 0;
  
  digitalWrite(ledPower,LOW);       //开启内部LED
  delayMicroseconds(samplingTime);  // 开启LED后的280us的等待时间
  
  voMeasured = analogRead(measurePin);   // 读取模拟值
  
  delayMicroseconds(deltaTime);        //  40us等待时间
  digitalWrite(ledPower,HIGH);         // 关闭LED
  delayMicroseconds(sleepTime);
  
  // 0 - 5V mapped to 0 - 1023 integer values
  // recover voltage
  calcVoltage = voMeasured * (5.0 / 1024.0);   //将模拟值转换为电压值
  // linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
  // Chris Nafis (c) 2012
  dustDensity = 0.17 * calcVoltage - 0.1;
  return dustDensity;
}
  
float averageDust(){
  float val1 = getdustval();
  float val2 = getdustval();
  float val3 = getdustval();
  int val = 1000 * (val1 + val2 + val3)/3;   //  ug/m3
  return val;
}
