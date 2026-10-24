/* BZ_Trader — Traveling_Trader on Wheels · editor · NUCLEO (logica pura, sin DOM)
 *
 * Lo usan la pagina (editor_bz_trader.html) y las pruebas automaticas (con Node).
 * Todo lo que escribe un archivo pasa por aca: el servicio, la tripulacion en la ruta y la zona nueva.
 *
 *  - EL SERVICIO ES EL ARCHIVO: el modelo del editor es el mismo objeto que lee el mod (profiles\BZ_Trader\<x>.json).
 *    Lo que el editor no toca queda como estaba (reabrir y exportar sin cambios = el mismo contenido).
 *  - LA RUTA SE ESCRIBE COMO TEXTO: solo la linea DriverClass y el bloque Crew, con el mismo fin de linea y la misma
 *    sangria. Nunca se reserializa (un JSON de ruta en una sola linea tumba el server; el master pesa 170-260 KB).
 *  - EL RUMBO ES EL DE DAYZ: HeadingDeg = la columna 15 del _wp.csv (0 = norte, sentido horario). Adelante =
 *    (sin h, cos h), derecha = (cos h, -sin h) en (x, z).
 */
(function (raiz, fabrica) {
  var n = fabrica();
  if (typeof module === "object" && module.exports) module.exports = n;
  else raiz.BZTN = n;
})(this, function () {
"use strict";

// los textos estan en bzt_i18n.js, ingles por defecto y espanol con idioma("es"). La pagina lo carga
// antes que este archivo; con Node se pide con require. Los mensajes del RPT no se traducen (no pasan por aca).
var I18N = (typeof module === "object" && module.exports) ? require("./bzt_i18n.js") : (typeof window !== "undefined" ? window.BZTI18N : null);
var LANG = "en";
function idioma(l) { if (l === "en" || l === "es") LANG = l; return LANG; }
function T(k) { return I18N ? I18N.t(LANG, k, Array.prototype.slice.call(arguments, 1)) : k; }

// = BZTRDef (scripts/4_World/BZTraderConfig.c). Un validador del build verifica que sean iguales.
var DEF = {
  LOOTING: "BANDAGES | FOOD | CLOTHING | UPGRADE",
  RESPAWN_S: 600, START_DELAY_S: 0,
  WAVE_RADIUS_M: 15, WAVE_COOLDOWN_S: 10, WAVE_TIME_S: 60, WAVE_MEMORY_S: 60, WAVE_MIN_GAP_M: 500, WAVE_MAX_PER_LAP: 3,
  STOP_WAVE_RADIUS_M: 20, BOARD_TIMEOUT_S: 60, BOARD_TIMEOUT_MIN_S: 5, TRADER_TIME_S: 60,
  PLAYER_RADIUS_M: 8, MAX_WITH_PLAYERS_S: 0, SEATED_WAIT_S: 20, PLAYER_HALT_M: 5, ZONE_RADIUS_MIN_M: 5,
  MAX_STALL_DISTANCE_M: 25, MARCA_CADA_S: 30, MARCA_MIN_S: 1, MARCA_QUIETO_M: 1, MARCA_PARPADEO: 1, MARCA_PARPADEO_S: 2,
  SIGNAL_TYPE: "auto", SIGNAL_COLOR: "green", SIGNAL_EVERY_S: 60, SIGNAL_START_DELAY_S: 0, SIGNALS_MAX: 10,
  FRONT_PLAYER_WAIT_S: 0, HORN_ON_DEPART: 0, FACCION: "InvincibleObservers"
};

// = los campos serializables de las clases de BZTraderConfig.c (lo que lee JsonFileLoader). editor_esquema.py lo
// verifica contra el .c, igual que config_real.py.
var ESQUEMA = {
  BZTRServicio: { Enable: "int", Name: "string", Route: "string", VehicleClass: "string", StopMode: "string",
    Wave: "BZTRWave", Driver: "BZTRDriver", TraderZone: "BZTRTraderZone", Traders: "array<refBZTRTrader>",
    Stops: "array<refBZTRStop>", OnRouteEnd: "BZTROnRouteEnd", Editor: "BZTREditor", StartDelayS: "float",
    Markers: "BZTRMarkers", RespawnSeconds: "int", PlayerRadiusM: "float", MaxWithPlayersS: "float",
    SeatedWaitS: "float", PlayerHaltM: "float", MaxStallDistanceM: "float", FrontPlayerWaitS: "float" },
  BZTRWave: { Enabled: "int", RadiusM: "float", AheadOnly: "int", CooldownS: "float",
    TimeS: "float", MemoryS: "float", MinGapM: "float", MaxPerLap: "int", HornOnDepart: "int" },
  BZTRDriver: { IsTrader: "int", AllCanDrive: "int" },
  BZTRTraderZone: { Name: "string", FollowsVehicle: "int", RadiusM: "float" },
  BZTRTrader: { Seat: "int", Name: "string", TraderAIClass: "string", Loadout: "string", TraderFile: "string",
    Faction: "string", LootingBehaviour: "string" },
  BZTRStop: { Mark: "int", HeadingDeg: "float", DistM: "float", Mode: "string", WaveRadiusM: "float",
    WhoGetsOff: "array<int>", BoardTimeoutS: "float", Traders: "array<refBZTRStopTrader>",
    Signals: "array<refBZTRStopSignal>", HornOnDepart: "int" },
  BZTRStopSignal: { Color: "string", EverySeconds: "float", StartDelayS: "float" },
  BZTRStopTrader: { Seat: "int", Waypoints: "array<refBZTRWaypoint>", TimeS: "float", MovementSpeed: "string" },
  BZTRWaypoint: { WayPointName: "string", Position: "vector" },
  BZTROnRouteEnd: { Action: "string", DelayS: "float", WaitS: "float" },
  BZTREditor: { Version: "int", Seats: "int", RouteLengthM: "float", Start: "array<float>", End: "array<float>",
    Example: "int" },
  BZTRMarkers: { useMapMarker: "int", MarkerUpdateSeconds: "int", MarkerLabel: "string", MarkersIcon: "string",
    Expansion3DMarkers: "int", Red: "string", Green: "string", Blue: "string", Alpha: "string", BlinkAtStops: "int", BlinkSeconds: "int" }
};
// claves que el mod lee del TEXTO a un campo [NonSerialized()] con otro nombre (Clothing y Pos son clases de DayZ;
// Type() es un metodo de todas las clases de Enforce)
var LEIDAS_DEL_TEXTO = { BZTRTrader: { Clothing: "array<string>" }, BZTRStop: { Pos: "array<float>" },
  BZTRStopSignal: { Type: "string", Pos: "array<float>" },
  // el rumbo del puesto (un campo serializado lo escribiria el ejemplo del mod como 0 = norte)
  BZTRStopTrader: { HeadingDeg: "float", LookAt: "array<float>" } };

// Plazas por clase de vehiculo: el "[SEATS] === <clase> CrewSize=N ===" que AutoDrive deja en el RPT, juntado de los
// RPT de varios servers (DayZ 1.29 y 1.30). Una clase con color (CivilianSedan_Black) cae en
// su base (CivilianSedan). Otra clase: la toma (Seats o CrewSize del hdr), lo recordado, o se pregunta.
var PLAZAS = {
  AEC_Matador: 4, BZ_ChernoBus: 22, BZ_ChernoBus_02: 22, BZ_ChernoTaxi: 4, CivilianSedan: 4, ExpansionBus: 22,
  ExpansionTractor: 1, Expansion_Landrover: 4, GT2RS_MR: 2, Gaz_Ambulance: 4, Gemera: 4, Hatchback_02: 4,
  HennesseyF150D: 4, Huracan_Tecnica: 2, M3_G80_Police: 4, M3_G80_Police2: 4, MBM_Apocalypse_Truck: 2,
  MBM_ApocalypticPAZ: 5, MBM_F350: 4, MBM_F350_Monster: 4, MBM_HarleyDavidsonFatBoy: 1, MBM_ImprezaWRX: 4,
  MBM_LandCruiser: 4, MBM_Sprinter_FreeCandyCargo: 2, MBM_Sprinter: 6, MBM_SuzukiGSXR750: 1, Mercedes_G4_W31: 5,
  Motorbike_02: 2, Nissan: 2, OffroadHatchback: 4, PzKpfw_VI_Tiger: 4, SVJ: 2, Scammel_Pioneer: 2, Sedan_02: 4,
  Star_APC_Cobra: 6, Star_Agera: 2, Star_Camaro_1969: 4, Star_Chevrolet_Z71: 4, Star_Enzo: 2, Star_Golf_MK1: 4,
  Star_Jeep_Rubi_Extra: 4, Star_Oshkosh_M1083: 2, Star_Porsche_944: 2, Toyota_86_RB: 2, Truck_01_Covered: 2,
  UAZ_452: 4, VW_T6: 4, WideBody_Hellcat: 2, Willys_MB: 3, ZiS5_Benzovoz: 2, a_m5: 4, daytona: 2, e60: 4,
  f22: 2, hx58: 2, jesko: 2, lfa: 2, m4_tractor_usa: 6, m4_tractor_winter: 6, m4w: 2, pofirebird: 4, r32: 2,
  viper: 2, x5mcompetition: 4,
  // medido: el UAZ sin techo de Expansion tiene CrewSize=4 (el chofer + 3)
  ExpansionUAZRoofless: 4
};

var BORIS = "eAI_SurvivorM_Boris";

// ------------------------------------------------------------------------------------------------ utilidades
function copia(o) { return o === undefined ? undefined : JSON.parse(JSON.stringify(o)); }
function r1(v) { return Math.round(v * 10) / 10; }
function finito(v) { return typeof v === "number" && isFinite(v); }
function nl(texto) { return texto.indexOf("\r\n") >= 0 ? "\r\n" : "\n"; }
function sinBom(t) { return t.charCodeAt(0) === 0xFEFF ? t.slice(1) : t; }
function nombreCorto(ruta) { return String(ruta || "").replace(/\.json$/i, "").replace(/^BZBusRoute_/, ""); }
function sello(d) {
  d = d || new Date();
  function p(x) { return (x < 10 ? "0" : "") + x; }
  return String(d.getFullYear()).slice(2) + p(d.getMonth() + 1) + p(d.getDate()) + "_" + p(d.getHours()) + p(d.getMinutes()) + p(d.getSeconds());
}
function nombreRespaldo(archivo, d) { return archivo + ".pre_editor_" + sello(d); }

// ------------------------------------------------------------------------------------------------ la toma
// _wp.csv: x, y, z en 0-2; stop en 3; el rumbo de DayZ en la columna 15 (base 0). Las filas cortas o rotas se saltean,
// como LoadWaypointsCSV de AutoDrive (menos de 14 columnas).
function leerWp(texto) {
  var out = [];
  String(texto || "").split(/\r?\n/).forEach(function (linea) {
    if (!linea) return;
    var p = linea.split(",");
    if (p.length < 14) return;
    var x = parseFloat(p[0]), y = parseFloat(p[1]), z = parseFloat(p[2]);
    if (!isFinite(x) || !isFinite(z)) return;
    var h = p.length > 15 ? parseFloat(p[15]) : NaN;
    out.push({ x: x, y: isFinite(y) ? y : 0, z: z, h: h, stop: p[3] === "1" });
  });
  return out;
}

function distancias(pts) {
  var acum = [0];
  for (var i = 1; i < pts.length; i++) acum.push(acum[i - 1] + Math.hypot(pts[i].x - pts[i - 1].x, pts[i].z - pts[i - 1].z));
  return acum;
}

// Rumbo de DayZ (0 = norte, horario) del tramo alrededor del wp i, para una toma sin la columna 15.
function rumboDelTramo(pts, acum, i) {
  var a = i, b = i;
  while (b < pts.length - 1 && acum[b] - acum[i] < 5) b++;
  while (a > 0 && acum[i] - acum[a] < 5) a--;
  var dx = pts[b].x - pts[a].x, dz = pts[b].z - pts[a].z;
  if (!dx && !dz) return 0;
  return Math.atan2(dx, dz) * 180 / Math.PI;
}

// Las marcas 4 con el MISMO criterio que el mod (BZTRRuta.LeerMarcas): eventos del hdr sin disparador o con
// trigger.type "", "wp_reached" o "waypoint"; el wp del trigger (o el del evento), 0 < wp < n-1, sin repetir. Si no
// hay ninguno, las filas stop = 1 del _wp.csv (menos la ultima). Numeradas 1..N por wp.
function marcasDeToma(hdr, pts) {
  var n = pts.length, wps = [], origen = "", origenK = "";
  var eventos = hdr && Array.isArray(hdr.Events) ? hdr.Events : [];
  eventos.forEach(function (ev) {
    if (!ev || typeof ev !== "object") return;
    var tipo = "wp_reached", w = 0;
    if (ev.trigger && typeof ev.trigger === "object") {
      tipo = ev.trigger.type === undefined ? "" : String(ev.trigger.type);
      w = parseInt(ev.trigger.wp, 10) || 0;
    }
    if (tipo !== "" && tipo !== "wp_reached" && tipo !== "waypoint") return;
    if (w <= 0) w = parseInt(ev.wp, 10) || 0;
    if (w <= 0 || w >= n - 1) return;
    if (wps.indexOf(w) < 0) wps.push(w);
  });
  if (eventos.length) { origenK = "n.origen_eventos"; origen = T(origenK); }
  if (!wps.length) {
    for (var i = 0; i < n - 1; i++) if (pts[i].stop) wps.push(i);
    if (wps.length) { origenK = "n.origen_filas"; origen = T(origenK); }
  }
  wps.sort(function (a, b) { return a - b; });
  var acum = distancias(pts);
  return { origen: origen, origenK: origenK, marcas: wps.map(function (w, k) {
    var p = pts[w], h = finito(p.h) ? p.h : rumboDelTramo(pts, acum, w);
    return { numero: k + 1, wp: w, pos: [r1(p.x), r1(p.z)], heading: r1(h), dist: Math.round(acum[w]) };
  }) };
}

// Todo lo que el editor necesita de una toma convertida.
function toma(nombre, hdr, wpTexto) {
  var pts = leerWp(wpTexto);
  if (pts.length < 2) throw new Error(T("n.wp_invalido"));
  var acum = distancias(pts), m = marcasDeToma(hdr || {}, pts);
  return {
    nombre: nombreCorto(nombre), ruta: "BZBusRoute_" + nombreCorto(nombre) + ".json",
    vehiculo: (hdr && hdr.VehicleClass) || "", driverClass: (hdr && hdr.DriverClass) || "",
    crew: hdr && Array.isArray(hdr.Crew) ? hdr.Crew : [],
    seatsHdr: hdr ? (finito(hdr.Seats) ? hdr.Seats : (finito(hdr.CrewSize) ? hdr.CrewSize : 0)) : 0,
    // el mundo donde se grabo ("World" en el hdr), si el grabador de AutoDrive lo escribe. Hoy no viene: "".
    mundo: hdr && typeof hdr.World === "string" ? hdr.World : "",
    pts: pts, acum: acum, largo: acum[acum.length - 1], marcas: m.marcas, origenMarcas: m.origen, origenMarcasK: m.origenK,
    inicio: [pts[0].x, pts[0].y, pts[0].z], fin: [pts[pts.length - 1].x, pts[pts.length - 1].z]
  };
}

// Plazas: la toma (Seats / CrewSize del hdr) > el server (el archivo de plazas que escribe el mod) > lo
// recordado por el admin > la tabla > nada (se pregunta).
function plazasDe(clase, seatsHdr, recordadas, deServer) {
  if (seatsHdr > 0) return { n: seatsHdr, fuente: T("n.fuente_toma") };
  if (deServer > 0) return { n: deServer, fuente: T("n.fuente_server") };
  if (recordadas && recordadas[clase] > 0) return { n: recordadas[clase], fuente: T("n.fuente_recordado", clase) };
  var c = String(clase || "");
  while (c) {
    if (PLAZAS[c] > 0) return { n: PLAZAS[c], fuente: T("n.fuente_tabla", c) };
    var k = c.lastIndexOf("_");
    if (k <= 0) break;
    c = c.slice(0, k);
  }
  return { n: 0, fuente: "" };
}

// ------------------------------------------------------------------------------------------------ geometria (DayZ)
function ejes(headingDeg) {
  var h = headingDeg * Math.PI / 180;
  return { fx: Math.sin(h), fz: Math.cos(h), rx: Math.cos(h), rz: -Math.sin(h) };
}
function aMundo(pos, headingDeg, f, r) {
  var e = ejes(headingDeg);
  return [pos[0] + f * e.fx + r * e.rx, pos[1] + f * e.fz + r * e.rz];
}
function aLocal(pos, headingDeg, x, z) {
  var e = ejes(headingDeg), dx = x - pos[0], dz = z - pos[1];
  return { f: dx * e.fx + dz * e.fz, r: dx * e.rx + dz * e.rz };
}
function difAngulo(a, b) { var d = ((a - b) % 360 + 540) % 360 - 180; return Math.abs(d); }

// ------------------------------------------------------------------------------------------------ el servicio
function marcasActivas(svc) { return svc.StopMode !== "wave_only"; }
function usaSena(svc) { return svc.StopMode !== "marks"; }
function traders(svc) { return (svc.Traders || []).slice().sort(function (a, b) { return a.Seat - b.Seat; }); }
function traderDe(svc, seat) { var t = (svc.Traders || []).filter(function (x) { return x.Seat === seat; }); return t[0] || null; }
function choferVende(svc) { return !!(svc.Driver && svc.Driver.IsTrader === 1); }
function maxComerciantes(svc) { var s = (svc.Editor && svc.Editor.Seats) || 0; return choferVende(svc) ? s : Math.max(0, s - 1); }

function paradaNueva(m) {
  return { Mark: m.numero, Pos: m.pos.slice(), HeadingDeg: m.heading, DistM: m.dist, Mode: "always", WaveRadiusM: 0,
           WhoGetsOff: [], BoardTimeoutS: 20, Traders: [] };
}

// Un servicio nuevo sobre una toma: sin comerciantes todavia (el primero que se agrega maneja); una parada
// por marca.
function servicioNuevo(tm, plazas) {
  var nombre = tm.nombre;
  return {
    Enable: 1, Name: nombre, Route: tm.ruta, VehicleClass: tm.vehiculo, StopMode: "marks",
    MaxWithPlayersS: 0,   // explicito, asi un server con un PBO viejo (default 180) tampoco estira la parada
    Wave: { Enabled: 0, RadiusM: DEF.WAVE_RADIUS_M, AheadOnly: 1, CooldownS: DEF.WAVE_COOLDOWN_S,
            TimeS: DEF.WAVE_TIME_S, MemoryS: DEF.WAVE_MEMORY_S, MinGapM: DEF.WAVE_MIN_GAP_M, MaxPerLap: DEF.WAVE_MAX_PER_LAP },
    Driver: { IsTrader: 1, AllCanDrive: 1 },
    TraderZone: { Name: "BZT_" + nombre, FollowsVehicle: 1, RadiusM: DEF.ZONE_RADIUS_MIN_M },
    Traders: [],
    Stops: tm.marcas.map(paradaNueva),
    OnRouteEnd: { Action: "restart", DelayS: 0 },
    Editor: { Version: 1, Seats: plazas, RouteLengthM: Math.round(tm.largo), Start: [r1(tm.inicio[0]), r1(tm.inicio[2])],
              End: [r1(tm.fin[0]), r1(tm.fin[1])], Example: 0 }
  };
}

var CLASES_TRADER = ["ExpansionTraderAIMirek", "ExpansionTraderAIDenis", "ExpansionTraderAIBoris", "ExpansionTraderAICyril",
  "ExpansionTraderAIElias", "ExpansionTraderAIFrancis", "ExpansionTraderAIGuo", "ExpansionTraderAIHassan",
  "ExpansionTraderAIIndar", "ExpansionTraderAIJose", "ExpansionTraderAIKaito", "ExpansionTraderAILewis",
  "ExpansionTraderAIManua", "ExpansionTraderAINiki", "ExpansionTraderAIOliver", "ExpansionTraderAIPeter",
  "ExpansionTraderAIQuinn", "ExpansionTraderAIRolf", "ExpansionTraderAISeth", "ExpansionTraderAITaiki",
  "ExpansionTraderAILinda", "ExpansionTraderAIMaria", "ExpansionTraderAIFrida", "ExpansionTraderAIGabi",
  "ExpansionTraderAIHelga", "ExpansionTraderAIIrena", "ExpansionTraderAIJudy", "ExpansionTraderAIKeiko",
  "ExpansionTraderAIEva", "ExpansionTraderAINaomi", "ExpansionTraderAIBaty"];

function traderNuevo(seat, archivo) {
  return { Seat: seat, Name: seat === 0 ? T("n.comerciante") : T("n.comerciante_n", seat),
           TraderAIClass: CLASES_TRADER[(seat * 7 + 1) % CLASES_TRADER.length], Loadout: "SurvivorLoadout", Clothing: [],
           TraderFile: archivo || "" };   // sin Faction: el mod la ignora (DEF.FACCION, siempre)
}

// ★ Los nombres por defecto de los comerciantes («Trader», «Trader N» / «Comerciante», «Comerciante N», de bzt_i18n.js)
// siguen el idioma del editor. Al cambiarlo, el que tiene el nombre por defecto del OTRO idioma pasa al nuevo; un nombre
// propio no se toca, y al abrir un archivo se respeta lo que diga (esto corre solo al cambiar el idioma). Devuelve cuantos.
function nombresPorDefecto(svc, lang) {
  var n = 0;
  if (!I18N || !svc || !Array.isArray(svc.Traders)) return 0;
  ["en", "es"].filter(function (o) { return o !== lang; }).forEach(function (o) {
    var solo = I18N.t(o, "n.comerciante", []), conN = I18N.t(o, "n.comerciante_n", ["\u0001"]);
    var i = conN.indexOf("\u0001"), pre = conN.slice(0, i), post = conN.slice(i + 1);
    svc.Traders.forEach(function (t) {
      if (!t || typeof t.Name !== "string") return;
      if (t.Name === solo) { t.Name = I18N.t(lang, "n.comerciante", []); n++; return; }
      if (t.Name.length <= pre.length + post.length || t.Name.indexOf(pre) !== 0 || t.Name.slice(t.Name.length - post.length) !== post) return;
      var num = t.Name.slice(pre.length, t.Name.length - post.length);
      if (!/^\d+$/.test(num)) return;
      t.Name = I18N.t(lang, "n.comerciante_n", [num]);
      n++;
    });
  });
  return n;
}

// La primera plaza libre para un comerciante (0 si el chofer vende y no hay nadie ahi). -1 si no entra.
function plazaLibre(svc) {
  var max = (svc.Editor && svc.Editor.Seats) || 0;
  for (var s = choferVende(svc) ? 0 : 1; s < max; s++) if (!traderDe(svc, s)) return s;
  return -1;
}

function agregarComerciante(svc, archivo) {
  var s = plazaLibre(svc);
  if (s < 0) return null;
  var t = traderNuevo(s, archivo);
  svc.Traders.push(t);
  svc.Traders.sort(function (a, b) { return a.Seat - b.Seat; });
  return t;
}

function quitarDeParadas(svc, seat) {
  (svc.Stops || []).forEach(function (st) {
    st.WhoGetsOff = (st.WhoGetsOff || []).filter(function (s) { return s !== seat; });
    st.Traders = (st.Traders || []).filter(function (p) { return p.Seat !== seat; });
  });
}

function quitarComerciante(svc, seat) {
  svc.Traders = (svc.Traders || []).filter(function (t) { return t.Seat !== seat; });
  quitarDeParadas(svc, seat);
}

// Lo que una plaza tiene en cada parada (si baja, su puesto) pasa a otra plaza.
function cambiarPlazaEnParadas(svc, de, a) {
  (svc.Stops || []).forEach(function (st) {
    st.WhoGetsOff = (st.WhoGetsOff || []).map(function (s) { return s === de ? a : s; }).sort(function (x, y) { return x - y; });
    (st.Traders || []).forEach(function (p) { if (p.Seat === de) p.Seat = a; });
    if (st.Traders) st.Traders.sort(function (x, y) { return x.Seat - y.Seat; });
  });
}

// ★ maneja uno de los comerciantes. El elegido pasa a la plaza 0 (Driver.IsTrader 1); si ya
// manejaba otro, intercambian plazas. Cada uno conserva lo suyo en las paradas (si bajaba, sigue bajando).
// En un servicio viejo (manejaba el chofer de la ruta), la plaza del elegido queda libre.
function ponerQuienManeja(svc, seat) {
  var t = traderDe(svc, seat), t0;
  if (!t) return false;
  svc.Driver = svc.Driver || { AllCanDrive: 1 };
  svc.Driver.IsTrader = 1;
  if (seat === 0) return true;
  t0 = traderDe(svc, 0);
  cambiarPlazaEnParadas(svc, seat, -1);
  if (t0) { cambiarPlazaEnParadas(svc, 0, seat); t0.Seat = seat; }
  else quitarDeParadas(svc, 0);
  cambiarPlazaEnParadas(svc, -1, 0);
  t.Seat = 0;
  svc.Traders.sort(function (a, b) { return a.Seat - b.Seat; });
  return true;
}

// El puesto de una plaza en una parada (su primer waypoint). null si no tiene.
function puesto(st, seat) {
  var p = (st.Traders || []).filter(function (x) { return x.Seat === seat; })[0];
  if (!p || !p.Waypoints || !p.Waypoints.length || !p.Waypoints[0].Position) return null;
  return p;
}
function puestoXZ(st, seat) {
  var p = puesto(st, seat);
  if (!p) return null;
  var w = p.Waypoints[0].Position;
  return [w[0], w[2]];
}

// Baja (o no) una plaza en una parada. Al bajar sin puesto se le arma uno a 4,5 m a la derecha, en fila cada 2 m.
function ponerBaja(svc, st, seat, baja) {
  st.WhoGetsOff = (st.WhoGetsOff || []).filter(function (s) { return s !== seat; });
  if (baja) {
    st.WhoGetsOff.push(seat);
    st.WhoGetsOff.sort(function (a, b) { return a - b; });
    if (!puesto(st, seat)) {
      var orden = traders(svc).map(function (t) { return t.Seat; }).indexOf(seat), cuantos = (svc.Traders || []).length;
      var f = 2 * (cuantos - 1) / 2 - 2 * Math.max(0, orden);
      var w = aMundo(st.Pos, st.HeadingDeg || 0, f, 4.5);
      st.Traders = st.Traders || [];
      st.Traders.push({ Seat: seat, Waypoints: [{ WayPointName: "M" + st.Mark + "-" + seat, Position: [r1(w[0]), 0, r1(w[1])] }],
                        TimeS: DEF.TRADER_TIME_S, MovementSpeed: "WALK" });
      st.Traders.sort(function (a, b) { return a.Seat - b.Seat; });
    }
  } else {
    st.Traders = (st.Traders || []).filter(function (p) { return p.Seat !== seat; });
  }
}

function moverPuesto(st, seat, x, z) {
  var p = puesto(st, seat);
  if (!p) return;
  p.Waypoints[0].Position = [r1(x), p.Waypoints[0].Position[1] || 0, r1(z)];
}

// ------------------------------------------------------------------------------------------------ el rumbo del puesto
// hacia donde mira cada comerciante en su puesto. = BZTRStopTrader.Mirada del mod: LookAt [x, z] gana
// (si no cae encima del puesto); si no, HeadingDeg (rumbo de DayZ: 0 = norte, horario). null = sin orientacion.
function rumboXZ(dx, dz) { var r = Math.atan2(dx, dz) * 180 / Math.PI; return r < 0 ? r + 360 : r; }
function rumboDePuesto(p) {
  if (!p || !p.Waypoints || !p.Waypoints[0] || !p.Waypoints[0].Position) return null;
  var w = p.Waypoints[0].Position;
  if (Array.isArray(p.LookAt) && p.LookAt.length === 2 && finito(p.LookAt[0]) && finito(p.LookAt[1]) && Math.hypot(p.LookAt[0] - w[0], p.LookAt[1] - w[2]) >= 0.2)
    return { deg: rumboXZ(p.LookAt[0] - w[0], p.LookAt[1] - w[2]), fuente: "LookAt" };
  if (finito(p.HeadingDeg)) return { deg: ((p.HeadingDeg % 360) + 360) % 360, fuente: "HeadingDeg" };
  return null;
}
// Un rumbo a mano (la flecha o el campo de grados): saca el LookAt. null = sin orientacion.
function ponerRumbo(p, deg) {
  if (!p) return;
  delete p.LookAt;
  if (deg === null || deg === undefined || !finito(deg)) { delete p.HeadingDeg; return; }
  p.HeadingDeg = Math.round((((deg % 360) + 360) % 360) * 10) / 10;
}
// El atajo "En semicirculo mirando al centro": los que bajan en la parada, en un arco de 'arco' grados y 'radio' m
// alrededor de 'centro' [x, z], cada uno con LookAt al centro. El arco se abre del lado opuesto a la marca (los
// jugadores llegan desde la calle y les ven la cara). Devuelve cuantos acomodo.
function semicirculo(svc, st, centro, radio, arco) {
  if (!st || !st.Pos || !Array.isArray(centro) || !finito(centro[0]) || !finito(centro[1])) return 0;
  var seats = (st.WhoGetsOff || []).filter(function (s) { return traderDe(svc, s); }).sort(function (a, b) { return a - b; });
  seats.forEach(function (s) { if (!puesto(st, s)) ponerBaja(svc, st, s, true); });
  var n = seats.length;
  if (!n) return 0;
  radio = finito(radio) && radio > 0 ? radio : 6;
  arco = finito(arco) ? Math.max(0, Math.min(360, arco)) : 180;
  var base = Math.hypot(centro[0] - st.Pos[0], centro[1] - st.Pos[1]) < 0.5 ? (st.HeadingDeg || 0) + 90 : rumboXZ(centro[0] - st.Pos[0], centro[1] - st.Pos[1]);
  var paso = n === 1 ? 0 : (arco >= 360 ? 360 / n : arco / (n - 1));
  seats.forEach(function (s, i) {
    var a = (n === 1 ? base : base - (arco >= 360 ? 0 : arco / 2) + paso * i) * Math.PI / 180;
    moverPuesto(st, s, centro[0] + radio * Math.sin(a), centro[1] + radio * Math.cos(a));
    var p = puesto(st, s);
    delete p.HeadingDeg;
    p.LookAt = [r1(centro[0]), r1(centro[1])];
  });
  return n;
}
// El atajo "Todos mirando a la calle": cada uno perpendicular al vehiculo, de cara a la ruta (el de la derecha mira a
// la izquierda del vehiculo y al reves). Devuelve cuantos.
function mirandoALaCalle(svc, st) {
  if (!st || !st.Pos) return 0;
  var H = st.HeadingDeg || 0, n = 0;
  (st.Traders || []).forEach(function (p) {
    if ((st.WhoGetsOff || []).indexOf(p.Seat) < 0 || !p.Waypoints || !p.Waypoints[0] || !p.Waypoints[0].Position) return;
    var w = p.Waypoints[0].Position, lc = aLocal(st.Pos, H, w[0], w[2]);
    ponerRumbo(p, lc.r >= 0 ? H - 90 : H + 90);
    n++;
  });
  return n;
}

// El puesto mas lejano de las paradas activas (en XZ, desde su marca).
function puestoMasLejos(svc) {
  var r = 0;
  if (!marcasActivas(svc)) return 0;
  (svc.Stops || []).forEach(function (st) {
    if (st.Mode === "off" || !st.Pos) return;
    (st.Traders || []).forEach(function (p) {
      (p.Waypoints || []).forEach(function (w) {
        if (!w.Position) return;
        r = Math.max(r, Math.hypot(w.Position[0] - st.Pos[0], w.Position[2] - st.Pos[1]));
      });
    });
  });
  return r;
}
// El radio que necesita la zona (el puesto mas lejano, minimo 5 m), redondeado hacia arriba a 0,1 m.
function radioNecesario(svc) { return Math.max(DEF.ZONE_RADIUS_MIN_M, Math.ceil(puestoMasLejos(svc) * 10 - 1e-9) / 10); }

// el tope de un puesto (y de una senal) a su marca. = BZTRServicio.MaxStallDistanceM (0 o falta: el default).
function distanciaMax(svc) { var v = svc && svc.MaxStallDistanceM; return finito(v) && v > 0 ? v : DEF.MAX_STALL_DISTANCE_M; }

// ------------------------------------------------------------------------------------------------ las senales
// Stops[].Signals[] = humo de colores de dia, la roadflare de noche, los fuegos artificiales. El mod
// las enciende al desplegar, renueva la que se apago cada EverySeconds y las borra al seguir (BZTraderBalizas.c).
// airdrop: el humo rojo y la luz de un airdrop de Expansion (Missions), enterrado en la posicion de la senal
var SENAL_TIPOS = ["auto", "smoke", "roadflare", "fireworks", "airdrop"];
// los que ya no se ofrecen y se leen como otro (como el mod): la pistola de bengalas no dispara sola, sale una roadflare
var SENAL_TIPOS_VIEJOS = { flaregun: "roadflare" };
var SENAL_COLORES = ["green", "purple", "red", "white", "yellow", "blue", "black"];
var SENAL_TRIANGULO_M = 15;
function senalTipo(sg) { var t = String((sg && sg.Type) || "").toLowerCase(); if (SENAL_TIPOS_VIEJOS[t]) t = SENAL_TIPOS_VIEJOS[t]; return SENAL_TIPOS.indexOf(t) >= 0 ? t : DEF.SIGNAL_TYPE; }
function senalColor(sg) { var c = String((sg && sg.Color) || "").toLowerCase(); return SENAL_COLORES.indexOf(c) >= 0 ? c : DEF.SIGNAL_COLOR; }
function usaHumo(sg) { var t = senalTipo(sg); return t === "auto" || t === "smoke"; }
// = BZTRBalizas.Clase del mod: la clase que crea (noche: lo que hace el auto de noche)
function claseSenal(sg, noche) {
  var t = senalTipo(sg), c = senalColor(sg);
  if (t === "auto") t = noche ? "roadflare" : "smoke";
  if (t === "roadflare") return "Roadflare";   // tambien la pistola de un archivo viejo (senalTipo la lee asi)
  if (t === "fireworks") return "FireworksLauncher";
  if (t === "airdrop") return "ExpansionAirdropContainer_Client";
  return { purple: "M18SmokeGrenade_Purple", blue: "M18SmokeGrenade_Purple", red: "M18SmokeGrenade_Red", white: "M18SmokeGrenade_White",
           yellow: "M18SmokeGrenade_Yellow", black: "RDG2SmokeGrenade_Black" }[c] || "M18SmokeGrenade_Green";
}
function senalNueva(st, f, r, tipo, color) {
  var w = aMundo(st.Pos, st.HeadingDeg || 0, f, r);
  return { Type: tipo || DEF.SIGNAL_TYPE, Color: color || DEF.SIGNAL_COLOR, Pos: [r1(w[0]), r1(w[1])],
           EverySeconds: DEF.SIGNAL_EVERY_S, StartDelayS: DEF.SIGNAL_START_DELAY_S };
}
// "+": a 10 m a la derecha del vehiculo, corriendo 3 m hacia adelante por cada una que ya haya
function agregarSenal(st) {
  if (!st || !st.Pos) return null;
  st.Signals = st.Signals || [];
  var sg = senalNueva(st, 3 * st.Signals.length, 10, DEF.SIGNAL_TYPE, SENAL_COLORES[st.Signals.length % SENAL_COLORES.length]);
  st.Signals.push(sg);
  return sg;
}
function quitarSenal(st, i) { if (st && st.Signals && i >= 0 && i < st.Signals.length) st.Signals.splice(i, 1); }
function moverSenal(st, i, x, z) { var sg = st && st.Signals && st.Signals[i]; if (sg) sg.Pos = [r1(x), r1(z)]; }
// El atajo: 3 automaticas en triangulo a 15 m de la marca, ninguna sobre la ruta (una a la derecha, dos a la izquierda,
// adelante y atras). Verde, amarilla y violeta. REEMPLAZA las de la parada; devuelve cuantas habia.
function tresSenales(st) {
  if (!st || !st.Pos) return -1;
  var antes = (st.Signals || []).length, R = SENAL_TRIANGULO_M, c = Math.cos(Math.PI / 6) * R, s = Math.sin(Math.PI / 6) * R;
  st.Signals = [senalNueva(st, 0, R, "auto", "green"), senalNueva(st, c, -s, "auto", "yellow"), senalNueva(st, -c, -s, "auto", "purple")];
  return antes;
}
function avisosSenales(svc, st, tope, A) {
  var l = st.Signals || [];
  if (l.length > DEF.SIGNALS_MAX) A("AVISO", T("n.sen_demasiadas", T("n.parada", st.Mark), l.length, DEF.SIGNALS_MAX));
  l.forEach(function (sg, i) {
    var q = T("n.parada_senal", st.Mark, i + 1);
    if (!sg || !Array.isArray(sg.Pos) || sg.Pos.length < 2 || !finito(sg.Pos[0]) || !finito(sg.Pos[1])) { A("AVISO", T("n.sen_sin_pos", q)); return; }
    if (sg.Type !== undefined && SENAL_TIPOS.indexOf(String(sg.Type).toLowerCase()) < 0 && !SENAL_TIPOS_VIEJOS[String(sg.Type).toLowerCase()]) A("AVISO", T("n.sen_tipo", q, sg.Type, DEF.SIGNAL_TYPE));
    if (sg.Color !== undefined && SENAL_COLORES.indexOf(String(sg.Color).toLowerCase()) < 0) A("AVISO", T("n.sen_color", q, sg.Color, DEF.SIGNAL_COLOR));
    if (st.Pos) {
      var d = Math.hypot(sg.Pos[0] - st.Pos[0], sg.Pos[1] - st.Pos[1]);
      if (d > tope) A("AVISO", T("n.sen_lejos", q, r1(d), tope));
    }
    if (senalColor(sg) === "blue" && usaHumo(sg)) A("AVISO", T("n.sen_azul", q));
  });
}

// El archivo que lee el mod. Lo que el editor no toco queda igual; la zona nunca queda mas chica que lo necesario.
function exportar(svc) {
  var o = copia(svc);
  if (o.TraderZone && o.TraderZone.Name && o.TraderZone.FollowsVehicle === 1) {
    var nec = radioNecesario(o);
    if (!(o.TraderZone.RadiusM >= nec)) o.TraderZone.RadiusM = nec;
  }
  return o;
}
function textoServicio(o) { return JSON.stringify(o, null, 2) + "\n"; }

// Igualdad de contenido (sin importar el orden de las claves ni el formato de los numeros).
function mismoContenido(a, b) {
  if (a === b) return true;
  if (typeof a === "number" && typeof b === "number") return Math.abs(a - b) < 1e-9;
  if (Array.isArray(a)) {
    if (!Array.isArray(b) || a.length !== b.length) return false;
    for (var i = 0; i < a.length; i++) if (!mismoContenido(a[i], b[i])) return false;
    return true;
  }
  if (a && b && typeof a === "object" && typeof b === "object" && !Array.isArray(b)) {
    var ka = Object.keys(a).sort(), kb = Object.keys(b).sort();
    if (ka.join("\u0001") !== kb.join("\u0001")) return false;
    for (var k = 0; k < ka.length; k++) if (!mismoContenido(a[ka[k]], b[ka[k]])) return false;
    return true;
  }
  return false;
}

// ------------------------------------------------------------------------------------------------ validacion
// Lo mismo que config_real.py: ERROR = un tipo que JsonFileLoader rechaza (el servicio no carga); AVISO = una clave sin
// campo (JsonFileLoader la ignora en silencio: un typo); info = lo que sale del default.
function tipoOk(tipo, v) {
  if (tipo === "int") return typeof v === "number" || typeof v === "boolean";
  if (tipo === "float") return typeof v === "number";
  if (tipo === "string") return typeof v === "string";
  if (tipo === "bool") return typeof v === "boolean" || typeof v === "number";
  if (tipo === "vector") return Array.isArray(v) && v.length === 3 && v.every(function (x) { return typeof x === "number"; });
  return null;
}
function revisarValor(v, tipo, ruta, out) {
  var ok = tipoOk(tipo, v);
  if (ok === true) return;
  if (ok === false) { out.push({ nivel: "ERROR", ruta: ruta, msg: T("n.se_esperaba", tipo, JSON.stringify(v)) }); return; }
  var m = /^array<(ref)?(\w+)>$/.exec(tipo);
  if (m) {
    if (!Array.isArray(v)) { out.push({ nivel: "ERROR", ruta: ruta, msg: T("n.se_esperaba_lista", tipo) }); return; }
    v.forEach(function (e, k) { revisarValor(e, m[2], ruta + "[" + k + "]", out); });
    return;
  }
  if (ESQUEMA[tipo]) { revisarObjeto(v, tipo, ruta, out); return; }
  out.push({ nivel: "AVISO", ruta: ruta, msg: T("n.sin_revisar", tipo) });
}
function revisarObjeto(valor, clase, ruta, out) {
  var campos = ESQUEMA[clase];
  if (!valor || typeof valor !== "object" || Array.isArray(valor)) { out.push({ nivel: "ERROR", ruta: ruta, msg: T("n.se_esperaba_objeto", clase) }); return; }
  Object.keys(valor).forEach(function (k) {
    var r = (ruta ? ruta + "." : "") + k;
    if (!campos.hasOwnProperty(k)) {
      if (LEIDAS_DEL_TEXTO[clase] && LEIDAS_DEL_TEXTO[clase].hasOwnProperty(k)) {
        // la lee el mod del texto, no JsonFileLoader: un tipo malo no rompe la carga (= config_real.py)
        var sub = [];
        revisarValor(valor[k], LEIDAS_DEL_TEXTO[clase][k], r, sub);
        sub.forEach(function (x) { if (x.nivel === "ERROR") { x.nivel = "AVISO"; x.msg += T("n.del_texto"); } out.push(x); });
        return;
      }
      out.push({ nivel: "AVISO", ruta: r, msg: T("n.clave_sin_campo", clase) });
      return;
    }
    revisarValor(valor[k], campos[k], r, out);
  });
  Object.keys(campos).forEach(function (k) { if (!valor.hasOwnProperty(k)) out.push({ nivel: "info", ruta: (ruta ? ruta + "." : "") + k, msg: T("n.default") }); });
}
function validar(o) { var out = []; revisarObjeto(o, "BZTRServicio", "", out); return out; }

// ---------------------------------------------------------------- el casco del vehiculo
// El archivo que escribe el mod la primera vez que ve una clase: $profile:BZ_Trader/_plazas/<VehicleClass>.json
// (BZTRPlazasArchivo en scripts/4_World/BZTraderCasco.c). Todo relativo al vehiculo: r = a la derecha, f = adelante (m).
// null si no sirve.
var CASCO = { ADENTRO_M: 0.3, ESQUINA_M: 1.5, APROX_M: 1.0 };   // = BZTRCasco.ADENTRO_M, ESQUINA_M y APROX_M
function plazasArchivo(d) {
  if (!d || typeof d !== "object" || !(d.CrewSize > 0) || !Array.isArray(d.HullBox) || d.HullBox.length !== 4 || !Array.isArray(d.SeatDoors)) return null;
  var c = d.HullBox.map(Number);
  if (c.some(function (v) { return !isFinite(v); }) || c[1] - c[0] < 0.2 || c[3] - c[2] < 0.2) return null;
  return {
    clase: String(d.VehicleClass || ""), n: d.CrewSize, fuente: String(d.HullBoxSource || ""),
    caja: { x0: c[0], x1: c[1], z0: c[2], z1: c[3] },
    puertas: d.SeatDoors.filter(function (q) { return q && finito(q.Seat); }).map(function (q) {
      return { seat: q.Seat, r: +q.EntryRight || 0, f: +q.EntryForward || 0, lado: String(q.DoorSide || ""),
               ar: +q.ApproachRight || 0, af: +q.ApproachForward || 0, anim: String(q.DoorAnimation || ""), comparte: (q.SharedWith || []).slice(),
               // la salida medida por el mod (donde quedo parado al bajar)
               medida: q.ExitMeasured === true || q.ExitMeasured === 1, er: +q.ExitRight || 0, ef: +q.ExitForward || 0,
               adentro: q.ExitInsideHull === true || q.ExitInsideHull === 1,
               // el asiento (CrewTransform del motor): donde se sienta; los archivos viejos no lo traen
               asiento: q.SeatMeasured === true || q.SeatMeasured === 1, sr: +q.SeatRight || 0, sf: +q.SeatForward || 0 };
    })
  };
}

// ★ DONDE DIBUJAR CADA PLAZA (r = a la derecha, f = adelante, en m desde el origen del vehiculo). modo:
//   "asiento"  el asiento medido por el mod (SeatRight/SeatForward del archivo de plazas): donde se sienta. Si se
//              pisan o caen fuera del casco (asientosDudosos), no se usa: la entrada o la grilla, con «descartado»;
//   "entrada"  todavia sin el asiento (un archivo de antes): la entrada de cada plaza llevada adentro del casco. En el
//              bus (entradas en el interior caminable) da la fila de verdad, del chofer para atras; las plazas que
//              comparten la entrada (los asientos dobles) se abren una al lado de la otra. En un auto, el lado y la
//              altura de su puerta, si no hay dos plazas por puerta;
//   "grilla"   sin archivo de plazas (la grilla de siempre), o con dos plazas por puerta de afuera (la T6, una
//              compuerta comun): la grilla dentro del casco real.
// {modo, caja, pos: [{seat, r, f}], descartado} con las n plazas en orden; descartado = el porque del asiento que no se
// uso (asientosDudosos) o null.
var PLAZA_MIN_M = 0.25;   // dos plazas a menos que esto se pisan (una entrada para dos)
var PLAZA_ABRIR_M = 0.45; // en el interior caminable, la que comparte la entrada se corre esto, hacia el lado con lugar
function distMinPlazas(pos) {
  var d = Infinity, i, j;
  for (i = 0; i < pos.length; i++) for (j = i + 1; j < pos.length; j++) d = Math.min(d, Math.hypot(pos[i].r - pos[j].r, pos[i].f - pos[j].f));
  return d;
}
// las plazas que comparten la entrada (los asientos dobles del bus): cada una que cae encima de otra anterior se corre
// PLAZA_ABRIR_M hacia el lado del casco con mas lugar, hasta 3 veces
function abrirJuntas(pos, caja, m) {
  var i, j, k, choca, dir;
  for (i = 1; i < pos.length; i++) {
    for (k = 0; k < 3; k++) {
      choca = false;
      for (j = 0; j < i; j++) if (Math.hypot(pos[i].r - pos[j].r, pos[i].f - pos[j].f) < PLAZA_MIN_M) { choca = true; break; }
      if (!choca) break;
      dir = (caja.x1 - pos[i].r) >= (pos[i].r - caja.x0) ? 1 : -1;
      pos[i].r = Math.max(caja.x0 + m, Math.min(caja.x1 - m, pos[i].r + dir * PLAZA_ABRIR_M));
    }
  }
}
// ★ GUARDA del asiento medido: si alguno cae a mas de PLAZA_FUERA_M afuera del casco, o dos se pisan, la medida no
// sirve para dibujar y se vuelve a la entrada (o a la grilla). {motivo: "fuera"|"pisan", plazas: [...]} o null.
var PLAZA_FUERA_M = 0.3;
function asientosDudosos(pos, caja) {
  var i, j, fuera = [];
  for (i = 0; i < pos.length; i++) if (!dentroCaja(caja, pos[i], PLAZA_FUERA_M)) fuera.push(pos[i].seat);
  if (fuera.length) return { motivo: "fuera", plazas: fuera };
  for (i = 0; i < pos.length; i++) for (j = i + 1; j < pos.length; j++) {
    if (Math.hypot(pos[i].r - pos[j].r, pos[i].f - pos[j].f) < PLAZA_MIN_M) return { motivo: "pisan", plazas: [pos[i].seat, pos[j].seat] };
  }
  return null;
}
function grillaPlazas(caja, n, real) {
  var filas = Math.ceil(n / 2), out = [], i, ancho = caja.x1 - caja.x0, paso, frente;
  if (real) {
    frente = caja.z1 - Math.min(1.2, (caja.z1 - caja.z0) / 4);
    paso = filas > 1 ? Math.min(1.2, (frente - caja.z0 - .5) / (filas - 1)) : 0;
    for (i = 0; i < n; i++) out.push({ seat: i, r: i % 2 === 0 ? caja.x0 + ancho * .3 : caja.x1 - ancho * .3, f: frente - Math.floor(i / 2) * paso });
    return out;
  }
  for (i = 0; i < n; i++) out.push({ seat: i, r: i % 2 === 0 ? -.45 : .45, f: caja.z1 - 1.6 - Math.floor(i / 2) * 1.2 });
  return out;
}
function posicionesPlazas(pl, n) {
  var i, q, m, pos = [], todas = true, todasEntrada = true, filas = Math.ceil(Math.max(1, n) / 2), largo = Math.max(4.4, 1.2 * filas + 2.2);
  var caja = pl ? pl.caja : { x0: -1.2, x1: 1.2, z0: -largo / 2, z1: largo / 2 }, dudoso = null;
  n = Math.max(1, n);
  if (pl) {
    for (i = 0; i < n; i++) { q = puertaDe(pl, i); if (!q || !q.asiento) todas = false; if (!q) todasEntrada = false; }
    if (todas) {
      for (i = 0; i < n; i++) { q = puertaDe(pl, i); pos.push({ seat: i, r: q.sr, f: q.sf }); }
      dudoso = asientosDudosos(pos, caja);
      if (!dudoso) return { modo: "asiento", caja: caja, pos: pos };
      // la medida no sirve (descartado): la entrada, o la grilla
      pos = [];
    }
    if (todasEntrada) {
      var adentro = 0;
      m = Math.min(.35, (caja.x1 - caja.x0) / 4);
      for (i = 0; i < n; i++) {
        q = puertaDe(pl, i);
        if (dentroCaja(caja, q, 0)) adentro++;
        pos.push({ seat: i, r: Math.max(caja.x0 + m, Math.min(caja.x1 - m, q.r)), f: Math.max(caja.z0 + m, Math.min(caja.z1 - m, q.f)) });
      }
      if (adentro * 2 > n) abrirJuntas(pos, caja, m);
      if (n === 1 || distMinPlazas(pos) >= PLAZA_MIN_M) return { modo: "entrada", caja: caja, pos: pos, descartado: dudoso };
      pos = [];
    }
  }
  return { modo: "grilla", caja: caja, pos: grillaPlazas(caja, n, !!pl), real: !!pl, descartado: dudoso };
}
function puertaDe(pl, seat) {
  if (!pl) return null;
  for (var i = 0; i < pl.puertas.length; i++) if (pl.puertas[i].seat === seat) return pl.puertas[i];
  return null;
}
function dentroCaja(c, p, m) { return p.r >= c.x0 - m && p.r <= c.x1 + m && p.f >= c.z0 - m && p.f <= c.z1 + m; }
// = BZTRCasco.LadoDe: el borde que tiene mas cerca, en proporcion a la medida de la caja
function ladoCaja(c, p) {
  var cx = (c.x0 + c.x1) / 2, cz = (c.z0 + c.z1) / 2, hx = Math.max(.1, (c.x1 - c.x0) / 2), hz = Math.max(.1, (c.z1 - c.z0) / 2);
  if (Math.abs(p.r - cx) / hx >= Math.abs(p.f - cz) / hz) return p.r >= cx ? T("n.lado.derecha") : T("n.lado.izquierda");
  return p.f >= cz ? T("n.lado.adelante") : T("n.lado.atras");
}
// "adentro" = 0,3 m o mas dentro de la caja (el pasillo del bus); sobre el borde (una puerta) es afuera
function adentroCaja(c, p) { return dentroCaja(c, p, -CASCO.ADENTRO_M); }
// De donde sale a pie una plaza: la salida medida por el mod si la hay; si no, la aproximacion de su puerta
// (sin medir). 'adentro' = la salida cae adentro de la caja: el mod va derecho al puesto, sin rodeo.
function salidaDe(pl, seat) {
  var q = puertaDe(pl, seat);
  if (!q) return null;
  if (q.medida) return { r: q.er, f: q.ef, medida: true, adentro: q.adentro };
  return { r: q.ar, f: q.af, medida: false, adentro: false, entradaAdentro: adentroCaja(pl.caja, { r: q.r, f: q.f }) };
}
// = BZTRCasco.PuntoDeVuelta: donde sube a la vuelta. 1) la salida medida si quedo afuera; 2) si la entrada cae adentro
// de la caja, el borde mas cercano a 'desde' a la altura de la entrada (APROX_M afuera); 3) la aproximacion de su puerta.
function vueltaDe(pl, seat, desde) {
  var q = puertaDe(pl, seat), c = pl && pl.caja;
  if (!q) return null;
  if (q.medida && !q.adentro) return { r: q.er, f: q.ef, como: T("n.vuelta_medida") };
  if (adentroCaja(c, { r: q.r, f: q.f })) {
    var A = CASCO.APROX_M, cand = [{ r: c.x1 + A, f: q.f, como: T("n.vuelta_der") }, { r: c.x0 - A, f: q.f, como: T("n.vuelta_izq") },
                                    { r: q.r, f: c.z1 + A, como: T("n.vuelta_frente") }, { r: q.r, f: c.z0 - A, como: T("n.vuelta_cola") }];
    cand.sort(function (a, b) { return Math.hypot(a.r - desde.r, a.f - desde.f) - Math.hypot(b.r - desde.r, b.f - desde.f); });
    return cand[0];
  }
  return { r: q.ar, f: q.af, como: T("n.vuelta_puerta") };
}
// ¿El tramo a-b entra a la caja agrandada en m? (Liang-Barsky, como BZTRCasco.Cruza)
function cruzaCaja(c, a, b, m) {
  var t0 = 0, t1 = 1, dx = b.r - a.r, dz = b.f - a.f;
  function recorte(pp, q) {
    if (pp === 0) return q >= 0;
    var r = q / pp;
    if (pp < 0) { if (r > t1) return false; if (r > t0) t0 = r; }
    else { if (r < t0) return false; if (r < t1) t1 = r; }
    return true;
  }
  if (!recorte(-dx, a.r - (c.x0 - m))) return false;
  if (!recorte(dx, (c.x1 + m) - a.r)) return false;
  if (!recorte(-dz, a.f - (c.z0 - m))) return false;
  if (!recorte(dz, (c.z1 + m) - a.f)) return false;
  return t1 - t0 > 0.0001;
}
// la esquina i (0 adelante-izq, 1 adelante-der, 2 atras-der, 3 atras-izq) de la caja agrandada en ESQUINA_M
function esquinaCaja(c, i) {
  return { r: (i === 1 || i === 2) ? c.x1 + CASCO.ESQUINA_M : c.x0 - CASCO.ESQUINA_M, f: (i === 2 || i === 3) ? c.z0 - CASCO.ESQUINA_M : c.z1 + CASCO.ESQUINA_M };
}
function ladoRodeo(i, j) {
  var adelante = i === 0 || i === 1, derecha = i === 1 || i === 2;
  if (j < 0) return T("n.rodeo." + (adelante ? "a" : "t") + (derecha ? "d" : "i"));
  if ((i === 0 || i === 1) && (j === 0 || j === 1)) return T("n.rodeo.a");
  if ((i === 2 || i === 3) && (j === 2 || j === 3)) return T("n.rodeo.t");
  if ((i === 0 || i === 3) && (j === 0 || j === 3)) return T("n.rodeo.i");
  return T("n.rodeo.d");
}
// El rodeo de a a b: { esquinas: [{r,f}], lado: "" (derecho) | "?" (sin camino) | por donde }. = BZTRCasco.Rodeo
function rodeoCasco(c, a, b) {
  var m = CASCO.ADENTRO_M, mejor = Infinity, mi = -1, mj = -1, i, j;
  function L(p, q) { return Math.hypot(q.r - p.r, q.f - p.f); }
  if (!cruzaCaja(c, a, b, m)) return { esquinas: [], lado: "" };
  for (i = 0; i < 4; i++) {
    var ci = esquinaCaja(c, i);
    if (cruzaCaja(c, a, ci, m) || cruzaCaja(c, ci, b, m)) continue;
    if (L(a, ci) + L(ci, b) < mejor) { mejor = L(a, ci) + L(ci, b); mi = i; mj = -1; }
  }
  for (i = 0; i < 4; i++) for (j = 0; j < 4; j++) {
    if (j !== (i + 1) % 4 && i !== (j + 1) % 4) continue;
    var a1 = esquinaCaja(c, i), b1 = esquinaCaja(c, j);
    if (cruzaCaja(c, a, a1, m) || cruzaCaja(c, b1, b, m)) continue;
    var l = L(a, a1) + L(a1, b1) + L(b1, b);
    if (l < mejor) { mejor = l; mi = i; mj = j; }
  }
  if (mi < 0) return { esquinas: [], lado: "?" };
  return { esquinas: mj < 0 ? [esquinaCaja(c, mi)] : [esquinaCaja(c, mi), esquinaCaja(c, mj)], lado: ladoRodeo(mi, mj) };
}

// Lo que el esquema no ve: plazas, clases, archivos del Market, puestos, rumbos, la reputacion.
// ctx = {toma, comerciantes:{archivo:{categorias:[..], items:[..]}}, categorias:{nombre:{...}}, hardline:bool,
//        catalogoLeido:bool}
function avisos(svc, ctx) {
  ctx = ctx || {};
  var out = [], seats = (svc.Editor && svc.Editor.Seats) || 0, vistos = {}, tope = distanciaMax(svc);
  function A(nivel, msg) { out.push({ nivel: nivel, msg: msg }); }
  if (!seats) A("ERROR", T("n.faltan_plazas"));
  if ((svc.Traders || []).length > maxComerciantes(svc)) A("ERROR", T("n.demasiados", svc.Traders.length, maxComerciantes(svc), seats, choferVende(svc) ? "" : T("n.una_chofer")));
  // maneja uno de los comerciantes; un servicio viejo no se guarda hasta elegir quien
  if (!choferVende(svc)) A("ERROR", T("n.elegi_chofer"));
  else if (!traderDe(svc, 0)) A("ERROR", (svc.Traders || []).length ? T("n.chofer_sin_0") : T("n.falta_chofer"));
  // las dos reglas del saludo (0 = sin limite)
  if (typeof svc.FrontPlayerWaitS === "number" && svc.FrontPlayerWaitS < 0) A("ERROR", T("n.frente_neg"));
  // los tramos de la ruta donde un saludo puede parar (MinGapM y las marcas de la toma)
  if (usaSena(svc)) {
    var tsal = tramosSaludo(svc);
    if (tsal.largo > 0 && tsal.total <= 0) A("AVISO", T("n.saludo_sin_tramos", tsal.gap, tsal.marcas.join(", ") || "-", tsal.largo));
    else if (tsal.largo > 0 && tsal.total < tsal.largo / 2) A("info", T("n.saludo_tramos", tsal.total, tsal.largo, tsal.tramos.map(function (x) { return x[0] + "-" + x[1] + " m"; }).join(", ")));
  }
  if (usaSena(svc) && svc.Wave) {
    var gapW = svc.Wave.MinGapM, porVuelta = svc.Wave.MaxPerLap, largoR = (svc.Editor && svc.Editor.RouteLengthM) || 0;
    if (typeof gapW === "number" && gapW < 0) A("ERROR", T("n.mingap_neg"));
    if (typeof porVuelta === "number" && (porVuelta < 0 || Math.round(porVuelta) !== porVuelta)) A("ERROR", T("n.maxlap_mal"));
    if (typeof gapW === "number" && largoR > 0 && gapW >= largoR) A("AVISO", T("n.mingap_largo", gapW, largoR));
  }
  (svc.Traders || []).forEach(function (t) {
    if (vistos[t.Seat]) A("ERROR", T("n.plaza_doble", t.Seat));
    vistos[t.Seat] = 1;
    if (seats && t.Seat >= seats) A("ERROR", T("n.plaza_no_existe", t.Name, t.Seat, seats));
    if (t.Seat === 0 && !choferVende(svc)) A("ERROR", T("n.plaza0_sin_elegir", t.Name));
    if (!/^ExpansionTraderAI/.test(t.TraderAIClass || "")) A("AVISO", T("n.no_trader_ai", t.Name, t.TraderAIClass || T("n.sin_clase")));
    if (!t.TraderFile) A("ERROR", T("n.falta_que_vende", t.Name));
    else if (ctx.catalogoLeido && ctx.comerciantes && !ctx.comerciantes[String(t.TraderFile).toLowerCase()]) A("AVISO", T("n.no_en_market", t.Name, t.TraderFile));
    else if (ctx.comerciantes && ctx.categorias && ctx.comerciantes[String(t.TraderFile).toLowerCase()]) {
      ctx.comerciantes[String(t.TraderFile).toLowerCase()].categorias.forEach(function (c) {
        if (!ctx.categorias[c.nombre.toLowerCase()]) A("AVISO", T("n.categoria_falta", t.Name, t.TraderFile, c.nombre));
      });
    }
  });
  if (ctx.hardline) A("AVISO", T("n.hardline"));
  // el marcador, contra el MapSettings.json de la mision (null = no se sabe: sin la mision, nada)
  var mk = svc.Markers || {};
  if (mk.useMapMarker === 1 && ctx.mapaNav) {
    if (!ctx.mapaNav.existe) A("AVISO", T("n.mk_sin_nav"));
    else {
      if (ctx.mapaNav.servidor === 0 || ctx.mapaNav.servidor === false) A("AVISO", T("n.mk_server_off"));
      if (ctx.mapaNav.nombres === 0 || ctx.mapaNav.nombres === false) A("info", T("n.mk_sin_nombre"));
    }
  }
  if (typeof mk.MarkerUpdateSeconds === "number" && mk.MarkerUpdateSeconds < DEF.MARCA_MIN_S) A("AVISO", T("n.mk_cada_min", DEF.MARCA_MIN_S));
  (svc.Stops || []).forEach(function (st) {
    (st.WhoGetsOff || []).forEach(function (s) { if (!traderDe(svc, s)) A("AVISO", T("n.baja_no_comerciante", T("n.parada", st.Mark), s)); });
    (st.Traders || []).forEach(function (p) {
      if ((st.WhoGetsOff || []).indexOf(p.Seat) < 0) A("AVISO", T("n.puesto_no_baja", T("n.parada", st.Mark), p.Seat));
      var w = p.Waypoints && p.Waypoints[0] && p.Waypoints[0].Position;
      if (w && st.Pos) {
        var d = Math.hypot(w[0] - st.Pos[0], w[2] - st.Pos[1]);
        if (d > tope) A("AVISO", T("n.puesto_lejos", T("n.parada", st.Mark), p.Seat, r1(d), tope));
      }
      // la puerta de su plaza (del archivo de plazas del server) contra el puesto
      var pu = puertaDe(ctx.plazas, p.Seat);
      if (w && st.Pos && pu && (st.WhoGetsOff || []).indexOf(p.Seat) >= 0) {
        var lc = aLocal(st.Pos, st.HeadingDeg || 0, w[0], w[2]), pp = { r: lc.r, f: lc.f };
        if (dentroCaja(ctx.plazas.caja, pp, CASCO.ADENTRO_M)) A("AVISO", T("n.puesto_pegado", T("n.parada", st.Mark), p.Seat));
        else {
          // desde su salida (la medida, o la aproximacion de su puerta si no se midio). Si sale adentro de la
          // caja (el pasillo del bus), el mod va derecho: no hay rodeo que avisar; sin medir y con la entrada adentro, no se sabe
          var sa = salidaDe(ctx.plazas, p.Seat);
          if (sa && !sa.adentro && !sa.entradaAdentro) {
            var ro = rodeoCasco(ctx.plazas.caja, { r: sa.r, f: sa.f }, pp);
            if (ro.lado && ro.lado !== "?") A("AVISO", T("n.puesto_otro_lado", T("n.parada", st.Mark), p.Seat, sa.medida ? T("n.su_salida", ladoCaja(ctx.plazas.caja, sa)) : T("n.su_puerta", pu.lado || "?"), ro.lado));
          }
        }
      }
    });
    avisosSenales(svc, st, tope, A);
  });
  if (ctx.toma) {
    var tm = ctx.toma;
    if (tm.vehiculo && svc.VehicleClass && tm.vehiculo !== svc.VehicleClass) A("AVISO", T("n.vehiculo_distinto", svc.VehicleClass, tm.vehiculo));
    // el mundo de la toma (si AutoDrive lo escribe) contra el mapa elegido en el editor
    if (tm.mundo && ctx.mapa && ctx.mapa.sufijo) {
      var w = tm.mundo.toLowerCase(), s = ctx.mapa.sufijo.toLowerCase(), nm = String(ctx.mapa.nombre || "").toLowerCase();
      if (w !== s && w !== nm && w.indexOf(s) < 0 && s.indexOf(w) < 0) A("AVISO", T("n.otro_mapa", tm.mundo, ctx.mapa.nombre, ctx.mapa.sufijo));
    }
    (svc.Stops || []).forEach(function (st) {
      var m = tm.marcas[st.Mark - 1];
      if (!m) { A("AVISO", T("n.marca_falta", T("n.parada", st.Mark), tm.marcas.length)); return; }
      if (st.Pos && Math.hypot(st.Pos[0] - m.pos[0], st.Pos[1] - m.pos[1]) > 15) A("AVISO", T("n.marca_lejos", T("n.parada", st.Mark), r1(Math.hypot(st.Pos[0] - m.pos[0], st.Pos[1] - m.pos[1]))));
      if (finito(st.HeadingDeg) && difAngulo(st.HeadingDeg, m.heading) > 20) A("AVISO", T("n.rumbo_viejo", T("n.parada", st.Mark), st.HeadingDeg, m.heading));
    });
    if (tm.marcas.length > (svc.Stops || []).length) A("info", T("n.faltan_paradas", tm.marcas.length, (svc.Stops || []).length));
  }
  return out;
}

// Pos, rumbo y distancia de cada parada desde la toma (por numero de marca); agrega las paradas que falten. Los
// puestos (posiciones absolutas) no se mueven. Devuelve que cambio.
function reanclar(svc, tm) {
  var cambios = [];
  svc.Stops = svc.Stops || [];
  tm.marcas.forEach(function (m) {
    var st = svc.Stops.filter(function (s) { return s.Mark === m.numero; })[0];
    if (!st) { svc.Stops.push(paradaNueva(m)); cambios.push(T("n.r_agregada", m.numero, m.wp)); return; }
    if (!st.Pos || Math.hypot(st.Pos[0] - m.pos[0], st.Pos[1] - m.pos[1]) > 0.05) { st.Pos = m.pos.slice(); cambios.push(T("n.r_pos", m.numero)); }
    if (!finito(st.HeadingDeg) || difAngulo(st.HeadingDeg, m.heading) > 0.05) { st.HeadingDeg = m.heading; cambios.push(T("n.r_rumbo", m.numero)); }
    if (st.DistM !== m.dist) { st.DistM = m.dist; cambios.push(T("n.r_dist", m.numero)); }
  });
  svc.Stops.sort(function (a, b) { return a.Mark - b.Mark; });
  svc.VehicleClass = tm.vehiculo || svc.VehicleClass;
  svc.Editor = svc.Editor || { Version: 1, Seats: 0, RouteLengthM: 0, Start: [0, 0], End: [0, 0], Example: 0 };
  var largo = Math.round(tm.largo), ini = [r1(tm.inicio[0]), r1(tm.inicio[2])], fin = [r1(tm.fin[0]), r1(tm.fin[1])];
  if (svc.Editor.RouteLengthM !== largo) { svc.Editor.RouteLengthM = largo; cambios.push(T("n.r_largo")); }
  if (!mismoContenido(svc.Editor.Start, ini)) { svc.Editor.Start = ini; cambios.push(T("n.r_inicio")); }
  if (!mismoContenido(svc.Editor.End, fin)) { svc.Editor.End = fin; cambios.push(T("n.r_fin")); }
  return cambios;
}

// ------------------------------------------------------------------------------------------------ la ruta (texto)
// Lo que va a la ruta: DriverClass y Crew, desde los comerciantes del servicio. Los offsets van afuera del vehiculo
// (3 m a la derecha, uno adelante del otro cada 2 m).
function tripulacion(svc, driverDeLaRuta) {
  var lista = traders(svc), dc;
  var t0 = choferVende(svc) ? traderDe(svc, 0) : null;
  if (t0) dc = t0.TraderAIClass;
  else if (driverDeLaRuta && !/^ExpansionTraderAI/.test(driverDeLaRuta)) dc = driverDeLaRuta;
  else dc = BORIS;
  var crew = lista.filter(function (t) { return t.Seat >= 1; }).map(function (t, j) {
    return { cls: t.TraderAIClass, seat: t.Seat, faction: DEF.FACCION, loadout: t.Loadout || "SurvivorLoadout",
             offsetRight: 3.0, offsetForward: 0.5 - 2.0 * j };
  });
  return { driverClass: dc, crew: crew };
}

function f1(v) { var s = (Math.round(v * 10) / 10).toFixed(1); return s === "-0.0" ? "0.0" : s; }
function lineaCrew(c, sangria, coma) {
  return sangria + '{ "cls": ' + JSON.stringify(c.cls) + ', "seat": ' + c.seat + ', "faction": ' + JSON.stringify(c.faction) +
         ', "loadout": ' + JSON.stringify(c.loadout) + ', "offsetRight": ' + f1(c.offsetRight) + ', "offsetForward": ' + f1(c.offsetForward) + " }" + (coma ? "," : "");
}

// Desde la linea i (que tiene la clave), la linea donde se cierra su valor ([...] o {...}). -1 si no cierra.
function finDelValor(lineas, i) {
  var prof = 0, empezo = false, enCad = false;
  for (var k = i; k < lineas.length; k++) {
    var l = lineas[k], j = k === i ? l.indexOf(":") + 1 : 0;
    for (; j < l.length; j++) {
      var c = l[j];
      if (enCad) { if (c === "\\") { j++; continue; } if (c === '"') enCad = false; continue; }
      if (c === '"') { enCad = true; continue; }
      if (c === "[" || c === "{") { prof++; empezo = true; }
      else if (c === "]" || c === "}") { prof--; if (empezo && prof === 0) return k; }
    }
    if (!empezo && k > i) return -1;
  }
  return -1;
}

// ★ Escribe DriverClass y Crew en el texto de una ruta (_hdr.json o master .json), sin reserializar. Devuelve
// {texto, cambios, error}. El resultado se verifica: tiene que ser JSON valido y decir lo pedido.
function escribirTripulacion(texto, driverClass, crew) {
  var bom = texto.charCodeAt(0) === 0xFEFF, t = sinBom(texto), eol = nl(t), lineas = t.split(eol), cambios = [];
  if (lineas.length < 5) return { error: T("n.t_una_linea") };
  var iDC = -1, iCrew = -1, iVC = -1, sangria = null, i, m, RE = /^(\s*)"(DriverClass|Crew|VehicleClass)"\s*:/;
  for (i = 0; i < lineas.length; i++) {      // 1: la sangria del nivel de arriba (la menor de esas claves)
    m = RE.exec(lineas[i]);
    if (m && (sangria === null || m[1].length < sangria.length)) sangria = m[1];
  }
  if (sangria === null) sangria = "  ";
  for (i = 0; i < lineas.length; i++) {      // 2: las lineas de esas claves, solo en el nivel de arriba
    m = RE.exec(lineas[i]);
    if (!m || m[1] !== sangria) continue;
    if (m[2] === "DriverClass" && iDC < 0) iDC = i;
    if (m[2] === "Crew" && iCrew < 0) iCrew = i;
    if (m[2] === "VehicleClass" && iVC < 0) iVC = i;
  }
  // DriverClass
  if (iDC >= 0) {
    var md = /^(\s*"DriverClass"\s*:\s*)"((?:[^"\\]|\\.)*)"(\s*,?\s*)$/.exec(lineas[iDC]);
    if (!md) return { error: T("n.t_forma", lineas[iDC].trim()) };
    if (md[2] !== driverClass) { lineas[iDC] = md[1] + JSON.stringify(driverClass) + md[3]; cambios.push("DriverClass " + md[2] + " -> " + driverClass); }
  } else {
    if (iVC < 0) return { error: T("n.t_sin_dc") };
    lineas.splice(iVC + 1, 0, sangria + '"DriverClass": ' + JSON.stringify(driverClass) + ",");
    iDC = iVC + 1;
    if (iCrew > iVC) iCrew++;
    cambios.push(T("n.t_dc_agregado", driverClass));
  }
  // Crew
  var bloque = [];
  if (crew.length) {
    bloque.push(sangria + '"Crew": [');
    crew.forEach(function (c, k) { bloque.push(lineaCrew(c, sangria + sangria, k < crew.length - 1)); });
    bloque.push(sangria + "],");
  } else {
    bloque.push(sangria + '"Crew": [],');
  }
  if (iCrew >= 0) {
    var fin = finDelValor(lineas, iCrew);
    if (fin < 0) return { error: T("n.t_fin_crew") };
    var ultima = lineas[fin], cola = ultima.slice(ultima.lastIndexOf("]") + 1).trim();
    if (cola !== ",") bloque[bloque.length - 1] = bloque[bloque.length - 1].replace(/,$/, "");   // era la ultima clave
    var viejo = lineas.slice(iCrew, fin + 1).join(eol);
    if (viejo !== bloque.join(eol)) {
      lineas.splice(iCrew, fin - iCrew + 1);
      lineas.splice.apply(lineas, [iCrew, 0].concat(bloque));
      cambios.push(T("n.t_crew_reemplazado", crew.length));
    }
  } else if (crew.length) {
    lineas.splice.apply(lineas, [iDC + 1, 0].concat(bloque));
    cambios.push(T("n.t_crew_agregado", crew.length));
  }
  var nuevo = lineas.join(eol);
  // la verificacion: JSON valido, en varias lineas, con lo pedido
  var obj;
  try { obj = JSON.parse(nuevo); } catch (e) { return { error: T("n.t_no_json", e.message) }; }
  if (obj.DriverClass !== driverClass) return { error: T("n.t_ver_dc", driverClass) };
  var crewOk = (obj.Crew || []).length === crew.length && crew.every(function (c, k) { var o = obj.Crew[k]; return o && o.cls === c.cls && o.seat === c.seat; });
  if (!crewOk) return { error: T("n.t_ver_crew") };
  if (nuevo.split(eol).length < 5) return { error: T("n.t_ver_linea") };
  return { texto: (bom ? "﻿" : "") + nuevo, cambios: cambios, error: "" };
}

// ------------------------------------------------------------------------------------------------ la zona
// Una zona nueva del Market para el servicio. Nace SIN stock: al engancharse cada comerciante, el Market le agrega
// sus articulos con MaxStockThreshold x InitStockPercent / 100 de su categoria (ExpansionTraderBase.UpdateTraderZone,
// 1.9.79). El editor no decide precios ni cantidades.
function zonaNueva(nombre, pos3, radio, servicio) {
  return { m_Version: 6, m_DisplayName: "BZ_Trader " + servicio, Position: [pos3[0], pos3[1], pos3[2]], Radius: radio,
           BuyPricePercent: 100.0, SellPricePercent: -1.0, Stock: {} };
}
function textoZona(o) { return JSON.stringify(o, null, 4) + "\n"; }

// ------------------------------------------------------------------------------------------------ el server
// La linea de arranque del .bat: -profiles=, -mission=, -mod=, -serverMod= (con o sin comillas).
function argumentos(l) {      // separa por espacios fuera de comillas y saca las comillas
  var out = [], cur = "", dentro = false, hay = false;
  for (var i = 0; i < l.length; i++) {
    var c = l[i];
    if (c === '"') { dentro = !dentro; hay = true; continue; }
    if (!dentro && (c === " " || c === "\t")) { if (hay || cur) out.push(cur); cur = ""; hay = false; continue; }
    cur += c; hay = true;
  }
  if (hay || cur) out.push(cur);
  return out;
}
// Las variables del .bat en una linea: las que el mismo .bat define antes con set (vars, en minusculas), %~dp0 y %CD%
// (la carpeta del .bat, que es la del server: nada). Las que no estan quedan como estan y se anotan en faltan.
function expandirBat(l, vars, faltan) {
  return String(l).replace(/%~dp0\\?/gi, "").replace(/%%/g, "\u0001").replace(/%([^%\s]+)%/g, function (todo, n) {
    var k = n.toLowerCase();
    if (k === "cd") return "";
    if (Object.prototype.hasOwnProperty.call(vars, k)) return vars[k];
    if (faltan && faltan.indexOf(n) < 0) faltan.push(n);
    return todo;
  }).replace(/\u0001/g, "%%");
}
// set NOMBRE=valor o set "NOMBRE=valor" (no set /a ni set /p); el valor ya con las variables de antes
function setDeBat(l, vars) {
  var s = /^\s*@?set\s+(?!\/)(")?([^="]+)=(.*)$/i.exec(l);
  if (!s) return false;
  var v = s[1] ? s[3].replace(/"\s*$/, "") : s[3];
  vars[s[2].trim().toLowerCase()] = expandirBat(v, vars, null);
  return true;
}
function leerBat(texto) {
  var out = [], vars = {};
  String(texto || "").split(/\r?\n/).forEach(function (l) {
    if (setDeBat(l, vars)) return;
    if (!/DayZServer_x64\.exe/i.test(l)) return;
    var faltan = [], le = expandirBat(l, vars, faltan);
    var r = { profiles: "", mission: "", mods: [], serverMods: [], linea: l.trim(), sinResolver: faltan };
    argumentos(le).forEach(function (a) {
      var m = /^-(\w+)=(.*)$/.exec(a);
      if (!m) return;
      var k = m[1].toLowerCase(), v = m[2];
      if (k === "profiles") r.profiles = v;
      else if (k === "mission") r.mission = v;
      else if (k === "mod") r.mods = v.split(";").filter(Boolean);
      else if (k === "servermod") r.serverMods = v.split(";").filter(Boolean);
    });
    out.push(r);
  });
  return out;
}
// La hora de arranque del server, del nombre del RPT mas nuevo (DayZServer_x64_2026-10-17_08-14-56.RPT).
function arranqueDeRpt(nombres) {
  var mejor = null;
  (nombres || []).forEach(function (n) {
    var m = /DayZServer_x64_(\d{4})-(\d\d)-(\d\d)_(\d\d)-(\d\d)-(\d\d)\.RPT$/i.exec(n);
    if (!m) return;
    var d = new Date(+m[1], +m[2] - 1, +m[3], +m[4], +m[5], +m[6]);
    if (!mejor || d > mejor.fecha) mejor = { fecha: d, archivo: n };
  });
  return mejor;
}

// El mapa de una mision, por su sufijo (mpmissions\dayzOffline.chernarusplus -> Chernarus). Uno desconocido: el sufijo tal cual.
var MAPAS = { chernarusplus: "Chernarus", enoch: "Livonia", sakhal: "Sakhal", nasdara: "Badlands", namalsk: "Namalsk",
              deerisle: "Deer Isle", takistanplus: "Takistan", banov: "Banov", esseker: "Esseker", pnw: "PNW" };
function mapaDeMision(mision) {
  var n = String(mision || "").replace(/\\/g, "/").replace(/\/+$/, "").split("/").pop(), k = n.lastIndexOf(".");
  var suf = k >= 0 ? n.slice(k + 1) : n;
  return { sufijo: suf, nombre: MAPAS[suf.toLowerCase()] || suf };
}

// Las configuraciones de un server: una por (perfil, mision) de sus .bat (dos .bat iguales cuentan una vez), primero
// las que cargan @BZ_Trader. bats = [{archivo, profiles, mission, mods, serverMods}] (leerBat + el nombre del .bat).
function configuraciones(bats) {
  var out = [], idx = {};
  (bats || []).forEach(function (b) {
    if (!b.profiles || conVariables(b)) return;
    var clave = (b.profiles + "|" + b.mission).toLowerCase(), carga = (b.mods || []).concat(b.serverMods || []).some(function (m) { return /BZ_Trader/i.test(m); });
    if (idx[clave] === undefined) { idx[clave] = out.length; out.push({ perfil: b.profiles, mision: b.mission, bats: [], cargaTrader: false, mapa: mapaDeMision(b.mission) }); }
    var c = out[idx[clave]];
    c.bats.push(b.archivo);
    c.cargaTrader = c.cargaTrader || carga;
  });
  return out.sort(function (a, b) { return (b.cargaTrader ? 1 : 0) - (a.cargaTrader ? 1 : 0); });
}
// El perfil o la mision de un .bat con variables que el .bat no define (vienen de afuera): no se pueden leer.
function conVariables(b) { return /%[^%\s]+%/.test(String(b.profiles || "") + " " + String(b.mission || "")); }
// Los .bat cuyo perfil o mision no se pueden leer, con sus variables: [{archivo, variables}] (una vez por .bat)
function batsConVariables(bats) {
  var out = [], vistos = {};
  (bats || []).forEach(function (b) {
    if (!conVariables(b) || vistos[b.archivo]) return;
    vistos[b.archivo] = 1;
    out.push({ archivo: b.archivo, variables: (b.sinResolver || []).slice() });
  });
  return out;
}

// La que se usa sin preguntar: la recordada si sigue existiendo; si no, la UNICA que carga @BZ_Trader; si no, la unica
// que hay. null = hay que elegir (con un clic).
function configPorDefecto(confs, recordada) {
  if (recordada) {
    var r = confs.filter(function (c) { return c.perfil.toLowerCase() === String(recordada.perfil || "").toLowerCase() && c.mision.toLowerCase() === String(recordada.mision || "").toLowerCase(); })[0];
    if (r) return r;
  }
  var t = confs.filter(function (c) { return c.cargaTrader; });
  if (t.length === 1) return t[0];
  if (confs.length === 1) return confs[0];
  return null;
}
// Que carpeta eligio el admin, por lo que tiene adentro: "server" (DayZServer_x64.exe, mpmissions o un .bat),
// "perfil" (BZ_AutoDrive o ExpansionMod) u "otra". entradas = [{nombre, dir}].
function tipoDeCarpeta(entradas) {
  var n = {};
  (entradas || []).forEach(function (e) { n[e.nombre.toLowerCase()] = e.dir ? "d" : "f"; });
  if (n["dayzserver_x64.exe"] === "f" || n.mpmissions === "d") return "server";
  if (n.bz_autodrive === "d" || n.expansionmod === "d" || n.bz_trader === "d") return "perfil";
  return "otra";
}

// De que server es el editor, por su propia ruta: file:///<server>/@<mod>/tools/editor/<pagina> -> <server>. null para
// una copia suelta (fuera de un @<mod>\tools\editor) o la del cliente (!Workshop), o si no es file:.
function serverDeUbicacion(href) {
  var u, p;
  try { u = new URL(href); p = decodeURIComponent(u.pathname); } catch (e) { return null; }
  if (u.protocol !== "file:") return null;
  var m = /^(.*)\/(@[^\/]+)\/tools\/editor\/[^\/]*$/i.exec(p);
  if (!m) return null;
  var base = m[1].replace(/^\/([A-Za-z]:)/, "$1").replace(/\//g, "\\");
  if (u.hostname) base = "\\\\" + u.hostname + base;
  if (!base || /(^|\\)!Workshop$/i.test(base)) return null;
  if (/^[A-Za-z]:$/.test(base)) base += "\\";
  return datosServer(base, m[2]);
}
// El editor abierto como archivo FUERA de la raiz de un server, por su ruta: fuera de un <x>\@<mod>\tools\editor (la
// carpeta del Workshop, sin @; una copia suelta) o en la carpeta de mods del cliente (!Workshop). {donde: la carpeta del
// mod (o la de la pagina), mod: su nombre}; null si la ruta puede ser la de un server (lo de arriba se mira al elegir la
// carpeta: el navegador no deja leerla antes) o si no es file:.
function fueraDeUnServer(href) {
  var u, p;
  try { u = new URL(href); p = decodeURIComponent(u.pathname); } catch (e) { return null; }
  if (u.protocol !== "file:" || serverDeUbicacion(href)) return null;
  var m = /^(.*)\/([^\/]+)\/tools\/editor\/[^\/]*$/i.exec(p), dir = m ? m[1] + "/" + m[2] : p.replace(/\/[^\/]*$/, "");
  dir = dir.replace(/^\/([A-Za-z]:)/, "$1").replace(/\//g, "\\");
  if (u.hostname) dir = "\\\\" + u.hostname + dir;
  return { donde: dir, mod: m ? m[2] : "" };
}
// {ruta, nombre (la ultima carpeta), mod, clave (la ruta en minusculas, sin la barra final), programFiles}
function datosServer(ruta, mod) {
  ruta = String(ruta || "");
  var partes = ruta.split("\\").filter(Boolean);
  return { ruta: ruta, nombre: partes.length ? partes[partes.length - 1] : ruta, mod: mod || "@BZ_Trader",
           clave: ruta.toLowerCase().replace(/\\+$/, ""), programFiles: /\\program files( \(x86\))?(\\|$)/i.test(ruta) };
}
// La clave de la carpeta recordada (IndexedDB): una por server; una copia suelta usa la de siempre.
function claveCarpeta(srv) { return srv ? "server:" + srv.clave : "server"; }
// ¿La carpeta elegida es la del server del editor? El navegador da solo el NOMBRE de la carpeta, no la ruta: se compara
// el nombre (el editor mira ademas que adentro este su @<mod>\tools\editor). Sin server conocido, cualquiera sirve.
function esCarpetaDelServer(srv, nombre) {
  if (!srv) return true;
  function limpio(x) { return String(x || "").replace(/[\\\/:]+$/g, "").toLowerCase(); }
  return limpio(nombre) !== "" && limpio(nombre) === limpio(srv.nombre);
}

// Por que un .json de la carpeta de comerciantes NO sirve como comerciante del Market ("" = sirve). Lo que no sirve se
// muestra igual, en gris y con el porque: que no se pierda nadie en silencio.
function porQueNoEsComerciante(d) {
  if (!d || typeof d !== "object" || Array.isArray(d)) return T("n.c_no_objeto");
  if (!Array.isArray(d.Currencies)) return T("n.c_currencies");
  if (!Array.isArray(d.Categories) && !d.Items) return T("n.c_categories");
  return "";
}

// que los jugadores cerca no cambien la secuencia de la parada: la
// SECUENCIA FIJA = no estirar la venta ni la espera sentado por jugadores (MaxWithPlayersS 0) y no frenarse a mirar a
// un jugador mientras vende (PlayerHaltM 0). Apagarla borra las dos claves: vuelven al default del mod.
function esSecuenciaFija(svc) { return !!svc && svc.MaxWithPlayersS === 0 && svc.PlayerHaltM === 0; }
function secuenciaFija(svc, on) {
  if (on) { svc.MaxWithPlayersS = 0; svc.PlayerHaltM = 0; }
  else { delete svc.MaxWithPlayersS; delete svc.PlayerHaltM; }
}

// ---------------------------------------------------------------- el marcador de mapa
// Navigation de Expansion. El color va como TEXTO ("0".."255", el formato de TTE) y Alpha siempre "255"; sin marcador 3D. Al
// prenderlo: 2 s (si el archivo no traia uno valido) y 3D en 0. Apagarlo deja el resto (prenderlo otra vez lo recupera).
var MARCA_AL_PRENDER_S = 2;
var MARCA_COLORES = [{ k: "white", hex: "#FFFFFF" }, { k: "red", hex: "#E62828" }, { k: "orange", hex: "#FF8C00" },
                     { k: "yellow", hex: "#FFD700" }, { k: "green", hex: "#28C846" }, { k: "lightblue", hex: "#46BEFF" },
                     { k: "blue", hex: "#285AFF" }, { k: "purple", hex: "#AA46E6" }];
// como lo lee el mod: vacio o roto = 255 (BZTRMarkers.Completar y string.ToInt)
function canalMarcador(v) { var n = parseInt(v, 10); return isFinite(n) ? Math.max(0, Math.min(255, n)) : 255; }
function colorMarcador(svc) {
  var m = (svc && svc.Markers) || {};
  return "#" + [m.Red, m.Green, m.Blue].map(function (c) { var h = canalMarcador(c).toString(16).toUpperCase(); return h.length < 2 ? "0" + h : h; }).join("");
}
function ponerColorMarcador(svc, hex) {
  var m = /^#?([0-9a-f]{2})([0-9a-f]{2})([0-9a-f]{2})$/i.exec(String(hex || ""));
  if (!m) return false;
  svc.Markers = svc.Markers || {};
  svc.Markers.Red = String(parseInt(m[1], 16));
  svc.Markers.Green = String(parseInt(m[2], 16));
  svc.Markers.Blue = String(parseInt(m[3], 16));
  svc.Markers.Alpha = "255";
  return true;
}
function ponerMarcador(svc, on) {
  svc.Markers = svc.Markers || {};
  svc.Markers.useMapMarker = on ? 1 : 0;
  if (!on) return;
  if (!(typeof svc.Markers.MarkerUpdateSeconds === "number" && svc.Markers.MarkerUpdateSeconds >= DEF.MARCA_MIN_S)) svc.Markers.MarkerUpdateSeconds = MARCA_AL_PRENDER_S;
  svc.Markers.Expansion3DMarkers = 0;
  if (svc.Markers.BlinkAtStops === undefined) svc.Markers.BlinkAtStops = DEF.MARCA_PARPADEO;
  if (svc.Markers.Red === undefined && svc.Markers.Green === undefined && svc.Markers.Blue === undefined) ponerColorMarcador(svc, "#FFFFFF");
}

// las claves que ya no se usan y el editor saca al guardar (devuelve cuales saco)
function limpiarObsoletas(svc) {
  var sacadas = [];
  if (svc && svc.Wave && Object.prototype.hasOwnProperty.call(svc.Wave, "WhoGetsOff")) {
    delete svc.Wave.WhoGetsOff;   // por saludo nadie baja
    sacadas.push("Wave.WhoGetsOff");
  }
  // los comerciantes son siempre invencibles
  var inv = false;
  ((svc && svc.Traders) || []).forEach(function (x) {
    if (!x || (!Object.prototype.hasOwnProperty.call(x, "Invincible") && !Object.prototype.hasOwnProperty.call(x, "CanBeLooted"))) return;
    delete x.Invincible;
    delete x.CanBeLooted;
    inv = true;
  });
  if (inv) sacadas.push("Traders[].Invincible / CanBeLooted");
  // la faccion es siempre DEF.FACCION: el mod ignora Traders[].Faction. Vacia o igual a esa, el mod no dice nada (el
  // ejemplo del primer arranque la escribe asi): se deja como esta
  var fac = false;
  ((svc && svc.Traders) || []).forEach(function (x) {
    if (!x || !Object.prototype.hasOwnProperty.call(x, "Faction") || x.Faction === "" || x.Faction === DEF.FACCION) return;
    delete x.Faction;
    fac = true;
  });
  if (fac) sacadas.push("Traders[].Faction");
  // los vehiculos van siempre cerrados a jugadores: el mod ignora PlayersCanBoard
  if (svc && Object.prototype.hasOwnProperty.call(svc, "PlayersCanBoard")) {
    delete svc.PlayersCanBoard;
    sacadas.push("PlayersCanBoard");
  }
  // la pistola de bengalas ya no se ofrece: queda bengala de ruta (el mod la lee igual)
  var pistolas = 0;
  ((svc && svc.Stops) || []).forEach(function (st) {
    ((st && st.Signals) || []).forEach(function (sg) {
      var v = sg && SENAL_TIPOS_VIEJOS[String(sg.Type || "").toLowerCase()];
      if (!v) return;
      sg.Type = v;
      pistolas++;
    });
  });
  if (pistolas) sacadas.push({ msg: T("rev.pistola", pistolas) });
  return sacadas;
}

// Los tramos de la ruta donde un saludo puede hacer parar el vehiculo (como el mod): desde la cabecera no hay espera;
// despues de una marca que para, Wave.MinGapM; y antes de la proxima marca que para, max(MinGapM, 50 m). En metros sobre
// la ruta. {tramos: [[desde, hasta]...], total, largo, gap, marcas}
function tramosSaludo(svc) {
  var largo = Math.round((svc.Editor && svc.Editor.RouteLengthM) || 0), w = svc.Wave || {};
  var gap = typeof w.MinGapM === "number" ? Math.max(0, w.MinGapM) : DEF.WAVE_MIN_GAP_M, antes = Math.max(gap, 50);
  var marcas = marcasActivas(svc) ? (svc.Stops || []).filter(function (s) { return s.Mode === "always" && typeof s.DistM === "number"; })
    .map(function (s) { return Math.round(s.DistM); }).sort(function (a, b) { return a - b; }) : [];
  var cortes = [0].concat(marcas).concat([largo]), tramos = [], total = 0;
  for (var i = 0; i + 1 < cortes.length; i++) {
    var desde = i === 0 ? cortes[i] : cortes[i] + gap, hasta = i + 2 < cortes.length ? cortes[i + 1] - antes : cortes[i + 1];
    if (hasta > desde) { tramos.push([desde, hasta]); total += hasta - desde; }
  }
  return { tramos: tramos, total: total, largo: largo, gap: gap, marcas: marcas };
}

// Un archivo del Market reconocido por su contenido.
function comercianteDeArchivo(nombre, d) {
  if (porQueNoEsComerciante(d)) return null;
  return { archivo: nombre, nombre: d.DisplayName || "",
           categorias: (d.Categories || []).map(function (c) { var p = String(c).split(":"); return { nombre: p[0], modo: p.length > 1 ? +p[1] : 1 }; }),
           items: Object.keys(d.Items || {}), monedas: d.Currencies.slice() };
}
function categoriaDeArchivo(nombre, d) {
  if (!d || !Array.isArray(d.Items) || d.InitStockPercent === undefined) return null;
  return { archivo: nombre, nombre: d.DisplayName || "", init: d.InitStockPercent,
           items: d.Items.map(function (it) { return { clase: String(it.ClassName || "").toLowerCase(), max: it.MaxStockThreshold }; }) };
}
// Un loadout de Expansion: el ClassName de la raiz suele venir VACIO (es el cuerpo); lo que lo identifica son las
// listas Sets / InventoryAttachments.
function esLoadout(d) { return !!(d && typeof d === "object" && ("ClassName" in d) && (Array.isArray(d.Sets) || Array.isArray(d.InventoryAttachments))); }
function nombreVisible(t) {
  if (!t) return "";
  if (/^#STR_EXPANSION_MARKET_TRADER_/.test(t.nombre)) return t.archivo.replace(/_/g, " ") + " (Expansion)";
  return t.nombre || t.archivo;
}

return {
  idioma: idioma, T: T,
  DEF: DEF, ESQUEMA: ESQUEMA, LEIDAS_DEL_TEXTO: LEIDAS_DEL_TEXTO, PLAZAS: PLAZAS, BORIS: BORIS, CLASES_TRADER: CLASES_TRADER,
  copia: copia, r1: r1, sello: sello, nombreRespaldo: nombreRespaldo, nombreCorto: nombreCorto, sinBom: sinBom,
  leerWp: leerWp, distancias: distancias, marcasDeToma: marcasDeToma, toma: toma, plazasDe: plazasDe,
  ejes: ejes, aMundo: aMundo, aLocal: aLocal, difAngulo: difAngulo,
  marcasActivas: marcasActivas, usaSena: usaSena, traders: traders, traderDe: traderDe, choferVende: choferVende,
  maxComerciantes: maxComerciantes, servicioNuevo: servicioNuevo, paradaNueva: paradaNueva, traderNuevo: traderNuevo,
  plazaLibre: plazaLibre, agregarComerciante: agregarComerciante, quitarComerciante: quitarComerciante, nombresPorDefecto: nombresPorDefecto,
  ponerQuienManeja: ponerQuienManeja, cambiarPlazaEnParadas: cambiarPlazaEnParadas, tramosSaludo: tramosSaludo, puesto: puesto, puestoXZ: puestoXZ, ponerBaja: ponerBaja, moverPuesto: moverPuesto,
  puestoMasLejos: puestoMasLejos, radioNecesario: radioNecesario, exportar: exportar, textoServicio: textoServicio,
  mismoContenido: mismoContenido, validar: validar, avisos: avisos, reanclar: reanclar,
  tripulacion: tripulacion, escribirTripulacion: escribirTripulacion, zonaNueva: zonaNueva, textoZona: textoZona,
  leerBat: leerBat, arranqueDeRpt: arranqueDeRpt, comercianteDeArchivo: comercianteDeArchivo,
  mapaDeMision: mapaDeMision, configuraciones: configuraciones, configPorDefecto: configPorDefecto,
  tipoDeCarpeta: tipoDeCarpeta, porQueNoEsComerciante: porQueNoEsComerciante,
  expandirBat: expandirBat, batsConVariables: batsConVariables, serverDeUbicacion: serverDeUbicacion, fueraDeUnServer: fueraDeUnServer,
  datosServer: datosServer, claveCarpeta: claveCarpeta, esCarpetaDelServer: esCarpetaDelServer,
  categoriaDeArchivo: categoriaDeArchivo, esLoadout: esLoadout, nombreVisible: nombreVisible,
  esSecuenciaFija: esSecuenciaFija, secuenciaFija: secuenciaFija, limpiarObsoletas: limpiarObsoletas,
  MARCA_COLORES: MARCA_COLORES, MARCA_AL_PRENDER_S: MARCA_AL_PRENDER_S, colorMarcador: colorMarcador, ponerColorMarcador: ponerColorMarcador, ponerMarcador: ponerMarcador,
  CASCO: CASCO, plazasArchivo: plazasArchivo, puertaDe: puertaDe, dentroCaja: dentroCaja, cruzaCaja: cruzaCaja, rodeoCasco: rodeoCasco,
  posicionesPlazas: posicionesPlazas, distMinPlazas: distMinPlazas, PLAZA_MIN_M: PLAZA_MIN_M, asientosDudosos: asientosDudosos, PLAZA_FUERA_M: PLAZA_FUERA_M,
  distanciaMax: distanciaMax, SENAL_TIPOS: SENAL_TIPOS, SENAL_TIPOS_VIEJOS: SENAL_TIPOS_VIEJOS, SENAL_COLORES: SENAL_COLORES, SENAL_TRIANGULO_M: SENAL_TRIANGULO_M,
  senalTipo: senalTipo, senalColor: senalColor, usaHumo: usaHumo, claseSenal: claseSenal, senalNueva: senalNueva,
  agregarSenal: agregarSenal, quitarSenal: quitarSenal, moverSenal: moverSenal, tresSenales: tresSenales,
  rumboXZ: rumboXZ, rumboDePuesto: rumboDePuesto, ponerRumbo: ponerRumbo, semicirculo: semicirculo, mirandoALaCalle: mirandoALaCalle,
  ladoCaja: ladoCaja, adentroCaja: adentroCaja, salidaDe: salidaDe, vueltaDe: vueltaDe
};
});
