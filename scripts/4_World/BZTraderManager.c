// ============================================================================
//  EL MANAGER: uno solo en el server, con un runtime por SERVICIO (un archivo cada uno en
//  $profile:BZ_Trader/). Arranca unos segundos despues de MissionServer.OnInit (como TTE):
//  el Market y AutoDrive ya inicializaron y el mundo ya restauro lo persistido. Despues, un
//  tick de 1 s que no maneja nada: solo decide cuando cambiar de estado.
//
//  ★ EN CALIENTE (convertir la toma y armar el servicio en el editor con el server y el cliente
//  corriendo): cada RECARGA_S se vuelven a listar y leer los archivos. Un archivo NUEVO se lanza; uno CAMBIADO se
//  aplica en el proximo lanzamiento de su servicio (ya, si esta esperando); con Enable 0 o BORRADO, su servicio se
//  retira (al terminar la parada en curso: nunca con comerciantes abajo). Un archivo que no se puede usar no tumba al
//  que corre: sigue la version anterior y el log lo dice una vez.
// ============================================================================

// Un archivo de servicio: el texto de la version que corre, su runtime y la version que espera aplicarse.
class BZTRArchivo {
    string m_Nombre;                 // "TRADER_X.json"
    string m_Texto;                  // la version vigente (la que corre, o la ultima vista si no corre)
    string m_TextoMalo;              // el ultimo texto que no se pudo usar: se avisa una sola vez
    string m_TextoPendiente;         // la version nueva, esperando el proximo lanzamiento
    ref BZTRServicio m_Pendiente;
    ref BZTRRuntime m_Rt;            // null = apagado (Enable 0) o sin poder leerse
    bool   m_RetiroPedido;
    bool   m_Visto;                  // marca del ultimo barrido

    void BZTRArchivo(string nombre) {
        m_Nombre = nombre;
    }
}

class BZTRManager {
    static const string DIR         = "$profile:BZ_Trader";
    static const string SUB         = "BZ_Trader";   // la carpeta, para el listado por el -profiles relativo
    static const string PATRON      = "*.json";
    static const string EJEMPLO     = "EXAMPLE_service.json";   // el ejemplo del primer arranque (en ingles, como el editor por defecto)
    static const int    TICK_MS     = 1000;
    static const int    TICK_RAPIDO_MS = 250;   // los saludos vigentes (solo si hay): la ventana para empezar a frenar es corta
    static const int    ARRANQUE_MS = 10000;
    static const float  ESCALON_S   = 5;      // entre el lanzamiento de un servicio y el siguiente
    static const float  SALUDO_LOG_S = 5;     // el aviso "saludo recibido" como mucho cada 5 s
    static const float  RECARGA_S   = 10;     // cada cuanto se releen los archivos (en caliente)
    // a cada jugador que entra, los datos de los comerciantes enganchados: a los 2 s y a los 8 s de verlo
    static const float  JUGADOR_DATOS_1_S = 2;
    static const float  JUGADOR_DATOS_2_S = 8;

    private static ref BZTRManager s_Instance;
    private static string s_PatronUsado;      // el patron de listado que anduvo: se loguea cuando cambia

    private ref array<ref BZTRArchivo> m_Archivos;
    private bool  m_Arrancado;
    private bool  m_TickActivo;
    private float m_UltSaludoLogT;
    private float m_UltRecargaT;
    private int   m_SigIdx;                   // indice unico de cada runtime (el uid de su marcador)

    static BZTRManager Get() {
        if (!s_Instance) s_Instance = new BZTRManager();
        return s_Instance;
    }

    // Desde MissionServer.OnInit.
    void Arrancar() {
        if (m_Arrancado) return;
        m_Arrancado = true;
        m_Archivos = new array<ref BZTRArchivo>();
        BZTRLog.Info("manager", "BZ_Trader: los servicios arrancan en " + BZTRLog.Seg(ARRANQUE_MS / 1000));
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.Iniciar, ARRANQUE_MS, false);
    }

    // ⚠ LA UNICA FUNCION QUE LISTA LOS ARCHIVOS. En DayZ 1.30, FindFile("$profile:...") resuelve "$profile:"
    // a la RAIZ del server, mientras OpenFile y FileExist siguen yendo a profiles\ (medido en AutoDrive).
    // ★ COPIA de BZCarpetas.ListarPerfil de AutoDrive 1.2.1 (3_Game\BZBusCommon.c), NO una llamada: BZCarpetas no existe
    // en la 1.2 que corre hoy en A, y llamarla no compilaria ahi. Cuando la 1.2.1 este en todos lados, se llama a ella.
    // Se prueba en orden:
    //   1. "$profile:BZ_Trader\*.json"   (en la 1.29 anda este)
    //   2. "$profile:BZ_Trader/*.json"
    //   3. el -profiles RELATIVO de la linea de comandos + "/BZ_Trader/*.json" (desde la raiz y con "/": la barra
    //      invertida en la subruta tambien falla en la 1.30)
    // Con -profiles ABSOLUTO la 1.30 no puede listar: 'absoluta' vuelve true (el llamador da el [ERROR]). Los nombres se
    // filtran por el prefijo y el sufijo del patron, asi un listado de la raiz no se cuela. El patron que anduvo se
    // loguea (una vez por boot: esto corre una vez).
    static void ListarServicios(array<string> salida, out bool absoluta) {
        string p = "";
        string pb;
        bool hay;
        absoluta = false;
        if (Recoger(DIR + "\\" + PATRON, salida)) return;
        if (Recoger(DIR + "/" + PATRON, salida)) return;
        hay = GetCLIParam("profiles", p);
        if (!hay || p == "") return;
        if (p.Contains(":") || p.IndexOf("\\") == 0 || p.IndexOf("/") == 0) {
            absoluta = true;
            return;
        }
        p.Replace("\\", "/");
        pb = p + "/" + SUB + "/" + PATRON;
        Recoger(pb, salida);
    }

    // Un patron: junta los nombres que cumplen el prefijo y el sufijo de PATRON; true si encontro alguno.
    private static bool Recoger(string pat, array<string> salida) {
        string nombre;
        string pref = PATRON;
        string suf = "";
        FileAttr attr;
        FindFileHandle h;
        int k = PATRON.IndexOf("*");
        int antes = salida.Count();
        int guarda = 0;
        int cuantos;
        if (k >= 0) {
            pref = PATRON.Substring(0, k);
            suf = PATRON.Substring(k + 1, PATRON.Length() - k - 1);
        }
        h = FindFile(pat, nombre, attr, FindFileFlags.ALL);
        if (!h) return false;
        while (guarda < 500) {
            guarda = guarda + 1;
            if (Cumple(nombre, pref, suf)) salida.Insert(nombre);
            if (!FindNextFile(h, nombre, attr)) break;
        }
        CloseFindFile(h);
        cuantos = salida.Count() - antes;
        if (cuantos <= 0) return false;
        salida.Sort();
        if (s_PatronUsado != pat) {
            s_PatronUsado = pat;
            BZTRLog.Info("manager", "servicios listados con '" + pat + "' (" + cuantos.ToString() + " archivo(s))");
        }
        return true;
    }

    // Como en AutoDrive, pero el sufijo sin distinguir mayusculas (en Windows FindFile tampoco las distingue: un
    // TRADER.JSON que la 1.29 listaba no se pierde).
    private static bool Cumple(string nombre, string pref, string suf) {
        string n = nombre;
        string s = suf;
        int ls;
        int ln;
        if (nombre == "") return false;
        if (pref != "" && nombre.IndexOf(pref) != 0) return false;
        if (suf == "") return true;
        n.ToLower();
        s.ToLower();
        ls = s.Length();
        ln = n.Length();
        if (ln < ls) return false;
        return n.Substring(ln - ls, ls) == s;
    }

    // Un archivo de servicio; null si no se puede usar (y el log dice por que).
    private BZTRServicio Cargar(string archivo) {
        string path = DIR + "/" + archivo;
        string texto = BZTRJson.LeerTexto(path);
        string err;
        string def;
        string rep;
        string base = archivo;
        array<string> av = new array<string>();
        int raiz;
        int i;
        BZTRServicio cfg;
        base.Replace(".json", "");
        raiz = texto.IndexOf("{");
        if (BZTRJson.Tiene(texto, raiz, "Comerciantes")) {
            BZTRLog.Warn(base, "formato P1 (Comerciantes, claves en espanol): se retiro, se ignora. Exportar el servicio con el editor");
            return null;
        }
        cfg = new BZTRServicio();
        if (!JsonFileLoader<BZTRServicio>.LoadFile(path, cfg, err)) {
            BZTRLog.Err(base, "no se pudo leer " + path + ": " + err);
            return null;
        }
        def = cfg.Completar(texto, base);
        if (def != "") BZTRLog.Info(cfg.Name, "por defecto (no escritos en " + archivo + "):" + def);
        rep = cfg.Asegurar();
        if (rep != "") BZTRLog.Err(cfg.Name, "la carga de " + archivo + " quedo A MEDIAS (una VME corto un Completar: ver el RPT justo arriba) · reparado con defaults:" + rep);
        cfg.Avisos(av);
        for (i = 0; i < av.Count(); i++) BZTRLog.Warn(cfg.Name, av.Get(i));
        return cfg;
    }

    private void EscribirEjemplo() {
        BZTRServicio ej = BZTRServicio.Ejemplo();
        string path = DIR + "/" + EJEMPLO;
        string err;
        if (!JsonFileLoader<BZTRServicio>.SaveFile(path, ej, err)) {
            BZTRLog.Err("manager", "no se pudo escribir el ejemplo en " + path + ": " + err);
            return;
        }
        BZTRLog.Info("manager", "no habia servicios: escrito " + path + " (APAGADO, Enable 0), con el formato del editor");
    }

    // Los numeros de GetCurrentCommandID() los pone el motor ("overridden from C++", dayzplayer.c): se loguean una vez
    // para leer el 'cmd N' de las lineas de la plaza.
    private void IdsDeComando() {
        int idVeh = DayZPlayerConstants.COMMANDID_VEHICLE;
        int idMove = DayZPlayerConstants.COMMANDID_MOVE;
        int idScript = DayZPlayerConstants.COMMANDID_SCRIPT;
        int idAccion = DayZPlayerConstants.COMMANDID_ACTION;
        string linea = "ids de comando del motor: vehiculo " + idVeh.ToString() + " · move " + idMove.ToString();
        linea = linea + " · script (eAI camina con uno) " + idScript.ToString() + " · action " + idAccion.ToString();
        BZTRLog.Info("manager", linea);
    }

    // A los ARRANQUE_MS del OnInit: un runtime por servicio habilitado, lanzamientos escalonados.
    void Iniciar() {
        float now = GetGame().GetTickTime();
        array<string> archivos = new array<string>();
        BZTRArchivo ar;
        string lista = "";
        bool absoluta;
        int i;
        int activos = 0;
        // en una instalacion nueva la carpeta NO existe (la leccion del 1002 de AutoDrive)
        if (!FileExist(DIR)) MakeDirectory(DIR);
        IdsDeComando();
        ListarServicios(archivos, absoluta);
        if (archivos.Count() == 0 && absoluta) {
            // el mismo aviso que AutoDrive 1.2.1 (BZCarpetas.ListarPerfil), como ERROR de BZ_Trader; sin ejemplo: no se veria
            BZTRLog.Err("manager", "DayZ 1.30 cannot list files under an absolute -profiles path: use a relative -profiles (e.g. -profiles=profiles) or services will not be listed · BZ_Trader: 0 servicios");
            return;
        }
        if (archivos.Count() == 0) {
            BZTRLog.Warn("manager", "ningun *.json en " + DIR + " con ningun patron de listado ($profile:\\, $profile:/, -profiles relativo)");
            EscribirEjemplo();
        } else {
            for (i = 0; i < archivos.Count(); i++) lista = lista + " " + archivos.Get(i);
            BZTRLog.Info("manager", archivos.Count().ToString() + " archivo(s) en " + DIR + ":" + lista);
        }
        for (i = 0; i < archivos.Count(); i++) {
            ar = new BZTRArchivo(archivos.Get(i));
            ar.m_Visto = true;
            m_Archivos.Insert(ar);
            if (Alta(ar, now, activos * ESCALON_S, "arranque")) activos = activos + 1;
        }
        BZTRLog.Info("manager", activos.ToString() + " servicio(s) habilitado(s) · tick " + TICK_MS.ToString() + " ms · los archivos se releen cada " + BZTRLog.Seg(RECARGA_S) + " (alta, cambio, Enable 0 o borrado, sin reiniciar)");
        m_UltRecargaT = now;
        m_TickActivo = true;
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.Tick, TICK_MS, true);
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.TickRapido, TICK_RAPIDO_MS, true);
    }

    // Lee el archivo y, si esta habilitado, le arma su runtime y lo programa. true si quedo un runtime.
    private bool Alta(BZTRArchivo ar, float now, float escalon, string como) {
        BZTRServicio cfg;
        string texto = BZTRJson.LeerTexto(DIR + "/" + ar.m_Nombre);
        ar.m_Texto = texto;
        cfg = Cargar(ar.m_Nombre);
        if (!cfg) {
            ar.m_TextoMalo = texto;
            return false;
        }
        if (cfg.Enable != 1) {
            BZTRLog.Info(cfg.Name, "APAGADO (Enable " + cfg.Enable.ToString() + ")");
            return false;
        }
        LanzarNuevo(ar, cfg, now, cfg.StartDelayS + escalon, como);
        return true;
    }

    private void LanzarNuevo(BZTRArchivo ar, BZTRServicio cfg, float now, float enS, string como) {
        string porQue = cfg.Invalido();
        if (porQue != "") BZTRLog.Err(cfg.Name, "config invalida: " + porQue + " (se reintenta cada minuto; a los 5 fallos seguidos, FIN)");
        BZTRLog.Info(cfg.Name, "habilitado · " + cfg.Resumen());
        ar.m_Rt = new BZTRRuntime(cfg, m_SigIdx, ar.m_Nombre);
        m_SigIdx = m_SigIdx + 1;
        ar.m_RetiroPedido = false;
        ar.m_Rt.Programar(now, enS, como);
    }

    // Los saludos vigentes, cada TICK_RAPIDO_MS: a 47 km/h el vehiculo hace 13 m por segundo, y la ventana para empezar a
    // frenar (y quedar unos 10 m antes del jugador) es de 10 m. Sin saludos vigentes no hace nada.
    // ------------------------------------------------------------------------
    //  EL MARCADOR DE MAPA DE TODOS LOS SERVICIOS: los settings del mapa a cada jugador, UNA vez por tick, si algun
    //  servicio cambio su marcador (posicion o visibilidad). Cada envio lleva todos los marcadores de servidor del mapa.
    // ------------------------------------------------------------------------
    static const int MARCA_BYTES_MAPA     = 40;    // los campos del mapa en cada envio (estimado)
    static const int MARCA_BYTES_MARCADOR = 100;   // cada marcador de servidor en cada envio (estimado)
    private bool m_MarcaAvisoMapa;
    // los jugadores vistos (por su id): desde cuando y cuantos envios de datos de comerciantes se les hicieron
    private ref map<string, float> m_JugT0 = new map<string, float>();
    private ref map<string, int> m_JugEnvios = new map<string, int>();
    private bool m_MarcaKDicho;

    static int MarcadoresEnElMapa() {
        ExpansionMapSettings mapa = GetExpansionSettings().GetMap(false);
        if (!mapa || !mapa.ServerMarkers) return -1;
        return mapa.ServerMarkers.Count();
    }

    static int BytesPorEnvio(int k) {
        return MARCA_BYTES_MAPA + MARCA_BYTES_MARCADOR * k;
    }

    // al arrancar, la primera vez que hay settings del mapa: cuantos marcadores de servidor lleva cada envio
    private void DecirMarcadoresDelMapa() {
        int k;
        if (m_MarcaKDicho) return;
        k = MarcadoresEnElMapa();
        if (k < 0) return;
        m_MarcaKDicho = true;
        BZTRLog.Info("manager", "marcadores: " + k.ToString() + " marcadores de servidor en el mapa (MapSettings) · cada envio ~" + BytesPorEnvio(k).ToString() + " bytes por jugador");
    }

    private void EnviarMapaSiHaceFalta() {
        array<Man> jugadores;
        ExpansionMapSettings mapa;
        PlayerIdentity ident;
        int i;
        if (!BZTRRuntime.s_MarcaSucia) return;
        BZTRRuntime.s_MarcaSucia = false;
        mapa = GetExpansionSettings().GetMap(false);
        if (!mapa || !mapa.IsLoaded()) {
            if (!m_MarcaAvisoMapa) BZTRLog.Err("manager", "marcadores: los settings del mapa de Expansion no estan cargados: no se puede mandar la posicion");
            m_MarcaAvisoMapa = true;
            return;
        }
        jugadores = new array<Man>();
        GetGame().GetPlayers(jugadores);
        for (i = 0; i < jugadores.Count(); i++) {
            if (!jugadores.Get(i)) continue;
            ident = jugadores.Get(i).GetIdentity();
            if (ident) mapa.Send(ident);
        }
    }

    void TickRapido() {
        float now = GetGame().GetTickTime();
        int i;
        BZTRArchivo ar;
        if (!m_Archivos) return;
        for (i = 0; i < m_Archivos.Count(); i++) {
            ar = m_Archivos.Get(i);
            if (!ar.m_Rt) continue;
            ar.m_Rt.TickSenasRapido(now);
            ar.m_Rt.TickLanzamientoRapido(now);
        }
    }

    // ★ El cliente pide los datos de un comerciante UNA vez, a los 250 ms de verlo, y no vuelve a pedir (Expansion,
    // RequestTraderObject). El que entra al server mientras un comerciante se engancha puede pedirlos antes del enganche
    // (Expansion le contesta NULL) y quedar fuera del envio del enganche (todavia no figura como jugador): sin
    // «Comerciar». A cada jugador nuevo se le mandan los de todos los comerciantes enganchados, a los JUGADOR_DATOS_1_S y
    // a los JUGADOR_DATOS_2_S de verlo (al que todavia no tiene al comerciante cerca no le llega, y ese los pide al verlo).
    private void DatosAJugadoresNuevos(float now) {
        array<Man> jugadores = new array<Man>();
        map<string, bool> estan = new map<string, bool>();
        PlayerIdentity ident;
        string uid;
        int i;
        int envios;
        float hace;
        GetGame().GetPlayers(jugadores);
        for (i = 0; i < jugadores.Count(); i++) {
            if (!jugadores.Get(i)) continue;
            ident = jugadores.Get(i).GetIdentity();
            if (!ident) continue;
            uid = ident.GetId();
            estan.Set(uid, true);
            if (!m_JugT0.Contains(uid)) {
                m_JugT0.Set(uid, now);
                m_JugEnvios.Set(uid, 0);
            }
            envios = m_JugEnvios.Get(uid);
            hace = now - m_JugT0.Get(uid);
            if (envios == 0 && hace >= JUGADOR_DATOS_1_S) {
                m_JugEnvios.Set(uid, 1);
                DatosAlJugador(ident, hace);
            } else if (envios == 1 && hace >= JUGADOR_DATOS_2_S) {
                m_JugEnvios.Set(uid, 2);
                DatosAlJugador(ident, hace);
            }
        }
        // los que se fueron: si vuelven, cuentan como nuevos
        for (i = m_JugT0.Count() - 1; i >= 0; i--) {
            uid = m_JugT0.GetKey(i);
            if (estan.Contains(uid)) continue;
            m_JugT0.Remove(uid);
            m_JugEnvios.Remove(uid);
        }
    }

    // los datos de los comerciantes enganchados de todos los servicios, a un jugador; una linea si hubo alguno
    private void DatosAlJugador(PlayerIdentity ident, float hace) {
        int i;
        int n = 0;
        int bytes = 0;
        int nS;
        int bS;
        int nAir;
        string linea;
        BZTRArchivo ar;
        for (i = 0; i < m_Archivos.Count(); i++) {
            ar = m_Archivos.Get(i);
            if (!ar.m_Rt) continue;
            nS = ar.m_Rt.DatosA(ident, bS);
            n = n + nS;
            bytes = bytes + bS;
        }
        // y el humo de airdrop de las paradas que estan con el encendido (el cliente ignora el que ya tiene)
        nAir = BZTRAirdrop.EnviarA(ident);
        if (n == 0 && nAir == 0) return;
        linea = "jugador " + ident.GetName() + " (entro hace " + BZTRLog.Seg(hace) + "): datos de " + n.ToString() + " comerciante(s) enviados";
        linea = linea + " (~" + bytes.ToString() + " bytes)";
        if (nAir > 0) linea = linea + " · " + nAir.ToString() + " humo(s) de airdrop";
        BZTRLog.Info("manager", linea);
    }

    void Tick() {
        float now = GetGame().GetTickTime();
        int i;
        BZTRArchivo ar;
        if (now - m_UltRecargaT >= RECARGA_S) {
            m_UltRecargaT = now;
            Recargar(now);
        }
        for (i = 0; i < m_Archivos.Count(); i++) {
            ar = m_Archivos.Get(i);
            if (ar.m_Rt) ar.m_Rt.Tick(now);
            AplicarPendiente(ar, now);
        }
        BZTRDesalojo.Tick();   // los vehiculos retirados con jugadores a bordo: se desregistran recien vacios
        DecirMarcadoresDelMapa();
        EnviarMapaSiHaceFalta();   // despues de los ticks de todos los servicios: un envio por tick
        DatosAJugadoresNuevos(now);
    }

    private BZTRArchivo ArchivoDe(string nombre) {
        int i;
        for (i = 0; i < m_Archivos.Count(); i++) {
            if (m_Archivos.Get(i).m_Nombre == nombre) return m_Archivos.Get(i);
        }
        return null;
    }

    // ★ La recarga en caliente: archivos nuevos, cambiados y borrados.
    private void Recargar(float now) {
        array<string> archivos = new array<string>();
        bool absoluta;
        int i;
        BZTRArchivo ar;
        string texto;
        string motivo;
        for (i = 0; i < m_Archivos.Count(); i++) m_Archivos.Get(i).m_Visto = false;
        ListarServicios(archivos, absoluta);
        for (i = 0; i < archivos.Count(); i++) {
            ar = ArchivoDe(archivos.Get(i));
            if (!ar) {
                ar = new BZTRArchivo(archivos.Get(i));
                ar.m_Visto = true;
                m_Archivos.Insert(ar);
                BZTRLog.Info("manager", "servicio " + ar.m_Nombre + ": archivo NUEVO · se lee y, si esta habilitado, se lanza");
                Alta(ar, now, 0, "alta en caliente");
                continue;
            }
            ar.m_Visto = true;
            texto = BZTRJson.LeerTexto(DIR + "/" + ar.m_Nombre);
            if (texto == "" || texto == ar.m_Texto || texto == ar.m_TextoMalo || texto == ar.m_TextoPendiente) continue;
            Cambio(ar, texto, now);
        }
        // los que no aparecieron: borrados (un listado que falla no retira a nadie: se confirma con FileExist)
        for (i = m_Archivos.Count() - 1; i >= 0; i--) {
            ar = m_Archivos.Get(i);
            if (ar.m_Visto || FileExist(DIR + "/" + ar.m_Nombre)) continue;
            if (!ar.m_Rt || !ar.m_Rt.EnServicio()) {
                BZTRLog.Info("manager", "servicio " + ar.m_Nombre + ": archivo BORRADO (su servicio no corria) · se olvida");
                m_Archivos.RemoveOrdered(i);
                continue;
            }
            if (ar.m_RetiroPedido) continue;
            ar.m_RetiroPedido = true;
            ar.m_Pendiente = null;
            ar.m_TextoPendiente = "";
            motivo = ar.m_Rt.PedirRetiro("archivo borrado");
            BZTRLog.Info("manager", "servicio " + ar.m_Nombre + ": archivo BORRADO · " + motivo);
        }
    }

    private void Cambio(BZTRArchivo ar, string texto, float now) {
        BZTRServicio cfg;
        string porQue = "";
        string cuando;
        cfg = Cargar(ar.m_Nombre);   // los defaults y los errores de lectura los loguea Cargar
        if (!cfg) {
            ar.m_TextoMalo = texto;
            BZTRLog.Err("manager", "servicio " + ar.m_Nombre + ": el archivo cambio pero no se pudo leer (ver arriba) · sigue la version anterior");
            return;
        }
        if (cfg.Enable == 1) porQue = cfg.Invalido();
        if (porQue != "") {
            ar.m_TextoMalo = texto;
            BZTRLog.Err("manager", "servicio " + ar.m_Nombre + ": el archivo cambio pero no se puede usar (" + porQue + ") · sigue la version anterior");
            return;
        }
        ar.m_TextoMalo = "";
        if (!ar.m_Rt || !ar.m_Rt.EnServicio()) {
            ar.m_Texto = texto;
            ar.m_Pendiente = null;
            ar.m_TextoPendiente = "";
            if (cfg.Enable != 1) {
                BZTRLog.Info("manager", "servicio " + ar.m_Nombre + ": cambio detectado · sigue APAGADO (Enable " + cfg.Enable.ToString() + ")");
                return;
            }
            if (ar.m_Rt) ar.m_Rt.Reemplazar();
            BZTRLog.Info("manager", "servicio " + ar.m_Nombre + ": cambio detectado · no estaba corriendo: se lanza");
            LanzarNuevo(ar, cfg, now, cfg.StartDelayS, "cambio en caliente");
            return;
        }
        if (cfg.Enable != 1) {
            ar.m_Texto = texto;
            ar.m_Pendiente = null;
            ar.m_TextoPendiente = "";
            if (ar.m_RetiroPedido) return;
            ar.m_RetiroPedido = true;
            BZTRLog.Info("manager", "servicio " + ar.m_Nombre + ": Enable " + cfg.Enable.ToString() + " · " + ar.m_Rt.PedirRetiro("Enable 0 en su archivo"));
            return;
        }
        ar.m_Pendiente = cfg;
        ar.m_TextoPendiente = texto;
        cuando = "en el proximo lanzamiento (ahora " + BZTREstado.Nombre(ar.m_Rt.Estado()) + ")";
        if (ar.m_Rt.ListoParaReemplazo()) cuando = "ya (estaba " + BZTREstado.Nombre(ar.m_Rt.Estado()) + ")";
        BZTRLog.Info("manager", "servicio " + ar.m_Nombre + ": cambio detectado · se aplica " + cuando);
    }

    // La version pendiente toma el lugar de la vieja cuando esta esta entre lanzamientos (o terminada).
    private void AplicarPendiente(BZTRArchivo ar, float now) {
        BZTRServicio cfg;
        float espera;
        if (!ar.m_Pendiente) return;
        if (ar.m_Rt && !ar.m_Rt.ListoParaReemplazo()) return;
        cfg = ar.m_Pendiente;
        espera = -1;
        if (ar.m_Rt) {
            espera = ar.m_Rt.EsperaRestante(now);
            ar.m_Rt.Reemplazar();
        }
        if (espera < 0) espera = cfg.StartDelayS;
        ar.m_Texto = ar.m_TextoPendiente;
        ar.m_Pendiente = null;
        ar.m_TextoPendiente = "";
        BZTRLog.Info("manager", "servicio " + ar.m_Nombre + ": version nueva aplicada · lanzamiento en " + BZTRLog.Seg(espera));
        LanzarNuevo(ar, cfg, now, espera, "version nueva del archivo");
    }

    // Desde EmoteManager.PlayEmote (server): un jugador hizo el gesto de saludo. Cada servicio decide.
    void Saludo(PlayerBase pb) {
        float now;
        int i;
        BZTRArchivo ar;
        string nombre = "?";
        if (!pb || !m_Archivos) return;
        // ★ solo JUGADORES. eAI hace gestos con el mismo EmoteManager.PlayEmote: con eAI de Expansion en el mapa, una
        // "sena de ?" se anotaba a kilometros del vehiculo.
        if (!pb.GetIdentity() || eAIBase.Cast(pb)) return;
        now = GetGame().GetTickTime();
        if (now - m_UltSaludoLogT >= SALUDO_LOG_S) {
            m_UltSaludoLogT = now;
            if (pb.GetIdentity()) nombre = pb.GetIdentity().GetName();
            BZTRLog.Info("manager", "saludo recibido en el server: " + nombre + " en " + BZTRLog.XZ(pb.GetPosition()));
        }
        for (i = 0; i < m_Archivos.Count(); i++) {
            ar = m_Archivos.Get(i);
            if (ar.m_Rt) ar.m_Rt.Saludo(pb, now);
        }
    }

    // Desde MissionServer.OnMissionFinish: el cierre LIMPIO retira los vehiculos (persisten).
    void Apagar() {
        int i;
        BZTRArchivo ar;
        if (m_TickActivo) GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.Tick);
        if (m_TickActivo) GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.TickRapido);
        m_TickActivo = false;
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.Iniciar);
        if (!m_Archivos) return;
        for (i = 0; i < m_Archivos.Count(); i++) {
            ar = m_Archivos.Get(i);
            if (ar.m_Rt) ar.m_Rt.Apagar();
        }
        BZTRDesalojo.Apagar();   // los que tienen jugadores a bordo no se retiran: se dice cuales
    }
}
