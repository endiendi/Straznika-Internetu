#ifndef CONSTANTS_H
#define CONSTANTS_H

// Stałe czasowe (wszystkie w ms)
const unsigned long FIVE_MINUTES_MS = 300000;
const unsigned long SESSION_TIMEOUT_MS = FIVE_MINUTES_MS;
const unsigned long AP_CHECK_INTERVAL_MS = FIVE_MINUTES_MS;
const unsigned long WATCHDOG_TIMEOUT_MS = 8000;

// Stałe konfiguracyjne
const uint32_t NOINIT_MAGIC = 0xDEADBEEF; // Marker dla pamięci NOINIT (przetrwa restart)
const int MAX_LOG_LINES = 10;
const int SESSION_TOKEN_LENGTH = 32;

// Limity PWM
const int PWM_RANGE = 1023; // ESP8266 PWM range
const int BRIGHTNESS_MIN = 0;
const int BRIGHTNESS_MAX = 255;

// Cookie
const char COOKIE_NAME[] = "ESPSESSIONID";

// === Limity dla trybu uśpienia (intermittent mode) ===
const unsigned long SLEEP_TIME_MIN_MS = 5 * 60 * 1000;  // 5 minut (300000 ms)
const unsigned long SLEEP_TIME_MAX_MS = 60 * 60 * 1000; // 60 minut (3600000 ms)

// === Limity dla logiki backoff i resetów ===
const unsigned long BACKOFF_MAX_MS = 60 * 60 * 1000;    // Maksymalnie +60 minut backoff
const unsigned long SIM_NO_WIFI_TIMEOUT_MS = 60 * 1000; // 60 sekund dla symulacji

// === DEBUG - Przełącznik wyłączający autoryzację ===
// Zmień na 'true' aby pominąć logowanie przy testach
const bool DEBUG_SKIP_AUTH = false; // ⚠️ UWAGA: ustaw na 'false' przed wdrożeniem w produkcji!

#endif
