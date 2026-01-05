#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <time.h>
#include <Updater.h>
#include <elapsedMillis.h>
#include <user_interface.h>
#include "WiFiConfig.h" // Własna biblioteka WiFiConfig
#include "config.h"
#include "diag.h"
#include "constants.h"
#include "webserver.h"
#include "watchdog.h"
#include "serial_handler.h" // Obsługa poleceń Serial Monitor

// Ustawienie nazwy sieciowej urządzenia (adres: http://straznik.local)
const char *NAZWA_ESP = "straznik";

// UWAGA: Definicje Config config i CONFIG_FILE znajdują się teraz w config.cpp

// --- ZMIENNE GLOBALNE ---
// Zmienne NOINIT przetrwają ESP.restart(), ale zresetują się po odłączeniu zasilania.
uint32_t noInitMagic __attribute__((section(".noinit")));
int totalResets __attribute__((section(".noinit")));
int totalResetsEver __attribute__((section(".noinit")));
// Boot Loop Detection - NOINIT zmienne do śledzenia resetów ESP w krótkim czasie
uint32_t bootLoopDetectionMagic __attribute__((section(".noinit")));
unsigned long bootLoopResetTimestamps[5] __attribute__((section(".noinit"))); // Ostatnie 5 timestampów resetów
int bootLoopResetCount __attribute__((section(".noinit")));                   // Liczba resetów w oknie

int failCount = 0;
unsigned long lastPingTime = 0;
unsigned long lastAPCheckTime = 0;              // Dla ponownego sprawdzania w AP
unsigned long apModeStartTime = 0;              // Czas wejścia w AP mode
int apModeAttempts = 0;                         // Licznik prób wyjścia z AP
unsigned long lastResetTime = 0;                // Czas ostatniego resetu
unsigned long nextResetDelay = FIVE_MINUTES_MS; // Opóźnienie przed kolejnym resetem (5 min)
unsigned long firstResetTime = 0;               // Czas pierwszego resetu
unsigned long noWiFiStartTime = 0;              // Czas rozpoczęcia braku WiFi
int lastPingMs = 0;
String statusMsg = "Oczekiwanie...";
bool simPingFail = false;
bool simNoWiFi = false;
bool simHighPing = false;
int simResetCount = 0;                       // Licznik resetów podczas symulacji
String simStatus = "";                       // Szczegółowy status symulacji
bool providerFailureNotified = false;        // Dodano brakującą zmienną używaną w watchdog.cpp
bool routerResetInProgress = false;          // Flaga blokująca watchdog podczas resetu routera
int lagCount = 0;                            // Licznik wysokich pingów z rzędu (Lag Detection)
int knownNetworksCount = 0;                  // Liczba zapisanych sieci Wi-Fi (pierwsze uruchomienie)
unsigned long apModeBackoffUntil = 0;        // Czas do końca backoff po porażce AP
bool lastGatewayFailReset = false;           // Czy ostatni reset był z powodu braku gateway
unsigned long ntpLastSyncTime = 0;           // Czas ostatniej synchronizacji z NTP
unsigned long routerBootStartTime = 0;       // Czas gdy router się ostatnio włączył (dla grace period)
unsigned long backupRouterBootStartTime = 0; // Czas gdy router backup się włączył (dla grace period backup)
bool ntpSyncLost = false;                    // Czy straciśmy synchronizację z NTP
time_t estimatedOfflineTime = 0;             // Szacunkowy czas offline (do offline timer)

unsigned long lastSessionActivity = 0;
bool isSessionActive = false;
bool safeMode = false; // Tryb bezpieczeństwa - boot loop protection

// Forward declaration - logEvent used by recordResetDiagnostics
const char *LOG_FILE = "/events.log";
void logEvent(String msg);

unsigned long wakeCycleStartMs = 0; // Początek bieżącego cyklu aktywności w trybie przerywanym

static const char *resetReasonName(uint8_t reason)
{
  switch (reason)
  {
  case REASON_DEFAULT_RST:
    return "Power on";
  case REASON_WDT_RST:
    return "Hardware WDT";
  case REASON_EXCEPTION_RST:
    return "Exception";
  case REASON_SOFT_WDT_RST:
    return "Software WDT";
  case REASON_SOFT_RESTART:
    return "Soft restart";
  case REASON_DEEP_SLEEP_AWAKE:
    return "Deep-sleep wake";
  case REASON_EXT_SYS_RST:
    return "External reset";
  default:
    return "Unknown";
  }
}

static String formatEpochTime(time_t epoch)
{
  if (epoch <= 0)
    return "brak NTP";

  struct tm *tm = localtime(&epoch);
  char buf[32];
  if (tm)
  {
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    return String(buf);
  }

  return "brak NTP";
}

static time_t estimateNowFromLastSync()
{
  if (config.lastNtpSync > 0 && config.ntpSyncMillis > 0)
  {
    unsigned long elapsedMs = millis() - config.ntpSyncMillis;
    return config.lastNtpSync + (elapsedMs / 1000);
  }
  return 0;
}

static void recordResetDiagnostics()
{
  rst_info *info = ESP.getResetInfoPtr();
  uint8_t reason = info ? info->reason : REASON_DEFAULT_RST;
  const char *reasonName = resetReasonName(reason);

  // Zwiększ licznik dla danej przyczyny
  switch (reason)
  {
  case REASON_DEFAULT_RST:
    config.resetDefault++;
    break;
  case REASON_WDT_RST:
    config.resetWdt++;
    break;
  case REASON_EXCEPTION_RST:
    config.resetException++;
    break;
  case REASON_SOFT_WDT_RST:
    config.resetSoftWdt++;
    break;
  case REASON_SOFT_RESTART:
    config.resetSoft++;
    break;
  case REASON_DEEP_SLEEP_AWAKE:
    config.resetDeepSleep++;
    break;
  case REASON_EXT_SYS_RST:
    config.resetExt++;
    break;
  default:
    break;
  }

  String msg = "Restart: ";
  msg += reasonName;
  logEvent(msg);

  // Zapisz zaktualizowane liczniki do flash (sporadyczne — na każdym starcie)
  saveConfig();
}

// Cookie-based authentication
String sessionToken = "";

// Funkcja generująca losowy token sesji
String generateToken()
{
  String token = "";
  const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  for (int i = 0; i < 32; i++)
  {
    token += alphanum[random(62)];
  }
  return token;
}

// Funkcje saveConfig i loadConfig zostały przeniesione do config.cpp

// --- ZMIENNE ---

int ledBrightness() { return constrain(config.ledBrightness, 0, 255); }

// Śledź aktualny stan LED
static bool currentRed = false;
static bool currentGreen = false;
static bool currentBlue = false;

void setLed(bool rOn, bool gOn, bool bOn)
{
  currentRed = rOn;
  currentGreen = gOn;
  currentBlue = bOn;
  // Przeliczenie jasności (ESP8266 PWM 0-1023)
  int pwmOn = map(ledBrightness(), 0, 255, 0, 1023);
  analogWrite(config.pinRed, rOn ? pwmOn : 0);
  analogWrite(config.pinGreen, gOn ? pwmOn : 0);
  analogWrite(config.pinBlue, bOn ? pwmOn : 0);
}

void refreshLed()
{
  // Odśwież LEDy z aktualnym stanem
  setLed(currentRed, currentGreen, currentBlue);
}

void ledOK() { setLed(false, true, false); }
void ledFail() { setLed(true, false, false); }
void ledBlue() { setLed(false, false, true); }
void toggleBlue()
{
  static bool state = false;
  state = !state;
  setLed(false, false, state);
}

// logEvent moved to top as forward declaration
// (actual implementation stays here for completeness)

void logEvent(String msg)
{
  File rFile = LittleFS.open(LOG_FILE, "r");
  String content = "";
  if (rFile)
  {
    content = rFile.readString();
    rFile.close();
  }

  String timestamp;
  time_t now = time(nullptr);
  if (now > 1600000000)
  { // Jeśli czas jest poprawny (np. > rok 2020)
    struct tm *timeinfo = localtime(&now);
    char timeStr[16];
    sprintf(timeStr, "[%02d:%02d:%02d] ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    timestamp = String(timeStr);
  }
  else
  {
    timestamp = "[" + String(millis() / 1000) + "s] ";
  }

  content += timestamp + msg + "\n";

  int lineCount = 0;
  for (unsigned int i = 0; i < content.length(); i++)
  {
    if (content[i] == '\n')
      lineCount++;
  }

  // Przechowuj maksymalnie 50 ostatnich zdarzeń
  while (lineCount > 50)
  {
    int firstNewLine = content.indexOf('\n');
    content = content.substring(firstNewLine + 1);
    lineCount--;
  }

  File wFile = LittleFS.open(LOG_FILE, "w");
  if (wFile)
  {
    wFile.print(content);
    wFile.close();
  }
}

bool checkAuth(bool quiet)
{
  // === DEBUG: Przełącznik wyłączający autoryzację ===
  if (DEBUG_SKIP_AUTH)
  {
    if (!quiet)
    {
      DIAG_PRINTLN("[AUTH] ⚠️ DEBUG MODE - Autoryzacja wyłączona (DEBUG_SKIP_AUTH=true)");
    }
    isSessionActive = true; // Symuluj aktywną sesję
    lastSessionActivity = millis();
    return true; // Zawsze zwróć true - pomiń logowanie
  }

  if (!quiet)
  {
    DIAG_PRINTLN("\n---------- checkAuth() START ----------");
    DIAG_PRINT("[AUTH] isSessionActive: ");
    DIAG_PRINTLN(isSessionActive ? "TRUE" : "FALSE");
  }

  // 1. Sprawdź czy przeglądarka wysłała ciasteczko
  if (server.hasHeader("Cookie"))
  {
    String cookie = server.header("Cookie");
    if (!quiet)
    {
      DIAG_PRINT("[AUTH] Cookie header: ");
      DIAG_PRINTLN(cookie);
    }

    // Sprawdź czy zawiera nasz token sesji
    String expectedCookie = String(COOKIE_NAME) + "=" + sessionToken;
    if (sessionToken.length() > 0 && cookie.indexOf(expectedCookie) != -1)
    {
      if (!quiet)
      {
        DIAG_PRINTLN("[AUTH] ✓ Valid session cookie found");
      }

      // Sprawdź timeout sesji
      unsigned long timeSinceActivity = millis() - lastSessionActivity;
      if (timeSinceActivity > SESSION_TIMEOUT_MS)
      {
        if (!quiet)
        {
          DIAG_PRINT("[AUTH] ✗ Session EXPIRED (");
          DIAG_PRINT(timeSinceActivity);
          DIAG_PRINTLN(" ms > timeout)");
        }
        isSessionActive = false;
        sessionToken = "";
        server.sendHeader("Location", "/login");
        server.sendHeader("Cache-Control", "no-cache");
        server.send(303);
        if (!quiet)
        {
          DIAG_PRINTLN("---------- checkAuth() END (expired) ----------\n");
        }
        return false;
      }

      // Sesja ważna - odśwież czas
      isSessionActive = true;
      lastSessionActivity = millis();
      if (!quiet)
      {
        DIAG_PRINTLN("[AUTH] Session refreshed");
        DIAG_PRINTLN("---------- checkAuth() END (valid) ----------\n");
      }
      return true;
    }
    else
    {
      if (!quiet)
      {
        DIAG_PRINTLN("[AUTH] ✗ Invalid or missing session token in cookie");
      }
    }
  }
  else
  {
    if (!quiet)
    {
      DIAG_PRINTLN("[AUTH] No Cookie header found");
    }
  }

  // 2. Brak ciasteczka lub niepoprawne - przekieruj na /login
  if (!quiet)
  {
    DIAG_PRINTLN("[AUTH] Redirecting to /login");
  }
  isSessionActive = false;
  server.sendHeader("Location", "/login");
  server.sendHeader("Cache-Control", "no-cache");
  server.send(303);
  if (!quiet)
  {
    DIAG_PRINTLN("---------- checkAuth() END (redirect) ----------\n");
  }
  return false;
}

// Wszystkie handlery (handleRoot, handleConfig, itp.) oraz logika Watchdoga
// zostały przeniesione do webserver.cpp i watchdog.cpp

static void handleSleepScheduler()
{
  if (!config.intermittentMode)
  {
    wakeCycleStartMs = millis();
    return;
  }

  // === ZABEZPIECZENIA - NIE IDŹ SPAĆ JEŚLI: ===

  // 1. Tryb AP (konfiguracja)
  if (WiFi.getMode() == WIFI_AP)
  {
    wakeCycleStartMs = millis();
    return;
  }

  // 2. Safe Mode aktywny (diagnostyka)
  if (safeMode)
  {
    wakeCycleStartMs = millis();
    Serial.println("[SLEEP] Blocked by Safe Mode - diagnostics needed");
    return;
  }

  // 3. Problem z internetem - watchdog musi działać!
  if (failCount > 0)
  {
    wakeCycleStartMs = millis();
    Serial.println("[SLEEP] Blocked - failCount > 0, watchdog must work");
    return;
  }

  // 4. WiFi nie połączone - nie śpij podczas próby reconnect
  if (WiFi.status() != WL_CONNECTED)
  {
    wakeCycleStartMs = millis();
    Serial.println("[SLEEP] Blocked - WiFi not connected");
    return;
  }

  // 5. Minimum 2 cykle ping przed snem (upewnij się że internet działa)
  if (millis() - lastPingTime < config.pingInterval * 2)
  {
    // Za wcześnie - poczekaj na potwierdzenie stabilności internetu
    return;
  }

  // 6. Sprawdź czy scheduled reset nie przypadnie podczas snu
  if (config.scheduledResetsEnabled)
  {
    time_t now = time(nullptr);
    if (now > 1000000000) // Mamy NTP
    {
      struct tm *timeinfo = localtime(&now);
      int currentHour = timeinfo->tm_hour;
      int currentMinute = timeinfo->tm_min;

      // Sprawdź czy któraś z zaplanowanych czasów (HH:MM) jest w oknie snu
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

        // Oblicz ile minut do najbliższego resetu
        int minutesToReset = (scheduleHour - currentHour) * 60 + (scheduleMin - currentMinute);
        if (minutesToReset < 0)
          minutesToReset += 24 * 60; // Następny dzień

        unsigned long msToReset = (unsigned long)minutesToReset * 60000UL;

        // Jeśli reset przypadnie podczas snu, nie śpij
        if (msToReset < config.sleepWindowMs)
        {
          wakeCycleStartMs = millis();
          Serial.println("[SLEEP] Blocked - scheduled reset in " + String(minutesToReset) + " min");
          return;
        }
      }
    }
  }

  // === OK - MOŻNA IŚĆ SPAĆ ===
  unsigned long elapsed = millis() - wakeCycleStartMs;
  if (elapsed >= config.awakeWindowMs)
  {
    logEvent("Tryb przerywany: usypiam na " + String(config.sleepWindowMs / 1000) + "s (internet OK, watchdog zawieszony)");
    Serial.println("[SLEEP] Going to deep sleep for " + String(config.sleepWindowMs / 1000) + "s");
    ESP.wdtFeed();
    ESP.deepSleep((uint64_t)config.sleepWindowMs * 1000ULL, WAKE_RF_DEFAULT);
  }
}

#ifndef UNIT_TEST

void setup()
{
  // === KRYTYCZNE: Bezpieczny stan przekaźnika podczas rozruchu ===
  // Piny mogą przez ułamek sekundy być w stanie losowym, co mogłoby włączyć/wyłączyć router
  // Wymuszamy stan BEZPIECZNY (przekaźnik wyłączony) ZANIM cokolwiek innego
  pinMode(D1, OUTPUT);   // Pin przekaźnika - NATYCHMIAST ustawić
  digitalWrite(D1, LOW); // LOW = NC (Normally Closed) = przekaźnik wyłączony = router bezpieczny
  delayMicroseconds(10); // Pewny margines

  Serial.begin(74880); // Zmniejszona prędkość = mniejszy bufor RAM
  delay(500);
  Serial.println("\n\n[SETUP] System startup...");
  Serial.println("[SETUP] Relay pin secured - router safe");
  // Uruchom system plików przed operacjami na plikach konfiguracyjnych
  if (!initLittleFS())
  {
    Serial.println("Blad inicjalizacji LittleFS. Restart ESP...");
    delay(3000);
    ESP.restart();
  }

  // --- WiFiConfig ---
  Serial.println("[SETUP] Loading configuration...");
  loadConfig(); // Odczyt konfiguracji
  time_t approxNow = estimateNowFromLastSync();
  Serial.print("[SETUP] Restart time: ");
  if (approxNow > 0)
    Serial.println(formatEpochTime(approxNow));
  else
  {
    Serial.print("brak NTP (uptime=");
    Serial.print(millis());
    Serial.println("ms)");
  }
  const uint8_t resetReason = system_get_rst_info()->reason;
  Serial.print("[SETUP] Reset reason: ");
  Serial.println(resetReasonName(resetReason));
  Serial.print("[SETUP] Config loaded - ledBrightness=");
  Serial.println(config.ledBrightness);
  // Ustawienie zmiennych globalnych z config

  recordResetDiagnostics();

  // === BOOT LOOP DETECTION (Safety Mode) ===
  // Śledzi nieplanowane resety ESP (WDT, Exception) - planowane resety (SOFT_RESTART) są ignorowane
  const int MAX_RESETS_BOOT_LOOP = 5;
  unsigned long BOOT_LOOP_WINDOW_MS = (unsigned long)config.bootLoopWindowSeconds * 1000UL;
  const bool isPlannedRestart = (resetReason == REASON_SOFT_RESTART || resetReason == REASON_DEFAULT_RST || resetReason == REASON_EXT_SYS_RST);

  Serial.print("[BOOT_LOOP] Window: ");
  Serial.print(config.bootLoopWindowSeconds);
  Serial.println("s");

  if (bootLoopDetectionMagic != NOINIT_MAGIC)
  {
    // Zimny start - inicjalizuj boot loop detection
    bootLoopDetectionMagic = NOINIT_MAGIC;
    bootLoopResetCount = 0;
    for (int i = 0; i < 5; i++)
      bootLoopResetTimestamps[i] = 0;
    safeMode = false;
  }
  else
  {
    // Ciepły start - sprawdzaj powód resetu

    if (!isPlannedRestart)
    {
      // To był NIEPLANOWANY reset (WDT, Exception, etc) - LICZ!
      unsigned long currentTime = millis(); // Uwaga: reset do 0 po ESP.restart()

      // Przesunięcie starych timestamp'ów (FIFO ring buffer)
      for (int i = 4; i > 0; i--)
        bootLoopResetTimestamps[i] = bootLoopResetTimestamps[i - 1];
      bootLoopResetTimestamps[0] = currentTime;
      bootLoopResetCount++;

      // Sprawdzenie czy 5 resetów w oknie 5 minut
      if (bootLoopResetCount >= MAX_RESETS_BOOT_LOOP)
      {
        unsigned long oldestReset = bootLoopResetTimestamps[MAX_RESETS_BOOT_LOOP - 1];
        unsigned long timeDiff = currentTime - oldestReset;

        if (timeDiff < BOOT_LOOP_WINDOW_MS)
        {
          // BOOT LOOP DETECTED!
          Serial.println("\n\n!!! BOOT LOOP DETECTED !!! (5+ unplanned resets in 5 minutes) !!!");
          Serial.println("[BOOT_LOOP] Entering SAFE MODE - no internet tests, no router resets, AP only");
          safeMode = true;
          config.safeModeActive = true;

          // Wymusz bezpieczny stan przekaźnika
          digitalWrite(D1, LOW); // Przekaźnik wyłączony (NC) - router bezpieczny

          logEvent("BOOT_LOOP DETECTED - SAFE MODE ACTIVATED (Reason: " + String(resetReason) + ")");
        }
        else
        {
          // Okno się rozeszło, zresetuj licznik
          bootLoopResetCount = 1;
        }
      }

      Serial.print("[BOOT_LOOP] Reset reason: ");
      Serial.println(resetReason);
      Serial.print("[BOOT_LOOP] Reset count in window: ");
      Serial.println(bootLoopResetCount);
    }
    else
    {
      // To był planowany restart (np. z wykonajReset) - NIE liczz
      Serial.println("[BOOT_LOOP] Planned restart detected - not counting towards boot loop");
    }
  }

  if (isPlannedRestart)
  {
    Serial.println("[BOOT_LOOP] Manual/planned restart - clearing failure/backoff counters");
    totalResets = 0;
    nextResetDelay = FIVE_MINUTES_MS;
    firstResetTime = 0;
    lastResetTime = 0;
    failCount = 0;
    apModeAttempts = 0;
    apModeBackoffUntil = 0;

    config.totalResets = 0;
    config.totalResetsEver = totalResetsEver;
    config.failCount = 0;
    config.nextResetDelay = FIVE_MINUTES_MS;
    config.firstResetTime = 0;
    config.lastResetTime = 0;
    config.accumulatedFailureTime = 0;
    config.noWiFiStartTime = 0;
    saveConfig();
  }

  // Obsługa pamięci NOINIT (zachowanie liczników po restarcie bez zapisu do Flash)
  if (noInitMagic != NOINIT_MAGIC)
  {
    // To jest "zimny start" (po braku zasilania) - inicjalizujemy zmienne
    totalResets = config.totalResets; // Wczytaj ostatnie znane z pliku (zazwyczaj 0)
    totalResetsEver = config.totalResetsEver;
    noInitMagic = NOINIT_MAGIC; // Ustaw znacznik
  }
  else
  {
    // To jest "ciepły start" (po ESP.restart()) - zachowujemy wartości z RAM
    // Aktualizujemy strukturę config, aby w razie zapisu przez WWW były aktualne
    config.totalResets = totalResets;
    config.totalResetsEver = totalResetsEver;

    // Akumulacja czasu awarii - dodaj czas od ostatniego restartu
    if (config.autoResetCountersHours > 0 && lastResetTime > 0)
    {
      unsigned long timeSinceLastReset = millis(); // millis() zaczyna od 0 po ESP.restart()
      config.accumulatedFailureTime += timeSinceLastReset;
      Serial.print("[SETUP] Akumulacja czasu awarii: +");
      Serial.print(timeSinceLastReset / 1000);
      Serial.print("s, razem=");
      Serial.print(config.accumulatedFailureTime / 1000);
      Serial.println("s");
      saveConfig(); // Zapisz zaktualizowany czas
    }
  }

  nextResetDelay = config.nextResetDelay;
  firstResetTime = config.firstResetTime;
  lastResetTime = config.lastResetTime;
  failCount = config.failCount;
  noWiFiStartTime = config.noWiFiStartTime;

  // --- DODATEK: Oczekiwanie na przerwę bezpieczeństwa (Backoff) przy starcie ---
  // Jeśli mamy zarejestrowane resety, to znaczy że jesteśmy w trakcie cyklu awarii.
  // Jeśli nextResetDelay jest duże (np. 15 min), a my właśnie wstaliśmy,
  // to znaczy że ruter był wyłączony, włączony i teraz musimy przeczekać okno ochronne.

  if (totalResets > 0 && nextResetDelay > FIVE_MINUTES_MS)
  {
    Serial.print("Wykryto cykl awarii. Czekam na koniec okna ochronnego: ");
    Serial.print(nextResetDelay / 1000);
    Serial.println(" sekund.");

    // Miganie podczas okna ochronnego
    elapsedMillis backoffTimer;
    while (backoffTimer < nextResetDelay)
    {
      ESP.wdtFeed();
      delay(100); // Krótki delay w pętli - OK (pętla ma wyjście, karmi WDT)
      if ((millis() / 1000) % 2 == 0)
        ledBlue();
      else
        ledOK();
    }
  }

  pinMode(config.pinRelay, OUTPUT);
  pinMode(config.pinRelayBackup, OUTPUT);
  pinMode(config.pinRed, OUTPUT);
  pinMode(config.pinGreen, OUTPUT);
  pinMode(config.pinBlue, OUTPUT);
  analogWriteRange(1023);                                                   // Ujednolicenie skali PWM
  pinMode(config.pinButton, INPUT_PULLUP);                                  // Przycisk z pull-up
  digitalWrite(config.pinRelay, LOW);                                       // NC
  digitalWrite(config.pinRelayBackup, config.relayActiveHigh ? LOW : HIGH); // Zabezpieczenie: backup wyłączony przy starcie

  // Włączenie Hardware Watchdog (8 sekund)
  ESP.wdtEnable(8000);

  // Konfiguracja NTP z obsługą strefy czasowej i DST dla Polski
  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); // Strefa czasowa: Warszawa
  tzset();
  logEvent("Start systemu");
  odczytajTabliceZPliku(WIFI_CONFIG_FILES);
  knownNetworksCount = liczbaZajetychMiejscTablicy(tablica, wielkoscTablicy);
  PolaczZWiFi(tablica, toggleBlue);

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Nie udalo sie polaczyc z WiFi. Uruchomiono AP.");
    // W WiFiConfig AP jest uruchomiony, więc kontynuujemy
  }
  else
  {
    // PolaczZWiFi już zalogował szczegóły i uruchomił mDNS
    lastPingTime = 0; // Wymuś ping natychmiast aby ustawić prawidłową diodę
  }

  // Konfiguracja zbierania nagłówków Cookie (wymagane dla session cookie)
  const char *headerkeys[] = {"Cookie"};
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
  server.collectHeaders(headerkeys, headerkeyssize);
  DIAG_PRINTLN("[SETUP] Cookie headers collection enabled");

  setupWebServer(); // Konfiguracja serwera WWW z webserver.cpp

  wakeCycleStartMs = millis();
}

void loop()
{
  ESP.wdtFeed();

  // Obsługa poleceń z Serial Monitor
  handleSerialCommands();

  // === SAFE MODE - Boot Loop Protection ===
  if (safeMode)
  {
    // Tryb bezpieczeństwa: ESP jest chronione przed boot loop
    // - Włączamy AP mode (user może konfigurować)
    // - NIE testujemy internetu
    // - NIE resetujemy routera
    // - Przekaźnik zablokowany (router bezpieczny)

    // Ustaw AP mode
    if (WiFi.getMode() != WIFI_AP)
    {
      WiFi.mode(WIFI_AP);
      Serial.println("[SAFE_MODE] Forcing AP mode");
    }

    // Migająca czerwona dioda (alarm)
    if ((millis() / 500) % 2 == 0)
      digitalWrite(config.pinRed, HIGH);
    else
      digitalWrite(config.pinRed, LOW);

    // Obsługa AP i web servera
    updateMDNS();
    server.handleClient();
    handleButtonPress();

    statusMsg = "⚠️ SAFE MODE - Boot loop detected! Configure and reboot.";
    logEvent("SAFE_MODE: Running - no internet tests, router locked");

    delay(100);
    return; // Pomiń resztę loop'a - nie testuj internetu, nie resetuj routera
  }

  // Normalny tryb - testuj internet i resetuj routera
  updateMDNS();
  server.handleClient();
  // Wyłącz symulacje po wygaśnięciu sesji
  if (!isSessionActive && (simPingFail || simNoWiFi || simHighPing))
  {
    simPingFail = false;
    simNoWiFi = false;
    simHighPing = false;
    simResetCount = 0;
    simStatus = "";
    Serial.println("Symulacje wyłączone z powodu wygaśnięcia sesji.");
    logEvent("SYMULACJE WYLACZONE (Wygasniecie sesji)");
  }
  handleButtonPress();

  // Obsługa trybów pracy
  if (WiFi.getMode() == WIFI_AP)
  {
    // Tryb AP (Konfiguracyjny)
    ledBlue(); // Niebieska dioda dla trybu AP
    failCount = 0;
    statusMsg = "Tryb konfiguracyjny - brak internetu";

    // Sprawdź ile sieci jest zapisanych (czy to pierwsze uruchomienie)
    int currentKnown = liczbaZajetychMiejscTablicy(tablica, wielkoscTablicy);
    knownNetworksCount = currentKnown;

    // Pierwsze uruchomienie: brak zapisanych sieci -> brak odliczania i brak resetów
    if (currentKnown == 0)
    {
      statusMsg = "Tryb konfiguracyjny (pierwsze uruchomienie) - brak zapisanych sieci";
      apModeAttempts = 0;
      apModeStartTime = 0;

      // Od czasu do czasu wczytaj plik, by wykryć nowe dane dodane przez użytkownika
      if (millis() - lastAPCheckTime > config.apConfigTimeout)
      {
        lastAPCheckTime = millis();
        odczytajTabliceZPliku(WIFI_CONFIG_FILES);
      }
      return; // Zostań w AP bez resetów
    }

    // Śledź czas wejścia w AP mode
    if (apModeStartTime == 0)
    {
      apModeStartTime = millis();
      apModeAttempts = 0;
      logEvent("Wejscie w AP mode - licznik prob: 0");
    }

    // Sprawdzaj co apConfigTimeout czy wyjść z AP
    if (millis() - lastAPCheckTime > config.apConfigTimeout)
    {
      // Sprawdź czy jesteśmy w backoff
      if (apModeBackoffUntil > millis())
      {
        // Jesteśmy w backoff, nie rób nic, czekaj
        statusMsg = "AP backoff: " + String((apModeBackoffUntil - millis()) / 60000) + " min do następnej próby";
        return;
      }

      lastAPCheckTime = millis();
      apModeAttempts++;
      Serial.println("Proba wyjscia z trybu AP (Licznik: " + String(apModeAttempts) + ")");
      logEvent("Proba wyjscia z AP #" + String(apModeAttempts));

      // Detektuj problemy: brak radia czy brak IP
      int wifiStatus = WiFi.status();
      IPAddress localIP = WiFi.localIP();
      String failureReason = "";

      if (wifiStatus != WL_CONNECTED)
      {
        if (wifiStatus == WL_NO_SSID_AVAIL)
          failureReason = "Brak sieci (radio martwe)";
        else if (wifiStatus == WL_CONNECT_FAILED)
          failureReason = "Połączenie odrzucone";
        else
          failureReason = "Brak WiFi (status=" + String(wifiStatus) + ")";
      }
      else if (localIP[0] == 0 && localIP[1] == 0 && localIP[2] == 0 && localIP[3] == 0)
      {
        failureReason = "Brak adresu IP (problem DHCP)";
      }

      if (failureReason.length() > 0)
      {
        logEvent("Proba #" + String(apModeAttempts) + " nieudana: " + failureReason);
      }

      odczytajTabliceZPliku(WIFI_CONFIG_FILES);
      PolaczZWiFi(tablica, toggleBlue);

      // Jeśli po apMaxAttempts próbach, zresetuj router i przejdź w backoff
      // Domyślnie: apMaxAttempts=4, apConfigTimeout=10 min -> 4 próby = 40 minut
      if (apModeAttempts >= config.apMaxAttempts)
      {
        Serial.println("RESET ROUTERA - AP mode nie zakonczyl sie po " + String(config.apMaxAttempts) + " probach");
        logEvent("RESET ROUTERA - AP mode trwa zbyt dlugo (" + String(config.apMaxAttempts) + "+ probe), backoff: " + String(config.apBackoffMs / 60000) + "min");

        apModeBackoffUntil = millis() + config.apBackoffMs; // Ustaw backoff na config.apBackoffMs
        apModeAttempts = 0;                                 // Reset licznika
        lastAPCheckTime = millis();                         // Zapamiętaj czas

        // Importuj zmienne z watchdog.cpp
        extern void wykonajReset();
        wykonajReset();
      }
    }
  }
  else
  {
    // Tryb STA (Normalna praca) - Watchdog zarządza połączeniem (Ping + WiFi Check)
    monitorInternetConnection();
  }

  // Obsługa trybu przerywanego (uśpienia)
  handleSleepScheduler();
}

#endif // UNIT_TEST