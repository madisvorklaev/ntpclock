

/*
  Simple RTC Alarm for Arduino Zero and MKR1000

  Demonstrates how to set an RTC alarm for the Arduino Zero and MKR1000

  This example code is in the public domain

  http://arduino.cc/en/Tutorial/SimpleRTCAlarm

  created by Arturo Guadalupi <a.guadalupi@arduino.cc>
  25 Sept 2015
 
  modified
  21 Oct 2015
*/
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <RTCZero.h>
#include <U8g2lib.h>
#include <U8x8lib.h>

byte mac[] = {0xA8, 0x61, 0x0A, 0xAE, 0x5A, 0x4B};
IPAddress ip(192, 168, 2, 188);
unsigned int localPort = 8888;       // local port to listen for UDP packets
const char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

/* Create an rtc object */
RTCZero rtc;

volatile byte flag = LOW;
volatile byte startup = HIGH;

uint8_t counter = 0;
uint8_t sendInterval = 10;
uint8_t checkInterval = 1;

const byte ledPin = 6;

////* Change these values to set the current initial time */
//const byte seconds = 0;
//const byte minutes = 0;
//const byte hours = 16;
//
////* Change these values to set the current initial date */
//const byte day = 25;
//const byte month = 9;
//const byte year = 15;

void setup()
{
  Ethernet.init(5);   // MKR ETH shield
  Ethernet.begin(mac, ip);
  Udp.begin(localPort);
  
  Serial.begin(9600);

  rtc.begin(); // initialize RTC 24H format

//  rtc.setTime(hours, minutes, seconds);
//  rtc.setDate(day, month, year);

  rtc.enableAlarm(rtc.MATCH_SS);
 
  rtc.attachInterrupt(tick);
}

void loop()
{
  if (flag == HIGH) {
    printRTCtime();
    flag = LOW;
  } 
  
  if (startup == HIGH) {
    sendNTPpacket(timeServer); // send an NTP packet to a time server
    startup = LOW;
  }

  if (counter >= sendInterval) {
    sendNTPpacket(timeServer); // send an NTP packet to a time server
    counter = 0;
  }
  
  if (Udp.parsePacket()) {
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
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
    Serial.println(epoch);
    rtc.setEpoch(epoch);
}}

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

void tick(void)
{
  if (RTC->MODE2.INTFLAG.bit.ALARM0 && RTC->MODE2.INTENSET.bit.ALARM0)    // Check for ALARM0 interrupt
  { 
     //SerialUSB.print(F("RTC Handler  "));
     //SerialUSB.println(RTC->MODE2.Mode2Alarm[0].ALARM.bit.SECOND);
     //SerialUSB.println(rtc.getEpoch());
     flag = HIGH;
     counter++;
     digitalWrite(ledPin, !digitalRead(ledPin));                           // Toggle digital pin D6
     RTC->MODE2.INTFLAG.reg = RTC_MODE2_INTFLAG_ALARM0;                   // Reset interrupt flag     
     RTC->MODE2.Mode2Alarm[0].ALARM.bit.SECOND = (RTC->MODE2.Mode2Alarm[0].ALARM.bit.SECOND + 1) % 60;   // Increment the ALARM0 compare register
     while (RTC->MODE2.STATUS.bit.SYNCBUSY);                              // Wait for synchronization
  }
}

void printRTCtime() {
  print2digits(rtc.getHours());
  Serial.print(':');
  print2digits(rtc.getMinutes());
  Serial.print(':');
  print2digits(rtc.getSeconds());
  Serial.println();
}

void print2digits(int number) {
  if (number < 10) {
    Serial.print("0");
  }
  Serial.print(number);
}
