#include "webserver.h"
#include "watchdog.h" // Dołączenie watchdog.h daje dostęp do zmiennych statusowych (failCount, itp.)
#include "config.h"
#include "constants.h"
#include "app_globals.h" // Centralne extern deklaracje
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Updater.h>
#include "WiFiConfig.h"  // Własna biblioteka WiFiConfig
#include "html_common.h" // Zunifikowany system HTML/CSS/JS
#include "diag.h"
#include "version.h"
void handleFactoryReset();   // Deklaracja funkcji
void handleReboot();         // Deklaracja funkcji
void handleSaveBrightness(); // Deklaracja funkcji - zapisuje jasność do Flash
void handleSimPingFail();    // Deklaracja funkcji symulacji awarii ping
void handleLoginPage();      // Formularz logowania
void handleLoginSubmit();    // Weryfikacja logowania
void handleDownloadLogs();   // Pobranie pliku logów

// Pozostałe funkcje i zmienne (tablica, uaktualnijTablicePlik itp.) są dostępne dzięki #include "WiFiConfig.h"

ESP8266WebServer server(80);

void setupWebServer()
{
    server.on("/", handleRoot);
    server.on("/login", HTTP_GET, handleLoginPage);
    server.on("/login", HTTP_POST, handleLoginSubmit);
    server.on("/reset", handleManualReset);
    server.on("/reboot", handleReboot);
    server.on("/config", handleConfig);
    server.on("/clearlogs", handleClearLogs);
    server.on("/saveconfig", HTTP_POST, handleSaveConfig);
    server.on("/addwifi", HTTP_POST, handleAddWiFi);
    server.on("/removewifi", HTTP_POST, handleRemoveWiFi);
    server.on("/manualconfig", handleManualConfig);
    server.on("/logout", handleLogout);
    server.on("/factoryreset", handleFactoryReset);
    server.on("/update", HTTP_GET, handleUpdatePage);
    server.on("/update", HTTP_POST, handleUpdateResult, handleUpdateUpload);
    server.on("/test/pingfail", handleSimPingFail);
    server.on("/test/nowifi", handleSimNoWiFi);
    server.on("/test/highping", handleSimHighPing);
    server.on("/test/stop", handleStopSim);
    server.on("/setbrightness", handleSetBrightness);
    server.on("/savebrightness", handleSaveBrightness);
    server.on("/downloadlogs", handleDownloadLogs);
    server.begin();
}

void handleRoot()
{
    sendHtmlHeader(server, "Strażnik Internetu", config.darkMode);

    String html = F("</head><body");
    html += F(" onload=\"initTheme()\"");
    html += F("><div class='container'>");
    html.reserve(2048);

    // Przełącznik trybu ciemnego
    html += F("<div style='display:flex; justify-content:flex-end; align-items:center; gap:20px;'>");
    html += F("<div class='switch-wrap'><span>Tryb ciemny</span><label class='switch'><input type='checkbox' id='themeSwitch' onchange='toggleTheme(this.checked)'");
    if (config.darkMode)
        html += F(" checked");
    html += F("><span class='slider'></span></label></div></div>");

    html += F("<h1>Strażnik Internetu</h1>");

    // Sekcja Statusu
    html += F("<div class='section'><h2>Status</h2>");

    if (failCount == 0)
    {
        html += F("<div style='padding:15px; background-color:#d4edda; color:#155724; border-radius:5px; margin-bottom:10px; border: 1px solid #c3e6cb;'><b>INTERNET DOSTĘPNY</b></div>");
    }
    else
    {
        html += F("<div style='padding:15px; background-color:#f8d7da; color:#721c24; border-radius:5px; margin-bottom:10px; border: 1px solid #f5c6cb;'><b>PROBLEMY Z SIECIĄ (");
        html += failCount;
        html += F("/");
        html += config.failLimit;
        html += F(")</b></div>");
    }

    html += F("<p>Ostatni Ping: <b>");
    html += lastPingMs;
    html += F(" ms</b></p>");
    html += F("<p>Liczba resetów routera: <b>");
    html += totalResets;
    html += F("</b></p>");
    html += F("<p>Komunikat: <b>");
    html += statusMsg;
    html += F("</b></p>");
    html += F("<p>Czas pracy (Uptime): <b>");
    html += (millis() / 1000 / 60);
    html += F(" min</b></p>");

    // Status Sieci Rezerwowej (Backup Network) - v1.1.2+
    if (config.enableBackupNetwork)
    {
        html += F("<p>Sieć rezerwowa: <b style='");
        if (config.backupNetworkActive)
            html += "color:#ff9800;"; // Pomarańczowy = rezerwowa
        else
            html += "color:#4caf50;"; // Zielony = główna
        html += F("'>");
        html += (config.backupNetworkActive ? "AKTYWNA (Backup)" : "Wyłączona (Główna)");
        html += F("</b></p>");

        if (config.backupNetworkActive)
        {
            html += F("<p>Błędy backupu: <b>");
            html += config.backupNetworkFailCount;
            html += F("/");
            html += config.backupNetworkFailLimit;
            html += F("</b></p>");
        }
    }

    html += F("</div>"); // Koniec sekcji Status

    // Sekcja Zdarzeń
    html += F("<div class='section'><h2>Ostatnie zdarzenia</h2>");
    html += F("<div style='background:var(--inp); padding:10px; border:1px solid var(--brd); border-radius:5px; max-height:200px; overflow-y:auto; font-family:monospace; font-size:0.9em;'>");
    File logFile = LittleFS.open(LOG_FILE, "r");
    if (logFile)
    {
        // Przechowuj tylko ostatnie 8 wpisów w buforze pierścieniowym (mniej Stringów = mniej RAM)
        const int maxShow = 8;
        String last[maxShow];
        int count = 0;
        while (logFile.available())
        {
            String line = logFile.readStringUntil('\n');
            if (line.length() == 0)
                continue;
            last[count % maxShow] = line;
            count++;
        }
        logFile.close();

        int toShow = (count < maxShow) ? count : maxShow;
        int start = (count >= maxShow) ? (count % maxShow) : 0;
        for (int i = 0; i < toShow; i++)
        {
            String line = last[(start + i) % maxShow];
            String lower = line;
            lower.toLowerCase();
            String style = "padding: 2px; border-bottom: 1px solid #eee;";
            if (lower.indexOf("blad") >= 0 || lower.indexOf("reset") >= 0 || lower.indexOf("brak") >= 0 || lower.indexOf("ota") >= 0)
            {
                style += "color:#dc3545; font-weight:bold;";
            }
            html += "<div style='" + style + "'>" + line + "</div>";
        }
    }
    else
    {
        html += F("Brak logów.");
    }
    html += F("</div>");
    html += F("</div>"); // Koniec sekcji Zdarzenia

    // Sekcja Akcji
    html += F("<div class='section'><h2>Akcje</h2>");
    html += F("<div style='display:flex; flex-wrap:wrap; gap:10px;'>");
    if (isSessionActive)
    {
        html += F("<a href='/reset' onclick=\"return confirm('Czy na pewno chcesz zresetować router?')\"><button style='background-color:#ff6b6b;'>Reset routera</button></a>");
        html += F("<a href='/reboot' onclick=\"return confirm('Czy na pewno chcesz zrestartować urządzenie (ESP)?')\"><button style='background-color:#dc3545;'>Restart urządzenia (ESP)</button></a>");
    }
    else
    {
        html += F("<span style='padding:8px; color:#666;'>Zaloguj się w konfiguracji, aby wykonać reset.</span>");
    }
    html += F("<a href='/config'><button>Konfiguracja</button></a>");
    html += F("<a href='/'><button style='background-color:#6c757d;'>Odśwież</button></a>");
    html += F("</div></div>");

    // Stopka z wersją oprogramowania (kontener jak w konfiguracji)
    html += F("<div class='section' style='text-align:center; border-top:1px solid var(--brd); padding-top:16px; color:#777; font-size:0.9em;'>Wersja oprogramowania: <b>");
    html += APP_VERSION;
    html += F("</b></div>");

    html += F("</div></body></html>");

    server.sendContent(html);
    server.sendContent(""); // Koniec transmisji
}

void handleManualReset()
{
    if (!checkAuth())
    {
        return;
    }
    Serial.println("RĘCZNY RESET ROUTERA!");
    logEvent("RECZNY RESET ROUTERA");

    // Blokuj watchdog podczas resetu
    routerResetInProgress = true;

    // Zarejestruj ręczny reset w liczniku
    config.routerResetCount++;
    if (!saveConfig())
    {
        Serial.println("BŁĄD: Nie udało się zapisać config po ręcznym resecie!");
        logEvent("BLAD ZAPISU CONFIG PO RECZNYM RESECIE");
    }

    // Oblicz całkowity czas resetu (wyłączenie + rozruch)
    int totalTime = (config.routerOffTime + config.baseBootTime) / 1000;
    String message = "Router jest resetowany. Wyłączenie na " + String(config.routerOffTime / 1000) + "s + rozruch " + String(config.baseBootTime / 1000) + "s.";

    // Wyświetl stronę z odliczaniem
    sendCountdownPage(server, "🔌 Reset routera",
                      message.c_str(),
                      totalTime, "/", config.darkMode);

    delay(500);
    digitalWrite(config.pinRelay, HIGH);
    safeDelay(config.routerOffTime); // Karmi WDT zamiast blokować
    digitalWrite(config.pinRelay, LOW);
    logEvent("RECZNY RESET ROUTERA zakonczony - router restartuje");

    // Czekaj na powrót WiFi lub timeout boot'u
    unsigned long resetStartTime = millis();
    bool wifiResumed = false;
    while (millis() - resetStartTime < (unsigned long)config.baseBootTime)
    {
        delay(100);
        // Sprawdzaj czy WiFi wróciło online
        if (WiFi.status() == WL_CONNECTED)
        {
            wifiResumed = true;
            delay(1000); // Czekaj jeszcze 1s na stabilizację
            break;
        }
    }

    // Odblokuj watchdog jak tylko WiFi wróci lub timeout
    routerResetInProgress = false;
    failCount = 0;       // Wyzeruj licznik błędów
    noWiFiStartTime = 0; // Wyzeruj licznik braku WiFi

    if (wifiResumed)
    {
        logEvent("Router powrócił do sieci WiFi");
    }
}

void handleClearLogs()
{
    if (!checkAuth())
    {
        return;
    }
    File file = LittleFS.open(LOG_FILE, "w");
    if (file)
        file.close(); // Otwarcie w trybie "w" czyści plik
    redirectTo(server, "/");
}

void handleDownloadLogs()
{
    if (!checkAuth())
    {
        return;
    }
    File file = LittleFS.open(LOG_FILE, "r");
    if (!file)
    {
        server.send(404, "text/plain", "Brak logów");
        return;
    }

    server.sendHeader("Content-Type", "text/plain; charset=utf-8");
    server.sendHeader("Content-Disposition", "attachment; filename=events.log");
    server.setContentLength(file.size());
    server.send(200);

    while (file.available())
    {
        String line = file.readStringUntil('\n');
        server.sendContent(line + "\n");
    }
    file.close();
    server.sendContent("");
}

void handleLogout()
{
    DIAG_PRINTLN(F("\n========== handleLogout START =========="));
    DIAG_PRINT(F("[LOGOUT] Session Active BEFORE logout: "));
    DIAG_PRINTLN(isSessionActive ? "TRUE" : "FALSE");
    DIAG_PRINT(F("[LOGOUT] Old session token: "));
    DIAG_PRINTLN(sessionToken);

    // Zniszcz sesję na serwerze
    isSessionActive = false;
    lastSessionActivity = 0;
    sessionToken = ""; // Kasuj token - stare ciasteczko nie będzie już ważne

    DIAG_PRINTLN(F("[LOGOUT] Session destroyed on server"));

    // Kasuj ciasteczko w przeglądarce (data z przeszłości)
    String deleteCookie = String(COOKIE_NAME) + "=deleted; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT";
    server.sendHeader("Set-Cookie", deleteCookie);
    redirectTo(server, "/");

    DIAG_PRINTLN(F("[LOGOUT] Cookie deleted, redirecting to /"));
    DIAG_PRINTLN(F("========== handleLogout END ==========\n"));
}

// --- SYMULACJA AWARII PING ---
void handleSimPingFail()
{
    if (!checkAuth())
        return;

    simPingFail = true;
    simStatus = "Rozpoczęto symulację awarii Ping - oczekiwanie na wykrycie...";
    Serial.println("Uruchomiono symulacje awarii Ping");
    logEvent("Uruchomiono symulacje awarii Ping");
    redirectTo(server, "/config");
}

void handleSimNoWiFi()
{
    if (!checkAuth())
        return;
    simNoWiFi = true;
    simStatus = "Rozpoczęto symulację braku WiFi - oczekiwanie na timeout (60s)...";
    Serial.println("Uruchomiono symulacje braku WiFi");
    logEvent("Uruchomiono symulacje braku WiFi");
    redirectTo(server, "/config");
}

void handleStopSim()
{
    if (!checkAuth())
        return;

    // Sprawdź czy była aktywna symulacja
    bool wasActive = (simPingFail || simNoWiFi || simHighPing);

    simPingFail = false;
    simNoWiFi = false;
    simHighPing = false;

    if (wasActive)
    {
        simStatus = "Symulacja zakończona - Internet przywrócony ręcznie";
        Serial.println("Symulacja zakończona - Internet przywrócony");
        logEvent("SYMULACJA: Powrót internetu (reczny)");
        // Zerujemy liczniki aby symulować powrót normalnego stanu
        failCount = 0;
        noWiFiStartTime = 0;
    }
    else
    {
        simStatus = "";
        Serial.println("Symulacje nie były aktywne");
    }

    redirectTo(server, "/config");
}

void handleSimHighPing()
{
    if (!checkAuth())
        return;
    simHighPing = true;
    simStatus = "Rozpoczęto symulację wysokiego ping - oczekiwanie na wykrycie...";
    Serial.println("Uruchomiono symulacje wysokiego ping");
    logEvent("Uruchomiono symulacje wysokiego ping");
    redirectTo(server, "/config");
}

// --- STRONA KONFIGURACYJNA ---
void handleConfig()
{
    DIAG_PRINTLN(F("\n========== handleConfig START =========="));
    DIAG_PRINT(F("[CONFIG] Client IP: "));
    DIAG_PRINTLN(server.client().remoteIP().toString());
    DIAG_PRINT(F("[CONFIG] Session Active BEFORE checkAuth: "));
    DIAG_PRINTLN(isSessionActive ? "TRUE" : "FALSE");

    if (!checkAuth())
    {
        DIAG_PRINTLN(F("[CONFIG] checkAuth returned FALSE - access DENIED"));
        DIAG_PRINTLN(F("========== handleConfig END (denied) ==========\n"));
        return;
    }

    DIAG_PRINTLN(F("[CONFIG] checkAuth returned TRUE - access GRANTED"));
    DIAG_PRINT(F("[CONFIG] Session Active AFTER checkAuth: "));
    DIAG_PRINTLN(isSessionActive ? "TRUE" : "FALSE");
    DIAG_PRINTLN(F("[CONFIG] Generating configuration page..."));

    Serial.print("[WEBSERVER] handleConfig: Current config values - ledBrightness=");
    Serial.print(config.ledBrightness);
    Serial.print(", darkMode=");
    Serial.print(config.darkMode);
    Serial.print(", pingInterval=");
    Serial.println(config.pingInterval);

    // Rozpocznij wysyłanie strumieniowe z zunifikowanym nagłówkiem
    sendHtmlHeader(server, "Konfiguracja - Strażnik Internetu", config.darkMode);

    // Rozpocznij body
    String html = F(R"rawliteral(
</head>
<body onload="initFields(); initTheme()">
    <div class="container">
        <h1>Konfiguracja Strażnika Internetu</h1>
        <form id="configForm" action="/saveconfig" method="POST">
        <div style="display:flex; justify-content:flex-end; align-items:center; gap:20px; margin-bottom:15px;">
            <div class="switch-wrap">
                <label>Jednostki: <select id="globalUnit" onchange="setGlobalUnit(this.value)" style="margin-left:5px; padding:4px;"><option value="1">ms</option><option value="1000" selected>s</option><option value="60000">min</option></select></label>
            </div>
            <div class="switch-wrap">
                <span>Tryb ciemny</span>
                <label class="switch">
                    <input type="checkbox" id="themeSwitch" name="darkMode" onchange="toggleTheme(this.checked)")rawliteral");
    if (config.darkMode)
        html += F(R"rawliteral( checked)rawliteral");
    html += F(R"rawliteral(>
                    <span class="slider"></span>
                </label>
            </div>
        </div>

        <div class="section">
            <h2>Diagnostyka resetów</h2>
            <div style="display:grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap:8px;">
                <div style="background:var(--inp); padding:10px; border:1px solid var(--brd); border-radius:6px;">Włączenie zasilania: <b>)rawliteral");
    html += config.resetDefault;
    html += F(R"rawliteral(</b></div>
                <div style="background:var(--inp); padding:10px; border:1px solid var(--brd); border-radius:6px;">Watchdog sprzętowy: <b>)rawliteral");
    html += config.resetWdt;
    html += F(R"rawliteral(</b></div>
                <div style="background:var(--inp); padding:10px; border:1px solid var(--brd); border-radius:6px;">Watchdog programowy: <b>)rawliteral");
    html += config.resetSoftWdt;
    html += F(R"rawliteral(</b></div>
                <div style="background:var(--inp); padding:10px; border:1px solid var(--brd); border-radius:6px;">Wyjątek/Crash: <b>)rawliteral");
    html += config.resetException;
    html += F(R"rawliteral(</b></div>
                <div style="background:var(--inp); padding:10px; border:1px solid var(--brd); border-radius:6px;">Soft restart: <b>)rawliteral");
    html += config.resetSoft;
    html += F(R"rawliteral(</b></div>
                <div style="background:var(--inp); padding:10px; border:1px solid var(--brd); border-radius:6px;">Wybudzenie z deep sleep: <b>)rawliteral");
    html += config.resetDeepSleep;
    html += F(R"rawliteral(</b></div>
                <div style="background:var(--inp); padding:10px; border:1px solid var(--brd); border-radius:6px;">Reset zewnętrzny: <b>)rawliteral");
    html += config.resetExt;
    html += F(R"rawliteral(</b></div>
                <div style="background:var(--inp); padding:10px; border:1px solid var(--brd); border-radius:6px;">Resety routera (łącznie): <b>)rawliteral");
    html += config.routerResetCount;
    html += F(R"rawliteral(</b></div>
            </div>
            <p style="font-size:0.85em; color:#666; margin-top:6px;">Liczby zapisywane w config.json na każdym starcie – pomagają wykryć WDT/exception vs. normalne resety. <b>Resety routera</b> to wszystkie resety routera wykonane przez ESP (automatyczne + ręczne).</p>
        </div>

        <div class="section">
            <h2>Legenda Diod LED</h2>
            <div style="display: flex; gap: 10px; flex-wrap: wrap;">
    )rawliteral");

    bool isBlue = (WiFi.getMode() == WIFI_AP);
    bool isRed = (!isBlue && (failCount > 0 || WiFi.status() != WL_CONNECTED || simNoWiFi));
    bool isGreen = (!isBlue && !isRed);

    html += F("<div style='display:flex; align-items:center; padding: 5px;");
    html += F("'><span style='height:15px; width:15px; background-color:green; border-radius:50%; display:inline-block; margin-right:5px; transition: box-shadow 0.3s;");
    if (isGreen)
        html += F(" box-shadow: 0 0 15px green;");
    html += F("'></span> <b>Zielona:</b> Internet OK</div>");

    html += F("<div style='display:flex; align-items:center; padding: 5px;");
    html += F("'><span style='height:15px; width:15px; background-color:red; border-radius:50%; display:inline-block; margin-right:5px; transition: box-shadow 0.3s;");
    if (isRed)
        html += F(" box-shadow: 0 0 15px red;");
    html += F("'></span> <b>Czerwona:</b> Awaria / Reset</div>");

    html += F("<div style='display:flex; align-items:center; padding: 5px;");
    html += F("'><span style='height:15px; width:15px; background-color:blue; border-radius:50%; display:inline-block; margin-right:5px; transition: box-shadow 0.3s;");
    if (isBlue)
        html += F(" box-shadow: 0 0 15px blue;");
    html += F("'></span> <b>Niebieska:</b> Tryb AP / Oczekiwanie</div>");

    html += F(R"rawliteral(
            </div>
            <p style="font-size: 0.9em; color: #666;">Dioda niebieska sygnalizuje tryb konfiguracyjny lub okres karencji po włączeniu zasilania.</p>
            <label for="ledBrightness" style="margin-top:12px;">Jasność diod LED:</label>
            <div style="display:flex; align-items:center; gap:10px;">
                <input type="range" id="ledBrightness" name="ledBrightness" min="0" max="255" value=")rawliteral");
    html += config.ledBrightness;
    html += F(R"rawliteral(" oninput="updateBrightness(this.value);">
                <span id="ledBrightnessVal">)rawliteral");
    html += config.ledBrightness;
    html += F(R"rawliteral(</span>
            </div>
            <p style="font-size: 0.85em; color:#666; margin-top:5px; margin-bottom:0px;">Dotyczy wszystkich diod (RGB). 0 = wyłączone, 255 = pełna jasność.</p>
        </div>

        

        <div class="section">
            <h2>⚙️ Parametry Watchdog - Ustawienia zaawansowane</h2>
            <p style="font-size:0.9em; color:#666; margin-bottom:20px;">Parametry podzielone według scenariuszy - kliknij aby rozwinąć sekcję.</p>

            <!-- Accordion Section 1: Podstawowe ustawienia -->
            <details class="accordion">
                <summary><b>📡 1. Podstawowe ustawienia monitoringu</b></summary>
                <div class="accordion-content">
                    <label for="pingInterval">Interwał ping: <span class="tooltip">?<span class="tooltiptext">Czas odstępu między sprawdzaniem połączenia.</span></span></label>
                    <div class="time-group">
                        <input type="number" id="pingInterval_disp" step="0.1" min="0" oninput="updateHidden('pingInterval')" value=")rawliteral");
    html += config.pingInterval;
    html += F(R"rawliteral(">
                        <select id="pingInterval_unit" class="unit-select" onchange="convertUnit('pingInterval')">
                            <option value="1">ms</option>
                            <option value="1000">s</option>
                            <option value="60000">min</option>
                        </select>
                    </div>
                    <input type="hidden" id="pingInterval" name="pingInterval" value=")rawliteral");
    html += config.pingInterval;
    html += F(R"rawliteral(">
                    
                    <label for="failLimit">Limit błędów przed resetem: <span class="tooltip">?<span class="tooltiptext">Liczba nieudanych prób ping przed resetem routera.</span></span></label>
                    <input type="number" id="failLimit" name="failLimit" value=")rawliteral");
    html += config.failLimit;
    html += F(R"rawliteral(" min="1" required>
                
                    
                    <h4>Sprawdzanie połączenia</h4>
                    <div style="margin-bottom:8px; font-size:0.9em; color:#555;">
                        Wykryta brama DHCP: <b>)rawliteral");
    String detectedGw = WiFi.gatewayIP().toString();
    html += detectedGw;
    html += F(R"rawliteral(</b> (używana, gdy nie podasz własnej)
                    </div>

                    <div class="switch-wrap" style="justify-content: flex-start; margin-bottom:8px;">
                        <label class="switch">
                            <input type="checkbox" id="useGatewayOverride" name="useGatewayOverride" )rawliteral");
    if (config.useGatewayOverride)
        html += "checked";
    html += F(R"rawliteral(>
                            <span class="slider"></span>
                        </label>
                        <span style="margin-left: 10px;">Użyj własnego adresu bramy</span>
                    </div>

                    <label for="gatewayOverride">Adres bramy (opcjonalnie): <span class="tooltip">?<span class="tooltiptext">Gdy włączysz przełącznik, watchdog pinguje ten adres zamiast bramy z DHCP.</span></span></label>
                    <input type="text" id="gatewayOverride" name="gatewayOverride" value=")rawliteral");
    html += (config.gatewayOverride.length() > 0 ? config.gatewayOverride : detectedGw);
    html += F(R"rawliteral(" pattern="^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$" title="Wprowadź poprawny adres IPv4 (np. 192.168.1.1)">

                    <label for="host1">Host 1 (serwer testowy): <span class="tooltip">?<span class="tooltiptext">Adres IP serwera do sprawdzania (np. 8.8.8.8).</span></span></label>
                    <input type="text" id="host1" name="host1" value=")rawliteral");
    html += config.host1;
    html += F(R"rawliteral(" required pattern="^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$" title="Wprowadź poprawny adres IPv4 (np. 8.8.8.8)">
                    
                    <label for="host2">Host 2 (zapasowy): <span class="tooltip">?<span class="tooltiptext">Zapasowy adres IP do sprawdzania.</span></span></label>
                    <input type="text" id="host2" name="host2" value=")rawliteral");
    html += config.host2;
    html += F(R"rawliteral(" required pattern="^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$" title="Wprowadź poprawny adres IPv4 (np. 1.1.1.1)">
                </div>
            </details>

            <!-- Accordion Section 2: Reset routera -->
            <details class="accordion">
                <summary><b>🔄 2. Parametry resetu routera</b></summary>
                <div class="accordion-content">
                    <label for="routerOffTime">Czas wyłączenia routera: <span class="tooltip">?<span class="tooltiptext">Czas odcięcia zasilania routera (długość resetu).</span></span></label>
                    <div class="time-group">
                        <input type="number" id="routerOffTime_disp" step="0.1" min="0" oninput="updateHidden('routerOffTime')" value=")rawliteral");
    html += config.routerOffTime;
    html += F(R"rawliteral(">
                        <select id="routerOffTime_unit" class="unit-select" onchange="convertUnit('routerOffTime')">
                            <option value="1">ms</option>
                            <option value="1000">s</option>
                            <option value="60000">min</option>
                        </select>
                    </div>
                    <input type="hidden" id="routerOffTime" name="routerOffTime" value=")rawliteral");
    html += config.routerOffTime;
    html += F(R"rawliteral(">
                    
                    <label for="baseBootTime">Czas rozruchu routera (grace period): <span class="tooltip">?<span class="tooltiptext">Czas na uruchomienie routera po włączeniu zasilania. ESP nie testuje internetu przez ten okres.</span></span></label>
                    <div class="time-group">
                        <input type="number" id="baseBootTime_disp" step="0.1" min="0" oninput="updateHidden('baseBootTime')" value=")rawliteral");
    html += config.baseBootTime;
    html += F(R"rawliteral(">
                        <select id="baseBootTime_unit" class="unit-select" onchange="convertUnit('baseBootTime')">
                            <option value="1">ms</option>
                            <option value="1000">s</option>
                            <option value="60000">min</option>
                        </select>
                    </div>
                    <input type="hidden" id="baseBootTime" name="baseBootTime" value=")rawliteral");
    html += config.baseBootTime;
    html += F(R"rawliteral(">
                </div>
            </details>

            <!-- Accordion Section 3: Safety Mode (Boot Loop) -->
            <details class="accordion">
                <summary><b>🛡️ 3. Ochrona przed boot loop (Safety Mode)</b></summary>
                <div class="accordion-content">
                    <label for="bootLoopWindowSeconds">Okno detekcji boot loop (w sekundach): <span class="tooltip">?<span class="tooltiptext">Jeśli ESP zresetuje się 5 razy w ciągu tego czasu, aktywuje się Safe Mode (router zablokowany, tryb AP). Domyślnie 1200s = 20 minut.</span></span></label>
                    <input type="number" id="bootLoopWindowSeconds" name="bootLoopWindowSeconds" value=")rawliteral");
    html += config.bootLoopWindowSeconds;
    html += F(R"rawliteral(" min="60" required>
                    <p style="font-size:0.85em; color:#666; margin-top:5px;">Formuła: (routerOffTime + baseBootTime + grace + testTime) × 5 resetów. Przykład: (60 + 150 + 150 + 30) × 5 = 1950s ≈ 32 min.</p>
                    
                    <div style="background:#fff3cd; padding:12px; border-radius:6px; margin-top:10px; border:1px solid #daa520;">
                        <b>Status Safe Mode:</b> <span style="color:)rawliteral");
    html += config.safeModeActive ? "red; font-weight:bold;\">⚠️ AKTYWNY - Router zablokowany!" : "green;\">✓ Nieaktywny";
    html += F(R"rawliteral(</span>
                    </div>
                </div>
            </details>

            <!-- Accordion Section 4: WiFi/AP Mode -->
            <details class="accordion">
                <summary><b>📶 4. Problemy z WiFi i tryb AP</b></summary>
                <div class="accordion-content">
                    <label for="noWiFiTimeout">Czas oczekiwania na WiFi przed resetem: <span class="tooltip">?<span class="tooltiptext">Po jakim czasie braku WiFi zresetować router.</span></span></label>
                    <div class="time-group">
                        <input type="number" id="noWiFiTimeout_disp" step="0.1" min="0" oninput="updateHidden('noWiFiTimeout')" value=")rawliteral");
    html += config.noWiFiTimeout;
    html += F(R"rawliteral(">
                        <select id="noWiFiTimeout_unit" class="unit-select" onchange="convertUnit('noWiFiTimeout')">
                            <option value="1">ms</option>
                            <option value="1000">s</option>
                            <option value="60000">min</option>
                        </select>
                    </div>
                    <input type="hidden" id="noWiFiTimeout" name="noWiFiTimeout" value=")rawliteral");
    html += config.noWiFiTimeout;
    html += F(R"rawliteral(">

                    <label for="apConfigTimeout">Timeout w trybie AP (oczekiwanie na konfigurację): <span class="tooltip">?<span class="tooltiptext">Po jakim czasie braku aktywności w AP, spróbować ponownie normalnego trybu STA.</span></span></label>
                    <div class="time-group">
                        <input type="number" id="apConfigTimeout_disp" step="0.1" min="0" oninput="updateHidden('apConfigTimeout')" value=")rawliteral");
    html += config.apConfigTimeout;
    html += F(R"rawliteral(">
                        <select id="apConfigTimeout_unit" class="unit-select" onchange="convertUnit('apConfigTimeout')">
                            <option value="1">ms</option>
                            <option value="1000">s</option>
                            <option value="60000">min</option>
                        </select>
                    </div>
                    <input type="hidden" id="apConfigTimeout" name="apConfigTimeout" value=")rawliteral");
    html += config.apConfigTimeout;
    html += F(R"rawliteral(">
                    
                    <label for="apMaxAttempts">Maksymalna liczba prób wyjścia z AP: <span class="tooltip">?<span class="tooltiptext">Po ilu nieudanych próbach połączenia z WiFi, zamiast trybu AP wykonać reset routera. Domyślnie 4.</span></span></label>
                    <input type="number" id="apMaxAttempts" name="apMaxAttempts" value=")rawliteral");
    html += config.apMaxAttempts;
    html += F(R"rawliteral(" min="1" required>
                    
                    <label for="apBackoffMs">Backoff po porażce AP (okno ochronne): <span class="tooltip">?<span class="tooltiptext">Czas oczekiwania po nieudanej próbie wyjścia z AP przed kolejną próbą. Domyślnie 60 minut.</span></span></label>
                    <div class="time-group">
                        <input type="number" id="apBackoffMs_disp" step="0.1" min="0" oninput="updateHidden('apBackoffMs')" value=")rawliteral");
    html += config.apBackoffMs;
    html += F(R"rawliteral(">
                        <select id="apBackoffMs_unit" class="unit-select" onchange="convertUnit('apBackoffMs')">
                            <option value="1">ms</option>
                            <option value="1000">s</option>
                            <option value="60000">min</option>
                        </select>
                    </div>
                    <input type="hidden" id="apBackoffMs" name="apBackoffMs" value=")rawliteral");
    html += config.apBackoffMs;
    html += F(R"rawliteral(">
                    
                    <label for="dhcpTimeoutMs">Timeout DHCP: <span class="tooltip">?<span class="tooltiptext">Maksymalny czas oczekiwania na przydzielenie adresu IP przez DHCP. Domyślnie 5 minut.</span></span></label>
                    <div class="time-group">
                        <input type="number" id="dhcpTimeoutMs_disp" step="0.1" min="0" oninput="updateHidden('dhcpTimeoutMs')" value=")rawliteral");
    html += config.dhcpTimeoutMs;
    html += F(R"rawliteral(">
                        <select id="dhcpTimeoutMs_unit" class="unit-select" onchange="convertUnit('dhcpTimeoutMs')">
                            <option value="1">ms</option>
                            <option value="1000">s</option>
                            <option value="60000">min</option>
                        </select>
                    </div>
                    <input type="hidden" id="dhcpTimeoutMs" name="dhcpTimeoutMs" value=")rawliteral");
    html += config.dhcpTimeoutMs;
    html += F(R"rawliteral(">
                </div>
            </details>

            <!-- Accordion Section 5: Awarie dostawcy -->
            <details class="accordion">
                <summary><b>🌐 5. Awarie dostawcy internetu</b></summary>
                <div class="accordion-content">
                    <label for="providerFailureLimit">Limit resetów dla awarii dostawcy: <span class="tooltip">?<span class="tooltiptext">Po ilu resetach bez sukcesu uznać awarię po stronie dostawcy (zamiast problemu z routerem).</span></span></label>
                    <input type="number" id="providerFailureLimit" name="providerFailureLimit" value=")rawliteral");
    html += config.providerFailureLimit;
    html += F(R"rawliteral(" min="1" required>
                    
                    <div class="switch-wrap" style="justify-content: flex-start; margin-top: 10px;">
                        <label class="switch">
                            <input type="checkbox" id="noWiFiBackoff" name="noWiFiBackoff" )rawliteral");
    if (config.noWiFiBackoff)
        html += "checked";
    html += F(R"rawliteral(>
                            <span class="slider"></span>
                        </label>
                        <span style="margin-left: 10px;">Wydłużaj czas przy powtarzającej się awarii (Exponential Backoff)</span>
                    </div>
                </div>
            </details>

            <!-- Accordion Section 6: Lag Detection -->
            <details class="accordion">
                <summary><b>⏱️ 6. Detekcja opóźnień (Lag Watchdog)</b></summary>
                <div class="accordion-content">
                    <label for="maxPingMs">Maksymalny czas ping (próg lagu): <span class="tooltip">?<span class="tooltiptext">Próg detekcji wysokiego opóźnienia. Jeśli ping przekroczy tę wartość wielokrotnie, router zostaje zresetowany.</span></span></label>
                    <div class="time-group">
                        <input type="number" id="maxPingMs_disp" step="0.1" min="0" oninput="updateHidden('maxPingMs')" value=")rawliteral");
    html += config.maxPingMs;
    html += F(R"rawliteral(">
                        <select id="maxPingMs_unit" class="unit-select" onchange="convertUnit('maxPingMs')">
                            <option value="1">ms</option>
                            <option value="1000">s</option>
                            <option value="60000">min</option>
                        </select>
                    </div>
                    <input type="hidden" id="maxPingMs" name="maxPingMs" value=")rawliteral");
    html += config.maxPingMs;
    html += F(R"rawliteral(">
                    
                    <label for="lagRetries">Liczba spike'ów do potwierdzenia lagu: <span class="tooltip">?<span class="tooltiptext">Ile kolejnych pingów musi przekroczyć próg, aby uznać że to rzeczywisty lag (nie pojedynczy spike). Domyślnie 3.</span></span></label>
                    <input type="number" id="lagRetries" name="lagRetries" value=")rawliteral");
    html += config.lagRetries;
    html += F(R"rawliteral(" min="1" required>
                </div>
            </details>

            <!-- Accordion Section 7: Zaplanowane resety -->
            <details class="accordion">
                <summary><b>📅 7. Zaplanowane resety i auto-reset liczników</b></summary>
                <div class="accordion-content">
                    <label for="scheduledResetsEnabled">Włącz zaplanowane resety: <span class="tooltip">?<span class="tooltiptext">Resetuj router o określonych czasach (HH:MM) niezależnie od stanu łącza - proaktywna konserwacja.</span></span></label>
                    <input type="checkbox" id="scheduledResetsEnabled" name="scheduledResetsEnabled" )rawliteral");
    html += config.scheduledResetsEnabled ? "checked" : "";
    html += F(R"rawliteral(>
                    
                    <label style="margin-top:10px;">Czasy zaplanowanych resetów (format HH:MM, puste = wyłączone):</label>
                    <div style="display: grid; grid-template-columns: repeat(5, 1fr); gap: 10px;">)rawliteral");

    for (int i = 0; i < 5; i++)
    {
        html += F(R"rawliteral(
                        <div>
                            <label for="resetTime)rawliteral");
        html += String(i);
        html += F(R"rawliteral(">Reset )rawliteral");
        html += String(i + 1);
        html += F(R"rawliteral(:</label>
                            <input type="text" id="resetTime)rawliteral");
        html += String(i);
        html += F(R"rawliteral(" name="resetTime)rawliteral");
        html += String(i);
        html += F(R"rawliteral(" value=")rawliteral");
        html += config.scheduledResetTimes[i];
        html += F(R"rawliteral(" placeholder="HH:MM" maxlength="5" pattern="\d{2}:\d{2}">
                        </div>)rawliteral");
    }

    html += F(R"rawliteral(
                    </div>
                    
                    <label for="autoResetCountersHours" style="margin-top:15px;">Auto-reset liczników po X godzinach (0=wyłączony): <span class="tooltip">?<span class="tooltiptext">Jeśli urządzenie akumuluje czas awarii przez określoną liczbę godzin, wszystkie liczniki awarii zostaną zresetowane - "czysta karta". 0 = wyłączone.</span></span></label>
                    <input type="number" id="autoResetCountersHours" name="autoResetCountersHours" value=")rawliteral");
    html += config.autoResetCountersHours;
    html += F(R"rawliteral(" min="0" required>
                </div>
            </details>
        </div>
    )rawliteral");

    // --- Watchdog Control ---
    html += F(R"rawliteral(
        <details class="section accordion">
            <summary><h2>🛡️ Kontrola Watchdog (Monitorowanie)</h2></summary>
            <div class="accordion-content">
                <div style="background:#1a3a1a; color:#a8f5a8; padding:12px; border-radius:6px; margin-bottom:15px; border:1px solid #4ade80;">
                    <b>⚠️ OSTRZEŻENIE - Dezaktywacja Watchdog:</b><br>
                    • Wyłączenie watchdog <b>WYŁĄCZA</b> monitorowanie awarii internetu<br>
                    • <b>BRAK automatycznych resetów</b> przy braku internetu<br>
                    • Router będzie resetowany <b>TYLKO</b> ręcznie<br>
                    • Zaplanowane resety mogą być ignorowane<br>
                    <b>Używaj TYLKO dla testów lub gdy monitorowanie jest obsługiwane inaczej!</b>
                </div>
                <label for="watchdogEnabled">Włącz Watchdog (Automatyczne resety): <span class="tooltip">?<span class="tooltiptext">Jeśli wyłączone, urządzenie nie będzie monitorować połączenia i nie będzie resetować routera automatycznie. Brak internetu nie spowoduje żadnych działań.</span></span></label>
                <input type="checkbox" id="watchdogEnabled" name="watchdogEnabled" )rawliteral");
    html += config.watchdogEnabled ? "checked" : "";
    html += F(R"rawliteral(>
            </div>
        </details>
    )rawliteral");

    // --- Tryb pracy ---
    html += F(R"rawliteral(
        <details class="section accordion">
            <summary><h2>Tryb pracy</h2></summary>
            <div class="accordion-content">
                <div style="background:#3a2f0f; color:#f8e7a1; padding:12px; border-radius:6px; margin-bottom:15px; border:1px solid #c59f2b;">
                    <b>⚠️ WAŻNE - Tryb przerywany:</b><br>
                    Deep sleep <b>WYŁĄCZA</b> funkcję watchdog internetu! ESP śpi i nie monitoruje połączenia.<br>
                    • Router <b>NIE zostanie zresetowany</b> podczas snu ESP<br>
                    • Scheduled resety mogą być pominięte jeśli przypadną na czas snu<br>
                    • WebUI będzie niedostępne podczas snu<br>
                    <b>Użyj tylko jeśli priorytetem jest oszczędność energii, nie monitoring 24/7!</b>
                </div>
                <div style="display:flex; gap:20px; align-items:center; flex-wrap:wrap;">
                    <label><input type="radio" name="workMode" value="continuous" )rawliteral");
    if (!config.intermittentMode)
        html += F("checked ");
    html += F(R"rawliteral(/> Praca ciągła (Watchdog 24/7)</label>
                    <label><input type="radio" name="workMode" value="intermittent" )rawliteral");
    if (config.intermittentMode)
        html += F("checked ");
    html += F(R"rawliteral(/> Praca przerywana (Deep sleep - BEZ watchdog!)</label>
                </div>
                <div id="dutyFields" style="margin-top:12px; padding:12px; border:1px solid var(--brd); border-radius:6px; background:var(--inp); opacity:1; transition:opacity 0.3s; pointer-events:auto;">
                    <label for="awakeWindowMs">Czas aktywności przed snem:</label>
                    <div class="time-group">
                        <input type="number" id="awakeWindowMs_disp" step="0.1" min="0" oninput="updateHidden('awakeWindowMs')" value=")rawliteral");
    html += config.awakeWindowMs;
    html += F(R"rawliteral(">
                        <select id="awakeWindowMs_unit" class="unit-select" onchange="convertUnit('awakeWindowMs')">
                            <option value="1">ms</option>
                            <option value="1000">s</option>
                            <option value="60000">min</option>
                        </select>
                    </div>
                    <input type="hidden" id="awakeWindowMs" name="awakeWindowMs" value=")rawliteral");
    html += config.awakeWindowMs;
    html += F(R"rawliteral(">

                    <label for="sleepWindowMs" style="margin-top:10px;">Czas uśpienia ESP: <span style="color:#c00; font-weight:bold;">*</span></label>
                    <div class="time-group">
                        <input type="number" id="sleepWindowMs_disp" step="0.1" min="0" oninput="updateHidden('sleepWindowMs'); validateSleepTimes();" value=")rawliteral");
    html += config.sleepWindowMs;
    html += F(R"rawliteral(">
                        <select id="sleepWindowMs_unit" class="unit-select" onchange="convertUnit('sleepWindowMs'); validateSleepTimes();">
                            <option value="1">ms</option>
                            <option value="1000">s</option>
                            <option value="60000">min</option>
                        </select>
                    </div>
                    <input type="hidden" id="sleepWindowMs" name="sleepWindowMs" value=")rawliteral");
    html += config.sleepWindowMs;
    html += F(R"rawliteral(">
                    <div style="background:#3a2f0f; color:#f8e7a1; padding:10px; border-radius:4px; margin-top:10px; font-size:0.85em; border:1px solid #c59f2b;">
                      <b>⚠️ Obowiązkowe zakresy:</b><br>
                      • Minimalny czas: <b>5 minut</b> (300s) – aby ESP8266 zdążył się wybudzić<br>
                      • Maksymalny czas: <b>60 minut</b> (3600s) – limit deep sleep ESP8266
                    </div>
                    <p style="font-size:0.85em; color:#666; margin-top:8px;">Uśpienie korzysta z deep sleep (wymaga połączenia GPIO16→RST do RST). Wybudzenie przez timer lub ręczny reset.</p>
                </div>
            </div>
        </details>

        <details class="section accordion">
            <summary><h2>Sieci WiFi</h2></summary>
            <div class="accordion-content">
                <label for="ssid">Nazwa sieci (SSID):</label>
                <input type="text" id="ssid" name="ssid" placeholder="Wprowadź SSID sieci WiFi">
                
                <label for="wifipass">Hasło sieci:</label>
                <div class="time-group">
                    <input type="password" id="wifipass" name="wifipass" placeholder="Hasło WiFi">
                    <button type="button" onclick="togglePassword('wifipass')">👁️</button>
                </div>
                
                <label for="networkType">Typ sieci: <span class="tooltip">?<span class="tooltiptext">Główna: domyślna sieć. Rezerwowa: włącza się gdy główna zawiedzie (wymaga drugiego routera i przekaźnika).</span></span></label>
                <select id="networkType" name="networkType">
                    <option value="0">🟢 Główna (Primary)</option>
                    <option value="1">🔴 Rezerwowa (Backup)</option>
                </select>
                
                <div style="text-align: center; margin-top: 15px;">
                    <button type="button" onclick="addWiFiNetwork()" style="padding: 10px 20px; background-color: #007bff;">Zapisz/dodaj sieć WiFi</button>
                </div>
                
                <h4 style="margin-top: 25px;">Zapisane sieci:</h4>
                <div class="wifi-list" id="wifiList">
    )rawliteral");
    server.sendContent(html); // Wyślij pierwszą część (duża lista)
    html = "";

    // Lista sieci
    for (int i = 0; i < wielkoscTablicy; i++)
    {
        if (tablica[i].ssid.length() > 0)
        {
            String networkTypeLabel = (tablica[i].networkType == 1) ? "🔴 Rezerwowa" : "🟢 Główna";
            html += F("<div class='wifi-item'>");
            html += F("<span>");
            html += tablica[i].ssid;
            html += F(" <small style='color: var(--fg); opacity: 0.7;'>[");
            html += networkTypeLabel;
            html += F("]</small>");
            html += F("</span>");
            html += F("<form action='/removewifi' method='POST' style='display:inline;'>");
            html += F("<input type='hidden' name='index' value='");
            html += i;
            html += F("'>");
            html += F("<button type='submit'>Usuń</button>");
            html += F("</form>");
            html += F("</div>");
        }
    }

    html += F(R"rawliteral(
                </div>
            </div>
        </details>

        <details class="section accordion">
            <summary><h2>Zabezpieczenia (Panel i OTA)</h2></summary>
            <div class="accordion-content">
                <label for="adminUser">Login administratora: <span class="tooltip">?<span class="tooltiptext">Nazwa użytkownika do logowania w panelu.</span></span></label>
                <input type="text" id="adminUser" name="adminUser" value=")rawliteral");
    html += config.adminUser;
    html += F(R"rawliteral(" placeholder="admin">
                
                <label for="adminPass">Hasło administratora: <span class="tooltip">?<span class="tooltiptext">Hasło do panelu administratora.</span></span></label>
                <div class="time-group">
                    <input type="password" id="adminPass" name="adminPass" value=")rawliteral");
    html += config.adminPass;
    html += F(R"rawliteral(" placeholder="admin">
                    <button type="button" onclick="togglePassword('adminPass')">👁️</button>
                </div>

                <div style="text-align: center; margin-top: 30px; margin-bottom: 30px;">
                    <button type="submit" style="padding: 12px 30px; font-size: 1.1em; background-color: #28a745;">Zapisz konfigurację</button>
                </div>
            </div>
        </details>

        <div class="section">
            <h3>Diagnostyka i Testy</h3>
                <p>Scenariusze testowe pozwalają sprawdzić reakcję urządzenia.</p>
                <p style="font-size:0.9em; color:#666; margin-top:5px;">ℹ️ Symulacje kończą się automatycznie po 3 resetach lub można je zakończyć ręcznie przyciskiem "Symuluj powrót internetu".</p>
                
                <div style="margin-bottom: 15px; padding: 10px; border: 1px solid #ccc; border-radius: 5px; background-color: var(--inp);">
                    <b>Status symulacji:</b> 
                    <span style="font-weight:bold; color: )rawliteral");
    if (simPingFail || simNoWiFi || simHighPing)
        html += "red";
    else
        html += "green";
    html += F(R"rawliteral(">)rawliteral");
    if (simPingFail)
        html += "Awaria Ping (Aktywna)";
    else if (simNoWiFi)
        html += "Brak WiFi (Aktywna)";
    else if (simHighPing)
        html += "Wysoki Ping (Aktywna)";
    else
        html += "Brak (Normalna praca)";
    html += F(R"rawliteral(</span>)rawliteral");
    if (simStatus.length() > 0)
    {
        html += F(R"rawliteral(<br><span style="font-size:0.9em; color:#666;">)rawliteral");
        html += simStatus;
        html += F(R"rawliteral(</span>)rawliteral");
    }
    html += F(R"rawliteral(
                </div>

                <div style="display:flex; flex-wrap:wrap; gap:10px;">
                    <a href="/test/pingfail"><button type="button" style="background-color:#dc3545;">Symuluj awarię Ping</button></a>
                    <a href="/test/highping"><button type="button" style="background-color:#ffc107;">Symuluj wysoki ping (lag)</button></a>
                    <a href="/test/nowifi"><button type="button" style="background-color:#fd7e14;">Symuluj brak WiFi (1 min)</button></a>
                    <a href="/test/stop"><button type="button" style="background-color:#28a745;">✓ Symuluj powrót internetu</button></a>
                </div>
                
                <h3 style="margin-top: 30px;">Inne opcje</h3>
                <div style="display:flex; flex-wrap:wrap; gap:10px;">
                    <a href="/"><button type="button">Powrót do statusu</button></a>
                    <a href="/reset" onclick="return confirm('Czy na pewno chcesz zresetować router?')"><button type="button" style="background-color:#ff6b6b;">Reset routera</button></a>
                    <a href="/reboot" onclick="return confirm('Czy na pewno chcesz zrestartować urządzenie (ESP)?')"><button type="button" style="background-color:#dc3545;">Restart urządzenia (ESP)</button></a>
                    <a href="/downloadlogs"><button type="button" style="background-color: #007bff;">Pobierz logi</button></a>
                    <a href="/clearlogs"><button type="button" style="background-color: #ffc107; color: black;">Wyczyść logi</button></a>
                    <a href="/update"><button type="button" style="background-color: #17a2b8;">Aktualizacja (OTA)</button></a>
                    <a href="/factoryreset" onclick="return confirm('Czy na pewno chcesz przywrócić ustawienia fabryczne? Spowoduje to usunięcie konfiguracji WiFi i wszystkich ustawień.')"><button type="button" style="background-color: #dc3545;">Przywróć ustawienia fabryczne</button></a>)rawliteral");

    // Wyświetl przycisk tylko jeśli NIE jesteśmy w trybie AP (czyli jesteśmy połączeni z routerem)
    if (WiFi.getMode() != WIFI_AP)
    {
        html += F(R"rawliteral(
                <a href="/manualconfig"><button type="button">Ręczny tryb konfiguracyjny (Wymuś AP)</button></a>)rawliteral");
    }

    html += F(R"rawliteral(
                    <a href="/logout"><button type="button" style="background-color: #6c757d;">Wyloguj</button></a>
                </div>
            </form>
            <div style="text-align:center; border-top:1px solid var(--brd); padding-top:16px; margin-top:20px; color:#777; font-size:0.9em;">Wersja oprogramowania: <b>)rawliteral");
    html += APP_VERSION;
    html += F(R"rawliteral(</b></div>
        </div>
    </div>
    <script>
    const SESSION_MS = 300000; // 5 minut
    const loadTime = Date.now();
    let countdownInterval = null;
    
    function updateSessionCountdown() {
        const elapsed = Date.now() - loadTime;
        const msLeft = SESSION_MS - elapsed;
        
        if (msLeft <= 0) {
            if (countdownInterval) clearInterval(countdownInterval);
            alert('Twoja sesja wygasła. Zostaniesz wylogowany.');
            window.location.href = '/';
            return;
        }
        
        const totalSec = Math.ceil(msLeft / 1000);
        const minutes = Math.floor(totalSec / 60);
        const seconds = totalSec % 60;
        const timeStr = minutes + ':' + (seconds < 10 ? '0' : '') + seconds;
        const timeElTop = document.getElementById('timeLeftTop');
        if (timeElTop) {
            timeElTop.textContent = timeStr;
        }
    }
    
    function startSessionCountdown() {
        if (!countdownInterval) {
            updateSessionCountdown();
            countdownInterval = setInterval(updateSessionCountdown, 1000);
        }
    }
    
    // Ostrzegaj przed opuszczeniem strony, jeśli są niezapisane dane
    let isDirty = false;
    const configForm = document.getElementById('configForm');
    if (configForm) {
        configForm.addEventListener('change', () => { isDirty = true; });
        configForm.addEventListener('submit', () => { isDirty = false; });
        window.addEventListener('beforeunload', (e) => {
            if (isDirty) {
                e.preventDefault();
                e.returnValue = '';
            }
        });
    }
    
    // Funkcja do dodawania sieci WiFi
    function addWiFiNetwork() {
        const ssid = document.getElementById('ssid').value;
        const pass = document.getElementById('wifipass').value;
        
        if (!ssid || ssid.trim() === '') {
            alert('Podaj nazwę sieci (SSID)!');
            return;
        }
        
        const formData = new URLSearchParams();
        formData.append('ssid', ssid);
        formData.append('pass', pass);
        
        fetch('/addwifi', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
            },
            body: formData.toString()
        })
        .then(response => {
            if (response.ok) {
                alert('Sieć WiFi została dodana!');
                // Wyczyść pola
                document.getElementById('ssid').value = '';
                document.getElementById('wifipass').value = '';
                // Odśwież stronę aby pokazać zaktualizowaną listę
                window.location.reload();
            } else {
                alert('Błąd podczas dodawania sieci WiFi.');
            }
        })
        .catch(error => {
            console.error('Error:', error);
            alert('Wystąpił błąd podczas komunikacji z urządzeniem.');
        });
    }
    
    function initFields() {
        // Przywróć stan trybu ciemnego będzie obsłużony przez initTheme()
        // (usunięto duplikację kodu)
        // Przywróć pola czasu
        initTimeField('pingInterval', )rawliteral");
    html += config.pingInterval;
    html += F(R"rawliteral());
        initTimeField('routerOffTime', )rawliteral");
    html += config.routerOffTime;
    html += F(R"rawliteral());
        initTimeField('baseBootTime', )rawliteral");
    html += config.baseBootTime;
    html += F(R"rawliteral());
        initTimeField('noWiFiTimeout', )rawliteral");
    html += config.noWiFiTimeout;
    html += F(R"rawliteral());
        initTimeField('apConfigTimeout', )rawliteral");
    html += config.apConfigTimeout;
    html += F(R"rawliteral());
        initTimeField('awakeWindowMs', )rawliteral");
    html += config.awakeWindowMs;
    html += F(R"rawliteral());
        initTimeField('sleepWindowMs', )rawliteral");
    html += config.sleepWindowMs;
    html += F(R"rawliteral());
        initTimeField('maxPingMs', )rawliteral");
    html += config.maxPingMs;
    html += F(R"rawliteral());
        function toggleDutyFields() {
            const intermittent = document.querySelector('input[name="workMode"][value="intermittent"]').checked;
            const dutyDiv = document.getElementById('dutyFields');
            
            if (intermittent) {
                // Włącz sekcję
                dutyDiv.style.display = 'block';
                dutyDiv.style.opacity = '1';
                dutyDiv.style.pointerEvents = 'auto';
                dutyDiv.style.filter = 'none';
            } else {
                // Wyłącz sekcję wizualnie
                dutyDiv.style.display = 'none';
                dutyDiv.style.opacity = '0.4';
                dutyDiv.style.pointerEvents = 'none';
                dutyDiv.style.filter = 'grayscale(1)';
            }
            
            // Włącz/wyłącz pola
            const inputs = dutyDiv.querySelectorAll('input, select');
            inputs.forEach(input => {
                input.disabled = !intermittent;
            });
        }
        document.querySelectorAll('input[name="workMode"]').forEach(r => r.addEventListener('change', toggleDutyFields));
        toggleDutyFields();
        // Wywołaj initTheme aby przywrócić zapisany tryb
        initTheme();
    }
    </script>
</body>
</html>
)rawliteral");

    server.sendContent(html);
    server.sendContent(""); // Koniec transmisji
}

void handleSaveConfig()
{
    if (!checkAuth())
    {
        return;
    }
    if (server.method() == HTTP_POST)
    {
        Serial.println("[WEBSERVER] Received config save request");
        config.pingInterval = server.arg("pingInterval").toInt();
        config.failLimit = server.arg("failLimit").toInt();
        config.providerFailureLimit = server.arg("providerFailureLimit").toInt();
        config.autoResetCountersHours = server.arg("autoResetCountersHours").toInt();
        config.scheduledResetsEnabled = server.hasArg("scheduledResetsEnabled");
        config.watchdogEnabled = server.hasArg("watchdogEnabled");

        // Ładowanie zaplanowanych czasów resetów (format HH:MM)
        for (int i = 0; i < 5; i++)
        {
            String argName = "resetTime" + String(i);
            if (server.hasArg(argName))
            {
                String timeStr = server.arg(argName);
                // Walidacja formatu HH:MM
                if (timeStr.length() == 5 && timeStr[2] == ':')
                {
                    config.scheduledResetTimes[i] = timeStr;
                }
                else
                {
                    config.scheduledResetTimes[i] = ""; // Puste jeśli niepoprawny format
                }
            }
            else
            {
                config.scheduledResetTimes[i] = "";
            }
        }

        config.maxPingMs = server.arg("maxPingMs").toInt();
        config.lagRetries = server.arg("lagRetries").toInt();
        config.routerOffTime = server.arg("routerOffTime").toInt();
        config.baseBootTime = server.arg("baseBootTime").toInt();
        config.bootLoopWindowSeconds = server.arg("bootLoopWindowSeconds").toInt();
        config.noWiFiTimeout = server.arg("noWiFiTimeout").toInt();
        config.apConfigTimeout = server.arg("apConfigTimeout").toInt();
        config.apMaxAttempts = server.arg("apMaxAttempts").toInt();
        config.apBackoffMs = server.arg("apBackoffMs").toInt();
        config.dhcpTimeoutMs = server.arg("dhcpTimeoutMs").toInt();
        config.noWiFiBackoff = server.hasArg("noWiFiBackoff"); // Checkbox zwraca true jeśli jest zaznaczony
        config.darkMode = server.hasArg("darkMode");           // Zapisz stan trybu ciemnego
        config.intermittentMode = (server.arg("workMode") == "intermittent");
        config.awakeWindowMs = server.arg("awakeWindowMs").toInt();
        config.sleepWindowMs = server.arg("sleepWindowMs").toInt();
        config.ledBrightness = constrain(server.arg("ledBrightness").toInt(), 0, 255);
        config.host1 = server.arg("host1");
        config.host2 = server.arg("host2");
        config.gatewayOverride = server.arg("gatewayOverride");
        config.useGatewayOverride = server.hasArg("useGatewayOverride");
        config.adminUser = server.arg("adminUser");
        config.adminPass = server.arg("adminPass");

        // === BACKUP NETWORK ===
        config.enableBackupNetwork = server.hasArg("enableBackupNetwork");
        config.backupNetworkFailLimit = constrain(server.arg("backupNetworkFailLimit").toInt(), 1, 10);
        config.backupNetworkRetryInterval = server.arg("backupNetworkRetryInterval").toInt();
        if (config.backupNetworkRetryInterval <= 0)
            config.backupNetworkRetryInterval = 600000; // Default 10 min
        config.pinRelayBackup = server.arg("pinRelayBackup").toInt();

        Serial.print("[WEBSERVER] Parsed config - ledBrightness=");
        Serial.print(config.ledBrightness);
        Serial.print(", darkMode=");
        Serial.print(config.darkMode);
        Serial.print(", pingInterval=");
        Serial.println(config.pingInterval);

        // Walidacja wartości
        if (config.pingInterval <= 0)
        {
            sendErrorPage(server, "❌ Błąd walidacji", "Interwał ping musi być > 0", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }
        if (config.failLimit <= 0)
        {
            sendErrorPage(server, "❌ Błąd walidacji", "Limit błędów musi być > 0", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }
        if (config.providerFailureLimit <= 0)
        {
            sendErrorPage(server, "❌ Błąd walidacji", "Limit resetów dla dostawcy musi być > 0", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }
        if (config.maxPingMs <= 0)
        {
            sendErrorPage(server, "❌ Błąd walidacji", "Maksymalny ping musi być > 0", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }
        if (config.lagRetries <= 0)
        {
            sendErrorPage(server, "❌ Błąd walidacji", "Liczba spike'ów (lagRetries) musi być > 0", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }
        if (config.bootLoopWindowSeconds < 60)
        {
            sendErrorPage(server, "❌ Błąd walidacji", "Okno boot loop musi być ≥ 60 sekund", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }
        if (config.apMaxAttempts <= 0)
        {
            sendErrorPage(server, "❌ Błąd walidacji", "Maksymalna liczba prób AP musi być > 0", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }
        if (config.routerOffTime <= 0)
        {
            sendErrorPage(server, "❌ Błąd walidacji", "Czas wyłączenia routera musi być > 0", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }
        if (config.baseBootTime <= 0)
        {
            sendErrorPage(server, "❌ Błąd walidacji", "Czas rozruchu routera musi być > 0", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }
        if (config.noWiFiTimeout <= 0)
        {
            sendErrorPage(server, "❌ Błąd walidacji", "Timeout WiFi musi być > 0", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }
        if (config.intermittentMode)
        {
            if (config.awakeWindowMs <= 0)
            {
                sendErrorPage(server, "❌ Błąd walidacji", "Czas pracy w trybie przerywanym musi być > 0", "/config", "Powrót do konfiguracji", config.darkMode);
                return;
            }
            if (config.sleepWindowMs <= 0)
            {
                sendErrorPage(server, "❌ Błąd walidacji", "Czas uśpienia w trybie przerywanym musi być > 0", "/config", "Powrót do konfiguracji", config.darkMode);
                return;
            }
            // Walidacja limitów czasów uśpienia
            if (config.sleepWindowMs < SLEEP_TIME_MIN_MS)
            {
                sendErrorPage(server, "❌ Błąd walidacji", "Czas uśpienia musi być co najmniej 5 minut (300s)", "/config", "Powrót do konfiguracji", config.darkMode);
                return;
            }
            if (config.sleepWindowMs > SLEEP_TIME_MAX_MS)
            {
                sendErrorPage(server, "❌ Błąd walidacji", "Czas uśpienia nie może przekraczać 60 minut (3600s)", "/config", "Powrót do konfiguracji", config.darkMode);
                return;
            }
        }
        // Prosta walidacja IP (podstawowa)
        if (!isValidIP(config.host1))
        {
            sendErrorPage(server, "❌ Błąd walidacji", "Host1 nie jest prawidłowym adresem IP", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }
        if (!isValidIP(config.host2))
        {
            sendErrorPage(server, "❌ Błąd walidacji", "Host2 nie jest prawidłowym adresem IP", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }
        if (config.useGatewayOverride)
        {
            if (config.gatewayOverride.length() == 0)
            {
                sendErrorPage(server, "❌ Błąd walidacji", "Włączono własną bramę, ale pole bramy jest puste", "/config", "Powrót do konfiguracji", config.darkMode);
                return;
            }
            if (!isValidIP(config.gatewayOverride))
            {
                sendErrorPage(server, "❌ Błąd walidacji", "Adres bramy nie jest prawidłowym adresem IP", "/config", "Powrót do konfiguracji", config.darkMode);
                return;
            }
        }
        // Walidacja haseł administratora
        if (config.adminUser.length() == 0)
        {
            sendErrorPage(server, "❌ Błąd walidacji", "Login administratora nie może być pusty", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }
        if (config.adminPass.length() == 0)
        {
            sendErrorPage(server, "❌ Błąd walidacji", "Hasło administratora nie może być puste", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }

        // Walidacja wykluczających się ustawień
        if (config.providerFailureLimit < config.failLimit)
        {
            sendErrorPage(server, "❌ Błąd walidacji", "Limit resetów dla dostawcy musi być >= limit błędów", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }
        if (config.maxTotalResetsEver <= config.providerFailureLimit)
        {
            sendErrorPage(server, "❌ Błąd walidacji", "Maksymalna liczba resetów ogółem musi być > limit resetów dla dostawcy", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }
        if (config.host1 == config.host2)
        {
            sendErrorPage(server, "❌ Błąd walidacji", "Host1 i Host2 nie mogą być takie same", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }

        if (!saveConfig())
        {
            sendErrorPage(server, "❌ Błąd zapisu", "Błąd zapisu konfiguracji! Sprawdź miejsce w pamięci.", "/config", "Powrót do konfiguracji", config.darkMode);
            return;
        }
        // Konfiguracja zapisana - przekieruj z komunikatem
        redirectTo(server, "/config");
    }
    else
    {
        server.send(405, "text/plain", "Method Not Allowed");
    }
}

void handleAddWiFi()
{
    if (!checkAuth())
    {
        return;
    }
    if (server.method() == HTTP_POST)
    {
        String ssid = server.arg("ssid");
        String pass = server.arg("pass");
        String networkTypeStr = server.arg("networkType"); // 0 = primary, 1 = backup

        uaktualnijTablicePlik(ssid, pass);

        // Ustawienie typu sieci dla ostatnio dodanej
        if (tablica[0].ssid == ssid)
        {
            tablica[0].networkType = networkTypeStr.toInt();
            zapiszTabliceDoPliku(WIFI_CONFIG_FILES, tablica);
        }

        String successMsg = "Sieć " + ssid + " (" + (networkTypeStr.toInt() == 1 ? "rezerwowa" : "główna") + ") została zapisana.";
        sendSuccessPage(server, "✅ Sieć dodana", successMsg.c_str(), "/config", "Powrót do konfiguracji", config.darkMode);
    }
    else
    {
        server.send(405, "text/plain", "Method Not Allowed");
    }
}

void handleRemoveWiFi()
{
    if (!checkAuth())
    {
        return;
    }
    if (server.method() == HTTP_POST)
    {
        int index = server.arg("index").toInt();
        if (index >= 0 && index < wielkoscTablicy)
        {
            tablica[index].ssid = "";
            tablica[index].pass = "";
            zapiszTabliceDoPliku(WIFI_CONFIG_FILES, tablica);
        }
        sendSuccessPage(server, "✅ Sieć usunięta", "Sieć została usunięta z listy.", "/config", "Powrót", config.darkMode);
    }
    else
    {
        server.send(405, "text/plain", "Method Not Allowed");
    }
}

void handleUpdatePage()
{
    if (!checkAuth())
    {
        return;
    }
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html; charset=utf-8", "");

    String html;
    html.reserve(1200);
    html = F("<!DOCTYPE html><html lang='pl'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Aktualizacja OTA</title>");
    html += F("<link rel='icon' href='data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI+PHBhdGggZmlsbD0iIzAwN2JmZiIgZD0iTTEyIDFMMyA1djZjMCA1LjU1IDMuODQgMTAuNzQgOSAxMiA1LjE2LTEuMjYgOS02LjQ1IDktMTJWNWwtOS00eiIvPjwvc3ZnPg=='>");
    html += F("<style>");
    server.sendContent(html);
    server.sendContent_P(HTML_THEME_STYLES);

    html = F(".container{max-width:640px;margin:auto;background:var(--card);padding:22px;border-radius:10px;box-shadow:0 10px 30px rgba(0,0,0,0.08);}h1{text-align:center;margin-bottom:10px;}p.subtitle{color:#666;margin-top:0;margin-bottom:16px;text-align:center;} .info{background:var(--inp);border:1px solid var(--brd);padding:10px;border-radius:6px;margin-bottom:12px;font-size:0.95em;}");
    html += F(".dropzone{margin-top:10px;border:2px dashed #007bff;border-radius:10px;padding:28px;text-align:center;background:rgba(0,123,255,0.05);color:var(--fg);transition:all 0.2s ease;} .dropzone.active{background:rgba(0,123,255,0.12);border-color:#0056b3;} .file-name{margin-top:10px;color:#666;font-size:0.9em;}");
    html += F(".btn-row{display:flex;gap:10px;flex-wrap:wrap;justify-content:center;margin-top:12px;}button{padding:10px 14px;background-color:var(--btn);color:white;border:none;border-radius:6px;cursor:pointer;}button:hover{background-color:var(--btn-hover);} .ghost{background:#6c757d;} .ghost:hover{background:#5a6268;}");
    html += F("#progress-wrap{width:100%;background-color:var(--inp);border:1px solid var(--brd);margin-top:20px;display:none;border-radius:6px;overflow:hidden;}#progress-bar{width:0%;height:18px;background-color:#28a745;text-align:center;line-height:18px;color:white;transition:width 0.2s ease;}");
    html += F(".muted{color:#777;font-size:0.9em;} .version{font-size:0.95em;color:#555;margin-bottom:8px;text-align:center;}");
    html += F("</style>");
    server.sendContent(html);

    server.sendContent(getThemeScript(config.darkMode));
    server.sendContent(F("</head><body>"));

    html = F("<div class='container'>");
    html += F("<h1>Aktualizacja OTA</h1>");
    html += F("<p class='version'>Aktualna wersja: <b>");
    html += APP_VERSION;
    html += F("</b></p>");
    html += F("<p class='subtitle'>Przeciągnij i upuść plik firmware (.bin) lub wybierz go ręcznie.</p>");
    html += F("<div class='info'>Podczas aktualizacji nie odłączaj zasilania. Po zakończeniu urządzenie zrestartuje się automatycznie.</div>");
    html += F("<div id='dropzone' class='dropzone' ondrop='handleDrop(event)' ondragover='handleDrag(event)' ondragleave='handleLeave(event)' onclick=\"document.getElementById('file').click();\">");
    html += F("<p style='margin:0 0 8px 0;'><b>📁 Przeciągnij i upuść</b> plik .bin tutaj</p>");
    html += F("<p class='muted' style='margin:0 0 8px 0; font-size:0.95em;'>lub <b style='color:#007bff; text-decoration:underline; cursor:pointer;'>kliknij tutaj</b>, aby wybrać plik</p>");
    html += F("<p id='fileName' class='file-name muted'>Nie wybrano pliku</p>");
    html += F("<input type='file' id='file' name='update' accept='.bin' style='display:none'>");
    html += F("</div>");
    html += F("<div class='btn-row'><button onclick='upload()'>Wgraj aktualizację</button><a href='/config'><button class='ghost'>Powrót</button></a></div>");
    html += F("<div id='progress-wrap'><div id='progress-bar'>0%</div></div>");
    html += F("</div>");

    html += F("<script>");
    html += F("let selectedFile = null;\n");
    html += F("const drop = document.getElementById('dropzone');\nconst fileInput = document.getElementById('file');\nconst fileNameEl = document.getElementById('fileName');\n");
    html += F("fileInput.addEventListener('change', ()=>{ if(fileInput.files.length){ setFile(fileInput.files[0]); } });\n");
    html += F("function setFile(f){ selectedFile = f; fileNameEl.textContent = 'Wybrano: ' + f.name; drop.classList.add('active'); }\n");
    html += F("function handleDrag(e){ e.preventDefault(); e.stopPropagation(); drop.classList.add('active'); }\n");
    html += F("function handleLeave(e){ e.preventDefault(); e.stopPropagation(); drop.classList.remove('active'); }\n");
    html += F("function handleDrop(e){ e.preventDefault(); e.stopPropagation(); drop.classList.add('active'); if(e.dataTransfer.files.length){ setFile(e.dataTransfer.files[0]); } }\n");
    html += F("function upload(){ if(!selectedFile){ alert('Wybierz plik .bin'); return; } if(!selectedFile.name.endsWith('.bin')){ alert('Błąd: Wymagany jest plik .bin'); return; } const formData = new FormData(); formData.append('update', selectedFile); const xhr = new XMLHttpRequest(); document.getElementById('progress-wrap').style.display='block'; xhr.upload.addEventListener('progress', function(e){ if(e.lengthComputable){ const percent = Math.round((e.loaded/e.total)*100); const bar = document.getElementById('progress-bar'); bar.style.width = percent + '%'; bar.innerText = percent + '%'; }}); xhr.onload = function(){ if(xhr.status === 200){ document.getElementById('progress-bar').innerText = 'Sukces! Restart...'; setTimeout(()=>{ window.location.href='/'; }, 15000); } else { alert('Błąd aktualizacji!'); } }; xhr.open('POST','/update'); xhr.send(formData); }\n");
    html += F("</script>");
    html += F("</body></html>");

    server.sendContent(html);
    server.sendContent(""); // zakończ transmisję
}

void handleUpdateResult()
{
    if (!checkAuth())
    {
        return;
    }
    server.sendHeader("Connection", "close");
    if (Update.hasError())
    {
        server.send(500, "text/plain", "Update Failed");
        logEvent("OTA: BLAD - aktualizacja zawiera bledy");
    }
    else
    {
        server.send(200, "text/html; charset=utf-8", "<META http-equiv='refresh' content='15;URL=/'><h1>Update Success!</h1><p>Rebooting...</p>");
    }

    // Upewnij się, że system plików zapisał wszystkie dane przed restartem
    // (flush + unmount, kolejny start sam zamontuje FS)
    LittleFS.end();
    delay(100);

    ESP.restart();
}

void handleUpdateUpload()
{
    // Upload handler woła się wielokrotnie (chunkami), więc wyciszamy logi auth
    if (!checkAuth(true))
    {
        return;
    }
    HTTPUpload &upload = server.upload();
    static bool otaReject = false; // odrzuć kolejne chunki po błędzie
    static size_t otaBytes = 0;    // zlicz zapisane bajty
    static String otaName;         // nazwa pliku do walidacji rozszerzenia
    if (upload.status == UPLOAD_FILE_START)
    {
        Serial.setDebugOutput(true);
        otaReject = false;
        otaBytes = 0;
        otaName = upload.filename;

        // Prosta walidacja: wymagane rozszerzenie .bin
        if (!otaName.endsWith(".bin"))
        {
            otaReject = true;
            Serial.println("[OTA] Invalid file extension, expected .bin");
            logEvent("OTA: BLAD - zly typ pliku (" + otaName + "), wymagany .bin");
            server.send(400, "text/plain", "Zły typ pliku (wymagany .bin)");
            return;
        }

        Serial.printf("Update: %s\n", otaName.c_str());
        logEvent("OTA: Rozpoczeto aktualizacje firmware: " + otaName);

        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace))
        {
            Update.printError(Serial);
            logEvent("OTA: BLAD - nie mozna rozpoczac aktualizacji");
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (otaReject)
        {
            return; // ignoruj dalsze chunki po błędzie
        }

        otaBytes += upload.currentSize;

        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
        {
            Update.printError(Serial);
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (otaReject)
        {
            Serial.println("[OTA] Upload aborted earlier");
            Serial.setDebugOutput(false);
            return;
        }

        // Prosta walidacja rozmiaru > 0
        if (otaBytes == 0)
        {
            Serial.println("[OTA] Empty file uploaded");
            logEvent("OTA: BLAD - plik pusty (0 bajtow)");
            server.send(400, "text/plain", "Plik pusty / rozmiar 0");
            Update.end(false); // Anuluj aktualizację (ESP8266 nie ma abort())
            Serial.setDebugOutput(false);
            return;
        }

        if (Update.end(true))
        {
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            logEvent("OTA: Aktualizacja zakonczona sukcesem (" + String(upload.totalSize) + " B). Restart...");
        }
        else
        {
            Update.printError(Serial);
            logEvent("OTA: BLAD - aktualizacja nie powiodla sie");
        }
        Serial.setDebugOutput(false);
    }
    yield();
}

void handleManualConfig()
{
    if (!checkAuth())
    {
        return;
    }
    WiFi.mode(WIFI_AP);
    uruchomAP();
    uruchommDNS();
    statusMsg = "Tryb konfiguracyjny - ręczny";
    lastAPCheckTime = millis(); // Reset licznika, aby dać czas na konfigurację (5 min)
    server.send(200, "text/html; charset=utf-8", "<h1>Tryb konfiguracyjny uruchomiony!</h1><p>Połącz się z siecią ESP8266_Config.</p><a href='/config'>Konfiguracja</a>");
}

void handleFactoryReset()
{
    if (!checkAuth())
    {
        return;
    }

    // Usuwanie plików konfiguracyjnych
    if (LittleFS.exists(CONFIG_FILE))
        LittleFS.remove(CONFIG_FILE);
    if (LittleFS.exists(LOG_FILE))
        LittleFS.remove(LOG_FILE);
    if (LittleFS.exists(WIFI_CONFIG_FILES))
        LittleFS.remove(WIFI_CONFIG_FILES);

    // Wyświetl stronę z odliczaniem (20 sekund na restart i konfigurację AP)
    sendCountdownPage(server, "🏭 Przywracanie ustawień fabrycznych",
                      "Konfiguracja została usunięta. Urządzenie uruchomi się w trybie AP. Połącz się z siecią ESP8266_Config.",
                      20, "/", config.darkMode);
    delay(500);
    ESP.restart();
}

void handleReboot()
{
    if (!checkAuth())
    {
        return;
    }
    // Wyświetl stronę z odliczaniem (15 sekund na restart ESP)
    sendCountdownPage(server, "🔄 Restartowanie urządzenia",
                      "Urządzenie uruchamia się ponownie. Za chwilę nastąpi automatyczne przekierowanie...",
                      15, "/", config.darkMode);
    delay(500);
    ESP.restart();
}

void handleNotFound()
{
    server.send(404, "text/plain", "Not Found");
}

void handleSetBrightness()
{
    if (!checkAuth())
    {
        return;
    }

    if (!server.hasArg("val"))
    {
        server.send(400, "text/plain", "Missing val parameter");
        return;
    }

    int brightness = constrain(server.arg("val").toInt(), 0, 255);
    config.ledBrightness = brightness;
    Serial.print("[WEBSERVER] handleSetBrightness: brightness=");
    Serial.println(brightness);

    // Odśwież LEDy z nową jasnością (zachowując aktualny stan kolorów)
    refreshLed();

    // Nie zapisujemy od razu do Flash - zrobi to debounced handler po 2 sekundach
    server.send(200, "text/plain", "OK");
}

void handleSaveBrightness()
{
    if (!checkAuth())
    {
        return;
    }

    // Endpoint wywoływany przez debounced JavaScript po zakończeniu zmian
    Serial.print("[WEBSERVER] handleSaveBrightness: saving brightness=");
    Serial.println(config.ledBrightness);
    if (saveConfig())
    {
        server.send(200, "text/plain", "Saved");
    }
    else
    {
        server.send(500, "text/plain", "Error");
    }
}