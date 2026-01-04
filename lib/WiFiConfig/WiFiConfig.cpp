#include "WiFiConfig.h"

#if defined(ESP8266)
// ESP8266WebServer server(80);
#elif defined(ESP32)
// WebServer server(80);
#endif

const char *NAZWA_ESP __attribute__((weak)) = "mojeesp";                  // Domyślna wartość
const char *WIFI_CONFIG_FILES __attribute__((weak)) = "/wifi_config.txt"; // Domyślna wartość
// extern const int wielkoscTablicy;
WiFiNetwork tablica[WIELKOSC_TABLICY];
// WiFiNetwork tablica[wielkoscTablicy];
bool uruchomTrybTestowy = false;

void updateMDNS()
{
#if defined(ESP8266)
    MDNS.update();
#elif defined(ESP32)
    // ESP32 nie wymaga okresowej aktualizacji mDNS
#endif
}

bool initLittleFS()
{
#if defined(ESP32)
    // Na ESP32 używamy parametru true, aby automatycznie sformatować system plików,
    // jeśli montowanie się nie powiedzie.
    if (!LittleFS.begin(true))
    {
        Serial.println("Nie udało się zainicjalizować LittleFS na ESP32.");
        return false;
    }
#elif defined(ESP8266)
    // Dla ESP8266 najpierw próbujemy zamontować LittleFS
    if (!LittleFS.begin())
    {
        Serial.println("Błąd inicjalizacji LittleFS. Formatowanie...");
        if (LittleFS.format() && LittleFS.begin())
        {
            Serial.println("LittleFS sformatowany i gotowy do użycia.");
        }
        else
        {
            Serial.println("Nie udało się zainicjalizować LittleFS.");
            return false;
        }
    }
#else
#error "Platforma nieobsługiwana!"
#endif
    return true;
}

void zapiszDoTablicy(const String &ssid, const String &pass);
void zapiszTabliceDoPliku(const char *nazwaPliku, WiFiNetwork sieci[]);
void wyczyscTablice(WiFiNetwork sieci[], int wielkoscTablicy);

int liczbaZajetychMiejscTablicy(WiFiNetwork sieci[], int maxSize)
{
    int licznik = 0;
    for (int i = 0; i < maxSize; i++)
    {
        if (sieci[i].ssid.length() > 0)
        {
            licznik++;
        }
    }
    return licznik;
}

void uruchomAP()
{
    // Jeśli nie udało się połączyć z żadną siecią, uruchamiamy punkt AP
    Serial.println("Nie udało się połączyć z żadną siecią. Uruchamiam punkt AP.");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP8266_Config");
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    Serial.print("Punkt AP uruchomiony. Adres IP: ");
    Serial.println(WiFi.softAPIP());
}
void uruchommDNS()
{
    if (MDNS.begin(NAZWA_ESP))
    { // Tutaj podajesz nazwę urządzenia
        Serial.printf("mDNS uruchomione jako %s.local\n", NAZWA_ESP);
    }
}

void PolaczZWiFi(WiFiNetwork sieci[], void (*ledHandler)(), int filterNetworkType)
{
    int liczbaZajetych = liczbaZajetychMiejscTablicy(sieci, wielkoscTablicy);
    if (!uruchomTrybTestowy)
    {
        Serial.println("\nŁączenie z WiFi...");

        bool polaczenieUdane = false;

        for (int i = 0; i < liczbaZajetych; i++)
        {
            // Filtruj sieci wg typu, jeśli filterNetworkType != -1
            if (filterNetworkType >= 0 && sieci[i].networkType != filterNetworkType)
            {
                Serial.printf("Pomijam sieć %s (typ %d, szukam %d)\n", sieci[i].ssid.c_str(), sieci[i].networkType, filterNetworkType);
                continue;
            }

            const char *ssid = sieci[i].ssid.c_str();
            const char *password = sieci[i].pass.c_str();

            Serial.print("\nPróba połączenia z siecią: ");
            Serial.println(ssid);

            WiFi.begin(ssid, password);

            unsigned long startAttemptTime = millis();
            while (WiFi.status() != WL_CONNECTED)
            {
                if (millis() - startAttemptTime > 10000)
                { // Timeout po 10 sekundach
                    Serial.println("\nNie udało się połączyć z WiFi.");
                    break;
                }
                delay(500);
                Serial.print(".");
                if (ledHandler)
                    ledHandler();
            }
            if (WiFi.status() == WL_CONNECTED)
            {
                Serial.printf("\nDane sieci z którą się połączyło ssid %s hasło %s \n", ssid, password);
                uaktualnijTablicePlik(ssid, password);
                // zapiszDoTablicy(ssid, password);
                // zapiszTabliceDoPliku(WIFI_CONFIG_FILES, tablica);
                Serial.println("\nPołączono z WiFi!");
                Serial.print("Adres IP: ");
                Serial.println(WiFi.localIP());
                polaczenieUdane = true;
                uruchommDNS();
                break; // Jeśli połączenie się powiedzie, przerywamy pętlę
            }
        }
        if (!polaczenieUdane)
        {
            // Twórz AP tylko jeśli nie jesteśmy wymuszoną rezerwową siecią bez AP
            // (to będzie obsługiwane przez callera - glavni.cpp może sprawdzić backupNetworkActive)
            uruchomAP();
            uruchommDNS();
        }
    }
    else
    {
        Serial.println("\nTryb testowy, nie połączono z WiFi");
    }
}
void uaktualnijTablicePlik(const String &ssid, const String &pass)
{
    zapiszDoTablicy(ssid, pass);
    zapiszTabliceDoPliku(WIFI_CONFIG_FILES, tablica);
}
void zapiszDoTablicy(const String &ssid, const String &pass)
{
    int liczbaZajetych = liczbaZajetychMiejscTablicy(tablica, wielkoscTablicy);
    int indexIstniejacy = -1;
    if (ssid != "")
    {
        for (int i = 0; i < liczbaZajetych; i++)
        {
            if (tablica[i].ssid == ssid)
            {
                indexIstniejacy = i;
                // Serial.println("1");
                break;
            }
        }

        if (indexIstniejacy != -1)
        {
            for (int i = indexIstniejacy; i > 0; i--)
            {
                tablica[i] = tablica[i - 1];
            }
            tablica[0].ssid = ssid;
            tablica[0].pass = pass;
            // Serial.println("2");
            return;
        }

        if (liczbaZajetych == wielkoscTablicy)
        {
            for (int i = wielkoscTablicy - 1; i > 0; i--)
            {
                tablica[i] = tablica[i - 1];
                // Serial.println("3");
            }
        }
        else
        {
            for (int i = liczbaZajetych; i > 0; i--)
            {
                tablica[i] = tablica[i - 1];
                // Serial.println("4");
            }
            liczbaZajetych++;
        }

        tablica[0].ssid = ssid;
        tablica[0].pass = pass;
        // Serial.println("5");
    }
}

void wyczyscPlik(const char *nazwaPliku)
{
    wyczyscTablice(tablica, wielkoscTablicy);
    File plik = LittleFS.open(nazwaPliku, "w");
    if (!plik)
    {
        Serial.println("Błąd zapisu pliku!");
        return;
    }
    plik.close();
    Serial.println("Plik wyczyszczony.");
}

void wyczyscTablice(WiFiNetwork sieci[], int wielkoscTablicy)
{
    for (int i = 0; i < wielkoscTablicy; i++)
    {
        // Ustawienie każdego elementu na wartości domyślne
        // sieci[i].ssid[0] = '\0'; // Pusty ciąg dla SSID
        // sieci[i].pass[0] = '\0'; // Pusty ciąg dla hasła
        sieci[i].ssid = "";
        sieci[i].pass = "";
    }
    Serial.println("Tablica wyczyszczona.");
}

void zapiszTabliceDoPliku(const char *nazwaPliku, WiFiNetwork sieci[])
{
    File plik = LittleFS.open(nazwaPliku, "w");
    if (!plik)
    {
        Serial.println("Błąd zapisu pliku!");
        return;
    }
    int liczbaSieci = liczbaZajetychMiejscTablicy(sieci, wielkoscTablicy);
    for (int i = 0; i < liczbaSieci; i++)
    {
        plik.println(sieci[i].ssid);
        plik.println(sieci[i].pass);
    }
    plik.close();
    Serial.println("Tablica zapisana.");
}

void trim(String &str)
{
    str.trim();
}

void odczytajTabliceZPliku(const char *nazwaPliku)
{
    File plik = LittleFS.open(nazwaPliku, "r");
    if (!plik)
    {
        Serial.println("Błąd odczytu pliku!");
        return;
    }
    int liczbaZajetych = 0;
    while (plik.available() && liczbaZajetych < wielkoscTablicy)
    {
        tablica[liczbaZajetych].ssid = plik.readStringUntil('\n');
        tablica[liczbaZajetych].ssid.trim();

        tablica[liczbaZajetych].pass = plik.readStringUntil('\n');
        tablica[liczbaZajetych].pass.trim();

        liczbaZajetych++;
    }
    plik.close();
}
