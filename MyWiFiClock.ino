#include "sceleton.h"

#include <WiFiUdp.h>
#include <core_esp8266_waveform.h>

#include "worklogic.h"
#include "lcd.h"

unsigned int localPort = 2390;      // local port to listen for UDP packets

IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

LcdScreen screen;
MAX72xx screenController(screen, D5, D7, D6);

#define BEEPER_PIN D2 // Beeper

int testCntr = 0;

void setup() {
  // Initialize comms hardware
  pinMode(BEEPER_PIN, OUTPUT);

  screenController.setup();

  sceleton::setup();

  sceleton::showMessageSink = [](const char* dd) {
    // 
    screenController.showMessage(dd);
  };

  udp.begin(localPort);

  WiFi.hostByName(ntpServerName, timeServerIP); 
}

unsigned long oldMicros = micros();

uint32_t timeRetreivedInMs = 0;
uint32_t initialUnixTime = 0;
uint32_t timeRequestedAt = 0;

uint16_t hours = 0;
uint16_t mins = 0;
uint64_t nowMs = 0;
boolean sleeps = false;

const int updateTimeEachSec = 600; // By default, update time each 600 seconds

void loop() {
  if (millis() % 5*60*1000 == 0 && (((millis() - timeRequestedAt) > (timeRetreivedInMs == 0 ? 5 : updateTimeEachSec)*1000))) {
    debugPrint("Requesting time");
    timeRequestedAt = millis();
    sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  }

  oldMicros = micros();
  testCntr++;

  screen.clear();
  boolean wasSleeping = sleeps;

  if (timeRetreivedInMs) {
    updateTime();
    if (!sleeps) {
      uint64_t dayInMs = 24*60*60*1000;
      screen.showTime(nowMs / dayInMs, nowMs % dayInMs);

      screenController.refreshAll();
    }

    int m = (hours*100 + mins);
    sleeps = m > 2330 || m < 540; // From 22:30 to 5:30 - do not show screen

    if (sleeps) {
      if (!wasSleeping) {
        debugPrint("Falling asleep");
      }
      screenController.refreshAll();
    }
  } else {
    const wchar_t* getTime = L"  Получаем время с сервера...  ";
    screen.printStr((micros() / 1000 / 50) % screen.getStrWidth(getTime), 0, getTime);
    screenController.refreshAll();
  }

  sceleton::loop();

  int cb = udp.parsePacket();
  if (cb >= NTP_PACKET_SIZE) {
    debugPrint("packet received, length=" + String(cb, DEC));
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    String pckt;
    for (int i = 0; i < cb; ++i) {
      if (i > 0) {
        pckt += " ";
      }
      pckt += String(packetBuffer[i], HEX);
    }
    debugPrint(pckt);

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    debugPrint("Seconds since Jan 1 1900 = " + String(secsSince1900, DEC));

    // Sometimes we get broken time. In case 2018 does not appear yet, let's just ignore it silently
    if (secsSince1900 > 118*365*24*60*60) {
      // now convert NTP time into everyday time:
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const uint64_t seventyYears = 2208988800ULL;
      // subtract seventy years:
      unsigned long epoch2 = secsSince1900 - seventyYears;

      timeRetreivedInMs = millis();
      initialUnixTime = epoch2;
      
      debugPrint("Unix time = " + String(epoch2, DEC));

      // print the hour, minute and second:

      updateTime();
    } else {
      debugPrint("INVALID TIME RECEIVED!");
    }
  }
}

void updateTime() {
  // UTC is the time at Greenwich Meridian (GMT)
  // print the hour (86400 equals secs per day)
  nowMs = initialUnixTime * 1000ull + ((uint64_t)millis() - (uint64_t)timeRetreivedInMs);
  nowMs += 3*60*60*1000; // Timezone (UTC+3)

  uint32_t epoch = nowMs/1000ull;
  hours = (epoch % 86400L) / 3600;
  mins = (epoch % 3600) / 60;

  // debugPrint("Time = " + String(hours, DEC) + ":" + String(mins, DEC));
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address) {
  debugPrint("sending NTP packet...");
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
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
