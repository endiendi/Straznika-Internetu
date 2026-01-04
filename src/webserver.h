#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESP8266WebServer.h>

extern ESP8266WebServer server;
extern unsigned long lastSessionActivity;
extern const unsigned long SESSION_TIMEOUT;

void setupWebServer();
void handleRoot();
void handleLogin();
void handleLogout();
void handleSaveConfig();
void handleReset();
void handleManualReset();
void handleConfig();
void handleClearLogs();
void handleAddWiFi();
void handleRemoveWiFi();
void handleManualConfig();
void handleUpdatePage();
void handleUpdateResult();
void handleUpdateUpload();
void handleSimPingFail();
void handleSimNoWiFi();
void handleSimHighPing();
void handleStopSim();
void handleNotFound();
void handleSetBrightness();
void handleSaveBrightness();

// Funkcje LED z main.cpp
extern void setLed(bool rOn, bool gOn, bool bOn);
extern int ledBrightness();
extern void refreshLed();

#endif