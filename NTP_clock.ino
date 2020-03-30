/*
Thanks to: Michael Margolis, Tom Igoe, Arturo Guadalupi

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
volatile byte state = LOW;
int kkk=0;

unsigned long previousSendMillis = 0;
unsigned long previousCheckMillis = 0;
const long sendInterval = 900000;
//const long sendInterval = 3000;
const long checkInterval = 1000;

int seconds;
int lastSecond;
bool flag;
bool startupFlag = 1;

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

void setup() {

  Wire.begin();
  Wire.beginTransmission(0x68);
  Wire.write(0x07);
  Wire.write(0x10);  // Set Square Wave to 1 Hz
  Wire.endTransmission();  
  RTC.begin();
//  RTC.writeSqwPinMode(DS1307_SquareWave1Hz);
//  RTC.squareWave(SQWAVE_1_HZ);
  lcd.begin();
  //lcd.backlight();
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
  attachInterrupt(digitalPinToInterrupt(interruptPin), blink, FALLING);

  /*
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    //Serial.println("Ethernet shield was not found.");
  }
  else if (Ethernet.hardwareStatus() == EthernetW5100) {
    //Serial.println("W5100 Ethernet controller detected.");
  }
  else if (Ethernet.hardwareStatus() == EthernetW5200) {
    //Serial.println("W5200 Ethernet controller detected.");
  }
  else if (Ethernet.hardwareStatus() == EthernetW5500) {
    //Serial.println("W5500 Ethernet controller detected.");
  }
  
  // start Ethernet and UDP
  if (Ethernet.begin(mac) == 0) {
    //Serial.println("Failed to configure Ethernet using DHCP");
    lcd.clear();
    lcd.setCursor(1,0);
    lcd.print("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      //Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      //Serial.println("Ethernet cable is not connected.");
      lcd.clear();
      lcd.setCursor(1,0);
      lcd.print("nocable");
    }
    // no point in carrying on, so do nothing forevermore:
    while (true) {
      delay(1);
    }
    
  }
  */
 /* 
// initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 6250;            // compare match register 16MHz/256/1Hz (16000000Hz/256val/1Hz = 62500 = 1sek)
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS12);    // 256 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  interrupts();             // enable all interrupts
}

ISR(TIMER1_COMPA_vect){    // timer compare interrupt service routine
  flag = 1;                // tick every second
*/
}

void loop() {
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
     
      /*Serial.print("highWord: ");
      Serial.println(highWord);
      Serial.print("lowWord: ");
      Serial.println(lowWord);
*/
/*
      // https://forum.arduino.cc/index.php?topic=526792.0
      // Combine the 4 timestamp bytes into one 32-bit number
      uint32_t NTPTime = (packetBuffer[40] << 24) | (packetBuffer[41] << 16) | (packetBuffer[42] << 8) | packetBuffer[43];     
      // Now get the fractional part
      uint32_t NTPmillis = (packetBuffer[44] << 24) | (packetBuffer[45] << 16) | (packetBuffer[46] << 8) | packetBuffer[47];
      int32_t fractionalPart = (int32_t)(((float)NTPmillis / UINT32_MAX) * 1000);

   
      // Increment the seconds as we are waiting for the next one
      NTPTime++;  

      // Burn off the remaining fractional part of the existing second
      delay(1000 - fractionalPart);  
*/
   
   /* Serial.print("NTPtime: ");
      Serial.println(NTPTime);
      Serial.print("NTPmillis: ");
      Serial.println(NTPmillis);
      Serial.print("Fractional: ");
      Serial.println(fractionalPart);
*/

      
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;
  
      // now convert NTP time into everyday time:
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      // subtract seventy years:
      unsigned long epoch = secsSince1900 - seventyYears;
      RTC.adjust(DateTime(epoch));
  }}

  if(state = !state){
    printRTCtime();
   }

    /*
    int hours = ((epoch  % 86400L) / 3600);
    int minutes = ((epoch  % 3600) / 60);
    int seconds = (epoch % 60);
    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if (((epoch % 3600) / 60) < 10) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ((epoch % 60) < 10) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second
  }
  //delay(100);
  //Ethernet.maintain(); //hold DHCP session
  */
}

// send an NTP request to the time server at the given address
void sendNTPpacket(const char * address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
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
  lcd.setCursor(0, 0);
  lcd.print(millis() - previousSendMillis);
  lcd.setCursor(0, 1);
  print2digits(now.hour());
  lcd.print(':');
  print2digits(now.minute());
  lcd.print(':');
  print2digits(now.second()+1);
  
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

  /*
  lcd.print(now.hour(), DEC);
  lcd.print(':');
  lcd.print(now.minute(), DEC);
  lcd.print(':');
  lcd.print(now.second(), DEC);
  */
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

void blink() {
  state = !state;
  Serial.println(state);
  digitalWrite(ledPin, state);

}
