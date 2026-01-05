#include "config.h"
#include "constants.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

Config config;
const char *CONFIG_FILE = "/config.json";

bool saveConfig()
{
    Serial.println(F("\n┌────────────────────────────────────────┐"));
    Serial.println(F("│ [ZAPIS] ZAPISUJĘ DO JSON / FLASH       │"));
    Serial.println(F("└────────────────────────────────────────┘"));

    Serial.println(F("\n[ZAPIS] Wartości do zapisu:"));
    Serial.print(F("  • config.pingInterval: "));
    Serial.println(config.pingInterval);
    Serial.print(F("  • config.failLimit: "));
    Serial.println(config.failLimit);
    Serial.print(F("  • config.providerFailureLimit: "));
    Serial.println(config.providerFailureLimit);
    Serial.print(F("  • config.autoResetCountersHours: "));
    Serial.println(config.autoResetCountersHours);
    Serial.print(F("  • config.maxPingMs: "));
    Serial.println(config.maxPingMs);
    Serial.print(F("  • config.lagRetries: "));
    Serial.println(config.lagRetries);
    Serial.print(F("  • config.routerOffTime: "));
    Serial.println(config.routerOffTime);
    Serial.print(F("  • config.baseBootTime: "));
    Serial.println(config.baseBootTime);
    Serial.print(F("  • config.apMaxAttempts: "));
    Serial.println(config.apMaxAttempts);
    Serial.print(F("  • config.sleepWindowMs: "));
    Serial.println(config.sleepWindowMs);
    Serial.print(F("  • config.awakeWindowMs: "));
    Serial.println(config.awakeWindowMs);
    Serial.print(F("  • config.darkMode: "));
    Serial.println(config.darkMode);
    Serial.print(F("  • config.ledBrightness: "));
    Serial.println(config.ledBrightness);
    Serial.print(F("  • config.host1: "));
    Serial.println(config.host1);
    Serial.print(F("  • config.host2: "));
    Serial.println(config.host2);
    Serial.print(F("  • config.gatewayOverride: "));
    Serial.println(config.gatewayOverride);
    Serial.print(F("  • config.useGatewayOverride: "));
    Serial.println(config.useGatewayOverride ? "ON" : "OFF");
    Serial.print(F("  • config.intermittentMode: "));
    Serial.println(config.intermittentMode ? "ON" : "OFF");
    Serial.print(F("  • config.watchdogEnabled: "));
    Serial.println(config.watchdogEnabled ? "ON" : "OFF");
    Serial.print(F("  • config.noWiFiBackoff: "));
    Serial.println(config.noWiFiBackoff ? "ON" : "OFF");
    Serial.print(F("  • config.bootLoopWindowSeconds: "));
    Serial.println(config.bootLoopWindowSeconds);
    Serial.print(F("  • config.adminUser: "));
    Serial.println(config.adminUser);
    Serial.print(F("  • config.globalUnit: "));
    Serial.println(config.globalUnit);
    Serial.println(F("\n[ZAPIS] Harmonogram resetów:"));
    Serial.print(F("  • config.scheduledResetsEnabled: "));
    Serial.println(config.scheduledResetsEnabled ? "ON" : "OFF");
    for (int i = 0; i < 5; i++)
    {
        Serial.print(F("  • config.scheduledResetTimes["));
        Serial.print(i);
        Serial.print(F("]: "));
        Serial.println(config.scheduledResetTimes[i].length() > 0 ? config.scheduledResetTimes[i] : "(pusty)");
    }

    Serial.println("[CONFIG] Attempting to save config...");
    Serial.print("[CONFIG] ledBrightness=");
    Serial.println(config.ledBrightness);
    Serial.print("[CONFIG] darkMode=");
    Serial.println(config.darkMode);
    Serial.print("[CONFIG] pingInterval=");
    Serial.println(config.pingInterval);

    File file = LittleFS.open(CONFIG_FILE, "w");
    if (!file)
    {
        Serial.println("[CONFIG] Failed to open config file for writing");
        return false;
    }

    JsonDocument doc;
    doc["pingInterval"] = config.pingInterval;
    doc["failLimit"] = config.failLimit;
    doc["routerOffTime"] = config.routerOffTime;
    doc["baseBootTime"] = config.baseBootTime;
    doc["noWiFiTimeout"] = config.noWiFiTimeout;
    doc["apConfigTimeout"] = config.apConfigTimeout;
    doc["apMaxAttempts"] = config.apMaxAttempts;
    doc["apBackoffMs"] = config.apBackoffMs;
    doc["dhcpTimeoutMs"] = config.dhcpTimeoutMs;
    doc["noWiFiBackoff"] = config.noWiFiBackoff;
    doc["darkMode"] = config.darkMode;
    doc["intermittentMode"] = config.intermittentMode;
    doc["awakeWindowMs"] = config.awakeWindowMs;
    doc["sleepWindowMs"] = config.sleepWindowMs;
    doc["host1"] = config.host1;
    doc["host2"] = config.host2;
    doc["gatewayOverride"] = config.gatewayOverride;
    doc["useGatewayOverride"] = config.useGatewayOverride;
    doc["pinRelay"] = config.pinRelay;
    doc["globalUnit"] = config.globalUnit;
    doc["relayActiveHigh"] = config.relayActiveHigh;
    doc["pinRed"] = config.pinRed;
    doc["pinGreen"] = config.pinGreen;
    doc["pinBlue"] = config.pinBlue;
    doc["pinButton"] = config.pinButton;
    doc["pinRelayBackup"] = config.pinRelayBackup;
    doc["ledBrightness"] = config.ledBrightness;
    doc["adminUser"] = config.adminUser;
    doc["adminPass"] = config.adminPass;
    doc["totalResets"] = config.totalResets;
    doc["nextResetDelay"] = config.nextResetDelay;
    doc["firstResetTime"] = config.firstResetTime;
    doc["lastResetTime"] = config.lastResetTime;
    doc["failCount"] = config.failCount;
    doc["noWiFiStartTime"] = config.noWiFiStartTime;
    doc["providerFailureLimit"] = config.providerFailureLimit;
    doc["maxPingMs"] = config.maxPingMs;
    doc["lagRetries"] = config.lagRetries;
    doc["maxTotalResetsEver"] = config.maxTotalResetsEver;
    doc["totalResetsEver"] = config.totalResetsEver;
    doc["resetDefault"] = config.resetDefault;
    doc["resetWdt"] = config.resetWdt;
    doc["resetException"] = config.resetException;
    doc["resetSoftWdt"] = config.resetSoftWdt;
    doc["resetSoft"] = config.resetSoft;
    doc["resetDeepSleep"] = config.resetDeepSleep;
    doc["resetExt"] = config.resetExt;
    doc["routerResetCount"] = config.routerResetCount;
    doc["autoResetCountersHours"] = config.autoResetCountersHours;
    doc["accumulatedFailureTime"] = config.accumulatedFailureTime;
    doc["scheduledResetsEnabled"] = config.scheduledResetsEnabled;
    doc["lastScheduledResetTime"] = (unsigned int)config.lastScheduledResetTime;
    doc["noNtpTimeSince"] = config.noNtpTimeSince;
    doc["lastNtpSync"] = (unsigned int)config.lastNtpSync;
    doc["ntpSyncMillis"] = config.ntpSyncMillis;
    doc["safeModeActive"] = config.safeModeActive;
    doc["bootLoopWindowSeconds"] = config.bootLoopWindowSeconds;
    doc["watchdogEnabled"] = config.watchdogEnabled;

    // === BACKUP NETWORK ===
    doc["enableBackupNetwork"] = config.enableBackupNetwork;
    doc["backupNetworkFailLimit"] = config.backupNetworkFailLimit;
    doc["backupNetworkRetryInterval"] = config.backupNetworkRetryInterval;
    doc["backupNetworkActive"] = config.backupNetworkActive;
    doc["lastBackupSwitchTime"] = (unsigned int)config.lastBackupSwitchTime;
    doc["lastBackupRetryTime"] = (unsigned int)config.lastBackupRetryTime;
    doc["backupNetworkFailCount"] = config.backupNetworkFailCount;

    // Tablica czasów scheduled resetów (format HH:MM)
    JsonArray scheduledTimes = doc.createNestedArray("scheduledResetTimes");
    for (int i = 0; i < 5; i++)
    {
        scheduledTimes.add(config.scheduledResetTimes[i]);
    }

    if (serializeJson(doc, file) == 0)
    {
        Serial.println("[CONFIG] Failed to write to config file");
        file.close();
        return false;
    }

    file.close();
    Serial.println("[CONFIG] Config saved successfully");

    Serial.println(F("\n[ZAPIS] ✅ Dane zostały zserializowane do JSON"));
    Serial.print(F("  Rozmiar JSON: 1200+ bajtów"));
    Serial.println(F("\n  Plik: /config.json"));

    // Weryfikacja: Sprawdzenie czy plik istnieje
    delay(50);
    File verifyFile = LittleFS.open(CONFIG_FILE, "r");
    if (verifyFile)
    {
        Serial.print("[CONFIG] Verification: File size = ");
        Serial.print(verifyFile.size());
        Serial.println(" bytes");
        verifyFile.close();
    }
    else
    {
        Serial.println("[CONFIG] ERROR: File was not saved properly!");
    }

    return true;
}

bool loadConfig()
{
    // Małe opóźnienie aby się upewnić że LittleFS jest gotowy
    delay(100);

    Serial.println(F("\n┌────────────────────────────────────────┐"));
    Serial.println(F("│ [ODCZYT] CZYTAM Z JSON / FLASH         │"));
    Serial.println(F("└────────────────────────────────────────┘"));

    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file)
    {
        Serial.println("[CONFIG] Config file not found, using defaults");
        Serial.println("[CONFIG] Creating default config file...");
        Serial.println(F("\n[ODCZYT] UŻYWAM WARTOŚCI DOMYŚLNYCH!"));
        delay(100); // Czekaj zanim spróbujesz zaoszczędzić
        config.pingInterval = 30000;
        config.failLimit = 3;
        config.routerOffTime = 60000;
        config.baseBootTime = 150000;
        config.noWiFiTimeout = 600000;
        config.apConfigTimeout = 600000;
        config.apMaxAttempts = 4;
        config.apBackoffMs = 3600000;
        config.dhcpTimeoutMs = 300000;
        config.noWiFiBackoff = false;
        config.darkMode = false;
        config.intermittentMode = false;
        config.awakeWindowMs = 300000;
        config.sleepWindowMs = 900000;
        config.host1 = "8.8.8.8";
        config.host2 = "1.1.1.1";
        config.gatewayOverride = "";
        config.useGatewayOverride = false;
        config.pinRelay = D1;
        config.pinRed = D6;
        config.pinGreen = D7;
        config.pinBlue = D8;
        config.pinButton = D5;
        config.pinRelayBackup = D2;
        config.ledBrightness = 255;
        config.adminUser = "admin";
        config.adminPass = "admin";
        config.totalResets = 0;
        config.nextResetDelay = FIVE_MINUTES_MS;
        config.firstResetTime = 0;
        config.lastResetTime = 0;
        config.failCount = 0;
        config.noWiFiStartTime = 0;
        config.providerFailureLimit = 5;
        config.maxPingMs = 2000;
        config.lagRetries = 3;
        config.maxTotalResetsEver = 20;
        config.totalResetsEver = 0;
        config.resetDefault = 0;
        config.resetWdt = 0;
        config.resetException = 0;
        config.resetSoftWdt = 0;
        config.resetSoft = 0;
        config.resetDeepSleep = 0;
        config.resetExt = 0;
        config.autoResetCountersHours = 0;
        config.accumulatedFailureTime = 0;

        // === BACKUP NETWORK DEFAULTS ===
        config.enableBackupNetwork = false;
        config.backupNetworkFailLimit = 5;
        config.backupNetworkRetryInterval = 600000;
        config.backupNetworkActive = false;
        config.lastBackupSwitchTime = 0;
        config.lastBackupRetryTime = 0;
        config.backupNetworkFailCount = 0;

        // Teraz zapisz domyślne wartości
        if (saveConfig())
        {
            Serial.println("[CONFIG] Default config file created successfully");
        }
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        Serial.println("[CONFIG] Failed to parse config file");
        return false;
    }

    config.pingInterval = doc["pingInterval"] | 30000;
    config.failLimit = doc["failLimit"] | 3;
    config.routerOffTime = doc["routerOffTime"] | 60000;
    config.baseBootTime = doc["baseBootTime"] | 150000;
    config.noWiFiTimeout = doc["noWiFiTimeout"] | 600000;
    config.apConfigTimeout = doc["apConfigTimeout"] | 600000;
    config.apMaxAttempts = doc["apMaxAttempts"] | 4;
    config.apBackoffMs = doc["apBackoffMs"] | 3600000;
    config.dhcpTimeoutMs = doc["dhcpTimeoutMs"] | 300000;
    config.noWiFiBackoff = doc["noWiFiBackoff"] | false;
    config.darkMode = doc["darkMode"] | false;
    config.intermittentMode = doc["intermittentMode"] | false;
    config.awakeWindowMs = doc["awakeWindowMs"] | 300000;
    config.sleepWindowMs = doc["sleepWindowMs"] | 900000;
    config.host1 = doc["host1"] | "8.8.8.8";
    config.host2 = doc["host2"] | "1.1.1.1";
    config.gatewayOverride = doc["gatewayOverride"] | "";
    config.useGatewayOverride = doc["useGatewayOverride"] | false;

    Serial.println(F("\n[ODCZYT] Wczytane wartości z JSON:"));
    Serial.print(F("  • config.pingInterval: "));
    Serial.println(config.pingInterval);
    Serial.print(F("  • config.failLimit: "));
    Serial.println(config.failLimit);
    Serial.print(F("  • config.providerFailureLimit: "));
    Serial.println(config.providerFailureLimit);
    Serial.print(F("  • config.autoResetCountersHours: "));
    Serial.println(config.autoResetCountersHours);
    Serial.print(F("  • config.maxPingMs: "));
    Serial.println(config.maxPingMs);
    Serial.print(F("  • config.lagRetries: "));
    Serial.println(config.lagRetries);
    Serial.print(F("  • config.routerOffTime: "));
    Serial.println(config.routerOffTime);
    Serial.print(F("  • config.baseBootTime: "));
    Serial.println(config.baseBootTime);
    Serial.print(F("  • config.apMaxAttempts: "));
    Serial.println(config.apMaxAttempts);
    Serial.print(F("  • config.sleepWindowMs: "));
    Serial.println(config.sleepWindowMs);
    Serial.print(F("  • config.awakeWindowMs: "));
    Serial.println(config.awakeWindowMs);
    Serial.print(F("  • config.noWiFiTimeout: "));
    Serial.println(config.noWiFiTimeout);
    Serial.print(F("  • config.darkMode: "));
    Serial.println(config.darkMode ? "ON" : "OFF");
    Serial.print(F("  • config.ledBrightness: "));
    Serial.println(config.ledBrightness);
    Serial.print(F("  • config.host1: "));
    Serial.println(config.host1);
    Serial.print(F("  • config.host2: "));
    Serial.println(config.host2);
    Serial.print(F("  • config.gatewayOverride: "));
    Serial.println(config.gatewayOverride);
    Serial.print(F("  • config.useGatewayOverride: "));
    Serial.println(config.useGatewayOverride ? "ON" : "OFF");
    Serial.print(F("  • config.intermittentMode: "));
    Serial.println(config.intermittentMode ? "ON" : "OFF");
    Serial.print(F("  • config.watchdogEnabled: "));
    Serial.println(config.watchdogEnabled ? "ON" : "OFF");
    Serial.print(F("  • config.noWiFiBackoff: "));
    Serial.println(config.noWiFiBackoff ? "ON" : "OFF");
    Serial.print(F("  • config.bootLoopWindowSeconds: "));
    Serial.println(config.bootLoopWindowSeconds);
    Serial.print(F("  • config.adminUser: "));
    Serial.println(config.adminUser);
    config.globalUnit = doc["globalUnit"] | 1000;
    Serial.print(F("  • config.globalUnit: "));
    Serial.println(config.globalUnit);
    Serial.println(F("\n[ODCZYT] Harmonogram resetów:"));
    Serial.print(F("  • config.scheduledResetsEnabled: "));
    Serial.println(config.scheduledResetsEnabled ? "ON" : "OFF");
    for (int i = 0; i < 5; i++)
    {
        Serial.print(F("  • config.scheduledResetTimes["));
        Serial.print(i);
        Serial.print(F("]: "));
        Serial.println(config.scheduledResetTimes[i].length() > 0 ? config.scheduledResetTimes[i] : "(pusty)");
    }

    config.pinRelay = doc["pinRelay"] | D1;
    config.relayActiveHigh = doc["relayActiveHigh"] | false;
    config.pinRed = doc["pinRed"] | D6;
    config.pinGreen = doc["pinGreen"] | D7;
    config.pinBlue = doc["pinBlue"] | D8;
    config.pinButton = doc["pinButton"] | D5;
    config.pinRelayBackup = doc["pinRelayBackup"] | D2;
    config.ledBrightness = doc["ledBrightness"] | 255;
    config.adminUser = doc["adminUser"] | "admin";
    config.adminPass = doc["adminPass"] | "admin";
    config.totalResets = doc["totalResets"] | 0;
    config.nextResetDelay = doc["nextResetDelay"] | FIVE_MINUTES_MS;
    config.firstResetTime = doc["firstResetTime"] | 0;
    config.lastResetTime = doc["lastResetTime"] | 0;
    config.failCount = doc["failCount"] | 0;
    config.noWiFiStartTime = doc["noWiFiStartTime"] | 0;
    config.providerFailureLimit = doc["providerFailureLimit"] | 5;
    config.maxPingMs = doc["maxPingMs"] | 2000;
    config.lagRetries = doc["lagRetries"] | 3;
    config.maxTotalResetsEver = doc["maxTotalResetsEver"] | 20;
    config.totalResetsEver = doc["totalResetsEver"] | 0;
    config.resetDefault = doc["resetDefault"] | 0;
    config.resetWdt = doc["resetWdt"] | 0;
    config.resetException = doc["resetException"] | 0;
    config.resetSoftWdt = doc["resetSoftWdt"] | 0;
    config.resetSoft = doc["resetSoft"] | 0;
    config.resetDeepSleep = doc["resetDeepSleep"] | 0;
    config.resetExt = doc["resetExt"] | 0;
    config.routerResetCount = doc["routerResetCount"] | 0;
    config.autoResetCountersHours = doc["autoResetCountersHours"] | 0;
    config.accumulatedFailureTime = doc["accumulatedFailureTime"] | 0;
    config.scheduledResetsEnabled = doc["scheduledResetsEnabled"] | false;
    config.lastScheduledResetTime = doc["lastScheduledResetTime"] | 0;
    config.noNtpTimeSince = doc["noNtpTimeSince"] | 0;
    config.lastNtpSync = doc["lastNtpSync"] | 0;
    config.ntpSyncMillis = doc["ntpSyncMillis"] | 0;
    config.safeModeActive = doc["safeModeActive"] | false;
    config.bootLoopWindowSeconds = doc["bootLoopWindowSeconds"] | 1200; // Default 20 min
    config.watchdogEnabled = doc["watchdogEnabled"] | true;             // Default enabled

    // === BACKUP NETWORK ===
    config.enableBackupNetwork = doc["enableBackupNetwork"] | false;
    config.backupNetworkFailLimit = doc["backupNetworkFailLimit"] | 5;
    config.backupNetworkRetryInterval = doc["backupNetworkRetryInterval"] | 600000;
    config.backupNetworkActive = doc["backupNetworkActive"] | false;
    config.lastBackupSwitchTime = doc["lastBackupSwitchTime"] | 0;
    config.lastBackupRetryTime = doc["lastBackupRetryTime"] | 0;
    config.backupNetworkFailCount = doc["backupNetworkFailCount"] | 0;

    // Ładowanie tablicy scheduled reset times (format HH:MM)
    JsonArray scheduledTimes = doc["scheduledResetTimes"];
    if (!scheduledTimes.isNull())
    {
        for (size_t i = 0; i < 5 && i < scheduledTimes.size(); i++)
        {
            config.scheduledResetTimes[i] = scheduledTimes[i] | "";
        }
    }
    else
    {
        // Domyślnie wszystkie puste (wyłączone)
        for (int i = 0; i < 5; i++)
        {
            config.scheduledResetTimes[i] = "";
        }
    }

    Serial.println("[CONFIG] Config loaded successfully");
    Serial.print("[CONFIG] ledBrightness=");
    Serial.println(config.ledBrightness);
    Serial.print("[CONFIG] darkMode=");
    Serial.println(config.darkMode);
    Serial.print("[CONFIG] pingInterval=");
    Serial.println(config.pingInterval);

    Serial.println(F("\n[ODCZYT] ✅ Wszystkie parametry załadowane z Flash"));

    return true;
}

bool isValidIP(String ip)
{
    int dots = 0;
    int num = 0;
    for (char c : ip)
    {
        if (c == '.')
        {
            if (num < 0 || num > 255)
                return false;
            num = 0;
            dots++;
        }
        else if (c >= '0' && c <= '9')
        {
            num = num * 10 + (c - '0');
        }
        else
        {
            return false;
        }
    }
    if (dots != 3 || num < 0 || num > 255)
        return false;
    return true;
}