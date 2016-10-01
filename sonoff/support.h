#ifndef __SUPPORT_H__
#define __SUPPORT_H__
#include <Arduino.h>
#include <WiFiUDP.h>

#ifdef __cplusplus
extern "C" {
#endif


#define VERSION                0x01002300   // 1.0.35

#define SONOFF                 1            // Sonoff, Sonoff TH10/16
#define ELECTRO_DRAGON         2            // Electro Dragon Wifi IoT Relay Board Based on ESP8266

#define DHT11                  11
#define DHT21                  21
#define DHT22                  22
#define AM2301                 21
#define AM2302                 22
#define AM2321                 22

enum log_t   {LOG_LEVEL_NONE, LOG_LEVEL_ERROR, LOG_LEVEL_INFO, LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG_MORE, LOG_LEVEL_ALL};
enum week_t  {Last, First, Second, Third, Fourth};
enum dow_t   {Sun=1, Mon, Tue, Wed, Thu, Fri, Sat};
enum month_t {Jan=1, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec};
enum wifi_t  {WIFI_STATUS, WIFI_SMARTCONFIG, WIFI_MANAGER, WIFI_WPSCONFIG};

#include "user_config.h"

/*********************************************************************************************\
 * Enable feature by removing leading // or disable feature by adding leading //
\*********************************************************************************************/

#define USE_SERIAL                          // Enable serial command line (+0.2k code, +0.2k mem)
#define USE_TICKER                          // Enable interrupts to keep RTC synced during subscription flooding
//#define USE_SPIFFS                          // Switch persistent configuration from flash to spiffs (+24k code, +0.6k mem)
#define USE_WEBSERVER                       // Enable web server and wifi manager (+37k code, +2k mem)

/*********************************************************************************************\
 * No more user configurable items below
\*********************************************************************************************/

#define MQTT_SUBTOPIC          "POWER"      // Default MQTT subtopic (POWER or LIGHT)
#define APP_POWER              0            // Default saved power state Off

#define DEF_WIFI_HOSTNAME      "%s-%04d"    // Expands to <MQTT_TOPIC>-<last 4 decimal chars of MAC address>
#define DEF_MQTT_CLIENT_ID     "DVES_%06X"  // Also fall back topic using Chip Id = last 6 characters of MAC address

#define STATES                 10           // loops per second
#define MQTT_RETRY_SECS        10           // Seconds to retry MQTT connection

#define TOPSZ                  40           // Max number of characters in topic string
#define MESSZ                  200          // Max number of characters in message string (Syntax string)
#define LOGSZ                  128          // Max number of characters in log string

#define MAX_LOG_LINES          80

enum butt_t {PRESSED, NOT_PRESSED};

extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end;

struct SYSCFG {
  unsigned long cfg_holder;
  unsigned long saveFlag;
  unsigned long version;
  byte          seriallog_level;
  byte          syslog_level;
  char          syslog_host[32];
  char          sta_ssid[32];
  char          sta_pwd[64];
  char          otaUrl[80];
  char          mqtt_host[32];
  char          mqtt_grptopic[32];
  char          mqtt_topic[32];
  char          mqtt_topic2[32];
  char          mqtt_subtopic[32];
  int8_t        timezone;
  uint8_t       power;
  uint8_t       ledstate;
  uint16_t      mqtt_port;
  char          mqtt_client[33];
  char          mqtt_user[33];
  char          mqtt_pwd[33];
  uint8_t       webserver;
  unsigned long bootcount;
  char          hostname[33];
  uint16_t      syslog_port;
  byte          weblog_level;
  uint16_t      tele_period;
  uint8_t       sta_config;
  int16_t      savedata;
};
extern struct SYSCFG sysCfg;

struct TIME_T {
  uint8_t       Second;
  uint8_t       Minute;
  uint8_t       Hour;
  uint8_t       Wday;   // day of week, sunday is day 1
  uint8_t       Day;
  uint8_t       Month;
  char          MonthName[4];
  uint16_t      Year;
  unsigned long Valid;
};
extern struct TIME_T rtcTime;

struct TimeChangeRule
{
  uint8_t       week;      // 1=First, 2=Second, 3=Third, 4=Fourth, or 0=Last week of the month
  uint8_t       dow;       // day of week, 1=Sun, 2=Mon, ... 7=Sat
  uint8_t       month;     // 1=Jan, 2=Feb, ... 12=Dec
  uint8_t       hour;      // 0-23
  int           offset;    // offset from UTC in minutes
};

extern TimeChangeRule myDST;  // Daylight Saving Time
extern TimeChangeRule mySTD;  // Standard Time

boolean spiffsPresent();
#ifdef USE_SPIFFS
void initSpiffs();
#endif  // USE_SPIFFS

String rtc_time(int type);
void rtc_init();

void CFG_Default();
void CFG_Save();
void CFG_Load();
void CFG_Erase();


boolean WIFI_WPSConfigDone(void);

boolean WIFI_beginWPSConfig(void);
void WIFI_config(int type);

void WIFI_check_ip();
void WIFI_Check(int param);

int WIFI_State();
void WIFI_Connect(char *Hostname);
boolean WIFI_configCounter();
extern char Hostname[32];

#ifdef SEND_TELEMETRY_DS18B20
/*********************************************************************************************\
 * DS18B20
 *
 * Source: Marinus vd Broek https://github.com/ESP8266nu/ESPEasy
\*********************************************************************************************/
uint8_t dsb_reset();
uint8_t dsb_read_bit(void);
uint8_t dsb_read(void);
void dsb_write_bit(uint8_t v);
void dsb_write(uint8_t ByteToWrite);
void dsb_readTempPrep();
boolean dsb_readTemp(float &t);
#endif  // SEND_TELEMETRY_DS18B20

#ifdef SEND_TELEMETRY_DHT
/*********************************************************************************************\
 * DHT11, DHT21 (AM2301), DHT22 (AM2302, AM2321)
 *
 * Reading temperature or humidity takes about 250 milliseconds!
 * Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
 * Source: Adafruit Industries https://github.com/adafruit/DHT-sensor-library
\*********************************************************************************************/

void dht_readPrep();
uint32_t dht_expectPulse(bool level);
boolean dht_read();
float dht_convertCtoF(float c);
boolean dht_readTempHum(bool S, float &t, float &h);
void dht_init();
#endif  // SEND_TELEMETRY_DHT

/*********************************************************************************************\
 * Syslog
\*********************************************************************************************/

void syslog(const char *message);

void addLog(byte loglevel, const char *line);

void addLog_P(byte loglevel, const char *formatP);


// declared in sonoff
extern int blinks;
extern int restartflag;
extern String Log[MAX_LOG_LINES];
extern byte logidx;
extern WiFiUDP portUDP;   // syslog

/*********************************************************************************************\
 *
\*********************************************************************************************/

#ifdef __cplusplus
}
#endif
#endif
