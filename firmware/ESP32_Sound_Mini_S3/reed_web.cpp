/*
 * reed_web.cpp  -  Soundmodul Mini
 *
 * Reed-Web-Oberflaeche (siehe reed_web.h): das JS liegt als PROGMEM hinter
 * /reed_ui.js, der Testbutton-Handler hier. Beides war vorher mitten in
 * WebServerManager.cpp und ist jetzt projektlokal.
 */

#include "reed_web.h"
#include <Arduino.h>
#include "config_reed.h"     // reedJsonGetInt()
#include "command_queue.h"   // enqueueCommand()
#include "reed_trigger.h"    // reedActionClamp()/Name()

// Das externe Script wird VOR dem Haupt-<script> geladen. Die hier
// definierten Funktionen greifen auf cfg/toast/xpost/fillSelNames/gi und
// loadConfig() zu -globale Namen aus WebServerManager.cpp, die zum
// Zeitpunkt des Aufrufs (Tab-Klick bzw. Config-Laden) alle initialisiert
// sind.
static const char REED_UI_JS[] PROGMEM = R"JS(
// ─── Reed-Trigger (v0.14) ────────────────────────────────────────────────
// Aktionen/Entprellzeit kommen aus cfg.reed (GET /api/config). Die Testbuttons
// rufen POST /api/reed/test auf - derselbe Kommando-Queue-Pfad wie ein echter
// Reed-Impuls.
function reedActionLabel(action){
  var names=((cfg.reed&&cfg.reed.action_options)||'').split(';');
  return names[action]||('Aktion '+action);
}
// ─── Sofortige Lautstaerke (v0.13+dl7mo.8) ───────────────────────────────
// Frueher als Fork-Diff mitten in WebServerManager.cpp: zwei onchange-
// Attribute, liveVol() und ein geaendertes testSound(). Das Backend
// /api/sound akzeptiert {id,vol} bereits - hier werden nur die vorhandenen
// Regler mit Listenern versehen. Dieses Script laeuft vor dem Haupt-<script>;
// cfg/curSnd/xpost werden erst beim Event aufgeloest (siehe Dateikommentar).
function appLiveVol(id,v){
  v=parseInt(v,10)||0;
  if(id===0){ if(cfg.motor) cfg.motor.vol=v; }
  else if(cfg.sounds&&cfg.sounds[id-1]) cfg.sounds[id-1].vol=v;
  xpost('/api/sound',{id:id,vol:v});
}
(function attachLiveVolume(){
  var m=document.getElementById('m-vol');
  var s=document.getElementById('s-vol');
  if(m) m.addEventListener('change',function(){ appLiveVol(0,this.value); });
  if(s) s.addEventListener('change',function(){ appLiveVol(curSnd,this.value); });
})();
function renderReed(){
  var r=cfg.reed;
  var box=document.getElementById('reed-ports');
  var empty=document.getElementById('reed-empty');
  var restart=document.getElementById('reed-restart');
  if(!box) return;
  box.innerHTML='';
  if(!r){
    if(empty) empty.className='';
    if(restart) restart.className='hidden';
    return;
  }
  var db=document.getElementById('reed-db');
  if(db){
    db.min=r.debounce_min||5; db.max=r.debounce_max||500;
    db.value=r.debounce_ms;
    var lv=document.getElementById('lv-reed-db');
    if(lv) lv.textContent=r.debounce_ms+' ms';
  }
  var shown=0, changed=[];
  (r.ports||[]).forEach(function(p){
    if(!p.present) return;
    if(!!p.configured!==!!p.active) changed.push(p.label);
    if(!p.configured) return;
    shown++;
    var head=document.createElement('div');
    head.style.cssText='font-weight:600;font-size:11px;color:var(--sub);text-transform:uppercase;letter-spacing:.05em;margin:10px 0 6px';
    head.textContent=p.label;
    box.appendChild(head);
    ['rx','tx'].forEach(function(pin){
      var row=document.createElement('div'); row.className='row';
      var lab=document.createElement('span'); lab.className='lbl';
      lab.style.cssText='flex:0 0 86px;font-size:12px';
      var pinNo=p.pins?p.pins[pin]:undefined;
      lab.textContent=pin.toUpperCase()+(pinNo!==undefined&&pinNo<255?' (IO'+pinNo+')':'');
      row.appendChild(lab);
      var sel=document.createElement('select');
      sel.id='reed-'+p.key+'-'+pin;
      fillSelNames(sel,r.action_options,null);
      sel.value=(r.actions&&r.actions[p.key+'_'+pin]!==undefined)?r.actions[p.key+'_'+pin]:0;
      row.appendChild(sel);
      var btn=document.createElement('button');
      btn.className='btn btn-s'; btn.textContent='Test';
      btn.style.cssText='width:auto;padding:6px 12px;margin:0;flex:0 0 auto;font-size:12px';
      btn.onclick=function(){
        var a=parseInt(sel.value)||0;
        testReed(a,reedActionLabel(a));
      };
      row.appendChild(btn);
      box.appendChild(row);
    });
  });
  if(empty) empty.className=shown?'hidden':'';
  if(restart){
    if(changed.length){
      restart.textContent='\u26A0 '+changed.join(', ')+': Rollenaenderung wirkt erst nach einem Neustart des Moduls.';
      restart.className='';
    } else {
      restart.className='hidden';
    }
  }
  loadLedOrder();
}
function testReed(action){
  if(!action){ toast('Keine Aktion gewaehlt',1); return; }
  xpost('/api/reed/test',{action:action},function(resp){
    var d=null; try{ d=JSON.parse(resp); }catch(e){}
    if(d&&d.ok) toast('Test: '+reedActionLabel(action));
    else toast('Test fehlgeschlagen',1);
  });
}
function saveReed(){
  var d={reed_debounce_ms:gi('reed-db')};
  var r=cfg.reed||{};
  (r.ports||[]).forEach(function(p){
    if(!p.present||!p.configured) return;
    d['reed_'+p.key+'_rx']=gi('reed-'+p.key+'-rx');
    d['reed_'+p.key+'_tx']=gi('reed-'+p.key+'-tx');
  });
  xpost('/api/config',d,function(resp){
    try{var q=JSON.parse(resp);if(q.ok){ loadConfig(); toast('Reed-Einstellungen gespeichert');return;}}catch(e){}
    toast('Fehler',1);
  });
}
// LED-Farbfolge: Im Reed-Betrieb gibt es kein Lua-Menue, daher hier im Web-Tab.
function loadLedOrder(){
  fetch('/api/led-order').then(function(r){return r.json();}).then(function(d){
    var s=document.getElementById('led-rgb-order');
    if(s&&d&&typeof d.rgb!=='undefined') s.value=d.rgb?'1':'0';
  }).catch(function(){});
}
function saveLedOrder(){
  var s=document.getElementById('led-rgb-order'); if(!s) return;
  xpost('/api/led-order',{rgb:parseInt(s.value,10)},function(resp){
    var d=null; try{ d=JSON.parse(resp); }catch(e){}
    if(d&&d.ok) toast('LED-Farbfolge gespeichert'); else toast('Fehler',1);
  });
}
)JS";

static WebServer* reedServer = nullptr;

static void reedSendOk(WebServer& sv)  { sv.send(200, "application/json", "{\"ok\":true}"); }
static void reedSendErr(WebServer& sv) { sv.send(400, "application/json", "{\"error\":\"bad request\"}"); }

// ── GET/POST /api/led-order  {"rgb":bool} ─────────────────────────────────
// Dieselbe Einstellung wie im Lua-Menue (config.LedRgb). updateLed() in der
// .ino uebernimmt den Wert im naechsten Loop, ein Neustart ist nicht noetig.
static void reedWebHandleLedOrderGet(WebServer& sv) {
  char buf[32];
  snprintf(buf, sizeof(buf), "{\"rgb\":%s}", config.LedRgb ? "true" : "false");
  sv.send(200, "application/json", buf);
}
static void reedWebHandleLedOrderPost(WebServer& sv) {
  if (!sv.hasArg("plain")) { reedSendErr(sv); return; }
  int v;
  if (!reedJsonGetInt(sv.arg("plain"), "rgb", v) || (v != 0 && v != 1)) { reedSendErr(sv); return; }
  bool rgb = (v == 1);
  if (config.LedRgb != rgb) { config.LedRgb = rgb; markDirty(); }
  reedSendOk(sv);
}

// ── POST /api/reed/test  {"action":1..6} ──────────────────────────────────
// Testbutton der Web-UI. Prueft die Aktion nur und legt sie in die Queue;
// ausgefuehrt wird sie - wie ein echter Reed-Impuls - in loop() ueber den
// gemeinsamen Command-Executor (ESP32_Sound_Mini_S3.ino).
static void reedWebHandleTest(WebServer& sv) {
  if (!sv.hasArg("plain")) { reedSendErr(sv); return; }
  const String& body = sv.arg("plain");
  int v;
  if (!reedJsonGetInt(body, "action", v)) { reedSendErr(sv); return; }
  uint8_t action = reedActionClamp(v);
  if (action == REED_ACTION_NONE) { reedSendErr(sv); return; }
  enqueueCommand(action);
  Serial.printf("[API] Reed-Test -> %s\n", reedActionName(action));
  reedSendOk(sv);
}

void reedWebRegister(WebServer& server) {
  reedServer = &server;
  server.on("/reed_ui.js", HTTP_GET, []() {
    reedServer->send_P(200, "application/javascript", REED_UI_JS);
  });
  server.on("/api/reed/test", HTTP_POST, []() { reedWebHandleTest(*reedServer); });
  server.on("/api/led-order", HTTP_GET,  []() { reedWebHandleLedOrderGet(*reedServer); });
  server.on("/api/led-order", HTTP_POST, []() { reedWebHandleLedOrderPost(*reedServer); });
}
