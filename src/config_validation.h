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
String validateAllConfigParams(
    int pingInterval, int failLimit, int providerFailureLimit,
    int autoResetCountersHours, int maxPingMs, int lagRetries,
    int bootLoopWindowSeconds, int apMaxAttempts, int routerOffTime,
    int baseBootTime, int noWiFiTimeout, bool intermittentMode,
    int awakeWindowMs, int sleepWindowMs, const String &host1,
    const String &host2, const String &gatewayOverride, bool useGatewayOverride,
    const String &adminUser, const String &adminPass, int maxTotalResetsEver)
{
    // Podstawowe walidacje dla pól wymaganych
    ValidationResult result;

    result = validatePositiveInt(pingInterval, "Interwał ping");
    if (!result.valid)
        return result.errorMsg;

    result = validatePositiveInt(failLimit, "Limit błędów");
    if (!result.valid)
        return result.errorMsg;

    result = validatePositiveInt(providerFailureLimit, "Limit resetów dla dostawcy");
    if (!result.valid)
        return result.errorMsg;

    result = validatePositiveInt(maxPingMs, "Maksymalny ping");
    if (!result.valid)
        return result.errorMsg;

    result = validatePositiveInt(lagRetries, "Liczba spike'ów (lagRetries)");
    if (!result.valid)
        return result.errorMsg;

    result = validateIntRange(bootLoopWindowSeconds, 60, INT_MAX, "Okno boot loop");
    if (!result.valid)
        return result.errorMsg;

    result = validatePositiveInt(apMaxAttempts, "Maksymalna liczba prób AP");
    if (!result.valid)
        return result.errorMsg;

    result = validatePositiveInt(routerOffTime, "Czas wyłączenia routera");
    if (!result.valid)
        return result.errorMsg;

    result = validatePositiveInt(baseBootTime, "Czas rozruchu routera");
    if (!result.valid)
        return result.errorMsg;

    result = validatePositiveInt(noWiFiTimeout, "Timeout WiFi");
    if (!result.valid)
        return result.errorMsg;

    // Walidacja trybu przerywanego
    if (intermittentMode)
    {
        result = validatePositiveInt(awakeWindowMs, "Czas pracy w trybie przerywanym");
        if (!result.valid)
            return result.errorMsg;

        result = validatePositiveInt(sleepWindowMs, "Czas uśpienia w trybie przerywanym");
        if (!result.valid)
            return result.errorMsg;

        result = validateIntRange(sleepWindowMs, SLEEP_TIME_MIN_MS, SLEEP_TIME_MAX_MS, "Czas uśpienia");
        if (!result.valid)
            return result.errorMsg;
    }

    // Walidacja adresów IP
    result = validateIpAddress(host1, "Host1");
    if (!result.valid)
        return result.errorMsg;

    result = validateIpAddress(host2, "Host2");
    if (!result.valid)
        return result.errorMsg;

    result = validateStringNotEqual(host1, host2, "Host1 i Host2 nie mogą być takie same");
    if (!result.valid)
        return result.errorMsg;

    // Walidacja bramy
    if (useGatewayOverride)
    {
        result = validateNonEmpty(gatewayOverride, "Włączono własną bramę, ale pole bramy jest puste");
        if (!result.valid)
            return result.errorMsg;

        result = validateIpAddress(gatewayOverride, "Adres bramy");
        if (!result.valid)
            return result.errorMsg;
    }

    // Walidacja haseł
    result = validateNonEmpty(adminUser, "Login administratora");
    if (!result.valid)
        return result.errorMsg;

    result = validateNonEmpty(adminPass, "Hasło administratora");
    if (!result.valid)
        return result.errorMsg;

    // Walidacja zależności między limitami
    result = validateGreaterOrEqual(providerFailureLimit, failLimit,
                                    "Limit resetów dla dostawcy", "limit błędów");
    if (!result.valid)
        return result.errorMsg;

    result = validateGreaterOrEqual(maxTotalResetsEver, providerFailureLimit,
                                    "Maksymalna liczba resetów ogółem", "limit resetów dla dostawcy");
    if (!result.valid)
        return result.errorMsg;

    return ""; // Wszystko OK
}

#endif // CONFIG_VALIDATION_H
