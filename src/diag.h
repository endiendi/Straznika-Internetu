#ifndef DIAG_H
#define DIAG_H

// Ustaw DIAG_ENABLED na 0, aby wyłączyć logi diagnostyczne bez modyfikacji kodu
// Oszczędza ~500B RAM gdy wyłączone
#ifndef DIAG_ENABLED
#define DIAG_ENABLED 1 // Wyłączone w celu oszczędności RAM
#endif

#if DIAG_ENABLED
#define DIAG_PRINTLN(msg) Serial.println(msg)
#define DIAG_PRINT(msg) Serial.print(msg)
#define DIAG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DIAG_PRINTLN(msg) \
    do                    \
    {                     \
    } while (0)
#define DIAG_PRINT(msg) \
    do                  \
    {                   \
    } while (0)
#define DIAG_PRINTF(...) \
    do                   \
    {                    \
    } while (0)
#endif

#endif // DIAG_H
