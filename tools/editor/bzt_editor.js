/* BZ_Trader — Traveling_Trader on Wheels · editor · LA PAGINA
 * La logica que escribe archivos esta en bzt_nucleo.js (con pruebas automaticas).
 * Aca: la carpeta del server, las listas, la cinta, las escenas, el inspector y guardar. */
(function () {
"use strict";
var N = window.BZTN;
// el editor en ingles por defecto, con espanol como opcion (EN / ES arriba a la derecha)
var I18N = window.BZTI18N;
var LANG = (function () { try { var l = localStorage.getItem("bzt_editor_lang"); return l === "es" ? "es" : "en"; } catch (e) { return "en"; } })();
N.idioma(LANG);
function T(k) { return I18N.t(LANG, k, Array.prototype.slice.call(arguments, 1)); }
// Un texto que se traduce al mostrarlo (lo que se arma al escanear la carpeta; el idioma puede cambiar despues).
function TL(k) { return { k: k, a: Array.prototype.slice.call(arguments, 1) }; }
function TX(x) {
  if (Array.isArray(x)) return x.map(TX).join("");
  if (typeof x === "function") return String(x());
  return x && typeof x === "object" && x.k ? I18N.t(LANG, x.k, x.a) : (x === undefined || x === null ? "" : String(x));
}
function $(id) { return document.getElementById(id); }
function esc(t) { return String(t === undefined || t === null ? "" : t).replace(/[&<>"']/g, function (c) { return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]; }); }
function num(v) { return (Math.round(v * 10) / 10).toString().replace(".", LANG === "es" ? "," : "."); }
function km(m) { return m >= 1000 ? num(m / 1000) + " km" : Math.round(m) + " m"; }
function leerLS(k, def) { try { var v = JSON.parse(localStorage.getItem(k) || "null"); return v === null ? def : v; } catch (e) { return def; } }
function guardarLS(k, v) { try { localStorage.setItem(k, JSON.stringify(v)); return true; } catch (e) { return false; } }
function norm(p) { return String(p || "").replace(/\\/g, "/").replace(/\/+/g, "/").replace(/^\.\//, "").replace(/^\/+|\/+$/g, ""); }
function unir(a, b) { a = norm(a); b = norm(b); return a && b ? a + "/" + b : (a || b); }
function hora(d) { function p(x) { return (x < 10 ? "0" : "") + x; } return p(d.getDate()) + "/" + p(d.getMonth() + 1) + " " + p(d.getHours()) + ":" + p(d.getMinutes()); }
function aviso(id, t) { $(id).textContent = t; }

// ============================================================================ el server de este editor
// El editor viaja en el server: file:///<server>/@BZ_Trader/tools/editor/editor_bz_trader.html. Con Editor.bat lo dice la
// API local (solo en esta PC). null = una copia suelta: se elige cualquier carpeta, como siempre.
var SERVIDOR = N.serverDeUbicacion(location.href);
// El editor fuera de la raiz de un server (la carpeta del Workshop, la del cliente, una copia suelta, o un @BZ_Trader sin
// DayZServer_x64.exe ni mpmissions al lado): {donde, mod}, y mientras este no se escanea nada. Lo dice la ruta al abrir;
// con Editor.bat, la API; con el .html en una ruta que puede ser un server, su carpeta al elegirla.
var RAIZ = N.fueraDeUnServer(location.href);

// ============================================================================ la carpeta del server
// Con acceso (Chrome/Edge, File System Access): se lee y se escribe ahi. Sin acceso (Firefox, o una copia): se lee la
// carpeta elegida con un input y lo que haya que escribir se descarga. Con Editor.bat ("api"): se lee y se escribe por
// la API local, sin elegir nada. El resultado es el mismo archivo.
var FS = (function () {
  var raiz = null, indice = null, modo = "ninguno", nombre = "", api = null;
  // un pedido a la API de Editor.bat: la clave de la sesion va en cada uno
  function pedir(metodo, que, rel, cuerpo, extra) {
    return fetch(api.base + "api/" + que + "?ruta=" + encodeURIComponent(norm(rel)) + (extra || ""),
                 { method: metodo, headers: { "X-BZT-Key": api.clave }, body: cuerpo, cache: "no-store" });
  }
  async function hijo(h, n, tipo) {
    try { return tipo === "dir" ? await h.getDirectoryHandle(n) : await h.getFileHandle(n); } catch (e) { /* sigue: sin distinguir mayusculas */ }
    for await (var par of h.entries()) {
      if (par[0].toLowerCase() === n.toLowerCase() && par[1].kind === (tipo === "dir" ? "directory" : "file")) return par[1];
    }
    return null;
  }
  async function dir(rel, crear) {
    var h = raiz, partes = norm(rel).split("/").filter(Boolean);
    for (var i = 0; i < partes.length; i++) {
      var s = await hijo(h, partes[i], "dir");
      if (!s && crear) s = await h.getDirectoryHandle(partes[i], { create: true });
      if (!s) return null;
      h = s;
    }
    return h;
  }
  function partir(rel) { var p = norm(rel), k = p.lastIndexOf("/"); return { dir: k >= 0 ? p.slice(0, k) : "", archivo: p.slice(k + 1) }; }
  return {
    modo: function () { return modo; },
    nombre: function () { return nombre; },
    escribe: function () { return modo === "acceso" || modo === "api"; },
    handle: function () { return raiz; },
    // soloLectura: el admin no dio permiso de escritura; se lee igual y al guardar Chrome pregunta donde (startIn)
    usarHandle: function (h, soloLectura) { raiz = h; indice = null; api = null; modo = soloLectura ? "lecturaHandle" : "acceso"; nombre = h.name; },
    // con Editor.bat: {base, clave, nombre}
    usarApi: function (a) { api = a; raiz = null; indice = null; modo = "api"; nombre = a.nombre; },
    // la carpeta (handle) de una ruta relativa, para el startIn de los cuadros; null si no hay
    dirHandle: async function (rel) { if (!raiz) return null; try { return await dir(rel); } catch (e) { return null; } },
    existe: async function (rel) {
      rel = norm(rel);
      if (modo === "api") { try { return (await pedir("GET", "listar", rel)).ok; } catch (e) { return false; } }
      if (raiz) { try { return !!(await dir(rel)); } catch (e) { return false; } }
      if (indice) { var pre = rel.toLowerCase() + "/"; return Object.keys(indice).some(function (k) { return k.indexOf(pre) === 0; }); }
      return false;
    },
    usarLista: function (files) {
      indice = {}; raiz = null; api = null; modo = "lectura"; nombre = "";
      Array.prototype.forEach.call(files, function (f) {
        var r = norm(f.webkitRelativePath || f.name), k = r.indexOf("/");
        if (!nombre) nombre = k >= 0 ? r.slice(0, k) : "";
        indice[(k >= 0 ? r.slice(k + 1) : r).toLowerCase()] = f;
      });
    },
    // [{nombre, dir}] de una carpeta (sin recursion)
    listar: async function (rel) {
      var out = [];
      rel = norm(rel);
      if (modo === "api") {
        var rl = await pedir("GET", "listar", rel);
        if (!rl.ok) return out;
        (await rl.json()).forEach(function (e) { out.push({ nombre: e.n, dir: !!e.d }); });
        return out;
      }
      if (raiz) {
        var d = await dir(rel);
        if (!d) return out;
        for await (var par of d.entries()) out.push({ nombre: par[0], dir: par[1].kind === "directory" });
        return out;
      }
      if (modo === "lectura") {
        var pre = rel ? rel.toLowerCase() + "/" : "", vistos = {};
        Object.keys(indice).forEach(function (k) {
          if (k.indexOf(pre) !== 0) return;
          var resto = k.slice(pre.length), j = resto.indexOf("/");
          var n = j >= 0 ? resto.slice(0, j) : resto, f = indice[k];
          if (vistos[n]) return;
          vistos[n] = 1;
          var real = j >= 0 ? n : f.name;
          out.push({ nombre: real, dir: j >= 0 });
        });
      }
      return out;
    },
    // el File (o null). max: con la API, trae solo los primeros max bytes (el RPT del server puede ser enorme)
    archivo: async function (rel, max) {
      if (modo === "api") {
        var ra = await pedir("GET", "leer", rel, undefined, max ? "&max=" + max : "");
        if (!ra.ok) return null;
        return new File([await ra.blob()], partir(rel).archivo, { lastModified: +(ra.headers.get("X-Mtime") || 0) });
      }
      if (raiz) {
        var p = partir(rel), d = await dir(p.dir);
        if (!d) return null;
        var h = await hijo(d, p.archivo, "file");
        return h ? await h.getFile() : null;
      }
      if (modo === "lectura") return indice[norm(rel).toLowerCase()] || null;
      return null;
    },
    leer: async function (rel) {
      var f = await this.archivo(rel);
      if (!f) return null;
      return { texto: await f.text(), mtime: f.lastModified, nombre: f.name };
    },
    escribir: async function (rel, texto) {
      if (modo === "api") {
        var rw = await pedir("POST", "escribir", rel, texto);
        if (!rw.ok) {
          var j = {};
          try { j = await rw.json(); } catch (e) { /* sin cuerpo */ }
          var err = new Error(j.detalle || ("HTTP " + rw.status));
          err.sinPermiso = j.error === "sin_permiso";
          throw err;
        }
        var leido = await this.leer(rel);
        if (!leido || leido.texto !== texto) throw new Error(T("fs.no_coincide"));
        return true;
      }
      if (modo !== "acceso") throw new Error(T("fs.sin_acceso"));
      var p = partir(rel), d = await dir(p.dir, true);
      var h = await d.getFileHandle(p.archivo, { create: true });
      var w = await h.createWritable();
      await w.write(texto);
      await w.close();
      var vuelta = await (await h.getFile()).text();
      if (vuelta !== texto) throw new Error(T("fs.no_coincide"));
      return true;
    }
  };
})();

// El handle de la carpeta queda guardado en IndexedDB: en la sesion siguiente se pide el permiso con un clic. Uno POR
// SERVER (la clave es la ruta del server del editor): el editor de un server no carga la carpeta recordada de otro.
function idb() {
  return new Promise(function (ok, mal) {
    var r = indexedDB.open("bzt_editor", 1);
    r.onupgradeneeded = function () { r.result.createObjectStore("h"); };
    r.onsuccess = function () { ok(r.result); };
    r.onerror = function () { mal(r.error); };
  });
}
async function guardarHandle(h) {
  var k = N.claveCarpeta(SERVIDOR);
  try { var db = await idb(); await new Promise(function (ok) { var tx = db.transaction("h", "readwrite"); tx.objectStore("h").put(h, k); tx.oncomplete = ok; tx.onerror = ok; }); } catch (e) { /* sin IndexedDB: se pide cada vez */ }
}
async function cargarHandle() {
  var k = N.claveCarpeta(SERVIDOR);
  try { var db = await idb(); return await new Promise(function (ok) { var tx = db.transaction("h", "readonly"), q = tx.objectStore("h").get(k); q.onsuccess = function () { ok(q.result || null); }; q.onerror = function () { ok(null); }; }); } catch (e) { return null; }
}

// ============================================================================ el server: un mapa a la vez
// se elige la carpeta del SERVER y nada mas. El perfil y la mision salen de un .bat (la configuracion de
// un mapa); lo demas es el arbol fijo de siempre, escaneado. "Avanzado" deja escribir otra carpeta para un caso raro.
var CFS = leerLS("bzt_editor_cf_v2", {});      // por carpeta de server: {conf: {perfil, mision, bats}, y lo de Avanzado}
var CF = {};
// la carpeta del server del editor se recuerda por su RUTA; otra carpeta, por su nombre (lo viejo, por nombre, se hereda)
function claveCF() { return SERVIDOR && SRV.esDelEditor ? "srv:" + SERVIDOR.clave : FS.nombre(); }
function cargarCF() { CF = Object.assign({}, CFS[claveCF()] || CFS[FS.nombre()] || {}); }
function guardarCF() { CFS[claveCF()] = CF; guardarLS("bzt_editor_cf_v2", CFS); }
var ARBOL = { rutas: "BZ_AutoDrive", traders: "ExpansionMod/Traders", market: "ExpansionMod/Market", loadouts: "ExpansionMod/Loadouts", config: "BZ_Trader" };
function perfilRel() { return SRV.tipo === "perfil" ? "" : norm((CF.conf && CF.conf.perfil) || ""); }
function misionRel() { return SRV.tipo === "perfil" ? "" : norm((CF.conf && CF.conf.mision) || ""); }
function carpetaDef(k) {
  if (ARBOL[k]) return unir(perfilRel(), ARBOL[k]);
  if (k === "zonas") return misionRel() ? unir(misionRel(), "expansion/traderzones") : "";
  if (k === "perfil") return perfilRel();
  if (k === "mision") return misionRel();
  return "";
}
function carpeta(k) { return CF[k] ? norm(CF[k]) : carpetaDef(k); }
// una ruta para mostrar: la carpeta del server, con barras de Windows
function ver(rel) { var r = norm(rel).replace(/\//g, "\\"); return (FS.nombre() ? FS.nombre() + (r ? "\\" : "") : "") + r; }
function dirDe(rel) { var p = norm(rel), k = p.lastIndexOf("/"); return k >= 0 ? p.slice(0, k) : ""; }

// ============================================================================ lo leido del server
function srvVacio() {
  return { leido: false, tipo: "", confs: [], eligiendo: false, perfiles: [], misiones: [], tomas: [], servicios: [], comerciantes: {},
           tradersMalos: [], categorias: {}, loadouts: [], zonas: {}, hardline: false, hardlineDe: "", arranque: null, rptMtime: 0,
           caliente: null, mod: null, arbol: [], mapaNav: null, esDelEditor: false, mapas: [], batsVariables: [] };
}
var SRV = srvVacio();

async function leerJson(rel) {
  var r = await FS.leer(rel);
  if (!r) return null;
  try { return { obj: JSON.parse(N.sinBom(r.texto)), texto: r.texto, mtime: r.mtime }; } catch (e) { return { obj: null, texto: r.texto, mtime: r.mtime, error: e.message }; }
}
async function jsons(rel) { return (await FS.listar(rel)).filter(function (e) { return !e.dir && /\.json$/i.test(e.nombre); }).map(function (e) { return e.nombre; }); }

// 1) que carpeta eligio el admin y sus configuraciones (los .bat); 2) con una configuracion, se escanea su arbol
async function leerServer() {
  SRV = srvVacio();
  aviso("srv-estado", T("srv.leyendo", FS.nombre()));
  // ¿es la carpeta del server de este editor? (con Editor.bat, siempre) Si no, se avisa y se sigue igual
  SRV.esDelEditor = FS.modo() === "api" || (!!SERVIDOR && N.esCarpetaDelServer(SERVIDOR, FS.nombre()) && (await FS.existe(unir(SERVIDOR.mod, "tools/editor"))));
  cargarCF();
  var raiz = await FS.listar(""), i;
  SRV.tipo = N.tipoDeCarpeta(raiz);
  // la carpeta de este mismo editor, y no es un server: su @BZ_Trader no esta en la raiz de uno
  if (SRV.esDelEditor && FS.modo() !== "api" && SRV.tipo !== "server") {
    RAIZ = { donde: SERVIDOR.ruta.replace(/\\+$/, "") + "\\" + SERVIDOR.mod, mod: SERVIDOR.mod };
    SRV = srvVacio(); renderServer(); renderListas(); todo(); return;
  }
  if (SRV.tipo === "otra") { renderServer(); renderListas(); todo(); return; }
  if (SRV.tipo === "server") {
    var bats = [];
    for (i = 0; i < raiz.length; i++) {
      if (raiz[i].dir || !/\.bat$/i.test(raiz[i].nombre)) continue;
      var b = await FS.leer(raiz[i].nombre), nb = raiz[i].nombre;
      N.leerBat(b ? b.texto : "").forEach(function (x) { x.archivo = nb; bats.push(x); });
    }
    SRV.confs = N.configuraciones(bats);
    SRV.batsVariables = N.batsConVariables(bats);
    // para el resumen del escaneo: las tomas y los servicios de cada mapa que carga @BZ_Trader (si ninguno, de todos)
    var deTrader = SRV.confs.filter(function (c) { return c.cargaTrader; });
    SRV.mapas = deTrader.length ? deTrader : SRV.confs.slice();
    for (i = 0; i < SRV.mapas.length; i++) {
      var cm = SRV.mapas[i];
      cm.tomas = (await FS.listar(unir(cm.perfil, ARBOL.rutas))).filter(function (e) { return !e.dir && /^BZBusRoute_.+_hdr\.json$/i.test(e.nombre); }).length;
      cm.servicios = (await jsons(unir(cm.perfil, ARBOL.config))).length;
    }
    // los perfiles (carpetas de la raiz con BZ_AutoDrive o ExpansionMod) y las misiones: para elegir a mano, con un clic
    for (i = 0; i < raiz.length; i++) {
      var n = raiz[i].nombre;
      if (!raiz[i].dir || n.charAt(0) === "@" || /^(mpmissions|addons|dta|keys|battleye|bliss|docs|logs|_)/i.test(n)) continue;
      if (N.tipoDeCarpeta(await FS.listar(n)) === "perfil") SRV.perfiles.push(n);
    }
    SRV.perfiles.sort();
    SRV.misiones = (await FS.listar("mpmissions")).filter(function (e) { return e.dir; }).map(function (e) { return "mpmissions\\" + e.nombre; }).sort();
    var conf = null;
    if (CF.conf && CF.conf.perfil && (await FS.existe(CF.conf.perfil))) conf = CF.conf;
    else conf = N.configPorDefecto(SRV.confs, null);
    if (!conf) { SRV.eligiendo = true; renderServer(); renderListas(); todo(); return; }
    if (conf !== CF.conf) { CF.conf = { perfil: conf.perfil, mision: conf.mision, bats: conf.bats || [] }; guardarCF(); }
  }
  await escanear();
}
async function elegirConf(perfil, mision, bats) {
  CF.conf = { perfil: perfil, mision: mision, bats: bats || [] }; guardarCF();
  SRV.eligiendo = false;
  await escanear();
}

async function escanear() {
  var k, i, arbol = [];
  SRV.tomas = []; SRV.servicios = []; SRV.comerciantes = {}; SRV.tradersMalos = []; SRV.categorias = {}; SRV.loadouts = []; SRV.zonas = {};
  SRV.hardline = false; SRV.hardlineDe = ""; SRV.arranque = null; SRV.caliente = null; SRV.mod = null; SRV.mapaNav = null;
  async function rama(que, k2, cuenta) { var rel = carpeta(k2), e = rel === "" ? false : await FS.existe(rel); arbol.push({ que: que, rel: rel, existe: e, cuenta: cuenta }); return e; }
  // las tomas convertidas y los servicios
  (await FS.listar(carpeta("rutas"))).forEach(function (e) {
    var m = /^BZBusRoute_(.+)_hdr\.json$/i.exec(e.nombre);
    if (m && !e.dir) SRV.tomas.push(m[1]);
  });
  SRV.tomas.sort();
  SRV.servicios = (await jsons(carpeta("config"))).sort();
  // las plazas y puertas de cada vehiculo que el mod ya vio (BZ_Trader\_plazas\<clase>.json)
  SRV.plazas = {};
  var dirPl = unir(carpeta("config"), "_plazas"), listaPl = await jsons(dirPl);
  for (k = 0; k < listaPl.length; k++) {
    var jp = await leerJson(unir(dirPl, listaPl[k])), pla = N.plazasArchivo(jp && jp.obj);
    if (pla && pla.clase) SRV.plazas[pla.clase.toLowerCase()] = pla;
  }
  // el Market: comerciantes (los que no sirven, tambien, con el porque), categorias, loadouts
  var carpetasTr = [carpeta("traders")].concat(carpeta("traders2") ? [carpeta("traders2")] : []);
  for (var c = 0; c < carpetasTr.length; c++) {
    var lista = await jsons(carpetasTr[c]);
    for (k = 0; k < lista.length; k++) {
      var j = await leerJson(unir(carpetasTr[c], lista[k])), nom = lista[k].replace(/\.json$/i, "");
      if (!j || !j.obj) { SRV.tradersMalos.push({ archivo: nom, carpeta: carpetasTr[c], porque: [TL("srv.json_roto"), j && j.error ? ": " + j.error : ""] }); continue; }
      var porque = N.porQueNoEsComerciante(j.obj);
      if (porque) { SRV.tradersMalos.push({ archivo: nom, carpeta: carpetasTr[c], porque: (function (o) { return function () { return N.porQueNoEsComerciante(o); }; })(j.obj) }); continue; }
      var t = N.comercianteDeArchivo(nom, j.obj);
      t.mtime = j.mtime; t.carpeta = carpetasTr[c]; SRV.comerciantes[t.archivo.toLowerCase()] = t;
    }
  }
  var cats = await jsons(carpeta("market"));
  for (k = 0; k < cats.length; k++) {
    var jc = await leerJson(unir(carpeta("market"), cats[k]));
    var cat = jc && N.categoriaDeArchivo(cats[k].replace(/\.json$/i, ""), jc.obj);
    if (cat) { cat.mtime = jc.mtime; SRV.categorias[cat.archivo.toLowerCase()] = cat; }
  }
  var los = await jsons(carpeta("loadouts"));
  for (k = 0; k < los.length; k++) {
    var jl = await leerJson(unir(carpeta("loadouts"), los[k]));
    if (jl && N.esLoadout(jl.obj)) SRV.loadouts.push(los[k].replace(/\.json$/i, ""));
  }
  SRV.loadouts.sort();
  if (carpeta("zonas")) {
    var zs = await FS.listar(carpeta("zonas"));
    for (k = 0; k < zs.length; k++) {
      if (zs[k].dir || !/\.json$/i.test(zs[k].nombre)) continue;
      var fz = await FS.archivo(unir(carpeta("zonas"), zs[k].nombre));
      SRV.zonas[zs[k].nombre.replace(/\.json$/i, "").toLowerCase()] = { nombre: zs[k].nombre.replace(/\.json$/i, ""), mtime: fz ? fz.lastModified : 0 };
    }
  }
  // la reputacion de la Hardline (la de la mision y la del perfil)
  var hs = [misionRel() ? unir(misionRel(), "expansion/settings/HardlineSettings.json") : "", unir(perfilRel(), "ExpansionMod/Settings/HardlineSettings.json")].filter(Boolean), hallada = "";
  for (k = 0; k < hs.length; k++) {
    var jh = await leerJson(hs[k]);
    if (jh && jh.obj) { hallada = hallada || hs[k]; if (jh.obj.UseReputation === 1) { SRV.hardline = true; SRV.hardlineDe = hs[k]; } }
  }
  // el mapa de Navigation (para el marcador): el MapSettings.json de la mision. Sin la mision no se sabe.
  if (misionRel()) {
    var jm = await leerJson(unir(misionRel(), "expansion/settings/MapSettings.json"));
    SRV.mapaNav = jm && jm.obj ? { existe: true, servidor: jm.obj.EnableServerMarkers, nombres: jm.obj.ShowNameOnServerMarkers } : { existe: false };
  }
  // el ultimo arranque (el RPT mas nuevo del perfil) y si su BZ_Trader recarga los servicios en caliente
  var rpts = (await FS.listar(perfilRel())).filter(function (e) { return !e.dir && /\.RPT$/i.test(e.nombre); }).map(function (e) { return e.nombre; });
  SRV.arranque = N.arranqueDeRpt(rpts);
  if (SRV.arranque) {
    var frpt = await FS.archivo(unir(perfilRel(), SRV.arranque.archivo), 48 * 1024 * 1024);
    if (frpt) {
      SRV.rptMtime = frpt.lastModified;
      var txt = await frpt.slice(0, 48 * 1024 * 1024).text();
      SRV.mod = txt.indexOf("[BZ_Trader]") >= 0;
      SRV.caliente = txt.indexOf("los archivos se releen cada") >= 0;
    }
  }
  // el arbol, como estado (encontrada o no, con cuantos)
  await rama(TL("arbol.tomas"), "rutas", TL("arbol.n_tomas", SRV.tomas.length));
  SRV.sinCarpetaTrader = !(await rama(TL("arbol.servicios"), "config", TL("arbol.n_servicios", SRV.servicios.length)));
  var nPl = Object.keys(SRV.plazas).length;
  arbol.push({ que: TL("arbol.plazas"), rel: unir(carpeta("config"), "_plazas"), existe: nPl ? true : null,
               cuenta: nPl ? TL("arbol.n_plazas", nPl) : TL("arbol.plazas_ninguno") });
  await rama(TL("arbol.comerciantes"), "traders", [TL("arbol.n_comerciantes", Object.keys(SRV.comerciantes).length), SRV.tradersMalos.length ? TL("arbol.n_malos", SRV.tradersMalos.length) : ""]);
  if (carpeta("traders2")) await rama(TL("arbol.traders2"), "traders2", "");
  await rama(TL("arbol.categorias"), "market", TL("arbol.n_categorias", Object.keys(SRV.categorias).length));
  await rama(TL("arbol.loadouts"), "loadouts", TL("arbol.n_loadouts", SRV.loadouts.length));
  if (carpeta("zonas")) await rama(TL("arbol.zonas_mision"), "zonas", TL("arbol.n_zonas", Object.keys(SRV.zonas).length));
  else arbol.push({ que: TL("arbol.zonas"), rel: "", existe: null, cuenta: TL("arbol.zonas_sin_mision") });
  arbol.push({ que: "Hardline", rel: hallada, existe: hallada ? true : null, cuenta: hallada ? (SRV.hardline ? "⚠ UseReputation = 1" : TL("arbol.rep_apagada")) : TL("arbol.no_encontrada") });
  SRV.arbol = arbol;
  SRV.leido = true;
  renderServer();
  renderListas();
  if (S) { await cargarTomaDelServicio(); todo(); }
}

// ============================================================================ el estado del servicio
var S = null;          // {svc, archivo, original, toma, sucio}
var nodoSel = "A", compSel = null;
var PLAZAS_RECORDADAS = leerLS("bzt_editor_plazas_v1", {});

function abrirServicio(svc, archivo, tm) {
  S = { svc: svc, archivo: archivo || "", original: N.copia(svc), toma: tm || null, sucio: false };
  if (!S.svc.Editor) S.svc.Editor = { Version: 1, Seats: 0, RouteLengthM: 0, Start: [0, 0], End: [0, 0], Example: 0 };
  // lo que ya no se usa sale de la copia de trabajo (queda "sin guardar"; la revision lo dice hasta guardar)
  S.sacadas = N.limpiarObsoletas(S.svc);
  if (S.sacadas.length) cambio();
  nodoSel = "A";
  var ts = N.traders(S.svc);
  compSel = ts.length ? ts[0].Seat : null;
  todo();
}
function cambio() { if (S) S.sucio = !N.mismoContenido(N.exportar(S.svc), S.original); }

// se elige un solo archivo: UNA eleccion carga
// la toma entera de la carpeta de rutas: el _hdr.json, el _wp.csv y el master .json. Lo que falte queda en
// TOMA_FALTA (sin hdr o wp no hay toma) o en tm.master (sin el master, la tripulacion va solo al _hdr.json).
var TOMA_FALTA = "";
async function tomaDeServer(nombre) {
  var base = unir(carpeta("rutas"), "BZBusRoute_" + nombre), faltan = [];
  var h = await leerJson(base + "_hdr.json"), w = await FS.leer(base + "_wp.csv");
  if (!h || !h.obj) faltan.push("BZBusRoute_" + nombre + "_hdr.json" + (h && h.error ? " (" + h.error + ")" : ""));
  if (!w) faltan.push("BZBusRoute_" + nombre + "_wp.csv");
  TOMA_FALTA = faltan.length ? (function (d) { return function () { return T("toma.no_entera", nombre, d, faltan.join(T("comun.y"))); }; })(ver(carpeta("rutas"))) : "";
  if (faltan.length) return null;
  var tm = N.toma(nombre, h.obj, w.texto);
  tm.master = !!(await FS.archivo(base + ".json"));
  return tm;
}
async function cargarTomaDelServicio() {
  TOMA_FALTA = "";
  if (!S || !S.svc.Route || FS.modo() === "ninguno") return;
  try { S.toma = await tomaDeServer(N.nombreCorto(S.svc.Route)); }
  catch (e) { S.toma = null; TOMA_FALTA = (function (r, m) { return function () { return T("toma.no_se_pudo", r, m); }; })(N.nombreCorto(S.svc.Route), e.message); }
}
function sinMaster(tm) { return tm && tm.master === false ? T("toma.sin_master", tm.nombre, ver(carpeta("rutas"))) : ""; }
function plazasDelServicio() { return (S.svc.Editor && S.svc.Editor.Seats) || 0; }
// el archivo de plazas del server para una clase de vehiculo (null si el mod todavia no la vio)
function plazasVeh(clase) { return clase && SRV.plazas ? SRV.plazas[String(clase).toLowerCase()] || null : null; }
function nPlazasVeh(clase) { var pl = plazasVeh(clase); return pl ? pl.n : 0; }
function rumboTxt(h) { return Math.round(((h % 360) + 360) % 360) + "°"; }

// ============================================================================ cabecera, server y listas
function renderChips() {
  var h = "";
  if (S) {
    var svc = S.svc;
    if (CF.conf && CF.conf.mision) h += '<span class="chip">' + T("chip.mapa") + ' <b>' + esc(N.mapaDeMision(CF.conf.mision).nombre) + '</b></span>';
    h += '<span class="chip">' + T("chip.servicio") + ' <b class="mono">' + esc(svc.Name) + '</b></span>';
    h += '<span class="chip">' + T("chip.vehiculo") + ' <b class="mono">' + esc(svc.VehicleClass || "?") + '</b> · ' + T("chip.plazas", plazasDelServicio()) + '</span>';
    if (S.toma) h += '<span class="chip">' + T("chip.marcas", S.toma.marcas.length) + ' · ' + km(S.toma.largo) + '</span>';
    else h += '<span class="chip warn">' + T("chip.sin_toma") + '</span>';
    if (S.sucio) h += '<span class="chip warn">' + T("chip.sin_guardar") + '</span>';
  }
  $("chips").innerHTML = h;
}
// La linea del escaneo: server <ruta> · N mapa(s) (...) · K toma(s) · S servicio(s), de los mapas que cargan @BZ_Trader.
function resumenEscaneo() {
  var donde = SRV.esDelEditor && SERVIDOR ? SERVIDOR.ruta : FS.nombre(), nombres = [], k = 0, s = 0;
  if (SRV.tipo === "perfil") return T("srv.resumen", donde, 1, FS.nombre(), SRV.tomas.length, SRV.servicios.length);
  if (!SRV.mapas.length) return T("srv.resumen", donde, 1, CF.conf ? N.mapaDeMision(CF.conf.mision).nombre || CF.conf.perfil : "-", SRV.tomas.length, SRV.servicios.length);
  SRV.mapas.forEach(function (c) { nombres.push(c.mapa.nombre || c.perfil); k += c.tomas || 0; s += c.servicios || 0; });
  return T("srv.resumen", donde, SRV.mapas.length, nombres.join(", "), k, s);
}
// Los .bat que usan variables que no se pueden leer (no las define el mismo .bat): dicen que se elija a mano.
function batsVariablesHtml() {
  if (!SRV.batsVariables.length) return "";
  var l = SRV.batsVariables.map(function (b) { return b.archivo + (b.variables.length ? " (" + b.variables.map(function (v) { return "%" + v + "%"; }).join(", ") + ")" : ""); });
  return '<span class="hint warn">' + esc(T("srv.bat_variables", l.join(", "))) + '</span>';
}
function renderServer() {
  var m = FS.modo(), partes = [];
  // «Escanear server»: siempre a la vista; la primera vez elige la carpeta y despues la vuelve a leer
  $("srv-escanear").textContent = T("srv.escanear");
  // Firefox: leer la carpeta (lo que se escribe, se descarga). En Chrome, a la vista mientras no haya carpeta: Chrome no
  // deja elegir nada dentro de Program Files ("contiene archivos del sistema"), y el server de Steam esta ahi.
  $("srv-lectura-btn").hidden = m === "api" || (!!window.showDirectoryPicker && m !== "ninguno");
  $("srv-lectura-btn").textContent = window.showDirectoryPicker ? T("srv.leer_sin_permiso") : T("srv.leer_carpeta");
  // «Elegir otra carpeta»: otro server a proposito. Una copia suelta sin carpeta elige con «Escanear server»
  $("srv-elegir").textContent = T("srv.elegir_otra");
  $("srv-elegir").hidden = !window.showDirectoryPicker || (m === "ninguno" && !SERVIDOR);
  $("srv-mapa").hidden = true; $("srv-confs").hidden = true; $("srv-arbol").hidden = true;
  $("srv-otro").hidden = true; $("srv-resumen").hidden = true; $("srv-guia").hidden = true;
  ["rutas", "traders", "traders2", "market", "loadouts", "config", "zonas"].forEach(function (k) {
    var el = $("cf-" + k);
    if (!el || document.activeElement === el) return;
    el.value = CF[k] || ""; el.placeholder = carpetaDef(k) ? carpetaDef(k).replace(/\//g, "\\") : T("srv.vacio");
  });
  // fuera de la raiz de un server: el aviso de arriba, y ningun boton que lea otra carpeta (no se leyo nada)
  renderRaiz();
  if (RAIZ) { $("srv-lectura-btn").hidden = true; $("srv-elegir").hidden = true; }
  if (RAIZ && !SRV.tipo) { aviso("srv-estado", T("raiz.estado")); return; }
  if (m === "ninguno") {
    var recordada = handlePendiente ? T("carp.recordada", handlePendiente.name) : "", falla = API_FALLA ? API_FALLA + " " : "";
    // el editor de un server dice de cual es; en Program Files (o sin File System Access) se lee sin escribir
    if (SERVIDOR) aviso("srv-estado", falla + T("srv.del_server", SERVIDOR.ruta) + (recordada ? " " + recordada :
                        (SERVIDOR.programFiles || !window.showDirectoryPicker ? T("srv.pf_lectura") : T("srv.primera_vez"))) + T("srv.editor_bat"));
    else aviso("srv-estado", falla + (recordada ? recordada + " " : "") + T("srv.elegi") + (window.showDirectoryPicker ? T("srv.elegi_chrome") : ""));
    return;
  }
  // la carpeta no es la del server de este editor: se avisa y se sigue (un admin puede querer editar otro server)
  if (SERVIDOR && !SRV.esDelEditor) { $("srv-otro").hidden = false; aviso("srv-otro", T("srv.otro_server", FS.nombre(), SERVIDOR.ruta)); }
  if (SRV.tipo === "otra") { aviso("srv-estado", T("srv.no_es_server", FS.nombre())); return; }
  partes.push((m === "api" ? T("srv.carpeta_api") : (m === "acceso" ? T("srv.carpeta") : (m === "lecturaHandle" ? T("srv.carpeta_sin_permiso") : T("srv.carpeta_lectura")))) +
              (SRV.esDelEditor && SERVIDOR ? SERVIDOR.ruta : FS.nombre()));
  if (SRV.arranque) {
    var hace = (Date.now() - SRV.rptMtime) / 1000;
    partes.push(T("srv.ultimo_arranque", hora(SRV.arranque.fecha)) + (hace < 120 ? T("srv.corriendo", Math.round(hace)) : T("srv.dejo_de", hora(new Date(SRV.rptMtime)))) +
                (SRV.mod === false ? T("srv.no_cargo") : (SRV.caliente === true ? T("srv.caliente") : (SRV.caliente === false ? T("srv.no_caliente") : ""))));
  }
  aviso("srv-estado", partes.join(" · "));
  var mp = $("srv-mapa");
  if (SRV.tipo === "perfil") {
    mp.hidden = false;
    mp.innerHTML = T("srv.es_perfil", esc(FS.nombre()));
  } else if (SRV.eligiendo) {
    var cs = $("srv-confs");
    cs.hidden = false;
    cs.innerHTML = '<p class="hint">' + (SRV.confs.length ? T("srv.n_confs", SRV.confs.length) : T("srv.sin_confs")) + '</p>' + batsVariablesHtml() +
      SRV.confs.map(function (c, i) {
        return '<button class="btn' + (c.cargaTrader ? " primary" : "") + '" type="button" data-conf="' + i + '"><b>' + esc(c.mapa.nombre) + '</b><span>' + esc(T("srv.conf_linea", c.perfil, c.mision, c.bats.join(", "))) + (c.cargaTrader ? T("srv.carga_trader") : "") +
               (typeof c.tomas === "number" ? esc(T("srv.conf_cuenta", c.tomas, c.servicios)) : "") + '</span></button>';
      }).join("") +
      // sin ninguna carpeta de perfil todavia (un server que nunca arranco) no hay perfil para elegir a mano
      (SRV.perfiles.length ? '<div class="otra"><span>' + T("srv.otra_combinacion") + '</span><select id="conf-perfil">' + SRV.perfiles.map(function (p) { return '<option>' + esc(p) + '</option>'; }).join("") + '</select>' +
      '<select id="conf-mision"><option value="">' + T("srv.sin_mision") + '</option>' + SRV.misiones.map(function (p) { return '<option>' + esc(p) + '</option>'; }).join("") + '</select><button class="btn" type="button" data-conf="mano">' + T("srv.usar") + '</button></div>' :
      '<p class="hint warn">' + T("srv.sin_perfiles") + '</p>');
  } else if (CF.conf) {
    var mapa = N.mapaDeMision(CF.conf.mision);
    mp.hidden = false;
    mp.innerHTML = '<span>' + T("srv.mapa") + ' <b>' + esc(mapa.nombre || "?") + '</b></span><span class="mono">' + esc(T("srv.perfil_mision", CF.conf.perfil, CF.conf.mision || T("srv.ninguna"))) + '</span>' +
      (CF.conf.bats && CF.conf.bats.length ? '<span class="hint">' + esc(T("srv.de_bats", CF.conf.bats.join(", "))) + '</span>' : '<span class="hint">' + T("srv.a_mano") + '</span>') +
      (SRV.confs.length > 1 || SRV.perfiles.length > 1 || SRV.batsVariables.length ? '<button class="btn" type="button" id="cambiar-mapa">' + T("srv.cambiar_mapa") + '</button>' : "") + batsVariablesHtml();
  }
  if (SRV.leido) { $("srv-resumen").hidden = false; aviso("srv-resumen", resumenEscaneo()); }
  var g = guiaServer();
  $("srv-guia").hidden = !g.length;
  $("srv-guia").innerHTML = g.map(function (x) { return "<li>" + esc(x) + "</li>"; }).join("");
  if (SRV.leido) {
    var ar = $("srv-arbol");
    ar.hidden = false;
    ar.innerHTML = SRV.arbol.map(function (x) {
      return '<li class="' + (x.existe === true ? "si" : (x.existe === false ? "no" : "info")) + '"><span><b>' + esc(TX(x.que)) + '</b> · ' + esc(x.existe === false ? T("arbol.no_esta") : TX(x.cuenta)) + '</span>' + (x.rel ? '<span class="mono" title="' + esc(ver(x.rel)) + '">' + esc(norm(x.rel).replace(/\//g, "\\")) + '</span>' : "") + '</li>';
    }).join("");
  }
}
// el aviso de arriba de todo: el editor no esta en la raiz de un server
function renderRaiz() {
  var a = $("raiz-aviso");
  a.hidden = !RAIZ;
  if (RAIZ) a.textContent = T("raiz.no_server", RAIZ.donde) + (RAIZ.mod && RAIZ.mod.charAt(0) !== "@" ? T("raiz.carpeta", RAIZ.mod) : "");
}
// Un server recien instalado: lo que falta antes de poder armar un servicio (el primer arranque con el mod, una toma)
function guiaServer() {
  var g = [];
  if (!SRV.leido || SRV.tipo === "otra") return g;
  if (SRV.sinCarpetaTrader) g.push(T("guia.arrancar", ver(carpeta("config"))));
  if (!SRV.tomas.length) g.push(T("guia.tomas", ver(carpeta("rutas"))));
  return g;
}
function renderListas() {
  var listo = SRV.leido;
  $("paso2").hidden = !listo;
  // con el server elegido y permiso de escritura no hace falta ningun cuadro de archivos
  $("toma-sel").innerHTML = SRV.tomas.length ? SRV.tomas.map(function (t) { return '<option>' + esc(t) + '</option>'; }).join("") : '<option value="">' + esc(T("lista.no_tomas", ver(carpeta("rutas")))) + '</option>';
  $("svc-sel").innerHTML = SRV.servicios.length ? SRV.servicios.map(function (t) { return '<option>' + esc(t) + '</option>'; }).join("") : '<option value="">' + esc(T("lista.no_servicios", ver(carpeta("config")))) + '</option>';
  if (!S) aviso("svc-estado", RAIZ && !SRV.tipo ? T("lista.primero_raiz") : (FS.modo() === "ninguno" ? T("lista.primero_carpeta") :
                              (SRV.tipo === "otra" ? T("lista.primero_no_es") :
                              (SRV.eligiendo ? T("lista.primero_mapa") :
                              (!SRV.tomas.length && !SRV.servicios.length ? T("lista.nada_todavia") : T("lista.arma_o_abri"))))));
}

// ============================================================================ la cinta
function renderCinta() {
  var svc = S.svc, prev = 0, largo = (S.toma ? S.toma.largo : (svc.Editor && svc.Editor.RouteLengthM)) || 0;
  var aBordo = (svc.Traders || []).length + (N.choferVende(svc) ? 0 : 1);
  var h = '<button class="nodo" type="button" data-n="A" aria-pressed="' + (nodoSel === "A") + '"><span class="o">A</span><span class="t"><b>' + T("cinta.inicio") + '</b>' + T("cinta.a_bordo", aBordo) + '</span></button>';
  (svc.Stops || []).forEach(function (st, i) {
    var apag = !N.marcasActivas(svc) || st.Mode === "off";
    var sub = apag ? T("cinta.no_para") : (st.Mode === "wave_only" ? T("cinta.si_saludan") : T("cinta.bajan", (st.WhoGetsOff || []).length));
    if (!apag && (st.Signals || []).length) sub += " · " + T(st.Signals.length > 1 ? "cinta.senales" : "cinta.senal", st.Signals.length);
    h += '<div class="tramo"><span>' + km((st.DistM || 0) - prev) + '</span></div>';
    h += '<button class="nodo marca' + (apag ? " apagada" : "") + '" type="button" data-n="M' + i + '" aria-pressed="' + (nodoSel === "M" + i) + '"><span class="o">' + st.Mark + '</span><span class="t"><b>' + T("cinta.marca", st.Mark) + '</b>' + sub + '</span></button>';
    prev = st.DistM || 0;
  });
  h += '<div class="tramo"><span>' + km(Math.max(0, largo - prev)) + '</span></div>';
  var fin = svc.OnRouteEnd || {};
  h += '<button class="nodo fin" type="button" data-n="FIN" aria-pressed="' + (nodoSel === "FIN") + '"><span class="o">' + T("cinta.fin") + '</span><span class="t"><b>' + T("cinta.fin_de_la_cinta") + '</b>' + ({ despawn: T("cinta.desaparece"), respawn: T("cinta.reaparece", fin.DelayS || 0), restart: T("cinta.empieza") }[fin.Action] || "?") + '</span></button>';
  $("fila").innerHTML = h;
  medirCinta();
}
// en dos columnas la cinta queda fija arriba (CSS). Pegada, esconde la ayuda; su alto (--cinta-alto) lo usa el plano para
// quedar debajo de ella
function medirCinta() {
  var c = $("cinta-caja"), fija = !c.hidden && getComputedStyle(c).position === "sticky", e = document.querySelector(".escena"), sobra = 0;
  c.classList.toggle("pegada", fija && window.scrollY > 0 && c.getBoundingClientRect().top <= 0.5);
  document.documentElement.style.setProperty("--cinta-alto", (fija ? c.offsetHeight : 0) + "px");
  // al final del inspector el plano fijo sube con su columna: la cinta sube con el, para no taparlo
  if (fija && c.classList.contains("pegada") && getComputedStyle(e).position === "sticky") sobra = c.offsetHeight + 10 - e.getBoundingClientRect().top;
  c.style.transform = sobra > 0 ? "translateY(" + (-Math.round(sobra)) + "px)" : "";
}
window.addEventListener("scroll", medirCinta, { passive: true });
window.addEventListener("resize", medirCinta);
// y cada vez que cambia su alto (la barra de scroll horizontal de una ruta larga, el ancho de la ventana)
if (window.ResizeObserver) new ResizeObserver(function () { medirCinta(); }).observe($("cinta-caja"));
// con la pagina bajada, el inspector del punto nuevo arranca a la vista: justo debajo de la cinta fija
function irAlInspector() {
  var c = $("cinta-caja"), tope = (getComputedStyle(c).position === "sticky" ? c.offsetHeight : 0) + 10, r = $("insp").getBoundingClientRect();
  if (r.top < tope) window.scrollTo(0, window.scrollY + r.top - tope);
}
$("fila").addEventListener("click", function (e) { var n = e.target.closest(".nodo"); if (!n) return; nodoSel = n.getAttribute("data-n"); todo(); irAlInspector(); });
function paradaSel() { return nodoSel.charAt(0) === "M" ? S.svc.Stops[+nodoSel.slice(1)] : null; }

// ============================================================================ escenas
// ★ «QUIEN VIAJA»: el vehiculo desde arriba (adelante a la derecha) con cada plaza donde esta (N.posicionesPlazas: el
// asiento medido por el mod, o su entrada, o la grilla) y, al costado, la lista de los que viajan. Cada plaza se
// dibuja del tamano que le dejan las vecinas (las 22 del bus no se pisan); con pocas plazas, el nombre va al lado. El
// zoom (+ / − / Ajustar, la rueda) y el arrastre del fondo mueven la vista, como en el plano. Un clic en una plaza con
// comerciante (en el dibujo o en la lista) lo elige.
var VISTA_A = {};   // la vista de cada vehiculo (centro y ancho, en m), mientras dura la sesion
var ASPECTO_A = 2.2;
function claveA(svc, P) { return (svc.VehicleClass || "") + "#" + P.pos.length + "#" + P.modo; }
function vistaAAjustada(P) {
  var c = P.caja, m = .6, nom = P.pos.length <= 8 ? 1.1 : 0, xa = c.z0 - m, xb = c.z1 + 1.2, ya = c.x0 - m - nom, yb = c.x1 + m + nom;
  var w = Math.max(xb - xa, (yb - ya) * ASPECTO_A, 6);
  return { cx: (xa + xb) / 2, cy: (ya + yb) / 2, w: w };
}
// Las posiciones para dibujar. El largo es el de verdad; el ancho se abre (hasta 2,5 veces) cuando dos plazas lado a
// lado quedan a menos de 0,62 m: el bus tiene la fila del chofer a 0,37 m de los asientos dobles, y a escala no se
// leerian. kr = cuanto se abrio.
function plazasDibujo() {
  var svc = S.svc, n = Math.max(1, plazasDelServicio()), P = N.posicionesPlazas(plazasVeh(svc.VehicleClass), n), d = Infinity, i, j;
  for (i = 0; i < P.pos.length; i++) for (j = i + 1; j < P.pos.length; j++) {
    if (Math.abs(P.pos[i].f - P.pos[j].f) < .45) d = Math.min(d, Math.abs(P.pos[i].r - P.pos[j].r));
  }
  var kr = d === Infinity || d <= 0 ? 1 : Math.max(1, Math.min(2.5, .62 / d));
  return { modo: P.modo, descartado: P.descartado, kr: kr, caja: { x0: P.caja.x0 * kr, x1: P.caja.x1 * kr, z0: P.caja.z0, z1: P.caja.z1 },
           pos: P.pos.map(function (p) { return { seat: p.seat, f: p.f, r: p.r * kr }; }) };
}
function asientosSVG() {
  var svc = S.svc, P = plazasDibujo(), c = P.caja, n = P.pos.length;
  var v = VISTA_A[claveA(svc, P)] || (VISTA_A[claveA(svc, P)] = vistaAAjustada(P)), alto = v.w / ASPECTO_A;
  var dmin = n > 1 ? N.distMinPlazas(P.pos) : 1.2, s = Math.max(.24, Math.min(.84, dmin * .8)), fs = s * .48, lab = .25 * Math.max(1, v.w / 11);
  // con muchas plazas (el bus) la lista va debajo: el dibujo usa todo el ancho
  var h = '<div class="asientos' + (n > 8 ? " ancho" : "") + '"><div class="asientos-dibujo"><div class="plano-barra" role="toolbar" aria-label="' + T("asientos.barra") + '">' +
    '<button class="btn" type="button" data-za="mas" aria-label="' + T("plano.acercar") + '" title="' + T("plano.acercar_tit") + '">+</button>' +
    '<button class="btn" type="button" data-za="menos" aria-label="' + T("plano.alejar") + '" title="' + T("plano.alejar") + '">−</button>' +
    '<button class="btn" type="button" data-za="ajustar" title="' + T("plano.ajustar_tit") + '">' + T("plano.ajustar") + '</button></div>';
  h += '<svg id="asientos" viewBox="' + (v.cx - v.w / 2).toFixed(2) + ' ' + (v.cy - alto / 2).toFixed(2) + ' ' + v.w.toFixed(2) + ' ' + alto.toFixed(2) + '" role="img" aria-label="' + T("asientos.aria") + '">';
  h += '<rect class="fondo" x="' + (v.cx - 1.5 * v.w) + '" y="' + (v.cy - 1.5 * alto) + '" width="' + 3 * v.w + '" height="' + 3 * alto + '" fill="var(--walk)"/>';
  // el casco (adelante a la derecha: x = adelante, y = a la derecha) y la flecha de la marcha
  h += '<rect pointer-events="none" x="' + c.z0 + '" y="' + c.x0 + '" width="' + (c.z1 - c.z0) + '" height="' + (c.x1 - c.x0) + '" rx=".45" fill="var(--surface)" stroke="var(--ink)" stroke-width=".06"/>';
  h += '<path pointer-events="none" d="M' + (c.z1 + .3) + ' ' + ((c.x0 + c.x1) / 2) + ' l.45 0 m-.18 -.18 l.18 .18 l-.18 .18" fill="none" stroke="var(--route)" stroke-width=".07" stroke-linecap="round"/>';
  P.pos.forEach(function (p, i) {
    var seat = p.seat, t = N.traderDe(svc, seat), esChofer = seat === 0, ocupada = esChofer || t, sel = t && t.Seat === compSel;
    var col = t ? (esChofer ? "var(--route)" : "var(--trader)") : (esChofer ? "var(--route)" : "var(--surface-2)");
    var nombre = t ? t.Name : (esChofer ? T("asientos.sin_chofer") : T("asientos.vacia"));
    h += '<g class="plaza" data-seat="' + seat + '" style="cursor:' + (t ? "pointer" : "default") + '"><title>' + esc(T("asientos.plaza", seat) + (esChofer ? T("asientos.maneja") : "") + " · " + nombre) + '</title>';
    h += '<rect x="' + (p.f - s / 2).toFixed(3) + '" y="' + (p.r - s * .42).toFixed(3) + '" width="' + s.toFixed(3) + '" height="' + (s * .84).toFixed(3) + '" rx="' + (s * .16).toFixed(3) + '" fill="' + col + '" stroke="' + (sel ? "var(--ink)" : (ocupada ? "none" : "var(--line)")) + '" stroke-width="' + (s * .07).toFixed(3) + '"/>';
    h += '<text x="' + p.f.toFixed(3) + '" y="' + (p.r + fs * .36).toFixed(3) + '" font-size="' + fs.toFixed(3) + '" text-anchor="middle" fill="' + (ocupada ? "var(--surface)" : "var(--muted)") + '" font-family="var(--body)" font-weight="700">' + seat + '</text>';
    // con pocas plazas el nombre va al lado, afuera del casco (las de la izquierda arriba, las de la derecha abajo)
    if (n <= 8) {
      var arriba = p.r < (c.x0 + c.x1) / 2, esc2 = (i % 4 < 2 ? 0 : 1) * lab * 2.3, ly = arriba ? c.x0 - .35 - lab * 1.2 - esc2 : c.x1 + .35 + lab + esc2;
      h += '<text pointer-events="none" x="' + p.f.toFixed(3) + '" y="' + ly.toFixed(3) + '" font-size="' + lab.toFixed(3) + '" text-anchor="middle" fill="var(--ink)" font-family="var(--body)">' + esc(nombre) + '</text>';
      h += '<text pointer-events="none" x="' + p.f.toFixed(3) + '" y="' + (ly + lab * 1.1).toFixed(3) + '" font-size="' + (lab * .76).toFixed(3) + '" text-anchor="middle" fill="var(--muted)" font-family="var(--mono)">' + T("asientos.plaza", seat) + (esChofer ? T("asientos.maneja") : "") + '</text>';
    }
    h += '</g>';
  });
  h += '</svg></div>';
  // al costado: los que viajan (un clic los elige) y cuantas plazas quedan vacias
  var vacias = 0, lista = "";
  P.pos.forEach(function (p) {
    var t = N.traderDe(svc, p.seat), esChofer = p.seat === 0;
    if (!t && !esChofer) { vacias++; return; }
    lista += '<li><button type="button" class="plaza' + (t && t.Seat === compSel ? " sel" : "") + '" data-seat="' + p.seat + '"' + (t ? "" : " disabled") + '><b class="' + (esChofer ? "chofer" : "comerciante") + '">' + p.seat + '</b><span>' + esc(t ? t.Name : T("asientos.sin_chofer")) + (esChofer ? '<small>' + T("asientos.maneja") + '</small>' : '') + '</span></button></li>';
  });
  h += '<ol class="lista-plazas" aria-label="' + T("asientos.lista") + '">' + lista + (vacias ? '<li class="hint">' + T("asientos.vacias", vacias) + '</li>' : '') + '</ol></div>';
  return h;
}
// el zoom de «Quien viaja»
function zoomAsientos(accion) {
  var svc = S.svc, P = plazasDibujo(), k = claveA(svc, P), v = VISTA_A[k] || vistaAAjustada(P);
  if (accion === "mas") v.w = Math.max(2, v.w / 1.4);
  else if (accion === "menos") v.w = Math.min(80, v.w * 1.4);
  else if (accion === "ajustar") v = vistaAAjustada(P);
  VISTA_A[k] = v;
  renderEscena();
}
// el arrastre del fondo y la rueda, como en el plano
function engancharAsientos() {
  var svg = $("asientos"); if (!svg) return;
  var P = plazasDibujo(), k = claveA(S.svc, P), drag = null;
  function pt(ev) { var p = svg.createSVGPoint(); p.x = ev.clientX; p.y = ev.clientY; return p.matrixTransform(svg.getScreenCTM().inverse()); }
  function caja(v) { var a = v.w / ASPECTO_A; return (v.cx - v.w / 2).toFixed(2) + " " + (v.cy - a / 2).toFixed(2) + " " + v.w.toFixed(2) + " " + a.toFixed(2); }
  svg.addEventListener("pointerdown", function (ev) {
    if (ev.target.closest(".plaza") || (ev.button !== 0 && ev.button !== 1)) return;
    var v = VISTA_A[k];
    drag = { sx: ev.clientX, sy: ev.clientY, cx: v.cx, cy: v.cy, k: 1 / svg.getScreenCTM().a };
    svg.setPointerCapture(ev.pointerId); svg.style.cursor = "grabbing"; ev.preventDefault();
  });
  svg.addEventListener("pointermove", function (e) {
    if (!drag) return;
    var v = VISTA_A[k];
    v.cx = drag.cx - (e.clientX - drag.sx) * drag.k; v.cy = drag.cy - (e.clientY - drag.sy) * drag.k;
    svg.setAttribute("viewBox", caja(v));
  });
  function soltar() { if (!drag) return; drag = null; renderEscena(); }
  svg.addEventListener("pointerup", soltar);
  svg.addEventListener("pointercancel", soltar);
  svg.addEventListener("wheel", function (e) {
    e.preventDefault();
    var p = pt(e), v = VISTA_A[k], w2 = Math.max(2, Math.min(80, v.w * (e.deltaY > 0 ? 1.2 : 1 / 1.2))), q = w2 / v.w;
    v.cx = p.x + (v.cx - p.x) * q; v.cy = p.y + (v.cy - p.y) * q; v.w = w2;
    renderEscena();
  }, { passive: false });
}

var VW0 = 36, VH0 = 20.25;   // la vista de referencia (KV = 1): 36 m de ancho, 16:9
var VW = VW0, VH = VH0, KV = 1, X0 = 0, Y0 = 0;
// ZOOM. La vista de cada parada (centro y ancho, en m) se recuerda mientras dura la
// sesion; la parada abre "ajustada". KV = cuanto mas grande que la de referencia: las fichas, los iconos y los textos
// se escalan con KV, asi mantienen su tamano en pantalla; el vehiculo, los circulos y las distancias son de verdad.
var VISTAS = {}, FORMAS = {}, GRANDE = false;
function claveParada(st) { return ((S && S.archivo) || "") + "#" + st.Mark; }
function altoDe(svc) { var v = svc && svc.PlayerHaltM; return typeof v === "number" && isFinite(v) && v >= 0 ? v : N.DEF.PLAYER_HALT_M; }
// "Ajustar": el vehiculo, los puestos (con su circulo de alto), las senales, el centro del semicirculo y la marca
function vistaAjustada(st) {
  var svc = S.svc, H = st.HeadingDeg || 0, plv = plazasVeh(svc.VehicleClass), cj = plv ? plv.caja : { x0: -.95, x1: .95, z0: -2.4, z1: 2.4 };
  var xa = Infinity, xb = -Infinity, za = Infinity, zb = -Infinity, alto = Math.max(1, altoDe(svc));
  function ver(x, z, m) { xa = Math.min(xa, x - m); xb = Math.max(xb, x + m); za = Math.min(za, z - m); zb = Math.max(zb, z + m); }
  ver(st.Pos[0], st.Pos[1], 0);
  [[cj.z1, cj.x0], [cj.z1, cj.x1], [cj.z0, cj.x1], [cj.z0, cj.x0]].forEach(function (c) { var w = N.aMundo(st.Pos, H, c[0], c[1]); ver(w[0], w[1], 0); });
  (st.Traders || []).forEach(function (p) {
    if ((st.WhoGetsOff || []).indexOf(p.Seat) < 0) return;
    (p.Waypoints || []).forEach(function (w) { if (w && w.Position) ver(w.Position[0], w.Position[2], alto); });
  });
  (st.Signals || []).forEach(function (sg) { if (sg && Array.isArray(sg.Pos) && isFinite(sg.Pos[0]) && isFinite(sg.Pos[1])) ver(sg.Pos[0], sg.Pos[1], 1.2); });
  var m = 1.5, w = Math.max(xb - xa + 2 * m, (zb - za + 2 * m) * VW0 / VH0, 12);
  return { cx: (xa + xb) / 2, cz: (za + zb) / 2, w: w };
}
function vistaDe(st) {
  var k = claveParada(st), v = VISTAS[k] || (VISTAS[k] = vistaAjustada(st));
  VW = v.w; VH = v.w * VH0 / VW0; KV = VW / VW0; X0 = v.cx - VW / 2; Y0 = -v.cz - VH / 2;
  return v;
}
function pasoGrilla() { return KV > 6 ? 50 : KV > 3 ? 20 : KV > 1.6 ? 10 : KV < .35 ? 1 : KV < .7 ? 2 : 5; }
// el centro, el radio y el arco del semicirculo de la parada (de la sesion, no van al archivo)
function formaDe(st) {
  var k = claveParada(st);
  if (!FORMAS[k]) { var c = N.aMundo(st.Pos, st.HeadingDeg || 0, 0, 8); FORMAS[k] = { cx: N.r1(c[0]), cz: N.r1(c[1]), r: 5, a: 180 }; }
  return FORMAS[k];
}
function bajanConPuesto(st) { return (st.WhoGetsOff || []).filter(function (s) { return !!N.puestoXZ(st, s); }); }
var COLOR_SENAL = { green: "#3A9D4A", purple: "#8A4FBF", red: "#D23C3C", white: "#F2F2EE", yellow: "#E3BE2A", blue: "#8A4FBF", black: "#2A2A2A" };
function TIPO_SENAL(v) { return T("sen.tipo." + v); }
function NOMBRE_COLOR(v) { return T("sen.color." + v); }
var senalSel = -1;
// El icono de una senal, centrado en 0,0 (en metros con KV = 1): humo = nube del color, bengala = rombo rojo,
// fuegos = estrella, airdrop = la caja con el humo rojo; la automatica es la nube con una luna.
function iconoSenal(sg, i, sel) {
  var tp = N.senalTipo(sg), col = COLOR_SENAL[N.senalColor(sg)] || COLOR_SENAL.green, borde = sel ? "var(--ink)" : "var(--surface)", h = "";
  if (tp === "smoke" || tp === "auto") {
    h += '<circle cx="-.38" cy=".12" r=".46" fill="' + col + '" stroke="' + borde + '" stroke-width=".1"/><circle cx=".4" cy=".1" r=".5" fill="' + col + '" stroke="' + borde + '" stroke-width=".1"/><circle cx="0" cy="-.3" r=".58" fill="' + col + '" stroke="' + borde + '" stroke-width=".1"/>';
    if (tp === "auto") h += '<path d="M.62 -.95 a.42 .42 0 1 0 .38 .58 a.32 .32 0 1 1 -.38 -.58z" fill="var(--ink)"/>';
  } else if (tp === "roadflare") {
    h += '<rect x="-.42" y="-.42" width=".84" height=".84" transform="rotate(45)" fill="#D23C3C" stroke="' + borde + '" stroke-width=".12"/><circle r=".18" fill="#FFD35A"/>';
  } else if (tp === "airdrop") {
    // el humo de airdrop: la caja (enterrada) con el humo rojo encima
    h += '<rect x="-.45" y="-.05" width=".9" height=".7" fill="#6B5A3A" stroke="' + borde + '" stroke-width=".1"/>' +
         '<circle cx="-.25" cy="-.35" r=".3" fill="#D23C3C"/><circle cx=".2" cy="-.55" r=".36" fill="#D23C3C"/><circle cx="0" cy="-.85" r=".22" fill="#D23C3C"/>';
  } else {
    var pts = []; for (var k = 0; k < 16; k++) { var a = Math.PI * k / 8, rr = k % 2 ? .32 : .8; pts.push((Math.sin(a) * rr).toFixed(2) + "," + (-Math.cos(a) * rr).toFixed(2)); }
    h += '<polygon points="' + pts.join(" ") + '" fill="#E3BE2A" stroke="#D23C3C" stroke-width=".1"/>';
  }
  return h + '<text y="1.35" font-size=".5" text-anchor="middle" fill="var(--ink)" font-family="var(--mono)" font-weight="700">S' + (i + 1) + '</text>';
}
// la flecha de hacia donde mira, en la ficha (KV = 1). rotate = el rumbo de DayZ (horario desde el norte),
// porque en pantalla el norte es arriba. La punta (.asa) se arrastra para girarla. Tenue = sin orientacion (la del
// elegido, para que se pueda empezar a girar).
function flechaRumbo(deg, tenue) {
  return '<g class="flecha" transform="rotate(' + deg.toFixed(1) + ')"' + (tenue ? ' opacity=".4"' : '') + '>' +
    '<line y1="-.95" y2="-2.05" stroke="var(--ink)" stroke-width=".14"' + (tenue ? ' stroke-dasharray=".18 .14"' : '') + '/>' +
    '<path d="M-.32 -1.85 L0 -2.4 L.32 -1.85 Z" fill="var(--ink)"/>' +
    '<circle class="asa" cy="-2.3" r=".5" fill="transparent" style="cursor:alias"><title>' + T("plano.girar") + '</title></circle></g>';
}
// de donde sale a pie: rombo = la salida que midio el mod; circulo hueco con "?" = sin medir todavia
function marcaSalida(w, medida) {
  var x = w[0], y = -w[1], s = .26 * KV;
  if (medida) return '<rect class="salida medida" pointer-events="none" x="' + (x - s) + '" y="' + (y - s) + '" width="' + 2 * s + '" height="' + 2 * s + '" transform="rotate(45 ' + x + ' ' + y + ')" fill="var(--trader)" stroke="var(--surface)" stroke-width="' + (.05 * KV) + '"/>';
  return '<g class="salida sin-medir" pointer-events="none"><circle cx="' + x + '" cy="' + y + '" r="' + .3 * KV + '" fill="var(--surface)" stroke="var(--trader)" stroke-width="' + (.06 * KV) + '" stroke-dasharray="' + (.1 * KV) + ' ' + (.07 * KV) + '"/>' +
    '<text x="' + x + '" y="' + (y + .13 * KV) + '" font-size="' + (.36 * KV) + '" text-anchor="middle" fill="var(--trader)" font-family="var(--mono)" font-weight="700">?</text></g>';
}
function paradaSVG(st) {
  vistaDe(st);
  var svc = S.svc, X = st.Pos[0], Z = st.Pos[1], H = st.HeadingDeg || 0, x0 = X0, y0 = Y0, K = KV;
  function P(w) { return w[0].toFixed(2) + "," + (-w[1]).toFixed(2); }
  function L(f, r) { return P(N.aMundo(st.Pos, H, f, r)); }
  var h = '<div class="plano-barra" role="toolbar" aria-label="' + T("plano.barra") + '">' +
    '<button class="btn" type="button" data-z="mas" aria-label="' + T("plano.acercar") + '" title="' + T("plano.acercar_tit") + '">+</button>' +
    '<button class="btn" type="button" data-z="menos" aria-label="' + T("plano.alejar") + '" title="' + T("plano.alejar") + '">−</button>' +
    '<button class="btn" type="button" data-z="ajustar" title="' + T("plano.ajustar_tit") + '">' + T("plano.ajustar") + '</button>' +
    '<button class="btn" type="button" data-z="grande" aria-pressed="' + GRANDE + '">' + (GRANDE ? T("plano.volver") : T("plano.agrandar")) + '</button>' +
    '<span class="hint mono" id="zoom-txt">' + T("plano.ancho", Math.round(VW)) + '</span></div>';
  h += '<svg id="plano" viewBox="' + x0.toFixed(2) + ' ' + y0.toFixed(2) + ' ' + VW.toFixed(2) + ' ' + VH.toFixed(2) + '" role="application" aria-label="' + esc(T("plano.aria", st.Mark)) + '">';
  // el fondo y la grilla cubren tres veces la vista: al arrastrar el fondo no se ve el borde
  h += '<rect class="fondo" x="' + (x0 - VW) + '" y="' + (y0 - VH) + '" width="' + 3 * VW + '" height="' + 3 * VH + '" fill="var(--walk)"/>';
  var paso = pasoGrilla();
  for (var gx = Math.ceil((x0 - VW) / paso) * paso; gx < x0 + 2 * VW; gx += paso) h += '<line pointer-events="none" x1="' + gx + '" y1="' + (y0 - VH) + '" x2="' + gx + '" y2="' + (y0 + 2 * VH) + '" stroke="var(--grid)" stroke-width="' + (.04 * K) + '"/>';
  for (var gy = Math.ceil((y0 - VH) / paso) * paso; gy < y0 + 2 * VH; gy += paso) h += '<line pointer-events="none" x1="' + (x0 - VW) + '" y1="' + gy + '" x2="' + (x0 + 2 * VW) + '" y2="' + gy + '" stroke="var(--grid)" stroke-width="' + (.04 * K) + '"/>';
  // el recorrido de la toma alrededor de la vista, o la direccion de llegada
  var cxv = x0 + VW / 2, czv = -(y0 + VH / 2);
  if (S.toma) {
    var pts = S.toma.pts.filter(function (p) { return Math.abs(p.x - cxv) < 1.6 * VW + 5 && Math.abs(p.z - czv) < 1.6 * VH + 5; }).map(function (p) { return p.x.toFixed(2) + "," + (-p.z).toFixed(2); });
    if (pts.length > 1) h += '<polyline pointer-events="none" points="' + pts.join(" ") + '" fill="none" stroke="var(--route)" stroke-width="' + (.3 * K) + '" stroke-dasharray="' + (1.2 * K) + ' ' + (.8 * K) + '" opacity=".6"/>';
  } else h += '<polyline pointer-events="none" points="' + [L(-30, 0), L(30, 0)].join(" ") + '" fill="none" stroke="var(--route)" stroke-width="' + (.3 * K) + '" stroke-dasharray="' + (1.2 * K) + ' ' + (.8 * K) + '" opacity=".6"/>';
  var rr = st.Mode === "wave_only" ? (st.WaveRadiusM || N.DEF.STOP_WAVE_RADIUS_M) : (N.usaSena(svc) ? (svc.Wave && svc.Wave.RadiusM) || 0 : 0);
  if (rr) h += '<circle pointer-events="none" cx="' + X + '" cy="' + (-Z) + '" r="' + rr + '" fill="none" stroke="var(--route)" stroke-width="' + (.08 * K) + '" stroke-dasharray="' + (.5 * K) + ' ' + (.4 * K) + '"/>';
  // el vehiculo, girado como llego a la marca (rumbo de DayZ). con el archivo de plazas del server, su
  // casco real y la puerta de cada plaza; sin el, el rectangulo de siempre.
  var plv = plazasVeh(svc.VehicleClass), cj = plv ? plv.caja : { x0: -.95, x1: .95, z0: -2.4, z1: 2.4 }, zm = (cj.z0 + cj.z1) / 2;
  h += '<polygon pointer-events="none" points="' + [L(cj.z1, cj.x0), L(cj.z1, cj.x1), L(cj.z0, cj.x1), L(cj.z0, cj.x0)].join(" ") + '" fill="var(--surface)" stroke="var(--ink)" stroke-width="' + (.08 * K) + '"/>';
  h += '<polyline pointer-events="none" points="' + [L(cj.z1 - 1.2, cj.x0 + .1), L(cj.z1 - .8, 0), L(cj.z1 - 1.2, cj.x1 - .1)].join(" ") + '" fill="none" stroke="var(--muted)" stroke-width="' + (.06 * K) + '"/>';
  h += '<polyline pointer-events="none" points="' + [L(cj.z1 + .6 * K, -.45 * K), L(cj.z1 + 1.3 * K, 0), L(cj.z1 + .6 * K, .45 * K)].join(" ") + '" fill="none" stroke="var(--route)" stroke-width="' + (.16 * K) + '" stroke-linecap="round" stroke-linejoin="round"/>';
  if (plv) plv.puertas.forEach(function (q) {
    var e = N.aMundo(st.Pos, H, q.f, q.r), s = .16 * K;
    h += '<rect class="puerta" data-seat="' + q.seat + '" pointer-events="none" x="' + (e[0] - s) + '" y="' + (-e[1] - s) + '" width="' + 2 * s + '" height="' + 2 * s + '" fill="var(--surface)" stroke="var(--ink)" stroke-width="' + (.05 * K) + '"/>';
    h += '<text pointer-events="none" x="' + e[0] + '" y="' + (-e[1] - .3 * K) + '" font-size="' + (.42 * K) + '" text-anchor="middle" fill="var(--muted)" font-family="var(--mono)">' + q.seat + '</text>';
  });
  var off = .6 + .8 * K, wi = N.aMundo(st.Pos, H, zm, cj.x0 - off), wd = N.aMundo(st.Pos, H, zm, cj.x1 + off);
  h += '<text pointer-events="none" x="' + wi[0].toFixed(2) + '" y="' + (-wi[1] + .2 * K).toFixed(2) + '" font-size="' + (.6 * K) + '" text-anchor="middle" fill="var(--muted)" font-family="var(--body)" font-weight="700">' + T("plano.izq") + '</text>';
  h += '<text pointer-events="none" x="' + wd[0].toFixed(2) + '" y="' + (-wd[1] + .2 * K).toFixed(2) + '" font-size="' + (.6 * K) + '" text-anchor="middle" fill="var(--ink)" font-family="var(--body)" font-weight="700">' + T("plano.der") + '</text>';
  var nx = x0 + VW - 1.6 * K, ny = y0 + 1.1 * K;
  h += '<g pointer-events="none" transform="translate(' + nx + ' ' + ny + ') scale(' + K + ')"><path d="M0 1.6 L0 0 M-.35 .45 L0 0 L.35 .45" fill="none" stroke="var(--ink)" stroke-width=".1" stroke-linecap="round"/>';
  h += '<text x="0" y="2.4" font-size=".6" text-anchor="middle" fill="var(--ink)" font-family="var(--display)" font-weight="700">N</text></g>';
  var sx = x0 + K, sy = y0 + VH - K;
  h += '<g class="escala" pointer-events="none"><line x1="' + sx + '" y1="' + sy + '" x2="' + (sx + paso) + '" y2="' + sy + '" stroke="var(--ink)" stroke-width="' + (.1 * K) + '"/><line x1="' + sx + '" y1="' + (sy - .3 * K) + '" x2="' + sx + '" y2="' + (sy + .3 * K) + '" stroke="var(--ink)" stroke-width="' + (.1 * K) + '"/><line x1="' + (sx + paso) + '" y1="' + (sy - .3 * K) + '" x2="' + (sx + paso) + '" y2="' + (sy + .3 * K) + '" stroke="var(--ink)" stroke-width="' + (.1 * K) + '"/>';
  h += '<text x="' + (sx + paso / 2) + '" y="' + (sy - .5 * K) + '" font-size="' + (.55 * K) + '" text-anchor="middle" fill="var(--ink)" font-family="var(--mono)">' + paso + ' m</text></g>';
  // las senales de la parada (debajo de los comerciantes)
  (st.Signals || []).forEach(function (sg, i) {
    if (!sg || !Array.isArray(sg.Pos) || !isFinite(sg.Pos[0]) || !isFinite(sg.Pos[1])) return;
    h += '<g class="senal" data-i="' + i + '" tabindex="0" role="button" aria-label="' + esc(T("plano.senal_aria", i + 1, TIPO_SENAL(N.senalTipo(sg)))) + '" transform="translate(' + sg.Pos[0] + ' ' + (-sg.Pos[1]) + ') scale(' + K + ')" style="cursor:grab">' + iconoSenal(sg, i, i === senalSel) + '</g>';
  });
  // el centro del semicirculo (con dos o mas que bajan)
  if (bajanConPuesto(st).length >= 2) {
    var fo = formaDe(st);
    h += '<g class="centro" tabindex="0" role="button" aria-label="' + esc(T("plano.centro_aria")) + '" transform="translate(' + fo.cx + ' ' + (-fo.cz) + ') scale(' + K + ')" style="cursor:move">' +
      '<circle r=".5" fill="var(--surface)" stroke="var(--route)" stroke-width=".12"/><path d="M-.85 0 H.85 M0 -.85 V.85" stroke="var(--route)" stroke-width=".12"/>' +
      '<text y="1.35" font-size=".45" text-anchor="middle" fill="var(--route)" font-family="var(--mono)">' + T("plano.centro") + '</text></g>';
  }
  // los que se quedan arriba (en su plaza: la del archivo de plazas, como en «Quien viaja»; sin el, en su lado del
  // vehiculo) y los que bajan: de su salida a su puesto
  var PP = plv ? N.posicionesPlazas(plv, Math.max(1, plazasDelServicio())) : null;
  function enPlaza(seat, fDef, rDef) { var pp = PP && PP.pos[seat]; return pp ? N.aMundo(st.Pos, H, pp.f, pp.r) : N.aMundo(st.Pos, H, fDef, rDef); }
  N.traders(svc).forEach(function (t) {
    var lado = t.Seat % 2 === 0 ? -.95 : .95, fila = Math.floor(t.Seat / 2), d = enPlaza(t.Seat, 1.2 - fila * 1.2, lado);
    var pq = N.puertaDe(plv, t.Seat), sa = pq ? N.salidaDe(plv, t.Seat) : null, rodeoPts = [], directo = false;
    if (sa) {
      // sale de su salida medida; sin medir, de la aproximacion de su puerta (o de su entrada, si la entrada
      // esta adentro del casco: no se sabe por donde sale). Desde adentro del casco (el bus) el mod va derecho.
      d = (!sa.medida && sa.entradaAdentro) ? N.aMundo(st.Pos, H, pq.f, pq.r) : N.aMundo(st.Pos, H, sa.f, sa.r);
      directo = sa.adentro || (!sa.medida && sa.entradaAdentro);
    }
    var baja = (st.WhoGetsOff || []).indexOf(t.Seat) >= 0, xz = N.puestoXZ(st, t.Seat);
    if (!baja || !xz) { h += '<circle pointer-events="none" cx="' + d[0] + '" cy="' + (-d[1]) + '" r="' + (.3 * K) + '" fill="' + (t.Seat === 0 ? "var(--route)" : "var(--trader)") + '" opacity=".55"/>'; return; }
    var lp = N.aLocal(st.Pos, H, xz[0], xz[1]);
    if (sa && !directo) {
      var ro = N.rodeoCasco(plv.caja, { r: sa.r, f: sa.f }, { r: lp.r, f: lp.f });
      rodeoPts = ro.esquinas.map(function (c) { return N.aMundo(st.Pos, H, c.f, c.r); });
    }
    var sel = t.Seat === compSel, p = puesto(st, t.Seat), rb = N.rumboDePuesto(p), ini = d, dash = (.35 * K) + ' ' + (.3 * K);
    if (rodeoPts.length) {
      h += '<polyline class="rodeo" pointer-events="none" data-seat="' + t.Seat + '" points="' + [d].concat(rodeoPts).map(function (w) { return w[0].toFixed(2) + "," + (-w[1]).toFixed(2); }).join(" ") + '" fill="none" stroke="var(--trader)" stroke-width="' + (.08 * K) + '" stroke-dasharray="' + dash + '"/>';
      d = rodeoPts[rodeoPts.length - 1];
    }
    h += '<line class="guia" pointer-events="none" data-seat="' + t.Seat + '" x1="' + d[0] + '" y1="' + (-d[1]) + '" x2="' + xz[0] + '" y2="' + (-xz[1]) + '" stroke="var(--trader)" stroke-width="' + (.08 * K) + '" stroke-dasharray="' + dash + '"/>';
    h += sa ? marcaSalida(ini, sa.medida) : '<circle pointer-events="none" cx="' + ini[0] + '" cy="' + (-ini[1]) + '" r="' + (.2 * K) + '" fill="var(--trader)"/>';
    if (sel && plv && pq) {
      // donde sube a la vuelta (= BZTRCasco.PuntoDeVuelta del mod)
      var vu = N.vueltaDe(plv, t.Seat, { r: lp.r, f: lp.f }), vw = N.aMundo(st.Pos, H, vu.f, vu.r), s2 = .24 * K;
      h += '<rect class="vuelta" pointer-events="none" data-seat="' + t.Seat + '" x="' + (vw[0] - s2) + '" y="' + (-vw[1] - s2) + '" width="' + 2 * s2 + '" height="' + 2 * s2 + '" fill="none" stroke="var(--route)" stroke-width="' + (.08 * K) + '"><title>' + esc(T("plano.vuelta_tit", vu.como)) + '</title></rect>';
    }
    var alto = altoDe(svc);
    if (sel && alto > 0) h += '<circle class="alto" pointer-events="none" data-seat="' + t.Seat + '" cx="' + xz[0] + '" cy="' + (-xz[1]) + '" r="' + alto + '" fill="var(--trader)" fill-opacity=".07" stroke="var(--trader)" stroke-width="' + (.05 * K) + '" stroke-dasharray="' + (.3 * K) + ' ' + (.3 * K) + '"/>';
    h += '<g class="ficha" data-seat="' + t.Seat + '" tabindex="0" role="button" aria-label="' + esc(T("plano.ficha_aria", t.Name)) + '" transform="translate(' + xz[0] + ' ' + (-xz[1]) + ') scale(' + K + ')" style="cursor:grab">';
    if (rb) h += flechaRumbo(rb.deg, false);
    else if (sel) h += flechaRumbo(N.rumboXZ(xz[0] - X, xz[1] - Z), true);
    h += '<circle r=".9" fill="' + (t.Seat === 0 ? "var(--route)" : "var(--trader)") + '" stroke="' + (sel ? "var(--ink)" : "var(--surface)") + '" stroke-width=".16"/>';
    if (t.Seat === 0) h += '<circle class="volante" r=".62" fill="none" stroke="var(--surface)" stroke-width=".1"/>';   // el que maneja
    h += '<text y=".3" font-size=".85" text-anchor="middle" fill="var(--surface)" font-weight="700" font-family="var(--body)">' + t.Seat + '</text>';
    h += '<text y="1.75" font-size=".55" text-anchor="middle" fill="var(--ink)" font-family="var(--mono)">' + (p ? p.TimeS : "") + ' s</text>';
    h += '</g>';
  });
  if (!N.choferVende(svc)) { var wb = enPlaza(0, 1.2, -.45); h += '<circle pointer-events="none" cx="' + wb[0] + '" cy="' + (-wb[1]) + '" r="' + (.3 * K) + '" fill="var(--route)"/>'; }
  return h + '</svg>';
}
function puesto(st, seat) { return N.puesto(st, seat); }

function resumen() {
  var svc = S.svc, L = [], w = svc.Wave || {};
  var t0 = N.choferVende(svc) ? N.traderDe(svc, 0) : null;
  L.push(["A", T("res.aparece", (svc.Traders || []).length + (N.choferVende(svc) ? 0 : 1)) + (t0 ? T("res.maneja", t0.Name) : T("res.sin_chofer")), "AutoDrive"]);
  // y las dos reglas (lo que se ve con 0 = sin limite: nada)
  var gapR = w.MinGapM === undefined ? N.DEF.WAVE_MIN_GAP_M : w.MinGapM, vueltaR = w.MaxPerLap === undefined ? N.DEF.WAVE_MAX_PER_LAP : w.MaxPerLap;
  if (N.usaSena(svc)) L.push(["↺", T("res.sena", w.RadiusM || 15, w.AheadOnly !== 0 ? T("res.adelante") : "", w.MemoryS || N.DEF.WAVE_MEMORY_S, w.TimeS || 60) +
    (gapR > 0 ? T("res.sena_gap", gapR) : "") + (vueltaR > 0 ? T("res.sena_vuelta", vueltaR) : ""), T("res.sena_quien")]);
  if (N.marcasActivas(svc)) (svc.Stops || []).forEach(function (st) {
    if (st.Mode === "off") return;
    var bj = (st.WhoGetsOff || []).map(function (s) { var t = N.traderDe(svc, s), p = puesto(st, s); return t ? t.Name + (p ? " (" + p.TimeS + " s)" : "") : ""; }).filter(Boolean).join(", ");
    var ns = (st.Signals || []).length;
    L.push([String(st.Mark), (st.Mode === "wave_only" ? T("res.si_saluda", st.WaveRadiusM || N.DEF.STOP_WAVE_RADIUS_M) : "") + T("res.freeze") + (ns ? T(ns > 1 ? "res.enciende_n" : "res.enciende_1", ns) : "") +
      (bj ? T("res.bajan", bj) : T("res.nadie_baja", svc.SeatedWaitS === undefined ? N.DEF.SEATED_WAIT_S : svc.SeatedWaitS)), T(ns ? "res.quien_senales" : "res.quien")]);
  });
  var fin = svc.OnRouteEnd || {};
  L.push([T("cinta.fin"), ({ despawn: T("res.fin_despawn"), respawn: T("res.fin_respawn", fin.DelayS || 0), restart: T("res.fin_restart") })[fin.Action] || "?", "AutoDrive"]);
  var h = '<ol style="list-style:none;margin:0;padding:0;display:flex;flex-direction:column;gap:10px">';
  L.forEach(function (x, i) {
    var col = i === L.length - 1 ? "var(--end)" : (x[0] === "A" ? "var(--route)" : "var(--trader)");
    h += '<li style="display:grid;grid-template-columns:42px 1fr;gap:10px;align-items:start"><span style="font-family:var(--display);font-weight:700;font-size:19px;color:' + col + ';text-align:center">' + esc(x[0]) + '</span><span>' + esc(x[1]) + '<br><small class="hint">' + esc(x[2]) + '</small></span></li>';
  });
  return h + '</ol>';
}

function renderEscena() {
  var st = paradaSel();
  if (!st && GRANDE) { GRANDE = false; document.querySelector(".escena").classList.remove("grande"); }
  if (nodoSel === "A") {
    $("esc-tit").textContent = T("esc.quien_viaja");
    var PA = plazasDibujo(), modoA = PA.modo, desc = PA.descartado;
    // de donde salen las posiciones: medidas, aproximadas por la entrada, o la grilla (sin archivo de plazas); si el
    // asiento medido se descarto (se pisan o caen fuera del casco), la linea dice por que y cuales
    $("esc-hint").textContent = T("esc.quien_viaja_hint") + (desc ? T("asientos.descartado_" + desc.motivo + "_" + modoA,
                                desc.plazas.slice(0, 4).join(", ") + (desc.plazas.length > 4 ? "…" : "")) :
                                T(modoA === "asiento" ? "asientos.modo_asiento" : (modoA === "entrada" ? "asientos.modo_entrada" :
                                (plazasVeh(S.svc.VehicleClass) ? "asientos.modo_grilla_juntas" : "asientos.modo_grilla"))));
    $("esc").innerHTML = asientosSVG();
    engancharAsientos();
    $("leyenda").innerHTML = '<span><i style="background:var(--route)"></i>' + T("esc.ley_chofer") + '</span><span><i style="background:var(--trader)"></i>' + T("esc.ley_comerciante") + '</span><span><i style="background:var(--surface-2);border:1px solid var(--line)"></i>' + T("esc.ley_vacia") + '</span>';
  } else if (st) {
    $("esc-tit").textContent = T("esc.parada_tit", st.Mark);
    $("esc-hint").textContent = N.marcasActivas(S.svc) ? T("esc.parada_hint") : T("esc.automatico");
    $("esc").innerHTML = st.Pos ? paradaSVG(st) : '<p class="nota">' + T("esc.sin_pos") + '</p>';
    $("esc-hint").textContent += plazasVeh(S.svc.VehicleClass) ? T("esc.plazas_server") : T("esc.plazas_sin", S.svc.VehicleClass);
    if ((st.Signals || []).length) $("esc-hint").textContent += T("esc.senales_hint");
    $("leyenda").innerHTML = '<span><i style="background:var(--trader)"></i>' + T("esc.ley_baja") + '</span><span><i style="background:var(--trader);opacity:.55"></i>' + T("esc.ley_arriba") + '</span><span><i style="background:var(--route)"></i>' + T("esc.ley_chofer") + '</span>' +
      '<span>' + T("esc.ley_mira") + '</span>' + (plazasVeh(S.svc.VehicleClass) ? '<span>' + T("esc.ley_salida") + '</span>' : '') +
      ((st.Signals || []).length ? '<span><i style="background:#3A9D4A;border-radius:50%"></i>' + T("esc.ley_senal") + '</span>' : '') +
      '<span class="mono">' + esc(T("esc.ley_marca", st.Pos ? Math.round(st.Pos[0]) : "?", st.Pos ? Math.round(st.Pos[1]) : "?", rumboTxt(st.HeadingDeg || 0))) + '</span>';
    if (st.Pos) engancharPlano(st);
  } else {
    $("esc-tit").textContent = T("esc.secuencia");
    $("esc-hint").textContent = T("esc.secuencia_hint");
    $("esc").innerHTML = resumen();
    $("leyenda").innerHTML = "";
  }
  // con un dibujo (el plano o los asientos), en dos columnas queda fijo arriba al bajar por el inspector (CSS)
  document.querySelector(".escena").classList.toggle("con-plano", !!$("esc").querySelector("svg"));
}

// el zoom (+ / − / Ajustar / Agrandar) y Esc para volver de la vista grande
function zoomParada(accion) {
  var st = paradaSel(); if (!st || !st.Pos) return;
  var k = claveParada(st), v = VISTAS[k] || vistaAjustada(st);
  if (accion === "mas") v.w = Math.max(4, v.w / 1.4);
  else if (accion === "menos") v.w = Math.min(600, v.w * 1.4);
  else if (accion === "ajustar") v = vistaAjustada(st);
  else if (accion === "grande") { GRANDE = !GRANDE; document.querySelector(".escena").classList.toggle("grande", GRANDE); }
  VISTAS[k] = v;
  renderEscena();
}
document.addEventListener("keydown", function (e) { if (e.key === "Escape" && GRANDE) zoomParada("grande"); });

function engancharPlano(st) {
  var svg = $("plano"); if (!svg) return;
  var drag = null, mg = .8 * KV, xa = X0 + mg, xb = X0 + VW - mg, za = -(Y0 + VH) + mg, zb = -Y0 - mg;
  function pt(ev) { var p = svg.createSVGPoint(); p.x = ev.clientX; p.y = ev.clientY; return p.matrixTransform(svg.getScreenCTM().inverse()); }
  function redondo(v, paso) { return Math.round(v / paso) * paso; }
  function pasoArrastre() { return KV < .6 ? .1 : .5; }   // con zoom alto, el arrastre afina a 0,1 m
  function mover(seat, x, z, paso) {
    x = Math.max(xa, Math.min(xb, x)); z = Math.max(za, Math.min(zb, z));
    N.moverPuesto(st, seat, redondo(x, paso), redondo(z, paso));
    var w = N.puestoXZ(st, seat);
    var g = svg.querySelector('.ficha[data-seat="' + seat + '"]'); if (g) g.setAttribute("transform", "translate(" + w[0] + " " + (-w[1]) + ") scale(" + KV + ")");
    var l = svg.querySelector('.guia[data-seat="' + seat + '"]'); if (l) { l.setAttribute("x2", w[0]); l.setAttribute("y2", -w[1]); }
    var a = svg.querySelector('.alto[data-seat="' + seat + '"]'); if (a) { a.setAttribute("cx", w[0]); a.setAttribute("cy", -w[1]); }
    actualizarCoord(st, seat);
    cambio(); renderJSON(); renderRevision(); renderChips();
  }
  // una senal se arrastra como un puesto
  function moverSenal(i, x, z, paso) {
    x = Math.max(xa, Math.min(xb, x)); z = Math.max(za, Math.min(zb, z));
    N.moverSenal(st, i, redondo(x, paso), redondo(z, paso));
    var sg = st.Signals[i], g = svg.querySelector('.senal[data-i="' + i + '"]');
    if (g) g.setAttribute("transform", "translate(" + sg.Pos[0] + " " + (-sg.Pos[1]) + ") scale(" + KV + ")");
    actualizarCoordSenal(st, i);
    cambio(); renderJSON(); renderRevision(); renderChips();
  }
  // el centro del semicirculo (de la sesion: no cambia el servicio)
  function moverCentro(x, z) {
    var fo = formaDe(st), g = svg.querySelector(".centro");
    fo.cx = N.r1(Math.max(xa, Math.min(xb, x))); fo.cz = N.r1(Math.max(za, Math.min(zb, z)));
    if (g) g.setAttribute("transform", "translate(" + fo.cx + " " + (-fo.cz) + ") scale(" + KV + ")");
    var cx = $("cen-x"), cz = $("cen-z");
    if (cx && document.activeElement !== cx) cx.value = fo.cx;
    if (cz && document.activeElement !== cz) cz.value = fo.cz;
  }
  // girar la flecha de un puesto hacia el puntero (de a 5°; con Mayús o Ctrl, de a 1°)
  function girar(seat, x, z, fino) {
    var w = N.puestoXZ(st, seat), p = puesto(st, seat); if (!w || !p) return;
    var deg = N.rumboXZ(x - w[0], z - w[1]);
    deg = fino ? Math.round(deg) : Math.round(deg / 5) * 5;
    N.ponerRumbo(p, deg);
    var f = svg.querySelector('.ficha[data-seat="' + seat + '"] .flecha');
    if (f) { f.setAttribute("transform", "rotate(" + p.HeadingDeg + ")"); f.removeAttribute("opacity"); }
    var c = $("rb-" + seat); if (c && document.activeElement !== c) c.value = p.HeadingDeg;
    cambio(); renderJSON(); renderRevision(); renderChips();
  }
  svg.addEventListener("pointerdown", function (ev) {
    var asa = ev.target.closest(".asa");
    if (asa) {
      var gf = asa.closest(".ficha"), sa = +gf.getAttribute("data-seat");
      if (compSel !== sa) { compSel = sa; renderInsp(); }
      drag = { asa: sa, g: gf }; svg.setPointerCapture(ev.pointerId); ev.preventDefault();
      return;
    }
    var gs = ev.target.closest(".senal");
    if (gs) {
      var i = +gs.getAttribute("data-i"), otra = senalSel !== i;
      senalSel = i; if (otra) renderInsp();
      drag = { senal: i, g: gs }; gs.setPointerCapture(ev.pointerId); gs.style.cursor = "grabbing"; ev.preventDefault();
      return;
    }
    var gc = ev.target.closest(".centro");
    if (gc) { drag = { centro: true, g: gc }; gc.setPointerCapture(ev.pointerId); ev.preventDefault(); return; }
    var g = ev.target.closest(".ficha");
    if (g) {
      var seat = +g.getAttribute("data-seat");
      if (compSel !== seat) { compSel = seat; renderInsp(); }
      drag = { seat: seat, g: g }; g.setPointerCapture(ev.pointerId); g.style.cursor = "grabbing"; ev.preventDefault();
      return;
    }
    if (ev.button !== 0 && ev.button !== 1) return;
    // el fondo (o el boton del medio) mueve la vista, no las fichas
    var v = VISTAS[claveParada(st)];
    drag = { pan: true, sx: ev.clientX, sy: ev.clientY, cx: v.cx, cz: v.cz, k: 1 / svg.getScreenCTM().a, g: svg };
    svg.setPointerCapture(ev.pointerId); svg.style.cursor = "grabbing"; ev.preventDefault();
  });
  svg.addEventListener("pointermove", function (e) {
    if (!drag) return;
    if (drag.pan) {
      var v = VISTAS[claveParada(st)];
      v.cx = drag.cx - (e.clientX - drag.sx) * drag.k;
      v.cz = drag.cz + (e.clientY - drag.sy) * drag.k;
      svg.setAttribute("viewBox", (v.cx - VW / 2).toFixed(2) + " " + (-v.cz - VH / 2).toFixed(2) + " " + VW.toFixed(2) + " " + VH.toFixed(2));
      return;
    }
    var p = pt(e);
    if (drag.asa !== undefined) girar(drag.asa, p.x, -p.y, e.shiftKey || e.ctrlKey);
    else if (drag.centro) moverCentro(p.x, -p.y);
    else if (drag.senal !== undefined) moverSenal(drag.senal, p.x, -p.y, pasoArrastre());
    else mover(drag.seat, p.x, -p.y, pasoArrastre());
  });
  // al soltar se redibuja (la grilla de la vista nueva, la linea y el rodeo del puesto movido)
  function soltar() { if (!drag) return; drag = null; renderEscena(); }
  svg.addEventListener("pointerup", soltar);
  svg.addEventListener("pointercancel", soltar);
  // la rueda acerca y aleja, centrada en el puntero
  svg.addEventListener("wheel", function (e) {
    e.preventDefault();
    var p = pt(e), v = VISTAS[claveParada(st)], w2 = Math.max(4, Math.min(600, v.w * (e.deltaY > 0 ? 1.2 : 1 / 1.2))), q = w2 / v.w;
    v.cx = p.x + (v.cx - p.x) * q; v.cz = -p.y + (v.cz + p.y) * q; v.w = w2;
    renderEscena();
  }, { passive: false });
  svg.addEventListener("keydown", function (ev) {
    var d = ev.ctrlKey ? .1 : (ev.shiftKey ? 2 : .5), paso = ev.ctrlKey ? .1 : .5;
    var k = { ArrowLeft: [-d, 0], ArrowRight: [d, 0], ArrowUp: [0, d], ArrowDown: [0, -d] }[ev.key]; if (!k) return;
    var gs = ev.target.closest(".senal");
    if (gs) {
      var i = +gs.getAttribute("data-i"), sg = st.Signals[i]; if (!sg) return;
      ev.preventDefault(); moverSenal(i, sg.Pos[0] + k[0], sg.Pos[1] + k[1], paso);
      return;
    }
    if (ev.target.closest(".centro")) { var fo = formaDe(st); ev.preventDefault(); moverCentro(fo.cx + k[0], fo.cz + k[1]); return; }
    var g = ev.target.closest(".ficha"); if (!g) return;
    var seat = +g.getAttribute("data-seat"), w = N.puestoXZ(st, seat); if (!w) return;
    ev.preventDefault(); mover(seat, w[0] + k[0], w[1] + k[1], paso);
  });
}
function relTxt(st, seat) { return relXZ(st, N.puestoXZ(st, seat)); }
function relXZ(st, w) {
  if (!w || !st.Pos) return "";   // una parada sin posicion (el ejemplo del primer arranque, sin toma): nada que medir
  var lc = N.aLocal(st.Pos, st.HeadingDeg || 0, w[0], w[1]);
  return T("rel.linea", lc.r >= 0 ? T("rel.derecha", num(lc.r)) : T("rel.izquierda", num(-lc.r)), lc.f >= 0 ? T("rel.adelante", num(lc.f)) : T("rel.atras", num(-lc.f)), num(Math.hypot(lc.f, lc.r)));
}
function actualizarCoord(st, seat) {
  var w = N.puestoXZ(st, seat);
  var x = $("x-" + seat), z = $("z-" + seat), r = $("rel-" + seat);
  if (x && document.activeElement !== x) x.value = w[0];
  if (z && document.activeElement !== z) z.value = w[1];
  if (r) r.textContent = relTxt(st, seat);
}
function actualizarCoordSenal(st, i) {
  var sg = st.Signals[i]; if (!sg) return;
  var x = $("sgx-" + i), z = $("sgz-" + i), r = $("sgrel-" + i);
  if (x && document.activeElement !== x) x.value = sg.Pos[0];
  if (z && document.activeElement !== z) z.value = sg.Pos[1];
  if (r) r.textContent = relXZ(st, sg.Pos);
}

// ============================================================================ el inspector
function seg(nombre, val, opciones) {
  return '<div class="seg" role="radiogroup">' + opciones.map(function (o) {
    return '<label><input type="radio" name="' + nombre + '" value="' + o[0] + '" data-r="' + nombre + '"' + (val === o[0] ? " checked" : "") + '>' + o[1] + '</label>';
  }).join("") + '</div>';
}
function getv(o, path) { var p = path.split("."); for (var i = 0; i < p.length; i++) { if (o === null || o === undefined || typeof o !== "object" || !(p[i] in o)) return undefined; o = o[p[i]]; } return o; }
function setv(o, path, v) { var p = path.split("."); for (var i = 0; i < p.length - 1; i++) { if (!o[p[i]] || typeof o[p[i]] !== "object") o[p[i]] = {}; o = o[p[i]]; } if (v === undefined) delete o[p[p.length - 1]]; else o[p[p.length - 1]] = v; }
// un numero del servicio: vacio = la clave se borra (vuelve al default del mod, que se ve en gris)
function nro(path, def, extra, desc) {
  var v = getv(S.svc, path);
  return '<input type="number" ' + (extra || "") + ' data-k="' + path + '" data-t="num" value="' + (v === undefined ? "" : v) + '" placeholder="' + (def === undefined ? "" : esc(T("comun.por_defecto", def))) + '"' + (desc ? ' aria-label="' + esc(desc) + '"' : "") + '>';
}
function sw(path, texto, def) {
  var v = getv(S.svc, path), on = v === undefined ? !!def : v === 1 || v === true;
  return '<label class="check"><input type="checkbox" data-k="' + path + '" data-t="b01"' + (on ? " checked" : "") + '>' + texto + '</label>';
}
function bola(seat) { return '<span class="bola' + (seat === 0 ? " chofer" : "") + '">' + seat + '</span>'; }
// los comerciantes del servicio; sin el de la plaza 0 elegido, primero «elegí quién maneja»
function opcionesChofer(svc) {
  var ts = N.traders(svc), t0 = N.choferVende(svc) ? N.traderDe(svc, 0) : null, h = "";
  if (!ts.length) return '<option value="" selected>' + esc(T("a.sin_comerciantes")) + '</option>';
  if (!t0) h += '<option value="" selected disabled>' + esc(T("a.elegi_quien")) + '</option>';
  ts.forEach(function (x) { h += '<option value="' + x.Seat + '"' + (x === t0 ? " selected" : "") + '>' + esc(x.Name) + ' · ' + esc(T("a.fila_t", x.Seat)) + '</option>'; });
  return h;
}
function opciones(lista, val, conVacio) {
  var l = lista.slice(); if (val && l.indexOf(val) < 0) l.unshift(val);
  return (conVacio ? '<option value=""' + (!val ? " selected" : "") + '>(ninguno)</option>' : "") + l.map(function (v) { return '<option' + (v === val ? " selected" : "") + '>' + esc(v) + '</option>'; }).join("");
}

function inspA() {
  var svc = S.svc, w = svc.Wave || {}, z = svc.TraderZone || {};
  var h = '<div class="cab"><span class="eyebrow">' + T("a.punto") + '</span><h2>' + T("a.config") + '</h2></div>';
  h += '<div class="bloque"><fieldset><legend>' + T("a.servicio") + '</legend>' +
       '<div class="campo"><span>' + T("a.nombre") + '</span><input type="text" data-k="Name" data-t="str" value="' + esc(svc.Name) + '"></div>' +
       '<label class="check"><input type="checkbox" data-k="Enable" data-t="b01"' + (svc.Enable === 1 ? " checked" : "") + '>' + T("a.habilitado") + '</label>' +
       '<p class="hint">' + esc(T("a.habilitado_hint")) + '</p>' +
       '<div class="campo"><span>' + T("a.ruta") + '</span><span class="mono" style="font-size:13px">' + esc(svc.Route) + '</span></div></fieldset></div>';
  h += '<div class="bloque"><fieldset><legend>' + T("a.como_para") + '</legend>' + seg("modo", svc.StopMode, [["marks", T("a.modo.marks")], ["wave_only", T("a.modo.wave_only")], ["marks_and_wave", T("a.modo.marks_and_wave")]]) +
       '<p class="hint">' + (["marks", "wave_only", "marks_and_wave"].indexOf(svc.StopMode) >= 0 ? T("a.modo_hint." + svc.StopMode) : "") + '</p></fieldset></div>';
  if (N.usaSena(svc)) {
    h += '<div class="bloque"><fieldset><legend>' + T("a.saludo") + '</legend>' +
      '<p class="hint">' + esc(T("a.saludo_hint")) + '</p>' +
      '<div class="campo"><span>' + T("a.radio_deteccion") + '</span>' + nro("Wave.RadiusM", N.DEF.WAVE_RADIUS_M, 'min="1" max="80" step="1"') + '</div>' +
      sw("Wave.AheadOnly", T("a.solo_adelante"), 1) +
      '<div class="campo"><span>' + T("a.saludo_vale") + '</span>' + nro("Wave.MemoryS", N.DEF.WAVE_MEMORY_S, 'min="1" step="1"') + '</div>' +
      '<div class="campo"><span>' + T("a.espera_senas") + '</span>' + nro("Wave.CooldownS", N.DEF.WAVE_COOLDOWN_S, 'min="0" step="1"') + '</div>' +
      // las dos reglas contra el saludo abusivo (0 = sin limite)
      '<div class="campo"><span>' + T("a.min_gap") + '</span>' + nro("Wave.MinGapM", N.DEF.WAVE_MIN_GAP_M, 'min="0" step="50"') + '</div>' +
      '<div class="campo"><span>' + T("a.max_lap") + '</span>' + nro("Wave.MaxPerLap", N.DEF.WAVE_MAX_PER_LAP, 'min="0" step="1"') + '</div>' +
      '<div class="campo"><span>' + T("a.tiempo_vendiendo") + '</span>' + nro("Wave.TimeS", N.DEF.WAVE_TIME_S, 'min="0" step="5"') + '</div>' +
      sw("Wave.HornOnDepart", T("a.bocina_saludo"), N.DEF.HORN_ON_DEPART) +
      '<p class="hint">' + esc(T("a.bocina_saludo_hint")) + '</p></fieldset></div>';
  }
  var max = plazasDelServicio(), pl = N.plazasDe(svc.VehicleClass, S.toma ? S.toma.seatsHdr : 0, PLAZAS_RECORDADAS, nPlazasVeh(svc.VehicleClass));
  h += '<div class="bloque"><fieldset><legend>' + T("a.ocupantes") + '</legend>' +
    '<div class="campo"><span>' + T("a.plazas_vehiculo") + '</span><input type="number" min="1" max="30" id="plazas" value="' + (max || "") + '" placeholder="' + T("a.cuantas") + '"></div>' +
    '<p class="hint">' + (pl.n ? esc(T("a.sugerido", pl.n, pl.fuente)) : esc(T("a.no_en_tabla", svc.VehicleClass))) +
      (plazasVeh(svc.VehicleClass) ? "" : esc(T("a.sin_plazas_server", svc.VehicleClass))) + '</p>' +
    // maneja uno de los comerciantes
    '<div class="campo"><span>' + T("a.quien_maneja") + '</span><select id="chofer"' + ((svc.Traders || []).length ? "" : " disabled") + '>' + opcionesChofer(svc) + '</select></div>' +
    '<p class="hint">' + esc(T("a.quien_maneja_hint")) + '</p>' +
    // AllCanDrive no hace nada en el mod (la sucesion queda para otra etapa): se muestra deshabilitado
    '<label class="check" style="opacity:.6"><input type="checkbox" disabled' + (getv(svc, "Driver.AllCanDrive") !== 0 ? " checked" : "") + '>' + T("a.todos_manejan") + ' <span class="hint">· ' + T("comun.no_disponible") + '</span></label>' +
    '<div class="campo"><span>' + T("a.comerciantes") + '</span><span class="stepper"><button type="button" data-a="menos" aria-label="' + T("a.uno_menos") + '"' + ((svc.Traders || []).length <= 0 ? " disabled" : "") + '>−</button><output>' + (svc.Traders || []).length + ' / ' + N.maxComerciantes(svc) + '</output><button type="button" data-a="mas" aria-label="' + T("a.uno_mas") + '"' + (N.plazaLibre(svc) < 0 ? " disabled" : "") + '>+</button></span></div>' +
    '</fieldset></div>';
  var zonaExiste = z.Name ? !!SRV.zonas[String(z.Name).toLowerCase()] : false, nec = N.radioNecesario(svc);
  h += '<div class="bloque"><fieldset><legend>' + T("a.zona") + '</legend>' +
    '<div class="campo"><span>' + T("a.zona_servicio") + '</span><input type="text" data-k="TraderZone.Name" data-t="str" value="' + esc(z.Name || "") + '" placeholder="' + T("a.zona_vacia") + '"></div>' +
    (z.Name ? sw("TraderZone.FollowsVehicle", T("a.zona_sigue"), 1) +
      '<p class="hint">' + T("a.zona_radio", num(Math.max(z.RadiusM || 0, nec)), num(N.puestoMasLejos(svc))) +
      (SRV.leido ? (zonaExiste ? T("a.zona_existe") : T("a.zona_no_existe")) : "") + '</p>'
      : '<p class="hint">' + T("a.sin_zona") + '</p>') +
    '<div class="campo"><span>' + T("a.max_stall") + '</span>' + nro("MaxStallDistanceM", N.DEF.MAX_STALL_DISTANCE_M, 'min="5" step="5"') + '</div>' +
    '<p class="hint">' + T("a.max_stall_hint") + '</p>' +
    '</fieldset></div>';
  // los jugadores cerca, con el atajo de la secuencia fija a la vista
  var fija = N.esSecuenciaFija(svc);
  h += '<div class="bloque"><fieldset><legend>' + T("a.jugadores") + '</legend>' +
    '<button type="button" class="btn' + (fija ? " primary" : "") + '" data-a="secuencia" aria-pressed="' + fija + '">' + T("a.secuencia_fija") + '</button>' +
    '<p class="hint">' + (fija ? T("a.secuencia_si") : T("a.secuencia_no")) + '</p>' +
    '<div class="campo"><span>' + T("a.con_jugadores") + '</span>' + nro("MaxWithPlayersS", N.DEF.MAX_WITH_PLAYERS_S, 'min="0" step="10"') + '</div>' +
    '<p class="hint">' + T("a.con_jugadores_hint") + '</p>' +
    '<div class="campo"><span>' + esc(T("a.radio_jugadores")) + '</span>' + nro("PlayerRadiusM", N.DEF.PLAYER_RADIUS_M, 'min="1" step="1"') + '</div>' +
    '<div class="campo"><span>' + T("a.alto") + '</span>' + nro("PlayerHaltM", N.DEF.PLAYER_HALT_M, 'min="0" step="1"') + '</div>' +
    '<p class="hint">' + T("a.alto_hint") + '</p>' +
    // al seguir, si hay un jugador en el camino de adelante
    '<div class="campo"><span>' + T("a.frente_espera") + '</span>' + nro("FrontPlayerWaitS", N.DEF.FRONT_PLAYER_WAIT_S, 'min="0" step="1"') + '</div>' +
    '<p class="hint">' + esc(T("a.frente_espera_hint")) + '</p>' +
    '</fieldset></div>';
  h += '<details class="bloque"><summary>' + T("a.tiempos") + ' <span>' + T("a.opcionales") + '</span></summary><fieldset style="margin-top:10px">' +
    '<div class="campo"><span>' + T("a.primer_lanzamiento") + '</span>' + nro("StartDelayS", N.DEF.START_DELAY_S, 'min="0" step="5"') + '</div>' +
    '<div class="campo"><span>' + T("a.espera_sentado") + '</span>' + nro("SeatedWaitS", N.DEF.SEATED_WAIT_S, 'min="0" step="5"') + '</div>' +
    '<div class="campo"><span>' + T("a.reaparece") + '</span>' + nro("RespawnSeconds", N.DEF.RESPAWN_S, 'step="10"') + '</div>' +
    '</fieldset></details>';
  h += bloqueMarcador(svc);
  h += '<div class="bloque"><fieldset><legend>' + T("a.los_comerciantes") + '</legend><div class="lista">';
  N.traders(svc).forEach(function (t) {
    h += '<button type="button" class="fila-t" data-sel="' + t.Seat + '" aria-pressed="' + (t.Seat === compSel) + '">' + bola(t.Seat) + '<span class="d"><b>' + esc(t.Name) + '</b><small>' + T("a.fila_t", t.Seat) + (t.Seat === 0 ? T("a.maneja") : "") + ' · ' + esc(t.TraderFile || T("a.sin_archivo")) + '</small></span></button>';
  });
  if (!(svc.Traders || []).length) h += '<p class="hint">' + esc(T("a.ningun_comerciante")) + '</p>';
  h += '</div></fieldset>';
  var t = N.traderDe(svc, compSel);
  if (t) h += tarjetaComerciante(t);
  h += '</div>';
  return h;
}

// el marcador de mapa (Navigation de Expansion), prendido por servicio: su texto, su color y cada cuanto se mueve.
// Con el interruptor apagado, todo deshabilitado (el archivo conserva lo que tenia).
function rgbMarcador(svc) { var m = svc.Markers || {}; return [m.Red, m.Green, m.Blue].map(function (c) { return c === undefined ? "255" : String(c); }).join(" "); }
function bloqueMarcador(svc) {
  var mk = svc.Markers || {}, on = mk.useMapMarker === 1, dis = on ? "" : " disabled", hex = N.colorMarcador(svc);
  return '<div class="bloque"><fieldset><legend>' + T("mk.titulo") + '</legend>' +
    '<label class="check"><input type="checkbox" data-k="Markers.useMapMarker" data-t="b01"' + (on ? " checked" : "") + '>' + T("mk.prender") + '</label>' +
    '<div class="campo"><span>' + T("a.texto_marcador") + '</span><input type="text" data-k="Markers.MarkerLabel" data-t="str" value="' + esc(mk.MarkerLabel || "") + '" placeholder="' + esc(T("comun.por_defecto", svc.Name)) + '"' + dis + '></div>' +
    '<div class="campo"><span>' + T("mk.color") + '</span><span class="muestras" role="group" aria-label="' + esc(T("mk.color")) + '">' +
    N.MARCA_COLORES.map(function (c) { return '<button type="button" class="muestra" data-color="' + c.hex + '" style="background:' + c.hex + '" aria-pressed="' + (c.hex === hex) + '" aria-label="' + esc(T("mk.color." + c.k)) + '" title="' + esc(T("mk.color." + c.k)) + '"' + dis + '></button>'; }).join("") +
    '<input type="color" id="mk-color" value="' + hex.toLowerCase() + '" aria-label="' + esc(T("mk.libre")) + '" title="' + esc(T("mk.libre")) + '"' + dis + '>' +
    '<span class="mono hint" id="mk-rgb">' + esc(rgbMarcador(svc)) + '</span></span></div>' +
    '<div class="campo"><span>' + T("mk.cada") + '</span>' + nro("Markers.MarkerUpdateSeconds", N.DEF.MARCA_CADA_S, 'min="' + N.DEF.MARCA_MIN_S + '" step="1"' + dis) + '</div>' +
    // parpadea en las paradas (senal de venta); fijo en marcha, en la cabecera y en la terminal
    '<label class="check"><input type="checkbox" data-k="Markers.BlinkAtStops" data-t="b01"' + (mk.BlinkAtStops !== 0 ? " checked" : "") + dis + '>' + T("mk.parpadeo") + '</label>' +
    '<div class="campo"><span>' + T("mk.parpadeo_cada") + '</span>' + nro("Markers.BlinkSeconds", N.DEF.MARCA_PARPADEO_S, 'min="0" step="1"' + dis) + '</div>' +
    '<p class="hint">' + esc(T("mk.parpadeo_hint")) + '</p>' +
    '<p class="hint">' + esc(T("mk.hint", N.DEF.MARCA_QUIETO_M)) + '</p></fieldset></div>';
}

function tarjetaComerciante(t) {
  var loadouts = SRV.loadouts.length ? SRV.loadouts : ["SurvivorLoadout"];
  var archivos = Object.keys(SRV.comerciantes).map(function (k) { return SRV.comerciantes[k]; }).sort(function (a, b) { return a.archivo.localeCompare(b.archivo); });
  var datos = SRV.comerciantes[String(t.TraderFile || "").toLowerCase()], ropa = Array.isArray(t.Clothing) ? t.Clothing : [];
  var aMedida = !t.Loadout && ropa.length > 0;
  var h = '<div class="tarjeta sel"><div class="tt">' + bola(t.Seat) + esc(T("tr.ajustes", t.Name, t.Seat)) + '</div>' +
    '<div class="campo"><span>' + T("tr.nombre") + '</span><input type="text" data-tk="Name" value="' + esc(t.Name) + '"></div>' +
    '<div class="campo"><span>' + T("tr.clase") + '</span><select data-tk="TraderAIClass">' + opciones(N.CLASES_TRADER, t.TraderAIClass) + '</select></div>' +
    '<div class="campo"><span>' + T("tr.vestimenta") + '</span>' + seg("ropamodo", aMedida ? "custom" : "loadout", [["loadout", T("tr.loadout")], ["custom", T("tr.a_medida")]]) + '</div>';
  if (!aMedida) h += '<div class="campo"><span>' + T("tr.loadout") + '</span><select data-tk="Loadout">' + opciones(loadouts, t.Loadout, true) + '</select></div>';
  else {
    h += '<div class="lista">' + ropa.map(function (p, k) { return '<div class="campo"><span>' + T("tr.prenda", k + 1) + '</span><span style="display:flex;gap:6px"><input type="text" data-prenda="' + k + '" value="' + esc(p) + '" placeholder="' + T("tr.prenda_ph") + '"><button class="btn" type="button" data-a="quitar-prenda" data-i="' + k + '" aria-label="' + T("tr.quitar_prenda", k + 1) + '">×</button></span></div>'; }).join("") +
         '<button class="btn" type="button" data-a="agregar-prenda" style="justify-self:start">' + T("tr.agregar_prenda") + '</button><p class="hint">' + T("tr.prenda_hint") + '</p></div>';
  }
  h += '<div class="campo"><span>' + T("tr.que_vende") + '</span><select data-tk="TraderFile">' +
       (archivos.length ? archivos.map(function (a) { return '<option value="' + esc(a.archivo) + '"' + (a.archivo.toLowerCase() === String(t.TraderFile || "").toLowerCase() ? " selected" : "") + '>' + esc(a.archivo) + ' · ' + esc(N.nombreVisible(a)) + '</option>'; }).join("") : "") +
       (!datos ? '<option selected value="' + esc(t.TraderFile || "") + '">' + esc(t.TraderFile || T("tr.ninguno")) + (SRV.leido && t.TraderFile ? T("tr.no_en_market") : "") + '</option>' : "") +
       SRV.tradersMalos.map(function (x) { return '<option class="malo" disabled>' + esc(x.archivo) + esc(T("tr.no_se_puede", TX(x.porque))) + '</option>'; }).join("") + '</select></div>' +
       (datos ? '<p class="hint">' + T("tr.archivo") + '<span class="mono">' + esc(ver(unir(datos.carpeta, datos.archivo + ".json"))) + '</span></p>' : "");
  if (datos) {
    var cats = datos.categorias.map(function (c) { return c.nombre + (c.modo === 0 ? T("tr.solo_compra") : c.modo === 2 ? T("tr.solo_venta") : c.modo === 3 ? T("tr.accesorio") : ""); });
    h += '<p class="hint">' + (cats.length ? T("tr.n_categorias", cats.length) + esc(cats.slice(0, 6).join(", ")) + (cats.length > 6 ? "…" : "") : T("tr.sin_categorias")) +
         (datos.items.length ? T("tr.n_sueltos", datos.items.length) + esc(datos.items.slice(0, 6).join(", ")) + (datos.items.length > 6 ? "…" : "") : "") +
         esc(T("tr.monedas", datos.monedas.join(", "))) + '</p>';
  }
  // la faccion es fija (el mod ignora Traders[].Faction): se muestra, no se edita
  h += '<div class="campo"><span>' + T("tr.faccion") + '</span><span class="mono" style="font-size:13px">' + esc(N.DEF.FACCION) + '</span></div>' +
    '<p class="hint">' + esc(T("tr.faccion_hint")) + '</p>' +
    '<div class="campo"><span>' + T("tr.junta") + '</span><input type="text" data-tk="LootingBehaviour" value="' + esc(t.LootingBehaviour || "") + '" placeholder="' + esc(T("comun.por_defecto", N.DEF.LOOTING)) + '"></div>' +
    '<p class="hint">' + T("tr.tte_hint") + '</p></div>';
  return h;
}

function inspM(st) {
  var svc = S.svc, h = '<div class="cab"><span class="eyebrow">' + esc(T("m.cab", st.Mark, km(st.DistM || 0))) + '</span><h2>' + T("m.la_parada") + '</h2></div>';
  if (!N.marcasActivas(svc)) return h + '<p class="nota">' + T("m.automatico") + '</p>';
  h += '<div class="bloque"><fieldset><legend>' + T("m.esta_marca") + '</legend>' + seg("m-modo", st.Mode, [["always", T("m.siempre")], ["wave_only", T("m.solo_saludan")], ["off", T("m.no_para")]]);
  if (st.Mode === "wave_only") h += '<div class="campo"><span>' + T("m.radio_saludo") + '</span><input type="number" min="1" max="80" data-sk="WaveRadiusM" value="' + (st.WaveRadiusM || "") + '" placeholder="' + esc(T("comun.por_defecto", N.DEF.STOP_WAVE_RADIUS_M)) + '"></div>';
  h += '</fieldset></div>';
  if (st.Mode === "off") return h + '<p class="nota">' + T("m.pasa") + '</p>';
  h += '<div class="bloque"><fieldset><legend>' + T("m.quien_baja") + '</legend>';
  if (!(svc.Traders || []).length) h += '<p class="hint">' + T("m.ninguno_viaja") + '</p>';
  N.traders(svc).forEach(function (t) {
    h += '<label class="check"><input type="checkbox" data-baja="' + t.Seat + '"' + ((st.WhoGetsOff || []).indexOf(t.Seat) >= 0 ? " checked" : "") + '>' + esc(t.Name) + ' <span class="hint">' + T("m.plaza", t.Seat) + (t.Seat === 0 ? T("m.maneja") : "") + '</span></label>';
  });
  h += '<p class="hint">' + T("m.nadie_hint") + '</p></fieldset></div>';
  h += '<div class="bloque"><fieldset><legend>' + T("m.cada_uno") + '</legend>';
  if (!(st.WhoGetsOff || []).length) h += '<p class="hint">' + T("m.nadie_baja") + '</p>';
  N.traders(svc).forEach(function (t) {
    if ((st.WhoGetsOff || []).indexOf(t.Seat) < 0) return;
    var p = puesto(st, t.Seat), w = N.puestoXZ(st, t.Seat);
    if (!p || !w) return;
    h += '<div class="tarjeta' + (t.Seat === compSel ? " sel" : "") + '"><div class="tt">' + bola(t.Seat) + esc(t.Name) + ' <span class="hint">· ' + esc(t.TraderFile) + '</span></div>' +
      '<div class="xy"><label>' + T("m.puesto_x") + '<input type="number" step="0.1" id="x-' + t.Seat + '" data-xz="' + t.Seat + '" value="' + w[0] + '"></label><label>' + T("m.puesto_z") + '<input type="number" step="0.1" id="z-' + t.Seat + '" data-xz="' + t.Seat + '" value="' + w[1] + '"></label></div>' +
      '<span class="rel" id="rel-' + t.Seat + '">' + relTxt(st, t.Seat) + '</span>' +
      '<div class="xy"><label>' + T("a.tiempo_vendiendo") + '<input type="number" min="0" step="5" data-pt="' + t.Seat + '" value="' + p.TimeS + '"></label><label>' + T("m.va") + '<select data-pv="' + t.Seat + '"><option value="WALK"' + (p.MovementSpeed !== "JOG" ? " selected" : "") + '>' + T("m.caminando") + '</option><option value="JOG"' + (p.MovementSpeed === "JOG" ? " selected" : "") + '>' + T("m.trotando") + '</option></select></label></div>' +
      campoRumbo(t.Seat, p) + '</div>';
  });
  h += '</fieldset></div>';
  h += bloqueRumbos(st);
  h += bloqueSenales(st);
  h += '<div class="bloque"><fieldset><legend>' + T("m.la_vuelta") + '</legend><div class="campo"><span>' + T("m.tope") + '</span><input type="number" min="5" step="5" data-sk="BoardTimeoutS" value="' + (st.BoardTimeoutS === undefined ? "" : st.BoardTimeoutS) + '" placeholder="' + esc(T("comun.por_defecto", N.DEF.BOARD_TIMEOUT_S)) + '"></div>' +
       '<p class="nota clave">' + T("m.sigue") + '</p>' +
       // la bocina al salir (Stops[].HornOnDepart): un toque y sale 2 s despues
       '<label class="check"><input type="checkbox" data-skb="HornOnDepart"' + (st.HornOnDepart === 1 ? " checked" : "") + '>' + T("m.bocina") + '</label>' +
       '<p class="hint">' + esc(T("m.bocina_hint")) + '</p></fieldset></div>';
  if (S.toma) h += '<div class="bloque"><button class="btn" type="button" data-a="reanclar">' + T("m.reanclar") + '</button><p class="hint">' + T("m.reanclar_hint") + '</p></div>';
  return h;
}

// hacia donde mira en su puesto (HeadingDeg, o LookAt del semicirculo)
function campoRumbo(seat, p) {
  var rb = N.rumboDePuesto(p), mirarA = rb && rb.fuente === "LookAt";
  var txt = rb ? (mirarA ? T("rb.mira_punto", p.LookAt.join(" : "), Math.round(rb.deg)) : T("rb.mira_a", Math.round(rb.deg))) : T("rb.sin");
  return '<div class="xy"><label>' + T("rb.campo") + '<input type="number" step="5" id="rb-' + seat + '" data-rb="' + seat + '" value="' + (rb && !mirarA ? p.HeadingDeg : "") + '" placeholder="' + esc(mirarA ? T("rb.al_punto", Math.round(rb.deg)) : T("rb.sin_orientacion")) + '"></label>' +
    '<span class="rel" style="align-self:end">' + esc(txt) + '</span></div>';
}
// los atajos de la orientacion (semicirculo alrededor de un centro, todos a la calle, sin orientacion)
function bloqueRumbos(st) {
  if (!st.Pos || !bajanConPuesto(st).length) return "";
  var fo = formaDe(st);
  return '<div class="bloque"><fieldset><legend>' + T("rb.hacia_donde") + '</legend>' +
    '<div class="xy"><label>' + T("rb.centro_x") + '<input type="number" step="0.5" id="cen-x" data-fo="cx" value="' + fo.cx + '"></label><label>' + T("rb.centro_z") + '<input type="number" step="0.5" id="cen-z" data-fo="cz" value="' + fo.cz + '"></label></div>' +
    '<div class="xy"><label>' + T("rb.radio") + '<input type="number" min="1" step="0.5" data-fo="r" value="' + fo.r + '"></label><label>' + T("rb.arco") + '<input type="number" min="0" max="360" step="15" data-fo="a" value="' + fo.a + '"></label></div>' +
    '<div style="display:flex;gap:8px;flex-wrap:wrap"><button class="btn" type="button" data-a="semicirculo">' + T("rb.semicirculo") + '</button>' +
    '<button class="btn" type="button" data-a="calle">' + T("rb.calle") + '</button><button class="btn" type="button" data-a="sin-rumbo">' + T("rb.sin_boton") + '</button></div>' +
    '<p class="hint">' + esc(T("rb.hint")) + '</p></fieldset></div>';
}

// las senales de la parada
function bloqueSenales(st) {
  var l = st.Signals || [], h = '<div class="bloque"><fieldset><legend>' + T("sen.titulo") + '</legend>' +
    '<p class="hint">' + T("sen.hint") + '</p>';
  l.forEach(function (sg, i) {
    var tp = N.senalTipo(sg), pos = Array.isArray(sg.Pos) ? sg.Pos : ["", ""];
    // el humo de airdrop es siempre rojo y no se renueva: el color queda fijo y el "cada" no se usa
    var air = tp === "airdrop", colN = air ? "red" : N.senalColor(sg), col = COLOR_SENAL[colN] || COLOR_SENAL.green, colores = air ? ["red"] : N.SENAL_COLORES;
    var clase = tp === "auto" ? T("sen.dia_noche", N.claseSenal(sg, false)) : N.claseSenal(sg, false);
    h += '<div class="tarjeta' + (i === senalSel ? " sel" : "") + '"><div class="tt"><span class="bola" style="background:' + col + ';color:' + (colN === "white" || colN === "yellow" ? "#1D221C" : "#fff") + '">S' + (i + 1) + '</span>' + T("sen.senal", i + 1) + ' <span class="hint">· ' + esc(clase) + '</span>' +
      '<button class="btn" type="button" data-a="quitar-senal" data-i="' + i + '" aria-label="' + T("sen.quitar", i + 1) + '" style="margin-left:auto">×</button></div>' +
      '<div class="xy"><label>' + T("sen.tipo") + '<select data-sg="Type" data-i="' + i + '">' + N.SENAL_TIPOS.map(function (v) { return '<option value="' + v + '"' + (v === tp ? " selected" : "") + '>' + esc(TIPO_SENAL(v)) + '</option>'; }).join("") + '</select></label>' +
      '<label>' + T("sen.color") + (tp === "roadflare" || tp === "fireworks" ? T("sen.solo_humo") : (air ? T("sen.airdrop_color") : "")) + '<select data-sg="Color" data-i="' + i + '"' + (air ? " disabled" : "") + '>' + colores.map(function (v) { return '<option value="' + v + '"' + (v === colN ? " selected" : "") + '>' + esc(NOMBRE_COLOR(v)) + '</option>'; }).join("") + '</select></label></div>' +
      (air ? '<p class="hint">' + esc(T("sen.airdrop_hint")) + '</p>' : "") +
      '<div class="xy"><label>x<input type="number" step="0.5" id="sgx-' + i + '" data-sgxz="' + i + '" value="' + pos[0] + '"></label><label>z<input type="number" step="0.5" id="sgz-' + i + '" data-sgxz="' + i + '" value="' + pos[1] + '"></label></div>' +
      '<span class="rel" id="sgrel-' + i + '">' + relXZ(st, Array.isArray(sg.Pos) ? sg.Pos : null) + '</span>' +
      '<div class="xy"><label>' + T("sen.cada") + '<input type="number" min="0" step="10" data-sg="EverySeconds" data-i="' + i + '" value="' + (air || sg.EverySeconds === undefined ? "" : sg.EverySeconds) + '" placeholder="' + (air ? "—" : esc(T("comun.por_defecto", N.DEF.SIGNAL_EVERY_S))) + '"' + (air ? " disabled" : "") + '></label>' +
      '<label>' + T("sen.retardo") + '<input type="number" min="0" step="5" data-sg="StartDelayS" data-i="' + i + '" value="' + (sg.StartDelayS === undefined ? "" : sg.StartDelayS) + '" placeholder="' + esc(T("comun.por_defecto", N.DEF.SIGNAL_START_DELAY_S)) + '"></label></div></div>';
  });
  h += '<div style="display:flex;gap:8px;flex-wrap:wrap"><button class="btn" type="button" data-a="agregar-senal"' + (st.Pos ? "" : " disabled") + '>' + T("sen.agregar") + '</button>' +
    '<button class="btn" type="button" data-a="tres-senales"' + (st.Pos ? "" : " disabled") + '>' + T("sen.tres") + '</button></div>' +
    '<p class="hint">' + esc(T("sen.hint2", N.SENAL_TRIANGULO_M)) + '</p></fieldset></div>';
  return h;
}

function inspFIN() {
  var svc = S.svc, fin = svc.OnRouteEnd || {};
  var h = '<div class="cab"><span class="eyebrow">' + T("fin.cab") + '</span><h2>' + T("fin.condiciones") + '</h2></div>';
  h += '<div class="bloque"><fieldset><legend>' + T("fin.al_llegar") + '</legend>' + seg("fin", fin.Action, [["despawn", T("fin.desaparece")], ["respawn", T("fin.reaparece")], ["restart", T("fin.empieza")]]);
  if (fin.Action === "respawn") h += '<div class="campo"><span>' + T("fin.reaparece_a") + '</span>' + nro("OnRouteEnd.DelayS", 0, 'min="0" step="5"') + '</div>';
  h += '<div class="campo"><span>' + T("fin.espera") + '</span>' + nro("OnRouteEnd.WaitS", 0, 'min="0" step="5"') + '</div>';
  h += '<p class="hint">' + T("fin.hint") + '</p></fieldset></div>';
  return h;
}

function renderInsp() {
  var st = paradaSel();
  $("insp").innerHTML = nodoSel === "A" ? inspA() : (st ? inspM(st) : inspFIN());
}

var insp = $("insp");
insp.addEventListener("input", function (e) {
  var t = e.target, st = paradaSel(), tr = N.traderDe(S.svc, compSel), v;
  if (t.hasAttribute("data-k") && (t.type === "number" || t.type === "text")) {
    v = t.type === "number" ? (t.value === "" ? undefined : +t.value) : t.value;
    if (t.type === "text" && t.getAttribute("data-k") !== "Name" && t.value === "" && t.getAttribute("data-k").indexOf("Markers.") === 0) v = undefined;
    setv(S.svc, t.getAttribute("data-k"), v);
  } else if (t.hasAttribute("data-tk") && t.type === "text" && tr) {
    var k = t.getAttribute("data-tk");
    if (k === "LootingBehaviour" && t.value === "") delete tr[k]; else tr[k] = t.value;
    if (k === "Name") { var f = insp.querySelector('.fila-t[data-sel="' + compSel + '"] b'); if (f) f.textContent = t.value; }
  } else if (t.hasAttribute("data-sk") && st) {
    if (t.value === "") delete st[t.getAttribute("data-sk")]; else st[t.getAttribute("data-sk")] = +t.value;
  } else if (t.hasAttribute("data-xz") && st) {
    var seat = +t.getAttribute("data-xz"), x = +$("x-" + seat).value, z = +$("z-" + seat).value;
    if (isFinite(x) && isFinite(z)) { N.moverPuesto(st, seat, x, z); $("rel-" + seat).textContent = relTxt(st, seat); }
  } else if (t.hasAttribute("data-pt") && st) {
    var p = puesto(st, +t.getAttribute("data-pt")); if (p) p.TimeS = +t.value || 0;
  } else if (t.hasAttribute("data-rb") && st) {
    var prb = puesto(st, +t.getAttribute("data-rb"));
    if (prb) N.ponerRumbo(prb, t.value === "" ? null : +t.value);
  } else if (t.hasAttribute("data-fo") && st) {
    if (t.value !== "" && isFinite(+t.value)) formaDe(st)[t.getAttribute("data-fo")] = +t.value;
  } else if (t.hasAttribute("data-sgxz") && st) {
    var si = +t.getAttribute("data-sgxz"), sx = +$("sgx-" + si).value, sz = +$("sgz-" + si).value;
    if ($("sgx-" + si).value !== "" && $("sgz-" + si).value !== "" && isFinite(sx) && isFinite(sz)) { N.moverSenal(st, si, sx, sz); $("sgrel-" + si).textContent = relXZ(st, st.Signals[si].Pos); }
  } else if (t.hasAttribute("data-sg") && t.type === "number" && st && st.Signals && st.Signals[+t.getAttribute("data-i")]) {
    var sgn = st.Signals[+t.getAttribute("data-i")], kk = t.getAttribute("data-sg");
    if (t.value === "") delete sgn[kk]; else sgn[kk] = Math.max(0, +t.value || 0);
  } else if (t.hasAttribute("data-prenda") && tr) {
    tr.Clothing[+t.getAttribute("data-prenda")] = t.value.trim();
  } else if (t.id === "mk-color") {
    if (!N.ponerColorMarcador(S.svc, t.value)) return;
    var hexMk = N.colorMarcador(S.svc);
    [].forEach.call(insp.querySelectorAll(".muestra"), function (bm) { bm.setAttribute("aria-pressed", String(bm.getAttribute("data-color") === hexMk)); });
    $("mk-rgb").textContent = rgbMarcador(S.svc);
  } else if (t.id === "plazas") {
    return;   // se aplica al salir del campo (change)
  } else return;
  cambio(); renderEscena(); renderCinta(); renderJSON(); renderRevision(); renderChips(); renderDonde();
});
insp.addEventListener("change", function (e) {
  var t = e.target, st = paradaSel(), tr = N.traderDe(S.svc, compSel), svc = S.svc;
  if (t.type === "radio") {
    var k = t.getAttribute("data-r");
    if (k === "modo") { svc.StopMode = t.value; svc.Wave = svc.Wave || {}; svc.Wave.Enabled = t.value === "marks" ? 0 : 1; }
    else if (k === "m-modo" && st) st.Mode = t.value;
    else if (k === "fin") { svc.OnRouteEnd = svc.OnRouteEnd || {}; svc.OnRouteEnd.Action = t.value; }
    else if (k === "ropamodo" && tr) {
      if (t.value === "custom") { tr.Loadout = ""; if (!Array.isArray(tr.Clothing) || !tr.Clothing.length) tr.Clothing = [""]; }
      else { tr.Clothing = []; if (!tr.Loadout) tr.Loadout = SRV.loadouts.indexOf("SurvivorLoadout") >= 0 || !SRV.loadouts.length ? "SurvivorLoadout" : SRV.loadouts[0]; }
    }
  } else if (t.type === "checkbox" && t.getAttribute("data-k") === "Markers.useMapMarker") {
    N.ponerMarcador(svc, t.checked);
  } else if (t.type === "checkbox" && t.hasAttribute("data-k")) {
    setv(svc, t.getAttribute("data-k"), t.checked ? 1 : 0);
  } else if (t.type === "checkbox" && t.hasAttribute("data-tk") && tr) {
    tr[t.getAttribute("data-tk")] = t.checked ? 1 : 0;
  } else if (t.type === "checkbox" && t.hasAttribute("data-skb") && st) {
    st[t.getAttribute("data-skb")] = t.checked ? 1 : 0;
  } else if (t.hasAttribute("data-baja") && st) {
    N.ponerBaja(svc, st, +t.getAttribute("data-baja"), t.checked);
  } else if (t.id === "chofer") {
    if (t.value === "" || !N.ponerQuienManeja(svc, +t.value)) return;
    compSel = 0;
  } else if (t.id === "plazas") {
    var n = Math.max(1, Math.min(30, parseInt(t.value, 10) || 0));
    svc.Editor.Seats = n;
    PLAZAS_RECORDADAS[svc.VehicleClass] = n; guardarLS("bzt_editor_plazas_v1", PLAZAS_RECORDADAS);
    N.traders(svc).forEach(function (x) { if (x.Seat >= n) N.quitarComerciante(svc, x.Seat); });
  } else if (t.tagName === "SELECT" && t.hasAttribute("data-k")) {
    setv(svc, t.getAttribute("data-k"), t.value);
  } else if (t.tagName === "SELECT" && t.hasAttribute("data-tk") && tr) {
    tr[t.getAttribute("data-tk")] = t.value;
  } else if (t.hasAttribute("data-pv") && st) {
    var p = puesto(st, +t.getAttribute("data-pv")); if (p) p.MovementSpeed = t.value;
  } else if (t.tagName === "SELECT" && t.hasAttribute("data-sg") && st && st.Signals && st.Signals[+t.getAttribute("data-i")]) {
    st.Signals[+t.getAttribute("data-i")][t.getAttribute("data-sg")] = t.value;
    senalSel = +t.getAttribute("data-i");
  } else return;
  if (compSel !== null && !N.traderDe(svc, compSel)) { var ts = N.traders(svc); compSel = ts.length ? ts[0].Seat : null; }
  cambio(); todo();
});
insp.addEventListener("click", function (e) {
  var b = e.target.closest("button"); if (!b) return;
  var a = b.getAttribute("data-a"), sel = b.getAttribute("data-sel"), svc = S.svc, tr = N.traderDe(svc, compSel);
  if (sel !== null) { compSel = +sel; todo(); return; }
  // el primero que se agrega maneja (plaza 0) y vende sentado; los demas bajan en cada parada
  if (a === "mas") { var nt = N.agregarComerciante(svc, ""); if (nt) { compSel = nt.Seat; if (nt.Seat !== 0) (svc.Stops || []).forEach(function (st) { N.ponerBaja(svc, st, nt.Seat, true); }); } }
  else if (a === "menos") { var ts = N.traders(svc); if (ts.length) N.quitarComerciante(svc, ts[ts.length - 1].Seat); }
  else if (a === "agregar-prenda" && tr) { tr.Clothing = tr.Clothing || []; tr.Clothing.push(""); }
  else if (a === "quitar-prenda" && tr) { tr.Clothing.splice(+b.getAttribute("data-i"), 1); }
  else if (a === "secuencia") {
    var fijaNueva = !N.esSecuenciaFija(svc);
    N.secuenciaFija(svc, fijaNueva);
    aviso("svc-toast", fijaNueva ? T("h.fija_si") : T("h.fija_no"));
  }
  else if (a === "agregar-senal" && paradaSel()) { var ps = paradaSel(); if (N.agregarSenal(ps)) { senalSel = ps.Signals.length - 1; VISTAS[claveParada(ps)] = vistaAjustada(ps); } }
  else if (a === "quitar-senal" && paradaSel()) { N.quitarSenal(paradaSel(), +b.getAttribute("data-i")); senalSel = -1; }
  else if (a === "tres-senales" && paradaSel()) {
    var habia = N.tresSenales(paradaSel()); senalSel = -1;
    VISTAS[claveParada(paradaSel())] = vistaAjustada(paradaSel());
    cambio(); todo();
    aviso("svc-toast", T("h.tres", N.SENAL_TRIANGULO_M, habia > 0 ? T("h.tres_reemplazo", habia) : ""));
    return;
  }
  else if (a === "semicirculo" && paradaSel()) {
    var pz = paradaSel(), fo = formaDe(pz), ns = N.semicirculo(svc, pz, [fo.cx, fo.cz], fo.r, fo.a);
    VISTAS[claveParada(pz)] = vistaAjustada(pz);
    cambio(); todo();
    aviso("svc-toast", ns ? T("h.semicirculo", ns, fo.r, fo.cx, fo.cz, fo.a) : T("h.nadie_baja"));
    return;
  }
  else if (a === "calle" && paradaSel()) { var nc = N.mirandoALaCalle(svc, paradaSel()); cambio(); todo(); aviso("svc-toast", T("h.calle", nc)); return; }
  else if (a === "sin-rumbo" && paradaSel()) { (paradaSel().Traders || []).forEach(function (pp) { N.ponerRumbo(pp, null); }); }
  else if (b.hasAttribute("data-color")) { N.ponerColorMarcador(svc, b.getAttribute("data-color")); }
  else if (a === "reanclar" && S.toma) { VISTAS = {}; var c = N.reanclar(svc, S.toma); aviso("svc-toast", c.length ? T("h.reanclado", c.join(", ")) : T("h.ya_coincidia")); }
  else return;
  if (compSel !== null && !N.traderDe(svc, compSel)) { var t2 = N.traders(svc); compSel = t2.length ? t2[0].Seat : null; }
  cambio(); todo();
});
$("esc").addEventListener("click", function (e) {
  var z = e.target.closest("[data-z]"); if (z) { zoomParada(z.getAttribute("data-z")); return; }
  var za = e.target.closest("[data-za]"); if (za) { zoomAsientos(za.getAttribute("data-za")); return; }
  var g = e.target.closest(".plaza"); if (!g) return;
  var seat = +g.getAttribute("data-seat");
  if (N.traderDe(S.svc, seat)) { compSel = seat; todo(); }
});
function primerArchivo() {
  var k = Object.keys(SRV.comerciantes).sort();
  return k.length ? SRV.comerciantes[k[0]].archivo : "";
}

// ============================================================================ revision
function ctxAvisos() {
  return { toma: S.toma, comerciantes: SRV.leido ? SRV.comerciantes : null, categorias: SRV.leido ? SRV.categorias : null,
           hardline: SRV.hardline, catalogoLeido: SRV.leido && Object.keys(SRV.comerciantes).length > 0,
           mapa: CF.conf && CF.conf.mision ? N.mapaDeMision(CF.conf.mision) : null, plazas: S ? plazasVeh(S.svc.VehicleClass) : null,
           mapaNav: SRV.leido ? SRV.mapaNav : null };
}
function revisar() {
  var out = N.exportar(S.svc), v = N.validar(out), a = N.avisos(out, ctxAvisos());
  return { errores: v.filter(function (x) { return x.nivel === "ERROR"; }).map(function (x) { return x.ruta + ": " + x.msg; }).concat(a.filter(function (x) { return x.nivel === "ERROR"; }).map(function (x) { return x.msg; })),
           avisos: v.filter(function (x) { return x.nivel === "AVISO"; }).map(function (x) { return x.ruta + ": " + x.msg; }).concat(a.filter(function (x) { return x.nivel === "AVISO"; }).map(function (x) { return x.msg; })),
           info: (S.sacadas || []).map(function (x) { return typeof x === "string" ? T("rev.se_saca", x) : x.msg; }).concat(a.filter(function (x) { return x.nivel === "info"; }).map(function (x) { return x.msg; })) };
}
function renderRevision() {
  var r = revisar(), h = "";
  r.errores.forEach(function (x) { h += '<li class="ERROR">' + esc(x) + '</li>'; });
  r.avisos.forEach(function (x) { h += '<li class="AVISO">' + esc(x) + '</li>'; });
  r.info.forEach(function (x) { h += '<li class="info">' + esc(x) + '</li>'; });
  if (!h) h = '<li class="info">' + T("rev.sin") + '</li>';
  $("rev-lista").innerHTML = h;
  var bloqueado = r.errores.length > 0;
  ["g-todo", "g-svc", "g-crew"].forEach(function (id) { $(id).disabled = bloqueado; });
  var z = S.svc.TraderZone || {};
  $("g-zona").hidden = !(z.Name && SRV.leido && carpeta("zonas") && !SRV.zonas[String(z.Name).toLowerCase()]);
  $("g-destino").innerHTML = bloqueado ? T("rev.corregi") : (FS.escribe() ? T("rev.escribe") : (window.showSaveFilePicker ? T("rev.picker") : T("rev.descarga")));
}

// ============================================================================ salida
function archivoServicio() { return S.archivo || (S.svc.Name + ".json"); }
function renderJSON() {
  $("json").textContent = N.textoServicio(N.exportar(S.svc));
  $("json-destino").innerHTML = T("json.destino", esc(unir(carpeta("config"), archivoServicio())));
}
$("copiar").addEventListener("click", function () {
  var txt = $("json").textContent;
  function sel() { var r = document.createRange(); r.selectNodeContents($("json")); var s = window.getSelection(); s.removeAllRanges(); s.addRange(r); aviso("toast", T("json.seleccionado")); }
  if (navigator.clipboard && navigator.clipboard.writeText) navigator.clipboard.writeText(txt).then(function () { aviso("toast", T("json.copiado")); }, sel); else sel();
});

function renderDonde() {
  var el = $("donde");
  if (!S || FS.modo() === "ninguno" || !SRV.leido) { el.hidden = true; return; }
  var L = [], base = "BZBusRoute_" + N.nombreCorto(S.svc.Route), z = S.svc.TraderZone || {};
  L.push([T("donde.toma"), unir(carpeta("rutas"), base + "_hdr.json"), S.toma ? (S.toma.master === false ? "info" : "si") : "no",
          S.toma ? (S.toma.master === false ? T("donde.toma_sin_master", base) : T("donde.toma_con", base)) : (TX(TOMA_FALTA) || T("donde.no_esta"))]);
  L.push([T("donde.servicio"), unir(carpeta("config"), archivoServicio()), SRV.servicios.indexOf(archivoServicio()) >= 0 ? "si" : "info", SRV.servicios.indexOf(archivoServicio()) >= 0 ? "" : T("donde.nuevo")]);
  N.traders(S.svc).forEach(function (t) {
    var d = SRV.comerciantes[String(t.TraderFile || "").toLowerCase()];
    L.push([T("donde.comerciante", t.Name, t.Seat), d ? unir(d.carpeta, d.archivo + ".json") : unir(carpeta("traders"), (t.TraderFile || "?") + ".json"), d ? "si" : "no", d ? "" : (t.TraderFile ? T("donde.no_market") : T("donde.falta_elegir"))]);
  });
  if (z.Name) {
    if (carpeta("zonas")) { var zi = SRV.zonas[String(z.Name).toLowerCase()]; L.push([T("donde.zona"), unir(carpeta("zonas"), z.Name + ".json"), zi ? "si" : "info", zi ? "" : T("donde.zona_no_existe")]); }
    else L.push([T("donde.zona"), "", "info", T("donde.zona_sin_mision")]);
  }
  el.hidden = false;
  el.innerHTML = L.map(function (x) { return '<li class="' + x[2] + '"><span><b>' + esc(x[0]) + '</b>' + (x[1] ? ' <span class="mono">' + esc(ver(x[1])) + '</span>' : "") + (x[3] ? ' <span class="hint">' + esc(x[3]) + '</span>' : "") + '</span></li>'; }).join("");
}

function todo() {
  var hay = !!S;
  ["zona-trabajo", "cinta-caja", "trabajo", "revision", "guardar", "salida", "paso3-tit"].forEach(function (id) { $(id).hidden = !hay; });
  renderChips();
  renderDonde();
  if (!hay) return;
  renderCinta(); renderEscena(); renderInsp(); renderRevision(); renderJSON();
  aviso("svc-estado", (S.archivo ? T("estado.abierto", S.archivo) : T("estado.nuevo", archivoServicio())) + (S.toma ? T("estado.toma", S.toma.nombre, S.toma.origenMarcasK ? T(S.toma.origenMarcasK) : S.toma.origenMarcas) : T("estado.sin_toma")) + ". " + T("donde.titulo") + ":");
}

// ============================================================================ guardar
function descargar(nombre, texto) {
  var a = document.createElement("a");
  a.href = URL.createObjectURL(new Blob([texto], { type: "application/json" }));
  a.download = nombre; document.body.appendChild(a); a.click();
  setTimeout(function () { URL.revokeObjectURL(a.href); a.remove(); }, 1000);
}
function res(clase, texto) { var li = document.createElement("li"); li.className = clase; li.textContent = texto; $("g-res").appendChild(li); }

// Escribe con respaldo de lo que pisa. Sin permiso de escritura: un cuadro de guardar que arranca en la carpeta que
// corresponde (startIn + un id por tipo: Chrome recuerda el ultimo lugar); sin eso (Firefox), una descarga que dice
// adonde va. true si quedo escrito, guardado o descargado.
async function escribirConRespaldo(rel, texto, carpetaRespaldo, que, idTipo) {
  var nombre = rel.split("/").pop();
  if (!FS.escribe()) {
    if (window.showSaveFilePicker) {
      try {
        var opts = { suggestedName: nombre, id: idTipo || "bzt-archivos", types: [{ description: "JSON", accept: { "application/json": [".json"] } }] };
        var dh = await FS.dirHandle(dirDe(rel));
        if (dh) opts.startIn = dh;
        var fh = await window.showSaveFilePicker(opts), w = await fh.createWritable();
        await w.write(texto); await w.close();
        res("ok", T("g.guardado_como", que, fh.name, ver(dirDe(rel))));
        return true;
      } catch (e) { if (e.name === "AbortError") { res("warn", T("g.cancelado", que)); return false; } }
    }
    descargar(nombre, texto);
    res("warn", T("g.descargado", que, nombre, ver(dirDe(rel))));
    return true;
  }
  var cual = rel;      // el que falla, si falla: el respaldo o el archivo
  try {
    var viejo = await FS.leer(rel);
    if (viejo && viejo.texto === texto) { res("ok", T("g.sin_cambios", que, ver(rel))); return true; }
    if (viejo) {
      var resp = unir(carpetaRespaldo, N.nombreRespaldo(nombre));
      cual = resp;
      await FS.escribir(resp, viejo.texto);
      res("ok", T("g.respaldo", que, ver(resp)));
    }
    cual = rel;
    await FS.escribir(rel, texto);
    res("ok", T("g.escrito", que, ver(rel)));
    return true;
  } catch (e) {
    // con Editor.bat, Windows no deja escribir en esa carpeta: se descarga, como sin permiso, y dice adonde va
    if (e.sinPermiso) { descargar(nombre, texto); res("warn", T("g.api_sin_permiso", que, e.message, nombre, ver(dirDe(rel)))); return true; }
    res("err", T("g.no_se_pudo", que, ver(cual), e.message)); return false;
  }
}

async function guardarServicio() {
  var out = N.exportar(S.svc), texto = N.textoServicio(out), rel = unir(carpeta("config"), archivoServicio());
  try { JSON.parse(texto); } catch (e) { res("err", T("g.json_invalido")); return false; }
  var ok = await escribirConRespaldo(rel, texto, unir(carpeta("config"), "_bz_trader_backups"), T("g.que_servicio"), "bzt-servicios");
  if (ok) { S.archivo = archivoServicio(); S.original = N.copia(S.svc); S.sucio = false; S.sacadas = []; if (SRV.servicios.indexOf(S.archivo) < 0) { SRV.servicios.push(S.archivo); SRV.servicios.sort(); renderListas(); } }
  return ok;
}

async function guardarTripulacion() {
  var base = unir(carpeta("rutas"), N.nombreCorto(S.svc.Route) === "" ? "" : "BZBusRoute_" + N.nombreCorto(S.svc.Route));
  if (FS.modo() === "ninguno") { res("err", T("g.trip_carpeta")); return false; }
  var hdr = await FS.leer(base + "_hdr.json"), master = await FS.leer(base + ".json");
  if (!hdr) { res("err", T("g.trip_no_encuentro", ver(base + "_hdr.json"))); return false; }
  var dcRuta = ""; try { dcRuta = JSON.parse(N.sinBom(hdr.texto)).DriverClass || ""; } catch (e) { res("err", T("g.trip_hdr_invalido")); return false; }
  var tr = N.tripulacion(N.exportar(S.svc), dcRuta), archivos = [[base + "_hdr.json", hdr]].concat(master ? [[base + ".json", master]] : []), nuevos = [];
  for (var i = 0; i < archivos.length; i++) {
    var r = N.escribirTripulacion(archivos[i][1].texto, tr.driverClass, tr.crew);
    if (r.error) { res("err", T("g.trip_error", archivos[i][0], r.error)); return false; }
    nuevos.push([archivos[i][0], r]);
  }
  if (!master) res("warn", T("g.trip_sin_master", ver(base + ".json")));
  for (i = 0; i < nuevos.length; i++) {
    if (!nuevos[i][1].cambios.length) { res("ok", T("g.trip_ya", ver(nuevos[i][0]))); continue; }
    var ok = await escribirConRespaldo(nuevos[i][0], nuevos[i][1].texto, unir(carpeta("rutas"), "_bz_trader_backups"), T("g.trip_cambios", nuevos[i][1].cambios.join("; ")), "bzt-tomas");
    if (!ok) return false;
  }
  return true;
}

async function crearZona() {
  var z = S.svc.TraderZone || {}, nombre = String(z.Name || "");
  if (!nombre) return true;
  if (SRV.zonas[nombre.toLowerCase()]) { res("ok", T("g.zona_existe", nombre, ver(unir(carpeta("zonas"), nombre + ".json")))); return true; }
  if (!carpeta("zonas")) { res("warn", T("g.zona_sin_mision")); return true; }
  var ini = S.toma ? S.toma.inicio : (S.svc.Editor && S.svc.Editor.Start ? [S.svc.Editor.Start[0], 0, S.svc.Editor.Start[1]] : null);
  if (!ini) { res("err", T("g.zona_donde")); return false; }
  var radio = Math.max(z.RadiusM || 0, N.radioNecesario(S.svc));
  var texto = N.textoZona(N.zonaNueva(nombre, ini, radio, S.svc.Name));
  var ok = await escribirConRespaldo(unir(carpeta("zonas"), nombre + ".json"), texto, unir(carpeta("zonas"), "_bz_trader_backups"), T("g.que_zona"), "bzt-zonas");
  if (ok && FS.escribe()) SRV.zonas[nombre.toLowerCase()] = { nombre: nombre, mtime: Date.now(), nueva: true };
  return ok;
}

// Que entra en caliente y que pide reiniciar, segun el ultimo arranque del server (el RPT mas nuevo).
function enCaliente() {
  var L = [], svc = S.svc, boot = SRV.arranque ? SRV.arranque.fecha.getTime() : 0;
  var srvViejo = SRV.caliente === false;
  L.push(srvViejo ? T("cal.srv_viejo") : T("cal.srv"));
  L.push(T("cal.ruta"));
  (svc.Traders || []).forEach(function (t) {
    var d = SRV.comerciantes[String(t.TraderFile || "").toLowerCase()];
    if (!d || !boot) return;
    if (d.mtime > boot) L.push(T("cal.comerciante", d.archivo));
    d.categorias.forEach(function (c) {
      var cat = SRV.categorias[c.nombre.toLowerCase()];
      if (cat && cat.mtime > boot) L.push(T("cal.categoria", cat.archivo, d.archivo));
    });
  });
  var z = svc.TraderZone || {}, zi = z.Name ? SRV.zonas[String(z.Name).toLowerCase()] : null;
  if (zi && (zi.nueva || (boot && zi.mtime > boot))) L.push(T("cal.zona", zi.nombre));
  if (!boot) L.push(T("cal.sin_rpt", carpeta("perfil")));
  L.push(T("cal.recordatorio"));
  return L;
}

async function guardar(que) {
  $("g-res").innerHTML = "";
  var r = revisar();
  if (r.errores.length) { res("err", T("g.hay_errores")); return; }
  var ok = true;
  if (que === "todo" || que === "zona") ok = (await crearZona()) && ok;
  if (que === "todo" || que === "crew") ok = (await guardarTripulacion()) && ok;
  if (que === "todo" || que === "svc") ok = (await guardarServicio()) && ok;
  res(ok ? "ok" : "err", ok ? T("g.listo") : T("g.algo"));
  $("g-caliente").innerHTML = enCaliente().map(esc).join("<br>");
  todo();
}
$("g-todo").addEventListener("click", function () { guardar("todo"); });
$("g-svc").addEventListener("click", function () { guardar("svc"); });
$("g-crew").addEventListener("click", function () { guardar("crew"); });
$("g-zona").addEventListener("click", function () { guardar("zona"); });

// ============================================================================ abrir y armar
async function armarDesdeToma(tm) {
  var pl = N.plazasDe(tm.vehiculo, tm.seatsHdr, PLAZAS_RECORDADAS, nPlazasVeh(tm.vehiculo));
  var svc = N.servicioNuevo(tm, pl.n), precargados = 0;
  // la ruta ya trae comerciantes (DriverClass o Crew ExpansionTraderAI*): se precargan con su clase y su plaza; que
  // vende (TraderFile) queda vacio y la revision pide elegirlo
  if (/^ExpansionTraderAI/.test(tm.driverClass)) { var t0 = N.traderNuevo(0, ""); t0.TraderAIClass = tm.driverClass; svc.Traders.push(t0); precargados++; }
  (tm.crew || []).forEach(function (c) {
    if (!c || !/^ExpansionTraderAI/.test(c.cls || "") || !(c.seat >= 1) || N.traderDe(svc, c.seat) || (pl.n && c.seat >= pl.n)) return;
    var t = N.traderNuevo(c.seat, "");
    t.TraderAIClass = c.cls; if (c.loadout) t.Loadout = c.loadout;
    svc.Traders.push(t); precargados++;
  });
  svc.Traders.sort(function (a, b) { return a.Seat - b.Seat; });
  // sin un comerciante al volante en la ruta, maneja el primero
  if (!N.traderDe(svc, 0) && svc.Traders.length) N.ponerQuienManeja(svc, svc.Traders[0].Seat);
  svc.Stops.forEach(function (st) { N.traders(svc).forEach(function (t) { if (t.Seat >= 1) N.ponerBaja(svc, st, t.Seat, true); }); });
  var existe = SRV.servicios.indexOf(svc.Name + ".json") >= 0;
  abrirServicio(svc, "", tm);
  aviso("svc-toast", T("ab.nuevo", tm.nombre, tm.marcas.length) + (pl.n ? T("ab.plazas", pl.n, pl.fuente) : T("ab.plazas_elegir")) +
        (precargados ? T("ab.precargados", precargados) : "") + (existe ? T("ab.ya_hay", svc.Name) : "") + sinMaster(tm));
}
$("toma-nueva").addEventListener("click", async function () {
  var n = $("toma-sel").value; if (!n) return;
  if (S && S.sucio && !confirm(T("ab.descartar", archivoServicio()))) return;
  try { var tm = await tomaDeServer(n); if (!tm) { aviso("svc-toast", TX(TOMA_FALTA) || T("ab.no_pude_toma", n)); return; } await armarDesdeToma(tm); }
  catch (e) { aviso("svc-toast", T("ab.toma_error", n, e.message)); }
});
$("svc-abrir").addEventListener("click", async function () {
  var n = $("svc-sel").value; if (!n) return;
  if (S && S.sucio && !confirm(T("ab.descartar", archivoServicio()))) return;
  var j = await leerJson(unir(carpeta("config"), n));
  if (!j || !j.obj) { aviso("svc-toast", T("ab.no_pude", n) + (j && j.error ? " (" + j.error + ")" : "")); return; }
  if (!j.obj.Traders || !j.obj.Stops || !j.obj.Route) { aviso("svc-toast", T("ab.no_es", n)); return; }
  abrirServicio(j.obj, n, null);
  await cargarTomaDelServicio();
  todo();
  aviso("svc-toast", S.toma ? T("ab.abierto_con", n, S.toma.nombre, sinMaster(S.toma)) : T("ab.abierto_sin", n, TX(TOMA_FALTA) || T("ab.su_toma_no", S.svc.Route)));
});
// sin archivos sueltos. El navegador no deja leer los vecinos de un archivo elegido (el hdr
// sin su wp, el servicio sin su toma): sin permiso de escritura tambien se elige la CARPETA, solo para leer.


// ============================================================================ la carpeta: elegir, permiso, leer
// Elegir una carpeta con permiso de escritura. Queda recordada solo si es la del server de este editor (o el editor es
// una copia suelta): la de otro server se usa esta vez, con el aviso, y el editor no la carga solo la proxima.
async function elegirCarpeta() {
  if (!window.showDirectoryPicker || RAIZ) return;
  try {
    var opts = { id: "bztrader-server", mode: "readwrite" };
    if (FS.handle()) opts.startIn = FS.handle();
    var h = await window.showDirectoryPicker(opts);
    FS.usarHandle(h); handlePendiente = null;
    await leerServer();
    if (!SERVIDOR || SRV.esDelEditor) await guardarHandle(h);
  } catch (e) { if (e.name !== "AbortError") aviso("srv-estado", T("carp.no_abrir", e.message)); }
}
$("srv-elegir").addEventListener("click", elegirCarpeta);
// La carpeta recordada de este server, sin permiso todavia: el navegador lo pide con un clic (el de «Escanear server»)
var handlePendiente = null;
async function darPermiso() {
  var h = handlePendiente;
  try {
    var p = await h.requestPermission({ mode: "readwrite" });
    if (p === "granted") FS.usarHandle(h);
    else if ((await h.requestPermission({ mode: "read" })) === "granted") FS.usarHandle(h, true);
    else { aviso("srv-estado", T("carp.sin_permiso_leer", h.name)); return; }
    handlePendiente = null;
    await leerServer();
  } catch (e) { aviso("srv-estado", T("carp.sin_permiso", e.message)); }
}
// «Escanear server»: lee entero el server de este editor, de una. Con Editor.bat, sin elegir nada. Si no: vuelve a leer
// la carpeta abierta si es la suya; si no, la recordada de ESE server (el permiso, con este clic); la primera vez se elige
// (en Program Files, o sin File System Access, sin escritura: es lo unico que el navegador deja).
async function escanearServer() {
  var m = FS.modo();
  // fuera de la raiz de un server no escanea: lo repite y lleva al aviso de arriba
  if (RAIZ) { aviso("srv-estado", T("raiz.estado")); $("raiz-aviso").scrollIntoView({ block: "nearest" }); $("raiz-aviso").focus(); return; }
  if (m === "api") { await leerServer(); return; }
  if ((m === "acceso" || m === "lecturaHandle") && (!SERVIDOR || SRV.esDelEditor)) { await leerServer(); return; }
  if (!handlePendiente) {
    var h = await cargarHandle();
    if (h && h.requestPermission && FS.handle() !== h) handlePendiente = h;
  }
  if (handlePendiente) { await darPermiso(); return; }
  if (!window.showDirectoryPicker || (SERVIDOR && SERVIDOR.programFiles)) { $("srv-lectura").click(); return; }
  await elegirCarpeta();
}
$("srv-escanear").addEventListener("click", escanearServer);
$("srv-lectura").addEventListener("change", async function (e) {
  if (!e.target.files.length) return;
  FS.usarLista(e.target.files); e.target.value = ""; handlePendiente = null;
  await leerServer();
});
$("srv-mapa").addEventListener("click", function (e) {
  if (e.target.id !== "cambiar-mapa") return;
  SRV.eligiendo = true; renderServer();
});
$("srv-confs").addEventListener("click", function (e) {
  var b = e.target.closest("button[data-conf]"); if (!b) return;
  var k = b.getAttribute("data-conf");
  if (k === "mano") { elegirConf($("conf-perfil").value, $("conf-mision").value, []); return; }
  var c = SRV.confs[+k]; if (c) elegirConf(c.perfil, c.mision, c.bats);
});
$("carpetas").addEventListener("change", function (e) {
  var k = e.target.getAttribute("data-cf"); if (!k) return;
  if (e.target.value.trim()) CF[k] = norm(e.target.value); else delete CF[k];
  guardarCF(); if (FS.modo() !== "ninguno" && SRV.tipo !== "otra" && !SRV.eligiendo) escanear(); else renderServer();
});
$("cf-def").addEventListener("click", function () {
  ["rutas", "traders", "traders2", "market", "loadouts", "config", "zonas"].forEach(function (k) { delete CF[k]; });
  guardarCF(); if (FS.modo() !== "ninguno" && SRV.tipo !== "otra" && !SRV.eligiendo) escanear(); else renderServer();
});

window.addEventListener("beforeunload", function (e) { if (S && S.sucio) { e.preventDefault(); e.returnValue = ""; } });

// Para las pruebas automaticas: la misma pagina sobre una carpeta del navegador
// (OPFS), que tiene la misma API que la carpeta del server elegida con el selector. No se usa en el uso normal.
// recordar = guardarla como la elegida con el selector (para probar que se recuerda al recargar), con la misma regla:
// solo si es la del server de este editor (o el editor es una copia suelta).
window.BZT_EDITOR = { usarCarpeta: async function (h, recordar) { FS.usarHandle(h); handlePendiente = null; await leerServer(); if (recordar && (!SERVIDOR || SRV.esDelEditor)) await guardarHandle(h); } };

// ============================================================================ el idioma y el manual
// ingles por defecto, espanol como opcion; cambia en el acto, sin recargar, y se recuerda.
function aplicarIdioma() {
  document.documentElement.lang = LANG;
  document.title = T("pagina.titulo");
  [].forEach.call(document.querySelectorAll("[data-i18n]"), function (el) { el.textContent = T(el.getAttribute("data-i18n")); });
  [].forEach.call(document.querySelectorAll("[data-i18n-html]"), function (el) { el.innerHTML = T(el.getAttribute("data-i18n-html")); });
  [].forEach.call(document.querySelectorAll("[data-i18n-ph]"), function (el) { el.placeholder = T(el.getAttribute("data-i18n-ph")); });
  [].forEach.call(document.querySelectorAll("[data-i18n-title]"), function (el) { el.title = T(el.getAttribute("data-i18n-title")); });
  [].forEach.call(document.querySelectorAll("[data-i18n-aria]"), function (el) { el.setAttribute("aria-label", T(el.getAttribute("data-i18n-aria"))); });
  [].forEach.call(document.querySelectorAll("#idioma button[data-lang]"), function (b) { b.setAttribute("aria-pressed", String(b.getAttribute("data-lang") === LANG)); });
}
$("idioma").addEventListener("click", function (e) {
  var b = e.target.closest("button[data-lang]"); if (!b) return;
  var l = b.getAttribute("data-lang") === "es" ? "es" : "en";
  if (l === LANG) return;
  LANG = l; N.idioma(LANG);
  // los avisos de la ultima accion quedaron en el otro idioma: se borran
  aviso("manual-aviso", ""); aviso("svc-toast", ""); aviso("toast", "");
  // los nombres por defecto de los comerciantes del servicio abierto pasan al idioma nuevo (los propios no se tocan)
  var renombrados = S ? N.nombresPorDefecto(S.svc, LANG) : 0;
  if (renombrados) { cambio(); aviso("svc-toast", T("idioma.nombres", renombrados)); }
  try { localStorage.setItem("bzt_editor_lang", LANG); } catch (e2) { /* sin localStorage: vale para esta vez */ }
  aplicarIdioma();
  renderServer(); renderListas(); todo();
});
// El manual en PDF, en el idioma del editor: suelto en la raiz del mod (desde tools\editor\, ../../). Servido por http
// se sabe si esta; abierto como archivo el navegador no deja mirar, y la pestana nueva dice si falta.
$("manual").addEventListener("click", async function () {
  var archivo = LANG === "es" ? "MANUAL_BZ_Trader.es.pdf" : "MANUAL_BZ_Trader.pdf";
  var url = new URL("../../" + archivo, location.href), dir = decodeURIComponent(new URL("./", url).href);
  if (url.protocol === "file:") dir = dir.replace(/^file:\/\/\//, "").replace(/\//g, "\\");
  if (url.protocol === "http:" || url.protocol === "https:") {
    var hay = false;
    try { hay = (await fetch(url.href, { method: "HEAD", cache: "no-store" })).ok; } catch (e) { hay = false; }
    if (!hay) { aviso("manual-aviso", T("man.falta", archivo, dir)); return; }
    aviso("manual-aviso", "");
  } else aviso("manual-aviso", T("man.abre", archivo, dir));
  window.open(url.href, "_blank", "noopener");
});

// ============================================================================ Editor.bat
// Editor.bat sirve el editor en http://localhost:<puerto>/ con la clave de la sesion en el enlace (#k=...). La API lee y
// escribe solo dentro del server donde vive el editor. Sin la clave, o si la API no contesta, el editor sigue como siempre.
var API_FALLA = "";
async function iniciarApi() {
  var k = /[#&?]k=([0-9a-f]{16,128})/i.exec((location.hash || "") + "&" + (location.search || ""));
  if (!k || !/^https?:$/.test(location.protocol)) return false;
  var base = location.origin + "/";
  try {
    var r = await fetch(base + "api/info", { headers: { "X-BZT-Key": k[1] }, cache: "no-store" });
    if (!r.ok) { API_FALLA = T("api.no_responde", "HTTP " + r.status); return false; }
    var info = await r.json();
    SERVIDOR = N.datosServer(info.servidor, info.mod);
    FS.usarApi({ base: base, clave: k[1], nombre: SERVIDOR.nombre });
    // Editor.bat mira al arrancar si al lado del mod hay DayZServer_x64.exe o mpmissions
    RAIZ = info.esServer === false ? { donde: info.modDir || "", mod: info.mod || "" } : null;
    latir(base, k[1]);
    return true;
  } catch (e) { API_FALLA = T("api.no_responde", e.message); return false; }
}

// ★ La ventana de Editor.bat termina con el editor. Cada pestaña se anota con un id y late cada LATIDO_MS (y apenas
// vuelve a verse o a despertar); al irse avisa (sendBeacon: no lleva encabezados, la clave va en la URL). Editor.ps1
// termina cuando no queda ninguna: tras el aviso de la ultima espera unos segundos (una recarga vuelve a latir) y una
// que dejo de latir cuenta como cerrada despues de su tope (los navegadores frenan los timers de una pestaña oculta
// hasta uno por minuto). Si el latido no llega, la ventana ya termino: se avisa en la pagina.
var LATIDO_MS = 15000;
function latir(base, clave) {
  var tab = Math.random().toString(36).slice(2, 12) + Date.now().toString(36);
  var uno = function () {
    fetch(base + "api/latido?tab=" + tab, { method: "POST", headers: { "X-BZT-Key": clave }, cache: "no-store" })
      .then(function (r) { if (r.ok) avisoCerrado(false); })
      .catch(function () { avisoCerrado(true); });
  };
  uno();
  setInterval(uno, LATIDO_MS);
  document.addEventListener("visibilitychange", function () { if (!document.hidden) uno(); });
  document.addEventListener("resume", uno);
  window.addEventListener("pageshow", function (e) { if (e.persisted) uno(); });
  window.addEventListener("pagehide", function () {
    try { navigator.sendBeacon(base + "api/adios?tab=" + tab + "&k=" + clave); } catch (e) { /* el tope del latido lo cubre */ }
  });
}
function avisoCerrado(si) {
  var a = $("bzt-cerrado");
  if (!si) { if (a) a.remove(); return; }
  if (a) return;
  document.body.insertAdjacentHTML("afterbegin", '<div id="bzt-cerrado" role="alert" style="position:sticky;top:0;z-index:50;padding:10px 16px;background:var(--warn,#b45309);color:#fff;font-weight:600">' + esc(T("api.cerrado")) + '</div>');
}

// ============================================================================ arranque
(async function () {
  // el editor abre LIMPIO; la sesion es el archivo del servicio en el server. Se
  // borran las claves de las versiones anteriores (el trabajo restaurado y la lista de sesiones).
  try { localStorage.removeItem("bzt_editor_trabajo_v1"); localStorage.removeItem("bzt_editor_sesiones_v1"); } catch (e) { /* sin localStorage */ }
  aplicarIdioma();
  renderServer(); renderListas(); todo();
  // con Editor.bat: la API local dice el server y se escanea solo, sin elegir nada (si es un server)
  if (await iniciarApi()) { if (RAIZ) { renderServer(); renderListas(); } else await leerServer(); return; }
  renderServer();
  var h = RAIZ ? null : await cargarHandle();
  if (h && h.queryPermission) {
    try {
      var p = await h.queryPermission({ mode: "readwrite" });
      if (p === "granted") { FS.usarHandle(h); await leerServer(); }
      else { handlePendiente = h; renderServer(); }      // «Escanear server» pide el permiso (un clic)
    } catch (e) { /* el handle ya no sirve: se elige de nuevo */ }
  }
})();
})();
