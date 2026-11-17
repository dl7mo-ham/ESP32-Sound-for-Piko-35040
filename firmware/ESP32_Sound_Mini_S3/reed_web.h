/*
 * reed_web.h  -  Soundmodul Mini
 *
 * Reed-Web-Oberflaeche: HTML-Fragment (Compile-Time in RCSOUND_HTML
 * eingefuegt, kein RAM- und kein zusaetzlicher Flash-Overhead durch einen
 * zweiten Request), externes JS (/reed_ui.js) und die REST-Routen.
 *
 * In WebServerManager.cpp stehen dadurch nur noch drei Zeilen: dieses
 * Include, das Makro im HTML und reedWebRegister(server) in begin().
 */

#ifndef REED_WEB_H
#define REED_WEB_H

#include <WebServer.h>

// Reed-Tab zwischen "Speicher" und "WiFi". Wird per Adjacent-String-
// Concatenation in das PROGMEM-HTML eingefuegt (siehe WebServerManager.cpp).
#define REED_TAB_HTML \
"<!-- ═══ REED-TRIGGER ════════════════════════════════════════════════════ -->\n" \
"<div id=\"tab-reed\" class=\"hidden\">\n" \
"  <div class=\"card\">\n" \
"    <div class=\"card-title\">Reed-Trigger</div>\n" \
"    <div style=\"font-size:12px;color:var(--sub);margin-bottom:10px\">\n" \
"      Ist ein Port auf die Rolle &quot;Reed&quot; gestellt, werden beide Pins als\n" \
"      Eingaenge mit Pull-up gelesen. Ein Impuls (Kontakt gegen GND) loest die\n" \
"      hier gewaehlte Aktion aus - genau wie der Testbutton daneben.\n" \
"    </div>\n" \
"    <div id=\"reed-restart\" class=\"hidden\" style=\"color:var(--yellow);font-size:12px;margin-bottom:10px\"></div>\n" \
"    <div id=\"reed-ports\"></div>\n" \
"    <div id=\"reed-empty\" class=\"hidden\" style=\"font-size:12px;color:var(--sub)\">\n" \
"      Noch kein Port auf &quot;Reed&quot; gestellt. Unter Einstellungen bei Port A/B/C\n" \
"      die Rolle &quot;Reed&quot; waehlen und das Modul neu starten.\n" \
"    </div>\n" \
"  </div>\n" \
"  <div class=\"card\">\n" \
"    <div class=\"card-title\">Entprellzeit</div>\n" \
"    <div class=\"sl-wrap\">\n" \
"      <div class=\"sl-head\"><span>Stabilzeit pro Impuls</span><span id=\"lv-reed-db\">-</span></div>\n" \
"      <input type=\"range\" id=\"reed-db\" min=\"5\" max=\"500\" step=\"5\" oninput=\"slUpd('reed-db','lv-reed-db')\">\n" \
"    </div>\n" \
"    <div style=\"font-size:11px;color:var(--sub);margin-bottom:10px\">\n" \
"      Kuerzer reagiert schneller, laenger unterdrueckt Prellen und Stoerungen.\n" \
"    </div>\n" \
"    <button class=\"btn btn-p\" onclick=\"saveReed()\">Speichern</button>\n" \
"  </div>\n" \
"</div>\n"

// Registriert /reed_ui.js und /api/reed/test am Webserver. Wird aus
// WebServerManager::begin() heraus mit dessen (privatem) server-Objekt
// aufgerufen.
void reedWebRegister(WebServer& server);

#endif
