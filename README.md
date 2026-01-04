# Strażnik Internetu (Internet Watchdog)

## Opis projektu
Urządzenie oparte na układzie ESP8266, którego zadaniem jest monitorowanie stabilności połączenia internetowego. W przypadku wykrycia awarii (brak odpowiedzi na Ping), urządzenie automatycznie resetuje router poprzez chwilowe odcięcie zasilania za pomocą przekaźnika.

## Funkcjonalności
*   **Monitorowanie**: Cykliczne sprawdzanie dostępności internetu (Ping do 8.8.8.8 i 1.1.1.1).
*   **Auto-Reset**: Automatyczny restart routera po przekroczeniu limitu błędów.
*   **Panel WWW**: Konfiguracja parametrów, podgląd statusu i logów zdarzeń przez przeglądarkę.
*   **Sieć Rezerwowa (v1.1.2+)**: Automatyczne przełączenie na drugi router/hotspot w przypadku wyczerpania prób naprawy sieci głównej (domyślnie drugi przekaźnik D2).
*   **Inteligentne zarządzanie**:
    *   Wykrywanie wysokiego pingu (lagów).
    *   Mechanizm "Backoff" – wydłużanie czasu między resetami w przypadku długotrwałej awarii.
    *   Zabezpieczenie przed pętlą resetów (limit resetów dla awarii dostawcy).
    *   Auto-reset liczników po upłynięciu czasu awaryjności (czysta karta).

## Schemat połączeń (Wiring)

Urządzenie wykorzystuje następujące piny (domyślna konfiguracja dla ESP8266 / NodeMCU / Wemos D1 Mini):

| Element | Pin ESP | Opis |
| :--- | :--- | :--- |
| **Przekaźnik (Relay główny)** | **D1** (GPIO 5) | Steruje zasilaniem routera głównego. Stan wysoki (HIGH) aktywuje przekaźnik (odcina zasilanie). Podłącz router przez styki NC (Normally Closed). |
| **Przekaźnik (Relay backup)** | **D2** (GPIO 4) | *(Opcjonalnie, v1.1.2+)* Steruje zasilaniem routera rezerwowego/hotspotu. Włącza się automatycznie gdy główna sieć zawiedzie. |
| **Dioda Czerwona** | **D6** (GPIO 12) | Sygnalizuje błąd połączenia lub trwający reset. |
| **Dioda Zielona** | **D7** (GPIO 13) | Sygnalizuje poprawne połączenie z internetem. |
| **Dioda Niebieska** | **D8** (GPIO 15) | Sygnalizuje tryb konfiguracji (AP), stan oczekiwania (Backoff) lub pracę na sieci rezerwowej. |
| **Przycisk** | **D5** (GPIO 14) | Przycisk sterujący (zwiera do masy/GND). Programowo włączony wewnętrzny rezystor pull-up, więc zewnętrzny nie jest wymagany. |

> **Uwaga:** Można zastosować diodę RGB (wspólna katoda/GND) podłączając odpowiednie piny R, G, B do wyjść D6, D7, D8. Pamiętaj o zastosowaniu rezystorów ograniczających prąd (np. 220Ω - 330Ω) na każdej linii sygnałowej diody.

## Sygnalizacja LED

*   🟢 **Zielona**: Internet działa poprawnie (z sieci głównej lub rezerwowej).
*   🔴 **Czerwona**: Wykryto błąd połączenia, trwa procedura resetu lub router jest wyłączony.
*   🔵 **Niebieska**: Urządzenie znajduje się w trybie punktu dostępowego (AP) i czeka na konfigurację LUB trwa okres karencji po włączeniu zasilania (oczekiwanie na ustabilizowanie sieci) LUB praca na sieci rezerwowej.

## Obsługa przycisku

1.  **Krótkie naciśnięcie**: Przełącza urządzenie w tryb konfiguracji ręcznej. Uruchamia punkt dostępowy WiFi (AP), umożliwiając zmianę ustawień, jeśli np. zmieniono hasło do domowego WiFi.
2.  **Długie naciśnięcie (> 10 sekund)**: Przywraca ustawienia fabryczne. Kasuje konfigurację WiFi oraz ustawienia aplikacji i restartuje układ.

## Instalacja i Konfiguracja

### 1. Pierwsze uruchomienie
Po podłączeniu zasilania, jeśli urządzenie nie ma skonfigurowanej sieci WiFi, uruchomi własną sieć (Hotspot).
1.  Wyszukaj na telefonie/komputerze sieć WiFi o nazwie: **`ESP8266_Config`**.
2.  Połącz się z nią (brak hasła).
3.  Otwórz przeglądarkę i wpisz adres: **`http://192.168.4.1`** lub **`http://straznik.local`**.

### 2. Logowanie
Domyślne dane dostępowe do panelu:
*   Użytkownik: **`admin`**
*   Hasło: **`admin`**

### 3. Konfiguracja w panelu
W zakładce "Konfiguracja":
*   **Sieci WiFi**: Wpisz nazwę (SSID) i hasło swojej sieci domowej, a następnie kliknij "Dodaj sieć".
    - Możesz oznaczyć sieć jako **Główna** (Primary) lub **Rezerwowa** (Backup) – rezerwowe włączają się gdy główna zawiedzie.
*   **Parametry aplikacji**: Dostosuj czasy pingowania, limity błędów oraz czasy resetu routera.
*   **Sieć Rezerwowa** (v1.1.2+): 
    - Włącz opcję "Sieć rezerwowa" jeśli posiadasz drugi router/hotspot.
    - Ustaw pin przekaźnika drugiego routera (domyślnie D2 – bezpieczny na starcie).
    - Skonfiguruj próg włączenia backupu (liczba błędów) i interwał powrotu do głównej.
Po zapisaniu konfiguracji urządzenie spróbuje połączyć się z Twoją siecią. Jeśli się uda, dioda zmieni kolor na zielony (po udanym pingu).

### 4. Sieć Rezerwowa (Backup Network) – v1.1.2+

Jeśli posiadasz drugi router lub hotspot:

1. **Przygotowanie sprzętu:**
    - Podłącz drugi router do drugiego przekaźnika (domyślnie pin D2 lub inny bezpieczny).
   - Upewnij się, że drugi router ma własną sieć WiFi skonfigurowaną.

2. **Dodanie sieci rezerwowej:**
   - W panelu przejdź do "Sieci WiFi".
   - Dodaj sieć drugiego routera i **zaznacz ją jako "Rezerwowa"**.

3. **Włączenie funkcji:**
   - Przejdź do sekcji "Sieć Rezerwowa".
   - Włącz opcję "Sieć rezerwowa".
   - Ustaw parametry (próg włączenia, interwał powrotu).

**Jak to działa:**
- Jeśli główna sieć zawiedzie i wszystkie procedury naprawy (resety, backoff, AP) się wyczerpią:
    - Włączy się drugi router (relay D2 → ON)
    - Po 150 sekund czekania (grace period) – urządzenie połączy się z siecią rezerwową
    - Co 10 minut będzie próbować powrotu do głównej
    - Jeśli główna wróci → automatyczny powrót, relay D2 → OFF

**Bezpieczeństwo:**
- Backup NIE tworzy AP (wymaga automatycznego działania)
- W trybie backup ignorowane są inne sieci WiFi
- Po `autoResetCountersHours` (np. 12h) – system się resetuje i powraca do głównej

## Aktualizacja oprogramowania (OTA)

Urządzenie wspiera aktualizację bezprzewodową.
1.  W panelu WWW przejdź do sekcji "Inne opcje".
2.  Kliknij przycisk **Aktualizacja (OTA)**.
3.  Wybierz plik `.bin` z nowym firmwarem i kliknij "Wgraj".

## Historia wersji

- **v1.1.2** (2026-01-04) – Sieć rezerwowa (Backup Network): automatyczne przełączenie na drugi router przy wyczerpaniu procedur naprawy głównej. Filtrowanie sieci, blokada AP w backup, grace period dla backup routera.
- **v1.0.48** – Wersja bazowa z monitorowaniem, auto-resetem i panelem konfiguracyjnym.

---

*Projekt stworzony w środowisku PlatformIO.*