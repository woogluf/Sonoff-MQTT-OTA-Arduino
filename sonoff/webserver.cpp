#include "support.h"
#include "webserver.h"
#include <DNSServer.h>
#ifdef USE_WEBSERVER

/* bof definit dans sonoff.ino */
void do_cmnd(char *cmnd);

void handleRoot();

void handleConfig();

void handleWifi1();

void handleWifi0();

void handleWifi(boolean scan);

void handleMqtt();

void handleLog();

void handleSave();

void handleReset();

void handleUpgrade();

void handleUpgradeStart();

void handleUploadDone();

void handleUploadLoop();

void handleConsole();

void handleAjax();

void handleInfo();

void handleRestart();

void handleNotFound();
/* Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal();
int getRSSIasQuality(int RSSI);
/** Is this an IP? */
boolean isIp(String str);

/*********************************************************************************************\
 * Web server and WiFi Manager
 *
 * Enables configuration and reconfiguration of WiFi credentials using a Captive Portal
 * Source by AlexT (https://github.com/tzapu)
\*********************************************************************************************/

void handleWifi(boolean scan);

const char HTTP_HEAD[] PROGMEM =
  "<!DOCTYPE html><html lang=\"en\">"
  "<head>"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1,user-scalable=no\"/>"
  "<title>{v}</title>"
  "<script>"
  "var cn=120;"
  "function u(){"
    "if(cn>=0){"
      "document.getElementById('t').innerHTML='Restart in '+cn+' seconds';"
      "cn--;"
      "setTimeout(u,1000);"
    "}"
  "}"
  "function c(l){"
    "document.getElementById('s').value=l.innerText||l.textContent;"
    "document.getElementById('p').focus();"
  "}"
  "var sn=0;"
  "function l(){"
    "var e=document.getElementById('t1');"
    "if(e.scrollTop>=sn){"
      "var x=new XMLHttpRequest();"
      "x.onreadystatechange=function(){"
        "if(x.readyState==4&&x.status==200){"
          "e.value=x.responseText;"
          "e.scrollTop=100000;"
          "sn=e.scrollTop;"
        "}"
      "};"
      "x.open('GET','ax',true);"
      "x.send();"
    "}"
    "setTimeout(l,2000);"
  "}"
  "</script>"
  "<style>"
  "div,fieldset,input,select{padding:5px;font-size:1em;}"
  "input{width:95%;}select{width:100%;}"
  "textarea{resize:none;width:98%;height:312px;padding:5px;overflow:auto;}"
  "body{text-align:center;font-family:verdana;}"
  "td{padding:0px 5px;}"
  "button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;-webkit-transition-duration:0.4s;transition-duration:0.4s;}"
  "button:hover{background-color:#006cba;}"
  ".q{float:right;width:64px;text-align:right;}"
  ".l{background:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6O"
  "Sk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eA"
  "XvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==') no-repeat left center;background-size:1em;}"
  "</style>"
  "</head>"
  "<body>"
  "<div style='text-align:left;display:inline-block;min-width:260px;'>"
  "<div style='text-align:center;'><h2>" APP_NAME "</h2><h3>{h}</h3></div>";
const char HTTP_MSG_RSTRT[] PROGMEM =
  "<br/><div style='text-align:center;'>Device will restart in a few seconds</div><br/>";
const char HTTP_BTN_TOGGL[] PROGMEM =
  "<div style='text-align:center;font-weight:bold;font-size:60px'>{r0}</div>"
  "<br/><form action='/?o=1' method='post'><button>Toggle</button></form><br/>";
const char HTTP_BTN_MENU1[] PROGMEM =
  "<br/><form action='/cn' method='post'><button>Configuration</button></form>"
  "<br/><form action='/in' method='post'><button>Information</button></form>"
  "<br/><form action='/up' method='post'><button>Firmware upgrade</button></form>"
  "<br/><form action='/cm' method='post'><button>Console</button></form>";
const char HTTP_BTN_RSTRT[] PROGMEM =
  "<br/><form action='/rb' method='post'><button>Restart</button></form>";
const char HTTP_BTN_MENU2[] PROGMEM =
  "<br/><form action='/w0' method='post'><button>Configure WiFi</button></form>"
  "<br/><form action='/mq' method='post'><button>Configure MQTT</button></form>"
  "<br/><form action='/lg' method='post'><button>Configure logging</button></form>"
  "<br/><form action='/rt' method='post'><button>Reset Configuration</button></form>";
const char HTTP_BTN_MAIN[] PROGMEM =
  "<br/><br/><form action='/' method='post'><button>Main menu</button></form>";
const char HTTP_BTN_CONF[] PROGMEM =
  "<br/><br/><form action='/cn' method='post'><button>Configuration menu</button></form>";
const char HTTP_LNK_ITEM[] PROGMEM =
  "<div><a href='#p' onclick='c(this)'>{v}</a>&nbsp;<span class='q {i}'>{r}%</span></div>";
const char HTTP_LNK_SCAN[] PROGMEM =
  "<div><a href='/w1'>Scan for wifi networks</a></div><br/>";
const char HTTP_FORM_WIFI[] PROGMEM =
  "<fieldset><legend><b>&nbsp;Wifi parameters&nbsp;</b></legend><form method='post' action='sv'>"
  "<input id='w' name='w' value='1' hidden><input id='r' name='r' value='1' hidden>"
  "<br/><b>SSId</b> (" STA_SSID ")<br/><input id='s' name='s' length=32 placeholder='" STA_SSID "' value='{s1}'><br/>"
  "<br/><b>Password</b></br><input id='p' name='p' length=64 type='password' placeholder='" STA_PASS "' value='{p1}'><br/>"
  "<br/><b>Hostname</b> ({h0})<br/><input id='h' name='h' length=32 placeholder='" WIFI_HOSTNAME" ' value='{h1}'><br/>";
const char HTTP_FORM_MQTT[] PROGMEM =
  "<fieldset><legend><b>&nbsp;MQTT parameters&nbsp;</b></legend><form method='post' action='sv'>"
  "<input id='w' name='w' value='2' hidden><input id='r' name='r' value='1' hidden>"
  "<br/><b>Host</b> (" MQTT_HOST ")<br/><input id='mh' name='mh' length=32 placeholder='" MQTT_HOST" ' value='{m1}'><br/>"
  "<br/><b>Port</b> ({ml})<br/><input id='ml' name='ml' length=5 placeholder='{ml}' value='{m2}'><br/>"
  "<br/><b>Client Id</b> ({m0})<br/><input id='mc' name='mc' length=32 placeholder='" MQTT_CLIENT_ID "' value='{m3}'><br/>"
  "<br/><b>User</b> (" MQTT_USER ")<br/><input id='mu' name='mu' length=32 placeholder='" MQTT_USER "' value='{m4}'><br/>"
  "<br/><b>Password</b> (" MQTT_PASS ")<br/><input id='mp' name='mp' length=32 placeholder='" MQTT_PASS "' value='{m5}'><br/>"
  "<br/><b>Topic</b> (" MQTT_TOPIC ")<br/><input id='mt' name='mt' length=32 placeholder='" MQTT_TOPIC" ' value='{m6}'><br/>";
const char HTTP_FORM_LOG[] PROGMEM =
  "<fieldset><legend><b>&nbsp;Logging parameters&nbsp;</b></legend><form method='post' action='sv'>"
  "<input id='w' name='w' value='3' hidden><input id='r' name='r' value='0' hidden>"
  "<br/><b>Serial log level</b> ({ls})<br/><select id='ls' name='ls'>"
  "<option{a0value='0'>0 None</option>"
  "<option{a1value='1'>1 Error</option>"
  "<option{a2value='2'>2 Info</option>"
  "<option{a3value='3'>3 Debug</option>"
  "<option{a4value='4'>4 More debug</option>"
  "</select></br>"
  "<br/><b>Web log level</b> ({lw})<br/><select id='lw' name='lw'>"
  "<option{b0value='0'>0 None</option>"
  "<option{b1value='1'>1 Error</option>"
  "<option{b2value='2'>2 Info</option>"
  "<option{b3value='3'>3 Debug</option>"
  "<option{b4value='4'>4 More debug</option>"
  "</select></br>"
  "<br/><b>Syslog level</b> ({ll})<br/><select id='ll' name='ll'>"
  "<option{c0value='0'>0 None</option>"
  "<option{c1value='1'>1 Error</option>"
  "<option{c2value='2'>2 Info</option>"
  "<option{c3value='3'>3 Debug</option>"
  "<option{c4value='4'>4 More debug</option>"
  "</select></br>"
  "<br/><b>Syslog host</b> (" SYS_LOG_HOST ")<br/><input id='lh' name='lh' length=32 placeholder='" SYS_LOG_HOST "' value='{l2}'><br/>"
  "<br/><b>Syslog port</b> ({lp})<br/><input id='lp' name='lp' length=5 placeholder='{lp}' value='{l3}'><br/>"
  "<br/><b>Telemetric period</b> ({lt})<br/><input id='lt' name='lt' length=4 placeholder='{lt}' value='{l4}'><br/>";
const char HTTP_FORM_END[] PROGMEM =
  "<br/><button type='submit'>Save</button></form></fieldset>";
const char HTTP_FORM_UPG[] PROGMEM =
  "<div id='f1' name='f1' style='display:block;'>"
  "<fieldset><legend><b>&nbsp;Upgrade by web server&nbsp;</b></legend>"
  "<form method='post' action='u1'>"
  "<br/>OTA Url<br/><input id='o' name='o' length=80 placeholder='OTA_URL' value='{o1}'><br/>"
  "<br/><button type='submit'>Start upgrade</button></form>"
  "</fieldset><br/><br/>"
  "<fieldset><legend><b>&nbsp;Upgrade by file upload&nbsp;</b></legend>"
  "<form method='post' action='u2' enctype='multipart/form-data'>"
  "<br/><input type='file' name='u2'><br/>"
//  "<br/><button type='submit' onclick='this.disabled=true;this.form.submit();'>Start upgrade</button></form></fieldset>"
  "<br/><button type='submit' onclick='document.getElementById(\"f1\").style.display=\"none\";document.getElementById(\"f2\").style.display=\"block\";this.form.submit();'>Start upgrade</button></form>"
  "</fieldset>"
  "</div>"
  "<div id='f2' name='f2' style='display:none;text-align:center;'><b>Upload started ...</b></div>";
const char HTTP_FORM_CMND[] PROGMEM =
  "<br/><textarea readonly id='t1' name='t1' cols='80' wrap='off'></textarea><br/><br/>"
  "<form method='post' action='cm'>"
  "<input style='width:98%' id='" SUB_PREFIX "' name='" SUB_PREFIX "' length=80 placeholder='Enter command' autofocus><br/>"
//  "<br/><button type='submit'>Send command</button>"
  "</form>";
const char HTTP_COUNTER[] PROGMEM =
  "<br/><div id='t' name='t' style='text-align:center;'></div>";
const char HTTP_END[] PROGMEM =
  "</div>"
  "</body>"
  "</html>";

#define DNS_PORT 53
enum http_t {HTTP_OFF, HTTP_USER, HTTP_ADMIN, HTTP_MANAGER};

DNSServer *dnsServer;
ESP8266WebServer *webServer;

boolean _removeDuplicateAPs = true;
int _minimumQuality = -1, _httpflag = HTTP_OFF, _uploaderror = 0, _colcount;

void startWebserver(int type, IPAddress ipweb)
{
  char log[LOGSZ];

  if (!_httpflag) {
    if (!webServer) {
      webServer = new ESP8266WebServer(80);
      webServer->on("/", handleRoot);
      webServer->on("/cn", handleConfig);
      webServer->on("/w1", handleWifi1);
      webServer->on("/w0", handleWifi0);
      webServer->on("/mq", handleMqtt);
      webServer->on("/lg", handleLog);
      webServer->on("/sv", handleSave);
      webServer->on("/rt", handleReset);
      webServer->on("/up", handleUpgrade);
      webServer->on("/u1", handleUpgradeStart);
      webServer->on("/u2", HTTP_POST, handleUploadDone, handleUploadLoop);
      webServer->on("/cm", handleConsole);
      webServer->on("/ax", handleAjax);
      webServer->on("/in", handleInfo);
      webServer->on("/rb", handleRestart);
      webServer->on("/fwlink", handleRoot);  // Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
      webServer->onNotFound(handleNotFound);
    }
    webServer->begin(); // Web server start
  }
  if (_httpflag != type) {
    snprintf_P(log, sizeof(log), PSTR("HTTP: Webserver active on %s with IP address %s"), Hostname, ipweb.toString().c_str());
    addLog(LOG_LEVEL_INFO, log);
  }
  if (type) _httpflag = type;
}

void stopWebserver()
{
  if (_httpflag) {
    webServer->close();
    _httpflag = HTTP_OFF;
    addLog_P(LOG_LEVEL_INFO, PSTR("HTTP: Webserver stopped"));
  }
}

void beginWifiManager()
{
  // setup AP
  if ((WiFi.waitForConnectResult() == WL_CONNECTED) && (static_cast<uint32_t>(WiFi.localIP()) != 0)) {
    WiFi.mode(WIFI_AP_STA);
    addLog_P(LOG_LEVEL_DEBUG, PSTR("Wifimanager: Set AccessPoint and keep Station"));
  } else {
    WiFi.mode(WIFI_AP);
    addLog_P(LOG_LEVEL_DEBUG, PSTR("Wifimanager: Set AccessPoint"));
  }

  stopWebserver();

  dnsServer = new DNSServer();
  WiFi.softAP(Hostname);
  delay(500); // Without delay I've seen the IP address blank
  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());

  startWebserver(HTTP_MANAGER, WiFi.softAPIP());
}

void pollDnsWeb()
{
  if (dnsServer) dnsServer->processNextRequest();
  if (webServer) webServer->handleClient();
}

void showPage(String &page)
{
  page.replace("{h}", Hostname);
  if (_httpflag == HTTP_MANAGER) {
    if (WIFI_configCounter()) {
      page.replace("<body>", "<body onload='u()'>");
      page += FPSTR(HTTP_COUNTER);
    }
  }
  page += FPSTR(HTTP_END);

  webServer->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  webServer->sendHeader("Pragma", "no-cache");
  webServer->sendHeader("Expires", "-1");
  webServer->send(200, "text/html", page);
}

void handleRoot()
{
  char svalue[MESSZ];

  addLog_P(LOG_LEVEL_DEBUG, PSTR("HTTP: Handle root"));

  if (captivePortal()) { // If captive portal redirect instead of displaying the page.
    return;
  }

  if (_httpflag == HTTP_MANAGER) {
    handleWifi0();
  } else {
    if (strlen(webServer->arg("o").c_str())) {
#ifdef MQTT_SUBTOPIC
      snprintf_P(svalue, sizeof(svalue), PSTR("%s 2"), sysCfg.mqtt_subtopic);
#else
      snprintf_P(svalue, sizeof(svalue), PSTR("light 2"));
#endif
      do_cmnd(svalue);
    }

    String page = FPSTR(HTTP_HEAD);
    page.replace("{v}", "Main menu");
    page += FPSTR(HTTP_BTN_TOGGL);
    page.replace("{r0}", (sysCfg.power) ? "ON" : "OFF");
    if (_httpflag == HTTP_ADMIN) {
      page += FPSTR(HTTP_BTN_MENU1);
      page += FPSTR(HTTP_BTN_RSTRT);
    }
    showPage(page);
  }
}

void handleConfig()
{
  addLog_P(LOG_LEVEL_DEBUG, PSTR("HTTP: Handle config"));

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "Configuration");
  page += FPSTR(HTTP_BTN_MENU2);
  page += FPSTR(HTTP_BTN_MAIN);
  showPage(page);
}

void handleWifi1()
{
  handleWifi(true);
}

void handleWifi0()
{
  handleWifi(false);
}

void handleWifi(boolean scan)
{
  char log[LOGSZ];

  addLog_P(LOG_LEVEL_DEBUG, PSTR("HTTP: Handle Wifi config"));

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "Configure Wifi");

  if (scan) {
    int n = WiFi.scanNetworks();
    addLog_P(LOG_LEVEL_DEBUG, PSTR("Wifi: Scan done"));

    if (n == 0) {
      addLog_P(LOG_LEVEL_DEBUG, PSTR("Wifi: No networks found"));
      page += F("No networks found. Refresh to scan again.");
    } else {
      //sort networks
      int indices[n];
      for (int i = 0; i < n; i++) {
        indices[i] = i;
      }

      // RSSI SORT
      for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
          if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
            std::swap(indices[i], indices[j]);
          }
        }
      }

      // remove duplicates ( must be RSSI sorted )
      if (_removeDuplicateAPs) {
        String cssid;
        for (int i = 0; i < n; i++) {
          if (indices[i] == -1) continue;
          cssid = WiFi.SSID(indices[i]);
          for (int j = i + 1; j < n; j++) {
            if (cssid == WiFi.SSID(indices[j])) {
              snprintf_P(log, sizeof(log), PSTR("Wifi: Duplicate AccessPoint %s"), WiFi.SSID(indices[j]).c_str());
              addLog(LOG_LEVEL_DEBUG, log);
              indices[j] = -1; // set dup aps to index -1
            }
          }
        }
      }

      //display networks in page
      for (int i = 0; i < n; i++) {
        if (indices[i] == -1) continue; // skip dups
        snprintf_P(log, sizeof(log), PSTR("Wifi: SSID %s, RSSI %d"), WiFi.SSID(indices[i]).c_str(), WiFi.RSSI(indices[i]));
        addLog(LOG_LEVEL_DEBUG, log);
        int quality = getRSSIasQuality(WiFi.RSSI(indices[i]));

        if (_minimumQuality == -1 || _minimumQuality < quality) {
          String item = FPSTR(HTTP_LNK_ITEM);
          String rssiQ;
          rssiQ += quality;
          item.replace("{v}", WiFi.SSID(indices[i]));
          item.replace("{r}", rssiQ);
          if (WiFi.encryptionType(indices[i]) != ENC_TYPE_NONE) {
            item.replace("{i}", "l");
          } else {
            item.replace("{i}", "");
          }
          page += item;
          delay(0);
        } else {
          addLog_P(LOG_LEVEL_DEBUG, PSTR("Wifi: Skipping due to low quality"));
        }

      }
      page += "<br/>";
    }
  } else {
    page += FPSTR(HTTP_LNK_SCAN);
  }

  page += FPSTR(HTTP_FORM_WIFI);

  char str[33];
  if (!strcmp(WIFI_HOSTNAME, DEF_WIFI_HOSTNAME)) {
    snprintf_P(str, sizeof(str), PSTR(DEF_WIFI_HOSTNAME), sysCfg.mqtt_topic, ESP.getChipId() & 0x1FFF);
  } else {
    snprintf_P(str, sizeof(str), PSTR(WIFI_HOSTNAME));
  }
  page.replace("{h0}", str);
  page.replace("{h1}", String(sysCfg.hostname));
  page.replace("{s1}", String(sysCfg.sta_ssid));
  page.replace("{p1}", String(sysCfg.sta_pwd));
  page += FPSTR(HTTP_FORM_END);
  if (_httpflag == HTTP_MANAGER) {
    page += FPSTR(HTTP_BTN_RSTRT);
  } else {
    page += FPSTR(HTTP_BTN_CONF);
  }
  showPage(page);
}

void handleMqtt()
{
  addLog_P(LOG_LEVEL_DEBUG, PSTR("HTTP: Handle MQTT config"));

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "Configure MQTT");
  page += FPSTR(HTTP_FORM_MQTT);
  char str[33];
  if (!strcmp(MQTT_CLIENT_ID, DEF_MQTT_CLIENT_ID)) {
    snprintf_P(str, sizeof(str), PSTR(DEF_MQTT_CLIENT_ID), ESP.getChipId());
  } else {
    snprintf_P(str, sizeof(str), PSTR(MQTT_CLIENT_ID));
  }
  page.replace("{m0}", str);
  page.replace("{m1}", String(sysCfg.mqtt_host));
  page.replace("{ml}", String((int)MQTT_PORT));
  page.replace("{m2}", String(sysCfg.mqtt_port));
  page.replace("{m3}", String(sysCfg.mqtt_client));
  page.replace("{m4}", String(sysCfg.mqtt_user));
  page.replace("{m5}", String(sysCfg.mqtt_pwd));
  page.replace("{m6}", String(sysCfg.mqtt_topic));
  page += FPSTR(HTTP_FORM_END);
  page += FPSTR(HTTP_BTN_CONF);
  showPage(page);
}

void handleLog()
{
  addLog_P(LOG_LEVEL_DEBUG, PSTR("HTTP: Handle Log config"));

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "Config logging");
  page += FPSTR(HTTP_FORM_LOG);
  page.replace("{ls}", String((int)SERIAL_LOG_LEVEL));
  page.replace("{lw}", String((int)WEB_LOG_LEVEL));
  page.replace("{ll}", String((int)SYS_LOG_LEVEL));
  for (byte i = LOG_LEVEL_NONE; i < LOG_LEVEL_ALL; i++) {
    page.replace("{a" + String(i), (i == sysCfg.seriallog_level) ? " selected " : " ");
    page.replace("{b" + String(i), (i == sysCfg.weblog_level) ? " selected " : " ");
    page.replace("{c" + String(i), (i == sysCfg.syslog_level) ? " selected " : " ");
  }
  page.replace("{l2}", String(sysCfg.syslog_host));
  page.replace("{lp}", String((int)SYS_LOG_PORT));
  page.replace("{l3}", String(sysCfg.syslog_port));
  page.replace("{lt}", String((int)TELE_PERIOD));
  page.replace("{l4}", String(sysCfg.tele_period));
  page += FPSTR(HTTP_FORM_END);
  page += FPSTR(HTTP_BTN_CONF);
  showPage(page);
}

void handleSave()
{
  char log[LOGSZ];
  byte what = 0, restart;
  String result = "";

  addLog_P(LOG_LEVEL_DEBUG, PSTR("HTTP: Parameter save"));

  if (strlen(webServer->arg("w").c_str())) what = atoi(webServer->arg("w").c_str());
  switch (what) {
  case 1:
    strlcpy(sysCfg.hostname, (!strlen(webServer->arg("h").c_str())) ? WIFI_HOSTNAME : webServer->arg("h").c_str(), sizeof(sysCfg.hostname));
    if (strstr(sysCfg.hostname,"%")) strlcpy(sysCfg.hostname, DEF_WIFI_HOSTNAME, sizeof(sysCfg.hostname));
    strlcpy(sysCfg.sta_ssid, (!strlen(webServer->arg("s").c_str())) ? STA_SSID : webServer->arg("s").c_str(), sizeof(sysCfg.sta_ssid));
    strlcpy(sysCfg.sta_pwd, (!strlen(webServer->arg("p").c_str())) ? STA_PASS : webServer->arg("p").c_str(), sizeof(sysCfg.sta_pwd));
    snprintf_P(log, sizeof(log), PSTR("HTTP: Wifi Hostname %s, SSID %s and Password %s"), sysCfg.hostname, sysCfg.sta_ssid, sysCfg.sta_pwd);
    addLog(LOG_LEVEL_INFO, log);
    result += F("<br/>Trying to connect device to network<br/>If it fails reconnect to try again");
    break;
  case 2:
    strlcpy(sysCfg.mqtt_host, (!strlen(webServer->arg("mh").c_str())) ? MQTT_HOST : webServer->arg("mh").c_str(), sizeof(sysCfg.mqtt_host));
    sysCfg.mqtt_port = (!strlen(webServer->arg("ml").c_str())) ? MQTT_PORT : atoi(webServer->arg("ml").c_str());
    strlcpy(sysCfg.mqtt_client, (!strlen(webServer->arg("mc").c_str())) ? MQTT_CLIENT_ID : webServer->arg("mc").c_str(), sizeof(sysCfg.mqtt_client));
    if (strstr(sysCfg.mqtt_client,"%")) strlcpy(sysCfg.mqtt_client, DEF_MQTT_CLIENT_ID, sizeof(sysCfg.mqtt_client));
    strlcpy(sysCfg.mqtt_user, (!strlen(webServer->arg("mu").c_str())) ? MQTT_USER : webServer->arg("mu").c_str(), sizeof(sysCfg.mqtt_user));
    strlcpy(sysCfg.mqtt_pwd, (!strlen(webServer->arg("mp").c_str())) ? MQTT_PASS : webServer->arg("mp").c_str(), sizeof(sysCfg.mqtt_pwd));
    strlcpy(sysCfg.mqtt_topic, (!strlen(webServer->arg("mt").c_str())) ? MQTT_TOPIC : webServer->arg("mt").c_str(), sizeof(sysCfg.mqtt_topic));
    snprintf_P(log, sizeof(log), PSTR("HTTP: MQTT Host %s, Port %d, Client %s, User %s, Password %s, Topic %s"),
      sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.mqtt_client, sysCfg.mqtt_user, sysCfg.mqtt_pwd, sysCfg.mqtt_topic);
    addLog(LOG_LEVEL_INFO, log);
    break;
  case 3:
    sysCfg.seriallog_level = (!strlen(webServer->arg("ls").c_str())) ? SERIAL_LOG_LEVEL : atoi(webServer->arg("ls").c_str());
    sysCfg.weblog_level = (!strlen(webServer->arg("lw").c_str())) ? WEB_LOG_LEVEL : atoi(webServer->arg("lw").c_str());
    sysCfg.syslog_level = (!strlen(webServer->arg("ll").c_str())) ? SYS_LOG_LEVEL : atoi(webServer->arg("ll").c_str());
    strlcpy(sysCfg.syslog_host, (!strlen(webServer->arg("lh").c_str())) ? SYS_LOG_HOST : webServer->arg("lh").c_str(), sizeof(sysCfg.syslog_host));
    sysCfg.syslog_port = (!strlen(webServer->arg("lp").c_str())) ? SYS_LOG_PORT : atoi(webServer->arg("lp").c_str());
    sysCfg.tele_period = (!strlen(webServer->arg("lt").c_str())) ? TELE_PERIOD : atoi(webServer->arg("lt").c_str());
    snprintf_P(log, sizeof(log), PSTR("HTTP: Logging Seriallog %d, Weblog %d, Syslog %d, Host %s, Port %d, TelePeriod %d"),
      sysCfg.seriallog_level, sysCfg.weblog_level, sysCfg.syslog_level, sysCfg.syslog_host, sysCfg.syslog_port, sysCfg.tele_period);
    addLog(LOG_LEVEL_INFO, log);
    break;
  }

  restart = (!strlen(webServer->arg("r").c_str())) ? 1 : atoi(webServer->arg("r").c_str());
  if (restart) {
    String page = FPSTR(HTTP_HEAD);
    page.replace("{v}", "Save parameters");
    page += F("<div style='text-align:center;'><b>Parameters saved</b><br/>");
    page += result;
    page += F("</div>");
    page += FPSTR(HTTP_MSG_RSTRT);
    if (_httpflag == HTTP_MANAGER) {
      _httpflag = HTTP_ADMIN;
    } else {
      page += FPSTR(HTTP_BTN_MAIN);
    }
    showPage(page);

    restartflag = 2;
  } else {
    handleConfig();
  }
}

void handleReset()
{
  char svalue[MESSZ];

  addLog_P(LOG_LEVEL_DEBUG, PSTR("HTTP: Reset parameters"));

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "Default parameters");
  page += F("<div style='text-align:center;'>Parameters reset to default</div>");
  page += FPSTR(HTTP_MSG_RSTRT);
  page += FPSTR(HTTP_BTN_MAIN);
  showPage(page);

  snprintf_P(svalue, sizeof(svalue), PSTR("reset 1"));
  do_cmnd(svalue);
}

void handleUpgrade()
{
  addLog_P(LOG_LEVEL_DEBUG, PSTR("HTTP: Handle upgrade"));

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "Firmware upgrade");
  page += FPSTR(HTTP_FORM_UPG);
  page.replace("{o1}", String(sysCfg.otaUrl));
  page += FPSTR(HTTP_BTN_MAIN);
  showPage(page);

  _uploaderror = 0;
}

void handleUpgradeStart()
{
  char svalue[MESSZ];

  addLog_P(LOG_LEVEL_DEBUG, PSTR("HTTP: Firmware upgrade start"));
  WIFI_configCounter();

  if (strlen(webServer->arg("o").c_str())) {
    snprintf_P(svalue, sizeof(svalue), PSTR("otaurl %s"), webServer->arg("o").c_str());
    do_cmnd(svalue);
  }

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "Info");
  page += F("<div style='text-align:center;'><b>Upgrade started ...</b></div>");
  page += FPSTR(HTTP_MSG_RSTRT);
  page += FPSTR(HTTP_BTN_MAIN);
  showPage(page);

  snprintf_P(svalue, sizeof(svalue), PSTR("upgrade 1"));
  do_cmnd(svalue);
}

void handleUploadDone()
{
  char svalue[MESSZ];

  addLog_P(LOG_LEVEL_DEBUG, PSTR("HTTP: Firmware upload done"));
  WIFI_configCounter();
  restartflag = 0;
  mqttcounter = 0;

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "Info");
  page += F("<div style='text-align:center;'><b>Upload ");
  if (_uploaderror) {
    page += F("<font color='red'>failed</font></b>");
    if (_uploaderror == 1) {
      page += F("<br/><br/>No file selected");
    } else if (_uploaderror == 3) {
      page += F("<br/><br/>File magic header does not start with 0xE9");
    } else if (_uploaderror == 4) {
      page += F("<br/><br/>File flash size is larger than device flash size");
    } else {
      page += F("<br/><br/>Upload error code ");
      page += String(_uploaderror);
    }
    if (Update.hasError()) {
      page += F("<br/><br/>Update error code ");
      page += String(Update.getError());
    }
  } else {
    page += F("<font color='green'>successful</font></b><br/><br/>Device will restart in a few seconds");
    restartflag = 2;
  }
  page += F("</div><br/>");
  page += FPSTR(HTTP_BTN_MAIN);
  showPage(page);
}

void handleUploadLoop()
{
  // Based on ESP8266HTTPUpdateServer.cpp uses ESP8266WebServer Parsing.cpp and Cores Updater.cpp (Update)
  char log[LOGSZ];
  boolean _serialoutput = (LOG_LEVEL_DEBUG <= sysCfg.seriallog_level);

  if (_uploaderror) {
    Update.end();
    return;
  }

  HTTPUpload& upload = webServer->upload();

  if (upload.status == UPLOAD_FILE_START) {
    restartflag = 60;
    mqttcounter = 60;
    if (upload.filename.c_str()[0] == 0)
    {
      _uploaderror = 1;
      return;
    }
//    WiFiUDP::stopAll();
    mqttClient.disconnect();

    snprintf_P(log, sizeof(log), PSTR("Upload: File %s ..."), upload.filename.c_str());
    addLog(LOG_LEVEL_INFO, log);

    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace)) {         //start with max available size
      if (_serialoutput) Update.printError(Serial);
      _uploaderror = 2;
      return;
    }
    _colcount = 0;
  } else if (!_uploaderror && (upload.status == UPLOAD_FILE_WRITE)) {
    if (upload.totalSize == 0)
    {
      if (upload.buf[0] != 0xE9) {
        addLog_P(LOG_LEVEL_DEBUG, PSTR("Upload: File magic header does not start with 0xE9"));
        _uploaderror = 3;
        return;
      }
      uint32_t bin_flash_size = ESP.magicFlashChipSize((upload.buf[3] & 0xf0) >> 4);
      if(bin_flash_size > ESP.getFlashChipRealSize()) {
        addLog_P(LOG_LEVEL_DEBUG, PSTR("Upload: File flash size is larger than device flash size"));
        _uploaderror = 4;
        return;
      }
    }
    if (!_uploaderror && (Update.write(upload.buf, upload.currentSize) != upload.currentSize)) {
      if (_serialoutput) Update.printError(Serial);
      _uploaderror = 5;
      return;
    }
    if (_serialoutput) {
      Serial.printf(".");
      _colcount++;
      if (!(_colcount % 80)) Serial.println();
    }
  } else if(!_uploaderror && (upload.status == UPLOAD_FILE_END)){
    if (_serialoutput && (_colcount % 80)) Serial.println();
    if (Update.end(true)) { // true to set the size to the current progress
      snprintf_P(log, sizeof(log), PSTR("Upload: Successful %u bytes. Restarting"), upload.totalSize);
      addLog(LOG_LEVEL_INFO, log);
    } else {
      if (_serialoutput) Update.printError(Serial);
      _uploaderror = 6;
      return;
    }
  } else if(upload.status == UPLOAD_FILE_ABORTED) {
    addLog_P(LOG_LEVEL_DEBUG, PSTR("Upload: Update was aborted"));
    restartflag = 0;
    mqttcounter = 0;
    _uploaderror = 7;
    Update.end();
  }
  delay(0);
}

void handleConsole()
{
  char svalue[MESSZ];

  addLog_P(LOG_LEVEL_DEBUG, PSTR("HTTP: Handle console"));

  if (strlen(webServer->arg(SUB_PREFIX).c_str())) {
    snprintf_P(svalue, sizeof(svalue), webServer->arg(SUB_PREFIX).c_str());
    do_cmnd(svalue);
  }

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "Console");
  page.replace("<body>", "<body onload='l()'>");
  page += FPSTR(HTTP_FORM_CMND);
  page += FPSTR(HTTP_BTN_MAIN);
  showPage(page);
}

void handleAjax()
{
  String message = "";

  byte counter = logidx;  // Points to oldest entry
  do {
    if (Log[counter].length()) {
      if (message.length()) message += F("\n");
      message += Log[counter];
    }
    counter++;
    if (counter > MAX_LOG_LINES -1) counter = 0;
  } while (counter != logidx);
  webServer->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  webServer->sendHeader("Pragma", "no-cache");
  webServer->sendHeader("Expires", "-1");
  webServer->send(200, "text/plain", message);
}

void handleInfo()
{
  addLog_P(LOG_LEVEL_DEBUG, PSTR("HTTP: Handle info"));

  int freeMem = ESP.getFreeHeap();

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "Information");
//  page += F("<fieldset><legend><b>&nbsp;Information&nbsp;</b></legend>");
  page += F("<table style'width:100%'>");
  page += F("<tr><td><b>Version</b></td><td>"); page += Version; page += F("</td></tr>");
  page += F("<tr><td><b>Core version</b></td><td>"); page += ESP.getCoreVersion(); page += F("</td></tr>");
  page += F("<tr><td><b>SDK version</b></td><td>"); page += String(ESP.getSdkVersion()); page += F("</td></tr>");
//  page += F("<tr><td><b>Boot version</b></td><td>"); page += String(ESP.getBootVersion()); page += F("</td></tr>");
  page += F("<tr><td><b>Uptime</b></td><td>"); page += String(uptime); page += F(" Hours</td></tr>");
  page += F("<tr><td><b>Flash write count</b></td><td>"); page += String(sysCfg.saveFlag); page += F("</td></tr>");
  page += F("<tr><td><b>Boot count</b></td><td>"); page += String(sysCfg.bootcount); page += F("</td></tr>");
  page += F("<tr><td><b>Reset reason</b></td><td>"); page += ESP.getResetReason(); page += F("</td></tr>");
  page += F("<tr><td>&nbsp;</td></tr>");
  page += F("<tr><td><b>SSId</b></td><td>"); page += sysCfg.sta_ssid; page += F("</td></tr>");
  page += F("<tr><td><b>Hostname</b></td><td>"); page += Hostname; page += F("</td></tr>");
  if (static_cast<uint32_t>(WiFi.localIP()) != 0) {
    page += F("<tr><td><b>IP address</b></td><td>"); page += WiFi.localIP().toString(); page += F("</td></tr>");
    page += F("<tr><td><b>Gateway</b></td><td>"); page += WiFi.gatewayIP().toString(); page += F("</td></tr>");
    page += F("<tr><td><b>MAC address</b></td><td>"); page += WiFi.macAddress(); page += F("</td></tr>");
  }
  if (static_cast<uint32_t>(WiFi.softAPIP()) != 0) {
    page += F("<tr><td><b>AP IP address</b></td><td>"); page += WiFi.softAPIP().toString(); page += F("</td></tr>");
    page += F("<tr><td><b>AP Gateway</b></td><td>"); page += WiFi.softAPIP().toString(); page += F("</td></tr>");
    page += F("<tr><td><b>AP MAC address</b></td><td>"); page += WiFi.softAPmacAddress(); page += F("</td></tr>");
  }
  page += F("<tr><td>&nbsp;</td></tr>");
  page += F("<tr><td><b>MQTT Host</b></td><td>"); page += sysCfg.mqtt_host; page += F("</td></tr>");
  page += F("<tr><td><b>MQTT Port</b></td><td>"); page += String(sysCfg.mqtt_port); page += F("</td></tr>");
  page += F("<tr><td><b>MQTT Client and<br/>&nbsp;Fallback Topic</b></td><td>"); page += MQTTClient; page += F("</td></tr>");
  page += F("<tr><td><b>MQTT User</b></td><td>"); page += sysCfg.mqtt_user; page += F("</td></tr>");
  page += F("<tr><td><b>MQTT Password</b></td><td>"); page += sysCfg.mqtt_pwd; page += F("</td></tr>");
  page += F("<tr><td><b>MQTT Topic</b></td><td>"); page += sysCfg.mqtt_topic; page += F("</td></tr>");
  page += F("<tr><td><b>MQTT Group Topic</b></td><td>"); page += sysCfg.mqtt_grptopic; page += F("</td></tr>");
  page += F("<tr><td>&nbsp;</td></tr>");
  page += F("<tr><td><b>ESP Chip id</b></td><td>"); page += String(ESP.getChipId()); page += F("</td></tr>");
  page += F("<tr><td><b>Flash Chip id</b></td><td>"); page += String(ESP.getFlashChipId()); page += F("</td></tr>");
  page += F("<tr><td><b>Flash size</b></td><td>"); page += String(ESP.getFlashChipRealSize() / 1024); page += F("kB</td></tr>");
  page += F("<tr><td><b>Sketch flash size</b></td><td>"); page += String(ESP.getFlashChipSize() / 1024); page += F("kB</td></tr>");
  page += F("<tr><td><b>Sketch size</b></td><td>"); page += String(ESP.getSketchSize() / 1024); page += F("kB</td></tr>");
  page += F("<tr><td><b>Free sketch space</b></td><td>"); page += String(ESP.getFreeSketchSpace() / 1024); page += F("kB</td></tr>");
  page += F("<tr><td><b>Free memory</b></td><td>"); page += String(freeMem / 1024); page += F("kB</td></tr>");
  page += F("</table>");
//  page += F("</fieldset>");
  page += FPSTR(HTTP_BTN_MAIN);
  showPage(page);
}

void handleRestart()
{
  addLog_P(LOG_LEVEL_DEBUG, PSTR("HTTP: Restarting"));

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "Info");
  page += FPSTR(HTTP_MSG_RSTRT);
  if (_httpflag == HTTP_MANAGER) {
    _httpflag = HTTP_ADMIN;
  } else {
    page += FPSTR(HTTP_BTN_MAIN);
  }
  showPage(page);

  restartflag = 2;
}

void handleNotFound()
{
  if (captivePortal()) { // If captive portal redirect instead of displaying the error page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += webServer->uri();
  message += "\nMethod: ";
  message += ( webServer->method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += webServer->args();
  message += "\n";
  for ( uint8_t i = 0; i < webServer->args(); i++ ) {
    message += " " + webServer->argName ( i ) + ": " + webServer->arg ( i ) + "\n";
  }

  webServer->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  webServer->sendHeader("Pragma", "no-cache");
  webServer->sendHeader("Expires", "-1");
  webServer->send(404, "text/plain", message);
}

/* Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal()
{
  if (!isIp(webServer->hostHeader())) {
    addLog_P(LOG_LEVEL_DEBUG, PSTR("HTTP: Request redirected to captive portal"));

    webServer->sendHeader("Location", String("http://") + webServer->client().localIP().toString(), true);
    webServer->send(302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    webServer->client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

int getRSSIasQuality(int RSSI)
{
  int quality = 0;

  if (RSSI <= -100) {
    quality = 0;
  } else if (RSSI >= -50) {
    quality = 100;
  } else {
    quality = 2 * (RSSI + 100);
  }
  return quality;
}

/** Is this an IP? */
boolean isIp(String str)
{
  for (int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

#endif  // USE_WEBSERVER
