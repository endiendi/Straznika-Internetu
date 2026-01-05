#include "serial_handler.h"
#include "WiFiConfig.h"
#include "constants.h"

// Forward declarations (z main.cpp)
extern String statusMsg;
extern void logEvent(String msg);

// ============================================================================
// ZMIENNE STANU AUTORYZACJI
// ============================================================================

static bool serialAuthorized = false;
static unsigned long lastAuthTime = 0;

// ============================================================================
// FUNKCJE POMOCNICZE BEZPIECZEŃSTWA
// ============================================================================

/**
 * @brief Sprawdza czy sesja jest zalogowana i nie wygasła
 * @return true - jeśli zalogowany i sesja ważna, false - inaczej
 */
static bool isSessionValid()
{
    if (!serialAuthorized)
        return false;

    // Sprawdzaj timeout sesji
    unsigned long sessionAge = (millis() - lastAuthTime) / 1000 / 60; // w minutach
    if (sessionAge > SERIAL_HANDLER_SESSION_TIMEOUT)
    {
        serialAuthorized = false;
        Serial.println(F("⏱️  Sesja wygasła - wymagane ponowne uwierzytelnianie (auth:PIN)"));
        logEvent("Serial: Sesja autoryzacji wygasła");
        return false;
    }

    return true;
}

/**
 * @brief Sprawdza czy polecenie może być wykonane
 * @return true - jeśli może, false - jeśli dostęp zabroniony
 */
static bool checkAccessPermission()
{
#if SERIAL_HANDLER_ONLY_IN_AP == 1
    // Dostęp tylko w AP mode
    if (WiFi.getMode() != WIFI_AP)
    {
        Serial.println(F("❌ Błąd: Polecenia Serial są dostępne TYLKO w trybie AP (Access Point)"));
        logEvent("Serial: Próba dostępu poza trybem AP");
        return false;
    }
#endif

    // Sprawdź autoryzację
    if (!isSessionValid())
    {
        Serial.println(F("🔒 Błąd: Brak autoryzacji! Uwierzytelnij się: auth:PIN"));
        return false;
    }

    return true;
}

/**
 * @brief Obsługuje polecenie autoryzacji
 * @param command Polecenie w formacie: auth:PIN
 */
static void handleAuthCommand(String command)
{
    // Parsuj format: auth:PIN
    int colonPos = command.indexOf(':');
    if (colonPos == -1)
    {
        Serial.println(F("❌ Błąd formatu! Użyj: auth:PIN"));
        logEvent("Serial: Błędny format auth");
        return;
    }

    String pin = command.substring(colonPos + 1);
    pin.trim();

    if (pin == SERIAL_HANDLER_AUTH_PIN)
    {
        serialAuthorized = true;
        lastAuthTime = millis();
        Serial.print(F("✅ Autoryzacja pomyślna! Sesja aktywna na "));
        Serial.print(SERIAL_HANDLER_SESSION_TIMEOUT);
        Serial.println(F(" minut."));
        Serial.println(F("💡 Wpisz 'help' aby zobaczyć dostępne polecenia"));
        logEvent("Serial: Udana autoryzacja");
    }
    else
    {
        Serial.println(F("❌ Błąd: Niepoprawny PIN!"));
        logEvent("Serial: PRÓBA DOSTĘPU Z NIEPOPRAWNYM PIN - POTENCJALNE ZAGROŻENIE!");
    }
}

// Makro do konwersji define'a na string
#define STRINGIFY(x) #x

// ============================================================================
// OBSŁUGA POLECEŃ Z SERIAL MONITOR
// ============================================================================

void printSerialHelp()
{
    Serial.println(F("\n╔════════════════════════════════════════════════════════════════╗"));
    Serial.println(F("║         DOSTĘPNE POLECENIA Z SERIAL MONITOR                     ║"));
    Serial.println(F("╠════════════════════════════════════════════════════════════════╣"));
#if SERIAL_HANDLER_ENABLED == 1
    Serial.println(F("║ BEZPIECZEŃSTWO:                                                ║"));
    Serial.println(F("║ auth:PIN               - Uwierzytelnianie (WYMAGANE NAJPIERW!) ║"));
    Serial.println(F("║                                                                ║"));
    Serial.println(F("║ POLECENIA (wymagają autoryzacji):                             ║"));
    Serial.println(F("║ wifi:SSID,HASŁO,TYP  - Dodaj sieć WiFi                        ║"));
    Serial.println(F("║   SSID = nazwa sieci                                          ║"));
    Serial.println(F("║   HASŁO = hasło do sieci                                      ║"));
    Serial.println(F("║   TYP = 0 (główna) lub 1 (rezerwowa)                          ║"));
    Serial.println(F("║   Przykład: wifi:MyNetwork,pass123,0                         ║"));
    Serial.println(F("║                                                                ║"));
    Serial.println(F("║ list                 - Wyświetl listę zapisanych sieci        ║"));
    Serial.println(F("║ status               - Wyświetl status urządzenia             ║"));
    Serial.println(F("║ help                 - Wyświetl tę pomoc                      ║"));
    Serial.println(F("║                                                                ║"));
    Serial.println(F("║ OGRANICZENIA:                                                  ║"));
#if SERIAL_HANDLER_ONLY_IN_AP == 1
    Serial.println(F("║ • Dostęp TYLKO w trybie AP (Access Point)                     ║"));
#else
    Serial.println(F("║ • Dostęp zawsze (tryb AP i STA)                              ║"));
#endif
    Serial.printf("║ • Timeout sesji: %d minut\n", SERIAL_HANDLER_SESSION_TIMEOUT);
    Serial.println(F("╚════════════════════════════════════════════════════════════════╝\n"));
#else
    Serial.println(F("║ ❌ SERIAL_HANDLER_ENABLED = 0 - funkcjonalność wyłączona      ║"));
    Serial.println(F("╚════════════════════════════════════════════════════════════════╝\n"));
#endif
}

void listWiFiNetworks()
{
    if (!checkAccessPermission())
        return;

    int zajete = liczbaZajetychMiejscTablicy(tablica, wielkoscTablicy);

    Serial.println(F("\n╔════════════════════════════════════════════════════════════════╗"));
    Serial.println(F("║           LISTA ZAPISANYCH SIECI WiFi                         ║"));
    Serial.println(F("╠════════════════════════════════════════════════════════════════╣"));
    Serial.printf("║ Sieci: %d/%d\n", zajete, wielkoscTablicy);
    Serial.println(F("╠════════════════════════════════════════════════════════════════╣"));

    if (zajete == 0)
    {
        Serial.println(F("║ [BRAK] Brak zapisanych sieci - dodaj pierwszą!               ║"));
    }
    else
    {
        for (int i = 0; i < zajete; i++)
        {
            Serial.printf("║ [%d] SSID: %-35s Typ: %s\n",
                          i,
                          tablica[i].ssid.c_str(),
                          (tablica[i].networkType == 1 ? "REZERWOWA" : "GŁÓWNA    "));
            Serial.printf("║     Pass: %-49s ║\n", tablica[i].pass.c_str());
            Serial.println(F("║────────────────────────────────────────────────────────────────║"));
        }
    }
    Serial.println(F("╚════════════════════════════════════════════════════════════════╝\n"));
}

void addWiFiViaSerial(String command)
{
    if (!checkAccessPermission())
        return;

    // Parsuj format: wifi:SSID,HASŁO,TYP
    command = command.substring(5); // Usuń "wifi:"

    // Rozdziel na części
    int comma1 = command.indexOf(',');
    int comma2 = command.lastIndexOf(',');

    if (comma1 == -1 || comma2 == -1 || comma1 == comma2)
    {
        Serial.println(F("❌ Błąd formatu! Użyj: wifi:SSID,HASŁO,TYP"));
        Serial.println(F("   Przykład: wifi:MyNetwork,pass123,0"));
        return;
    }

    String ssid = command.substring(0, comma1);
    String pass = command.substring(comma1 + 1, comma2);
    String typeStr = command.substring(comma2 + 1);

    ssid.trim();
    pass.trim();
    typeStr.trim();

    // Walidacja SSID
    if (ssid.length() == 0)
    {
        Serial.println(F("❌ Błąd: Pole SSID nie może być puste!"));
        return;
    }

    if (ssid.length() > 32)
    {
        Serial.println(F("❌ Błąd: SSID zbyt długie (max 32 znaki)!"));
        return;
    }

    // Walidacja TYP
    int networkType = typeStr.toInt();
    if (typeStr != "0" && typeStr != "1")
    {
        Serial.println(F("❌ Błąd: TYP musi być 0 (główna) lub 1 (rezerwowa)!"));
        return;
    }

    // Sprawdź czy sieć już istnieje
    int zajete = liczbaZajetychMiejscTablicy(tablica, wielkoscTablicy);
    for (int i = 0; i < zajete; i++)
    {
        if (tablica[i].ssid == ssid)
        {
            Serial.print(F("⚠️  UWAGA: Sieć '"));
            Serial.print(ssid);
            Serial.println(F("' już istnieje - zostanie przesunięta na pozycję 1 (aktualizacja)"));
            break;
        }
    }

    // Dodaj sieć za pomocą istniejącej funkcji WiFiConfig
    Serial.print(F("⏳ Dodawanie sieci: '"));
    Serial.print(ssid);
    Serial.println(F("'..."));

    uaktualnijTablicePlik(ssid, pass, networkType);

    // Weryfikacja dodania
    if (tablica[0].ssid == ssid && tablica[0].pass == pass && tablica[0].networkType == networkType)
    {
        Serial.println(F("✅ Sieć została pomyślnie dodana!"));
        Serial.print(F("   SSID: "));
        Serial.println(ssid);
        Serial.print(F("   Hasło: "));
        Serial.println(pass);
        Serial.print(F("   Typ: "));
        Serial.println(networkType == 0 ? F("GŁÓWNA") : F("REZERWOWA"));
        Serial.println(F("   💾 Zapisana do pamięci (persystentna)"));

        logEvent("Serial: Dodana sieć WiFi: " + ssid + " (Typ:" + String(networkType) + ")");
    }
    else
    {
        Serial.println(F("❌ Błąd: Sieć nie została dodana!"));
        logEvent("Serial: BŁĄD podczas dodawania sieci WiFi: " + ssid);
    }

    Serial.println();
}

void handleSerialCommands()
{
#if SERIAL_HANDLER_ENABLED == 0
    return; // Funkcjonalność wyłączona w konfiguracji
#endif

    // Tylko przetwarzaj komendy gdy dostępne są dane w Serial
    if (!Serial.available())
        return;

    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.length() == 0)
        return;

    // Przetworzenie poleceń
    if (command.startsWith(F("auth:")))
    {
        // Polecenie auth nie wymaga sprawdzenia dostępu (to walidacja dostępu)
        handleAuthCommand(command);
    }
    else if (command.startsWith(F("wifi:")))
    {
        addWiFiViaSerial(command);
    }
    else if (command == F("list"))
    {
        listWiFiNetworks();
    }
    else if (command == F("help") || command == F("?"))
    {
        printSerialHelp();
    }
    else if (command == F("status"))
    {
        Serial.print(F("Status: "));
        Serial.println(statusMsg);
        Serial.print(F("Tryb: "));
        Serial.print(WiFi.getMode() == WIFI_AP ? F("AP (Konfiguracyjny)") : F("STA (Normalny)"));
        Serial.print(F(" | Autoryzacja: "));
        Serial.println(isSessionValid() ? F("✅ WAŻNA") : F("❌ BRAK"));
    }
    else if (command == F("logout"))
    {
        serialAuthorized = false;
        Serial.println(F("👋 Wylogowano - sesja zakończona"));
        logEvent("Serial: Wylogowanie");
    }
    else
    {
        Serial.print(F("❌ Nieznane polecenie: '"));
        Serial.print(command);
        Serial.println(F("' - Wpisz 'help' aby zobaczyć dostępne polecenia"));
    }
}
