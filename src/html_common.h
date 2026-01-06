#ifndef HTML_COMMON_H
#define HTML_COMMON_H

#include <Arduino.h>
#include <ESP8266WebServer.h>

// ============================================================================
// WSPÓLNE STYLE CSS - ZUNIFIKOWANY SYSTEM MOTYWÓW
// ============================================================================

const char HTML_THEME_STYLES[] PROGMEM = R"rawliteral(
:root {
    --bg: #f4f4f4;
    --fg: #333;
    --card: #fff;
    --btn: #007bff;
    --btn-hover: #0056b3;
    --inp: #fff;
    --brd: #ccc;
}
[data-theme='dark'] {
    --bg: #1a1a1a;
    --fg: #e0e0e0;
    --card: #2d2d2d;
    --btn: #0d6efd;
    --btn-hover: #0a58ca;
    --inp: #3a3a3a;
    --brd: #444;
}
body {
    font-family: Arial, sans-serif;
    margin: 20px;
    background-color: var(--bg);
    color: var(--fg);
    transition: background 0.3s, color 0.3s;
}
.container {
    max-width: 800px;
    margin: auto;
    background: var(--card);
    padding: 20px;
    border-radius: 8px;
    box-shadow: 0 0 10px rgba(0,0,0,0.1);
}
h1 {
    color: var(--fg);
    text-align: center;
}
.section {
    margin-bottom: 30px;
}
.section h2 {
    border-bottom: 2px solid #007bff;
    padding-bottom: 5px;
    color: var(--fg);
}
h3 {
    border-bottom: 2px solid #007bff;
    padding-bottom: 5px;
    color: var(--fg);
    margin-top: 30px;
}
form {
    display: flex;
    flex-direction: column;
}
label {
    margin-top: 10px;
    font-weight: bold;
    display: block;
}
input, select {
    padding: 8px;
    margin-top: 5px;
    border: 1px solid var(--brd);
    border-radius: 4px;
    background: var(--inp);
    color: var(--fg);
    width: 100%;
    box-sizing: border-box;
    display: block;
}
button {
    margin-top: 20px;
    padding: 10px;
    background-color: #007bff;
    color: white;
    border: none;
    border-radius: 4px;
    cursor: pointer;
}
button:hover {
    background-color: #0056b3;
}
.wifi-list {
    margin-top: 15px;
}
.wifi-item {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 10px;
    border: 1px solid var(--brd);
    margin-bottom: 8px;
    background-color: var(--inp);
    border-radius: 4px;
}
.wifi-item span {
    font-weight: 500;
    flex: 1;
}
.wifi-item button {
    background-color: #dc3545;
    color: white;
    border: none;
    padding: 6px 12px;
    border-radius: 4px;
    cursor: pointer;
    margin-left: 10px;
}
.wifi-item button:hover {
    background-color: #c82333;
}
.unit-select {
    width: 70px;
    margin-left: 5px;
}
.time-group {
    display: flex;
    align-items: center;
    position: relative;
}
.time-group input {
    flex: 1;
    padding: 8px 35px 8px 8px;
    margin-top: 5px;
}
.time-group button {
    position: absolute !important;
    right: 5px;
    top: 50%;
    transform: translateY(-50%);
    background: transparent !important;
    border: none !important;
    padding: 8px !important;
    margin: 0 !important;
    cursor: pointer;
    font-size: 18px;
    color: var(--fg);
}
.switch-wrap {
    display: flex;
    justify-content: flex-end;
    align-items: center;
    margin-bottom: 10px;
}
.switch {
    position: relative;
    display: inline-block;
    width: 50px;
    height: 24px;
    margin-left: 10px;
}
.switch input {
    opacity: 0;
    width: 0;
    height: 0;
}
.slider {
    position: absolute;
    cursor: pointer;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: #ccc;
    transition: .4s;
    border-radius: 24px;
}
.slider:before {
    position: absolute;
    content: "";
    height: 16px;
    width: 16px;
    left: 4px;
    bottom: 4px;
    background-color: white;
    transition: .4s;
    border-radius: 50%;
}
input:checked + .slider {
    background-color: #007bff;
}
input:checked + .slider:before {
    transform: translateX(26px);
}
.tooltip {
    position: relative;
    display: inline-block;
    cursor: pointer;
    color: #007bff;
    margin-left: 5px;
    font-weight: bold;
}
.tooltip .tooltiptext {
    visibility: hidden;
    width: 200px;
    background-color: #555;
    color: #fff;
    text-align: center;
    padding: 5px;
    border-radius: 6px;
    position: absolute;
    z-index: 1;
    bottom: 125%;
    left: 50%;
    margin-left: -100px;
    opacity: 0;
    transition: opacity 0.3s;
    font-size: 12px;
    font-weight: normal;
    box-shadow: 0 2px 5px rgba(0,0,0,0.2);
}
.tooltip .tooltiptext::after {
    content: "";
    position: absolute;
    top: 100%;
    left: 50%;
    margin-left: -5px;
    border-width: 5px;
    border-style: solid;
    border-color: #555 transparent transparent transparent;
}
.tooltip:hover .tooltiptext {
    visibility: visible;
    opacity: 1;
}
/* Accordion styles */
.accordion {
    background-color: var(--card);
    border: 1px solid var(--brd);
    border-radius: 6px;
    margin-bottom: 10px;
    overflow: hidden;
}
.accordion summary {
    padding: 12px 15px;
    cursor: pointer;
    background-color: var(--inp);
    color: var(--fg);
    font-size: 1.05em;
    user-select: none;
    transition: background-color 0.2s;
    list-style-position: inside;
}
.accordion summary:hover {
    background-color: var(--btn);
    color: white;
}
.accordion[open] summary {
    background-color: #007bff;
    color: white;
    border-bottom: 1px solid var(--brd);
}
.accordion-content {
    padding: 15px;
    background-color: var(--card);
}
.tab-nav {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
    margin: 12px 0 16px;
}
.tab-button {
    flex: 1 1 160px;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    padding: 10px 12px;
    border: 1px solid var(--brd);
    background: var(--inp);
    color: var(--fg);
    border-radius: 6px;
    cursor: pointer;
    transition: background-color 0.15s, color 0.15s, border-color 0.15s, box-shadow 0.15s;
}
.tab-button:hover {
    border-color: var(--btn);
    box-shadow: 0 2px 6px rgba(0,0,0,0.12);
}
.tab-button.active {
    background: var(--btn);
    border-color: var(--btn);
    color: white;
    box-shadow: 0 3px 10px rgba(0,0,0,0.15);
}
.tab-panel {
    margin-bottom: 14px;
}
.box {
    border: 1px solid #ccc;
    padding: 20px;
    border-radius: 10px;
    margin: 10px;
}
.ok {
    background-color: #d4edda;
    color: #155724;
}
.err {
    background-color: #f8d7da;
    color: #721c24;
}
@media (max-width: 600px) {
    body {
        margin: 10px;
    }
    .container {
        padding: 15px;
    }
}
)rawliteral";

// ============================================================================
// WSPÓLNE SKRYPTY JAVASCRIPT
// ============================================================================

const char HTML_COMMON_SCRIPTS[] PROGMEM = R"rawliteral(
<script>
function updateHidden(id) {
    var dispInput = document.getElementById(id + '_disp');
    var disp = dispInput.value;
    if (disp < 0) { disp = 0; dispInput.value = 0; }
    var unit = document.getElementById(id + '_unit').value;
    document.getElementById(id).value = Math.round(disp * unit);
}
function initTimeField(id, valMs) {
    console.log('[JS] initTimeField(' + id + ', ' + valMs + ' ms)');
    
    var unit = 1;
    if (valMs > 0) {
        if (valMs % 60000 == 0) unit = 60000;
        else if (valMs % 1000 == 0) unit = 1000;
    }
    document.getElementById(id + '_unit').value = unit;
    document.getElementById(id + '_disp').value = valMs / unit;
    document.getElementById(id).value = valMs;
    
    console.log('[JS] ' + id + '_disp = ' + (valMs / unit) + ', unit = ' + unit);
}
function convertUnit(id) {
    var valMs = document.getElementById(id).value;
    var unit = document.getElementById(id + '_unit').value;
    var newDisp = valMs / unit;
    if (newDisp % 1 !== 0) newDisp = parseFloat(newDisp.toFixed(2));
    document.getElementById(id + '_disp').value = newDisp;
}
function setGlobalUnit(unit) {
    // Zmień jednostki we wszystkich fieldach
    var fieldIds = ['pingInterval', 'routerOffTime', 'baseBootTime', 'noWiFiTimeout', 'apConfigTimeout', 'awakeWindowMs', 'sleepWindowMs', 'maxPingMs', 'apBackoffMs', 'dhcpTimeoutMs'];
    fieldIds.forEach(id => {
        var unitEl = document.getElementById(id + '_unit');
        if (unitEl) {
            unitEl.value = unit;
            convertUnit(id);
        }
    });
    // Zapisz wybraną jednostkę w ukrytym polu
    var globalUnitInput = document.getElementById('globalUnitValue');
    if (globalUnitInput) {
        globalUnitInput.value = unit;
    }
}
function showSaveNotice() {
    const params = new URLSearchParams(window.location.search);
    if (params.get('saved') !== '1') return;

    const toast = document.createElement('div');
    toast.textContent = '✅ Konfiguracja zapisana';
    toast.style.position = 'fixed';
    toast.style.top = '12px';
    toast.style.right = '12px';
    toast.style.background = '#0b5137';
    toast.style.color = '#d1fae5';
    toast.style.border = '1px solid #34d399';
    toast.style.padding = '10px 14px';
    toast.style.borderRadius = '6px';
    toast.style.boxShadow = '0 6px 16px rgba(0,0,0,0.25)';
    toast.style.zIndex = '9999';
    document.body.appendChild(toast);

    setTimeout(() => {
        toast.style.transition = 'opacity 0.4s ease, transform 0.4s ease';
        toast.style.opacity = '0';
        toast.style.transform = 'translateY(-8px)';
        setTimeout(() => toast.remove(), 450);
    }, 1800);

    // Usuń parametr z adresu, żeby nie powtarzać powiadomienia
    history.replaceState({}, '', window.location.pathname);
}
function validateSleepTimes() {
    // Walidacja czasów uśpienia: min 5 min, max 60 min
    const SLEEP_MIN_MS = 5 * 60 * 1000;      // 5 minut
    const SLEEP_MAX_MS = 60 * 60 * 1000;     // 60 minut
    const sleepField = document.getElementById('sleepWindowMs');
    const sleepDispField = document.getElementById('sleepWindowMs_disp');
    const sleepUnitField = document.getElementById('sleepWindowMs_unit');
    
    if (!sleepField || !sleepDispField || !sleepUnitField) return;
    
    let valMs = parseInt(sleepField.value) || 0;
    let currentUnit = parseInt(sleepUnitField.value);
    
    // Sprawdzenie limitów
    if (valMs < SLEEP_MIN_MS) {
        sleepField.value = SLEEP_MIN_MS;
        sleepDispField.value = (SLEEP_MIN_MS / currentUnit).toFixed(currentUnit >= 60000 ? 1 : (currentUnit >= 1000 ? 0 : 0));
        
        let unitName = currentUnit >= 60000 ? 'min' : (currentUnit >= 1000 ? 's' : 'ms');
        let displayValue = (SLEEP_MIN_MS / currentUnit).toFixed(1);
        alert('⚠️ Minimalny czas uśpienia to 5 minut (' + displayValue + ' ' + unitName + ')');
    } else if (valMs > SLEEP_MAX_MS) {
        sleepField.value = SLEEP_MAX_MS;
        sleepDispField.value = (SLEEP_MAX_MS / currentUnit).toFixed(currentUnit >= 60000 ? 1 : (currentUnit >= 1000 ? 0 : 0));
        
        let unitName = currentUnit >= 60000 ? 'min' : (currentUnit >= 1000 ? 's' : 'ms');
        let displayValue = (SLEEP_MAX_MS / currentUnit).toFixed(1);
        alert('⚠️ Maksymalny czas uśpienia to 60 minut (' + displayValue + ' ' + unitName + ')');
    }
}
function toggleTheme(checked) {
    document.documentElement.setAttribute('data-theme', checked ? 'dark' : 'light');
    localStorage.setItem('theme', checked ? 'dark' : 'light');
}
function initTheme() {
    var checkbox = document.getElementById('themeSwitch');
    if (!checkbox) return;
    var isConfigPage = (checkbox.getAttribute('form') === 'configForm');
    if (isConfigPage) {
        var isDark = checkbox.checked;
        document.documentElement.setAttribute('data-theme', isDark ? 'dark' : 'light');
        localStorage.setItem('theme', isDark ? 'dark' : 'light');
    } else {
        var savedTheme = localStorage.getItem('theme');
        if (savedTheme) {
            var isDark = (savedTheme === 'dark');
            document.documentElement.setAttribute('data-theme', isDark ? 'dark' : 'light');
            checkbox.checked = isDark;
        } else {
            var isDark = checkbox.checked;
            document.documentElement.setAttribute('data-theme', isDark ? 'dark' : 'light');
            localStorage.setItem('theme', isDark ? 'dark' : 'light');
        }
    }
}
function togglePassword(id) {
    var x = document.getElementById(id);
    var btn = x.parentElement.querySelector('button');
    x.type = (x.type === "password") ? "text" : "password";
    if (btn) {
        btn.textContent = x.type === "password" ? "👁️" : "👁️‍🗨️";
    }
    if (x) {
        x.focus();
        if (x.value && x.value.length) {
            x.setSelectionRange(x.value.length, x.value.length);
        }
    }
}
function updateBrightness(value) {
    document.getElementById('ledBrightnessVal').textContent = value;
    fetch('/setbrightness?val=' + value);
}

// Accordion - zamykaj poprzednio otwartą zakładkę gdy otworzy się nowa
document.addEventListener('DOMContentLoaded', function() {
    const accordions = document.querySelectorAll('.accordion, .section.accordion');
    accordions.forEach(accordion => {
        const summary = accordion.querySelector('summary');
        if (summary) {
            summary.addEventListener('click', function() {
                const wasOpen = accordion.hasAttribute('open');
                const container = accordion.parentElement || accordion;
                const allAccordions = container.querySelectorAll('.accordion, .section.accordion');
                allAccordions.forEach(other => {
                    if (other !== accordion) {
                        other.removeAttribute('open');
                    }
                });
                // Przewiń do otwartej sekcji (po krótkim opóźnieniu aby accordion się rozwinął)
                if (!wasOpen) {
                    setTimeout(() => {
                        accordion.scrollIntoView({ behavior: 'smooth', block: 'start' });
                    }, 100);
                }
            });
        }
    });
});
</script>
)rawliteral";

// ============================================================================
// FUNKCJE POMOCNICZE DO GENEROWANIA HTML
// ============================================================================

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

// Generuje pełny nagłówek HTML z ikoną, stylami i skryptami
inline void sendHtmlHeader(ESP8266WebServer &server, const char *title, bool darkMode)
{
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html; charset=utf-8", "");

    String html = F("<!DOCTYPE html><html lang='pl'><head>");
    html += F("<meta charset='UTF-8'>");
    html += F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    html += F("<title>");
    html += title;
    html += F("</title>");
    html += F("<link rel='icon' href='data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI+PHBhdGggZmlsbD0iIzAwN2JmZiIgZD0iTTEyIDFMMyA1djZjMCA1LjU1IDMuODQgMTAuNzQgOSAxMiA1LjE2LTEuMjYgOS02LjQ1IDktMTJWNWwtOS00eiIvPjwvc3ZnPg=='>");
    html += F("<style>");
    server.sendContent(html);
    server.sendContent_P(HTML_THEME_STYLES);
    html = F("</style>");
    server.sendContent(html);
    server.sendContent_P(HTML_COMMON_SCRIPTS);
    server.sendContent(getThemeScript(darkMode));
}

// Generuje kompletną stronę błędu z motywem
inline void sendErrorPage(ESP8266WebServer &server, const char *title, const char *message, const char *backLink, const char *backText, bool darkMode)
{
    String html = F("<!DOCTYPE html><html><head>");
    html += F("<meta charset='UTF-8'>");
    html += F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    html += F("<title>");
    html += title;
    html += F("</title>");
    html += F("<link rel='icon' href='data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI+PHBhdGggZmlsbD0iIzAwN2JmZiIgZD0iTTEyIDFMMyA1djZjMCA1LjU1IDMuODQgMTAuNzQgOSAxMiA1LjE2LTEuMjYgOS02LjQ1IDktMTJWNWwtOS00eiIvPjwvc3ZnPg=='>");
    html += F("<style>");
    html += FPSTR(HTML_THEME_STYLES);
    html += F("body{display:flex;justify-content:center;align-items:center;height:100vh;margin:0}");
    html += F(".error-box{background:var(--card);padding:40px;border-radius:8px;box-shadow:0 4px 6px rgba(0,0,0,0.1);text-align:center;width:320px}");
    html += F("h2{color:#d9534f;margin-bottom:20px}");
    html += F("p{color:var(--fg);margin-bottom:20px}");
    html += F("a{display:inline-block;padding:10px 20px;background:var(--btn);color:white;text-decoration:none;border-radius:4px}");
    html += F("a:hover{background:var(--btn-hover)}");
    html += F("</style>");
    html += getThemeScript(darkMode);
    html += F("</head><body>");
    html += F("<div class='error-box'>");
    html += F("<h2>");
    html += title;
    html += F("</h2>");
    html += F("<p>");
    html += message;
    html += F("</p>");
    html += F("<a href='");
    html += backLink;
    html += F("'>");
    html += backText;
    html += F("</a>");
    html += F("</div></body></html>");

    server.send(200, "text/html; charset=utf-8", html);
}

// Generuje kompletną stronę sukcesu z motywem
inline void sendSuccessPage(ESP8266WebServer &server, const char *title, const char *message, const char *backLink, const char *backText, bool darkMode)
{
    String html = F("<!DOCTYPE html><html><head>");
    html += F("<meta charset='UTF-8'>");
    html += F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    html += F("<title>");
    html += title;
    html += F("</title>");
    html += F("<link rel='icon' href='data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI+PHBhdGggZmlsbD0iIzAwN2JmZiIgZD0iTTEyIDFMMyA1djZjMCA1LjU1IDMuODQgMTAuNzQgOSAxMiA1LjE2LTEuMjYgOS02LjQ1IDktMTJWNWwtOS00eiIvPjwvc3ZnPg=='>");
    html += F("<style>");
    html += FPSTR(HTML_THEME_STYLES);
    html += F("body{display:flex;justify-content:center;align-items:center;height:100vh;margin:0}");
    html += F(".success-box{background:var(--card);padding:40px;border-radius:8px;box-shadow:0 4px 6px rgba(0,0,0,0.1);text-align:center;width:320px}");
    html += F("h2{color:#28a745;margin-bottom:20px}");
    html += F("p{color:var(--fg);margin-bottom:20px}");
    html += F("a{display:inline-block;padding:10px 20px;background:var(--btn);color:white;text-decoration:none;border-radius:4px;margin:5px}");
    html += F("a:hover{background:var(--btn-hover)}");
    html += F("</style>");
    html += getThemeScript(darkMode);
    html += F("</head><body>");
    html += F("<div class='success-box'>");
    html += F("<h2>");
    html += title;
    html += F("</h2>");
    html += F("<p>");
    html += message;
    html += F("</p>");
    html += F("<a href='");
    html += backLink;
    html += F("'>");
    html += backText;
    html += F("</a>");
    html += F("</div></body></html>");

    server.send(200, "text/html; charset=utf-8", html);
}

// Przekierowanie HTTP (zamiast JavaScript)
inline void redirectTo(ESP8266WebServer &server, const char *location)
{
    server.sendHeader("Location", location);
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    server.send(303);
}

// Generuje stronę z odliczaniem i automatycznym przekierowaniem
inline void sendCountdownPage(ESP8266WebServer &server, const char *title, const char *message, int seconds, const char *redirectUrl, bool darkMode)
{
    String html = F("<!DOCTYPE html><html><head>");
    html += F("<meta charset='UTF-8'>");
    html += F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    html += F("<title>");
    html += title;
    html += F("</title>");
    html += F("<link rel='icon' href='data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI+PHBhdGggZmlsbD0iIzAwN2JmZiIgZD0iTTEyIDFMMyA1djZjMCA1LjU1IDMuODQgMTAuNzQgOSAxMiA1LjE2LTEuMjYgOS02LjQ1IDktMTJWNWwtOS00eiIvPjwvc3ZnPg=='>");
    html += F("<style>");
    html += FPSTR(HTML_THEME_STYLES);
    html += F("body{display:flex;justify-content:center;align-items:center;height:100vh;margin:0}");
    html += F(".countdown-box{background:var(--card);padding:40px;border-radius:8px;box-shadow:0 4px 6px rgba(0,0,0,0.1);text-align:center;width:400px}");
    html += F("h2{color:#007bff;margin-bottom:20px}");
    html += F("p{color:var(--fg);margin-bottom:20px;font-size:16px}");
    html += F(".countdown{font-size:72px;font-weight:bold;color:#007bff;margin:30px 0;font-family:monospace}");
    html += F(".spinner{border:4px solid var(--brd);border-top:4px solid #007bff;border-radius:50%;width:60px;height:60px;animation:spin 1s linear infinite;margin:20px auto}");
    html += F("@keyframes spin{0%{transform:rotate(0deg)}100%{transform:rotate(360deg)}}");
    html += F(".progress-bar{width:100%;height:8px;background:var(--brd);border-radius:4px;overflow:hidden;margin:20px 0}");
    html += F(".progress-fill{height:100%;background:#007bff;width:100%;animation:progress ");
    html += seconds;
    html += F("s linear}");
    html += F("@keyframes progress{from{width:100%}to{width:0%}}");
    html += F("</style>");
    html += getThemeScript(darkMode);
    html += F("<script>");
    html += F("let timeLeft=");
    html += seconds;
    html += F(";");
    html += F("function updateCountdown(){");
    html += F("document.getElementById('countdown').textContent=timeLeft;");
    html += F("if(timeLeft<=0){");
    html += F("document.getElementById('message').textContent='Przekierowywanie...';");
    html += F("window.location.href='");
    html += redirectUrl;
    html += F("';}else{timeLeft--;setTimeout(updateCountdown,1000);}}");
    html += F("window.onload=updateCountdown;");
    html += F("</script>");
    html += F("</head><body>");
    html += F("<div class='countdown-box'>");
    html += F("<h2>");
    html += title;
    html += F("</h2>");
    html += F("<div class='spinner'></div>");
    html += F("<div class='countdown' id='countdown'>");
    html += seconds;
    html += F("</div>");
    html += F("<p id='message'>");
    html += message;
    html += F("</p>");
    html += F("<div class='progress-bar'><div class='progress-fill'></div></div>");
    html += F("</div></body></html>");

    server.send(200, "text/html; charset=utf-8", html);
}

#endif
