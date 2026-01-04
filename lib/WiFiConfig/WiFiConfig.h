#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
// extern ESP8266WebServer server;
#elif defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
// extern WebServer server;
#else
#error "Platforma nieobsługiwana!"
#endif

#include <LittleFS.h>

struct WiFiNetwork
{
    String ssid;
    String pass;
    int networkType; // 0 = główna (primary), 1 = rezerwowa (backup)
};

#ifndef WIELKOSC_TABLICY
#define WIELKOSC_TABLICY 5
#endif

constexpr int wielkoscTablicy = WIELKOSC_TABLICY; // Użycie stałej preprocesora jako constexpr

// constexpr const int wielkoscTablicy = 5;
// extern WiFiNetwork tablica[wielkoscTablicy];
extern WiFiNetwork tablica[WIELKOSC_TABLICY];
extern bool uruchomTrybTestowy;
extern const char *NAZWA_ESP;
extern const char *WIFI_CONFIG_FILES;

void PolaczZWiFi(WiFiNetwork sieci[], void (*ledHandler)() = nullptr, int filterNetworkType = -1); // Łączy z siecią WIFI. filterNetworkType: -1=wszystkie, 0=główne, 1=rezerwowe
// void zapiszDoTablicy(const String &ssid, const String &pass);
void zapiszTabliceDoPliku(const char *nazwaPliku, WiFiNetwork sieci[]);
void trim(String &str);
void odczytajTabliceZPliku(const char *nazwaPliku);
int liczbaZajetychMiejscTablicy(WiFiNetwork sieci[], int maxSize); // Podaje ilość zajętych miejsc w tablicy
void uruchomAP();
void uaktualnijTablicePlik(const String &ssid, const String &pass); // zapisuje do tablicy a potem do pliku
void wyczyscPlik(const char *nazwaPliku);                           // Czyści tablicę a potem dane z pliku
void wyczyscTablice(WiFiNetwork sieci[], int wielkoscTablicy);
bool initLittleFS(); // Inicjalizacja obsługi plików potszebne do zapizu danych do pliku wywołanie w setup
void updateMDNS();   // inicjalizacja mDNS do obsługi nazw wywołanie w Loop
void uruchommDNS();

#endif // WIFI_CONFIG_H;
