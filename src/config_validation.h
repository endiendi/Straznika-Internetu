#ifndef CONFIG_VALIDATION_H
#define CONFIG_VALIDATION_H

#include "config.h"

// ============================================================================
// STRUKTURA WYNIKU WALIDACJI
// ============================================================================
struct ValidationResult
{
    bool valid;
    String errorMsg;

    ValidationResult() : valid(true), errorMsg("") {}
    ValidationResult(bool v, const String &msg) : valid(v), errorMsg(msg) {}
};

// ============================================================================
// FUNKCJE WALIDACJI POMOCNICZE
// ============================================================================

/// Waliduje czy wartość całkowita jest większa od 0
ValidationResult validatePositiveInt(int value, const String &fieldName)
{
    if (value <= 0)
    {
        return ValidationResult(false, fieldName + " musi być > 0");
    }
    return ValidationResult(true, "");
}

/// Waliduje czy wartość całkowita mieści się w zakresie
ValidationResult validateIntRange(int value, int minVal, int maxVal, const String &fieldName)
{
    if (value < minVal || value > maxVal)
    {
        return ValidationResult(false,
                                fieldName + " musi być pomiędzy " + String(minVal) + " a " + String(maxVal));
    }
    return ValidationResult(true, "");
}

/// Waliduje czy string nie jest pusty
ValidationResult validateNonEmpty(const String &value, const String &fieldName)
{
    if (value.length() == 0)
    {
        return ValidationResult(false, fieldName + " nie może być pusty");
    }
    return ValidationResult(true, "");
}

/// Waliduje adres IP (podstawowa walidacja)
ValidationResult validateIpAddress(const String &ip, const String &fieldName)
{
    if (!isValidIP(ip))
    {
        return ValidationResult(false, fieldName + " nie jest prawidłowym adresem IP");
    }
    return ValidationResult(true, "");
}

/// Waliduje że dwie wartości całkowite są równe lub pierwsza >= drugiej
ValidationResult validateGreaterOrEqual(int value1, int value2, const String &field1, const String &field2)
{
    if (value1 < value2)
    {
        return ValidationResult(false, field1 + " musi być >= " + field2);
    }
    return ValidationResult(true, "");
}

/// Waliduje że dwie wartości całkowite nie są równe
ValidationResult validateNotEqual(int value1, int value2, const String &description)
{
    if (value1 == value2)
    {
        return ValidationResult(false, description);
    }
    return ValidationResult(true, "");
}

/// Waliduje że dwie wartości string'owe nie są równe
ValidationResult validateStringNotEqual(const String &str1, const String &str2, const String &description)
{
    if (str1 == str2)
    {
        return ValidationResult(false, description);
    }
    return ValidationResult(true, "");
}

// ============================================================================
// WALIDACJA KONFIGURACJI - GŁÓWNA FUNKCJA
// ============================================================================

/// Waliduje wszystkie parametry konfiguracji
/// Zwraca pusty String jeśli wszystko OK, lub komunikat błędu
///
/// LOGI DIAGNOSTYCZNE:
/// - Wyświetla wszystkie wartości przed walidacją
/// - Raportuje każdy błąd walidacji
/// - Potwierdza sukces przy zapisie
String validateAllConfigParams(
    int pingInterval, int failLimit, int providerFailureLimit,
    int autoResetCountersHours, int maxPingMs, int lagRetries,
    int bootLoopWindowSeconds, int apMaxAttempts, int routerOffTime,
    int baseBootTime, int noWiFiTimeout, bool intermittentMode,
    int awakeWindowMs, int sleepWindowMs, const String &host1,
    const String &host2, const String &gatewayOverride, bool useGatewayOverride,
    const String &adminUser, const String &adminPass, int maxTotalResetsEver)
{
    // ========== FAZA 1: DIAGNOSTYKA WARTOŚCI ODEBRANYCH ==========
    Serial.println(F("\n╔═══════════════════════════════════════════════════════════╗"));
    Serial.println(F("║  WALIDACJA PARAMETRÓW - LOGI DIAGNOSTYCZNE               ║"));
    Serial.println(F("╠═══════════════════════════════════════════════════════════╣"));

    Serial.println(F("\n[WALIDACJA] 1️⃣ MONITOROWANIE:"));
    Serial.print(F("  • pingInterval: "));
    Serial.print(pingInterval);
    Serial.println(F(" ms"));
    Serial.print(F("  • failLimit: "));
    Serial.println(failLimit);

    Serial.println(F("\n[WALIDACJA] 2️⃣ RESET ROUTERA:"));
    Serial.print(F("  • routerOffTime: "));
    Serial.print(routerOffTime);
    Serial.println(F(" ms"));
    Serial.print(F("  • baseBootTime: "));
    Serial.print(baseBootTime);
    Serial.println(F(" ms"));

    Serial.println(F("\n[WALIDACJA] 3️⃣ BOOT LOOP:"));
    Serial.print(F("  • bootLoopWindowSeconds: "));
    Serial.println(bootLoopWindowSeconds);

    Serial.println(F("\n[WALIDACJA] 4️⃣ WiFi/AP:"));
    Serial.print(F("  • noWiFiTimeout: "));
    Serial.print(noWiFiTimeout);
    Serial.println(F(" ms"));
    Serial.print(F("  • apMaxAttempts: "));
    Serial.println(apMaxAttempts);

    Serial.println(F("\n[WALIDACJA] 5️⃣ DOSTAWCA:"));
    Serial.print(F("  • providerFailureLimit: "));
    Serial.println(providerFailureLimit);
    Serial.print(F("  • noWiFiBackoff: "));
    Serial.println(useGatewayOverride ? "true" : "false");

    Serial.println(F("\n[WALIDACJA] 6️⃣ LAG:"));
    Serial.print(F("  • maxPingMs: "));
    Serial.print(maxPingMs);
    Serial.println(F(" ms"));
    Serial.print(F("  • lagRetries: "));
    Serial.println(lagRetries);

    Serial.println(F("\n[WALIDACJA] 7️⃣ ZAPLANOWANE RESETY:"));
    Serial.print(F("  • autoResetCountersHours: "));
    Serial.println(autoResetCountersHours);

    Serial.println(F("\n[WALIDACJA] 8️⃣ TRYB PRACY:"));
    Serial.print(F("  • intermittentMode: "));
    Serial.println(intermittentMode ? "true" : "false");
    Serial.print(F("  • awakeWindowMs: "));
    Serial.print(awakeWindowMs);
    Serial.println(F(" ms"));
    Serial.print(F("  • sleepWindowMs: "));
    Serial.print(sleepWindowMs);
    Serial.println(F(" ms"));

    Serial.println(F("\n[WALIDACJA] 9️⃣ ADRESY IP:"));
    Serial.print(F("  • host1: "));
    Serial.println(host1);
    Serial.print(F("  • host2: "));
    Serial.println(host2);
    Serial.print(F("  • gatewayOverride: "));
    Serial.println(gatewayOverride.length() > 0 ? gatewayOverride : "(pusty)");
    Serial.print(F("  • useGatewayOverride: "));
    Serial.println(useGatewayOverride ? "true" : "false");

    Serial.println(F("\n[WALIDACJA] 🔟 BEZPIECZEŃSTWO:"));
    Serial.print(F("  • adminUser: "));
    Serial.println(adminUser);
    Serial.print(F("  • adminPass: "));
    Serial.println(adminPass);

    // ========== FAZA 2: WALIDACJA WARTOŚCI ==========
    Serial.println(F("\n╠═══════════════════════════════════════════════════════════╣"));
    Serial.println(F("║  FAZA WALIDACJI                                         ║"));
    Serial.println(F("╠═══════════════════════════════════════════════════════════╣"));

    ValidationResult result;

    // Walidacja sekcja 1: Monitorowanie
    result = validatePositiveInt(pingInterval, "Interwał ping");
    if (!result.valid)
    {
        Serial.print(F("❌ BŁĄD: "));
        Serial.println(result.errorMsg);
        return result.errorMsg;
    }
    Serial.println(F("✅ Interwał ping: OK"));

    result = validatePositiveInt(failLimit, "Limit błędów");
    if (!result.valid)
    {
        Serial.print(F("❌ BŁĄD: "));
        Serial.println(result.errorMsg);
        return result.errorMsg;
    }
    Serial.println(F("✅ Limit błędów: OK"));

    // Walidacja sekcja 2: Reset routera
    result = validatePositiveInt(routerOffTime, "Czas wyłączenia routera");
    if (!result.valid)
    {
        Serial.print(F("❌ BŁĄD: "));
        Serial.println(result.errorMsg);
        return result.errorMsg;
    }
    Serial.println(F("✅ Czas wyłączenia routera: OK"));

    result = validatePositiveInt(baseBootTime, "Czas rozruchu routera");
    if (!result.valid)
    {
        Serial.print(F("❌ BŁĄD: "));
        Serial.println(result.errorMsg);
        return result.errorMsg;
    }
    Serial.println(F("✅ Czas rozruchu routera: OK"));

    // Walidacja sekcja 3: Boot loop
    result = validateIntRange(bootLoopWindowSeconds, 60, INT_MAX, "Okno boot loop");
    if (!result.valid)
    {
        Serial.print(F("❌ BŁĄD: "));
        Serial.println(result.errorMsg);
        return result.errorMsg;
    }
    Serial.println(F("✅ Okno boot loop: OK"));

    // Walidacja sekcja 4: WiFi/AP
    result = validatePositiveInt(noWiFiTimeout, "Timeout WiFi");
    if (!result.valid)
    {
        Serial.print(F("❌ BŁĄD: "));
        Serial.println(result.errorMsg);
        return result.errorMsg;
    }
    Serial.println(F("✅ Timeout WiFi: OK"));

    result = validatePositiveInt(apMaxAttempts, "Maksymalna liczba prób AP");
    if (!result.valid)
    {
        Serial.print(F("❌ BŁĄD: "));
        Serial.println(result.errorMsg);
        return result.errorMsg;
    }
    Serial.println(F("✅ Maksymalna liczba prób AP: OK"));

    // Walidacja sekcja 5: Dostawca
    result = validatePositiveInt(providerFailureLimit, "Limit resetów dla dostawcy");
    if (!result.valid)
    {
        Serial.print(F("❌ BŁĄD: "));
        Serial.println(result.errorMsg);
        return result.errorMsg;
    }
    Serial.println(F("✅ Limit resetów dla dostawcy: OK"));

    // Walidacja sekcja 6: Lag
    result = validatePositiveInt(maxPingMs, "Maksymalny ping");
    if (!result.valid)
    {
        Serial.print(F("❌ BŁĄD: "));
        Serial.println(result.errorMsg);
        return result.errorMsg;
    }
    Serial.println(F("✅ Maksymalny ping: OK"));

    result = validatePositiveInt(lagRetries, "Liczba spike'ów (lagRetries)");
    if (!result.valid)
    {
        Serial.print(F("❌ BŁĄD: "));
        Serial.println(result.errorMsg);
        return result.errorMsg;
    }
    Serial.println(F("✅ Liczba spike'ów: OK"));

    // Walidacja sekcja 7: Tryb przerwany
    if (intermittentMode)
    {
        Serial.println(F("\n[WALIDACJA] Tryb przerwany jest włączony - sprawdzam awakeWindowMs i sleepWindowMs..."));

        result = validatePositiveInt(awakeWindowMs, "Czas pracy w trybie przerywanym");
        if (!result.valid)
        {
            Serial.print(F("❌ BŁĄD: "));
            Serial.println(result.errorMsg);
            return result.errorMsg;
        }
        Serial.println(F("✅ Czas pracy: OK"));

        result = validatePositiveInt(sleepWindowMs, "Czas uśpienia w trybie przerywanym");
        if (!result.valid)
        {
            Serial.print(F("❌ BŁĄD: "));
            Serial.println(result.errorMsg);
            return result.errorMsg;
        }
        Serial.println(F("✅ Czas uśpienia (> 0): OK"));

        result = validateIntRange(sleepWindowMs, SLEEP_TIME_MIN_MS, SLEEP_TIME_MAX_MS, "Czas uśpienia");
        if (!result.valid)
        {
            Serial.print(F("❌ BŁĄD: "));
            Serial.println(result.errorMsg);
            return result.errorMsg;
        }
        Serial.print(F("✅ Czas uśpienia (zakres 5-60 min): OK ["));
        Serial.print(sleepWindowMs / 60000);
        Serial.println(F(" min]"));
    }
    else
    {
        Serial.println(F("[WALIDACJA] Tryb ciągły - pomijam walidację sleep/awake"));
    }

    // Walidacja adresy IP
    Serial.println(F("\n[WALIDACJA] Sprawdzam adresy IP..."));

    result = validateIpAddress(host1, "Host1");
    if (!result.valid)
    {
        Serial.print(F("❌ BŁĄD: "));
        Serial.println(result.errorMsg);
        return result.errorMsg;
    }
    Serial.print(F("✅ Host1 ("));
    Serial.print(host1);
    Serial.println(F("): OK"));

    result = validateIpAddress(host2, "Host2");
    if (!result.valid)
    {
        Serial.print(F("❌ BŁĄD: "));
        Serial.println(result.errorMsg);
        return result.errorMsg;
    }
    Serial.print(F("✅ Host2 ("));
    Serial.print(host2);
    Serial.println(F("): OK"));

    result = validateStringNotEqual(host1, host2, "Host1 i Host2 nie mogą być takie same");
    if (!result.valid)
    {
        Serial.print(F("❌ BŁĄD: "));
        Serial.println(result.errorMsg);
        return result.errorMsg;
    }
    Serial.println(F("✅ Host1 ≠ Host2: OK"));

    if (useGatewayOverride)
    {
        result = validateNonEmpty(gatewayOverride, "Włączono własną bramę, ale pole bramy jest puste");
        if (!result.valid)
        {
            Serial.print(F("❌ BŁĄD: "));
            Serial.println(result.errorMsg);
            return result.errorMsg;
        }

        result = validateIpAddress(gatewayOverride, "Adres bramy");
        if (!result.valid)
        {
            Serial.print(F("❌ BŁĄD: "));
            Serial.println(result.errorMsg);
            return result.errorMsg;
        }
        Serial.print(F("✅ Adres bramy ("));
        Serial.print(gatewayOverride);
        Serial.println(F("): OK"));
    }
    else
    {
        Serial.println(F("[WALIDACJA] Własna brama wyłączona - pomijam"));
    }

    // Walidacja hasła
    Serial.println(F("\n[WALIDACJA] Sprawdzam hasła..."));

    result = validateNonEmpty(adminUser, "Login administratora");
    if (!result.valid)
    {
        Serial.print(F("❌ BŁĄD: "));
        Serial.println(result.errorMsg);
        return result.errorMsg;
    }
    Serial.print(F("✅ Login administratora: "));
    Serial.println(adminUser);

    result = validateNonEmpty(adminPass, "Hasło administratora");
    if (!result.valid)
    {
        Serial.print(F("❌ BŁĄD: "));
        Serial.println(result.errorMsg);
        return result.errorMsg;
    }
    Serial.println(F("✅ Hasło administratora: ***"));

    // Walidacja zależności
    Serial.println(F("\n[WALIDACJA] Sprawdzam zależności między parametrami..."));

    result = validateGreaterOrEqual(providerFailureLimit, failLimit,
                                    "Limit resetów dla dostawcy", "limit błędów");
    if (!result.valid)
    {
        Serial.print(F("❌ BŁĄD: "));
        Serial.println(result.errorMsg);
        return result.errorMsg;
    }
    Serial.println(F("✅ providerFailureLimit >= failLimit: OK"));

    result = validateGreaterOrEqual(maxTotalResetsEver, providerFailureLimit,
                                    "Maksymalna liczba resetów ogółem", "limit resetów dla dostawcy");
    if (!result.valid)
    {
        Serial.print(F("❌ BŁĄD: "));
        Serial.println(result.errorMsg);
        return result.errorMsg;
    }
    Serial.println(F("✅ maxTotalResetsEver >= providerFailureLimit: OK"));

    // ========== SUKCES ==========
    Serial.println(F("\n╠═══════════════════════════════════════════════════════════╣"));
    Serial.println(F("║  ✅ WALIDACJA POWIODŁA SIĘ - DANE SĄ PRAWIDŁOWE         ║"));
    Serial.println(F("║  Parametry będą teraz zapisane do pamięci Flash         ║"));
    Serial.println(F("╚═══════════════════════════════════════════════════════════╝\n"));

    return ""; // Wszystko OK
}

#endif // CONFIG_VALIDATION_H
