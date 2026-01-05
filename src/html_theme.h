#ifndef HTML_THEME_H
#define HTML_THEME_H

#include <Arduino.h>

// Wspólne style CSS dla motywów (jasny/ciemny)
// Używane przez wszystkie strony HTML (login, error, główna)

const char HTML_THEME_STYLES[] PROGMEM = R"rawliteral(
:root{
  --bg:#f4f4f4;
  --fg:#333;
  --card:#fff;
  --btn:#007bff;
  --btn-hover:#0056b3;
  --inp:#fff;
  --brd:#ddd;
}
[data-theme='dark']{
  --bg:#1a1a1a;
  --fg:#e0e0e0;
  --card:#2d2d2d;
  --btn:#0d6efd;
  --btn-hover:#0a58ca;
  --inp:#3a3a3a;
  --brd:#444;
}
body{
  font-family:Arial,sans-serif;
  background:var(--bg);
  color:var(--fg);
  transition:background 0.3s,color 0.3s;
}
@keyframes pulse{
  0%{box-shadow:0 2px 8px rgba(40,167,69,0.3);}
  50%{box-shadow:0 4px 20px rgba(40,167,69,0.8);}
  100%{box-shadow:0 2px 8px rgba(40,167,69,0.3);}
}
)rawliteral";

// Generuje inline script ustawiający motyw na podstawie config.darkMode
inline String getThemeScript(bool darkMode)
{
  String script = F("<script>");
  script += F("document.documentElement.setAttribute('data-theme','");
  script += (darkMode ? "dark" : "light");
  script += F("');");
  script += F("</script>");
  return script;
}

#endif
