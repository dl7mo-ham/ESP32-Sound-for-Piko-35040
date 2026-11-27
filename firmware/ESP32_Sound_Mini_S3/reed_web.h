/*
 * reed_web.h  -  Soundmodul Mini
 *
 * Reed-/IR-Web-Oberflaeche: HTML-Fragment (Compile-Time in RCSOUND_HTML
 * eingefuegt, kein RAM- und kein zusaetzlicher Flash-Overhead durch einen
 * zweiten Request), externes JS (/reed_ui.js) und die REST-Routen.
 * Enthaelt auch die LED-Farbfolge (kein Lua-Menue im Reed-Betrieb) und die
 * sofortige Lautstaerke-Uebertragung der vorhandenen Web-Regler, damit
 * WebServerManager.cpp dafuer upstream-identisch bleiben kann.
 *
 * In WebServerManager.cpp stehen dadurch nur noch drei Zeilen: dieses
 * Include, das Makro im HTML und reedWebRegister(server) in begin().
 */

#ifndef REED_WEB_H
#define REED_WEB_H

#include <WebServer.h>

// Reed-/IR-Tab zwischen "Speicher" und "WiFi". Wird per Adjacent-String-
// Concatenation in das PROGMEM-HTML eingefuegt (siehe WebServerManager.cpp).
#define REED_TAB_HTML \
"<!-- ═══ REED-TRIGGER ════════════════════════════════════════════════════ -->\n" \
"<div id=\"tab-reed\" class=\"hidden\">\n" \
"  <div class=\"card\">\n" \
"    <div class=\"card-title\">Aktionen: Reed &amp; IR (NEC)</div>\n" \
"    <div style=\"font-size:12px;color:var(--sub);margin-bottom:10px\">\n" \
"      Jede Zeile ist eine Aktion. Reed-Pins (Rolle &quot;Reed&quot;, Pull-up,\n" \
"      Kontakt gegen GND) und ein gelernter IR-Code koennen dieselbe Aktion\n" \
"      ausloesen. IR-Empfaenger: IO11 (nur S3). IR lernen: Button klicken und\n" \
"      innerhalb von 10 s eine Taste der Fernbedienung druecken.\n" \
"    </div>\n" \
"    <div id=\"reed-restart\" class=\"hidden\" style=\"color:var(--yellow);font-size:12px;margin-bottom:10px\"></div>\n" \
"    <div id=\"ir-status\" class=\"hidden\" style=\"color:var(--yellow);font-size:12px;margin-bottom:10px\"></div>\n" \
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
"  <div class=\"card\">\n" \
"    <div class=\"card-title\">Drehzahl per IR / Reed</div>\n" \
"    <div class=\"sl-wrap\">\n" \
"      <div class=\"sl-head\"><span>Schritt pro Drehzahl +/-</span><span id=\"lv-reed-speed-step\">-</span></div>\n" \
"      <input type=\"range\" id=\"reed-speed-step\" min=\"5\" max=\"50\" step=\"5\" oninput=\"reedSpeedUpd()\">\n" \
"    </div>\n" \
"    <div style=\"font-size:11px;color:var(--sub);margin-bottom:10px\">\n" \
"      Gilt fuer die Aktionen \"Drehzahl +\" und \"Drehzahl -\", wenn kein CRSF-Gaskanal\n" \
"      konfiguriert ist und der Motor eingeschaltet ist. Rampe sowie Drehzahl min/max bleiben aktiv.\n" \
"    </div>\n" \
"    <button class=\"btn btn-p\" onclick=\"saveReed()\">Speichern</button>\n" \
"  </div>\n" \
"  <div class=\"card\">\n" \
"    <div class=\"card-title\">Letzter IR-Befehl (NEC/NEC2)</div>\n" \
"    <div id=\"ir-last\" style=\"font-size:12px;color:var(--sub)\">Noch keiner empfangen</div>\n" \
"  </div>\n" \
"  <div class=\"card\">\n" \
"    <div class=\"card-title\">LED-Farbfolge (Status &amp; Flamme)</div>\n" \
"    <div class=\"row\" style=\"margin-bottom:10px\">\n" \
"      <span class=\"lbl\">Farbfolge der WS2812</span>\n" \
"      <select id=\"led-rgb-order\">\n" \
"        <option value=\"1\">RGB (S3-Zero)</option>\n" \
"        <option value=\"0\">GRB (Standard)</option>\n" \
"      </select>\n" \
"    </div>\n" \
"    <div style=\"font-size:11px;color:var(--sub);margin-bottom:10px\">\n" \
"      Umstellen, wenn die Flamme gruen statt orange leuchtet (Rot/Gruen vertauscht).\n" \
"      Wirkt sofort und wird dauerhaft gespeichert.\n" \
"    </div>\n" \
"    <button class=\"btn btn-p\" onclick=\"saveLedOrder()\">Speichern</button>\n" \
"  </div>\n" \
"</div>\n"

// Registriert /reed_ui.js, /api/reed/test und /api/led-order am Webserver. Wird aus
// WebServerManager::begin() heraus mit dessen (privatem) server-Objekt
// aufgerufen.
void reedWebRegister(WebServer& server);

#endif
