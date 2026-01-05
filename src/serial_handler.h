#ifndef SERIAL_HANDLER_H
#define SERIAL_HANDLER_H

#include <Arduino.h>

// ============================================================================
// KONFIGURACJA OBSŁUGI POLECEŃ Z SERIAL MONITOR
// ============================================================================

/** @brief Włącz/wyłącz obsługę Serial Monitor (1=włączona, 0=wyłączona) */
#define SERIAL_HANDLER_ENABLED 1

/** @brief Dostęp tylko w trybie AP (1=tak, 0=dostęp zawsze) */
#define SERIAL_HANDLER_ONLY_IN_AP 1

/** @brief PIN do uwierzytelnienia poleceń (zmień na inny dla bezpieczeństwa) */
#define SERIAL_HANDLER_AUTH_PIN "280603"

/** @brief Timeout sesji autoryzacji w minutach (session timeout) */
#define SERIAL_HANDLER_SESSION_TIMEOUT 5

// ============================================================================
// DEKLARACJE FUNKCJI
// ============================================================================

/**
 * @brief Wyświetla pomoc dla dostępnych poleceń Serial Monitor
 */
void printSerialHelp();

/**
 * @brief Wyświetla listę zapisanych sieci WiFi
 */
void listWiFiNetworks();

/**
 * @brief Dodaje sieć WiFi z polecenia Serial
 * @param command Polecenie w formacie: wifi:SSID,HASŁO,TYP
 */
void addWiFiViaSerial(String command);

/**
 * @brief Obsługuje polecenia otrzymane z Serial Monitor
 * Sprawdza dostępne dane i przetwarza komendy:
 * - auth:PIN - Uwierzytelnianie (wymagane przed innymi komendami)
 * - wifi:SSID,HASŁO,TYP - Dodaj sieć WiFi (wymaga autoryzacji)
 * - list - Wyświetl listę sieci (wymaga autoryzacji)
 * - help/? - Wyświetl pomoc
 * - status - Wyświetl status urządzenia
 */
void handleSerialCommands();

#endif // SERIAL_HANDLER_H
