/*
 * reed_web.cpp  -  Soundmodul Mini
 *
 * Reed-/IR-Web-Oberflaeche (siehe reed_web.h): das JS liegt als PROGMEM hinter
 * /reed_ui.js, die REST-Handler hier. Beides war vorher mitten in
 * WebServerManager.cpp und ist jetzt projektlokal.
 *
 * Die Tabelle ist action-centric: Zeile = Aktion, Spalten = Reed-Pins und
 * IR-Code. Intern bleibt die Reed-Zuordnung unverändert "Pin -> Aktion"
 * (NVS "reedact"), die IR-Zuordnung ist "Code -> Aktion" (NVS "ircode").
 */

#include "reed_web.h"
#include <Arduino.h>
#include "config_reed.h"     // reedJsonGetInt()
#include "command_queue.h"   // enqueueCommand()
#include "reed_trigger.h"    // reedActionClamp()/Name()
#include "ir_control.h"      // Lernmodus/IR-REST

// Das externe Script wird VOR dem Haupt-<script> geladen. Die hier
// definierten Funktionen greifen auf cfg/toast/xpost/fillSelNames/gi und
// loadConfig() zu - globale Namen aus WebServerManager.cpp, die zum
// Zeitpunkt des Aufrufs (Tab-Klick bzw. Config-Laden) alle initialisiert
// sind.
static const char REED_UI_JS[] PROGMEM = R"JS(
// ─── Aktionen: Reed-Pins + IR-Code (dl7mo.11) ─────────────────────────────
// cfg.reed und cfg.ir kommen aus GET /api/config. Reed-Test und IR-Trigger
// laufen durch denselben Kommando-Queue-Pfad wie ein echter Impuls.
function reedActionLabel(action){
  var names=((cfg.reed&&cfg.reed.action_options)||'').split(';');
  return names[action]||('Aktion '+action);
}
function irCodeText(proto,addr,cmd){
  if(!proto) return 'nicht belegt';
  return 'NEC'+(proto===2?'2':'')+' 0x'+('0000'+(addr||0).toString(16)).slice(-4)+
         ' / 0x'+('0'+(cmd||0).toString(16)).slice(-2);
}
function irAvail(){ return !!(cfg.ir&&cfg.ir.available); }
function reedSpeedUpd(){
  var e=document.getElementById('lv-reed-speed-step');
  if(e) e.textContent=gi('reed-speed-step')+' %';
}
function irSetStatus(text){
  var e=document.getElementById('ir-status');
  if(!e) return;
  e.textContent=text||'';
  e.className=text?'':'hidden';
}
var irPollTimer=null;
var irLastTmr=null;
function irRenderLast(d){
  var e=document.getElementById('ir-last');
  if(!e||!d) return;
  if(!(d.last_protocol>0)){
    e.textContent=irAvail()?'Noch kein NEC/NEC2-Befehl empfangen':'IR nicht verfuegbar (C3)';
    e.style.color='var(--sub)';
    return;
  }
  var txt=irCodeText(d.last_protocol,d.last_address,d.last_command);
  if(d.last_action>0) txt+=' \u2192 '+reedActionLabel(d.last_action);
  else txt+=' (ohne Zuordnung)';
  if(typeof d.last_age_ms==='number' && d.last_age_ms<60000)
    txt+=' \u00b7 vor '+Math.round(d.last_age_ms/1000)+' s';
  e.textContent=txt; e.style.color='var(--txt)';
}
function irPollLast(){
  var tab=document.getElementById('tab-reed');
  if(!tab || tab.className.indexOf('hidden')>=0){
    if(irLastTmr){ clearInterval(irLastTmr); irLastTmr=0; }
    return;
  }
  fetch('/api/ir/status').then(function(r){return r.json();}).then(irRenderLast).catch(function(){});
}
function irPollLearn(action){
  if(irPollTimer){ clearTimeout(irPollTimer); irPollTimer=null; }
  fetch('/api/ir/status').then(function(r){return r.json();}).then(function(d){
    if(!d) return;
    irRenderLast(d);
    if(d.active){
      var left=Math.ceil((d.remaining_ms||0)/1000);
      irSetStatus('Warte auf IR-Taste fuer '+reedActionLabel(action)+' ... '+left+' s');
      irPollTimer=setTimeout(function(){irPollLearn(action);},300);
      return;
    }
    irSetStatus('');
    if(d.result_action===action && d.result_protocol>0){
      toast('IR-Command gelernt: '+irCodeText(d.result_protocol,d.result_address,d.result_command));
      loadConfig();
    } else {
      toast('Kein IR-Command erkannt (Timeout)',1);
    }
  }).catch(function(){ irSetStatus(''); toast('IR-Status nicht erreichbar',1); });
}
function irLearn(action){
  if(!irAvail()){ toast('IR nur auf dem S3 (IO11)',1); return; }
  xpost('/api/ir/learn',{action:action},function(resp){
    var d=null; try{ d=JSON.parse(resp); }catch(e){}
    if(d&&d.ok){ irSetStatus('Warte auf IR-Taste ...'); irPollLearn(action); }
    else toast('IR-Lernen nicht gestartet',1);
  });
}
function irClear(action){
  xpost('/api/ir/clear',{action:action},function(resp){
    var d=null; try{ d=JSON.parse(resp); }catch(e){}
    if(d&&d.ok){ toast('IR-Command geloescht'); loadConfig(); }
    else toast('Fehler',1);
  });
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
  var r=cfg.reed||{}, ir=cfg.ir||{};
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
  var ss=document.getElementById('reed-speed-step');
  if(ss){
    ss.min=r.speed_step_min||5; ss.max=r.speed_step_max||50;
    ss.value=r.speed_step||20;
    reedSpeedUpd();
  }
  if(!irLastTmr && document.getElementById('tab-reed') &&
     document.getElementById('tab-reed').className.indexOf('hidden')<0){
    irPollLast(); irLastTmr=setInterval(irPollLast,1000);
  }
  var changed=[], pins=[];
  (r.ports||[]).forEach(function(p){
    if(!p.present) return;
    if(!!p.configured!==!!p.active) changed.push(p.label);
    if(!p.configured) return;
    ['rx','tx'].forEach(function(pin){
      var no=p.pins?p.pins[pin]:undefined;
      var short=p.label.replace('Port ','')+'-'+pin.toUpperCase();
      pins.push({key:p.key,pin:pin,label:short+(no!==undefined&&no<255?' IO'+no:'')});
    });
  });
  // Kopfzeile
  var head=document.createElement('div'); head.className='row';
  head.style.cssText='font-size:11px;color:var(--sub);border-bottom:1px solid var(--sub);padding-bottom:4px';
  var c0=document.createElement('span'); c0.className='lbl';
  c0.style.cssText='flex:1 1 120px'; c0.textContent='Aktion';
  head.appendChild(c0);
  pins.forEach(function(pn){
    var c=document.createElement('span');
    c.style.cssText='flex:0 0 42px;text-align:center;font-size:10px'; c.textContent=pn.label;
    head.appendChild(c);
  });
  var ci=document.createElement('span');
  ci.style.cssText='flex:1 1 200px;font-size:10px'; ci.textContent='IR-Code (IO11)';
  head.appendChild(ci);
  box.appendChild(head);

  var names=((r.action_options||'').split(';'));
  var codes=ir.codes||[];
  for(var a=0;a<names.length;a++){
    var row=document.createElement('div'); row.className='row';
    row.style.cssText='align-items:center;border-bottom:1px solid rgba(255,255,255,.06);padding:3px 0';
    var lab=document.createElement('span'); lab.className='lbl';
    lab.style.cssText='flex:1 1 120px;font-size:12px'; lab.textContent=names[a];
    if(a>0){
      var tb=document.createElement('button'); tb.className='btn btn-s'; tb.textContent='Test';
      tb.style.cssText='width:auto;padding:3px 7px;margin:0;font-size:11px';
      tb.onclick=(function(act){return function(){testReed(act);};})(a);
      lab.appendChild(document.createTextNode(' ')); lab.appendChild(tb);
    }
    row.appendChild(lab);
    pins.forEach(function(pn){
      var cell=document.createElement('span');
      cell.style.cssText='flex:0 0 42px;text-align:center';
      var rad=document.createElement('input'); rad.type='radio';
      rad.name='reedpin_'+pn.key+'_'+pn.pin; rad.value=String(a);
      var cur=(r.actions&&r.actions[pn.key+'_'+pn.pin]!==undefined)?r.actions[pn.key+'_'+pn.pin]:0;
      rad.checked=(cur===a); rad.title=names[a];
      cell.appendChild(rad); row.appendChild(cell);
    });
    var code=null;
    for(var k=0;k<codes.length;k++) if(codes[k].action===a) code=codes[k];
    var ic=document.createElement('span');
    ic.style.cssText='flex:1 1 200px;font-size:11px;display:flex;gap:4px;align-items:center;flex-wrap:wrap';
    var txt=document.createElement('span');
    txt.style.cssText='min-width:104px;color:'+((code&&code.protocol)?'var(--txt)':'var(--sub)');
    txt.textContent=(a===0?'\u2014':irCodeText(code&&code.protocol,code&&code.address,code&&code.command));
    ic.appendChild(txt);
    if(a>0){
      var lb=document.createElement('button'); lb.className='btn btn-s'; lb.textContent='IR lernen';
      lb.style.cssText='width:auto;padding:3px 7px;margin:0;font-size:11px'; lb.disabled=!irAvail();
      lb.onclick=(function(act){return function(){irLearn(act);};})(a);
      ic.appendChild(lb);
      var db2=document.createElement('button'); db2.className='btn btn-s'; db2.textContent='L\u00f6schen';
      db2.style.cssText='width:auto;padding:3px 7px;margin:0;font-size:11px';
      db2.disabled=!(code&&code.protocol);
      db2.onclick=(function(act){return function(){irClear(act);};})(a);
      ic.appendChild(db2);
    }
    row.appendChild(ic);
    box.appendChild(row);
  }
  if(empty){
    empty.textContent='Kein Port auf "Reed" gestellt - die Reed-Spalten sind leer. IR-Codes lassen sich trotzdem lernen.';
    empty.className=pins.length?'hidden':'';
  }
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
  var d={reed_debounce_ms:gi('reed-db'),reed_speed_step:gi('reed-speed-step')};
  var r=cfg.reed||{};
  (r.ports||[]).forEach(function(p){
    if(!p.present||!p.configured) return;
    ['rx','tx'].forEach(function(pin){
      var sel=document.getElementsByName('reedpin_'+p.key+'_'+pin);
      for(var i=0;i<sel.length;i++){
        if(sel[i].checked){ d['reed_'+p.key+'_'+pin]=sel[i].value; break; }
      }
    });
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

// ── IR-Lernen (dl7mo.11) ──────────────────────────────────────────────────
// POST /api/ir/learn  {"action":1..6}  startet das 10-s-Fenster, action=0
// bricht ab. POST /api/ir/clear {"action":n} entfernt einen gelernten Code.
// GET  /api/ir/status liefert Lernzustand, das letzte Lernergebnis und den
// letzten empfangenen NEC/NEC2-Frame (Proto/Adresse/Command/Aktion/Alter).
static void reedWebHandleIrLearn(WebServer& sv) {
  if (!sv.hasArg("plain")) { reedSendErr(sv); return; }
  int v;
  if (!reedJsonGetInt(sv.arg("plain"), "action", v) || v < 0 || v >= REED_ACTION_COUNT) {
    reedSendErr(sv); return;
  }
  if (v == 0) irLearnCancel(); else irLearnStart((uint8_t)v);
  reedSendOk(sv);
}

static void reedWebHandleIrClear(WebServer& sv) {
  if (!sv.hasArg("plain")) { reedSendErr(sv); return; }
  int v;
  if (!reedJsonGetInt(sv.arg("plain"), "action", v) || v <= 0 || v >= REED_ACTION_COUNT) {
    reedSendErr(sv); return;
  }
  irClearCode((uint8_t)v);
  reedSendOk(sv);
}

static void reedWebHandleIrStatus(WebServer& sv) {
  irLearnTick();
  IrCode res = irLearnResultCode();
  IrCode rx;
  uint8_t rxAction;
  uint32_t rxAge;
  irLastReceived(rx, rxAction, rxAge);
  char buf[320];
  snprintf(buf, sizeof(buf),
           "{\"active\":%s,\"action\":%d,\"remaining_ms\":%lu,"
           "\"result_action\":%d,\"result_protocol\":%d,"
           "\"result_address\":%u,\"result_command\":%u,"
           "\"last_protocol\":%d,\"last_address\":%u,\"last_command\":%u,"
           "\"last_action\":%d,\"last_age_ms\":%lu}",
           irLearnActive() ? "true" : "false", (int)irLearnAction(),
           (unsigned long)irLearnRemainingMs(),
           (int)irLearnResultAction(), (int)res.protocol,
           (unsigned)res.address, (unsigned)res.command,
           (int)rx.protocol, (unsigned)rx.address, (unsigned)rx.command,
           (int)rxAction, (unsigned long)rxAge);
  sv.send(200, "application/json", buf);
}

void reedWebRegister(WebServer& server) {
  reedServer = &server;
  server.on("/reed_ui.js", HTTP_GET, []() {
    reedServer->send_P(200, "application/javascript", REED_UI_JS);
  });
  server.on("/api/reed/test", HTTP_POST, []() { reedWebHandleTest(*reedServer); });
  server.on("/api/led-order", HTTP_GET,  []() { reedWebHandleLedOrderGet(*reedServer); });
  server.on("/api/led-order", HTTP_POST, []() { reedWebHandleLedOrderPost(*reedServer); });
  server.on("/api/ir/learn",  HTTP_POST, []() { reedWebHandleIrLearn(*reedServer); });
  server.on("/api/ir/clear",  HTTP_POST, []() { reedWebHandleIrClear(*reedServer); });
  server.on("/api/ir/status", HTTP_GET,  []() { reedWebHandleIrStatus(*reedServer); });
}
