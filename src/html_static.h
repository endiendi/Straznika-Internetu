#ifndef HTML_STATIC_H
#define HTML_STATIC_H

#include <Arduino.h>
#include "html_common.h"

// Ten plik używa teraz zunifikowanego systemu z html_common.h
// Zachowano stare nazwy dla kompatybilności wstecznej

// Alias do wspólnych stylów (dla kompatybilności)
#define HTML_HEAD HTML_THEME_STYLES
#define JS_SCRIPTS HTML_COMMON_SCRIPTS

#endif