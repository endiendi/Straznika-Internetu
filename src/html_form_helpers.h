#ifndef HTML_FORM_HELPERS_H
#define HTML_FORM_HELPERS_H

#include <ESP8266WebServer.h>

// ============================================================================
// HELPERY DO GENEROWANIA FORMANTÓW HTML
// ============================================================================

/// Generuje pole czasu z możliwością konwersji jednostek
/// Zwraca HTML dla pola czasu (display + select + hidden)
String generateTimeInput(const String &fieldName, int value, const String &label)
{
    String html = F("<label for='");
    html += fieldName;
    html += F("'>");
    html += label;
    html += F(":</label>");
    html += F("<div class='time-group'>");
    html += F("<input type='number' id='");
    html += fieldName;
    html += F("_disp' step='0.1' min='0' oninput=\"updateHidden('");
    html += fieldName;
    html += F("')\" value='");
    html += value;
    html += F("'>");
    html += F("<select id='");
    html += fieldName;
    html += F("_unit' class='unit-select' onchange=\"convertUnit('");
    html += fieldName;
    html += F("')\">");
    html += F("<option value='1'>ms</option>");
    html += F("<option value='1000'>s</option>");
    html += F("<option value='60000'>min</option>");
    html += F("</select>");
    html += F("</div>");
    html += F("<input type='hidden' id='");
    html += fieldName;
    html += F("' name='");
    html += fieldName;
    html += F("' value='");
    html += value;
    html += F("'>");
    return html;
}

/// Generuje pole czasu z dodatkową walidacją JavaScript
String generateTimeInputWithValidation(const String &fieldName, int value, const String &label, const String &onchange)
{
    String html = F("<label for='");
    html += fieldName;
    html += F("' style='margin-top:10px;'>");
    html += label;
    html += F(": <span style='color:#c00; font-weight:bold;'>*</span></label>");
    html += F("<div class='time-group'>");
    html += F("<input type='number' id='");
    html += fieldName;
    html += F("_disp' step='0.1' min='0' oninput=\"updateHidden('");
    html += fieldName;
    html += F("'); ");
    html += onchange;
    html += F("\" value='");
    html += value;
    html += F("'>");
    html += F("<select id='");
    html += fieldName;
    html += F("_unit' class='unit-select' onchange=\"convertUnit('");
    html += fieldName;
    html += F("'); ");
    html += onchange;
    html += F("\">");
    html += F("<option value='1'>ms</option>");
    html += F("<option value='1000'>s</option>");
    html += F("<option value='60000'>min</option>");
    html += F("</select>");
    html += F("</div>");
    html += F("<input type='hidden' id='");
    html += fieldName;
    html += F("' name='");
    html += fieldName;
    html += F("' value='");
    html += value;
    html += F("'>");
    return html;
}

/// Generuje pole IP z walidacją
String generateIpInput(const String &fieldName, const String &value, const String &label, bool required = true)
{
    String html = F("<label for='");
    html += fieldName;
    html += F("'>");
    html += label;
    html += F(":</label>");
    html += F("<input type='text' id='");
    html += fieldName;
    html += F("' name='");
    html += fieldName;
    html += F("' value='");
    html += value;
    html += F("' pattern='^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$' ");
    html += F("title='Wprowadź poprawny adres IPv4 (np. 192.168.1.1)'");
    if (required)
        html += F(" required");
    html += F(">");
    return html;
}

/// Generuje checkbox
String generateCheckbox(const String &fieldName, bool checked, const String &label, const String &tooltip = "")
{
    String html = F("<div class='switch-wrap' style='justify-content: flex-start; margin-bottom:8px;'>");
    html += F("<label class='switch'>");
    html += F("<input type='checkbox' id='");
    html += fieldName;
    html += F("' name='");
    html += fieldName;
    html += F("'");
    if (checked)
        html += F(" checked");
    html += F(">");
    html += F("<span class='slider'></span>");
    html += F("</label>");
    html += F("<span style='margin-left: 10px;'>");
    html += label;
    if (tooltip.length() > 0)
    {
        html += F(" <span class='tooltip'>?<span class='tooltiptext'>");
        html += tooltip;
        html += F("</span></span>");
    }
    html += F("</span>");
    html += F("</div>");
    return html;
}

/// Generuje pole tekstowe
String generateTextInput(const String &fieldName, const String &value, const String &label,
                         const String &placeholder = "", bool required = false)
{
    String html = F("<label for='");
    html += fieldName;
    html += F("'>");
    html += label;
    html += F(":</label>");
    html += F("<input type='text' id='");
    html += fieldName;
    html += F("' name='");
    html += fieldName;
    html += F("' value='");
    html += value;
    html += F("'");
    if (placeholder.length() > 0)
    {
        html += F(" placeholder='");
        html += placeholder;
        html += F("'");
    }
    if (required)
        html += F(" required");
    html += F(">");
    return html;
}

/// Generuje pole hasła z przyciskiem toggle widoczności
String generatePasswordInput(const String &fieldName, const String &value, const String &label)
{
    String html = F("<label for='");
    html += fieldName;
    html += F("'>");
    html += label;
    html += F(":</label>");
    html += F("<div class='time-group'>");
    html += F("<input type='password' id='");
    html += fieldName;
    html += F("' name='");
    html += fieldName;
    html += F("' value='");
    html += value;
    html += F("'");
    html += F(" placeholder='");
    html += label;
    html += F("'>");
    html += F("<button type='button' onclick=\"togglePassword('");
    html += fieldName;
    html += F("')\">👁️</button>");
    html += F("</div>");
    return html;
}

/// Generuje pole liczby całkowitej
String generateNumberInput(const String &fieldName, int value, const String &label,
                           int minVal = 1, bool required = false)
{
    String html = F("<label for='");
    html += fieldName;
    html += F("'>");
    html += label;
    html += F(":</label>");
    html += F("<input type='number' id='");
    html += fieldName;
    html += F("' name='");
    html += fieldName;
    html += F("' value='");
    html += value;
    html += F("' min='");
    html += minVal;
    html += F("'");
    if (required)
        html += F(" required");
    html += F(">");
    return html;
}

/// Generuje pole select (dropdown)
String generateSelectField(const String &fieldName, const String &selectedValue,
                           const String &label, const String *options, const String *values, int optionCount)
{
    String html = F("<label for='");
    html += fieldName;
    html += F("'>");
    html += label;
    html += F(":</label>");
    html += F("<select id='");
    html += fieldName;
    html += F("' name='");
    html += fieldName;
    html += F("'>");

    for (int i = 0; i < optionCount; i++)
    {
        html += F("<option value='");
        html += values[i];
        html += F("'");
        if (values[i] == selectedValue)
            html += F(" selected");
        html += F(">");
        html += options[i];
        html += F("</option>");
    }

    html += F("</select>");
    return html;
}

/// Generuje przycisk radio
String generateRadioButtons(const String &fieldName, const String &selectedValue,
                            const String &option1, const String &value1,
                            const String &option2, const String &value2)
{
    String html = F("<div style='display:flex; gap:20px; align-items:center; flex-wrap:wrap;'>");

    html += F("<label><input type='radio' name='");
    html += fieldName;
    html += F("' value='");
    html += value1;
    html += F("'");
    if (selectedValue == value1)
        html += F(" checked");
    html += F("> ");
    html += option1;
    html += F("</label>");

    html += F("<label><input type='radio' name='");
    html += fieldName;
    html += F("' value='");
    html += value2;
    html += F("'");
    if (selectedValue == value2)
        html += F(" checked");
    html += F("> ");
    html += option2;
    html += F("</label>");

    html += F("</div>");
    return html;
}

/// Generuje komunikat ostrzeżenia
String generateWarningBox(const String &title, const String &content)
{
    String html = F("<div style='background:#1a3a1a; color:#a8f5a8; padding:12px; border-radius:6px; margin-bottom:15px; border:1px solid #4ade80;'>");
    html += F("<b>⚠️ ");
    html += title;
    html += F(":</b><br>");
    html += content;
    html += F("</div>");
    return html;
}

/// Generuje komunikat informacyjny
String generateInfoBox(const String &content)
{
    String html = F("<div style='background:var(--inp); padding:10px; border:1px solid var(--brd); border-radius:6px; margin-bottom:12px; font-size:0.95em;'>");
    html += content;
    html += F("</div>");
    return html;
}

#endif // HTML_FORM_HELPERS_H
