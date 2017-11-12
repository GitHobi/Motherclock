#include "RTClib.h"
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <TimeLib.h>
#include <NtpClientLib.h>


extern "C" {
#include "user_interface.h"
}

extern "C" {
#include "gpio.h"
}

#define  BUILTIN_LED1  2 //GPIO2


PCF8583 rtc;
DateTime alarm_l;
const byte rtc_int = 14;

#include "WifiConfig.h"

#define MAX 13

#define ONBOARDLED1 2  // Built in LED on ESP-12/ESP-07
#define ONBOARDLED2 D0 // Built in LED on ESP-12/ESP-07
int8_t timeZone = 1;

void showCurrentStatus()
{
  char buf[100];
  strncpy(buf, "hh:mm:ss\0", 100);
  DateTime now = rtc.now();
  Serial.print("RTC Time: ");
  Serial.print(now.format(buf));

  Serial.print(" - NTP Time: ");
  Serial.print(NTP.getTimeDateString()); Serial.print(" ");
  Serial.print(NTP.isSummerTime() ? "Summer Time. " : "Winter Time. ");
  Serial.print("WiFi is ");
  Serial.print(WiFi.isConnected() ? "connected" : "not connected"); Serial.print(". ");
  Serial.print("Uptime: ");
  Serial.print(NTP.getUptimeString()); Serial.print(" since ");
  Serial.println(NTP.getTimeDateString(NTP.getFirstSync()).c_str());
}


//
// Start NTP only after IP network is connected
//
void onSTAGotIP(WiFiEventStationModeGotIP ipInfo) {
  Serial.printf("Got IP: %s\r\n", ipInfo.ip.toString().c_str());
  //NTP.begin("pool.ntp.org", timeZone, true);
  NTP.begin("192.168.20.1", timeZone, true);
  NTP.setInterval(63); // sync alle 15 Minuten
}

//
// Manage network disconnection
//
void onSTADisconnected(WiFiEventStationModeDisconnected event_info) {
  Serial.printf("Disconnected from SSID: %s\n", event_info.ssid.c_str());
  Serial.printf("Reason: %d\n", event_info.reason);
  //NTP.stop();
}

//
// Handle NTP sync event
//
void processSyncEvent(NTPSyncEvent_t ntpEvent) {
  if (ntpEvent) {
    Serial.print("Time Sync error: ");
    if (ntpEvent == noResponse)
      Serial.println("NTP server not reachable");
    else if (ntpEvent == invalidAddress)
      Serial.println("Invalid NTP server address");
  }
  else {
    Serial.print("Got NTP time: ");
    Serial.println(NTP.getTimeDateString(NTP.getLastNTPSync()));
    rtc.adjust ( now() );
    showCurrentStatus();
    if ( rtc.now().second() > 0 )
    {
      setAlarmToNextFullMinute();
    }
  }
}

boolean syncEventTriggered = false; // True if a time even has been triggered
NTPSyncEvent_t ntpEvent; // Last triggered event



uint8_t step = -1;

uint8_t getCurrentStep ()
{
  step = rtc.read(20);
  return step;
}

uint8_t setCurrentStep ( uint8_t value)
{
  //Serial.print ( "Switching to step " );
  //Serial.println ( value );
  step = value;
  rtc.write ( 20, value);
}

bool isValidSetup ()
{
  DateTime currentAlarm = rtc.get_alarm();
  currentAlarm.setyear ( 2000 );
  currentAlarm.setmonth(1);
  currentAlarm.setday (1 );

  if ( currentAlarm.second() > 59 ) return false;
  if ( currentAlarm.minute() > 59 ) return false;
  if ( currentAlarm.hour() > 23 ) return false;

  DateTime currentTime = rtc.now();
  currentTime.setyear ( 2000 );
  currentTime.setmonth(1);
  currentTime.setday (1 );

  char buf[100];
  strncpy(buf, "hh:mm:ss\0", 100);
  Serial.print ( "Current time: ");
  Serial.println(currentTime.format(buf));

  strncpy(buf, "hh:mm:ss\0", 100);
  Serial.print ( "Current alarm: ");
  Serial.println(currentAlarm.format(buf));


  if ( currentTime < currentAlarm )
  {
    Serial.println ( "alarm still valid ");
    return true;
  }

  return false;
}





void interrupt()
{
  //Serial.println("Timer triggered ...");
  setCurrentStep ( 1 );
}

//
// Create an alarm on the next full minute
// based on the current RTC time
//
void setAlarmToNextFullMinute()
{
  DateTime alarm = rtc.now();

  uint8_t min = alarm.minute();
  uint8_t hour = alarm.hour();

  min = min + 1;
  if ( min > 59 )
  {
    min = 0;
    hour = hour + 1;
    if ( hour > 23 )
    {
      hour = 0;
    }
  }

  alarm.sethour( hour );
  alarm.setminute( min );
  alarm.setsecond(0);
  rtc.set_alarm( alarm);

  char buf[100];
  strncpy(buf, "DD.MM.YYYY hh:mm:ss\0", 100);
  Serial.print("Setting alarm: ");
  strncpy(buf, "hh:mm:ss\0", 100);
  Serial.println(alarm.format(buf));
}

//
// Cancel the current active alarm
// and schedule a new alarm on the next full minute
//
void prepareNextAlarm()
{
  rtc.off_alarm();
  rtc.begin();

  setAlarmToNextFullMinute();

  step = 1;
}

//
// ---------------------------------------
//
void setup() {

  static WiFiEventHandler e1, e2;

  Serial.begin(115200);
  Serial.println("start");

  Wire.begin();

  rtc.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(YOUR_WIFI_SSID, YOUR_WIFI_PASSWD);

  NTP.onNTPSyncEvent([](NTPSyncEvent_t event) {
    ntpEvent = event;
    syncEventTriggered = true;
  });

  e1 = WiFi.onStationModeGotIP(onSTAGotIP);// As soon WiFi is connected, start NTP Client
  e2 = WiFi.onStationModeDisconnected(onSTADisconnected);

  wifi_set_sleep_type(LIGHT_SLEEP_T);

  pinMode(MAX, OUTPUT);
  pinMode(BUILTIN_LED1, OUTPUT);
  digitalWrite(BUILTIN_LED1, HIGH);

  pinMode(ONBOARDLED2, OUTPUT); // Onboard LED
  digitalWrite(ONBOARDLED2, HIGH); // Switch off LED

  pinMode(rtc_int, INPUT);
  attachInterrupt(digitalPinToInterrupt(rtc_int), interrupt, FALLING);

  step = getCurrentStep();
  uint8_t al = rtc.read(0x00);
  bool s = isValidSetup();
  //Serial.print ( "Setup ") ;
  //Serial.println ( s );

  //Serial.print ("Current step: " ); Serial.println ( step );
  if ( (step > 3) || ( al != 0x04) || (!s))
  {
    Serial.println("RTC not initialized");
    setCurrentStep ( 1 );
  }
}


void loop()
{

  static int last = 0;

  if (syncEventTriggered) {
    processSyncEvent(ntpEvent);
    syncEventTriggered = false;
  }


  if ((millis() - last) > 1000) {
    last = millis();

    showCurrentStatus();

  }


  //
  //  if ( step == 0 )
  //  {
  //    DateTime now = rtc.now();
  //    if ( now.second() > 0 )
  //    {
  //      digitalWrite(BUILTIN_LED1, LOW);
  //      delay ( 10 );
  //      digitalWrite(BUILTIN_LED1, HIGH);
  //    }
  //  }

  if ( step == 1 )
  {
    prepareNextAlarm();

    setCurrentStep ( 0 );

    DateTime now = rtc.now();
    uint8_t timeToSleep = 55 - now.second();
    if (timeToSleep > 55 ) timeToSleep = 10;
    //Serial.print ( "Going to sleep: " );
    //Serial.println ( timeToSleep );

    bool v = digitalRead ( MAX);
    digitalWrite ( MAX, !v);

    delay ( timeToSleep * 1000 );
  }

  delay(0);
}



