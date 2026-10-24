// ============================================================================
//  UN SERVICIO EN MARCHA: su runner de AutoDrive + sus comerciantes.
//
//  ★ CAMINO B DE LA API (un runner propio por servicio): new BZBusService ->
//  GetRunners().Insert -> SetHoldAtStops(true) -> SetEtiqueta -> RespawnFromPath. AutoDrive
//  crea el vehiculo, el chofer y la Crew de la ruta, los sube y no arranca hasta que las
//  plazas estan completas (gate de muster). BZ_Trader engancha el Market a cada plaza que
//  el archivo declara como comerciante (Transport.CrewMember(seat)).
//
//  ★ LA PARADA: marca 4 de la toma (HoldRunner unos wp antes), sena de un jugador
//  (HoldRunner en el acto) o un HOLD propio de AutoDrive (fin de leg / isStop). Congelado el
//  vehiculo, bajan los que dice el archivo, caminan a su puesto, venden y vuelven; SIGUE
//  cuando las plazas ocupadas al congelar vuelven a estar ocupadas (regla de BZ_Trader, hasta
//  que AutoDrive la tenga). Si nadie baja, espera sentado.
//
//  ★ REFERENCIAS: el runner lo retiene el registro de AutoDrive; aca una sola referencia,
//  'ref' (consultarlo siempre es seguro). A las entidades, nunca 'ref'.
//  ★ TODA ESPERA TIENE SALIDA: cada fase tiene tope, y la parada entera tambien.
// ============================================================================
class BZTRTipoParada {
    static const int MARCA = 1;   // marca 4 de la toma
    static const int SENA  = 2;   // saludo de un jugador
    static const int AJENA = 3;   // HOLD propio de AutoDrive (fin de leg / isStop) con el chofer comerciante

    static string Nombre(int t) {
        if (t == MARCA) return "marca";
        if (t == SENA)  return "sena";
        if (t == AJENA) return "HOLD de AutoDrive";
        return "?";
    }
}

// Un saludo recibido queda vigente: en cada tick se reevaluan las condiciones con la posicion del jugador EN ESE TICK
// (a 25-30 km/h el vehiculo cruza los 15 m del radio en ~2 s; evaluado solo en el instante del gesto, habia
// que saludar justo en ese momento). ★ vale hasta que el vehiculo para, LO PASA o SE ALEJA, con el tope
// Wave.MemoryS (antes vencia a los 10 s: a 40 km/h, desde 150 m, el vehiculo llegaba justo o no llegaba).
class BZTRSenaVigente {
    PlayerBase m_Pb;        // puntero comun: si se desconecta, queda null
    string m_Nombre;
    float  m_T;             // el ultimo saludo (renovarlo reinicia la vigencia)
    float  m_T0;            // el primero
    bool   m_Adelante;      // estuvo adelante del vehiculo en algun tick
    float  m_DMin = -1;
    string m_FaltaEnMin;    // lo que falto en el tick de distancia minima
    string m_Falta;         // lo que falto en el ultimo tick
    string m_FaltaVentana;  // lo que falto con el vehiculo en la ventana de frenado (el ultimo)
    string m_FaltaCerca;    // la primera vez que ya no alcanzaba a frenar antes del jugador

    void BZTRSenaVigente(PlayerBase pb, string nombre, float now) {
        m_Pb = pb;
        m_Nombre = nombre;
        m_T = now;
        m_T0 = now;
    }
}

// ★ las reglas contra el saludo abusivo, como funciones PURAS (solo valores: se pueden probar fuera del juego, con
// los casos: un grupo que saluda, uno que sigue al vehiculo, un saludo antes de una marca, el tope por vuelta, 0 = sin
// limite). "" = la regla deja parar; si no, por que no (va a la linea del saludo ignorado).
class BZTRSaludoReglas {
    // regla 1: despues de CUALQUIER parada, recorrer Wave.MinGapM antes de parar por otro saludo
    static string FaltaTrasParada(bool huboParada, float recorridoM, float gapM) {
        string linea;
        if (gapM <= 0 || !huboParada || recorridoM >= gapM) return "";
        linea = "paro hace " + BZTRLog.Metros(recorridoM) + " (MinGapM " + BZTRLog.Metros(gapM);
        linea = linea + ": faltan " + BZTRLog.Metros(gapM - recorridoM) + ")";
        return linea;
    }

    // regla 1, y la marca cercana de antes (50 m en linea recta) unificadas: la proxima parada marcada a menos de
    // max(Wave.MinGapM, minimoM) por delante, sobre la ruta. Con MinGapM 0 queda el minimo: parar por un saludo justo
    // antes de una marca haria dos paradas seguidas.
    static string FaltaPorMarca(int numero, float dM, float gapM, float minimoM) {
        float tope = gapM;
        string linea;
        if (tope < minimoM) tope = minimoM;
        if (dM >= tope) return "";
        linea = "la proxima parada marcada (marca " + numero.ToString() + ") esta a " + BZTRLog.Metros(dM);
        if (gapM > minimoM) return linea + " (MinGapM " + BZTRLog.Metros(gapM) + ")";
        return linea + " (va a parar ahi)";
    }

    // regla 2: como mucho Wave.MaxPerLap paradas por saludo por vuelta
    static string FaltaPorVuelta(int hechas, int maxPorVuelta) {
        string linea;
        if (maxPorVuelta <= 0 || hechas < maxPorVuelta) return "";
        linea = "ya hizo " + hechas.ToString() + " paradas por saludo en esta vuelta (MaxPerLap " + maxPorVuelta.ToString() + ")";
        return linea;
    }

    // la distancia que necesita para parar desde 'kmh': v^2/(2 a) frenando, mas v * latencia hasta que el freno actua
    static float DistFrenado(float kmh, float aMs2, float latenciaS) {
        float v = kmh / 3.6;
        float frena = v * v / (2 * aMs2);
        float reaccion = v * latenciaS;
        return frena + reaccion;
    }

    // con dJ = lo que falta del frente del vehiculo al jugador: 0 = todavia lejos, 1 = frena ahora (queda entre 0 y antesM
    // antes del jugador), 2 = ya no alcanza a parar antes del jugador
    static int FaseDeFrenado(float dJ, float dFr, float antesM) {
        if (dJ > dFr + antesM) return 0;
        if (dJ < dFr) return 2;
        return 1;
    }

    // la linea al empezar una vuelta, con lo que hizo la anterior
    static string ResumenVuelta(int vuelta, int hechas, int maxPorVuelta) {
        string topeTxt = "sin tope";
        if (maxPorVuelta > 0) topeTxt = "MaxPerLap " + maxPorVuelta.ToString();
        return "vuelta " + vuelta.ToString() + ": " + hechas.ToString() + " parada(s) por saludo (" + topeTxt + ")";
    }
}

class BZTRRuntime {
    static const float REINTENTO_S          = 60;
    static const int   MAX_FALLOS           = 5;
    static const float LANZADO_MAX_S        = 30;
    static const float CABECERA_MAX_S       = 300;
    static const float CABECERA_AVISO_S     = 60;
    // ⛔ el AR borra y repone el vehiculo al reubicarlo: parpadeo
    static const int   SIN_VEH_TICKS        = 4;
    static const int   SIN_CHOFER_TICKS     = 4;
    // ⛔ el runner nunca reporta los ultimos waypoints de la toma
    static const int   MARGEN_FIN_WP        = 5;
    static const int   MARGEN_MARCA_WP      = 5;     // HoldRunner unos wp ANTES de la marca
    static const int   MARCA_PASADA_WP      = 15;    // pasada por mas que esto sin HOLD: no engancho
    static const float TERMINAL_HOLD_MAX_S  = 120;
    static const float REHOLD_S             = 30;
    static const float REHOLD_M             = 15;
    // ★ el vehiculo tiene que estar EN la marca para que la parada despliegue. Tras una recuperacion
    // de AutoDrive el cursor de wp puede quedar desfasado del vehiculo (medido: una parada pedida con el vehiculo a 161 m
    // de su marca; los comerciantes caminaron 155 m y uno subio por teleport; las otras 16 paradas, a menos de 3 m)
    static const float PARADA_LEJOS_M       = 15;
    static const float SALIDA_CHEQUEO_S     = 10;
    static const float SALIDA_MIN_M         = 3;
    static const float FRENTE_LARGO_M       = 12;    // al seguir: el camino de adelante que tiene que estar libre de jugadores
    static const float FRENTE_MARGEN_M      = 1;     // ... con el ancho del casco y esto a cada lado
    static const float SALIDA_ARMADO_MAX_S  = 60;    // soltado y AutoDrive sin armar el controlador: se avisa
    static const float QUIETO_KMH           = 3;     // DayZ no deja bajar con el vehiculo en movimiento
    static const float QUIETO_MAX_S         = 8;
    static const float PARADA_TOPE_S        = 600;   // la parada entera
    static const float PLAZAS_GRACIA_S      = 10;    // todos arriba y alguna plaza vacia: se sigue igual
    static const float SENA_PEDIDA_MAX_S    = 30;
    static const float SENA_MARCA_M         = 50;    // sin sena con una marca activa tan cerca adelante
    static const float SENA_ANDANDO_KMH     = 3;
    // al lanzar: los comerciantes de la tripulacion se buscan por CLASE cerca del vehiculo, se enganchan y se sientan
    // en SU plaza por teleport (el embarque nativo de eAI no los sube, o los sube cambiados de plaza)
    static const float CLASE_VENTANA_S      = 15;
    static const float CLASE_RADIO_M        = 10;
    static const float CLASE_AVISO_S        = 10;
    static const float CLASE_REINTENTO_S    = 3;
    static const float CLASE_EDAD_S         = 0.8;   // se lo sienta en el tick SIGUIENTE al que se lo vio
    static const float CLASE_EDAD_MAX_S     = 4;     // ... y si a esa edad todavia no camina, igual (con aviso)
    // en la cabecera, el que maneja lo sienta AutoDrive (animado, con su puerta abierta; si no termina, por teleport):
    // no se sale ni se cierran las puertas mientras sube. Como mucho esto desde el lanzamiento; despues sale igual
    static const float CHOFER_ESPERA_S      = 12;
    // en cada parada, los datos de los comerciantes otra vez a los jugadores a esta distancia (Expansion los pide una
    // sola vez: si se perdieron, la parada los repone)
    static const float PARADA_DATOS_M       = 500;
    // despues de quitar la PAUSA del panel de AutoDrive, una parada no sigue antes de esto (el jugador que esperaba no ve
    // cerrarse las puertas en el acto)
    static const float PAUSA_MIN_S          = 10;
    // la bocina al salir (Stops[].HornOnDepart, Wave.HornOnDepart): el toque, y se suelta esto despues
    static const float BOCINA_ANTES_S       = 2;
    static const float PUERTAS_SOSTENER_S   = 5;     // al seguir, las puertas de plaza se sostienen cerradas
    // no sale hasta que los comerciantes vivos esten sentados DE VERDAD; al tope, se relanza la vuelta
    static const float EMBARQUE_LANZAR_TOPE_S = 60;  // al lanzar, retenido en la cabecera como mucho esto
    static const float EMBARQUE_PARADA_FACTOR = 3;   // en una parada, el tope es 3 x BoardTimeoutS de la parada
    static const float EMBARQUE_MIN_S       = 30;    // ... y nunca menos que esto
    static const float REEMBARQUE_S         = 3;     // entre dos intentos de volver a sentar al mismo
    static const float EMBARQUE_LOG_S       = 5;
    static const int   MAX_EMBARQUES_FALLIDOS = 5;   // vueltas relanzadas seguidas sin poder sentarlos: FIN con error
    static const float SENA_ANOTAR_FACTOR   = 10;    // una sena a mas de 10 x Wave.RadiusM ni se anota
    static const float SENA_PASO_FACTOR     = 3;     // lo paso = estuvo adelante, a menos de 3 x RadiusM, y quedo atras
    // la parada por saludo empieza con la distancia de frenado, para que el FRENTE quede unos SENA_ANTES_M antes del
    // jugador. AutoDrive no publica cuanto frena el HOLD (freno y freno de mano). Medido en un bus, desde el pedido hasta
    // quieto: 8,1 m desde 40,5 km/h y 14,2 m desde 47,2 km/h (este derrapando: 6,1 m/s2). SENA_FRENADA_MS2 es esa peor
    // medida redondeada hacia abajo y SENA_LATENCIA_S lo que tarda en actuar: el modelo da 14,3 y 18,9 m, del lado seguro.
    // El chequeo cada 250 ms lo cubre la ventana (SENA_ANTES_M).
    static const float SENA_FRENADA_MS2     = 5.5;
    static const float SENA_LATENCIA_S      = 0.25;
    static const float SENA_ANTES_M         = 10;
    static const float SENA_RUTA_LATERAL_M  = 15;    // mas lejos que esto de la ruta, la distancia al jugador va en linea recta
    static const float ODO_SALTO_M          = 60;    // mas que esto en un tick (una reubicacion de AutoDrive) no es recorrido
    static const float SOLTAR_MAX_S         = 20;
    static const float RELANZAR_TRAS_RETIRO_S = 3;
    // en la 1.30 el retiro de AutoDrive es asincrono (BZRetiro: baja a los NPC, a los 5 s los borra, el vehiculo 0,5 s
    // despues): mirar el botin antes de los ~5,5 s diria "sigue existiendo" de un vehiculo que se esta retirando
    static const float VERIFICAR_BOTIN_S    = 7;

    private ref BZTRServicio m_Cfg;
    private int    m_Idx;
    private string m_Archivo;
    private string m_MarcaUid;
    private string m_Firma;
    private ref array<ref BZTRBot> m_Bots;

    // la toma
    private ref array<ref BZTRMarca> m_Marcas;
    private ref array<vector> m_WpPos;
    private ref array<bool> m_MarcaArmada;   // wave_only: alguien saludo cerca de esa marca
    private int    m_ProxMarca;
    private int    m_MarcaPedida;
    private bool   m_TomaLeida;

    private ref BZBusService m_Runner;
    private ref BZTRCasco m_Casco;        // la caja y las puertas del vehiculo de esta vuelta
    private int    m_BusVueltaId;         // el id del vehiculo de esta vuelta (0 = todavia no visto): ver RespawnDelPanel
    private int    m_Estado = BZTREstado.ESPERANDO;
    private float  m_Hasta;
    private float  m_T0;
    private int    m_Lanzamientos;
    private int    m_FallosSeguidos;
    private float  m_CabeceraAvisoT;
    // el embarque (sentados de verdad antes de seguir)
    private bool   m_Embarcando;          // al lanzar: retenido en la cabecera
    private float  m_EmbarqueT0;          // desde cuando falta alguien (0 = nadie)
    private float  m_EmbarqueLogT;
    private bool   m_EmbarqueAvisoMov;
    private int    m_EmbarquesFallidos;   // vueltas relanzadas seguidas por no poder sentarlos
    private int    m_EmbVivos;
    private int    m_EmbSentados;
    private int    m_EmbMuertos;
    private int    m_EmbSinBot;

    // la ruta
    private bool   m_WasHeld;
    private bool   m_TerminalPedida;
    private float  m_TerminalPedidaT;
    private int    m_SinVeh;
    private int    m_SinChofer;
    private vector m_UltPos;
    private float  m_UltSueltaT;
    private vector m_UltSueltaPos;
    private bool   m_SalidaPendiente;
    private float  m_SalidaArmadoT;       // desde cuando corre el chequeo de la salida (0 = sin armar todavia)
    private float  m_PuertasHasta;        // hasta cuando se sostienen cerradas las puertas al seguir
    private bool   m_PuertasAvisadas;

    // la parada / la terminal
    private int    m_TipoParada;
    private ref BZTRStop m_Stop;
    private ref BZTRBalizas m_Balizas;    // las senales que encendio la parada en curso
    private bool   m_CerradoAvisado;      // el cierre para jugadores ya se informo en esta vuelta
    private bool   m_CierreImposible;     // SetLockState no lo cerro (sin Expansion-Vehicles): no se reintenta
    private bool   m_SinCierreAvisado;
    private string m_ParadaNombre;
    private float  m_ParadaT0;
    private int    m_ParadaFase;          // 0 esperando quieto · 1 abajo · 2 sentada · 3 tope forzado
    private ref array<int> m_Congeladas;  // plazas ocupadas al congelar
    private int    m_Bajaron;
    private float  m_EsperaT0;
    private float  m_EsperaMin;
    private int    m_EsperaJugIni;
    private int    m_EsperaJugMax;
    private float  m_TodosArribaT;
    private float  m_FrenteT0;            // al seguir: desde cuando espera que un jugador se corra del frente (0 = no espera)
    private string m_FrenteQuien;
    private bool   m_FrenteVencido;       // ... el tope ya se cumplio: sale igual (hasta el Soltar)
    private bool   m_ChoferAviso;         // en la cabecera: ya se dijo que se espera al que maneja
    private bool   m_BocinaTocada;        // la bocina al salir ya sono en esta parada: se suelta BOCINA_ANTES_S despues
    private float  m_BocinaT;             // reloj del servicio del toque
    private bool   m_BocinaVista;         // en este tick la parada llego a la bocina (si no, la decision se cayo)
    private bool   m_BocinaSinAutoDicho;  // el vehiculo no es un auto (una moto): ya se dijo que sale sin bocina
    // ★ el RELOJ DEL SERVICIO: el del juego menos el tiempo en PAUSA (el panel de AutoDrive o NUMPAD .). Todo lo del
    // servicio corre con el; la pausa lo detiene, y al quitarla todo sigue donde estaba. Ver VigilarPausa.
    private float  m_PausaTotal;          // segundos de pausa ya descontados
    private float  m_PausaDesde;          // el reloj del juego al empezar la pausa en curso (0 = no hay)
    private float  m_TrasPausaHasta;      // reloj del servicio: despues de una pausa, la parada no sigue antes de esto
    private float  m_AhoraReal;           // el reloj del juego del ultimo tick (el marcador titila con el, tambien en pausa)

    // la sena
    private bool   m_SenaPedida;
    private float  m_SenaPedidaT;
    private float  m_UltSenaT;
    private string m_SenaDe;
    private PlayerBase m_SenaPb;          // el jugador de la parada por saludo (puntero comun), para medir donde quedo
    private int    m_SenaFase;            // lo que dejo FaltaSena: -1 sin calcular, 0 lejos, 1 en la ventana, 2 ya no alcanza
    private ref array<ref BZTRSenaVigente> m_Senas;
    // las reglas contra el saludo abusivo
    private float  m_OdoM;               // regla 1: lo recorrido desde que salio de la ultima parada
    private vector m_OdoPrev;
    private bool   m_OdoTiene;           // hubo una parada en esta vuelta (si no, la regla 1 no mira lo recorrido)
    private int    m_SenasVuelta;        // regla 2: paradas por saludo en esta vuelta
    private ref array<float> m_WpAcum;   // la distancia acumulada sobre la toma, por wp (regla 1: medir sobre la ruta)

    // la busqueda por clase al lanzar
    private float  m_ClaseT0;
    private float  m_ClaseHasta;
    private bool   m_ClaseResumida;
    // ★ los bots del runner anterior. En la 1.30 el retiro es asincrono (~5,5 s) y en una ruta de ida y
    // vuelta siguen en la cabecera cuando se relanza: la busqueda por clase no los toma. Punteros comunes (no 'ref':
    // se borran solos); la lista se vacia al cerrar la ventana de busqueda siguiente.
    private ref array<eAIBase> m_Retirados;
    private ref array<eAIBase> m_RetiradosAvisados;

    // la muerte (los comerciantes son invencibles: queda como red de seguridad)
    private float  m_MuerteT;
    private bool   m_FinLogueado;
    private bool   m_SoltadoLogueado;
    private EntityAI m_BotinVeh;
    private Man    m_BotinCuerpo;
    private bool   m_VerificarBotin;
    private float  m_VerificarT;

    // el marcador y la zona
    private bool   m_MarcaVisible;      // el marcador existe en los settings del mapa (un solo uid por servicio)
    private float  m_MarcaT;
    private bool   m_MarcaAvisada;
    private vector m_MarcaPos;          // donde se mando por ultima vez (parado no se reenvia)
    private ExpansionMarkerData m_MarcaDato;   // el dato que vive en los settings del mapa del server (sin ref: lo borra Navigation)
    private bool   m_MarcaOculta;       // apagado por visibilidad (el parpadeo de la parada)
    private int    m_MarcaEnvios;       // cambios del marcador en esta vuelta (posicion o visibilidad): el resumen por vuelta
    private int    m_MarcaK = -1;       // marcadores de servidor en el mapa en el ultimo envio (mientras el marcador propio vivia)
    private bool   m_Parpadeando;       // el marcador esta parpadeando (parada)
    private float  m_ParpadeoT;         // el ultimo cambio del parpadeo
    private float  m_ParpadeoT0;        // cuando empezo el parpadeo de esta parada
    private int    m_ParpadeoCambios;
    // la zona del servicio: 'ref' a proposito. ExpansionMarketTraderZone NO deriva de Managed: un puntero comun
    // podria quedar colgado si el Market recargara sus zonas.
    private ref ExpansionMarketTraderZone m_Zona;
    private bool   m_ZonaBuscada;
    // ★ las zonas ya resueltas EN ESTE PROCESO, por nombre (en minusculas): la instancia del Market,
    // o la que BZ_Trader cargo de su archivo, y si es propia. Un runtime nuevo (archivo releido, servicio relanzado) volvia a
    // buscar por la posicion del ARCHIVO, y la instancia del Market ya no estaba ahi (SeguirZona la mueve con el vehiculo,
    // sin guardarla): ganaba 'World' y se cargaba una SEGUNDA copia del archivo (medido: el mismo arranque la encontro por la
    // API y despues la "cargo de su archivo"). 'ref' a proposito, como m_Zona.
    private static ref map<string, ref ExpansionMarketTraderZone> s_ZonasVistas;
    private static ref map<string, bool> s_ZonasPropias;
    // ★ true = la zona NO esta en la lista del Market (archivo nuevo despues del arranque): BZ_Trader la cargo
    // de su archivo y se la asigna a cada comerciante con SetTraderZone
    private bool   m_ZonaPropia;
    // el vigia del stock de la zona: cada cambio de stock de un articulo que ya estaba = un comercio
    private ref map<string, int> m_StockVisto;

    // ★ el manager pide el retiro (Enable 0 o archivo borrado); con comerciantes abajo, al terminar la parada
    private bool   m_RetiroPedido;
    private string m_RetiroMotivo;

    void BZTRRuntime(BZTRServicio cfg, int idx, string archivo) {
        int i;
        BZTRTrader t;
        array<int> plazas = new array<int>();
        m_Cfg = cfg;
        m_Idx = idx;
        m_Archivo = archivo;
        m_MarcaUid = "bztrader_" + idx.ToString() + "_" + cfg.Name;
        m_Firma = cfg.Name;
        m_Bots = new array<ref BZTRBot>();
        for (i = 0; i < cfg.Traders.Count(); i++) {
            t = cfg.Traders.Get(i);
            if (!t || t.Seat < 0) continue;
            if (plazas.Find(t.Seat) >= 0) continue;
            if (t.Seat == 0 && cfg.Driver.IsTrader != 1) continue;
            plazas.Insert(t.Seat);
            m_Bots.Insert(new BZTRBot(t));
        }
        m_Marcas = new array<ref BZTRMarca>();
        m_WpPos = new array<vector>();
        m_WpAcum = new array<float>();
        m_MarcaArmada = new array<bool>();
        m_Congeladas = new array<int>();
        m_Senas = new array<ref BZTRSenaVigente>();
        m_Balizas = new BZTRBalizas();
        m_Retirados = new array<eAIBase>();
        m_RetiradosAvisados = new array<eAIBase>();
        m_MarcaPedida = -1;
    }

    string Firma() {
        if (m_Runner) m_Firma = m_Runner.NombreLog();
        return m_Firma;
    }

    int Estado() { return m_Estado; }

    // ------------------------------------------------------------------------
    //  LA TOMA: las marcas 4, cruzadas con Stops[] (una vez, al iniciar)
    // ------------------------------------------------------------------------
    void LeerLaToma() {
        string origen;
        string lista = "";
        string clase;
        int i;
        int j;
        bool ok;
        float d;
        BZTRMarca m;
        BZTRStop s;
        vector pe;
        string cab;
        float acum;
        float tramo;
        m_Marcas.Clear();
        m_WpPos.Clear();
        BZTRRuta.LeerMarcas(m_Cfg.Route, m_WpPos, m_Marcas, origen);
        // la distancia acumulada sobre la toma (para medir sobre la ruta, no en linea recta)
        m_WpAcum.Clear();
        acum = 0;
        for (i = 0; i < m_WpPos.Count(); i++) {
            if (i > 0) {
                tramo = DistanciaXZ(m_WpPos.Get(i - 1), m_WpPos.Get(i));
                acum = acum + tramo;
            }
            m_WpAcum.Insert(acum);
        }
        for (i = 0; i < m_Marcas.Count(); i++) {
            m = m_Marcas.Get(i);
            lista = lista + " " + m.Wp.ToString();
        }
        BZTRLog.Info(Firma(), "marcas en la toma: " + m_Marcas.Count().ToString() + " (wp" + lista + ") · de " + origen + " · " + m_WpPos.Count().ToString() + " wp");
        m_MarcaArmada.Clear();
        for (i = 0; i < m_Marcas.Count(); i++) {
            m_MarcaArmada.Insert(false);
            m = m_Marcas.Get(i);
            s = m_Cfg.StopDeLaMarca(m.Numero);
            if (!s) {
                BZTRLog.Warn(Firma(), "la marca " + m.Numero.ToString() + " (wp " + m.Wp.ToString() + ") no tiene Stops en el archivo: no para ahi");
                continue;
            }
            pe = s.PosEditor(ok);
            cab = "marca " + m.Numero.ToString() + " (wp " + m.Wp.ToString() + ", " + BZTRLog.XZ(m.Donde) + ")";
            if (!ok) {
                BZTRLog.Info(Firma(), cab + " = Stops[Mark " + s.Mark.ToString() + "] · Mode " + s.Mode + " (el archivo no trae Pos para verificar)");
                continue;
            }
            d = Math.Sqrt(BZTRJugadores.DistXZ2(pe, m.Donde));
            if (d > 15) BZTRLog.Warn(Firma(), cab + " y Stops[Mark " + s.Mark.ToString() + "].Pos en " + BZTRLog.XZ(pe) + ": a " + BZTRLog.Metros(d) + " (se reconvirtio la toma? reabrir en el editor)");
            else BZTRLog.Info(Firma(), cab + " = Stops[Mark " + s.Mark.ToString() + "] a " + BZTRLog.Metros(d) + " · Mode " + s.Mode + " · bajan " + s.WhoGetsOff.Count().ToString());
        }
        for (j = 0; j < m_Cfg.Stops.Count(); j++) {
            s = m_Cfg.Stops.Get(j);
            if (!s) continue;
            if (s.Mark < 1 || s.Mark > m_Marcas.Count()) BZTRLog.Warn(Firma(), "Stops[Mark " + s.Mark.ToString() + "]: la toma no tiene esa marca (tiene " + m_Marcas.Count().ToString() + ")");
        }
        clase = BZTRRuta.ClaseDelVehiculo(m_Cfg.Route);
        if (m_Cfg.VehicleClass != "" && clase != "" && clase != m_Cfg.VehicleClass) BZTRLog.Warn(Firma(), "VehicleClass del archivo " + m_Cfg.VehicleClass + " y de la ruta " + clase + ": manda la ruta");
    }

    // ------------------------------------------------------------------------
    //  PROGRAMAR / LANZAR / RETIRAR
    // ------------------------------------------------------------------------
    // now: el reloj del servicio. El manager la llama solo al dar de alta un servicio nuevo (sin pausas todavia: el
    // reloj del servicio es el del juego).
    void Programar(float now, float enS, string motivo) {
        m_Estado = BZTREstado.ESPERANDO;
        m_Hasta = now + enS;
        BZTRLog.Info(Firma(), "proximo lanzamiento en " + BZTRLog.Seg(enS) + " (" + motivo + ")");
    }

    private void Terminar(string motivo, bool esFalla) {
        m_Estado = BZTREstado.FIN;
        if (esFalla) BZTRLog.Warn(Firma(), "FIN: no se relanza mas · " + motivo);
        else BZTRLog.Info(Firma(), "FIN del servicio · " + motivo);
    }

    private void Lanzar(float now) {
        BZBusService r;
        string porQue;
        int i;
        if (m_Runner) Retirar();
        porQue = m_Cfg.Invalido();
        if (porQue != "") {
            FallaDeLanzamiento(now, porQue);
            return;
        }
        if (!m_TomaLeida) {
            m_TomaLeida = true;
            LeerLaToma();
        }
        m_Lanzamientos = m_Lanzamientos + 1;
        // ★ el tope de paradas por saludo es POR VUELTA = un lanzamiento (de la cabecera a la terminal, o
        // hasta un relanzamiento: OnRouteEnd restart o respawn, una reaparicion, un embarque que no se pudo). El contador
        // vuelve a 0 aca (ReiniciarVuelta), con una linea si la vuelta anterior tuvo paradas por saludo.
        if (m_SenasVuelta > 0) BZTRLog.Info(Firma(), BZTRSaludoReglas.ResumenVuelta(m_Lanzamientos - 1, m_SenasVuelta, m_Cfg.Wave.MaxPerLap));
        if (m_MarcaEnvios > 0) ResumirMarcador();
        r = new BZBusService();
        BZBusService.GetRunners().Insert(r);   // 1. el registro toma la referencia fuerte
        r.SetHoldAtStops(true);                // 2. HOLD en cada parada declarada y en el fin de cada leg
        r.SetEtiqueta(m_Cfg.Name);             // 3. antes de cargar: la carga ya firma con ella
        r.RespawnFromPath(m_Cfg.Route);        // 4. carga + spawn de vehiculo, chofer y Crew en el wp0 + arranca
        m_Runner = r;
        ReiniciarVuelta();
        for (i = 0; i < m_Bots.Count(); i++) m_Bots.Get(i).Reiniciar();
        if (r.RunnerIsActive()) {
            EnRuta(now, "lanzado #" + m_Lanzamientos.ToString());
            return;
        }
        if (r.RunnerEsperandoCabecera()) {
            m_Estado = BZTREstado.LANZADO;
            m_T0 = now;
            m_CabeceraAvisoT = now;
            BZTRLog.Info(Firma(), "lanzado #" + m_Lanzamientos.ToString() + " · " + m_Cfg.Route + " · CABECERA OCUPADA: el runner espera el wp0 libre (tope " + BZTRLog.Seg(CABECERA_MAX_S) + ")");
            return;
        }
        FallaDeLanzamiento(now, "lanzado #" + m_Lanzamientos.ToString() + " y NO SPAWNEO: ni vehiculo ni espera de cabecera (ver [RouteList] / [SpawnGuard] de AutoDrive justo arriba)");
    }

    private void FallaDeLanzamiento(float now, string motivo) {
        m_FallosSeguidos = m_FallosSeguidos + 1;
        BZTRLog.Err(Firma(), motivo);
        Retirar();
        if (m_FallosSeguidos >= MAX_FALLOS) {
            Terminar(MAX_FALLOS.ToString() + " lanzamientos fallidos seguidos: revisar la ruta y el RPT de AutoDrive", true);
            return;
        }
        Programar(now, REINTENTO_S, "reintento " + m_FallosSeguidos.ToString() + " de " + MAX_FALLOS.ToString());
    }

    // UnregisterRunner = StopBus (borra vehiculo, chofer y Crew) + sacarlo del registro. Si AutoDrive ya lo retiro
    // solo (fin de ruta con despawn), no se vuelve a llamar.
    // ⛔ Con un jugador a bordo NO (BZTraderDesalojo.c): el runner queda registrado y frenado, y se desregistra recien
    // vacio. Es el unico lugar donde BZ_Trader desregistra un runner: todos los retiros y relanzamientos pasan por aca.
    private void Retirar() {
        array<ref BZBusService> rs;
        ApagarBalizas("al retirar el vehiculo");
        if (!m_Runner) return;
        Firma();
        AnotarRetirados();
        rs = BZBusService.GetRunners();
        if (rs && rs.Find(m_Runner) >= 0) {
            if (BZTRDesalojo.Pedir(m_Runner, m_Casco, m_Firma, "retiro (estado " + BZTREstado.Nombre(m_Estado) + ")")) {
                m_Runner = null;
                return;
            }
            BZBusService.UnregisterRunner(m_Runner);
            BZTRLog.Info(m_Firma, "UnregisterRunner hecho");
        } else {
            BZTRLog.Info(m_Firma, "el runner ya no estaba en el registro de AutoDrive (se retiro solo al cerrar la ruta): nada que desregistrar");
        }
        m_Runner = null;
    }

    // Antes del UnregisterRunner: el chofer, la tripulacion sentada y los comerciantes enganchados (que pueden estar
    // abajo) quedan anotados para que la busqueda por clase del proximo lanzamiento no los tome.
    private void AnotarRetirados() {
        Transport tr = Transport.Cast(m_Runner.GetBus());
        eAIBase x;
        int i;
        m_Retirados.Clear();
        m_RetiradosAvisados.Clear();
        AnotarRetirado(m_Runner.GetDriver());
        if (tr) {
            for (i = 0; i < tr.CrewSize(); i++) {
                x = eAIBase.Cast(tr.CrewMember(i));
                AnotarRetirado(x);
            }
        }
        for (i = 0; i < m_Bots.Count(); i++) AnotarRetirado(m_Bots.Get(i).m_Bot);
    }

    private void AnotarRetirado(eAIBase x) {
        if (!x || m_Retirados.Find(x) >= 0) return;
        m_Retirados.Insert(x);
    }

    private void ReiniciarVuelta() {
        int i;
        m_BusVueltaId = 0;
        m_Embarcando = false;
        m_EmbarqueT0 = 0;
        m_EmbarqueLogT = 0;
        m_EmbarqueAvisoMov = false;
        m_WasHeld = false;
        m_TerminalPedida = false;
        m_TerminalPedidaT = 0;
        m_SinVeh = 0;
        m_SinChofer = 0;
        m_UltSueltaT = 0;
        m_SalidaPendiente = false;
        m_FinLogueado = false;
        m_SoltadoLogueado = false;
        m_ProxMarca = 0;
        m_MarcaPedida = -1;
        m_SenaPedida = false;
        m_UltSenaT = 0;
        m_Senas.Clear();
        m_OdoM = 0;
        m_OdoTiene = false;
        m_SenasVuelta = 0;
        m_FrenteT0 = 0;
        m_FrenteVencido = false;
        m_ClaseT0 = 0;
        m_ClaseHasta = 0;
        m_ClaseResumida = true;
        m_PuertasHasta = 0;
        m_PuertasAvisadas = false;
        m_CerradoAvisado = false;
        m_SinCierreAvisado = false;
        m_CierreImposible = false;
        for (i = 0; i < m_MarcaArmada.Count(); i++) m_MarcaArmada.Set(i, false);
    }

    private void EnRuta(float now, string como) {
        EntityAI v = m_Runner.GetBus();
        eAIBase d = m_Runner.GetDriver();
        string cv = "?";
        string cd = "NINGUNO";
        string linea;
        m_Estado = BZTREstado.EN_RUTA;
        m_FallosSeguidos = 0;
        m_UltPos = m_Runner.RunnerPos();
        if (v) cv = v.GetType();
        if (d) cd = d.GetType();
        linea = como + " · " + m_Cfg.Route + " · vehiculo " + cv + " en " + BZTRLog.XZ(m_UltPos);
        linea = linea + " · chofer " + cd + " · " + m_Runner.RunnerWpTotal().ToString() + " wp · comerciantes esperados " + m_Bots.Count().ToString();
        BZTRLog.Info(Firma(), linea);
        m_ClaseT0 = now;
        m_ClaseHasta = now + CLASE_VENTANA_S;
        m_ClaseResumida = false;
        m_ChoferAviso = false;
        MedirCasco();
        CerrarAJugadores();
        BuscarPorClase(now, false);
        EnlazarBots(now);
        ActualizarMarcador(m_AhoraReal, true);
        // ★ no sale hasta que todos estan sentados: el muster de AutoDrive cuenta IsInTransport, y
        // un fantasma lo cumple. BZ_Trader retiene el vehiculo (HoldRunner: AutoDrive lo respeta en cuanto pasa a manejar,
        // con freno de mano) hasta que su tripulacion viva este sentada de verdad. El chofer lo sienta AutoDrive.
        if (Tripulantes() > 0) {
            m_Embarcando = true;
            m_EmbarqueT0 = now;
            m_EmbarqueLogT = now;
            m_EmbarqueAvisoMov = false;
            m_Runner.HoldRunner();
            BZTRLog.Info(Firma(), "embarque: el vehiculo queda RETENIDO en la cabecera (HoldRunner) hasta que los " + Tripulantes().ToString() + " comerciante(s) de la tripulacion esten sentados de verdad (tope " + BZTRLog.Seg(EMBARQUE_LANZAR_TOPE_S) + ")");
        }
    }

    // ★ el casco del vehiculo (su caja y la puerta de cada plaza) para que el comerciante lo rodee a pie, y el
    // archivo de plazas de la clase para el editor ($profile:BZ_Trader/_plazas/<clase>.json, una vez por arranque).
    private void MedirCasco() {
        EntityAI bus = m_Runner.GetBus();
        Transport tr = Transport.Cast(bus);
        string linea;
        float xa;
        float xb;
        float za;
        float zb;
        int i;
        m_Casco = null;
        if (tr) {
            m_Casco = new BZTRCasco(bus);
            m_Casco.IncluirEntradas(tr);
            xa = Math.Round(m_Casco.m_XMin * 10) / 10;
            xb = Math.Round(m_Casco.m_XMax * 10) / 10;
            za = Math.Round(m_Casco.m_ZMin * 10) / 10;
            zb = Math.Round(m_Casco.m_ZMax * 10) / 10;
            BZTRLog.Info(Firma(), "casco de " + bus.GetType() + ": a la derecha " + xa.ToString() + " a " + xb.ToString() + " m, adelante " + za.ToString() + " a " + zb.ToString() + " m (" + m_Casco.m_Fuente + ") · los comerciantes lo rodean a pie");
            linea = BZTRPlazas.Escribir(tr, m_Casco);
            if (linea != "") BZTRLog.Info(Firma(), linea);
        }
        for (i = 0; i < m_Bots.Count(); i++) m_Bots.Get(i).m_Casco = m_Casco;
    }

    // ★ las plazas que comparten puerta (la compuerta de atras del APC: la 2 y la 3, la 4 y la 5) suben de a
    // una. En cada parada se arma el grupo de cada uno (la plaza mas chica de las que comparten con el).
    private void GruposDePuerta(Transport tr) {
        int i;
        int j;
        BZTRBot b;
        BZTRBot o;
        string comparten = "";
        for (i = 0; i < m_Bots.Count(); i++) {
            b = m_Bots.Get(i);
            b.m_GrupoPuerta = b.m_Seat;
            b.m_PuertaOcupada = -1;
            if (!tr) continue;
            for (j = 0; j < m_Bots.Count(); j++) {
                o = m_Bots.Get(j);
                if (o.m_Seat >= b.m_GrupoPuerta) continue;
                if (BZTRCasco.Comparten(tr, b.m_Seat, o.m_Seat)) b.m_GrupoPuerta = o.m_Seat;
            }
            if (b.m_GrupoPuerta != b.m_Seat) comparten = comparten + " " + b.m_Seat.ToString() + "=" + b.m_GrupoPuerta.ToString();
        }
        if (comparten != "") BZTRLog.Info(Firma(), "PARADA (" + m_ParadaNombre + "): plazas que comparten puerta (suben de a una, " + BZTRLog.Seg(BZTRBot.TURNO_S) + " entre uno y otro):" + comparten);
    }

    // Justo antes del tick de un comerciante: ¿otro de su misma puerta la esta usando?
    private void TurnoDe(BZTRBot b, float now) {
        int j;
        BZTRBot o;
        b.m_PuertaOcupada = -1;
        for (j = 0; j < m_Bots.Count(); j++) {
            o = m_Bots.Get(j);
            if (o == b || o.m_GrupoPuerta != b.m_GrupoPuerta) continue;
            if (o.UsaSuPuerta(now)) {
                b.m_PuertaOcupada = o.m_Seat;
                return;
            }
        }
    }

    // Los comerciantes que no manejan (los de la tripulacion de la ruta).
    private int Tripulantes() {
        int i;
        int n = 0;
        for (i = 0; i < m_Bots.Count(); i++) {
            if (!m_Bots.Get(i).m_Chofer) n = n + 1;
        }
        return n;
    }

    // ------------------------------------------------------------------------
    //  EN CALIENTE: lo que el manager le pide cuando cambia su archivo
    // ------------------------------------------------------------------------
    // ¿Se lo puede reemplazar por la version nueva de su archivo? Solo entre lanzamientos (esperando el proximo: el
    // StartDelayS, un reintento o el OnRouteEnd) o ya terminado. Nunca en ruta, en una parada ni en la terminal.
    bool ListoParaReemplazo() {
        if (m_Estado == BZTREstado.ESPERANDO) return true;
        if (m_Estado == BZTREstado.FIN || m_Estado == BZTREstado.APAGADO) return true;
        return false;
    }

    // false = terminado (FIN o APAGADO): el manager lo puede olvidar o relanzar con otra version.
    bool EnServicio() {
        if (m_Estado == BZTREstado.FIN || m_Estado == BZTREstado.APAGADO) return false;
        return true;
    }

    // Lo que le falta al lanzamiento que esta esperando (-1 si no esta esperando): la version nueva lo respeta.
    float EsperaRestante(float now) {
        float s;
        now = Reloj(now);   // del manager: el reloj del juego
        if (m_Estado != BZTREstado.ESPERANDO) return -1;
        s = m_Hasta - now;
        if (s < 0) s = 0;
        return s;
    }

    // Enable 0 o archivo borrado. Devuelve cuando (para el log del manager).
    string PedirRetiro(string motivo) {
        if (m_Estado == BZTREstado.FIN || m_Estado == BZTREstado.APAGADO) return "ya estaba terminado";
        m_RetiroMotivo = motivo;
        if (m_Estado == BZTREstado.PARADA) {
            m_RetiroPedido = true;
            return "se retira al terminar la parada en curso (" + m_ParadaNombre + ")";
        }
        RetirarYTerminar(motivo);
        return "se retira ya (estaba " + BZTREstado.Nombre(m_Estado) + ")";
    }

    // La version nueva de su archivo toma su lugar: este runtime se apaga sin relanzar.
    void Reemplazar() {
        QuitarMarcador();
        if (m_Runner) Retirar();
        m_Estado = BZTREstado.FIN;
        BZTRLog.Info(Firma(), "reemplazado por la version nueva de su archivo");
    }

    private void RetirarYTerminar(string motivo) {
        m_RetiroPedido = false;
        QuitarMarcador();
        if (m_Runner) Retirar();
        Terminar("retirado en caliente: " + motivo, false);
    }

    // ------------------------------------------------------------------------
    //  TICK (1 s, lo llama el manager)
    // ------------------------------------------------------------------------
    void Tick(float now) {
        // del manager: el reloj del juego. La pausa se mira con el; todo lo demas, con el reloj del servicio
        m_AhoraReal = now;
        VigilarPausa(now);
        now = Reloj(now);
        if (m_VerificarBotin && now >= m_VerificarT) VerificarBotin();
        if (m_Estado == BZTREstado.APAGADO || m_Estado == BZTREstado.FIN) return;
        // nunca con comerciantes abajo: la parada termina (todos arriba) y recien ahi se retira
        if (m_RetiroPedido && m_Estado != BZTREstado.PARADA) {
            RetirarYTerminar(m_RetiroMotivo);
            return;
        }
        if (m_Estado == BZTREstado.ESPERANDO) {
            if (now >= m_Hasta) Lanzar(now);
            return;
        }
        if (!m_Runner) {
            Programar(now, REINTENTO_S, "estado " + BZTREstado.Nombre(m_Estado) + " sin runner");
            return;
        }
        if (RespawnDelPanel(now)) return;
        if (m_Estado == BZTREstado.LANZADO) {
            TickLanzado(now);
            return;
        }
        if (m_Estado == BZTREstado.MUERTO) {
            TickMuerto(now);
            return;
        }
        TickServicio(now);
    }

    // ★ EL RESPAWN DEL PANEL DE AUTODRIVE (el boton Respawn, o RESET en la tarjeta del runner: RespawnBus). AutoDrive
    // borra el vehiculo, el chofer y la tripulacion (CleanupEntities) y crea todo de nuevo en el wp 0 en el mismo cuadro,
    // en el MISMO runner, con su hold de spawn y sin avisarle a nadie. La tripulacion nueva sube sola (seguidora del
    // chofer, por eAI) en las plazas que eAI elige: sin la busqueda por clase del lanzamiento quedan cruzadas. Se detecta
    // por el vehiculo: el runner sigue registrado y su vehiculo es OTRO que el de esta vuelta (por el id: el puntero al
    // viejo se anula al borrarlo). Una muerte no cambia el vehiculo y uno perdido lo deja en null. Se trata como un
    // relanzamiento limpio: lo que estaba en curso (parada, saludo, terminal, embarque, pausa) se corta sin bocina ni
    // "sigue", se retira ese runner (Retirar: con un jugador a bordo, el desalojo) y se lanza uno nuevo desde la cabecera,
    // con el embarque y las plazas de siempre. Con un retiro en caliente pedido (Enable 0, archivo borrado), se retira y
    // termina. true = lo detecto (el tick no sigue).
    private bool RespawnDelPanel(float now) {
        EntityAI bus = m_Runner.GetBus();
        if (!bus) return false;
        if (m_BusVueltaId == 0) {
            m_BusVueltaId = bus.GetID();
            return false;
        }
        if (bus.GetID() == m_BusVueltaId) return false;
        BZTRLog.Warn(Firma(), "RESPAWN del panel de AutoDrive: el runner tiene OTRO vehiculo (" + bus.GetType() + " en " + BZTRLog.XZ(bus.GetPosition()) + "; estaba " + BZTREstado.Nombre(m_Estado) + "): se corta lo que estaba en curso y se relanza la vuelta limpia (retiro + lanzamiento desde la cabecera, con las plazas de siempre)");
        m_BocinaTocada = false;
        m_BocinaVista = false;
        QuitarMarcador();
        if (m_RetiroPedido) {
            RetirarYTerminar(m_RetiroMotivo);
            return true;
        }
        Retirar();
        Programar(now, RELANZAR_TRAS_RETIRO_S, "Respawn del panel de AutoDrive: vuelta relanzada limpia");
        return true;
    }

    private void TickLanzado(float now) {
        float s;
        if (m_Runner.RunnerIsActive()) {
            EnRuta(now, "cabecera libre tras " + BZTRLog.Seg(now - m_T0) + ": vehiculo en el mundo");
            return;
        }
        if (m_Runner.RunnerEsperandoCabecera()) {
            s = m_Runner.RunnerEsperandoCabeceraS();
            if (s > CABECERA_MAX_S) {
                BZTRLog.Warn(Firma(), "cabecera ocupada hace " + BZTRLog.Seg(s) + " (tope " + BZTRLog.Seg(CABECERA_MAX_S) + "): se cancela la espera (UnregisterRunner) y se reintenta");
                Retirar();
                Programar(now, REINTENTO_S, "cabecera ocupada");
                return;
            }
            if (now - m_CabeceraAvisoT >= CABECERA_AVISO_S) {
                m_CabeceraAvisoT = now;
                BZTRLog.Info(Firma(), "cabecera ocupada: el runner sigue esperando el wp0 libre (" + BZTRLog.Seg(s) + ")");
            }
            return;
        }
        if (now - m_T0 > LANZADO_MAX_S) FallaDeLanzamiento(now, "el runner dejo de esperar la cabecera y no tiene vehiculo tras " + BZTRLog.Seg(now - m_T0));
    }

    private void TickServicio(float now) {
        string muerte;
        bool terminada;
        muerte = MotivoMuerte();
        if (muerte != "" && m_Estado != BZTREstado.PARADA) {
            Muerte(now, muerte);
            return;
        }
        terminada = m_Runner.RunnerRouteEnded();
        if (terminada && m_Estado == BZTREstado.EN_RUTA) {
            EntrarTerminal(now, "la ruta cerro: " + m_Runner.RunnerEndReason());
            return;
        }
        if (!terminada && !m_Runner.RunnerIsActive()) {
            m_SinVeh = m_SinVeh + 1;
            if (m_SinVeh >= SIN_VEH_TICKS) VehiculoPerdido(now);
            return;
        }
        m_SinVeh = 0;
        if (m_Runner.RunnerIsActive()) m_UltPos = m_Runner.RunnerPos();
        Odometro();
        MedirPlazas(now);
        if (!m_ClaseResumida) BuscarPorClase(now, false);
        EnlazarBots(now);
        SostenerPuertas(now);
        CerrarAJugadores();
        ManosVacias();
        Invencibles();
        Facciones();
        SeguirZona();
        VigilarStock();
        ActualizarMarcador(m_AhoraReal, false);
        ChequearSalida(now);
        // en PAUSA, la parada y el embarque de la cabecera se congelan: ni corren ni sueltan (el reloj ya no avanza)
        if (m_PausaDesde > 0 && (m_Estado == BZTREstado.PARADA || m_Embarcando)) return;
        if (m_Estado == BZTREstado.EN_RUTA && m_Embarcando) TickEmbarque(now);
        else if (m_Estado == BZTREstado.EN_RUTA) TickEnRuta(now);
        else if (m_Estado == BZTREstado.PARADA) TickParada(now);
        else if (m_Estado == BZTREstado.TERMINAL) TickTerminal(now);
        if (m_Runner && !m_Embarcando) TickSenas(now);
    }

    // ★ El reloj del servicio: el del juego menos lo que duro la PAUSA del panel de AutoDrive; durante la pausa, quieto.
    // Todo lo que el manager le pasa al servicio (los ticks, el saludo, programar) entra por aca.
    private float Reloj(float real) {
        if (m_PausaDesde > 0) return m_PausaDesde - m_PausaTotal;
        return real - m_PausaTotal;
    }

    // ★ LA PAUSA del panel de AutoDrive (RunnerIsPaused: el panel o NUMPAD .). Mientras dura, el reloj del servicio no
    // corre. En una parada (marca o saludo) o en el embarque de la cabecera, la parada se congela:
    //  - no corre la venta, ni el tope con jugadores, ni el embarque;
    //  - las puertas quedan como estan y los comerciantes en sus puestos;
    //  - no se suelta el HOLD.
    // Al quitarla sigue con lo que le faltaba, y no antes de PAUSA_MIN_S. En marcha, al vehiculo lo frena AutoDrive; aca
    // solo no corre el reloj (ni el aviso de la salida, ni el descarte de un saludo pedido).
    private void VigilarPausa(float real) {
        bool pausado = false;
        float p;
        string donde = DondePausa();
        string linea;
        if (m_Runner && (m_Estado == BZTREstado.EN_RUTA || m_Estado == BZTREstado.PARADA || m_Estado == BZTREstado.TERMINAL)) pausado = m_Runner.RunnerIsPaused();
        if (pausado && m_PausaDesde <= 0) {
            linea = donde + ": PAUSA del panel de AutoDrive · " + EstadoParaPausa(Reloj(real));
            m_PausaDesde = real;
            if (m_Estado == BZTREstado.PARADA || m_Embarcando) linea = linea + " · se congela: no corre el reloj, las puertas quedan como estan y no se suelta";
            else linea = linea + " · el vehiculo lo frena AutoDrive; el reloj del servicio no corre";
            if (m_BocinaTocada) {
                m_BocinaTocada = false;
                linea = linea + " · la bocina al salir ya habia sonado: se cancela y se vuelve a decidir al quitar la pausa";
            }
            BZTRLog.Info(Firma(), linea);
            return;
        }
        if (!pausado && m_PausaDesde > 0) {
            p = real - m_PausaDesde;
            m_PausaTotal = m_PausaTotal + p;
            m_PausaDesde = 0;
            linea = donde + ": pausa quitada tras " + BZTRLog.Seg(p) + " · " + EstadoParaPausa(Reloj(real));
            if (m_Estado == BZTREstado.PARADA || m_Embarcando) {
                m_TrasPausaHasta = Reloj(real) + PAUSA_MIN_S;
                linea = linea + " · sigue donde estaba, no antes de " + BZTRLog.Seg(PAUSA_MIN_S);
            }
            BZTRLog.Info(Firma(), linea);
        }
    }

    // donde lo agarro la pausa, para las lineas
    private string DondePausa() {
        if (m_Estado == BZTREstado.PARADA) return "PARADA (" + m_ParadaNombre + ")";
        if (m_Embarcando) return "cabecera (embarque)";
        if (m_Estado == BZTREstado.TERMINAL) return "terminal";
        return "en ruta";
    }

    // en que va la parada (o el embarque) al pausar y al quitar la pausa
    private string EstadoParaPausa(float now) {
        int abajo = 0;
        int i;
        float faltan;
        string s;
        if (m_Estado == BZTREstado.PARADA && m_ParadaFase == 2) {
            faltan = m_EsperaMin - (now - m_EsperaT0);
            if (faltan < 0) faltan = 0;
            s = "reloj en " + BZTRLog.Seg(now - m_EsperaT0) + " de minimo " + BZTRLog.Seg(m_EsperaMin) + " (faltan " + BZTRLog.Seg(faltan) + ")";
            if (m_TipoParada == BZTRTipoParada.SENA) s = s + " · puertas abiertas";
            return s;
        }
        if (m_Estado == BZTREstado.PARADA) {
            for (i = 0; i < m_Bots.Count(); i++) {
                if (m_Bots.Get(i).Abajo()) abajo = abajo + 1;
            }
            return "a los " + BZTRLog.Seg(now - m_ParadaT0) + " de parada · " + abajo.ToString() + " comerciante(s) abajo (siguen en sus puestos)";
        }
        if (m_Embarcando) return "embarque a los " + BZTRLog.Seg(now - m_ClaseT0) + " del lanzamiento";
        if (!m_Runner) return "sin vehiculo";
        return "a " + Math.Round(m_Runner.RunnerSpeedKmh()).ToString() + " km/h";
    }

    // despues de quitar la pausa, la parada (o la cabecera) no sigue antes de PAUSA_MIN_S
    private bool RecienReanudada(float now) {
        return now < m_TrasPausaHasta;
    }

    // ------------------------------------------------------------------------
    //  EL COMERCIO: enganchar a cada plaza que el archivo declara comerciante
    // ------------------------------------------------------------------------
    private void EnlazarBots(float now) {
        Transport tr = Transport.Cast(m_Runner.GetBus());
        int i;
        BZTRBot b;
        eAIBase x;
        string vistio;
        string linea;
        for (i = 0; i < m_Bots.Count(); i++) {
            b = m_Bots.Get(i);
            if (b.Abajo()) continue;   // abajo es nuestro: no se re-busca en la plaza
            vistio = b.VestirSiToca(now);
            if (vistio != "") {
                BZTRLog.Info(Firma(), b.Quien() + ": " + vistio);
                MedirPlaza(b, tr, "tras vestirse");
            }
            x = null;
            if (b.m_Chofer) x = m_Runner.GetDriver();
            else if (tr) x = eAIBase.Cast(tr.CrewMember(b.m_Seat));
            if (!x || !x.IsAlive()) continue;
            if (x == b.m_Bot && b.m_Enganchado) continue;
            Enganchar(b, x, now);
            if (b.m_Chofer) continue;
            // sentado por la ruta (en el grupo Passive de Boris ya es pasivo por la faccion): explicito igual
            linea = b.Pacificar();
            if (linea != "") BZTRLog.Info(Firma(), b.Quien() + ": " + linea);
            MedirPlaza(b, tr, "enganchado ya sentado por la ruta +0 s");
            b.m_MedirT0 = now;
            b.m_Medidas = 0;
        }
    }

    // ★ una linea con el estado de la plaza de un comerciante.
    // el FANTASMA (anotado en su plaza sin comando de vehiculo) se dice con esa palabra y como aviso.
    private void MedirPlaza(BZTRBot b, Transport tr, string cuando) {
        if (!b || b.m_Chofer) return;
        if (b.EsFantasma(tr)) {
            BZTRLog.Warn(Firma(), b.Quien() + ": plaza " + cuando + " · FANTASMA (anotado en la plaza, sin comando de vehiculo: no va a bajar) · " + b.EstadoPlaza(tr));
            return;
        }
        BZTRLog.Info(Firma(), b.Quien() + ": plaza " + cuando + " · " + b.EstadoPlaza(tr));
    }

    // ★ las puertas de TODAS las plazas abiertas se cierran. Devuelve cuales ("" si ninguna).
    // una puerta puede quedar abierta (la abre un fantasma en Vehicles/GoToVehicle, por ejemplo la del acompanante). Y una
    // puerta abierta retiene a un eAI en el estado Vehicles.
    private string CerrarPuertas(EntityAI bus) {
        Transport tr = Transport.Cast(bus);
        int i;
        bool hay;
        string src;
        string abiertas = "";
        if (!tr) return "";
        for (i = 0; i < tr.CrewSize(); i++) {
            hay = false;
            src = "";
            ExpansionFSMHelper.DoorAnimationSource(bus, i, hay, src);
            if (!hay || src == "") continue;
            if (bus.GetAnimationPhase(src) <= 0.5) continue;
            bus.SetAnimationPhase(src, 0.0);
            abiertas = abiertas + " " + i.ToString();
        }
        return abiertas;
    }

    // Cada tick: la puerta del que se acaba de sentar y, unos segundos despues de seguir, las de todas las plazas.
    private void SostenerPuertas(float now) {
        EntityAI bus = m_Runner.GetBus();
        int i;
        string abiertas;
        if (!bus) return;
        for (i = 0; i < m_Bots.Count(); i++) m_Bots.Get(i).SostenerPuerta(now, bus);
        if (now >= m_PuertasHasta) return;
        abiertas = CerrarPuertas(bus);
        if (abiertas == "" || m_PuertasAvisadas) return;
        m_PuertasAvisadas = true;
        BZTRLog.Warn(Firma(), "se volvio a abrir la puerta de la(s) plaza(s)" + abiertas + " despues de seguir: cerrada otra vez (se sostiene " + BZTRLog.Seg(PUERTAS_SOSTENER_S) + " desde que sigue)");
    }

    private void MedirTodas(string cuando) {
        Transport tr = Transport.Cast(m_Runner.GetBus());
        int i;
        for (i = 0; i < m_Bots.Count(); i++) MedirPlaza(m_Bots.Get(i), tr, cuando);
    }

    // +1 s y +5 s desde que se lo sento (por clase) o se lo engancho sentado (por la ruta).
    private void MedirPlazas(float now) {
        Transport tr = Transport.Cast(m_Runner.GetBus());
        int i;
        BZTRBot b;
        float t;
        for (i = 0; i < m_Bots.Count(); i++) {
            b = m_Bots.Get(i);
            if (b.m_MedirT0 <= 0 || b.Abajo()) continue;
            t = now - b.m_MedirT0;
            if (b.m_Medidas == 0 && t >= 1) {
                b.m_Medidas = 1;
                MedirPlaza(b, tr, "+" + BZTRLog.Seg(t));
                continue;
            }
            if (b.m_Medidas == 1 && t >= 5) {
                b.m_Medidas = 2;
                b.m_MedirT0 = 0;
                MedirPlaza(b, tr, "+" + BZTRLog.Seg(t));
            }
        }
    }

    private void Enganchar(BZTRBot b, eAIBase x, float now) {
        BZTRManos.Registrar(x);   // eAI no le saca nada a la mano
        ExpansionTraderAIBase tAI = ExpansionTraderAIBase.Cast(x);
        ExpansionTraderObjectBase obj;
        ExpansionMarketTrader mk;
        ExpansionMarketTraderZone zn;
        BZTRTrader c = b.m_Cfg;
        string invalidos;
        string linea;
        string mercado = "NINGUNO";
        string zona = "NINGUNA";
        string bound = "0";
        string zonaMovida;
        string noCargado;
        int bits;
        b.m_Bot = x;
        b.m_Enganchado = true;
        b.m_Enganches = b.m_Enganches + 1;
        if (c.TraderAIClass != "" && x.GetType() != c.TraderAIClass) BZTRLog.Warn(Firma(), b.Quien() + ": la ruta creo " + x.GetType() + " y el archivo dice " + c.TraderAIClass + " (manda la Crew de la ruta)");
        if (!tAI) {
            BZTRLog.Err(Firma(), b.Quien() + ": comercio NO enganchado tradeBound=0 · el bot es " + x.GetType() + ", no un ExpansionTraderAI* (la clase la pone la ruta: DriverClass o Crew cls)");
            return;
        }
        zonaMovida = ZonaSobre(x.GetPosition());
        // un comerciante del Market escrito despues del arranque se carga aca, sin reiniciar (si puede)
        noCargado = BZTRMercado.Asegurar(c.TraderFile, Firma());
        if (noCargado != "") BZTRLog.Err(Firma(), b.Quien() + ": " + noCargado);
        tAI.LoadTrader(c.TraderFile);
        bits = c.BitsLooting(invalidos);
        tAI.eAI_SetLootingBehavior(bits);
        // ★ los comerciantes son SIEMPRE invencibles (el que maneja tambien) y no se saquean
        tAI.Expansion_SetCanBeLooted(false);
        x.SetAllowDamage(false);
        if (c.SeViste()) b.Desvestir(now);
        obj = tAI.GetTraderObject();
        if (obj) zonaMovida = zonaMovida + AsignarZona(obj);
        if (obj) {
            bound = "1";
            mk = obj.GetTraderMarket();
            zn = obj.GetTraderZone();
            if (mk) mercado = mk.m_FileName;
            if (zn) zona = zn.m_FileName + " (" + zn.m_DisplayName + ")";
        }
        linea = b.Quien() + ": comercio enganchado tradeBound=" + bound + " · " + x.GetType() + " · TraderFile " + c.TraderFile;
        linea = linea + " · mercado " + mercado + " · zona " + zona + zonaMovida;
        linea = linea + " · en " + BZTRLog.XZ(x.GetPosition()) + " · enganche #" + b.m_Enganches.ToString();
        BZTRLog.Info(Firma(), linea);
        if (invalidos != "") BZTRLog.Warn(Firma(), b.Quien() + ": LootingBehaviour con tokens desconocidos, ignorados: " + invalidos);
        if (obj && !mk) BZTRLog.Err(Firma(), b.Quien() + ": el Market no conoce el archivo de comerciante '" + c.TraderFile + "' (se cargan de $profile:ExpansionMod/Traders): NO va a vender");
        if (obj && mk && !zn) BZTRLog.Err(Firma(), b.Quien() + ": nacio FUERA de toda zona de comercio (" + BZTRLog.XZ(x.GetPosition()) + "): NO va a vender");
        if (obj && mk) EmpujarDatos(b, obj);
    }

    // ★ el cliente pide los datos del comerciante UNA vez, a los 250 ms de ver la entidad
    // (ExpansionTraderObjectBase.RequestTraderObject, m_TraderObjectRequested), y no vuelve a preguntar. Si el
    // LoadTrader llega despues, ese jugador se queda sin 'Comerciar' ("trader object ... is NULL!" en el server).
    // La rama del server de RPC_TraderObject escribe los datos y se los manda a 'sender' sin leer ctx: se la llama
    // por cada jugador conectado. A quien todavia no tiene la entidad no le llega, y ese la pide solo al verla.
    private void EmpujarDatos(BZTRBot b, ExpansionTraderObjectBase obj) {
        array<Man> jugadores = new array<Man>();
        PlayerIdentity id;
        int i;
        int n = 0;
        int bytes = BytesDatos(obj.GetTraderMarket());
        string linea;
        GetGame().GetPlayers(jugadores);
        for (i = 0; i < jugadores.Count(); i++) {
            if (!jugadores.Get(i)) continue;
            id = jugadores.Get(i).GetIdentity();
            if (!id) continue;
            obj.RPC_TraderObject(id, null);
            n = n + 1;
        }
        linea = b.Quien() + ": datos del comerciante enviados a " + n.ToString() + " jugador(es) (~" + bytes.ToString() + " bytes c/u)";
        BZTRLog.Info(Firma(), linea);
    }

    // ★ lo que pesa el envio de los datos de un comerciante (RPC_TraderObject de Expansion: el archivo, el nombre, el
    // icono, las monedas, el valor y el nombre de la moneda que se muestra, si ordena por categoria y, por categoria, su
    // id y si compra o vende). Cada texto: 4 bytes de largo + el texto; cada numero: 4. Sin la cabecera del RPC.
    static int BytesDatos(ExpansionMarketTrader mk) {
        int n = 0;
        int i;
        int cats = 0;
        int largo;
        string moneda;
        if (!mk) return 0;
        largo = mk.m_FileName.Length();
        n = n + 4 + largo;
        largo = mk.DisplayName.Length();
        n = n + 4 + largo;
        largo = mk.TraderIcon.Length();
        n = n + 4 + largo;
        n = n + 4;
        if (mk.Currencies) {
            for (i = 0; i < mk.Currencies.Count(); i++) {
                moneda = mk.Currencies.Get(i);
                largo = moneda.Length();
                n = n + 4 + largo;
            }
        }
        largo = mk.DisplayCurrencyName.Length();
        n = n + 4 + 4 + largo;
        n = n + 4 + 4;
        if (mk.m_CategoryIDs) cats = mk.m_CategoryIDs.Count();
        n = n + 8 * cats;
        return n;
    }

    // el objeto de comercio de un comerciante enganchado y vivo, con su mercado (null si no)
    private ExpansionTraderObjectBase ObjetoDeComercio(BZTRBot b) {
        ExpansionTraderAIBase tAI;
        ExpansionTraderObjectBase obj;
        if (!b || !b.m_Enganchado || !b.m_Bot || !b.m_Bot.IsAlive()) return null;
        tAI = ExpansionTraderAIBase.Cast(b.m_Bot);
        if (!tAI) return null;
        obj = tAI.GetTraderObject();
        if (!obj || !obj.GetTraderMarket()) return null;
        return obj;
    }

    // ★ Los datos de los comerciantes enganchados de este servicio, a UN jugador (el que entra al server: lo llama el
    // manager). Expansion los pide una sola vez, a los 250 ms de ver al comerciante; si el que entra los pidio antes del
    // enganche, se queda sin «Comerciar» (Expansion le contesta NULL y no vuelve a pedir). Devuelve cuantos; bytes, el total.
    int DatosA(PlayerIdentity ident, out int bytes) {
        int i;
        int n = 0;
        ExpansionTraderObjectBase obj;
        bytes = 0;
        if (!ident) return 0;
        for (i = 0; i < m_Bots.Count(); i++) {
            obj = ObjetoDeComercio(m_Bots.Get(i));
            if (!obj) continue;
            obj.RPC_TraderObject(ident, null);
            n = n + 1;
            bytes = bytes + BytesDatos(obj.GetTraderMarket());
        }
        return n;
    }

    // ★ En cada parada (marca o saludo), ya quieto: los datos de los comerciantes otra vez, a los jugadores a
    // PARADA_DATOS_M. Si a alguno se le perdio el envio, la parada lo repone antes de que quiera comprar.
    private void ReenviarDatosCerca(string donde) {
        array<Man> cerca = new array<Man>();
        PlayerIdentity ident;
        ExpansionTraderObjectBase obj;
        int i;
        int k;
        int nJug = 0;
        int nCom = 0;
        int bytes = 0;
        string linea;
        BZTRJugadores.Cerca(m_Runner.RunnerPos(), PARADA_DATOS_M, cerca);
        for (k = 0; k < m_Bots.Count(); k++) {
            obj = ObjetoDeComercio(m_Bots.Get(k));
            if (!obj) continue;
            nCom = nCom + 1;
            bytes = bytes + BytesDatos(obj.GetTraderMarket());
            for (i = 0; i < cerca.Count(); i++) {
                ident = cerca.Get(i).GetIdentity();
                if (ident) obj.RPC_TraderObject(ident, null);
            }
        }
        for (i = 0; i < cerca.Count(); i++) {
            if (cerca.Get(i).GetIdentity()) nJug = nJug + 1;
        }
        if (nJug == 0 || nCom == 0) return;
        linea = "parada (" + donde + "): datos de " + nCom.ToString() + " comerciante(s) reenviados a " + nJug.ToString() + " jugador(es) a <= " + BZTRLog.Metros(PARADA_DATOS_M);
        linea = linea + " (~" + bytes.ToString() + " bytes c/u)";
        BZTRLog.Info(Firma(), linea);
    }

    // Al lanzar, cada 250 ms (el manager): ver y enganchar a los comerciantes que AutoDrive va creando. Ver BuscarPorClase.
    void TickLanzamientoRapido(float now) {
        now = Reloj(now);   // del manager: el reloj del juego
        if (!m_Runner || m_ClaseResumida || now >= m_ClaseHasta) return;
        BuscarPorClase(now, true);
    }

    // ------------------------------------------------------------------------
    //  AL LANZAR: los comerciantes de la tripulacion, por CLASE, a SU plaza
    // ------------------------------------------------------------------------
    // En la ventana de CLASE_VENTANA_S desde que el vehiculo esta en el mundo: por cada comerciante sin enganchar
    // (no el chofer: a ese lo sienta el runner), el bot a <= CLASE_RADIO_M del vehiculo, de su TraderAIClass (o
    // cualquier ExpansionTraderAI* si el archivo no la dice), vivo, a pie y de nadie; se engancha ahi mismo, sale del
    // grupo de Boris (Apartar) y se lo sienta en SU plaza por teleport EN EL TICK SIGUIENTE (SentarPorClase). Con las
    // plazas llenas el muster de AutoDrive sale enseguida.
    //
    // ⛔ MEDIDO (8 lanzamientos del UAZ = 24 teleports): AutoDrive crea un tripulante por segundo
    // y esta busqueda lo encontraba EN EL MISMO CUADRO en que nacia. Los 21 teleports hechos en ese cuadro quedaron
    // FANTASMAS (al segundo, comando de caminar en vez del de vehiculo: el motor termina el comando y vanilla arranca el
    // de caminar, DayZPlayerImplement.CommandHandler). Los 3 hechos con el bot visto un tick antes (~1 s de vida: vueltas
    // 3 y 7) conservaron el comando, y los 2 que llegaron a la marca BAJARON. La FSM (FollowFormation, Dormant,
    // GoToVehicle) era la consecuencia: Dormant ni siquiera entra con el comando de vehiculo (pide el de caminar).
    //
    // rapido (el tick de 250 ms): SOLO ver y enganchar. El cliente pide los datos del comerciante a los 250 ms de verlo
    // y una sola vez; enganchado al primer cuarto de segundo de nacer, el pedido ya lo encuentra con su comercio. El
    // teleport sigue en el tick de 1 s, con su edad minima (CLASE_EDAD_S desde que se lo vio) y el comando de caminar.
    private void BuscarPorClase(float now, bool rapido) {
        EntityAI bus = m_Runner.GetBus();
        Transport tr = Transport.Cast(bus);
        int i;
        BZTRBot b;
        BZTRTrader cfg;
        eAIBase x;
        Human enLaPlaza;
        float d;
        string clase;
        string linea;
        if (!tr) return;
        if (rapido && now >= m_ClaseHasta) return;
        if (now >= m_ClaseHasta) {
            // el que quedo esperando su tick al cerrar la ventana se sienta igual
            for (i = 0; i < m_Bots.Count(); i++) {
                b = m_Bots.Get(i);
                if (b.m_SentarPendiente && b.m_Bot && b.m_Bot.IsAlive() && !b.m_Bot.IsInTransport()) SentarPorClase(b, tr, now, true);
            }
            ResumenPorClase(tr);
            return;
        }
        for (i = 0; i < m_Bots.Count(); i++) {
            b = m_Bots.Get(i);
            if (b.m_Chofer || b.Abajo()) continue;
            cfg = b.m_Cfg;
            clase = cfg.TraderAIClass;
            if (clase == "") clase = "ExpansionTraderAI*";
            enLaPlaza = tr.CrewMember(b.m_Seat);
            // ya enganchado por clase y todavia a pie: esperando su tick o el teleport no quedo
            if (b.m_PorClase && b.m_Bot && b.m_Bot.IsAlive() && !b.m_Bot.IsInTransport()) {
                if (rapido) continue;
                if (b.m_SentarPendiente) {
                    SentarPorClase(b, tr, now, false);
                    continue;
                }
                if (now - b.m_PorClaseT < CLASE_REINTENTO_S) continue;
                // su plaza ocupada por otro (un jugador): no se reintenta encima (le pisaria la plaza); se le avisa
                if (enLaPlaza) {
                    if (rapido) continue;
                    linea = b.PlazaOcupada(now, tr);
                    if (linea != "") BZTRLog.Warn(Firma(), b.Quien() + ": " + linea);
                    continue;
                }
                b.m_PorClaseT = now;
                BZTRLog.Warn(Firma(), b.Quien() + ": " + clase + " no quedo sentado en " + BZTRLog.Seg(CLASE_REINTENTO_S) + " (plaza libre): se reintenta el teleport");
                b.SentarCon(m_Runner, tr, true, "BZ_Trader: plaza declarada (reintento)");
                MedirPlaza(b, tr, "tras el teleport (reintento) +0 s");
                b.m_MedirT0 = now;
                b.m_Medidas = 0;
                continue;
            }
            if (b.m_Enganchado) continue;
            if (!rapido && !b.m_AvisoClase && now - m_ClaseT0 >= CLASE_AVISO_S) {
                b.m_AvisoClase = true;
                BZTRLog.Warn(Firma(), "plaza " + b.m_Seat.ToString() + ": no encontre a " + clase + " cerca del vehiculo (a <= " + BZTRLog.Metros(CLASE_RADIO_M) + ", vivo, a pie) ni sentado en su plaza en " + BZTRLog.Seg(CLASE_AVISO_S));
            }
            // la plaza ya tiene a alguien: un bot lo engancha EnlazarBots (con aviso si la clase no es); a un jugador se le
            // avisa que la deje
            if (enLaPlaza) {
                if (!rapido && !eAIBase.Cast(enLaPlaza)) {
                    linea = b.PlazaOcupada(now, tr);
                    if (linea != "") BZTRLog.Warn(Firma(), b.Quien() + ": " + linea);
                }
                continue;
            }
            x = CandidatoDeClase(b, bus, d);
            if (!x) continue;
            linea = b.Quien() + ": " + x.GetType() + " encontrado a " + BZTRLog.Metros(d) + " del vehiculo, a pie a los " + BZTRLog.Seg(now - m_ClaseT0);
            linea = linea + " (cmd " + x.GetCurrentCommandID().ToString() + ")";
            if (rapido) linea = linea + " (visto en el tick de 250 ms)";
            BZTRLog.Info(Firma(), linea + " · se engancha, sale del grupo de Boris y se sienta en SU plaza por teleport en el tick siguiente");
            Enganchar(b, x, now);
            b.m_PorClase = true;
            b.m_PorClaseT = now;
            b.m_VistoT = now;
            b.m_VistoAviso = false;
            b.m_SentarPendiente = true;
            b.Apartar();
            if (b.m_Pacificado != "") BZTRLog.Info(Firma(), b.Quien() + ": " + b.m_Pacificado);
            MedirPlaza(b, tr, "visto a pie, en su grupo propio y pasivo, antes del teleport");
        }
    }

    // ★ el teleport del lanzamiento, un tick despues de verlo y con su comando de caminar andando
    // (DayZPlayerConstants.COMMANDID_MOVE). Si a CLASE_EDAD_MAX_S todavia no camina, o se cierra la ventana, igual.
    private void SentarPorClase(BZTRBot b, Transport tr, float now, bool forzar) {
        float edad = now - b.m_VistoT;
        int cmd = b.m_Bot.GetCurrentCommandID();
        string como;
        if (!forzar && edad < CLASE_EDAD_S) return;
        if (!forzar && cmd != DayZPlayerConstants.COMMANDID_MOVE && edad < CLASE_EDAD_MAX_S) {
            if (!b.m_VistoAviso) {
                b.m_VistoAviso = true;
                BZTRLog.Info(Firma(), b.Quien() + ": espera para sentarlo: cmd " + cmd.ToString() + " (todavia no camina) a los " + BZTRLog.Seg(edad) + " de verlo (tope " + BZTRLog.Seg(CLASE_EDAD_MAX_S) + ")");
            }
            return;
        }
        como = "visto hace " + BZTRLog.Seg(edad) + ", cmd " + cmd.ToString();
        if (cmd != DayZPlayerConstants.COMMANDID_MOVE) BZTRLog.Warn(Firma(), b.Quien() + ": se lo sienta sin que camine todavia (" + como + ")");
        b.m_SentarPendiente = false;
        b.m_PorClaseT = now;
        // se viste un segundo despues del teleport, como en los 3 que quedaron bien (no en el mismo tick)
        if (b.m_VestirPendiente && b.m_VestirT < now + 1) b.m_VestirT = now + 1;
        // su plaza ocupada por otro (un jugador): no se lo sienta encima; el reintento de BuscarPorClase le avisa
        if (!b.SentarCon(m_Runner, tr, true, "BZ_Trader: plaza declarada")) {
            como = b.PlazaOcupada(now, tr);
            if (como != "") BZTRLog.Warn(Firma(), b.Quien() + ": " + como);
            return;
        }
        MedirPlaza(b, tr, "tras el teleport del lanzamiento +0 s (" + como + ")");
        b.m_MedirT0 = now;
        b.m_Medidas = 0;
    }

    // El mas cercano de la clase del comerciante, a <= CLASE_RADIO_M del vehiculo, vivo, a pie y sin dueno.
    private eAIBase CandidatoDeClase(BZTRBot b, EntityAI bus, out float dMejor) {
        array<Object> objetos = new array<Object>();
        array<CargoBase> cargas = new array<CargoBase>();
        eAIBase x;
        eAIBase mejor = null;
        eAIBase chofer = m_Runner.GetDriver();
        BZTRTrader cfg = b.m_Cfg;
        string clase = cfg.TraderAIClass;
        float d;
        int i;
        dMejor = -1;
        GetGame().GetObjectsAtPosition3D(bus.GetPosition(), CLASE_RADIO_M, objetos, cargas);
        for (i = 0; i < objetos.Count(); i++) {
            x = eAIBase.Cast(objetos.Get(i));
            if (!x || x == chofer || !x.IsAlive() || x.IsInTransport()) continue;
            if (clase != "" && x.GetType() != clase) continue;
            if (clase == "" && !ExpansionTraderAIBase.Cast(x)) continue;
            if (DeOtro(x)) continue;
            d = vector.Distance(x.GetPosition(), bus.GetPosition());
            if (m_Retirados.Find(x) >= 0) {
                if (m_RetiradosAvisados.Find(x) < 0) {
                    m_RetiradosAvisados.Insert(x);
                    BZTRLog.Info(Firma(), b.Quien() + ": descartado " + x.GetType() + " a " + BZTRLog.Metros(d) + " del vehiculo: es del runner anterior (se esta retirando)");
                }
                continue;
            }
            if (mejor && d >= dMejor) continue;
            mejor = x;
            dMejor = d;
        }
        return mejor;
    }

    private bool DeOtro(eAIBase x) {
        int i;
        for (i = 0; i < m_Bots.Count(); i++) {
            if (m_Bots.Get(i).m_Bot == x) return true;
        }
        return false;
    }

    // Una linea al cerrar la ventana: quien quedo en su plaza y como.
    private void ResumenPorClase(Transport tr) {
        int i;
        BZTRBot b;
        string s = "";
        string como;
        string abiertas;
        if (m_ClaseResumida) return;
        m_ClaseResumida = true;
        m_Retirados.Clear();
        m_RetiradosAvisados.Clear();
        for (i = 0; i < m_Bots.Count(); i++) {
            b = m_Bots.Get(i);
            if (b.m_Chofer) continue;
            como = "VACIA";
            if (tr.CrewMember(b.m_Seat)) {
                como = tr.CrewMember(b.m_Seat).GetType();
                if (b.m_PorClase && tr.CrewMember(b.m_Seat) == b.m_Bot) como = como + " (por clase)";
                else if (b.m_PorClase) como = como + " (por la ruta; el de su clase no quedo)";
                else como = como + " (lo sento la ruta)";
                if (b.EsFantasma(tr)) como = como + " FANTASMA";
            }
            s = s + " · plaza " + b.m_Seat.ToString() + " " + como;
        }
        // ya en marcha, ninguna puerta de plaza abierta
        abiertas = CerrarPuertas(tr);
        if (abiertas != "") s = s + " · puerta(s) abierta(s) cerrada(s):" + abiertas;
        BZTRLog.Info(Firma(), "busqueda por clase cerrada a los " + BZTRLog.Seg(CLASE_VENTANA_S) + s);
    }

    // ★ los comerciantes son SIEMPRE invencibles, arriba y abajo, el que maneja tambien. AutoDrive
    // hace invencible a la Crew hasta que baja, y a la plaza 0 le aplica el DriverInvincible de la ruta (true por defecto).
    private void Invencibles() {
        int i;
        BZTRBot b;
        for (i = 0; i < m_Bots.Count(); i++) {
            b = m_Bots.Get(i);
            if (!b.m_Bot) continue;
            b.m_Bot.SetAllowDamage(false);
        }
    }

    // ★ nuestros comerciantes son SIEMPRE de la faccion BZTRDef.FACCION (InvincibleObservers) y pasivos, en el grupo que
    // esten: el propio (ya nace con ella) o el del runner de AutoDrive, que lleva al que maneja y a la tripulacion hasta
    // apartarla y que AutoDrive crea "Passive" en cada aparicion. Cambiar la faccion de un grupo la cambia a todos sus
    // miembros y llega al juego de cada jugador (el tipo de faccion se sincroniza): "Reclutar" no aparece. AutoDrive no
    // mira la faccion. El pasivo tambien en cada tick: AutoDrive se lo saca a un tripulante al sentarlo.
    private void Facciones() {
        int i;
        BZTRBot b;
        eAIGroup g;
        eAIFaction bzFaccion;
        string antes;
        for (i = 0; i < m_Bots.Count(); i++) {
            b = m_Bots.Get(i);
            if (!b.m_Bot) continue;
            b.m_Bot.eAI_SetPassive(true);
            g = b.m_Bot.GetGroup();
            if (!g || eAIFactionInvincibleObservers.Cast(g.GetFaction())) continue;
            bzFaccion = eAIFaction.Create(BZTRDef.FACCION);
            if (!bzFaccion) continue;
            antes = g.GetFaction().GetName();
            g.SetFaction(bzFaccion);
            b.m_FaccionVeces = b.m_FaccionVeces + 1;
            if (b.m_FaccionVeces == 1) BZTRLog.Info(Firma(), b.Quien() + ": faccion " + BZTRDef.FACCION + " (su grupo era " + antes + "; cambia para todo el grupo)");
            else if (b.m_FaccionVeces <= 3) BZTRLog.Warn(Firma(), b.Quien() + ": la faccion de su grupo volvio a " + antes + " (vez " + b.m_FaccionVeces.ToString() + " en esta vuelta): otra vez " + BZTRDef.FACCION);
        }
    }

    // ------------------------------------------------------------------------
    //  LA ZONA DEL SERVICIO (TraderZone): su centro sigue al vehiculo
    // ------------------------------------------------------------------------
    // La zona se busca UNA vez, en el primer uso (el primer lanzamiento), y se guarda. null = sin zona propia.
    private ExpansionMarketTraderZone Zona() {
        if (m_Cfg.TraderZone.Name == "") return null;
        if (!m_ZonaBuscada) {
            m_ZonaBuscada = true;
            m_Zona = ZonaVista();
            if (!m_Zona) {
                m_Zona = BuscarZona();
                if (m_Zona) AnotarZonaVista(m_Zona);
            }
        }
        return m_Zona;
    }

    static string ClaveZona(string nombre) {
        string clave = nombre;
        clave.Replace(".json", "");
        clave.ToLower();
        return clave;
    }

    // ★ la zona ya resuelta en este proceso (ver s_ZonasVistas). null = todavia no.
    private ExpansionMarketTraderZone ZonaVista() {
        ExpansionMarketTraderZone zv;
        bool propia = false;
        string clave = ClaveZona(m_Cfg.TraderZone.Name);
        string como = "por el Market";
        if (!s_ZonasVistas) return null;
        if (!s_ZonasVistas.Find(clave, zv) || !zv) return null;
        if (s_ZonasPropias) s_ZonasPropias.Find(clave, propia);
        m_ZonaPropia = propia;
        if (propia) como = "por BZ_Trader, de su archivo (no esta en la lista del Market)";
        BZTRLog.Info(Firma(), "TraderZone '" + zv.m_FileName + "': ya cargada " + como + " · se asigna (centro ahora en " + BZTRLog.XZ(zv.Position) + ", radio " + zv.Radius.ToString() + " m)");
        return zv;
    }

    private void AnotarZonaVista(ExpansionMarketTraderZone z) {
        string clave = ClaveZona(m_Cfg.TraderZone.Name);
        if (!s_ZonasVistas) s_ZonasVistas = new map<string, ref ExpansionMarketTraderZone>();
        if (!s_ZonasPropias) s_ZonasPropias = new map<string, bool>();
        s_ZonasVistas.Set(clave, z);
        s_ZonasPropias.Set(clave, m_ZonaPropia);
    }

    // ⛔ SOLO CON LA API PUBLICA DEL MARKET. Un modded class de ExpansionMarketSettings (para leer su lista protected
    // de zonas) rompe al Market: se carga a si misma con JsonFileLoader<ExpansionMarketSettings>(this) y con el modded
    // el modulo Game no compila. Receta:
    //   1. la posicion de la zona, de su archivo ($mission:expansion\traderzones\<Name>.json, campo Position): es la
    //      misma con la que el Market la cargo al arrancar (y la que guarda en cada venta);
    //   2. GetTraderZoneByPosition(esa posicion): entre las zonas que contienen el punto gana la de centro mas
    //      cercano, y la nuestra esta a 0 m;
    //   3. verificar m_FileName. Si no es la pedida, sin zona propia (el comerciante usa la del lugar).
    private ExpansionMarketTraderZone BuscarZona() {
        string nombre = m_Cfg.TraderZone.Name;
        string buscado;
        string encontrado;
        string path;
        string texto;
        BZTRNodo raiz;
        array<float> xyz = new array<float>();
        vector p;
        ExpansionMarketSettings ms;
        ExpansionMarketTraderZone z;
        string otra;
        nombre.Replace(".json", "");
        path = "$mission:expansion\\traderzones\\" + nombre + ".json";
        // red de seguridad: un servicio armado a mano, o con el editor sin la mision, puede nombrar una zona
        // que no existe. Se crea aca con el Save del propio Market (el mismo formato) y se sigue por ZonaDeSuArchivo.
        if (!FileExist(path)) return CrearZona(nombre, path);
        texto = BZTRJson.LeerTexto(path);
        if (texto == "") {
            BZTRLog.Err(Firma(), "TraderZone '" + nombre + "': no se pudo leer " + path + " · sin zona propia: los comerciantes se enganchan con la zona del lugar");
            return null;
        }
        raiz = new BZTRNodo(texto, texto.IndexOf("{"));
        raiz.Numeros("Position", xyz);
        if (xyz.Count() < 3) {
            BZTRLog.Err(Firma(), "TraderZone '" + nombre + "': " + path + " no trae Position [x, y, z] · sin zona propia");
            return null;
        }
        p = Vector(xyz.Get(0), xyz.Get(1), xyz.Get(2));
        ms = GetExpansionSettings().GetMarket();
        if (!ms) {
            BZTRLog.Err(Firma(), "TraderZone '" + nombre + "': el Market no esta cargado · sin zona propia");
            return null;
        }
        z = ms.GetTraderZoneByPosition(p);
        otra = "ninguna";
        if (z) {
            buscado = nombre;
            buscado.ToLower();
            encontrado = z.m_FileName;
            encontrado.ToLower();
            if (encontrado == buscado) {
                BZTRLog.Info(Firma(), "TraderZone '" + z.m_FileName + "': ya cargada por el Market (" + z.m_DisplayName + ", encontrada en su posicion de archivo " + BZTRLog.XZ(p) + ") · se asigna · radio " + z.Radius.ToString() + " m");
                return z;
            }
            otra = "'" + z.m_FileName + "'";
        }
        return ZonaDeSuArchivo(nombre, p, otra);
    }

    // ★ LA ZONA NUEVA SIN REINICIO. El Market lee las zonas solo al arrancar y su lista es protected (y
    // ExpansionMarketSettings no se puede moddear). Pero ExpansionMarketTraderZone.Load(nombre) es publico, y
    // el comerciante usa SU zona asignada para todo (comprar, vender, el stock que ve el cliente); la busqueda por
    // posicion es solo la del LoadTrader. Entonces: la zona se carga de su archivo, se guarda aca, y despues de cada
    // LoadTrader se le asigna al comerciante con SetTraderZone + UpdateTraderZone (que le agrega los articulos que le
    // falten, con el stock que fija el Market). Ningun comerciante ajeno la encuentra (no esta en la lista).
    // Si en ese punto ganaba otra zona y la nuestra SI estaba cargada en el Market, quedarian dos copias: el log lo dice.
    private ExpansionMarketTraderZone ZonaDeSuArchivo(string nombre, vector p, string otra) {
        ExpansionMarketTraderZone zp = ExpansionMarketTraderZone.Load(nombre);
        string linea;
        if (!zp) {
            BZTRLog.Err(Firma(), "TraderZone '" + nombre + "': el Market no la tiene cargada y no se pudo leer su archivo · sin zona propia");
            return null;
        }
        zp.Update(false, null);   // lo mismo que hace el Market al cargar sus zonas (los dos argumentos son opcionales)
        m_ZonaPropia = true;
        linea = "TraderZone '" + nombre + "': no estaba cargada (en " + BZTRLog.XZ(p) + " gana " + otra + ") · se carga del archivo";
        linea = linea + " y se asigna a cada comerciante con SetTraderZone · radio " + zp.Radius.ToString() + " m · " + zp.Stock.Count().ToString() + " articulo(s) en stock";
        BZTRLog.Info(Firma(), linea);
        // el Market no da su lista de zonas: si la tenia cargada en OTRA posicion (la archivo la trae de donde se guardo la
        // ultima vez), no se puede ver desde aca; quedarian dos copias. Dentro de un proceso ya no pasa (s_ZonasVistas)
        if (otra != "ninguna") BZTRLog.Warn(Firma(), "TraderZone '" + nombre + "': si el Market la tenia cargada en otra posicion (gana " + otra + " por estar mas cerca), quedan dos copias de su stock: reiniciar el server");
        return zp;
    }

    // ★ la zona que el servicio nombra y no existe. Centro = el inicio de la ruta (el wp 0 de la toma), radio =
    // el del servicio, sin stock: UpdateTraderZone le agrega los articulos de cada comerciante al engancharse (con
    // MaxStockThreshold x InitStockPercent de su categoria). La escribe ExpansionMarketTraderZone.Save (el formato del
    // Market, en $mission:expansion\traderzones\). No esta en la lista del Market: se asigna con SetTraderZone.
    private ExpansionMarketTraderZone CrearZona(string nombre, string path) {
        ExpansionMarketTraderZone zc = new ExpansionMarketTraderZone();
        vector p;
        string linea;
        if (m_WpPos.Count() > 0) p = m_WpPos.Get(0);
        else if (m_Runner && m_Runner.GetBus()) p = m_Runner.GetBus().GetPosition();
        else {
            BZTRLog.Err(Firma(), "TraderZone '" + nombre + "': no existe y no se donde crearla (sin la toma ni el vehiculo) · sin zona propia");
            return null;
        }
        zc.Defaults();
        zc.m_FileName = nombre;
        zc.m_DisplayName = "BZ_Trader " + m_Cfg.Name;
        zc.Position = p;
        zc.Radius = m_Cfg.TraderZone.RadiusM;
        zc.Save();
        if (!FileExist(path)) {
            BZTRLog.Err(Firma(), "TraderZone '" + nombre + "': no existia y no se pudo crear " + path + " · sin zona propia");
            return null;
        }
        m_ZonaPropia = true;
        linea = "TraderZone '" + nombre + "': no existia · creada en la mision (" + path + ", centro " + BZTRLog.XZ(p) + " = el inicio de la ruta, radio ";
        linea = linea + zc.Radius.ToString() + " m, sin stock: el Market le agrega los articulos de cada comerciante al engancharse) · se asigna con SetTraderZone";
        BZTRLog.Info(Firma(), linea);
        return zc;
    }

    // Despues de cada LoadTrader: la zona del servicio, asignada al comerciante si no es la suya. "" o el detalle.
    private string AsignarZona(ExpansionTraderObjectBase obj) {
        ExpansionMarketTraderZone z = Zona();
        if (!z || !obj.GetTraderMarket()) return "";
        if (obj.GetTraderZone() == z) return "";
        obj.SetTraderZone(z);
        obj.UpdateTraderZone();
        if (m_ZonaPropia) return " (zona del servicio asignada con SetTraderZone: no esta en la lista del Market)";
        return " (zona del servicio asignada con SetTraderZone: en su lugar ganaba otra)";
    }

    // El vigia del stock: un articulo que ya estaba y cambia de stock = alguien compro o vendio. Es la prueba, del lado
    // del server, de que un jugador pudo comerciar con estos comerciantes (el comerciante cargado en caliente).
    private void VigilarStock() {
        ExpansionMarketTraderZone z;
        string linea = "";
        int antes;
        int n = 0;
        int resto;
        if (!m_ZonaBuscada) return;
        z = m_Zona;
        if (!z || !z.Stock) return;
        if (!m_StockVisto) {
            m_StockVisto = new map<string, int>();
            foreach (string clase0, int stock0 : z.Stock) {
                m_StockVisto.Set(clase0, stock0);
            }
            return;
        }
        foreach (string clase, int stock : z.Stock) {
            if (!m_StockVisto.Find(clase, antes)) {
                m_StockVisto.Set(clase, stock);   // recien agregado por UpdateTraderZone: no es un comercio
                continue;
            }
            if (antes == stock) continue;
            m_StockVisto.Set(clase, stock);
            n = n + 1;
            if (n <= 3) linea = linea + " · " + clase + " " + antes.ToString() + " -> " + stock.ToString();
        }
        if (n == 0) return;
        resto = n - 3;
        if (resto > 0) linea = linea + " · y " + resto.ToString() + " mas";
        BZTRLog.Info(Firma(), "COMERCIO en la zona '" + z.m_FileName + "' (un jugador compro o vendio):" + linea);
    }

    // Antes de cada LoadTrader: la zona del servicio, sobre el comerciante (asi gana: centro a 0 m). "" o el detalle.
    private string ZonaSobre(vector p) {
        ExpansionMarketTraderZone z = Zona();
        if (!z) return "";
        z.Position = p;
        z.Radius = m_Cfg.TraderZone.RadiusM;
        return " (zona del servicio movida al comerciante, radio " + m_Cfg.TraderZone.RadiusM.ToString() + " m)";
    }

    private void SeguirZona() {
        ExpansionMarketTraderZone z;
        EntityAI v;
        if (m_Cfg.TraderZone.Name == "" || m_Cfg.TraderZone.FollowsVehicle != 1) return;
        v = m_Runner.GetBus();
        if (!v) return;
        z = Zona();
        if (!z) return;
        z.Position = v.GetPosition();
        z.Radius = m_Cfg.TraderZone.RadiusM;
    }

    // ------------------------------------------------------------------------
    //  LA RUTA: marcas, senas y terminal
    // ------------------------------------------------------------------------
    private void TickEnRuta(float now) {
        bool held = m_Runner.IsHeld();
        int idx = m_Runner.RunnerWpIdx();
        int tot = m_Runner.RunnerWpTotal();
        bool cercaDelFin = false;
        if (tot > 0 && idx >= tot - 1 - MARGEN_FIN_WP) cercaDelFin = true;

        // flanco de subida del HOLD = llego a una parada (o a la terminal)
        if (held && !m_WasHeld) {
            m_WasHeld = true;
            ArribarHold(now, idx, tot, cercaDelFin);
            return;
        }
        if (!held) m_WasHeld = false;

        PedirMarca(idx);

        if (m_SenaPedida && !held && now - m_SenaPedidaT > SENA_PEDIDA_MAX_S) {
            m_SenaPedida = false;
            BZTRLog.Warn(Firma(), "la sena de " + m_SenaDe + " no engancho HOLD en " + BZTRLog.Seg(SENA_PEDIDA_MAX_S) + ": se descarta");
        }

        // ★ LA TERMINAL LA SOSTENEMOS NOSOTROS: sin HOLD, al completar la ruta AutoDrive despawnea (origen "lista").
        if (cercaDelFin && !m_TerminalPedida) {
            m_TerminalPedida = true;
            m_TerminalPedidaT = now;
            m_Runner.HoldRunner();
            BZTRLog.Info(Firma(), "llegando a la terminal (wp " + idx.ToString() + "/" + tot.ToString() + "): HoldRunner para que quede viva en la terminal");
        }
        if (m_TerminalPedida && !held && now - m_TerminalPedidaT > TERMINAL_HOLD_MAX_S) {
            EntrarTerminal(now, "el HOLD de la terminal no engancho en " + BZTRLog.Seg(TERMINAL_HOLD_MAX_S) + " (wp " + idx.ToString() + "/" + tot.ToString() + ")");
        }
    }

    // "" si en esta marca hay que parar; si no, por que no.
    private string NoParaEn(int k, BZTRStop s) {
        if (!s) return "no tiene Stops en el archivo";
        if (!m_Cfg.MarcasActivas()) return "StopMode wave_only (solo para por sena)";
        if (s.Mode == "off") return "Mode off";
        if (s.Mode == "wave_only" && !m_MarcaArmada.Get(k)) return "Mode wave_only y nadie saludo cerca";
        return "";
    }

    private void PedirMarca(int idx) {
        BZTRMarca m;
        BZTRStop s;
        string no;
        int guarda = 0;
        while (m_ProxMarca < m_Marcas.Count() && guarda < 100) {
            guarda = guarda + 1;
            m = m_Marcas.Get(m_ProxMarca);
            if (m_MarcaPedida == m_ProxMarca) {
                if (idx <= m.Wp + MARCA_PASADA_WP) return;
                BZTRLog.Warn(Firma(), "marca " + m.Numero.ToString() + " (wp " + m.Wp.ToString() + "): el HOLD pedido no engancho y el vehiculo ya va por el wp " + idx.ToString() + ": se pasa sin parar");
                m_MarcaPedida = -1;
                m_ProxMarca = m_ProxMarca + 1;
                continue;
            }
            if (idx > m.Wp + MARCA_PASADA_WP) {
                m_ProxMarca = m_ProxMarca + 1;
                continue;
            }
            if (idx < m.Wp - MARGEN_MARCA_WP) return;
            s = m_Cfg.StopDeLaMarca(m.Numero);
            no = NoParaEn(m_ProxMarca, s);
            if (no != "") {
                BZTRLog.Info(Firma(), "marca " + m.Numero.ToString() + " (wp " + m.Wp.ToString() + "): no para · " + no);
                m_ProxMarca = m_ProxMarca + 1;
                continue;
            }
            m_MarcaPedida = m_ProxMarca;
            MedirTodas("antes del HOLD (marca " + m.Numero.ToString() + ")");
            m_Runner.HoldRunner();
            BZTRLog.Info(Firma(), "marca " + m.Numero.ToString() + " (wp " + m.Wp.ToString() + "): HoldRunner en el wp " + idx.ToString() + " · Mode " + s.Mode + " · bajan " + s.WhoGetsOff.Count().ToString());
            return;
        }
    }

    private void ArribarHold(float now, int idx, int tot, bool cercaDelFin) {
        BZTRMarca m;
        BZTRStop s;
        float dRe;
        float dMarca;
        if (m_MarcaPedida >= 0) {
            m = m_Marcas.Get(m_MarcaPedida);
            s = m_Cfg.StopDeLaMarca(m.Numero);
            m_MarcaPedida = -1;
            m_ProxMarca = m_ProxMarca + 1;
            // ★ lejos de la marca (PARADA_LEJOS_M) no se baja nadie ni se encienden senales: sigue
            if (m.Wp >= 0 && m.Wp < m_WpPos.Count()) {
                dMarca = DistanciaXZ(m_Runner.RunnerPos(), m.Donde);
                if (ParadaLejos(dMarca)) {
                    SaltarParadaLejos(now, m, dMarca, idx);
                    return;
                }
            }
            EntrarParada(now, BZTRTipoParada.MARCA, s, "marca " + m.Numero.ToString() + " (wp " + m.Wp.ToString() + ")");
            return;
        }
        if (m_SenaPedida) {
            m_SenaPedida = false;
            EntrarParada(now, BZTRTipoParada.SENA, null, "sena de " + m_SenaDe);
            return;
        }
        if (cercaDelFin || m_TerminalPedida) {
            EntrarTerminal(now, "HOLD en la terminal");
            return;
        }
        // ⛔ re-HOLD en el mismo lugar recien soltado: no se repite la parada (toda espera tiene salida)
        if (m_UltSueltaT > 0 && now - m_UltSueltaT < REHOLD_S) {
            dRe = vector.Distance(m_Runner.RunnerPos(), m_UltSueltaPos);
            if (dRe < REHOLD_M) {
                BZTRLog.Warn(Firma(), "re-HOLD en wp " + idx.ToString() + " a " + BZTRLog.Metros(dRe) + " de donde se solto hace " + BZTRLog.Seg(now - m_UltSueltaT) + ": se suelta sin repetir la parada");
                Soltar(now);
                return;
            }
        }
        if (m_Cfg.Driver.IsTrader == 1) {
            EntrarParada(now, BZTRTipoParada.AJENA, null, "HOLD de AutoDrive en wp " + idx.ToString() + " (fin de leg o isStop)");
            return;
        }
        BZTRLog.Info(Firma(), "HOLD de AutoDrive en wp " + idx.ToString() + "/" + tot.ToString() + " (fin de leg o parada isStop, no es una marca) y el chofer no vende: se suelta");
        Soltar(now);
    }

    // ------------------------------------------------------------------------
    //  LA PARADA
    // ------------------------------------------------------------------------
    static float DistanciaXZ(vector a, vector b) {
        float dx = a[0] - b[0];
        float dz = a[2] - b[2];
        return Math.Sqrt(dx * dx + dz * dz);
    }

    // ★ mas lejos que esto de su marca, la parada no despliega
    static bool ParadaLejos(float dM) {
        return dM > PARADA_LEJOS_M;
    }

    // el wp de la toma mas cercano (XZ) a una posicion; -1 sin toma
    private int WpMasCercano(vector p, out float dM) {
        int i;
        int mejor = -1;
        float d;
        dM = -1;
        for (i = 0; i < m_WpPos.Count(); i++) {
            d = DistanciaXZ(p, m_WpPos.Get(i));
            if (mejor < 0 || d < dM) {
                mejor = i;
                dM = d;
            }
        }
        return mejor;
    }

    // ★ el HOLD llego con el vehiculo lejos de la marca (el cursor de wp desfasado tras una recuperacion
    // de AutoDrive): no se baja nadie, no se encienden senales, se suelta y sigue. Vale igual para el chofer comerciante que
    // vende sentado (es la misma entrada a la parada).
    private void SaltarParadaLejos(float now, BZTRMarca m, float dMarca, int idx) {
        float dCerca;
        int cerca = WpMasCercano(m_Runner.RunnerPos(), dCerca);
        string linea = "PARADA (marca " + m.Numero.ToString() + " (wp " + m.Wp.ToString() + ")): el vehiculo esta a " + BZTRLog.Metros(dMarca) + " de la marca";
        linea = linea + " (cursor wp " + idx.ToString() + ", vehiculo cerca del wp " + cerca.ToString() + " de la toma, a " + BZTRLog.Metros(dCerca) + ")";
        linea = linea + ", tope " + BZTRLog.Metros(PARADA_LEJOS_M) + ": no se baja nadie y sigue";
        BZTRLog.Warn(Firma(), linea);
        m_Runner.ResumeFromHold();
        m_WasHeld = false;
        m_UltSueltaT = now;
        m_UltSueltaPos = m_Runner.RunnerPos();
        m_Estado = BZTREstado.EN_RUTA;
    }

    private BZTRBot BotDeLaPlaza(int seat) {
        int i;
        BZTRBot b;
        for (i = 0; i < m_Bots.Count(); i++) {
            b = m_Bots.Get(i);
            if (b.m_Seat == seat) return b;
        }
        return null;
    }

    private void EntrarParada(float now, int tipo, BZTRStop s, string nombre) {
        Transport tr = Transport.Cast(m_Runner.GetBus());
        vector p = m_Runner.RunnerPos();
        int n = BZTRJugadores.Cerca(p, m_Cfg.PlayerRadiusM, null);
        int i;
        int jugadores = 0;
        string plazas = "";
        string linea;
        m_Estado = BZTREstado.PARADA;
        m_TipoParada = tipo;
        // cada parada por saludo cuenta para el tope de la vuelta
        if (tipo == BZTRTipoParada.SENA) {
            m_SenasVuelta = m_SenasVuelta + 1;
            linea = "parada por saludo " + m_SenasVuelta.ToString() + " de la vuelta " + m_Lanzamientos.ToString();
            if (m_Cfg.Wave.MaxPerLap > 0) linea = linea + " (MaxPerLap " + m_Cfg.Wave.MaxPerLap.ToString() + ")";
            BZTRLog.Info(Firma(), linea);
        }
        m_Stop = s;
        m_ParadaNombre = nombre;
        m_ParadaT0 = now;
        m_FrenteT0 = 0;
        m_FrenteVencido = false;
        m_BocinaTocada = false;
        m_BocinaVista = false;
        m_ParadaFase = 0;
        m_Bajaron = 0;
        m_TodosArribaT = 0;
        m_EmbarqueT0 = 0;
        m_EmbarqueLogT = 0;
        m_EsperaJugIni = n;
        m_EsperaJugMax = n;
        m_Congeladas.Clear();
        if (tr) {
            for (i = 0; i < tr.CrewSize(); i++) {
                if (!tr.CrewMember(i)) continue;
                // solo la tripulacion nuestra (bots). Un jugador sentado que baja en la parada no traba la salida
                if (!eAIBase.Cast(tr.CrewMember(i))) {
                    jugadores = jugadores + 1;
                    continue;
                }
                m_Congeladas.Insert(i);
                plazas = plazas + " " + i.ToString();
            }
        }
        linea = "PARADA (" + nombre + ") · wp " + m_Runner.RunnerWpIdx().ToString() + "/" + m_Runner.RunnerWpTotal().ToString() + " · " + BZTRLog.XZ(p);
        linea = linea + " · plazas ocupadas al congelar:" + plazas + " · jugadores a <= " + BZTRLog.Metros(m_Cfg.PlayerRadiusM) + ": " + n.ToString();
        if (jugadores > 0) linea = linea + " · jugadores sentados: " + jugadores.ToString() + " (no se esperan al seguir)";
        BZTRLog.Info(Firma(), linea);
        MedirTodas("al congelar");
        GruposDePuerta(tr);
    }

    private void TickParada(float now) {
        float kmh;
        string abiertasHold;
        if (!m_Runner.IsHeld()) {
            BZTRLog.Warn(Firma(), "PARADA (" + m_ParadaNombre + "): el HOLD se solto desde afuera tras " + BZTRLog.Seg(now - m_ParadaT0) + ": los que estaban abajo se sientan por teleport");
            ForzarTodos(now);
            abiertasHold = CerrarPuertas(m_Runner.GetBus());   // las que se abrieron para vender no viajan abiertas
            if (abiertasHold != "") BZTRLog.Info(Firma(), "al seguir (el HOLD se solto desde afuera): puerta(s) de plaza abierta(s), cerrada(s):" + abiertasHold);
            ApagarBalizas("el HOLD se solto desde afuera");
            m_Estado = BZTREstado.EN_RUTA;
            m_WasHeld = false;
            return;
        }
        if (m_Balizas.Cuantas() > 0) m_Balizas.Tick(now);
        if (m_ParadaFase == 0) {
            kmh = m_Runner.RunnerSpeedKmh();
            if (kmh < QUIETO_KMH || now - m_ParadaT0 > QUIETO_MAX_S) Desplegar(now, kmh);
            return;
        }
        if (m_ParadaFase == 2) TickSentada(now);
        else TickAbajo(now);
        BocinaVigilar();
    }

    // Para calibrar la distancia de frenado: ya quieto en una parada por saludo, a cuanto quedo el FRENTE del jugador
    // (negativo = lo paso). Lo buscado: unos SENA_ANTES_M.
    private void DondeQuedo(EntityAI bus) {
        vector l;
        float frente = 2.5;
        float delante;
        if (!bus || !m_SenaPb) return;
        if (m_Casco) frente = m_Casco.m_ZMax;
        l = BZTRCasco.ALocal(bus, m_SenaPb.GetPosition());
        delante = l[2] - frente;
        BZTRLog.Info(Firma(), "parada por saludo: el frente quedo a " + BZTRLog.Metros(delante) + " de " + m_SenaDe + " (buscado: " + BZTRLog.Metros(SENA_ANTES_M) + "; negativo = lo paso)");
    }

    // Bajan los que dice la parada. Si no baja nadie, espera sentado.
    private void Desplegar(float now, float kmh) {
        EntityAI bus = m_Runner.GetBus();
        int i;
        int seat;
        int quieren = 0;
        BZTRBot b;
        BZTRStopTrader st;
        float tiempo;
        string no = "";
        string porQueNo;
        ReenviarDatosCerca(m_ParadaNombre);
        if (m_TipoParada == BZTRTipoParada.MARCA && m_Stop) {
            for (i = 0; i < m_Stop.WhoGetsOff.Count(); i++) {
                seat = m_Stop.WhoGetsOff.Get(i);
                // ★ el que maneja baja como los demas si la parada lo dice. AutoDrive 1.2.4 no lo
                // vuelve a sentar durante el HOLD (y la parada no sigue hasta que este sentado de verdad); en una moto no baja
                // (sin nadie arriba se cae) ni con el vehiculo andando
                if (seat == 0) {
                    porQueNo = ChoferNoBaja(bus, kmh);
                    if (porQueNo != "") {
                        no = no + " 0(" + porQueNo + ")";
                        continue;
                    }
                }
                b = BotDeLaPlaza(seat);
                quieren = quieren + 1;
                if (!b) {
                    no = no + " " + seat.ToString() + "(no es comerciante)";
                    continue;
                }
                st = m_Stop.DeLaPlaza(seat);
                tiempo = BZTRDef.TRADER_TIME_S;
                if (st) tiempo = st.TimeS;
                if (BajarUno(now, b, bus, st, tiempo, m_Stop.BoardTimeoutS)) continue;
                no = no + " " + seat.ToString() + "(no estaba sentado)";
                BZTRLog.Warn(Firma(), b.Quien() + ": NO BAJA · " + b.m_NoBajo);
            }
        }
        if (m_TipoParada == BZTRTipoParada.SENA) {
            // ★ por saludo NADIE baja. El vehiculo para donde esta (un risco, una banquina sin lugar
            // para puestos) y los comerciantes venden desde sus asientos; sigue a los Wave.TimeS (se estira con jugadores cerca,
            // como toda espera sentada). Las senales y los puestos son de las marcas. Wave.WhoGetsOff ya no existe.
            BZTRLog.Info(Firma(), "PARADA por saludo de " + m_SenaDe + " · wp " + m_Runner.RunnerWpIdx().ToString() + " · nadie baja: venden sentados · sigue en " + BZTRLog.Seg(m_Cfg.Wave.TimeS));
            DondeQuedo(bus);
        }
        BZTRLog.Info(Firma(), "PARADA (" + m_ParadaNombre + "): quieto (" + Math.Round(kmh).ToString() + " km/h a los " + BZTRLog.Seg(now - m_ParadaT0) + ") · despliegue " + m_Bajaron.ToString() + "/" + quieren.ToString() + no);
        if (m_TipoParada == BZTRTipoParada.SENA) AbrirPuertasParaVender(bus);
        EncenderBalizas(now);
        if (m_Bajaron > 0) {
            m_ParadaFase = 1;
            return;
        }
        m_ParadaFase = 2;
        m_EsperaT0 = now;
        m_EsperaMin = m_Cfg.SeatedWaitS;
        if (m_TipoParada == BZTRTipoParada.SENA) m_EsperaMin = m_Cfg.Wave.TimeS;
        BZTRLog.Info(Firma(), "PARADA (" + m_ParadaNombre + "): nadie baja · espera sentado minimo " + BZTRLog.Seg(m_EsperaMin) + ", tope " + BZTRLog.Seg(m_Cfg.MaxWithPlayersS) + " con jugadores a <= " + BZTRLog.Metros(m_Cfg.PlayerRadiusM));
    }

    // ★ en la parada por saludo nadie baja, pero se abren las puertas de las plazas con comerciante (una vez por puerta,
    // aunque varias plazas la compartan: la puerta es su animacion) para que vendan desde el asiento; al seguir, Soltar las
    // cierra (CerrarPuertas, "al seguir: puerta(s)..."). Se abren con el vehiculo cerrado a jugadores: el cierre
    // FORCEDLOCKED de Expansion niega subir (ActionGetInTransport) y el inventario (IsInventoryVisible lo corta antes de
    // mirar las puertas), y las guardas de BZTRGuardas niegan los asientos igual.
    // En un vehiculo sin puertas de plaza (una moto) no hay nada que abrir.
    private void AbrirPuertasParaVender(EntityAI bus) {
        Transport tr = Transport.Cast(bus);
        int i;
        BZTRBot b;
        bool hay;
        string src;
        string clave;
        string vistas = "";    // las animaciones de puerta ya abiertas en esta parada, entre barras
        string plazas = "";
        string cierre = "";
        int n = 0;
        if (!tr) return;
        for (i = 0; i < m_Bots.Count(); i++) {
            b = m_Bots.Get(i);
            if (!b.SentadoDeVerdad(tr)) continue;
            hay = false;
            src = "";
            ExpansionFSMHelper.DoorAnimationSource(bus, b.m_Seat, hay, src);
            if (!hay || src == "") continue;
            plazas = plazas + " " + b.m_Seat.ToString();
            b.m_CerrarHasta = 0;   // sentado la sostiene cerrada unos segundos: ahora no
            clave = "|" + src + "|";
            if (vistas.IndexOf(clave) >= 0) continue;
            vistas = vistas + clave;
            if (bus.GetAnimationPhase(src) <= 0.5) bus.SetAnimationPhase(src, 1.0);
            n = n + 1;
        }
        if (n == 0) {
            BZTRLog.Info(Firma(), "PARADA (" + m_ParadaNombre + "): sin puertas de plaza que abrir (venden sentados igual)");
            return;
        }
        if (m_CerradoAvisado) cierre = " · con el cierre de Expansion puesto (nadie sube ni toca el inventario)";
        BZTRLog.Info(Firma(), "PARADA (" + m_ParadaNombre + "): puertas abiertas para vender: " + n.ToString() + " (plazas" + plazas + ")" + cierre);
    }

    // ★ "" si el que maneja puede bajar en esta parada; si no, por que (vende sentado). Una moto
    // (1.30: no es CarScript) sin nadie arriba se cae; con el vehiculo andando el HOLD todavia no lo freno.
    private string ChoferNoBaja(EntityAI bus, float kmh) {
        if (!CarScript.Cast(bus)) return "no es un auto: sin nadie arriba se cae, vende sentado";
        if (kmh >= QUIETO_KMH) return "el vehiculo todavia anda: vende sentado";
        return "";
    }

    private bool BajarUno(float now, BZTRBot b, EntityAI bus, BZTRStopTrader st, float tiempo, float boardTimeoutS) {
        string puesto = "sin puesto: vende donde baja";
        string ruta;
        if (!b.Bajar(now, bus, st, tiempo, m_Cfg.MaxWithPlayersS, m_Cfg.PlayerRadiusM, boardTimeoutS, m_Cfg.PlayerHaltM)) return false;
        m_Bajaron = m_Bajaron + 1;
        if (st && st.Waypoints.Count() > 0) {
            ruta = st.AlturaDelTerreno();
            puesto = "puesto:" + ruta + " · " + st.MovementSpeed;
        }
        if (b.m_Chofer) puesto = puesto + " · es el que maneja: el vehiculo espera en el HOLD sin chofer hasta que vuelva";
        BZTRLog.Info(Firma(), b.Quien() + ": BAJA · " + puesto + " · vende minimo " + BZTRLog.Seg(tiempo));
        if (b.m_Pacificado != "") BZTRLog.Warn(Firma(), b.Quien() + ": al bajar tenia blancos de eAI · " + b.m_Pacificado);
        return true;
    }

    private void TickAbajo(float now) {
        Transport tr = Transport.Cast(m_Runner.GetBus());
        EntityAI bus = m_Runner.GetBus();
        int i;
        int perdidos = 0;
        int sentados = 0;
        bool todos = true;
        BZTRBot b;
        string linea;
        string diag;
        if (!tr) return;
        for (i = 0; i < m_Bots.Count(); i++) {
            b = m_Bots.Get(i);
            TurnoDe(b, now);   // antes de su tick, con lo que hicieron los anteriores en este mismo
            linea = b.Tick(now, tr, bus, m_Runner);
            if (linea != "") BZTRLog.Info(Firma(), b.Quien() + ": " + linea);
            diag = b.Diagnostico(now, bus);
            if (diag != "") BZTRLog.Info(Firma(), b.Quien() + " · diag · " + diag);
            if (b.m_Fase == BZTRFase.PERDIDO) {
                perdidos = perdidos + 1;
                if (m_Congeladas.Find(b.m_Seat) >= 0) m_Congeladas.RemoveItem(b.m_Seat);
            }
            if (!b.Terminado()) todos = false;
        }
        if (todos) {
            // nunca sigue con un vivo que no este sentado de verdad (fantasma o a pie); TodosSentados lo
            // vuelve a sentar, y al tope relanza la vuelta
            if (!TodosSentados(now, "PARADA (" + m_ParadaNombre + ")")) return;
            if (PlazasLlenas(tr, sentados)) {
                if (RecienReanudada(now)) return;
                if (!FrenteLibre(now)) return;
                if (!BocinaAntes(now)) return;
                linea = "PARADA (" + m_ParadaNombre + "): embarcados " + m_EmbSentados.ToString() + " de " + m_EmbVivos.ToString() + " vivos · muertos " + m_EmbMuertos.ToString();
                linea = linea + " · las plazas ocupadas al congelar vuelven a estarlo (" + sentados.ToString() + ")";
                linea = linea + " · perdidos " + perdidos.ToString() + " · sigue tras " + BZTRLog.Seg(now - m_ParadaT0);
                BZTRLog.Info(Firma(), linea);
                Soltar(now);
                return;
            }
            if (m_TodosArribaT <= 0) m_TodosArribaT = now;
            if (now - m_TodosArribaT > PLAZAS_GRACIA_S) {
                if (RecienReanudada(now)) return;
                if (!FrenteLibre(now)) return;
                if (!BocinaAntes(now)) return;
                BZTRLog.Warn(Firma(), "PARADA (" + m_ParadaNombre + "): los comerciantes terminaron pero hay plazas vacias (" + sentados.ToString() + "/" + m_Congeladas.Count().ToString() + " ocupadas) tras " + BZTRLog.Seg(PLAZAS_GRACIA_S) + ": sigue igual");
                Soltar(now);
            }
            return;
        }
        if (now - m_ParadaT0 > PARADA_TOPE_S && m_ParadaFase != 3) {
            m_ParadaFase = 3;
            BZTRLog.Warn(Firma(), "PARADA (" + m_ParadaNombre + "): TOPE de " + BZTRLog.Seg(PARADA_TOPE_S) + ": los que siguen abajo se sientan por teleport");
            ForzarTodos(now);
        }
    }

    // las senales de la parada (Stops[].Signals[]): se encienden al desplegar, con o sin comerciantes abajo.
    // En una parada por sena no hay (no tiene Stops).
    private void EncenderBalizas(float now) {
        string linea;
        if (m_TipoParada != BZTRTipoParada.MARCA || !m_Stop) return;
        linea = m_Balizas.Encender(now, m_Stop, Firma(), m_ParadaNombre);
        if (linea != "") BZTRLog.Info(Firma(), "PARADA (" + m_ParadaNombre + "): " + linea);
    }

    // Las borra todas (al seguir, al retirar el vehiculo, al apagar el server). Sin senales no escribe nada.
    private void ApagarBalizas(string cuando) {
        string linea = m_Balizas.Apagar();
        if (linea != "") BZTRLog.Info(Firma(), "PARADA (" + m_ParadaNombre + "): " + linea + " · " + cuando);
    }

    // ¿Las plazas ocupadas al congelar vuelven a estar ocupadas (y nadie esta subiendo)?
    private bool PlazasLlenas(Transport tr, out int ocupadas) {
        int i;
        int seat;
        Human h;
        HumanCommandVehicle vc;
        bool llenas = true;
        ocupadas = 0;
        for (i = 0; i < m_Congeladas.Count(); i++) {
            seat = m_Congeladas.Get(i);
            h = tr.CrewMember(seat);
            if (!h) {
                llenas = false;
                continue;
            }
            vc = h.GetCommand_Vehicle();
            if (vc && vc.IsGettingIn()) {
                llenas = false;
                continue;
            }
            ocupadas = ocupadas + 1;
        }
        return llenas;
    }

    private void ForzarTodos(float now) {
        Transport tr = Transport.Cast(m_Runner.GetBus());
        EntityAI bus = m_Runner.GetBus();
        int i;
        BZTRBot b;
        string linea;
        if (!tr) return;
        for (i = 0; i < m_Bots.Count(); i++) {
            b = m_Bots.Get(i);
            linea = b.Forzar(now, tr, bus, m_Runner);
            if (linea != "") BZTRLog.Info(Firma(), b.Quien() + ": " + linea);
        }
    }

    // Nadie bajo: vende sentado (el chofer comerciante) o solo espera. Minimo, mas tiempo con jugadores, tope.
    private void TickSentada(float now) {
        int n;
        string porque = PorQueSeguir(now, n);
        string linea;
        if (porque == "") return;
        // aunque nadie haya bajado, no sigue con un fantasma adentro
        if (!TodosSentados(now, "PARADA (" + m_ParadaNombre + ")")) return;
        if (RecienReanudada(now)) return;
        if (!FrenteLibre(now)) return;
        if (!BocinaAntes(now)) return;
        linea = "PARADA (" + m_ParadaNombre + "): sigue tras " + BZTRLog.Seg(now - m_EsperaT0) + " sentado · " + porque;
        linea = linea + " · jugadores: al llegar " + m_EsperaJugIni.ToString() + ", maximo " + m_EsperaJugMax.ToString() + ", al seguir " + n.ToString();
        BZTRLog.Info(Firma(), linea);
        Soltar(now);
    }

    // "" = seguir esperando; si no, por que se sigue. Para la parada sentada y la terminal.
    private string PorQueSeguir(float now, out int nAhora) {
        float t = now - m_EsperaT0;
        float tope = m_Cfg.MaxWithPlayersS;
        if (tope < m_EsperaMin) tope = m_EsperaMin;
        nAhora = BZTRJugadores.Cerca(m_Runner.RunnerPos(), m_Cfg.PlayerRadiusM, null);
        if (nAhora > m_EsperaJugMax) m_EsperaJugMax = nAhora;
        if (t < m_EsperaMin) return "";
        if (nAhora == 0) return "sin jugadores a <= " + BZTRLog.Metros(m_Cfg.PlayerRadiusM);
        if (t >= tope) return "TOPE de " + BZTRLog.Seg(tope) + " con " + nAhora.ToString() + " jugador(es) todavia cerca";
        return "";
    }

    // ------------------------------------------------------------------------
    //  EL EMBARQUE: no sale hasta que todos estan sentados, al lanzar y en las paradas (al menos los
    //  que esten vivos). Sentado = sentado DE VERDAD (BZTRBot.SentadoDeVerdad). El que no lo esta se vuelve a sentar
    //  (Reembarcar: el fantasma sin moverlo, el que esta a pie por teleport); al tope, se relanza la vuelta con
    //  tripulacion nueva: nunca se sale con un fantasma. Los muertos (si otro mod los mata) salen de la cuenta.
    // ------------------------------------------------------------------------
    // Cuenta y reintenta. Devuelve las plazas que faltan ("" = ninguna). Con conChofer, tambien la plaza 0 (en una
    // parada puede haber bajado); al lanzar, no (al chofer lo sienta AutoDrive).
    private string Embarque(float now, Transport tr, EntityAI bus, bool conChofer) {
        int i;
        BZTRBot b;
        string faltan = "";
        string linea;
        string porque;
        bool enParada = (m_Estado == BZTREstado.PARADA);
        m_EmbVivos = 0;
        m_EmbSentados = 0;
        m_EmbMuertos = 0;
        m_EmbSinBot = 0;
        for (i = 0; i < m_Bots.Count(); i++) {
            b = m_Bots.Get(i);
            if (b.m_Chofer && !conChofer) continue;
            if (!b.m_Bot) {
                // al lanzar, la ventana de busqueda por clase todavia lo puede encontrar; despues, no hay comerciante
                if (!enParada && now - m_ClaseT0 < CLASE_VENTANA_S) {
                    m_EmbVivos = m_EmbVivos + 1;
                    faltan = faltan + " " + b.m_Seat.ToString() + "(todavia no aparecio)";
                    continue;
                }
                m_EmbSinBot = m_EmbSinBot + 1;
                continue;
            }
            if (!b.m_Bot.IsAlive()) {
                m_EmbMuertos = m_EmbMuertos + 1;
                continue;
            }
            m_EmbVivos = m_EmbVivos + 1;
            if (b.SentadoDeVerdad(tr)) {
                m_EmbSentados = m_EmbSentados + 1;
                if (b.m_Intentos > 0) BZTRLog.Info(Firma(), b.Quien() + ": sentado de verdad tras " + b.m_Intentos.ToString() + " intento(s)");
                b.m_Intentos = 0;
                continue;
            }
            porque = b.PorQueNoSentado(tr);
            faltan = faltan + " " + b.m_Seat.ToString() + "(" + porque + ")";
            if (b.m_SentarPendiente) continue;                 // lo sienta BuscarPorClase en su tick
            if (b.Abajo()) continue;                           // abajo en una parada: su fase lo trae
            if (now - b.m_PorClaseT < REEMBARQUE_S) continue;  // recien sentado por clase: se le da su tiempo
            if (now - b.m_IntentoT < REEMBARQUE_S) continue;
            b.m_IntentoT = now;
            linea = b.Reembarcar(now, tr, bus, m_Runner, enParada);
            if (linea == "") continue;
            b.m_Intentos = b.m_Intentos + 1;
            BZTRLog.Warn(Firma(), b.Quien() + ": no estaba sentado de verdad (" + porque + ") · " + linea + " (intento " + b.m_Intentos.ToString() + ")");
            MedirPlaza(b, tr, "tras volver a sentarlo (intento " + b.m_Intentos.ToString() + ") +0 s");
            b.m_MedirT0 = now;
            b.m_Medidas = 0;
        }
        return faltan;
    }

    private string CuentaEmbarque() {
        string s = "embarcados " + m_EmbSentados.ToString() + " de " + m_EmbVivos.ToString() + " vivos · muertos " + m_EmbMuertos.ToString();
        if (m_EmbSinBot > 0) s = s + " · sin comerciante " + m_EmbSinBot.ToString();
        return s;
    }

    // En una parada: true = todos los vivos sentados de verdad (puede seguir). false = espera; al tope relanza la vuelta.
    private bool TodosSentados(float now, string donde) {
        Transport tr = Transport.Cast(m_Runner.GetBus());
        EntityAI bus = m_Runner.GetBus();
        string faltan;
        float tope = EMBARQUE_PARADA_FACTOR * BZTRDef.BOARD_TIMEOUT_S;
        if (!tr) return true;
        if (m_Stop) tope = EMBARQUE_PARADA_FACTOR * m_Stop.BoardTimeoutS;
        if (tope < EMBARQUE_MIN_S) tope = EMBARQUE_MIN_S;
        faltan = Embarque(now, tr, bus, true);
        if (m_EmbSentados >= m_EmbVivos) {
            if (m_EmbarqueT0 > 0) BZTRLog.Info(Firma(), donde + ": todos sentados de verdad tras esperar " + BZTRLog.Seg(now - m_EmbarqueT0) + " · " + CuentaEmbarque());
            m_EmbarqueT0 = 0;
            m_EmbarquesFallidos = 0;
            return true;
        }
        if (m_EmbarqueT0 <= 0) m_EmbarqueT0 = now;
        if (now - m_EmbarqueT0 > tope) {
            RelanzarVuelta(now, donde + ": no se pudo sentar a" + faltan + " en " + BZTRLog.Seg(now - m_EmbarqueT0) + " · " + CuentaEmbarque());
            return false;
        }
        if (now - m_EmbarqueLogT >= EMBARQUE_LOG_S) {
            m_EmbarqueLogT = now;
            BZTRLog.Info(Firma(), donde + ": no sigue · " + CuentaEmbarque() + " · faltan:" + faltan + " (tope " + BZTRLog.Seg(tope) + ")");
        }
        return false;
    }

    // Al lanzar: retenido en la cabecera hasta que la tripulacion viva este sentada de verdad.
    private void TickEmbarque(float now) {
        Transport tr = Transport.Cast(m_Runner.GetBus());
        EntityAI bus = m_Runner.GetBus();
        string faltan;
        float kmh;
        if (!tr) return;
        kmh = m_Runner.RunnerSpeedKmh();
        if (!m_EmbarqueAvisoMov && kmh > QUIETO_KMH) {
            m_EmbarqueAvisoMov = true;
            BZTRLog.Warn(Firma(), "embarque: el vehiculo se mueve (" + Math.Round(kmh).ToString() + " km/h) con la retencion pedida · IsHeld " + m_Runner.IsHeld().ToString());
        }
        faltan = Embarque(now, tr, bus, false);
        if (faltan == "" && m_EmbSentados >= m_EmbVivos) {
            if (!ChoferSentado(now, tr)) return;
            if (RecienReanudada(now)) return;
            if (!FrenteLibre(now)) return;
            m_Embarcando = false;
            m_EmbarqueT0 = 0;
            m_EmbarquesFallidos = 0;
            BZTRLog.Info(Firma(), "embarque: " + CuentaEmbarque() + " · todos sentados de verdad: SALE tras " + BZTRLog.Seg(now - m_ClaseT0) + " en la cabecera (IsHeld " + m_Runner.IsHeld().ToString() + ")");
            Soltar(now);
            return;
        }
        if (now - m_ClaseT0 > EMBARQUE_LANZAR_TOPE_S) {
            RelanzarVuelta(now, "embarque: no se pudo sentar a" + faltan + " en " + BZTRLog.Seg(now - m_ClaseT0) + " · " + CuentaEmbarque());
            return;
        }
        if (now - m_EmbarqueLogT >= EMBARQUE_LOG_S) {
            m_EmbarqueLogT = now;
            BZTRLog.Info(Firma(), "embarque: retenido en la cabecera · " + CuentaEmbarque() + " · faltan:" + faltan + " (tope " + BZTRLog.Seg(EMBARQUE_LANZAR_TOPE_S) + ")");
        }
    }

    // ★ En la cabecera, al que maneja lo sienta AutoDrive: sube ANIMADO por su puerta (la abre) y, si no termina, por
    // teleport (AutoDrive no maneja hasta que este sentado; su subida avanza con el HOLD puesto). Salir antes cierra la
    // puerta con el subiendo: la animacion no termina y la puerta queda abierta. Se espera a que este sentado de verdad,
    // como mucho CHOFER_ESPERA_S desde el lanzamiento. true = puede seguir.
    private bool ChoferSentado(float now, Transport tr) {
        eAIBase ch = m_Runner.GetDriver();
        HumanCommandVehicle vc;
        string falta = "";
        if (!ch || !ch.IsAlive()) return true;
        if (ch.GetParent() != tr) falta = "todavia no esta adentro";
        if (falta == "") {
            vc = ch.GetCommand_Vehicle();
            if (!vc) falta = "sin el comando de vehiculo";
            else if (vc.IsGettingIn()) falta = "subiendo (animado, con su puerta abierta)";
        }
        if (falta == "") {
            if (m_ChoferAviso) BZTRLog.Info(Firma(), "embarque: el que maneja ya esta sentado, a los " + BZTRLog.Seg(now - m_ClaseT0) + " del lanzamiento");
            m_ChoferAviso = false;
            return true;
        }
        if (now - m_ClaseT0 >= CHOFER_ESPERA_S) {
            BZTRLog.Warn(Firma(), "embarque: el que maneja sigue " + falta + " a los " + BZTRLog.Seg(now - m_ClaseT0) + ": sale igual (AutoDrive lo termina de sentar)");
            m_ChoferAviso = false;
            return true;
        }
        if (!m_ChoferAviso) {
            m_ChoferAviso = true;
            BZTRLog.Info(Firma(), "embarque: los comerciantes sentados; el que maneja " + falta + ": se espera a que AutoDrive lo siente (tope " + BZTRLog.Seg(CHOFER_ESPERA_S) + " desde el lanzamiento)");
        }
        return false;
    }

    // El tope del embarque: retiro + lanzamiento con tripulacion nueva. Tras MAX_EMBARQUES_FALLIDOS seguidos, FIN.
    private void RelanzarVuelta(float now, string motivo) {
        m_Embarcando = false;
        m_EmbarqueT0 = 0;
        m_EmbarquesFallidos = m_EmbarquesFallidos + 1;
        BZTRLog.Err(Firma(), motivo + ": se relanza la vuelta (" + m_EmbarquesFallidos.ToString() + " de " + MAX_EMBARQUES_FALLIDOS.ToString() + " seguidas)");
        QuitarMarcador();
        Retirar();
        if (m_EmbarquesFallidos >= MAX_EMBARQUES_FALLIDOS) {
            Terminar(MAX_EMBARQUES_FALLIDOS.ToString() + " vueltas seguidas sin poder sentar a la tripulacion: revisar las lineas de plaza del RPT", true);
            return;
        }
        Programar(now, RELANZAR_TRAS_RETIRO_S, "vuelta relanzada con tripulacion nueva");
    }

    // ★ Antes de seguir (de una parada o de la cabecera): con un jugador EN EL CAMINO de adelante, espera a que se corra,
    // como mucho FrontPlayerWaitS (0 = no espera); despues sale igual. Arrancar contra un jugador parado enfrente lo
    // empuja, y AutoDrive lo toma como un choque (reversa y re-ubicacion). Vencido el tope, sigue vencido hasta el
    // Soltar: si la bocina demora la salida, la espera no vuelve a empezar (sonaria otra vez y no saldria nunca).
    // true = puede seguir.
    private bool FrenteLibre(float now) {
        EntityAI bus;
        string quien;
        string linea;
        float dist;
        if (!m_Runner || m_Cfg.FrontPlayerWaitS <= 0) return true;
        if (m_FrenteVencido) return true;
        bus = m_Runner.GetBus();
        if (!bus) return true;
        quien = JugadorEnfrente(bus, dist);
        if (quien == "") {
            if (m_FrenteT0 > 0) BZTRLog.Info(Firma(), "jugador " + m_FrenteQuien + " se corrio a los " + BZTRLog.Seg(now - m_FrenteT0));
            m_FrenteT0 = 0;
            return true;
        }
        if (m_FrenteT0 <= 0) {
            m_FrenteT0 = now;
            m_FrenteQuien = quien;
            linea = "jugador " + quien + " enfrente a " + BZTRLog.Metros(dist) + ": espera que se corra (tope " + BZTRLog.Seg(m_Cfg.FrontPlayerWaitS) + ")";
            BZTRLog.Info(Firma(), linea);
            return false;
        }
        if (now - m_FrenteT0 < m_Cfg.FrontPlayerWaitS) return false;
        BZTRLog.Info(Firma(), "jugador " + quien + " sigue enfrente: sale igual tras " + BZTRLog.Seg(now - m_FrenteT0));
        m_FrenteVencido = true;
        return true;
    }

    // ¿Esta parada toca bocina al salir? En una marca, su Stops[].HornOnDepart; en una por saludo, Wave.HornOnDepart.
    // Un HOLD de AutoDrive que no es una marca (fin de leg) no tiene opcion: no toca.
    private bool BocinaPedida() {
        if (m_TipoParada == BZTRTipoParada.SENA) return m_Cfg.Wave.HornOnDepart == 1;
        if (m_TipoParada != BZTRTipoParada.MARCA || !m_Stop) return false;
        return m_Stop.HornOnDepart == 1;
    }

    // ★ La bocina al salir: cuando la parada ya decidio seguir (todos sentados, el frente y el minimo tras una pausa),
    // un toque (HornBeep de AutoDrive: SHORT y apagada a los 600 ms, replicada a los clientes) y se suelta
    // BOCINA_ANTES_S despues. Mientras tanto se vuelve a decidir en cada tick: si la parada vuelve a esperar (un
    // comerciante, un jugador enfrente, una pausa), BocinaVigilar la cancela y la proxima vez suena de nuevo.
    // true = ya puede soltar.
    private bool BocinaAntes(float now) {
        CarScript carBocina;
        if (!BocinaPedida()) return true;
        m_BocinaVista = true;
        if (m_BocinaTocada) return now - m_BocinaT >= BOCINA_ANTES_S;
        carBocina = CarScript.Cast(m_Runner.GetBus());
        if (!carBocina) {
            // la moto de AutoDrive no tiene bocina (BZVehiculoMoto no implementa Bocina)
            if (!m_BocinaSinAutoDicho) BZTRLog.Info(Firma(), "PARADA (" + m_ParadaNombre + "): bocina al salir: el vehiculo no es un auto (una moto): AutoDrive no le toca bocina · sale sin bocina (no se repite)");
            m_BocinaSinAutoDicho = true;
            return true;
        }
        m_Runner.HornBeep();
        m_BocinaTocada = true;
        m_BocinaT = now;
        BZTRLog.Info(Firma(), "PARADA (" + m_ParadaNombre + "): bocina al salir · suelta en " + BZTRLog.Seg(BOCINA_ANTES_S));
        return false;
    }

    // Al final del tick de la parada: si la bocina ya sono y en este tick la parada no llego hasta ella (volvio a
    // esperar), se cancela y se vuelve a decidir.
    private void BocinaVigilar() {
        if (m_BocinaTocada && !m_BocinaVista && m_Estado == BZTREstado.PARADA) {
            m_BocinaTocada = false;
            BZTRLog.Info(Firma(), "PARADA (" + m_ParadaNombre + "): bocina al salir: se cancela (la parada volvio a esperar) · se vuelve a decidir");
        }
        m_BocinaVista = false;
    }

    // El jugador mas cercano EN EL CAMINO de adelante: delante del casco, con su ancho + FRENTE_MARGEN_M a cada lado y
    // hasta FRENTE_LARGO_M. "" si no hay; dist = lo que le falta al frente del vehiculo. Sin casco medido, una caja de
    // auto comun.
    private string JugadorEnfrente(EntityAI bus, out float dist) {
        array<Man> cerca = new array<Man>();
        PlayerBase pb;
        vector l;
        float xMin = -1.5;
        float xMax = 1.5;
        float zFrente = 2.5;
        float radio;
        float delante;
        string nombre = "";
        int i;
        dist = -1;
        if (m_Casco) {
            xMin = m_Casco.m_XMin;
            xMax = m_Casco.m_XMax;
            zFrente = m_Casco.m_ZMax;
        }
        radio = zFrente + FRENTE_LARGO_M + xMax - xMin;
        BZTRJugadores.Cerca(bus.GetPosition(), radio, cerca);
        for (i = 0; i < cerca.Count(); i++) {
            pb = PlayerBase.Cast(cerca.Get(i));
            if (!pb) continue;
            l = BZTRCasco.ALocal(bus, pb.GetPosition());
            delante = l[2] - zFrente;
            if (delante <= 0 || delante > FRENTE_LARGO_M) continue;
            if (l[0] < xMin - FRENTE_MARGEN_M || l[0] > xMax + FRENTE_MARGEN_M) continue;
            if (dist >= 0 && delante >= dist) continue;
            dist = delante;
            nombre = "?";
            if (pb.GetIdentity()) nombre = pb.GetIdentity().GetName();
        }
        return nombre;
    }

    private void Soltar(float now) {
        string abiertas = CerrarPuertas(m_Runner.GetBus());
        // ★ despues de una parada en una marca, la espera entre saludos (Wave.CooldownS) corre igual que
        // despues de una parada por saludo: no para otra vez apenas arranca. Las dos se cuentan desde que SALE
        // (la de saludo se contaba desde que paraba: con CooldownS 10 y 60 s de venta, al salir ya no quedaba espera)
        if (m_Estado == BZTREstado.PARADA && (m_TipoParada == BZTRTipoParada.MARCA || m_TipoParada == BZTRTipoParada.SENA)) m_UltSenaT = now;
        // ★ desde CUALQUIER parada, el vehiculo tiene que recorrer Wave.MinGapM antes de parar por un saludo
        if (m_Estado == BZTREstado.PARADA) {
            m_OdoM = 0;
            m_OdoTiene = true;
            m_OdoPrev = m_Runner.RunnerPos();
        }
        ApagarBalizas("al seguir");
        if (abiertas != "") BZTRLog.Info(Firma(), "al seguir: puerta(s) de plaza abierta(s), cerrada(s):" + abiertas);
        m_PuertasHasta = now + PUERTAS_SOSTENER_S;
        m_PuertasAvisadas = false;
        m_Runner.ResumeFromHold();
        m_WasHeld = false;
        m_UltSueltaT = now;
        m_UltSueltaPos = m_Runner.RunnerPos();
        m_SalidaPendiente = true;
        m_FrenteT0 = 0;
        m_FrenteVencido = false;
        m_BocinaTocada = false;
        m_SalidaArmadoT = 0;
        if (ControladorArmado()) m_SalidaArmadoT = now;   // en una parada ya esta armado; en la cabecera, no
        m_Estado = BZTREstado.EN_RUTA;
    }

    // ------------------------------------------------------------------------
    //  EL VEHICULO VA SIEMPRE CERRADO A JUGADORES: ninguno se sienta (ni abre puertas ni toca el inventario). Un jugador
    //  que va PARADO (arriba o adentro, sin plaza: la caja de un camion, el techo, el interior de un bus) viaja igual: el
    //  cierre no lo impide y no se lo baja. Es una limitacion del vehiculo; el admin elige cual usa.
    // ------------------------------------------------------------------------
    // El cierre de Expansion (FORCEDLOCKED): su ActionGetInTransport niega subir a un vehiculo cerrado y el estado del
    // cierre se sincroniza al cliente (m_LockState), asi que el cliente ni ofrece "subir" (ni abrir puertas, ni tocar el
    // inventario del vehiculo). A nuestros bots no les toca: AutoDrive y BZ_Trader los sientan sin acciones. Se mira en
    // cada tick: si alguien lo abre (una llave de admin), se vuelve a cerrar. Sin el cierre (un vehiculo que no es de
    // Expansion, o sin su modulo Vehicles), las guardas de los asientos (BZTRGuardas) igual niegan sentarse.
    private void CerrarAJugadores() {
        EntityAI bus;
        ExpansionVehicle exv;
        if (!m_Runner || m_BotinVeh) return;
        bus = m_Runner.GetBus();
        if (!bus) return;
        BZTRGuardas.AnotarVehiculo(bus);   // los asientos: ningun jugador se sienta
        if (!ExpansionVehicle.Get(exv, bus) || !exv) {
            if (!m_SinCierreAvisado) BZTRLog.Warn(Firma(), "cierre: " + bus.GetType() + " no es un vehiculo de Expansion: no se lo puede cerrar (las guardas del server igual niegan los asientos)");
            m_SinCierreAvisado = true;
            return;
        }
        if (exv.IsLocked() || m_CierreImposible) return;
        exv.SetLockState(ExpansionVehicleLockState.FORCEDLOCKED);
        // el cierre lo implementa Expansion-Vehicles (Core solo lo declara): sin ese modulo no queda puesto
        if (!exv.IsLocked()) {
            m_CierreImposible = true;
            BZTRLog.Warn(Firma(), "cierre: no quedo puesto (falta el modulo Vehicles de Expansion?): las guardas del server igual niegan los asientos");
            return;
        }
        if (!m_CerradoAvisado) BZTRLog.Info(Firma(), "cierre: vehiculo cerrado para jugadores (cierre de Expansion FORCEDLOCKED: el cliente no ofrece subir ni abrir puertas; los comerciantes y el chofer suben igual)");
        else BZTRLog.Warn(Firma(), "cierre: el vehiculo estaba ABIERTO (alguien lo abrio): se lo vuelve a cerrar");
        m_CerradoAvisado = true;
    }

    // las manos vacias de los comerciantes, en cada tick y en cualquier fase (sentados, abajo, volviendo).
    private void ManosVacias() {
        int i;
        BZTRBot b;
        string linea;
        for (i = 0; i < m_Bots.Count(); i++) {
            b = m_Bots.Get(i);
            linea = b.VaciarManos();
            if (linea == "") continue;
            if (linea.IndexOf("VOLVIO") >= 0) BZTRLog.Warn(Firma(), b.Quien() + ": " + linea);
            else BZTRLog.Info(Firma(), b.Quien() + ": " + linea);
        }
    }

    // Botin (el chofer murio): el vehiculo es de quien lo encuentre, se abre y deja de ser de un servicio (sin guardas).
    private void AbrirAJugadores(EntityAI bus, string motivo) {
        ExpansionVehicle exv;
        if (!bus) return;
        BZTRGuardas.QuitarVehiculo(bus);
        if (!ExpansionVehicle.Get(exv, bus) || !exv || !exv.IsLocked()) return;
        exv.SetLockState(ExpansionVehicleLockState.NOLOCK);
        BZTRLog.Info(Firma(), "cierre: vehiculo abierto · " + motivo);
    }

    // ★ AutoDrive ya maneja: el cerebro 1.2 armado (EstaManejando12: Configurar12 termino, "CEREBRO 1.2 AL
    // VOLANTE"), o el 1.1 si el 1.2 no pudo cargar la ruta (RunnerFellBackTo11: maneja igual, sin ese armado).
    // EstaManejando12 es publico en BZBusService (1.2.3 y 1.2.4) pero no esta en la seccion de la API.
    private bool ControladorArmado() {
        if (m_Runner.RunnerFellBackTo11()) return true;
        return m_Runner.EstaManejando12();
    }

    // Diagnostico: soltado y no se movio. No actua (el "que no quede detenido" es del framework): avisa.
    // ★ en la cabecera AutoDrive arma el controlador recien cuando el vehiculo esta suelto (Configurar12 corre en
    // Tick12, despues del corte del HOLD), ~10 s despues, y hasta entonces el vehiculo espera quieto: no es una falla. Los
    // SALIDA_CHEQUEO_S corren desde que esta armado; si no se arma en SALIDA_ARMADO_MAX_S, se avisa eso. En una parada ya
    // esta armado al soltar: nada cambia. No se retiene hasta el armado: con el HOLD puesto, Configurar12 no corre nunca.
    private void ChequearSalida(float now) {
        float d;
        float armadoHace;
        string linea;
        if (!m_SalidaPendiente) return;
        if (m_SalidaArmadoT <= 0) {
            if (ControladorArmado()) {
                m_SalidaArmadoT = now;
                return;
            }
            if (now - m_UltSueltaT < SALIDA_ARMADO_MAX_S) return;
            m_SalidaPendiente = false;
            linea = "soltado hace " + BZTRLog.Seg(now - m_UltSueltaT) + " y AutoDrive todavia no armo el controlador";
            linea = linea + " (EstaManejando12 false; tope " + BZTRLog.Seg(SALIDA_ARMADO_MAX_S) + ")";
            BZTRLog.Warn(Firma(), linea);
            return;
        }
        if (now - m_SalidaArmadoT < SALIDA_CHEQUEO_S) return;
        m_SalidaPendiente = false;
        d = vector.Distance(m_Runner.RunnerPos(), m_UltSueltaPos);
        if (d >= SALIDA_MIN_M) return;
        armadoHace = now - m_SalidaArmadoT;
        linea = "soltado hace " + BZTRLog.Seg(now - m_UltSueltaT) + " (controlador armado hace " + BZTRLog.Seg(armadoHace) + ") y se movio " + BZTRLog.Metros(d);
        linea = linea + " · IsHeld " + m_Runner.IsHeld().ToString();
        linea = linea + " · RunnerRecuperando '" + m_Runner.RunnerRecuperandoQue() + "' · RunnerBloqueado " + m_Runner.RunnerBloqueado().ToString();
        BZTRLog.Warn(Firma(), linea);
    }

    // ------------------------------------------------------------------------
    //  LA SENA (el gesto de saludo): la decide el manager por servicio
    // ------------------------------------------------------------------------
    void Saludo(PlayerBase pb, float now) {
        EntityAI bus;
        vector pj;
        float d;
        float dm;
        float alcance;
        int k;
        int armadas = 0;
        string nombre = "?";
        string porQueMarca;
        string porQueVuelta;
        string pre;
        string linea;
        BZTRMarca m;
        BZTRStop s;
        BZTRSenaVigente sv;
        now = Reloj(now);   // del manager: el reloj del juego
        if (!m_Runner) return;
        bus = m_Runner.GetBus();
        if (!bus || !pb) return;
        if (pb.GetIdentity()) nombre = pb.GetIdentity().GetName();
        pj = pb.GetPosition();
        d = vector.Distance(bus.GetPosition(), pj);
        alcance = SENA_ANOTAR_FACTOR * m_Cfg.Wave.RadiusM;
        // ★ UNA linea por saludo que no termina en parada, con el por que (antes, fuera del alcance, con el
        // vehiculo parado o embarcando no se decia nada)
        pre = "saludo de " + nombre + " ignorado: ";

        // una marca wave_only por delante se ARMA si el saludo es cerca de ella
        if (m_Estado == BZTREstado.EN_RUTA && !m_Embarcando) {
            for (k = m_ProxMarca; k < m_Marcas.Count(); k++) {
                m = m_Marcas.Get(k);
                s = m_Cfg.StopDeLaMarca(m.Numero);
                if (!s || s.Mode != "wave_only" || m_MarcaArmada.Get(k)) continue;
                dm = Math.Sqrt(BZTRJugadores.DistXZ2(pj, m.Donde));
                if (dm > s.WaveRadiusM) continue;
                m_MarcaArmada.Set(k, true);
                armadas = armadas + 1;
                BZTRLog.Info(Firma(), "saludo de " + nombre + " a " + BZTRLog.Metros(dm) + " de la marca " + m.Numero.ToString() + " (wave_only, radio " + BZTRLog.Metros(s.WaveRadiusM) + "): esa marca queda ARMADA");
            }
        }

        if (!m_Cfg.SenaLibre()) {
            // un servicio que no para por saludo: se dice solo con el vehiculo cerca (los lejanos no vienen al caso)
            if (armadas > 0 || d > alcance) return;
            linea = pre + "este servicio no para por saludo fuera de sus marcas (StopMode " + m_Cfg.StopMode;
            linea = linea + " · Wave.Enabled " + m_Cfg.Wave.Enabled.ToString() + ")";
            BZTRLog.Info(Firma(), linea);
            return;
        }
        // lejos del vehiculo ni se anota (antes se anotaba y vencia sola, y llenaba el log)
        if (d > alcance) {
            if (armadas > 0) return;
            linea = pre + "fuera del alcance (" + BZTRLog.Metros(d) + " > 10 × RadiusM = " + BZTRLog.Metros(alcance) + ")";
            BZTRLog.Info(Firma(), linea);
            return;
        }
        if (m_Estado != BZTREstado.EN_RUTA || m_Embarcando) {
            BZTRLog.Info(Firma(), pre + EstadoParaSaludo());
            return;
        }
        // ★ con el tope de la vuelta cumplido no se anota (no cambia hasta la vuelta siguiente)
        porQueVuelta = BZTRSaludoReglas.FaltaPorVuelta(m_SenasVuelta, m_Cfg.Wave.MaxPerLap);
        if (porQueVuelta != "") {
            BZTRLog.Info(Firma(), pre + porQueVuelta);
            return;
        }
        // ★ regla 1 (y la marca cercana de antes): con la proxima parada marcada a menos de max(MinGapM, SENA_MARCA_M)
        // por delante, sobre la ruta, el saludo no hace una parada aparte (la de la marca esta por llegar): se ignora y se dice
        porQueMarca = MarcaActivaCerca(bus.GetPosition());
        if (porQueMarca != "") {
            BZTRLog.Info(Firma(), pre + porQueMarca);
            return;
        }
        sv = SenaDe(pb);
        if (sv) {
            sv.m_T = now;   // otro saludo del mismo jugador renueva la vigencia (una sola sena por jugador)
        } else {
            sv = new BZTRSenaVigente(pb, nombre, now);
            m_Senas.Insert(sv);
            linea = "sena de " + nombre + " anotada a " + BZTRLog.Metros(d) + " del vehiculo · vale hasta que pare, lo pase o se aleje";
            linea = linea + " (tope MemoryS " + BZTRLog.Seg(m_Cfg.Wave.MemoryS) + ") · radio " + BZTRLog.Metros(m_Cfg.Wave.RadiusM);
            BZTRLog.Info(Firma(), linea);
        }
        TickSenas(now);   // en el acto, como antes; y despues en cada tick mientras siga vigente
    }

    // ★ lo recorrido desde que salio de la ultima parada, sumando lo que se mueve en cada tick (el
    // camino, no la linea recta: una ruta que vuelve cerca del mismo punto no la engana). Un salto de mas de ODO_SALTO_M
    // en un tick (AutoDrive reubica el vehiculo) no cuenta.
    private void Odometro() {
        vector p;
        float d;
        if (!m_Runner.RunnerIsActive()) return;
        p = m_Runner.RunnerPos();
        if (m_Estado == BZTREstado.EN_RUTA && !m_Embarcando && m_OdoTiene) {
            d = DistanciaXZ(p, m_OdoPrev);
            if (d <= ODO_SALTO_M) m_OdoM = m_OdoM + d;
        }
        m_OdoPrev = p;
    }

    // la distancia SOBRE LA RUTA (la toma) del cursor del runner al wp dado; -1 si no se puede (sin toma, el
    // cursor fuera de rango o ya pasado)
    private float DistRutaHasta(int wp) {
        int cur = m_Runner.RunnerWpIdx();
        float hasta;
        float desde;
        if (cur < 0 || cur >= m_WpAcum.Count() || wp < cur || wp >= m_WpAcum.Count()) return -1;
        hasta = m_WpAcum.Get(wp);
        desde = m_WpAcum.Get(cur);
        return hasta - desde;
    }

    // por que un saludo no vale con el servicio fuera de la ruta
    private string EstadoParaSaludo() {
        if (m_Embarcando) return "el vehiculo esta embarcando en la cabecera";
        if (m_Estado == BZTREstado.PARADA) return "el vehiculo ya esta parado (" + m_ParadaNombre + ")";
        if (m_Estado == BZTREstado.TERMINAL) return "el vehiculo esta en la terminal";
        return "el servicio esta en " + BZTREstado.Nombre(m_Estado);
    }

    // el jugador esta adelante del vehiculo (en su sentido de marcha, en el plano)
    static bool JugadorAdelante(EntityAI bus, vector pj) {
        vector dir = bus.GetDirection();
        vector rel = pj - bus.GetPosition();
        float producto = dir[0] * rel[0] + dir[2] * rel[2];
        return producto > 0;
    }

    private BZTRSenaVigente SenaDe(PlayerBase pb) {
        int i;
        for (i = 0; i < m_Senas.Count(); i++) {
            if (m_Senas.Get(i).m_Pb == pb) return m_Senas.Get(i);
        }
        return null;
    }

    // Cada sena vigente: las condiciones de siempre con la posicion del jugador EN ESTE TICK. Se cumple -> HoldRunner;
    // termina sin parar (lo paso, se alejo, vencio el tope, el vehiculo paro antes) -> UNA linea con el por que.
    private void TickSenas(float now) {
        int i = 0;
        float d;
        bool adelante;
        string falta;
        string fin;
        EntityAI bus;
        BZTRSenaVigente sv;
        while (i < m_Senas.Count()) {
            sv = m_Senas.Get(i);
            // el vehiculo paro (una marca, la parada de otro saludo, la terminal) antes de llegar: la sena se termina
            if (m_Estado != BZTREstado.EN_RUTA) {
                if (m_Estado == BZTREstado.PARADA) TerminarSena(sv, now, "atendido por la parada en " + m_ParadaNombre);
                else TerminarSena(sv, now, "ignorado: " + EstadoParaSaludo());
                m_Senas.RemoveOrdered(i);
                continue;
            }
            falta = FaltaSena(sv.m_Pb, now, d);
            if (d >= 0 && (sv.m_DMin < 0 || d <= sv.m_DMin)) {
                sv.m_DMin = d;
                sv.m_FaltaEnMin = falta;
            }
            sv.m_Falta = falta;
            if (falta != "" && m_SenaFase == 1) sv.m_FaltaVentana = falta;
            if (falta != "" && m_SenaFase == 2 && sv.m_FaltaCerca == "") sv.m_FaltaCerca = falta;
            if (falta == "") {
                CumplirSena(sv, now, d);
                m_Senas.RemoveOrdered(i);
                continue;
            }
            bus = m_Runner.GetBus();
            adelante = false;
            if (bus && sv.m_Pb) adelante = JugadorAdelante(bus, sv.m_Pb.GetPosition());
            if (adelante) sv.m_Adelante = true;
            fin = FinDeSena(sv, now, d, adelante);
            if (fin != "") {
                TerminarSena(sv, now, fin);
                m_Senas.RemoveOrdered(i);
                continue;
            }
            i = i + 1;
        }
    }

    // Mientras haya un saludo vigente, el manager lo chequea cada 250 ms (no solo en el tick de 1 s).
    void TickSenasRapido(float now) {
        now = Reloj(now);   // del manager: el reloj del juego
        if (!m_Runner || m_Embarcando || m_Senas.Count() == 0) return;
        if (m_Estado != BZTREstado.EN_RUTA) return;
        TickSenas(now);
    }

    // ★ un saludo vale hasta que el vehiculo para por el, LO PASA (estuvo adelante, se acerco a menos de
    // SENA_PASO_FACTOR x RadiusM y ahora quedo atras y fuera del radio), SE ALEJA (mas de 10 x RadiusM) o vence el tope
    // (Wave.MemoryS). "" = sigue vigente; si no, el motivo para la linea del log.
    private string FinDeSena(BZTRSenaVigente sv, float now, float d, bool adelante) {
        float r = m_Cfg.Wave.RadiusM;
        string cerca;
        string linea;
        bool loPaso;
        bool seAlejo;
        bool vencio;
        if (d < 0) return "ignorado: " + sv.m_Falta;
        loPaso = (sv.m_Adelante && !adelante && d > r && sv.m_DMin >= 0 && sv.m_DMin <= SENA_PASO_FACTOR * r);
        seAlejo = (d > SENA_ANOTAR_FACTOR * r);
        vencio = (now - sv.m_T > m_Cfg.Wave.MemoryS);
        if (!loPaso && !seAlejo && !vencio) return "";
        // llego a la ventana de frenado y otra cosa lo freno (la espera, la distancia desde la parada, el tope...): eso
        if (sv.m_FaltaVentana != "") return "ignorado: " + sv.m_FaltaVentana;
        // o ya no alcanzaba a frenar antes del jugador (el saludo se anoto cerca, o el vehiculo venia rapido)
        if (sv.m_FaltaCerca != "") return "ignorado: " + sv.m_FaltaCerca;
        cerca = "la distancia de frenado";
        if (loPaso) return "ignorado: el vehiculo lo paso sin llegar a " + cerca;
        if (seAlejo && !sv.m_Adelante && m_Cfg.Wave.AheadOnly == 1) return "ignorado: no estaba adelante del vehiculo";
        if (seAlejo) return "ignorado: el vehiculo se alejo (a " + BZTRLog.Metros(d) + ") sin llegar a " + cerca;
        linea = "ignorado: vencio (MemoryS " + BZTRLog.Seg(m_Cfg.Wave.MemoryS) + ") sin que el vehiculo llegara a " + cerca;
        linea = linea + " (estaba a " + BZTRLog.Metros(d) + ")";
        return linea;
    }

    // la linea de un saludo que termina sin parar por el (una por saludo)
    private void TerminarSena(BZTRSenaVigente sv, float now, string motivo) {
        string linea = "saludo de " + sv.m_Nombre + " " + motivo;
        if (sv.m_DMin >= 0) linea = linea + " · lo mas cerca " + BZTRLog.Metros(sv.m_DMin);
        linea = linea + " · vigente " + BZTRLog.Seg(now - sv.m_T0);
        BZTRLog.Info(Firma(), linea);
    }

    // "" si la sena se cumple ahora; si no, la primera condicion que falta. d = distancia al vehiculo (-1 si no hay).
    private string FaltaSena(PlayerBase pb, float now, out float d) {
        EntityAI bus;
        vector pv;
        vector pj;
        float kmh;
        float dJ;
        float dFr;
        string regla;
        string linea;
        d = -1;
        m_SenaFase = -1;
        if (m_Estado != BZTREstado.EN_RUTA) return "el servicio estaba en " + BZTREstado.Nombre(m_Estado);
        bus = m_Runner.GetBus();
        if (!bus) return "sin vehiculo";
        if (!pb) return "el jugador ya no esta";
        pv = bus.GetPosition();
        pj = pb.GetPosition();
        d = vector.Distance(pv, pj);
        if (!pb.IsAlive() || pb.IsUnconscious()) return "no esta vivo y consciente";
        if (pb.IsInVehicle()) return "no esta a pie";
        kmh = m_Runner.RunnerSpeedKmh();
        // adelante: la ventana de frenado. Al costado o atras (con AheadOnly 0): el radio de siempre
        if (!JugadorAdelante(bus, pj)) {
            if (m_Cfg.Wave.AheadOnly == 1) return "no estaba adelante del vehiculo";
            if (d > m_Cfg.Wave.RadiusM) return "fuera del radio de " + BZTRLog.Metros(m_Cfg.Wave.RadiusM);
        } else {
            m_SenaFase = FaseFrenado(bus, pj, kmh, dJ, dFr);
            if (m_SenaFase == 0) {
                linea = "todavia lejos para frenar (a " + BZTRLog.Metros(dJ) + "; frena a " + BZTRLog.Metros(dFr + SENA_ANTES_M) + ")";
                return linea;
            }
            if (m_SenaFase == 2) {
                linea = "no alcanza a frenar antes del jugador (a " + BZTRLog.Metros(dJ) + ", necesita " + BZTRLog.Metros(dFr) + ")";
                return linea;
            }
        }
        if (kmh < SENA_ANDANDO_KMH) return "el vehiculo no esta andando (" + Math.Round(kmh).ToString() + " km/h)";
        if (m_UltSenaT > 0 && now - m_UltSenaT < m_Cfg.Wave.CooldownS) return "espera entre saludos (faltan " + BZTRLog.Seg(m_Cfg.Wave.CooldownS - (now - m_UltSenaT)) + ")";
        // lo recorrido desde la ultima parada
        regla = BZTRSaludoReglas.FaltaTrasParada(m_OdoTiene, m_OdoM, m_Cfg.Wave.MinGapM);
        if (regla != "") return regla;
        if (m_SenaPedida) return "ya hay una sena pedida";
        if (m_MarcaPedida >= 0) return "ya va a parar en una marca";
        if (m_TerminalPedida) return "ya llega a la terminal";
        // el tope de la vuelta
        regla = BZTRSaludoReglas.FaltaPorVuelta(m_SenasVuelta, m_Cfg.Wave.MaxPerLap);
        if (regla != "") return regla;
        return MarcaActivaCerca(pv);
    }

    // ★ La parada por saludo empieza con la distancia de frenado. dJ = del FRENTE del vehiculo al jugador (sobre la ruta,
    // o en linea recta); dFr = lo que necesita para parar: v^2/(2 SENA_FRENADA_MS2) + v SENA_LATENCIA_S. 0 = todavia
    // lejos (dJ > dFr + SENA_ANTES_M), 1 = frena ahora (queda entre 0 y SENA_ANTES_M antes), 2 = ya no alcanza (dJ < dFr).
    private int FaseFrenado(EntityAI bus, vector pj, float kmh, out float dJ, out float dFr) {
        float frente = 2.5;
        if (m_Casco) frente = m_Casco.m_ZMax;
        dJ = DistAlJugador(bus, pj) - frente;
        dFr = BZTRSaludoReglas.DistFrenado(kmh, SENA_FRENADA_MS2, SENA_LATENCIA_S);
        return BZTRSaludoReglas.FaseDeFrenado(dJ, dFr, SENA_ANTES_M);
    }

    // Del vehiculo al jugador SOBRE LA RUTA: el wp de la toma mas cerca del vehiculo (cerca del cursor) y el mas cerca del
    // jugador hacia adelante, si el jugador esta a menos de SENA_RUTA_LATERAL_M de la ruta. Si no, en linea recta (XZ).
    private float DistAlJugador(EntityAI bus, vector pj) {
        vector pv = bus.GetPosition();
        float recta = DistanciaXZ(pv, pj);
        float tope = SENA_ANOTAR_FACTOR * m_Cfg.Wave.RadiusM + 50;
        float dv = -1;
        float dj = -1;
        float d;
        float base;
        float hasta;
        int n = m_WpPos.Count();
        int cur = m_Runner.RunnerWpIdx();
        int wv = -1;
        int wj = -1;
        int i;
        if (cur < 0 || cur >= n || m_WpAcum.Count() != n) return recta;
        for (i = cur - 15; i <= cur + 15; i++) {
            if (i < 0 || i >= n) continue;
            d = DistanciaXZ(pv, m_WpPos.Get(i));
            if (wv < 0 || d < dv) {
                wv = i;
                dv = d;
            }
        }
        if (wv < 0) return recta;
        base = m_WpAcum.Get(wv);
        for (i = wv; i < n; i++) {
            hasta = m_WpAcum.Get(i);
            if (hasta - base > tope) break;
            d = DistanciaXZ(pj, m_WpPos.Get(i));
            if (wj < 0 || d < dj) {
                wj = i;
                dj = d;
            }
        }
        if (wj < 0 || dj > SENA_RUTA_LATERAL_M) return recta;
        hasta = m_WpAcum.Get(wj);
        return hasta - base;
    }

    private void CumplirSena(BZTRSenaVigente sv, float now, float d) {
        float kmh = m_Runner.RunnerSpeedKmh();
        string linea;
        m_SenaPedida = true;
        m_SenaPedidaT = now;
        m_UltSenaT = now;
        m_SenaDe = sv.m_Nombre;
        m_SenaPb = sv.m_Pb;
        MedirTodas("antes del HOLD (sena)");
        m_Runner.HoldRunner();
        linea = "SENA de " + sv.m_Nombre + " a " + BZTRLog.Metros(d) + " · adelante · parada por saludo en wp " + m_Runner.RunnerWpIdx().ToString();
        linea = linea + " (" + Math.Round(kmh).ToString() + " km/h, frena con " + BZTRLog.Metros(BZTRSaludoReglas.DistFrenado(kmh, SENA_FRENADA_MS2, SENA_LATENCIA_S)) + " de frenado)";
        linea = linea + " · a los " + BZTRLog.Seg(now - sv.m_T0) + " del saludo · HoldRunner";
        BZTRLog.Info(Firma(), linea);
    }

    // "" o por que no: una marca que va a parar, a menos de SENA_MARCA_M por delante.
    private string MarcaActivaCerca(vector pv) {
        int k;
        float d;
        BZTRMarca m;
        BZTRStop s;
        for (k = m_ProxMarca; k < m_Marcas.Count(); k++) {
            m = m_Marcas.Get(k);
            s = m_Cfg.StopDeLaMarca(m.Numero);
            if (NoParaEn(k, s) != "") continue;
            // sobre la ruta (la toma); sin toma o con el cursor fuera de rango, en linea recta
            d = DistRutaHasta(m.Wp);
            if (d < 0) d = Math.Sqrt(BZTRJugadores.DistXZ2(pv, m.Donde));
            return BZTRSaludoReglas.FaltaPorMarca(m.Numero, d, m_Cfg.Wave.MinGapM, SENA_MARCA_M);
        }
        return "";
    }

    // ------------------------------------------------------------------------
    //  LA TERMINAL: OnRouteEnd (decision del admin)
    // ------------------------------------------------------------------------
    private void EntrarTerminal(float now, string motivo) {
        vector p = m_Runner.RunnerPos();
        int n = BZTRJugadores.Cerca(p, m_Cfg.PlayerRadiusM, null);
        string linea;
        m_Estado = BZTREstado.TERMINAL;
        m_EsperaT0 = now;
        m_EsperaMin = m_Cfg.OnRouteEnd.WaitS;
        m_EsperaJugIni = n;
        m_EsperaJugMax = n;
        linea = "TERMINAL (" + motivo + ") · wp " + m_Runner.RunnerWpIdx().ToString() + "/" + m_Runner.RunnerWpTotal().ToString() + " · " + BZTRLog.XZ(p);
        linea = linea + " · OnRouteEnd " + m_Cfg.OnRouteEnd.Action + " (DelayS " + m_Cfg.OnRouteEnd.DelayS.ToString() + ", WaitS " + m_Cfg.OnRouteEnd.WaitS.ToString() + ")";
        BZTRLog.Info(Firma(), linea);
    }

    private void TickTerminal(float now) {
        int n;
        string porque = "";
        float demora;
        if (m_EsperaMin > 0) {
            porque = PorQueSeguir(now, n);
            if (porque == "") return;
        }
        QuitarMarcador();
        if (m_Cfg.OnRouteEnd.Action == "despawn") {
            BZTRLog.Info(Firma(), "terminal: fin de la vuelta " + porque + " · OnRouteEnd despawn: UnregisterRunner y no se relanza");
            Retirar();
            Terminar("OnRouteEnd despawn", false);
            return;
        }
        demora = RELANZAR_TRAS_RETIRO_S;
        if (m_Cfg.OnRouteEnd.Action == "respawn" && m_Cfg.OnRouteEnd.DelayS > demora) demora = m_Cfg.OnRouteEnd.DelayS;
        BZTRLog.Info(Firma(), "terminal: fin de la vuelta " + porque + " · OnRouteEnd " + m_Cfg.OnRouteEnd.Action + ": UnregisterRunner y se relanza " + m_Cfg.Route);
        Retirar();
        Programar(now, demora, "OnRouteEnd " + m_Cfg.OnRouteEnd.Action);
    }

    // ------------------------------------------------------------------------
    //  MUERTE, BOTIN Y REAPARICION (red de seguridad: los comerciantes son invencibles)
    // ------------------------------------------------------------------------
    private string MotivoMuerte() {
        eAIBase d = m_Runner.GetDriver();
        if (d) {
            m_SinChofer = 0;
            if (!d.IsAlive()) return "el chofer murio";
            return "";
        }
        if (!m_Runner.RunnerIsActive()) return "";
        m_SinChofer = m_SinChofer + 1;
        if (m_SinChofer >= SIN_CHOFER_TICKS) return "el chofer ya no existe y el vehiculo sigue en el mundo";
        return "";
    }

    private void Muerte(float now, string motivo) {
        eAIBase d = m_Runner.GetDriver();
        EntityAI v = m_Runner.GetBus();
        string pc = "?";
        string pv = "sin vehiculo";
        string fin = m_Runner.RunnerEndReason();
        string linea;
        m_Estado = BZTREstado.MUERTO;
        m_MuerteT = now;
        m_BotinCuerpo = d;
        m_BotinVeh = v;
        m_FinLogueado = false;
        m_SoltadoLogueado = false;
        QuitarMarcador();
        AbrirAJugadores(v, "el chofer murio: el vehiculo queda como botin");
        if (d) pc = BZTRLog.XZ(d.GetPosition());
        if (v) pv = v.GetType() + " en " + BZTRLog.XZ(v.GetPosition());
        if (fin != "") m_FinLogueado = true;
        linea = "MUERTE: " + motivo + " · cuerpo en " + pc + " · vehiculo " + pv;
        linea = linea + " · RunnerEndReason '" + fin + "' · reaparece en " + m_Cfg.RespawnSeconds.ToString() + " s (< 0 = nunca)";
        BZTRLog.Info(Firma(), linea);
    }

    private void TickMuerto(float now) {
        float desde = now - m_MuerteT;
        float espera;
        bool soltado;
        string fin = m_Runner.RunnerEndReason();
        if (!m_FinLogueado && fin != "") {
            m_FinLogueado = true;
            BZTRLog.Info(Firma(), "RunnerEndReason tras la muerte: '" + fin + "'");
        }
        soltado = !m_Runner.RunnerIsActive();
        if (soltado && !m_SoltadoLogueado) {
            m_SoltadoLogueado = true;
            if (m_BotinVeh) BZTRLog.Info(Firma(), "AutoDrive solto el vehiculo como botin tras " + BZTRLog.Seg(desde) + ": " + m_BotinVeh.GetType() + " en " + BZTRLog.XZ(m_BotinVeh.GetPosition()));
            else BZTRLog.Info(Firma(), "el runner ya no tiene vehiculo tras " + BZTRLog.Seg(desde));
        }
        if (!soltado && desde < SOLTAR_MAX_S) return;
        if (!soltado && !m_SoltadoLogueado) {
            m_SoltadoLogueado = true;
            BZTRLog.Warn(Firma(), "AutoDrive NO solto el vehiculo en " + BZTRLog.Seg(SOLTAR_MAX_S) + ": el UnregisterRunner de la reaparicion lo va a borrar");
        }
        if (m_Cfg.RespawnSeconds < 0) {
            Terminar("RespawnSeconds < 0: no reaparece; el runner queda registrado y el cuerpo y el vehiculo quedan en el mundo", true);
            return;
        }
        espera = m_Cfg.RespawnSeconds;
        if (espera < 1) espera = 1;
        if (desde < espera) return;
        BZTRLog.Info(Firma(), "reaparicion: pasaron " + BZTRLog.Seg(desde) + " desde la muerte (RespawnSeconds " + m_Cfg.RespawnSeconds.ToString() + ") -> UnregisterRunner y ruta nueva");
        Retirar();
        m_VerificarBotin = true;
        m_VerificarT = now + VERIFICAR_BOTIN_S;
        Programar(now, RELANZAR_TRAS_RETIRO_S, "reaparicion tras la muerte");
    }

    private void VerificarBotin() {
        string sv = "NO";
        string sc = "NO";
        m_VerificarBotin = false;
        if (m_BotinVeh && !m_BotinVeh.IsSetForDeletion()) sv = "SI (" + m_BotinVeh.GetType() + " en " + BZTRLog.XZ(m_BotinVeh.GetPosition()) + ")";
        if (m_BotinCuerpo && !m_BotinCuerpo.IsSetForDeletion()) sc = "SI (en " + BZTRLog.XZ(m_BotinCuerpo.GetPosition()) + ")";
        BZTRLog.Info(Firma(), "botin despues del UnregisterRunner: el vehiculo sigue existiendo: " + sv + " · el cuerpo del chofer sigue existiendo: " + sc);
        m_BotinVeh = null;
        m_BotinCuerpo = null;
    }

    private void VehiculoPerdido(float now) {
        BZTRLog.Warn(Firma(), "VEHICULO PERDIDO: hace " + m_SinVeh.ToString() + " ticks que no esta en el mundo (no es la cabecera ni una ruta cerrada) · ultima pos " + BZTRLog.XZ(m_UltPos) + " · wp " + m_Runner.RunnerWpIdx().ToString() + "/" + m_Runner.RunnerWpTotal().ToString());
        QuitarMarcador();
        Retirar();
        Programar(now, REINTENTO_S, "vehiculo perdido");
    }

    // ------------------------------------------------------------------------
    //  MARCADOR DE MAPA (posicion del VEHICULO). Un solo uid por servicio, creado UNA vez con CreateServerMarker (el alta
    //  la manda Navigation a todos). En marcha, cada MarkerUpdateSeconds, si se movio MARCA_QUIETO_M o mas, se cambia la
    //  POSICION del dato que vive en los settings del mapa del server y el servicio queda SUCIO: el manager manda los
    //  settings del mapa a cada jugador UNA vez por tick (ExpansionMapSettings.Send), para todos los servicios juntos.
    //  Leido en el cliente de Navigation (1.9.79): al recibir los settings del mapa, un uid que ya tiene se actualiza
    //  EN EL LUGAR (ExpansionMapSettings.OnRecieve) y el widget del mapa lee la posicion del dato en cada cuadro
    //  (ExpansionMapMarker.Update): el icono salta a la posicion nueva sin duplicarse ni apagarse. Un alta con un uid
    //  repetido el cliente la RECHAZA y una baja borra el dato en el acto: por eso borrar y crear dejaba un fantasma o
    //  un hueco segun el orden, y no se podia alinear con el refresco de 1 s del cliente (es por cliente).
    //  En una parada el marcador parpadea cambiando la VISIBILIDAD del mismo dato (viaja en el mismo envio): sin altas
    //  ni bajas. Cada envio lleva TODOS los marcadores de servidor del mapa (~100 bytes cada uno) mas los campos del
    //  mapa (~40): el manager dice cuantos hay al arrancar, para elegir la cadencia.
    // ------------------------------------------------------------------------
    static bool s_MarcaSucia;   // algun servicio cambio su marcador en este tick: el manager manda los settings del mapa
    // la visibilidad del marcador, con los valores de Expansion (ExpansionMarkerData: NONE 0, WORLD 2, MAP 4; prendido =
    // WORLD | MAP, lo que pone el constructor). Constantes propias: una global de otro mod no se valida de este lado.
    static const int MARCA_VIS_APAGADO  = 0;
    static const int MARCA_VIS_PRENDIDO = 6;

    private int VisibilidadPrendida() {
        return MARCA_VIS_PRENDIDO;
    }

    // el alta, una sola vez por servicio (y de nuevo si el dato ya no esta)
    private bool CrearMarcador(float now, BZTRMarkers mk, ExpansionMarkerModule mod, vector posMarca) {
        ExpansionMarkerData creado;
        string lineaMk;
        bool en3D = false;
        if (mk.Expansion3DMarkers == 1) en3D = true;
        mod.RemoveServerMarker(m_MarcaUid);   // un uid que haya quedado de un cierre sucio (si no esta, no manda nada)
        creado = mod.CreateServerMarker(mk.MarkerLabel, mk.MarkersIcon, posMarca, mk.Color(), en3D, m_MarcaUid);
        m_MarcaT = now;
        if (!creado) {
            if (!m_MarcaAvisada) BZTRLog.Err(Firma(), "marcador: CreateServerMarker devolvio null (sin settings del mapa de Expansion)");
            m_MarcaAvisada = true;
            return false;
        }
        m_MarcaDato = creado;
        m_MarcaPos = posMarca;
        m_MarcaVisible = true;
        m_MarcaOculta = false;
        lineaMk = "marcador creado: '" + mk.MarkerLabel + "' (" + mk.MarkersIcon + ", uid " + m_MarcaUid + ")";
        lineaMk = lineaMk + " · color " + mk.Red + " " + mk.Green + " " + mk.Blue;
        lineaMk = lineaMk + " en " + BZTRLog.XZ(posMarca);
        lineaMk = lineaMk + " · se mueve cada " + mk.MarkerUpdateSeconds.ToString() + " s";
        lineaMk = lineaMk + " si se movio " + BZTRLog.Metros(BZTRDef.MARCA_QUIETO_M) + " o mas";
        BZTRLog.Info(Firma(), lineaMk);
        return true;
    }

    // la posicion nueva sobre el MISMO dato; el envio lo hace el manager
    private void MoverMarcador(float now, vector posMarca) {
        m_MarcaDato.SetPosition(posMarca);
        m_MarcaPos = posMarca;
        m_MarcaT = now;
        m_MarcaEnvios = m_MarcaEnvios + 1;
        m_MarcaK = BZTRManager.MarcadoresEnElMapa();
        s_MarcaSucia = true;
    }

    private void ActualizarMarcador(float now, bool forzar) {
        BZTRMarkers mk = m_Cfg.Markers;
        ExpansionMarkerModule mod;
        EntityAI v;
        vector posMarca;
        if (!mk || mk.useMapMarker != 1) return;
        v = m_Runner.GetBus();
        if (!v) return;
        posMarca = v.GetPosition();
        // el alta, una sola vez (o de nuevo si el dato ya no esta: alguien lo saco de los settings del mapa)
        if (!m_MarcaVisible || !m_MarcaDato) {
            mod = ExpansionMarkerModule.GetModuleInstance();
            if (!mod) {
                if (!m_MarcaAvisada) BZTRLog.Err(Firma(), "marcador: ExpansionMarkerModule no esta cargado (Navigation): sin marcador");
                m_MarcaAvisada = true;
                return;
            }
            CrearMarcador(now, mk, mod, posMarca);
            return;
        }
        // en una parada parpadea; al salir de la parada, visible y fijo otra vez en la posicion actual (forzado)
        if (MarcaParpadea(mk)) {
            Parpadear(now, mk, posMarca);
            return;
        }
        if (m_Parpadeando) {
            TerminarParpadeo(now);
            forzar = true;
        }
        if (!forzar && now - m_MarcaT < mk.MarkerUpdateSeconds) return;
        if (!forzar && DistanciaXZ(posMarca, m_MarcaPos) < BZTRDef.MARCA_QUIETO_M) {
            m_MarcaT = now;   // parado: no se manda nada; se vuelve a mirar en MarkerUpdateSeconds
            return;
        }
        MoverMarcador(now, posMarca);
    }

    // ★ en PARADA (con despliegue, sentada o por saludo) el marcador se apaga y se prende cada BlinkSeconds, como senal
    // de venta. En marcha, en la cabecera (EN_RUTA embarcando) y en la terminal (TERMINAL), fijo. Apagar y prender es la
    // VISIBILIDAD del mismo dato: el cliente la aplica en el cuadro siguiente al envio, sin depender de su refresco.
    private bool MarcaParpadea(BZTRMarkers mk) {
        if (m_Estado != BZTREstado.PARADA) return false;
        if (mk.BlinkAtStops != 1) return false;
        return mk.BlinkSeconds > 0;
    }

    private void Parpadear(float now, BZTRMarkers mk, vector posMarca) {
        string lineaPp;
        if (!m_Parpadeando) {
            m_Parpadeando = true;
            m_ParpadeoT = now;
            m_ParpadeoT0 = now;
            m_ParpadeoCambios = 0;
            MoverMarcador(now, posMarca);   // la posicion exacta de la parada (pudo frenar unos metros despues del ultimo envio)
            lineaPp = "marcador: parpadea en la parada (" + m_ParadaNombre + ")";
            lineaPp = lineaPp + " · cambia cada " + mk.BlinkSeconds.ToString() + " s (la visibilidad del mismo marcador)";
            BZTRLog.Info(Firma(), lineaPp);
            return;   // la primera fase, visible (como estaba)
        }
        // el tick es de 1 s: un margen para que 1 s cambie en cada tick y no cada dos
        if (now - m_ParpadeoT < mk.BlinkSeconds - 0.1) return;
        m_ParpadeoT = now;
        if (m_MarcaOculta) m_MarcaDato.ApplyVisibility(VisibilidadPrendida());
        else m_MarcaDato.ApplyVisibility(MARCA_VIS_APAGADO);
        m_MarcaOculta = !m_MarcaOculta;
        m_ParpadeoCambios = m_ParpadeoCambios + 1;
        m_MarcaEnvios = m_MarcaEnvios + 1;
        m_MarcaK = BZTRManager.MarcadoresEnElMapa();
        s_MarcaSucia = true;
    }

    private void TerminarParpadeo(float now) {
        string lineaPp;
        m_Parpadeando = false;
        if (m_MarcaDato && m_MarcaOculta) {
            m_MarcaDato.ApplyVisibility(VisibilidadPrendida());
            s_MarcaSucia = true;
        }
        m_MarcaOculta = false;
        lineaPp = "marcador: fin del parpadeo (" + m_ParpadeoCambios.ToString() + " cambios en " + BZTRLog.Seg(now - m_ParpadeoT0) + ")";
        lineaPp = lineaPp + " · visible y fijo otra vez";
        BZTRLog.Info(Firma(), lineaPp);
    }

    // el resumen del marcador por vuelta (en marcha no se escribe nada por envio). K es el del ultimo envio: el resumen se
    // escribe al relanzar, cuando el marcador propio ya se dio de baja, y contar recien ahi daba uno menos.
    private void ResumirMarcador() {
        int kMk = m_MarcaK;
        int kbMk = 0;
        string lineaRm;
        if (kMk >= 0) kbMk = (BZTRManager.BytesPorEnvio(kMk) * m_MarcaEnvios) / 1024;
        lineaRm = "marcador: " + m_MarcaEnvios.ToString() + " envios en la vuelta";
        if (kMk >= 0) lineaRm = lineaRm + " (" + kMk.ToString() + " marcadores en el mapa, ~" + kbMk.ToString() + " KB por jugador)";
        BZTRLog.Info(Firma(), lineaRm);
        m_MarcaEnvios = 0;
    }

    private void QuitarMarcador() {
        ExpansionMarkerModule mod;
        m_Parpadeando = false;
        m_MarcaOculta = false;
        if (!m_MarcaVisible) return;
        m_MarcaDato = null;   // RemoveServerMarker borra el dato en el server
        mod = ExpansionMarkerModule.GetModuleInstance();
        if (mod) mod.RemoveServerMarker(m_MarcaUid);   // una sola baja (Navigation la manda a todos)
        m_MarcaVisible = false;
        BZTRLog.Info(Firma(), "marcador quitado");
    }

    // ------------------------------------------------------------------------
    //  APAGADO DEL SERVER: el vehiculo de AutoDrive PERSISTE; se retira para que no vuelva vacio
    // ------------------------------------------------------------------------
    void Apagar() {
        int nSen = m_Balizas.Cuantas();
        QuitarMarcador();
        ApagarBalizas("cierre del server");
        if (nSen > 0) BZTRLog.Info(Firma(), "cierre del server: " + nSen.ToString() + " señales borradas");
        if (m_Runner) {
            BZTRLog.Info(Firma(), "apagado del server: se retira el runner (estado " + BZTREstado.Nombre(m_Estado) + ")");
            Retirar();
        }
        m_Estado = BZTREstado.FIN;
    }
}
