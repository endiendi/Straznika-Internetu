#ifndef APP_GLOBALS_H
#define APP_GLOBALS_H

#include <Arduino.h>
#include <ESP8266WebServer.h>

// ============================================================================
// GLOBALNE ZMIENNE I FUNKCJE APLIKACJI
// ============================================================================
// Plik centralizuje wszystkie extern deklaracje używane w wielu miejscach.
// Zmniejsza duplikację i ułatwia zarządzanie zależnościami.
//
// REFAKTORYZACJA 2026-01-03:
// - Przeniesiono 16+ extern deklaracji z webserver.cpp, login_handlers.cpp, watchdog.cpp
// - Cel: redukcja duplikacji kodu i lepsze zarządzanie zależnościami
// - Kompiluje się pomyślnie: RAM 69.9%, Flash 55.3%

// --- Webserver (z webserver.cpp) ---
extern ESP8266WebServer server;

// --- Zmienne sesji (z main.cpp) ---
extern bool isSessionActive;              // Czy sesja użytkownika jest aktywna
extern unsigned long lastSessionActivity; // Ostatnia aktywność sesji (ms)
extern String sessionToken;               // Token sesji cookie

// --- Zmienne symulacji (z main.cpp) ---
extern bool simPingFail; // Symulacja awarii ping
extern bool simNoWiFi;   // Symulacja braku WiFi
extern bool simHighPing; // Symulacja wysokiego opóźnienia
extern String simStatus; // Status symulacji (tekst dla UI)

// --- Zmienne watchdog (z main.cpp) ---
extern unsigned long noWiFiStartTime; // Czas rozpoczęcia braku WiFi

// --- Zmienne backup network (z main.cpp) ---
extern bool currentlyOnBackupNetwork;        // Czy aktualnie na sieci rezerwowej
extern unsigned long backupNetworkFailCount; // Licznik błędów na sieci rezerwowej
extern unsigned long lastBackupSwitchTime;   // Czas ostatniego przełączenia

// --- Stałe (z main.cpp) ---
extern const char *LOG_FILE; // Ścieżka do pliku logów

// --- Funkcje pomocnicze (z main.cpp) ---
extern void logEvent(String msg);          // Zapisz zdarzenie do logów
extern void ledOK();                       // Ustaw LED na zielony (OK)
extern void ledFail();                     // Ustaw LED na czerwony (błąd)
extern bool checkAuth(bool quiet = false); // Sprawdź autoryzację użytkownika
extern String generateToken();             // Generuj losowy token sesji

#endif // APP_GLOBALS_H
