/* Used code from http://arduino.cc/en/Tutorial/SimpleRTCAlarm and https://forum.arduino.cc/index.php?topic=553612.0
*/
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <RTCZero.h>
#include <U8g2lib.h>
#include <U8x8lib.h>

U8G2_T6963_256X64_F_8080 u8g2(U8G2_R0, 12, 1, 11, 2, 7, 3, 19, 4, /*enable/wr=*/ 17, /*cs/ce=*/ 18, /*dc=*/ 21, /*reset=*/ 0); // Connect RD with +5V, FS0 and FS1 with GND

byte mac[] = {0xA8, 0x61, 0x0A, 0xAE, 0x5A, 0x4B};
//byte mac[] = {0xA8, 0x61, 0x0A, 0xAE, 0x5A, 0x3A};

const char timeServer[] = "time.nist.gov";
// IPAddress ip(192, 168, 1, 188); //disable for DHCP
// IPAddress myDns(192, 168, 1, 1); //disable for DHCP
// IPAddress gateway(192, 168, 1, 1); //disable for DHCP
// IPAddress subnet(255, 255, 255, 0); //disable for DHCP

unsigned int localPort = 8888;       // local port to listen for UDP packets

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
EthernetUDP Udp; // A UDP instance to let us send and receive packets over UDP

String message;

RTCZero rtc;

volatile byte flag = LOW;
volatile byte startup = HIGH;
volatile byte noEth = LOW;

uint16_t counter = 0;
uint16_t sendInterval = 601; //time in seconds between NTP requests
uint8_t checkInterval = 1;

unsigned long startMillis;
unsigned long currentMillis;
const unsigned long period = 1000;

const byte heartbeat = 16; //blinking led @D16

void setup()
{
  Ethernet.init(5);   // MKR ETH shield
  Ethernet.begin(mac); //DHCP
//Ethernet.begin(mac, ip, myDns, gateway, subnet); //static IP (DNS, GW, SN are optional)
  Udp.begin(localPort);
  
  pinMode(heartbeat, OUTPUT);
  pinMode(20, OUTPUT);
  digitalWrite(20, HIGH);
  
 // Serial.begin(115200);

  rtc.begin(); // initialize RTC 24H format
  rtc.enableAlarm(rtc.MATCH_SS); //set interrupt mask to match seconds
  rtc.attachInterrupt(tick); //ISR
  //Serial.print("LCD INIT");

  u8g2.begin();
  
  u8g2.setFont(u8g2_font_t0_11_t_all);
  u8g2.setCursor(10, 10);
  u8g2.print("Initializing...");
  u8g2.setCursor(10, 20);
  u8g2.print("IP: ");
  u8g2.print(Ethernet.localIP());
  u8g2.sendBuffer();
  delay(2000);

  startMillis = millis();
}

void loop(){
  currentMillis = millis();
  if (currentMillis - startMillis >= period) {
    printRTCtime();
 //   Serial.println("Millis took over!");
    startMillis = millis();
  }
  
  if(Ethernet.linkStatus() != LinkON) {   
    message = "ETHERNET DISCONNECTED!";
    noEth = HIGH;
    startup = HIGH;
  }
  else {
    message = "";
    noEth = LOW;
  }
  
  if (startup == HIGH && noEth == LOW) {
    sendNTPpacket(timeServer); // send an NTP packet to a time server
    startup = LOW;
  //  Serial.println("First packet sent");
  }

  if (counter >= sendInterval) {
    counter = sendInterval -1; // if NTP response hasn't arrived, send a new one in one second
    sendNTPpacket(timeServer); // send an NTP packet to a time server
 //   Serial.print("Counter value: ");
 //   Serial.println(counter);
 //   Serial.println("NTP packet sent");
      }
  
  if (Udp.parsePacket()) {
    counter = 0;
 //   Serial.println("Packet recieved");
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    // the timestamp starts at byte 40 of the received packet and is four bytes,
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
    // Serial.println(epoch);
    rtc.setEpoch(epoch);
    // Serial.println("Time set on RTC");
}}

void sendNTPpacket(const char * address) { // send an NTP request to the time server at the given address
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root delay & Root Dispersion
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
     startMillis = millis(); // Show that the interrupt has occured
     //Serial.println("TICK");
     printRTCtime();
     counter++;
     //Serial.print("Counter value: ");
     //Serial.println(counter);
     digitalWrite(heartbeat, !digitalRead(heartbeat));                    // Toggle digital pin D16
     RTC->MODE2.INTFLAG.reg = RTC_MODE2_INTFLAG_ALARM0;                   // Reset interrupt flag     
     RTC->MODE2.Mode2Alarm[0].ALARM.bit.SECOND = (RTC->MODE2.Mode2Alarm[0].ALARM.bit.SECOND + 1) % 60;   // Increment the ALARM0 compare register
     while (RTC->MODE2.STATUS.bit.SYNCBUSY);                              // Wait for synchronization
  }
}

void printRTCtime() {
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_fub42_tf); // choose a suitable font
  u8g2.setCursor(0, 55);
  //Serial.println("New second on display");
  print2digits(rtc.getHours());
  u8g2.print(":");
  print2digits(rtc.getMinutes());
  u8g2.print(":");  
  print2digits(rtc.getSeconds());
  u8g2.setFont(u8g2_font_t0_11_t_all);
  u8g2.setCursor(100, 10);
  u8g2.print(message);
  u8g2.sendBuffer();
}

void print2digits(int number) {
  if (number < 10) {
    u8g2.print("0");
  }
  u8g2.print(number); 
}
