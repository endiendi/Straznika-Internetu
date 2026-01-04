#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <Arduino.h>

extern int failCount;
extern int totalResets;
extern int totalResetsEver;
extern unsigned long lastPingTime;
extern unsigned long lastAPCheckTime;
extern unsigned long apModeStartTime;    // Czas wejścia w AP mode
extern int apModeAttempts;               // Licznik prób wyjścia z AP
extern unsigned long apModeBackoffUntil; // Timestamp gdy kończy się backoff AP mode
extern unsigned long lastResetTime;
extern unsigned long nextResetDelay;
extern unsigned long firstResetTime;
extern int lastPingMs;
extern String statusMsg;
extern int simResetCount;
extern bool providerFailureNotified;
extern bool routerResetInProgress;              // Flaga blokująca watchdog podczas resetu routera
extern int lagCount;                            // Licznik wysokich pingów z rzędu (dla Lag Detection)
extern bool lastGatewayFailReset;               // Czy ostatni reset był z powodu braku gateway
extern unsigned long ntpLastSyncTime;           // Czas ostatniej synchronizacji z NTP
extern unsigned long routerBootStartTime;       // Czas gdy router się ostatnio włączył (dla grace period)
extern unsigned long backupRouterBootStartTime; // Czas gdy router backup się włączył (dla grace period backup)
extern bool safeMode;                           // Tryb bezpieczeństwa - boot loop protection
extern bool ntpSyncLost;                        // Czy straciśmy synchronizację z NTP
extern time_t estimatedOfflineTime;             // Szacunkowy czas offline

void monitorInternetConnection();
bool sprawdzInternet();
bool pingGateway();
bool shouldExecuteScheduledReset(time_t currentTime); // Funkcja do sprawdzania harmonogramu resetów (format HH:MM)
void handleBackupNetworkSwitching();                  // Obsługa przełączania na sieć rezerwową
void safeDelay(unsigned long ms);
void wykonajReset();
void handleButtonPress();

#endif