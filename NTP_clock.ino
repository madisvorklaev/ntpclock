/*
Thanks to: Michael Margolis, Tom Igoe, Arturo Guadalupi, Phil at https://forum.arduino.cc/index.php?topic=526792.0
This code is in the public domain.
*/

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <RTClib.h>
RTC_DS1307 RTC;
LiquidCrystal_I2C lcd(0x3F,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 2, 187);
const char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server

unsigned int localPort = 8888;       // local port to listen for UDP packets
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

const byte ledPin = 13;
const byte interruptPin = 7;
volatile byte flag = LOW;
volatile byte startup = HIGH;
const int counter = 0;

unsigned long previousSendMillis = 0;
unsigned long previousCheckMillis = 0;
const long sendInterval = 10000;
const long checkInterval = 1000;

int seconds;
int lastSecond;


// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

void setup() {

  Wire.begin();
  Wire.beginTransmission(0x68);
  Wire.write(0x07);
  Wire.write(0x10);  // Set Square Wave to 1 Hz
  Wire.endTransmission();  
  RTC.begin();
  lcd.begin();
  if (! RTC.isrunning()) {
    lcd.setCursor(0,0);
    lcd.print("RTC is NOT running!");
    while (true) {
      delay(1);
    }
  }
  
  Ethernet.init(10);  // CS pin
  //Ethernet.init(5);   // MKR ETH shield
  Ethernet.begin(mac, ip);
  Udp.begin(localPort);

  Serial.begin(9600);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("IP is: ");
  lcd.setCursor(0,1);
  lcd.print(ip);
  delay(2000);
  lcd.clear();

  pinMode(ledPin, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), setFlag, FALLING);
  }
  
void loop() {
  if(startup == HIGH) {
    sendNTPpacket(timeServer); // send an NTP packet to a time server
    startup = LOW;
  }
  unsigned long currentMillis = millis();
  if (currentMillis - previousSendMillis >= sendInterval) {
    previousSendMillis = currentMillis;
    sendNTPpacket(timeServer); // send an NTP packet to a time server
  }
  if (currentMillis - previousCheckMillis >= checkInterval) {
    previousCheckMillis = currentMillis;  
    if (Udp.parsePacket()) {
      // We've received a packet, read the data from it
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
      Serial.print("packetBuffer: ");
      Serial.println(word(packetBuffer, NTP_PACKET_SIZE));
      // the timestamp starts at byte 40 of the received packet and is four b ytes,
      // or two words, long. First, extract the two words:
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      // now convert NTP time into everyday time:
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      // subtract seventy years:
      unsigned long epoch = secsSince1900 - seventyYears;
      RTC.adjust(DateTime(epoch));
      Wire.beginTransmission(0x68);
      Wire.write(0x07);
      Wire.write(0x10);  // Set Square Wave to 1 Hz
      Wire.endTransmission(); 
  }}

  if(flag == HIGH){
    printRTCtime();
    flag = LOW;
   }
}

void sendNTPpacket(const char * address) { // send an NTP request to the time server at the given address
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void printRTCtime(){
  DateTime now = RTC.now();
  lcd.setCursor(0, 1);
  print2digits(now.hour());
  lcd.print(':');
  print2digits(now.minute());
  lcd.print(':');
  print2digits(now.second());
  
  //unsigned long unix = now.unixtime();
  //Serial.print(now.year());
  //Serial.print('/');
  //Serial.print(now.month());
  //Serial.print('/');
  //Serial.print(now.day());
  //Serial.print(' ');
  //Serial.print(now.hour(), DEC);
  //Serial.print(':');
  //Serial.print(now.minute(), DEC);
  //Serial.print(':');
  //Serial.print(now.second(), DEC);
  //Serial.println(); 
}

void print2digits(int number) {
  if (number == 60) {
    lcd.print("00");
  }
  else{
  if (number >= 0 && number < 10) {
    lcd.print('0');
  }
  lcd.print(number);
}}

void checkEpoch(unsigned long ntpTime){
  DateTime now = RTC.now();
  unsigned long rtcTime = now.unixtime();
  lcd.setCursor(0, 0);
  lcd.print(ntpTime);
  lcd.println(" NTP");
  lcd.setCursor(0, 1);
  lcd.print(rtcTime);
  lcd.println(" RTC");
}

void setFlag() {
  flag = HIGH;
  counter ++;
  digitalWrite(ledPin, !digitalRead(ledPin));
}
