// Simuliertes Sound Mini (Firmware ESP32_Sound_Mini_S3, S3 und C3).
// Startwerte aller Antworten kommen aus dem Code (build.py fuehrt
// buildConfigJson(), handleApiDebug() und handleApiSoundFiles() trocken aus),
// Pins aus pins.h. Hier steht nur, was sich zur Laufzeit aendert.
PST.register('mock_sound', function (variant, ctx) {
  var K = ctx.variant.consts || {};
  var M = PST.autoMock(ctx, { alias: { wifi_always_on: 'always_on', wifi_auto_enable: 'auto_enable', wifi_auto_timeout: 'auto_timeout' } });
  var C = M.get('/api/config');
  C.version = ctx.variant.version;
  C.settings.storage_total = 1441792;
  C.settings.storage_ready = 1;
  var boot = { funk: C.funk.mode, portb: C.settings.portb_role, portc: C.settings.portc_role };
  var apActive = true, ledGrb = K.STATUS_LED_RGB_DEFAULT ? 0 : 1;
  var snd = PST.soundCore({ C: C, store: 'Flash', total: C.settings.storage_total, version: C.version,
    sport: function () { return boot.portb === 2 || boot.portc === 2; } });
  var common = PST.common({
    name: function () { return C.wifi.device; }, addr: function () { return C.settings.modul_adr; },
    funkRole: function () { return boot.funk === 1 ? 'Zentrale' : (boot.funk === 2 ? 'Teilnehmer' : null); },
    funkCfg: function () { return { id: C.funk.id, ch: C.funk.ch, pwr: C.funk.pwr, rate: C.funk.rate, back: !!C.funk.back }; },
    backupData: function () { return C; }
  });
  function mem() {
    var c3 = !K.HAS_PORT_C;
    return { app_used: 1180000, app_size: 1310720, fs_used: snd.used(), fs_size: C.settings.storage_total,
      heap_free: c3 ? 150000 : 210000, heap_min: c3 ? 120000 : 180000, heap_size: c3 ? 280000 : 330000,
      psram_free: c3 ? 0 : 2090000, psram_size: c3 ? 0 : 2097152 };
  }
  function debug() {
    var d = JSON.parse(JSON.stringify(M.get('/api/debug'))), t = Date.now() / 1000;
    d.channels = d.channels.map(function (x, i) { return i === 2 ? 172 + Math.round(800 * (1 + Math.sin(t / 2))) : 992; });
    d.wav = snd.slots.map(function (k, i) { var f = C.sound_files[k]; return { n: snd.labels[i] + ': ' + (f || '(leer)'), e: snd.fsize(f) ? 1 : 0, s: snd.fsize(f) }; });
    d.fsfiles = snd.files().map(function (f) { return { n: '/' + f.name, s: f.size }; });
    d.mem = mem();
    d.portb_role = boot.portb; d.portc_role = boot.portc;
    d.crsf_diag = { raw_bytes: Math.round(t * 1450) % 1e7, valid_frames: Math.round(t * 150) % 1e6, crc_errors: 0, device_pings: 3, param_reads: 41, param_writes: 2 };
    d.wifi = { active: apActive, bus_ok: true };
    if (boot.portb === 2 || boot.portc === 2) { d.sport_active = true; d.sport.total_v = 11.82; d.sport.min_cell = 3.93; d.sport.cells = 3; d.sport.soc = 78;
      d.sport.packs[0] = { online: true, cells: 3, total: 11.82, min: 3.93, cv: [3.95, 3.94, 3.93, 0, 0, 0] }; }
    if (boot.portb === 3 || boot.portc === 3) d.gps = { active: 1, speed: 12.4, sats: 9, fix: 1, baud: 9600, auto: 1, locked: 1, pinConflict: 0 };
    return d;
  }
  var routes = PST.merge({}, snd.routes);
  PST.merge(routes, {
    'GET /api/config': function () { C.settings.storage_used = snd.used(); C.funk.boot = boot.funk; return PST.json(C); },
    'POST /api/config': function (q, b) {
      // Feldnamen wie handleApiConfigPost(): flach, Funk als funk_*
      var d = PST.parseBody(b), rest = {};
      Object.keys(d).forEach(function (k) { var f = /^funk_(.*)$/.exec(k); if (f) C.funk[f[1]] = +d[k]; else rest[k] = d[k]; });
      M.apply(JSON.stringify(rest));
      return PST.json({ ok: true });
    },
    '/api/debug': function () { return PST.json(debug()); },
    '/api/wifi_enable': function (q, b) { var d = PST.parseBody(b); apActive = d.on === true || d.on === 1 || d.on === '1' || d.on === 'true'; return PST.json({ ok: true, on: apActive }); }
  });
  function handle(m, path, q, b) {
    var r = common.handle(m, path, q, b); if (r) return r;
    var h = routes[m + ' ' + path] || routes[path];
    if (h) return h(q, b, m);
    return M.handle(m, path, q, b);
  }
  // ── Lua, Funktionsnamen wie in ESP32_Sound_Mini_S3.ino ──
  var BAUDS = [1, 9600, 19200, 38400, 57600, 115200, 230400, 420000], ROLE_BAUD = C.settings.portb_baud_defaults || [];
  var funkCfg = { get id() { return C.funk.id; }, set id(v) { C.funk.id = v; }, get ch() { return C.funk.ch; }, set ch(v) { C.funk.ch = v; },
    get pwr() { return C.funk.pwr; }, set pwr(v) { C.funk.pwr = v; }, get rate() { return C.funk.rate; }, set rate(v) { C.funk.rate = v; },
    get back() { return !!C.funk.back; }, set back(v) { C.funk.back = v ? 1 : 0; } };
  var MB = 1048576;
  var lua = PST.mergeLua({ get: {}, set: {}, views: {} }, snd.lua, {
    get: {
      luaGetModAdr: function () { return C.settings.modul_adr; }, luaGetGroupAdr: function (a) { return C.settings['mkan_adr' + a]; },
      luaGetPortRole: function (a) { return a ? C.settings.portc_role : C.settings.portb_role; },
      luaGetPortBaud: function (a) { return Math.max(0, BAUDS.indexOf(a ? C.settings.portc_baud : C.settings.portb_baud)); },
      luaGetWifiNow: function () { return apActive ? 1 : 0; }, luaGetWifiAuto: function () { return C.wifi.auto_enable; },
      luaGetWifiTimeout: function () { return C.wifi.auto_timeout; }, luaGetWifiAlways: function () { return C.wifi.always_on; },
      luaGetLedRgb: function () { return ledGrb; }
    },
    set: {
      luaSetModAdr: function (a, v) { C.settings.modul_adr = v; }, luaSetGroupAdr: function (a, v) { C.settings['mkan_adr' + a] = v; },
      luaSetPortRole: function (a, v) { var k = a ? 'portc' : 'portb'; if (v !== C.settings[k + '_role'] && ROLE_BAUD[v] && v !== 4) C.settings[k + '_baud'] = ROLE_BAUD[v]; C.settings[k + '_role'] = v; },
      luaSetPortBaud: function (a, v) { C.settings[(a ? 'portc' : 'portb') + '_baud'] = BAUDS[v]; },
      luaSetWifiNow: function (a, v) { apActive = !!v; }, luaSetWifiAuto: function (a, v) { C.wifi.auto_enable = v; },
      luaSetWifiTimeout: function (a, v) { C.wifi.auto_timeout = v; }, luaSetWifiAlways: function (a, v) { C.wifi.always_on = v; },
      luaSetLedRgb: function (a, v) { ledGrb = v ? 1 : 0; }
    },
    views: {
      luaShowVersion: function (it, v) { v.text = ctx.variant.version + ' Mini ' + (K.BOARD_SHORT || ''); },
      luaShowDeviceName: function (it, v) { v.text = C.wifi.device + ' (Web aendern)'; },
      luaShowMemory: function (it, v) {
        var d = mem();
        v.text = [(d.app_used / MB).toFixed(2) + ' von ' + (d.app_size / MB).toFixed(2) + ' MB (' + Math.round(100 * d.app_used / d.app_size) + '%)',
          (d.fs_used / MB).toFixed(2) + ' von ' + (d.fs_size / MB).toFixed(2) + ' MB (' + Math.round(100 * d.fs_used / d.fs_size) + '%)',
          Math.round(d.heap_free / 1024) + ' KB (min ' + Math.round(d.heap_min / 1024) + ' KB)',
          d.psram_size ? (d.psram_free / MB).toFixed(2) + ' von ' + (d.psram_size / MB).toFixed(2) + ' MB' : 'kein PSRAM'][it.arg];
      },
      luaShowSettingsFolder: function (it, v) { v.name = 'Einstellungen (Adr ' + C.settings.modul_adr + ')'; },
      luaViewPortBaud: function (it, v) { if (!(it.arg ? C.settings.portc_role : C.settings.portb_role)) v.hidden = true; }
    }
  }, PST.funkLua(funkCfg, function () { return C.funk.mode; }, function (v) { C.funk.mode = Math.min(v, 2); }));
  return {
    handle: handle, get: lua.get, set: lua.set, views: lua.views,
    deviceName: function () { return C.wifi.device; },
    reboot: function () { boot.funk = C.funk.mode; boot.portb = C.settings.portb_role; boot.portc = C.settings.portc_role; },
    hw: function () {
      function pin(a, b) { return K[a] == null || K[a] === 255 ? 'nicht vorhanden' : 'RX ' + K[a] + ' / TX ' + K[b]; }
      return [['Board', K.BOARD_NAME || variant], ['Verstärker MAX98357A', 'DIN ' + K.PIN_I2S_DOUT + ', BCLK ' + K.PIN_I2S_BCLK + ', LRC ' + K.PIN_I2S_LRC + ', GAIN ' + K.PIN_MAX_GAIN],
        ['Port A (CRSF)', pin('PIN_CRSF_RX', 'PIN_CRSF_TX')], ['Port B', pin('PIN_PORTB_RX', 'PIN_PORTB_TX')],
        ['Port C', K.HAS_PORT_C ? pin('PIN_PORTC_RX', 'PIN_PORTC_TX') : 'nicht vorhanden'],
        ['Status-LED', 'GPIO ' + K.PIN_STATUS_LED + (K.STATUS_LED_WS2812 ? ' (WS2812)' : '')], ['BOOT-Taste (WLAN)', 'GPIO ' + K.PIN_BOOT_BUTTON], ['Sound-Speicher', 'Flash']];
    }
  };
});
