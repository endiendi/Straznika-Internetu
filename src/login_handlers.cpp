// Handlery logowania - oddzielny plik dla przejrzystości
#include "webserver.h"
#include "config.h"
#include "diag.h"
#include "html_common.h"
#include "constants.h"
#include "app_globals.h" // Centralne extern deklaracje
#include <ESP8266WebServer.h>

extern ESP8266WebServer server;

// Strona logowania - formularz HTML
void handleLoginPage()
{
    DIAG_PRINTLN(F("\n========== handleLoginPage START =========="));

    // Jeśli już zalogowany, przekieruj na /config
    if (isSessionActive && sessionToken.length() > 0)
    {
        DIAG_PRINTLN(F("[LOGIN] User already logged in, redirecting to /config"));
        redirectTo(server, "/config");
        return;
    }

    DIAG_PRINTLN(F("[LOGIN] Displaying login form"));

    // Prosty formularz logowania - używa wspólnych stylów motywu
    String html = F("<!DOCTYPE html><html><head>");
    html += F("<meta charset='UTF-8'>");
    html += F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    html += F("<title>Logowanie - ESP Tester</title>");
    html += F("<link rel='icon' href='data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI+PHBhdGggZmlsbD0iIzAwN2JmZiIgZD0iTTEyIDFMMyA1djZjMCA1LjU1IDMuODQgMTAuNzQgOSAxMiA1LjE2LTEuMjYgOS02LjQ1IDktMTJWNWwtOS00eiIvPjwvc3ZnPg=='>");
    html += F("<style>");
    html += FPSTR(HTML_THEME_STYLES); // Wspólne style motywu
    html += F("body{display:flex;justify-content:center;align-items:center;height:100vh;margin:0}");
    html += F(".login-box{background:var(--card);padding:40px;border-radius:8px;box-shadow:0 4px 6px rgba(0,0,0,0.1);text-align:center;width:320px;transition:background 0.3s}");
    html += F("h2{color:var(--fg);margin-bottom:20px}");
    html += F(".login-field{position:relative;margin:10px 0}");
    html += F(".login-field input{width:100%;padding:12px 44px 12px 12px;border:1px solid var(--brd);border-radius:4px;font-size:15px;background:var(--inp);color:var(--fg);box-sizing:border-box;margin:0}");
    html += F(".login-field input:focus{outline:none;border-color:var(--btn)}");
    html += F(".login-field button{position:absolute;right:10px;top:50%;transform:translateY(-50%);background:transparent!important;border:none!important;outline:none!important;box-shadow:none!important;padding:0;margin:0;cursor:pointer;font-size:18px;line-height:1;display:flex;align-items:center;justify-content:center;width:28px;height:28px;color:var(--fg);z-index:10;-webkit-tap-highlight-color:transparent;appearance:none}");
    html += F(".login-field button:focus{outline:none!important;box-shadow:none!important;background:transparent!important}");
    html += F("input[type=text]{width:100%;padding:12px;margin:10px 0;border:1px solid var(--brd);border-radius:4px;font-size:15px;background:var(--inp);color:var(--fg);box-sizing:border-box}");
    html += F("input[type=text]:focus{outline:none;border-color:var(--btn)}");
    html += F("button{width:100%;padding:12px;background:var(--btn);color:white;border:none;border-radius:4px;cursor:pointer;font-size:16px;margin-top:10px}");
    html += F("button:hover{background:var(--btn-hover)}");
    html += F("a{color:var(--btn);text-decoration:none;display:inline-block;margin-top:15px}");
    html += F("a:hover{text-decoration:underline}");
    html += F("</style>");
    html += getThemeScript(config.darkMode); // Automatyczne ustawienie motywu
    html += F("</head><body>");
    html += F("<div class='login-box'>");
    html += F("<h2>🔐 Logowanie</h2>");
    html += F("<form action='/login' method='POST'>");
    html += F("<input type='text' name='user' placeholder='Nazwa użytkownika' required autofocus>");
    html += F("<div class='login-field'>");
    html += F("<input type='password' id='loginpass' name='pass' placeholder='Hasło' required>");
    html += F("<button type='button' onclick=\"togglePassword('loginpass')\">👁️</button>");
    html += F("</div>");
    html += F("<button type='submit'>Zaloguj</button>");
    html += F("</form>");
    html += F("<script>function togglePassword(id){var x=document.getElementById(id);var btn=x.parentElement.querySelector('button');x.type=(x.type==='password')?'text':'password';if(btn)btn.textContent=x.type==='password'?'👁️':'👁️‍🗨️';if(x){x.focus();if(x.value){x.setSelectionRange(x.value.length,x.value.length);}}}</script>");
    html += F("<br><a href='/'>← Powrót do strony głównej</a>");
    html += F("</div></body></html>");

    server.send(200, "text/html; charset=utf-8", html);
    DIAG_PRINTLN(F("========== handleLoginPage END ==========\n"));
}

// Weryfikacja logowania i ustawienie cookie
void handleLoginSubmit()
{
    DIAG_PRINTLN(F("\n========== handleLoginSubmit START =========="));

    if (!server.hasArg("user") || !server.hasArg("pass"))
    {
        DIAG_PRINTLN(F("[LOGIN] Missing username or password"));
        server.send(400, "text/html; charset=utf-8",
                    "<html><body style='font-family:Arial;text-align:center;padding:50px'>"
                    "<h2>Błąd</h2><p>Brak danych logowania.</p>"
                    "<a href='/login'>Spróbuj ponownie</a></body></html>");
        return;
    }

    String user = server.arg("user");
    String pass = server.arg("pass");

    DIAG_PRINT(F("[LOGIN] Login attempt - user: "));
    DIAG_PRINTLN(user);

    // Weryfikacja credentials
    if (user == config.adminUser && pass == config.adminPass)
    {
        DIAG_PRINTLN(F("[LOGIN] ✓ Credentials VALID - creating session"));

        // Generuj nowy token sesji
        sessionToken = generateToken();
        isSessionActive = true;
        lastSessionActivity = millis();

        DIAG_PRINT(F("[LOGIN] Generated session token: "));
        DIAG_PRINTLN(sessionToken);

        // Ustaw ciasteczko w przeglądarce
        String cookieValue = String(COOKIE_NAME) + "=" + sessionToken + "; Path=/; HttpOnly; Max-Age=3600";
        server.sendHeader("Set-Cookie", cookieValue);
        redirectTo(server, "/config");

        DIAG_PRINTLN(F("[LOGIN] Cookie set, redirecting to /config"));
        DIAG_PRINTLN(F("========== handleLoginSubmit END (success) ==========\n"));
    }
    else
    {
        DIAG_PRINTLN(F("[LOGIN] ✗ Credentials INVALID"));

        // Błędne dane - użyj zunifikowanej funkcji błędu
        sendErrorPage(server, "❌ Błąd logowania",
                      "Nieprawidłowa nazwa użytkownika lub hasło.",
                      "/login", "Spróbuj ponownie",
                      config.darkMode);

        DIAG_PRINTLN(F("========== handleLoginSubmit END (failed) ==========\n"));
    }
}
