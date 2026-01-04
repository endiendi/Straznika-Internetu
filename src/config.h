#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- STRUKTURA KONFIGURACJI ---
struct Config
{
    unsigned long pingInterval = 60000; // Zwiększono do 60s - mniej logów = mniej RAM
    int failLimit = 3;
    unsigned long routerOffTime = 60000;
    unsigned long baseBootTime = 150000;
    unsigned long apConfigTimeout = 600000; // Timeout oczekiwania na konfigurację WiFi w AP (domyślnie 10 min)
    int apMaxAttempts = 4;                  // Po ilu próbach wyjścia z AP zmień na reset routera (domyślnie 4)
    unsigned long apBackoffMs = 3600000;    // Backoff po niepowodzeniu AP (domyślnie 60 min)
    unsigned long dhcpTimeoutMs = 300000;   // Timeout dla DHCP (domyślnie 5 min)
    unsigned long noWiFiTimeout = 600000;   // Domyślnie 10 minut
    bool noWiFiBackoff = false;             // Czy wydłużać czas przy awarii
    bool darkMode = false;                  // Tryb ciemny
    bool intermittentMode = false;          // Praca przerywana (uśpienia między cyklami)
    unsigned long awakeWindowMs = 300000;   // Jak długo pracować przed snem (domyślnie 5 min)
    unsigned long sleepWindowMs = 900000;   // Jak długo spać w trybie przerywanym (domyślnie 15 min)
    String host1 = "8.8.8.8";
    String host2 = "1.1.1.1";
    String gatewayOverride = "";     // Ręcznie podany adres bramy (opcjonalnie)
    bool useGatewayOverride = false; // Czy używać ręcznie podanej bramy zamiast DHCP
    int pinRelay = D1;
    bool relayActiveHigh = false; // true = HIGH włącza, false = LOW włącza (NC - Normally Closed)
    int pinRed = D6;
    int pinGreen = D7;
    int pinBlue = D8;        // Dioda niebieska (np. tryb AP / oczekiwanie)
    int pinButton = D5;      // Przycisk do ręcznego trybu konfiguracyjnego
    int pinRelayBackup = D2; // Pin drugiego przekaźnika dla routera rezerwowego (bezpieczniejszy niż D0)
    int ledBrightness = 128; // Jasność diod RGB (50% - oszczędność energii)
    // Admin config (OTA)
    String adminUser = "admin";
    String adminPass = "admin";
    // === KONFIGURACJA SIECI REZERWOWEJ ===
    bool enableBackupNetwork = false;                  // Włącz funkcjonalność sieci rezerwowej
    int backupNetworkFailLimit = 5;                    // Po ilu błędach przełączyć na rezerwową (domyślnie 5)
    unsigned long backupNetworkRetryInterval = 600000; // Co ile czasu spróbować powrót do głównej (10 min)
    bool backupNetworkActive = false;                  // Czy aktualnie używamy sieci rezerwowej
    unsigned long lastBackupSwitchTime = 0;            // Czas ostatniego przełączenia na rezerwową
    unsigned long lastBackupRetryTime = 0;             // Czas ostatniej próby powrotu do głównej
    int backupNetworkFailCount = 0;                    // Licznik błędów gdy używamy rezerwowej
    // Dynamiczne liczniki (zachowywane po restarcie)
    int totalResets = 0;
    unsigned long nextResetDelay = 300000;
    unsigned long firstResetTime = 0;
    unsigned long lastResetTime = 0;
    int failCount = 0;
    unsigned long noWiFiStartTime = 0;
    int providerFailureLimit = 5;             // Limit resetów bez sukcesu, po którym uznać awarię dostawcy
    int maxPingMs = 2000;                     // Maksymalny czas ping w ms, powyżej którego uznaje się lag
    int lagRetries = 3;                       // Ile pingów w jednym teście do potwierdzenia lagu
    int maxTotalResetsEver = 20;              // Maksymalna liczba resetów ogółem, niezależnie od powodzenia
    int totalResetsEver = 0;                  // Licznik wszystkich resetów ever
    int autoResetCountersHours = 0;           // Auto-reset liczników po X godzinach (0 = wyłączone)
    unsigned long accumulatedFailureTime = 0; // Skumulowany czas awarii w milisekundach
    // Scheduled resets (cykliczny reset routera o określonym czasie - z dokładnością do minuty)
    bool scheduledResetsEnabled = false;                  // Czy włączyć zaplanowane resety
    String scheduledResetTimes[5] = {"", "", "", "", ""}; // Czasy resetów w formacie HH:MM (np "08:30"), "" = nieużywane
    unsigned long lastScheduledResetTime = 0;             // Czas ostatniego scheduled resetu (unika duplikatów)
    unsigned long noNtpTimeSince = 0;                     // Czas gdy ostatnio straciliśmy zsynchronizowanie z NTP
    // Hybrid offline timer - dla zaplanowanych resetów bez internetu
    time_t lastNtpSync = 0;          // Timestamp ostatniej synchronizacji z NTP
    unsigned long ntpSyncMillis = 0; // millis() kiedy ostatnio synchronizowaliśmy z NTP (dla offline obliczeń)
    // Watchdog disable (możliwość wyłączenia całego modułu monitorowania)
    bool watchdogEnabled = true; // Czy włączyć watchdog - jeśli false, wyłącza monitorowanie awarii i automatyczne resety
    // Safety Mode (Boot Loop Detection)
    bool safeModeActive = false;                // Czy aktualnie w trybie bezpieczeństwa
    unsigned long bootLoopWindowSeconds = 1200; // Okno detekcji boot loop (domyślnie 20 minut = 1200s)
    // Statystyki przyczyn resetów
    int resetDefault = 0;
    int resetWdt = 0;
    int resetException = 0;
    int resetSoftWdt = 0;
    int resetSoft = 0;
    int resetDeepSleep = 0;
    int resetExt = 0;
    int routerResetCount = 0; // Licznik resetów routera przez watchdog
};

extern Config config;
extern const char *CONFIG_FILE;

bool saveConfig();
bool loadConfig();
bool isValidIP(String ip);

#endif