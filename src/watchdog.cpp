#include "watchdog.h"
#include "config.h"
#include "constants.h"
#include "WiFiConfig.h"
#include "app_globals.h" // Centralne extern deklaracje
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <time.h>
#include <elapsedMillis.h>

// Pomocnicza funkcja do sprawdzenia dostępności bramy (routera)
bool pingGateway()
{
    IPAddress gw;

    // Jeśli włączono ręczną bramę i jest poprawna, użyj jej; w przeciwnym razie sięgnij po DHCP
    if (config.useGatewayOverride && config.gatewayOverride.length() > 0)
    {
        if (!gw.fromString(config.gatewayOverride))
        {
            return false; // Niepoprawny adres w konfiguracji
        }
    }
    else
    {
        gw = WiFi.gatewayIP();
    }

    // Jeśli nie mamy bramy (np. klient DHCP nie dostał adresu) traktuj jak błąd
    if ((gw[0] == 0) && (gw[1] == 0) && (gw[2] == 0) && (gw[3] == 0))
    {
        return false;
    }

    // Jedno zapytanie wystarczy do klasyfikacji, nie nadpisujemy lastPingMs (to jest ping do zewnętrznych hostów)
    return Ping.ping(gw, 1);
}

// === HARMONOGRAM RESETÓW - PARSE HH:MM ===
// Parsuje czas w formacie "HH:MM" i porównuje z bieżącą godziną:minutą
// Zwraca true jeśli bieżący czas pasuje do któregokolwiek harmonogramu
bool shouldExecuteScheduledReset(time_t currentTime)
{
    if (!config.scheduledResetsEnabled)
        return false;

    struct tm *timeinfo = localtime(&currentTime);
    int currentHour = timeinfo->tm_hour;
    int currentMin = timeinfo->tm_min;
    int currentDay = timeinfo->tm_yday;

    // Sprawdzamy każdy zaplanowany czas
    for (int i = 0; i < 5; i++)
    {
        if (config.scheduledResetTimes[i].length() == 0)
            continue; // Puste, pomiń

        // Parsuj format "HH:MM"
        int scheduleHour = -1;
        int scheduleMin = -1;
        int parts = sscanf(config.scheduledResetTimes[i].c_str(), "%d:%d", &scheduleHour, &scheduleMin);

        if (parts != 2 || scheduleHour < 0 || scheduleHour > 23 || scheduleMin < 0 || scheduleMin > 59)
            continue; // Niepoprawny format, pomiń

        // Sprawdzamy czy pasuje godzina i minuta
        if (scheduleHour == currentHour && scheduleMin == currentMin)
        {
            // Zanim resetujemy, sprawdzamy czy już to zrobiliśmy w tej minucie
            time_t lastResetTimeT = config.lastScheduledResetTime;
            struct tm *lastResetTm = localtime(&lastResetTimeT);

            int lastResetHour = lastResetTm->tm_hour;
            int lastResetMin = lastResetTm->tm_min;
            int lastResetDay = lastResetTm->tm_yday;

            // Wykonaj reset tylko jeśli to inna minuta lub inny dzień
            if (lastResetHour != currentHour || lastResetMin != currentMin || lastResetDay != currentDay)
            {
                return true;
            }
        }
    }

    return false;
}

// === BACKUP NETWORK MANAGEMENT ===
void handleBackupNetworkSwitching()
{
    // Jeśli funkcja wyłączona - wymuś bezpieczny stan przekaźnika i wyjdź
    if (!config.enableBackupNetwork)
    {
        pinMode(config.pinRelayBackup, OUTPUT);
        digitalWrite(config.pinRelayBackup, config.relayActiveHigh ? LOW : HIGH);
        config.backupNetworkActive = false;
        return;
    }

    // Backup uruchamia się dopiero po wyczerpaniu procedur naprawczych sieci głównej
    // (reset/backoff/AP próby). Chroni to przed zbyt wczesnym przełączeniem na rezerwową.
    bool mainRecoveryExhausted = (totalResets >= config.providerFailureLimit) || (apModeAttempts >= config.apMaxAttempts);
    if (!mainRecoveryExhausted)
        return;

    // Liczymy sieci rezerwowe dostępne
    int backupNetworkCount = 0;
    int backupNetworkIndex = -1;
    for (int i = 0; i < wielkoscTablicy; i++)
    {
        if (tablica[i].ssid.length() > 0 && tablica[i].networkType == 1) // type 1 = backup
        {
            backupNetworkCount++;
            if (backupNetworkIndex == -1)
                backupNetworkIndex = i;
        }
    }

    // Brak sieci rezerwowej - nie ma co robić
    if (backupNetworkCount == 0)
        return;

    unsigned long now = millis();
    bool onPrimaryNetwork = !config.backupNetworkActive;

    // === SCENARIUSZ 1: Bieżąca sieć to główna, a failCount wysoki ===
    if (onPrimaryNetwork && failCount >= config.backupNetworkFailLimit)
    {
        Serial.printf("[BACKUP] Switching to backup network (failCount=%d)\n", failCount);
        logEvent(String("BACKUP: Przełączenie na sieć rezerwową (błędy głównej: ") + failCount + ")");

        // Włącz drugi przekaźnik
        pinMode(config.pinRelayBackup, OUTPUT);
        digitalWrite(config.pinRelayBackup, config.relayActiveHigh ? HIGH : LOW);
        delay(100);
        digitalWrite(config.pinRelayBackup, config.relayActiveHigh ? LOW : HIGH);

        // Zmień ustawienia i spróbuj połączyć
        config.backupNetworkActive = true;
        config.lastBackupSwitchTime = now;
        config.backupNetworkFailCount = 0;
        failCount = 0;
        backupRouterBootStartTime = millis(); // Grace period dla backup routera

        // Przełącz WiFi na sieć rezerwową
        if (backupNetworkIndex >= 0)
        {
            Serial.printf("[BACKUP] Attempting to connect to: %s\n", tablica[backupNetworkIndex].ssid.c_str());
            WiFi.begin(tablica[backupNetworkIndex].ssid.c_str(), tablica[backupNetworkIndex].pass.c_str());
        }
    }

    // === SCENARIUSZ 2: Bieżąca sieć to rezerwowa, spróbuj powrót do głównej ===
    if (!onPrimaryNetwork && (now - config.lastBackupSwitchTime) >= config.backupNetworkRetryInterval)
    {
        // Jeśli rezerwowa działa (failCount niski), spróbuj powrotu do głównej
        if (config.backupNetworkFailCount < 2)
        {
            Serial.println("[BACKUP] Attempting to switch back to primary network");
            logEvent("BACKUP: Próba powrotu do sieci głównej");

            // Wyłącz drugi przekaźnik
            digitalWrite(config.pinRelayBackup, config.relayActiveHigh ? LOW : HIGH);
            delay(100);

            config.backupNetworkActive = false;
            config.lastBackupRetryTime = now;
            failCount = 0;

            // Przełącz WiFi na sieć główną
            if (tablica[0].ssid.length() > 0)
            {
                Serial.printf("[BACKUP] Connecting to primary: %s\n", tablica[0].ssid.c_str());
                WiFi.begin(tablica[0].ssid.c_str(), tablica[0].pass.c_str());
            }
        }
        else
        {
            // Rezerwowa też zawodzi - zwiększ licznik i poczekaj dłużej
            config.lastBackupSwitchTime = now; // Resetuj timer
        }
    }

    // === SCENARIUSZ 3: Rezerwowa nie działa - zliczaj błędy ===
    if (!onPrimaryNetwork && failCount >= config.backupNetworkFailLimit)
    {
        config.backupNetworkFailCount++;
        Serial.printf("[BACKUP] Backup network failing (count=%d)\n", config.backupNetworkFailCount);

        if (config.backupNetworkFailCount > 5)
        {
            logEvent("BACKUP: Obie sieci (główna i rezerwowa) niedostępne. Tryb bezpieczeństwa.");
            // Tutaj można dodać dodatkową logikę (np. spróbować ponownie główną)
        }
    }
}

void monitorInternetConnection()
{
    // === WATCHDOG DISABLE ===
    // Jeśli watchdog wyłączony, nie monitoruj i nie resetuj
    if (!config.watchdogEnabled)
    {
        ledOK(); // Sygnał że wszystko OK (watchdog wyłączony)
        statusMsg = "Watchdog WYŁĄCZONY";
        return;
    }

    // === BACKUP NETWORK MANAGEMENT ===
    handleBackupNetworkSwitching();

    // Pomiń sprawdzanie podczas resetu routera
    if (routerResetInProgress)
    {
        return;
    }

    // === SCHEDULED RESETS (Cykliczne resety o wybranym czasie - z dokładnością do minuty) ===
    if (config.scheduledResetsEnabled)
    {
        time_t now = time(nullptr);
        bool hasValidNtp = (now > 1000000000); // Timestamp powinien być po 2001 roku

        // Jeśli mamy NTP, zaktualizuj offline timer
        if (hasValidNtp)
        {
            ntpLastSyncTime = millis();
            ntpSyncLost = false;
            config.noNtpTimeSince = 0;
            config.lastNtpSync = now;        // Zapamięć timestamp synchronizacji
            config.ntpSyncMillis = millis(); // Zapamięć millis kiedy to było
            estimatedOfflineTime = now;      // Używaj "realnego" czasu
        }
        else
        {
            // Brak NTP - używaj offline timer
            if (!ntpSyncLost && config.lastNtpSync > 0)
            {
                ntpSyncLost = true;
                config.noNtpTimeSince = millis();
                logEvent("Utracono synchronizację z NTP - offline timer AKTYWNY");
                statusMsg = "Offline timer aktywny";
            }

            // Oblicz szacunkowy czas offline na podstawie ostatniej synchronizacji
            if (config.lastNtpSync > 0)
            {
                unsigned long elapsedMillis = millis() - config.ntpSyncMillis;
                estimatedOfflineTime = config.lastNtpSync + (elapsedMillis / 1000);
            }
            else
            {
                // Nigdy nie było synchronizacji - nie możemy resetować
                return;
            }
        }

        // Teraz mamy czas (z NTP lub offline timer) - sprawdzaj zaplanowane resety
        if (shouldExecuteScheduledReset(estimatedOfflineTime))
        {
            struct tm *timeinfo = localtime(&estimatedOfflineTime);
            int currentHour = timeinfo->tm_hour;
            int currentMin = timeinfo->tm_min;

            String ntpStatus = (hasValidNtp) ? " (NTP)" : " (offline timer)";
            String timeStr = String(currentHour < 10 ? "0" : "") + String(currentHour) + ":" +
                             String(currentMin < 10 ? "0" : "") + String(currentMin);
            logEvent("Zaplanowany reset o " + timeStr + ntpStatus);
            statusMsg = "Zaplanowany reset routera...";
            config.lastScheduledResetTime = estimatedOfflineTime;
            saveConfig();
            wykonajReset();
            return;
        }
    }

    // Sprawdzenie czy należy zresetować liczniki (auto-reset po X godzinach)
    if (config.autoResetCountersHours > 0 && config.accumulatedFailureTime > 0)
    {
        unsigned long thresholdMs = (unsigned long)config.autoResetCountersHours * 3600000UL;
        if (config.accumulatedFailureTime >= thresholdMs)
        {
            logEvent("Auto-reset liczników po " + String(config.autoResetCountersHours) + "h (akumulacja: " +
                     String(config.accumulatedFailureTime / 1000) + "s) - czysta karta");

            // Reset wszystkich liczników awarii
            totalResets = 0;
            config.totalResets = 0;
            failCount = 0;
            config.failCount = 0;
            nextResetDelay = FIVE_MINUTES_MS;
            config.nextResetDelay = FIVE_MINUTES_MS;
            firstResetTime = 0;
            config.firstResetTime = 0;
            lastResetTime = 0;
            config.lastResetTime = 0;
            config.accumulatedFailureTime = 0;

            // Reset liczników AP mode i lagów
            apModeAttempts = 0;
            apModeBackoffUntil = 0;
            lagCount = 0;

            // === RESET BACKUP NETWORK ===
            // Po auto-resecie liczników, powrót do sieci głównej
            if (config.backupNetworkActive && config.enableBackupNetwork)
            {
                Serial.println("[BACKUP] Auto-reset: Powrót do sieci głównej po wyczerpaniu czasu");
                logEvent("BACKUP: Auto-reset liczników - powrót do sieci głównej");

                // Wyłącz drugi przekaźnik
                digitalWrite(config.pinRelayBackup, config.relayActiveHigh ? LOW : HIGH);
                delay(100);

                config.backupNetworkActive = false;
                config.backupNetworkFailCount = 0;
                backupRouterBootStartTime = 0;

                // Przełącz WiFi na sieć główną
                if (tablica[0].ssid.length() > 0)
                {
                    WiFi.begin(tablica[0].ssid.c_str(), tablica[0].pass.c_str());
                }
            }

            saveConfig();
            Serial.println("[WATCHDOG] Wszystkie liczniki zresetowane - rozpoczęcie z czystą kartą");
        }
    }

    // Nie resetuj routera gdy urządzenie jest w trybie AP (konfiguracja WiFi)
    // Ale NIE zwracaj się - pozwól resetować w monitorInternetConnection() jeśli licznik prób się wyczerpał
    // (To będzie obsługiwane w main.cpp - jeśli apModeAttempts >= 3, główna pętla wywoła reset)
    // WYJĄTEK: Jeśli jesteśmy w trybie backup network, nie tworzysz AP (backup musi działać bez ręcznej konfiguracji)
    if (WiFi.getMode() == WIFI_AP)
    {
        // Jeśli w trybie backup, wyłącz AP i spróbuj powrócić do connection mode
        if (config.backupNetworkActive && config.enableBackupNetwork)
        {
            Serial.println("[BACKUP] Exiting AP mode - backup network requires auto-reconnect");
            WiFi.mode(WIFI_STA); // Powrót do STA (station) mode
        }
        else
        {
            return;
        }
    }

    // Sprawdzamy czy jest połączenie WiFi (uwzględniając symulację braku WiFi)
    bool isConnected = (WiFi.status() == WL_CONNECTED);
    if (simNoWiFi)
        isConnected = false;

    if (isConnected)
    {
        noWiFiStartTime = 0; // Reset licznika braku WiFi, skoro jest połączenie

        // Grace period po starcie routera - poczekaj zanim zaczynsz testy
        // Router przez pierwszych 1,5 min jest niestabilny (EMI)
        unsigned long gracePeriodTime = routerBootStartTime;
        bool isBackupGracePeriod = false;

        // Jeśli w trybie backup, użyj backup boot time
        if (config.backupNetworkActive && backupRouterBootStartTime > 0)
        {
            gracePeriodTime = backupRouterBootStartTime;
            isBackupGracePeriod = true;
        }

        if (gracePeriodTime > 0 && millis() - gracePeriodTime < config.baseBootTime)
        {
            // Jesteśmy w grace period - nie testuj internetu
            unsigned long remainMs = config.baseBootTime - (millis() - gracePeriodTime);
            String routerLabel = isBackupGracePeriod ? "Router backup" : "Router";
            statusMsg = routerLabel + " startuje... grace period " + String(remainMs / 1000) + "s";
            return; // Pomiń testy
        }
        else if (gracePeriodTime > 0)
        {
            // Grace period skończony, router powinien być stabilny
            if (isBackupGracePeriod)
                backupRouterBootStartTime = 0; // Zresetuj flagę backup
            else
                routerBootStartTime = 0; // Zresetuj flagę główną
        }

        if (millis() - lastPingTime > config.pingInterval)
        {
            lastPingTime = millis();
            ESP.wdtFeed();

            // Najpierw sprawdź, czy osiągalna jest brama (router). Jeśli nie, to problem lokalny/LAN.
            static int gatewayFailCount = 0;
            if (!pingGateway())
            {
                gatewayFailCount++;
                failCount = 0; // Nie mieszaj błędów pingu do Internetu z brakiem bramy
                statusMsg = "Brama nie odpowiada (" + String(gatewayFailCount) + "/" + String(config.failLimit) + ")";

                if (gatewayFailCount == 1 || gatewayFailCount == config.failLimit)
                {
                    logEvent("Brama (" + WiFi.gatewayIP().toString() + ") nie odpowiada " + String(gatewayFailCount) + "/" + String(config.failLimit));
                }

                // INTELIGENTNA DETEKCJA: Jeśli ostatni reset był z powodu gateway i problem się powtarza,
                // to nie jest zwykłe zawieszenie routera - traktuj jako awarię dostawcy (limit 5 prób)
                if (gatewayFailCount >= config.failLimit)
                {
                    if (lastGatewayFailReset && totalResets > 0)
                    {
                        // Drugi raz z rzędu brak gateway po resecie → awaria dostawcy
                        logEvent("Gateway nie wrócił po resecie - prawdopodobnie awaria dostawcy (Total: " + String(totalResets) + ")");
                        lastGatewayFailReset = false; // Reset flagi, przełączamy na tryb awarii dostawcy
                        // Od teraz failCount i totalResets będą kontrolować limit 5 prób
                    }
                    else
                    {
                        // Pierwszy raz brak gateway → prawdopodobnie zawieszenie routera
                        logEvent("Prawdopodobnie zawieszenie routera - wykonuję reset");
                        lastGatewayFailReset = true; // Zapamiętaj że resetujemy z powodu gateway
                    }
                    wykonajReset();
                }
                return; // Nie sprawdzaj Internetu, skoro brama jest martwa
            }
            else
            {
                gatewayFailCount = 0;         // Brama żyje, można sprawdzać Internet
                lastGatewayFailReset = false; // Reset OK, nie był problem z gateway
            }

            if (sprawdzInternet())
            {
                ledOK();
                if (failCount > 0)
                    logEvent("Internet OK (Po " + String(failCount) + " bledach)");
                failCount = 0;
                lagCount = 0; // Reset licznika spike'ów gdy internet OK
                statusMsg = "Internet OK";

                // --- POPRAWKA: Logika sukcesu ---
                // Jeśli Internet działa, a mamy zarejestrowane wcześniejsze resety, to znaczy, że AWARIA MINĘŁA.
                if (totalResets > 0)
                {
                    logEvent("Internet wrocil po awarii (Resety: " + String(totalResets) + ")");

                    if (simPingFail || simNoWiFi || simHighPing)
                    {
                        simStatus = "Internet przywrócony - symulacja zakończona pomyślnie";
                    }

                    // Zerujemy liczniki awarii i zapisujemy "czysty" stan
                    totalResets = 0;
                    nextResetDelay = FIVE_MINUTES_MS; // Reset do domyślnych 5 min
                    firstResetTime = 0;
                    simResetCount = 0;               // Reset licznika symulacji
                    providerFailureNotified = false; // Reset flagi awarii dostawcy
                    lastGatewayFailReset = false;    // Reset flagi zawieszenia routera

                    // Aktualizujemy strukturę config i zapisujemy
                    config.totalResets = 0;
                    config.nextResetDelay = FIVE_MINUTES_MS;
                    config.firstResetTime = 0;
                    if (!saveConfig())
                    {
                        Serial.println("BŁĄD ZAPISU CONFIG PO SUKCESIE!");
                        logEvent("BLAD ZAPISU CONFIG PO SUKCESIE");
                    }
                }
            }
            else
            {
                failCount++;
                ledFail();
                if (failCount == 1 || failCount == config.failLimit)
                {
                    logEvent("Blad polaczenia (" + String(failCount) + "/" + String(config.failLimit) + ")");
                }
                statusMsg = "Blad polaczenia (" + String(failCount) + "/" + String(config.failLimit) + ")";
                if (simPingFail || simNoWiFi || simHighPing)
                {
                    simStatus = "Wykryto " + String(failCount) + "/" + String(config.failLimit) + " błędów - oczekiwanie na reset...";
                }

                if (failCount >= config.failLimit)
                {
                    wykonajReset();
                }
            }
        }
    }
    else
    {
        // --- Watchdog dla braku WiFi ---
        ledFail(); // Sygnalizacja błędu diodą
        statusMsg = "Brak połączenia z WiFi";
        failCount = 0; // Resetujemy licznik błędów pingu, bo brak WiFi to inna kategoria błędu

        if (noWiFiStartTime == 0)
        {
            noWiFiStartTime = millis();
            logEvent("Utracono polaczenie WiFi");
            if (simNoWiFi)
            {
                simStatus = "Wykryto brak WiFi - rozpoczęto odliczanie...";
            }
        }

        unsigned long currentTimeout = config.noWiFiTimeout;

        // Logika Backoff: Jeśli włączona i były już resety, wydłużamy czas
        if (config.noWiFiBackoff && totalResets > 0)
        {
            unsigned long extra = (unsigned long)totalResets * FIVE_MINUTES_MS; // +5 minut za każdy reset
            if (extra > BACKOFF_MAX_MS)
                extra = BACKOFF_MAX_MS; // Maksymalnie +60 minut
            currentTimeout += extra;
        }

        // Dla symulacji używamy krótkiego czasu (1 min)
        unsigned long timeout = simNoWiFi ? SIM_NO_WIFI_TIMEOUT_MS : currentTimeout;

        if (millis() - noWiFiStartTime > timeout)
        {
            logEvent("Brak WiFi przez " + String(timeout / 1000) + "s (Limit: " + String(currentTimeout / 1000) + "s) - reset routera");
            statusMsg = "Brak WiFi - reset";
            if (simNoWiFi)
            {
                simStatus = "Brak WiFi przez " + String(timeout / 1000) + "s - rozpoczynam reset routera...";
            }
            wykonajReset();
        }
        else if (simNoWiFi)
        {
            unsigned long elapsed = (millis() - noWiFiStartTime) / 1000;
            simStatus = "Brak WiFi od " + String(elapsed) + "s / " + String(timeout / 1000) + "s...";
        }
    }
}

bool sprawdzInternet()
{
    if (simPingFail)
        return false;

    // --- LAG DETECTION: konfigurowalna liczba prób (lagRetries) ---
    // Próbujemy kilka razy aby potwierdzić wysoki ping (nie chwilowy spike)
    int retries = (config.lagRetries > 0) ? config.lagRetries : 3; // Fallback gdy 0
    for (int attempt = 1; attempt <= retries; attempt++)
    {
        bool pingSuccess = false;
        int pingMs = 0;

        // Próba host 1 (8.8.8.8)
        if (Ping.ping(config.host1.c_str(), 1))
        {
            pingMs = Ping.averageTime();
            pingSuccess = true;
        }
        // Jeśli host 1 fail, spróbuj host 2 (1.1.1.1)
        else if (Ping.ping(config.host2.c_str(), 1))
        {
            pingMs = Ping.averageTime();
            pingSuccess = true;
        }

        if (!pingSuccess)
        {
            // Żaden host nie odpowiada - to nie lag, to brak pingu
            return false;
        }

        // Symulacja wysokiego pingu
        if (simHighPing)
            pingMs = config.maxPingMs + 100;

        lastPingMs = pingMs;

        // Sprawdzenie czy ping jest wysoki
        if (pingMs > config.maxPingMs)
        {
            lagCount++; // Zwiększ licznik spike'ów
            logEvent("Spike #" + String(lagCount) + "/" + String(retries) + ": Wysoki ping " + String(pingMs) + "ms > " + String(config.maxPingMs) + "ms (proba " + String(attempt) + "/" + String(retries) + ")");

            // Jeśli to trzeci spike z rzędu = potwierdzony lag
            if (lagCount >= retries)
            {
                logEvent("LAG POTWIERDZONY: " + String(retries) + " spike'i z rzędu - wymuszam reset routera");
                return false; // Zwróć false = bład, będzie reset
            }

            // Czekaj przed kolejną próbą (daję szansę sieci się ustabilizować)
            delay(500);
            continue; // Spróbuj ponownie
        }
        else
        {
            // Ping OK! Zresetuj licznik
            if (lagCount > 0)
            {
                logEvent("Lag odzyskany: ping " + String(pingMs) + "ms jest OK (licznik spike'ów reset z " + String(lagCount) + ")");
                lagCount = 0; // Reset licznika
            }
            return true; // Sukces
        }
    }

    // Jeśli doszliśmy tutaj, to znaczy że wszystkie 3 próby miały wysoki ping
    return false;
}

void safeDelay(unsigned long ms)
{
    elapsedMillis timer;
    while (timer < ms)
    {
        ESP.wdtFeed();
        delay(100); // Karmienie co 100 ms
    }
}

void wykonajReset()
{
    // === SAFE MODE CHECK ===
    if (safeMode)
    {
        // W Safe Mode nie resetuj routera! To chroni router przed pętlą
        Serial.println("[SAFE_MODE] Reset blocked - boot loop protection active");
        logEvent("SAFE_MODE: Reset blocked - boot loop protection");
        statusMsg = "Safe Mode: Resets blocked";
        return;
    }

    // Dodatkowe zabezpieczenie: maksymalna liczba resetów w krótkim czasie
    const int MAX_RESETS_SHORT_TIME = 5;
    const unsigned long SHORT_TIME_WINDOW = 3600000UL; // 1 godzina
    static int resetsInWindow = 0;
    static unsigned long windowStart = 0;

    bool wasSimulation = (simPingFail || simNoWiFi || simHighPing);

    if (millis() - windowStart > SHORT_TIME_WINDOW)
    {
        resetsInWindow = 0;
        windowStart = millis();
    }
    if (resetsInWindow >= MAX_RESETS_SHORT_TIME)
    {
        Serial.println("Zbyt wiele resetów w krótkim czasie! Zatrzymano resety.");
        logEvent("ZATRZYMANO RESETY (Zbyt wiele w oknie czasowym)");
        statusMsg = "Zatrzymano resety - zbyt wiele w krótkim czasie";
        return;
    }
    resetsInWindow++;

    // Sprawdzenie maksymalnej liczby resetów ogółem
    if (totalResetsEver >= config.maxTotalResetsEver)
    {
        Serial.println("Osiągnięto maksymalną liczbę resetów ogółem! Zatrzymano resety.");
        logEvent("ZATRZYMANO RESETY (Maksymalna liczba ogółem)");
        statusMsg = "Zatrzymano resety - maksymalna liczba ogółem";
        return;
    }
    totalResetsEver++;

    // Sprawdź symulacje i ewentualnie wyłącz po kilku resetach
    if (simPingFail || simNoWiFi || simHighPing)
    {
        simResetCount++;
        simStatus = "Reset nr " + String(simResetCount) + "/3 - procedura resetu w toku...";
        if (simResetCount >= 3)
        {
            simPingFail = false;
            simNoWiFi = false;
            simHighPing = false;
            Serial.println("AUTOMATYCZNIE WYŁĄCZONO SYMULACJE po " + String(simResetCount) + " resetach!");
            logEvent("AUTOMATYCZNIE WYLACZONO SYMULACJE");
            simStatus = "Symulacja zakończona - wykonano 3 resety";
            simResetCount = 0;
        }
    }

    if (totalResets >= config.providerFailureLimit)
    {
        if (providerFailureNotified)
        {
            // Już powiadomiliśmy, nie rób nic, tylko czekaj
            statusMsg = "Awaria dostawcy - oczekiwanie na interwencję";
            return;
        }

        // Awaria po stronie dostawcy - nie resetuj więcej
        Serial.println("AWARIA DOSTAWCY! Po " + String(totalResets) + " resetach bez sukcesu.");
        logEvent("AWARIA DOSTAWCY (Resety: " + String(totalResets) + ")");
        statusMsg = "Awaria dostawcy - zatrzymano resety";
        providerFailureNotified = true; // Zapobiegaj spamowaniu
        return;                         // Nie wykonuj resetu
    }

    totalResets++;                // Zwiększamy licznik
    config.routerResetCount++;    // Licznik resetów routera
    routerResetInProgress = true; // Blokuj watchdog podczas resetu

    // --- POPRAWKA: Obliczamy czasy PRZED restartem ---
    nextResetDelay += FIVE_MINUTES_MS; // Zwiększ opóźnienie o 5 min
    if (nextResetDelay > 3600000)
        nextResetDelay = 3600000; // Max 60 min (zmienilem na 60min, 12 to troche malo)

    statusMsg = "Trwa procedura resetu...";
    Serial.println("RESET RUTERA!");
    logEvent("RESET RUTERA (Total: " + String(totalResets) + ")");
    if (simPingFail || simNoWiFi || simHighPing)
    {
        simStatus = "Wyłączam router na " + String(config.routerOffTime / 1000) + "s...";
    }

    // Wyłączenie rutera
    digitalWrite(config.pinRelay, HIGH);
    safeDelay(config.routerOffTime);
    digitalWrite(config.pinRelay, LOW);

    if (simPingFail || simNoWiFi || simHighPing)
    {
        simStatus = "Czekam na restart routera (" + String(config.baseBootTime / 1000) + "s)...";
    }

    // Czekanie na wstanie rutera
    elapsedMillis bootTimer;
    while (bootTimer < (unsigned long)config.baseBootTime)
    {
        ESP.wdtFeed();
        delay(100); // Krótki delay w pętli - OK (pętla ma wyjście, karmi WDT)
        if ((millis() / 500) % 2 == 0)
            ledFail();
        else
            ledOK();
    }

    // Oznacz że router właśnie się włączył - grace period będzie obsługiwany w monitorInternetConnection()
    routerBootStartTime = millis();

    // Liczniki są utrzymywane w sekcji .noinit; nie zapisujemy na Flash aby ograniczyć zużycie.
    config.totalResets = totalResets;
    config.totalResetsEver = totalResetsEver;
    config.nextResetDelay = nextResetDelay;
    config.firstResetTime = (firstResetTime == 0) ? millis() : firstResetTime;
    config.lastResetTime = millis();
    config.failCount = 0;
    config.noWiFiStartTime = 0;

    // Zapisz liczniki do konfiguracji
    if (!saveConfig())
    {
        Serial.println("BŁĄD: Nie udało się zapisać config przed restartem!");
        logEvent("BLAD ZAPISU CONFIG PRZED RESTARTEM");
    }

    if (wasSimulation)
    {
        Serial.println("Tryb symulacji: Pomijam restart ESP.");
        logEvent("Symulacja: Pominieto restart ESP");
        simStatus = "Reset zakończony - wznawianie monitorowania...";
        failCount = 0;
        noWiFiStartTime = 0;
        routerResetInProgress = false; // Odblokuj watchdog
        return;
    }

    Serial.println("Restart ESP...");
    ESP.restart();
    // Tutaj kod już nie dotrze, i to jest OK.
}

void handleButtonPress()
{
    if (digitalRead(config.pinButton) == LOW)
    {
        unsigned long pressStartTime = millis();
        unsigned long pressDuration = 0;

        // Debounce
        delay(50);
        if (digitalRead(config.pinButton) != LOW)
            return;

        Serial.println("Przycisk wciśnięty...");

        // Czekaj na zwolnienie przycisku i mierz czas
        while (digitalRead(config.pinButton) == LOW)
        {
            delay(100);
            ESP.wdtFeed();
            pressDuration = millis() - pressStartTime;

            // Wizualna sygnalizacja długiego naciśnięcia (>10s)
            if (pressDuration > 10000)
            {
                // Miganie jako ostrzeżenie przed factory reset
                if ((pressDuration / 200) % 2 == 0)
                {
                    digitalWrite(config.pinRed, HIGH);
                    digitalWrite(config.pinGreen, HIGH);
                }
                else
                {
                    digitalWrite(config.pinRed, LOW);
                    digitalWrite(config.pinGreen, LOW);
                }
            }
        }

        // Oblicz finalny czas naciśnięcia
        pressDuration = millis() - pressStartTime;

        // === 3 POZIOMY NACIŚNIĘĆ ===

        if (pressDuration > 10000)
        {
            // DŁUGIE (>10s) -> Factory Reset
            Serial.println("Wykryto długie wciśnięcie (>10s) - PRZYWRACANIE USTAWIEŃ FABRYCZNYCH!");
            logEvent("PRZYCISK: Factory reset (>10s)");

            // Sygnalizacja LED
            for (int i = 0; i < 5; i++)
            {
                digitalWrite(config.pinRed, HIGH);
                digitalWrite(config.pinGreen, HIGH);
                delay(100);
                digitalWrite(config.pinRed, LOW);
                digitalWrite(config.pinGreen, LOW);
                delay(100);
            }

            // Usuwanie plików konfiguracyjnych
            if (LittleFS.exists("/config.json"))
                LittleFS.remove("/config.json");
            if (LittleFS.exists("/wifi_config.txt"))
                LittleFS.remove("/wifi_config.txt");
            if (LittleFS.exists("/events.log"))
                LittleFS.remove("/events.log");

            Serial.println("Ustawienia usunięte. Restart...");
            delay(1000);
            ESP.restart();
        }
        else if (pressDuration > 3000)
        {
            // ŚREDNIE (3-10s) -> Tryb AP
            Serial.println("Średnie wciśnięcie (3-10s) - uruchamiam tryb konfiguracyjny (AP)");
            logEvent("PRZYCISK: Tryb AP ręczny (3-10s)");
            WiFi.mode(WIFI_AP);
            uruchomAP();
            uruchommDNS();
            statusMsg = "Tryb konfiguracyjny - ręczny";
            ledFail();
        }
        else
        {
            // KRÓTKIE (<3s) -> Reset routera
            Serial.println("Krótkie wciśnięcie (<3s) - RESET ROUTERA");
            logEvent("PRZYCISK: Reset routera ręczny");

            // Sygnalizacja (niebieska dioda mruga)
            for (int i = 0; i < 3; i++)
            {
                digitalWrite(config.pinBlue, HIGH);
                delay(100);
                digitalWrite(config.pinBlue, LOW);
                delay(100);
            }

            wykonajReset();
        }
    }
}