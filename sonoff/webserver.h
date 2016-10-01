#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

#include <ESP8266WebServer.h>
#include <PubSubClient.h>



#ifdef __cplusplus
extern "C" {
#endif

//declared into sonoff.ino
extern char Version[16];
extern char MQTTClient[32];
extern uint8_t mqttcounter;
extern int uptime;
extern PubSubClient mqttClient;

void pollDnsWeb();
void beginWifiManager();
void startWebserver(int type, IPAddress ipweb);
void stopWebserver();

#ifdef __cplusplus
}
#endif
#endif
