# WiFiConfig Library - Historia zmian

> 📌 **Wersja:** PL | [EN](#english-version)

---

## 📖 Opis główny (Polski)

Biblioteka **WiFiConfig** została rozszerzona w wersji **1.1.0** o funkcjonalność **filtrowania sieci Wi-Fi po typie**. Ta zmiana umożliwia aplikacji szybkie przełączanie się między sieciami głównymi a rezerwowymi, co jest niezbędne dla implementacji funkcji **Sieci Rezerwowej (Backup Network)**.

### Główne cechy rozszerzenia:
- ✅ **Klasyfikacja sieci** – każda sieć jest oznaczona jako główna (0) lub rezerwowa (1)
- ✅ **Filtrowanie logiczne** – połączenie tylko z wybranym typem sieci
- ✅ **Wsteczna kompatybilność** – istniejący kod działa bez zmian
- ✅ **Brak efektów ubocznych** – zmiana dotyczy tylko parametrów wejścia funkcji
- ✅ **Diagnostyka** – logowanie każdej pominiętej sieci

---

## v1.1.0 (2026-01-04) - Network Type Filtering

### Nowe funkcjonalności

#### 1. Rozszerzenie struktury WiFiNetwork
**Plik:** `WiFiConfig.h` (linia 21-23)

Dodano pole do klasyfikacji sieci:
```cpp
struct WiFiNetwork
{
    String ssid;
    String pass;
    int networkType; // 0 = główna (primary), 1 = rezerwowa (backup)
};
```

**Zastosowanie:**
- `networkType = 0` – sieć główna (domyślnie)
- `networkType = 1` – sieć rezerwowa (backup)

---

#### 2. Nowy parametr funkcji PolaczZWiFi()
**Plik:** `WiFiConfig.h` (linia 41)

Zaktualizowana sygnatura:
```cpp
void PolaczZWiFi(WiFiNetwork sieci[], void (*ledHandler)() = nullptr, int filterNetworkType = -1);
// filterNetworkType:
//   -1  = wszystkie sieci (domyślnie, zachowuje wsteczną kompatybilność)
//    0  = tylko sieci główne (Primary)
//    1  = tylko sieci rezerwowe (Backup)
```

**Użycie:**
```cpp
// Połącz tylko z sieciami rezerwowymi (backup)
PolaczZWiFi(tablica, ledHandler, 1);

// Połącz ze wszystkimi sieciami (domyślne zachowanie)
PolaczZWiFi(tablica, ledHandler);
PolaczZWiFi(tablica, ledHandler, -1);
```

---

#### 3. Logika filtrowania w PolaczZWiFi()
**Plik:** `WiFiConfig.cpp` (linie 103-107)

Dodano warunkowe pominięcie sieci niezgodnych z filtrem:
```cpp
// Filtruj sieci wg typu, jeśli filterNetworkType != -1
if (filterNetworkType >= 0 && sieci[i].networkType != filterNetworkType)
{
    Serial.printf("Pomijam sieć %s (typ %d, szukam %d)\n", 
                  sieci[i].ssid.c_str(), sieci[i].networkType, filterNetworkType);
    continue;
}
```

**Zachowanie:**
- Pętla iteruje po wszystkich sieciach w tablicy
- Jeśli `filterNetworkType >= 0` i typ sieci nie pasuje, sieć jest pomijana
- Wyświetlane są logi dla każdej pominiętej sieci
- Połączenie następuje tylko z sieciami pasującymi do filtru

---

### Wsteczna kompatybilność
✅ **Pełna kompatybilność wstecz** – istniejący kod działać będzie bez zmian:
- Domyślna wartość `filterNetworkType = -1` oznacza "wszystkie sieci"
- Pole `networkType` w strukturze nie wpływa na istniejącą logikę
- Stary kod: `PolaczZWiFi(tablica, ledHandler)` – działa identycznie jak wcześniej

---

### Integracja z Backup Network (v1.1.2+)

Ta biblioteka wspiera funkcjonalność **Sieci Rezerwowej** implementowaną w głównej aplikacji:

1. **Faza główna** (odzyskiwanie głównej sieci):
   ```cpp
   PolaczZWiFi(tablica, ledHandler, 0);  // Tylko główne
   ```

2. **Faza rezerwowa** (gdy główna zawiedzie):
   ```cpp
   PolaczZWiFi(tablica, ledHandler, 1);  // Tylko rezerwowe
   ```

3. **Normalny tryb** (bez preferencji):
   ```cpp
   PolaczZWiFi(tablica, ledHandler);     // Wszystkie
   ```

---

### Notatki techniczne

- **Zmiana minimalna** – nie dotyka logiki połączeniowej WiFi
- **Tylko filtrowanie** – pominięte sieci po prostu nie są próbowane
- **Diagnostyka** – logi pokazują każdą pominięcie sieć dla debugowania
- **Brak efektu ubocznego** – jeśli żadna sieć nie pasuje do filtru, tworzone jest AP (istniejące zachowanie)

---

### Zmieniane pliki
- ✅ `WiFiConfig.h` – sygnatura funkcji + komentarz
- ✅ `WiFiConfig.cpp` – logika filtrowania w funkcji PolaczZWiFi()

### Pliki NIE zmieniane
- `WiFiConfig.cpp` – pozostała logika bez zmian
- Zawartość funkcji `zapiszTabliceDoPliku()`, `odczytajTabliceZPliku()` – bez zmian
- Inne funkcjonalności biblioteki – bez zmian

---

---

# English Version

> 📌 **Language:** [PL](#wificonfig-library---historia-zmian) | EN

## 📖 Main Description (English)

The **WiFiConfig** library has been extended in version **1.1.0** with **network type filtering functionality**. This change allows the application to quickly switch between primary and backup Wi-Fi networks, which is essential for implementing the **Backup Network** feature.

### Key features of the extension:
- ✅ **Network classification** – each network is marked as primary (0) or backup (1)
- ✅ **Logical filtering** – connection only to selected network type
- ✅ **Backward compatibility** – existing code works without changes
- ✅ **No side effects** – change only affects function input parameters
- ✅ **Diagnostics** – logging of each skipped network

## v1.1.0 (2026-01-04) - Network Type Filtering

### New Features

#### 1. WiFiNetwork Structure Extension
**File:** `WiFiConfig.h` (line 21-23)

Added field for network classification:
```cpp
struct WiFiNetwork
{
    String ssid;
    String pass;
    int networkType; // 0 = primary, 1 = backup
};
```

**Usage:**
- `networkType = 0` – primary network (default)
- `networkType = 1` – backup network

---

#### 2. New PolaczZWiFi() Function Parameter
**File:** `WiFiConfig.h` (line 41)

Updated signature:
```cpp
void PolaczZWiFi(WiFiNetwork sieci[], void (*ledHandler)() = nullptr, int filterNetworkType = -1);
// filterNetworkType:
//   -1  = all networks (default, maintains backward compatibility)
//    0  = primary networks only
//    1  = backup networks only
```

**Usage Example:**
```cpp
// Connect only to backup networks
PolaczZWiFi(tablica, ledHandler, 1);

// Connect to all networks (default behavior)
PolaczZWiFi(tablica, ledHandler);
PolaczZWiFi(tablica, ledHandler, -1);
```

---

#### 3. Filtering Logic in PolaczZWiFi()
**File:** `WiFiConfig.cpp` (lines 103-107)

Added conditional skipping of networks that don't match the filter:
```cpp
// Filter networks by type if filterNetworkType != -1
if (filterNetworkType >= 0 && sieci[i].networkType != filterNetworkType)
{
    Serial.printf("Skipping network %s (type %d, looking for %d)\n", 
                  sieci[i].ssid.c_str(), sieci[i].networkType, filterNetworkType);
    continue;
}
```

**Behavior:**
- Loop iterates through all networks in array
- If `filterNetworkType >= 0` and network type doesn't match, network is skipped
- Logs are displayed for each skipped network
- Connection occurs only with networks matching the filter

---

### Backward Compatibility
✅ **Full backward compatibility** – existing code will work without changes:
- Default value `filterNetworkType = -1` means "all networks"
- Field `networkType` in structure doesn't affect existing logic
- Old code: `PolaczZWiFi(tablica, ledHandler)` – works identically as before

---

### Integration with Backup Network (v1.1.2+)

This library supports the **Backup Network** functionality implemented in the main application:

1. **Primary phase** (recovering main network):
   ```cpp
   PolaczZWiFi(tablica, ledHandler, 0);  // Primary only
   ```

2. **Backup phase** (when primary fails):
   ```cpp
   PolaczZWiFi(tablica, ledHandler, 1);  // Backup only
   ```

3. **Normal mode** (no preference):
   ```cpp
   PolaczZWiFi(tablica, ledHandler);     // All networks
   ```

---

### Technical Notes

- **Minimal change** – doesn't touch WiFi connection logic
- **Filtering only** – skipped networks are simply not attempted
- **Diagnostics** – logs show each skipped network for debugging
- **No side effect** – if no network matches the filter, AP is created (existing behavior)

---

### Changed Files
- ✅ `WiFiConfig.h` – function signature + comment
- ✅ `WiFiConfig.cpp` – filtering logic in PolaczZWiFi() function

### Unchanged Files
- `WiFiConfig.cpp` – remaining logic unchanged
- Content of `zapiszTabliceDoPliku()`, `odczytajTabliceZPliku()` functions – unchanged
- Other library functionalities – unchanged

