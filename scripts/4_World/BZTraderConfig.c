// ============================================================================
//  CONFIGURACION: un archivo por SERVICIO en $profile:BZ_Trader/*.json
//
//  Es exactamente el archivo que exporta el editor (tools/editor,
//  funcion exportar()): claves en ingles, y el archivo ES la sesion del editor. Donde el
//  campo es el mismo que en una ruta de TTE (TraderFile, LootingBehaviour,
//  Waypoints{WayPointName, Position}, MovementSpeed) se usa el mismo nombre y formato. Invincible y CanBeLooted de TTE
//  no: en BZ_Trader los comerciantes son siempre invencibles.
//
//  Opcionales que el editor todavia no escribe (defaults seguros si faltan):
//  StartDelayS, Markers{...}, RespawnSeconds, PlayerRadiusM, MaxWithPlayersS, SeatedWaitS,
//  PlayerHaltM, MaxStallDistanceM, OnRouteEnd.WaitS y Traders[].LootingBehaviour.
//  Stops[].Signals[] es opcional: sin ella la parada no enciende nada. Stops[].HornOnDepart y Wave.HornOnDepart
//  tambien (sin ellas, no hay bocina al salir).
//
//  ⚠ Los defaults se deciden por PRESENCIA de la clave (ver BZTRJson): JsonFileLoader deja
//  en cero lo que falta, y aca el 0 es un valor valido en varios campos.
// ============================================================================

class BZTRDef {
    static const string LOOTING            = "BANDAGES | FOOD | CLOTHING | UPGRADE";   // el mismo default que TTE
    static const int    RESPAWN_S          = 600;
    static const float  START_DELAY_S      = 0;
    static const float  WAVE_RADIUS_M      = 15;
    static const float  WAVE_COOLDOWN_S    = 10;    // 10 s, seteable (antes 120)
    static const float  WAVE_TIME_S        = 60;
    static const float  WAVE_MEMORY_S      = 60;    // el TOPE; el saludo vale hasta que para, lo pasa o se aleja
    static const float  WAVE_MIN_GAP_M     = 500;   // a recorrer desde una parada antes de parar por un saludo
    static const int    WAVE_MAX_PER_LAP   = 3;     // paradas por saludo por vuelta, como mucho
    static const float  STOP_WAVE_RADIUS_M = 20;
    static const float  BOARD_TIMEOUT_S    = 60;
    static const float  BOARD_TIMEOUT_MIN_S = 5;
    static const float  TRADER_TIME_S      = 60;
    static const float  PLAYER_RADIUS_M    = 8;
    static const float  MAX_WITH_PLAYERS_S = 0;    // los jugadores no demoran la salida (era 180)
    static const float  SEATED_WAIT_S      = 20;
    static const float  PLAYER_HALT_M      = 5;     // el alto de TTE frente a un jugador al vender (0 = sin alto)
    static const float  ZONE_RADIUS_MIN_M  = 5;
    // el default de MaxStallDistanceM. Un puesto o una senal mas lejos de su marca se avisa (el editor igual)
    static const float  MAX_STALL_DISTANCE_M = 25;
    static const int    MARCA_CADA_S       = 30;
    static const int    MARCA_MIN_S        = 1;     // que se mueva como el de un jugador (era 5)
    // el marcador no se reenvia si el vehiculo se movio menos que esto desde el ultimo envio (parado, no cuesta)
    static const float  MARCA_QUIETO_M     = 1;
    // en una parada el marcador parpadea (BlinkAtStops), cambiando cada BlinkSeconds; 0 = fijo
    static const int    MARCA_PARPADEO     = 1;
    static const int    MARCA_PARPADEO_S   = 2;
    // las senales de una parada (Stops[].Signals[])
    static const string SIGNAL_TYPE        = "auto";
    static const string SIGNAL_COLOR       = "green";
    static const float  SIGNAL_EVERY_S     = 60;
    static const float  SIGNAL_START_DELAY_S = 0;
    static const int    SIGNALS_MAX        = 10;    // por parada: las de mas no se encienden (se avisa)
    // al seguir, con un jugador enfrente: cuanto espera que se corra, como mucho. Apagado por defecto: el aviso es la
    // bocina al salir (HornOnDepart)
    static const float  FRONT_PLAYER_WAIT_S = 0;
    static const int    HORN_ON_DEPART     = 0;     // Stops[].HornOnDepart y Wave.HornOnDepart: sin bocina al salir
    // la faccion de eAI de nuestros comerciantes, siempre (Traders[].Faction se ignora): invencible y observadora, nadie
    // la recluta (tampoco en el juego del jugador: el tipo de faccion se sincroniza). Es la de los comerciantes IA de
    // Expansion
    static const string FACCION            = "InvincibleObservers";
}

// Markers{...}: el marcador de mapa, con el formato de ExpansionMarkers de TTE (sin MapMarkerDisplayName).
class BZTRMarkers {
    int    useMapMarker;
    int    MarkerUpdateSeconds;
    string MarkerLabel;
    string MarkersIcon;
    int    Expansion3DMarkers;
    string Red;
    string Green;
    string Blue;
    string Alpha;
    int    BlinkAtStops;       // 1 = parpadea en las paradas (senal de venta)
    int    BlinkSeconds;       // cada cuanto cambia (s); 0 = fijo tambien parado

    string Completar(BZTRNodo n, string pre, string nombre) {
        string def = "";
        if (!n.Tiene("useMapMarker")) {
            useMapMarker = 0;
            def = def + " " + pre + "useMapMarker";
        }
        if (!n.Tiene("MarkerUpdateSeconds")) {
            MarkerUpdateSeconds = BZTRDef.MARCA_CADA_S;
            def = def + " " + pre + "MarkerUpdateSeconds";
        }
        if (MarkerUpdateSeconds < BZTRDef.MARCA_MIN_S) MarkerUpdateSeconds = BZTRDef.MARCA_MIN_S;
        if (!n.Tiene("Expansion3DMarkers")) {
            Expansion3DMarkers = 0;
            def = def + " " + pre + "Expansion3DMarkers";
        }
        if (!n.Tiene("BlinkAtStops")) {
            BlinkAtStops = BZTRDef.MARCA_PARPADEO;
            def = def + " " + pre + "BlinkAtStops";
        }
        if (!n.Tiene("BlinkSeconds")) {
            BlinkSeconds = BZTRDef.MARCA_PARPADEO_S;
            def = def + " " + pre + "BlinkSeconds";
        }
        if (BlinkSeconds < 0) BlinkSeconds = 0;
        if (MarkerLabel == "") MarkerLabel = nombre;
        if (MarkersIcon == "") MarkersIcon = "Trader";
        if (Red == "") Red = "255";
        if (Green == "") Green = "255";
        if (Blue == "") Blue = "255";
        if (Alpha == "") Alpha = "255";
        return def;
    }

    int Color() {
        int a = Alpha.ToInt();
        int r = Red.ToInt();
        int g = Green.ToInt();
        int b = Blue.ToInt();
        if (a <= 0) a = 255;
        return ARGB(a, r, g, b);
    }
}

// Wave{...}: la sena (el gesto de saludo) en plena cinta.
class BZTRWave {
    int    Enabled;
    float  RadiusM;
    int    AheadOnly;
    float  CooldownS;
    // WhoGetsOff ya no existe (por saludo nadie baja); si el archivo lo trae, Avisos lo dice
    [NonSerialized()]
    bool   m_TraeWhoGetsOff;
    float  TimeS;
    float  MemoryS;      // el TOPE de un saludo (vale hasta que el vehiculo para, lo pasa o se aleja); se reevalua en cada tick
    float  MinGapM;      // metros a recorrer desde una parada antes de parar por un saludo (0 = sin minimo)
    int    MaxPerLap;    // paradas por saludo por vuelta, como mucho (0 = sin tope)
    int    HornOnDepart; // 1 = un toque de bocina antes de seguir de una parada por saludo

    string Completar(BZTRNodo n, string pre) {
        string def = "";
        if (!n.Tiene("Enabled")) {
            Enabled = 0;
            def = def + " " + pre + "Enabled";
        }
        if (!n.Tiene("RadiusM")) {
            RadiusM = BZTRDef.WAVE_RADIUS_M;
            def = def + " " + pre + "RadiusM";
        }
        if (RadiusM < 1) RadiusM = 1;
        if (!n.Tiene("AheadOnly")) {
            AheadOnly = 1;
            def = def + " " + pre + "AheadOnly";
        }
        if (!n.Tiene("CooldownS")) {
            CooldownS = BZTRDef.WAVE_COOLDOWN_S;
            def = def + " " + pre + "CooldownS";
        }
        if (CooldownS < 0) CooldownS = 0;
        m_TraeWhoGetsOff = n.Tiene("WhoGetsOff");
        if (!n.Tiene("TimeS")) {
            TimeS = BZTRDef.WAVE_TIME_S;
            def = def + " " + pre + "TimeS";
        }
        if (TimeS < 0) TimeS = 0;
        if (!n.Tiene("MemoryS")) {
            MemoryS = BZTRDef.WAVE_MEMORY_S;
            def = def + " " + pre + "MemoryS";
        }
        if (MemoryS < 1) MemoryS = 1;
        if (!n.Tiene("MinGapM")) {
            MinGapM = BZTRDef.WAVE_MIN_GAP_M;
            def = def + " " + pre + "MinGapM";
        }
        if (MinGapM < 0) MinGapM = 0;
        if (!n.Tiene("MaxPerLap")) {
            MaxPerLap = BZTRDef.WAVE_MAX_PER_LAP;
            def = def + " " + pre + "MaxPerLap";
        }
        if (MaxPerLap < 0) MaxPerLap = 0;
        if (!n.Tiene("HornOnDepart")) {
            HornOnDepart = BZTRDef.HORN_ON_DEPART;
            def = def + " " + pre + "HornOnDepart";
        }
        if (HornOnDepart != 0) HornOnDepart = 1;
        return def;
    }
}

// Driver{...}
class BZTRDriver {
    int IsTrader;      // 1 = el chofer es comerciante (va en Traders con Seat 0)
    int AllCanDrive;   // sucesion: fuera de esta etapa (todo es invencible)

    string Completar(BZTRNodo n, string pre) {
        string def = "";
        if (!n.Tiene("IsTrader")) {
            IsTrader = 0;
            def = def + " " + pre + "IsTrader";
        }
        if (!n.Tiene("AllCanDrive")) {
            AllCanDrive = 1;
            def = def + " " + pre + "AllCanDrive";
        }
        return def;
    }
}

// TraderZone{...}: la zona del Market con la que se enganchan los comerciantes del servicio.
class BZTRTraderZone {
    string Name;             // m_FileName de la zona (traderzones\<Name>.json); "" = la del lugar donde nacen
    int    FollowsVehicle;   // 1 = su centro sigue al vehiculo
    float  RadiusM;

    string Completar(BZTRNodo n, string pre) {
        string def = "";
        if (!n.Tiene("FollowsVehicle")) {
            FollowsVehicle = 0;
            def = def + " " + pre + "FollowsVehicle";
        }
        if (!n.Tiene("RadiusM")) def = def + " " + pre + "RadiusM";
        if (RadiusM < BZTRDef.ZONE_RADIUS_MIN_M) RadiusM = BZTRDef.ZONE_RADIUS_MIN_M;
        return def;
    }
}

// Traders[]: quien viaja y que vende. La CLASE la crea AutoDrive (Crew de la ruta); aca se lee para avisar.
class BZTRTrader {
    int    Seat;
    string Name;
    string TraderAIClass;
    string Loadout;
    // ⛔ La clave del archivo es "Clothing", pero Clothing es una CLASE de 4_World: un campo con ese nombre no tiene
    // precedente en codigo que compila. Se lee del texto (BZTRNodo.Textos) a un campo con otro nombre; el serializador
    // ignora la clave "Clothing" (ignora las claves sin campo: el _hdr.json de AutoDrive trae Envelope, Fingerprint...).
    [NonSerialized()]
    ref array<string> m_Ropa;
    string TraderFile;
    string Faction;          // se ignora: nuestros comerciantes son siempre BZTRDef.FACCION (el resumen lo dice)
    // ★ Invincible y CanBeLooted ya no son campos: los comerciantes son siempre invencibles.
    // Si el archivo los trae, Avisos lo dice.
    [NonSerialized()]
    bool   m_TraeInvencible;
    string LootingBehaviour;

    string Completar(BZTRNodo n, string pre) {
        string def = "";
        if (!n.Tiene("Seat")) {
            Seat = -1;
            def = def + " " + pre + "Seat(falta: se ignora)";
        }
        if (Name == "") Name = "comerciante de la plaza " + Seat.ToString();
        m_Ropa = new array<string>();
        n.Textos("Clothing", m_Ropa);
        // Faction se ignora (BZTRDef.FACCION, siempre): queda para leer los archivos con el formato de TTE
        if (Faction == "") Faction = BZTRDef.FACCION;
        m_TraeInvencible = (n.Tiene("Invincible") || n.Tiene("CanBeLooted"));
        if (LootingBehaviour == "") {
            LootingBehaviour = BZTRDef.LOOTING;
            def = def + " " + pre + "LootingBehaviour";
        }
        return def;
    }

    // LootingBehaviour -> bitmask de eAILootingBehavior (tokens con |). Los desconocidos van a 'invalidos'.
    int BitsLooting(out string invalidos) {
        array<string> partes = new array<string>();
        string p;
        string nombreEnum = "eAILootingBehavior";   // el enum de Expansion AI, resuelto por nombre
        typename tipoEnum = nombreEnum.ToType();
        int bits = 0;
        int v;
        int i;
        invalidos = "";
        if (!tipoEnum) {
            invalidos = "(no existe el enum eAILootingBehavior: Expansion AI no cargado)";
            return 0;
        }
        LootingBehaviour.Split("|", partes);
        for (i = 0; i < partes.Count(); i++) {
            p = partes.Get(i);
            p.TrimInPlace();
            p.ToUpper();
            if (p == "") continue;
            v = typename.StringToEnum(tipoEnum, p);
            if (v < 0) {
                if (invalidos != "") invalidos = invalidos + ", ";
                invalidos = invalidos + p;
                continue;
            }
            bits = bits | v;
        }
        return bits;
    }

    bool SeViste() {
        if (Loadout != "") return true;
        if (m_Ropa && m_Ropa.Count() > 0) return true;
        return false;
    }
}

// Waypoints[] de un comerciante en una parada: el esquema de TTE. La altura viene en 0 del editor.
class BZTRWaypoint {
    string WayPointName;
    vector Position;
}

// Stops[].Traders[]: que hace cada comerciante que baja en esa parada.
class BZTRStopTrader {
    int    Seat;
    ref array<ref BZTRWaypoint> Waypoints;
    float  TimeS;
    string MovementSpeed;   // WALK · JOG (RUN se toma como JOG, como TTE)
    // hacia donde mira en su puesto mientras vende. Opcionales: sin ninguno, mira hacia donde llego.
    // Se leen del TEXTO (como Pos): un campo serializado lo escribiria SaveFile en el ejemplo como "HeadingDeg": 0, que
    // es el norte, no "sin orientacion".
    [NonSerialized()]
    float  m_Rumbo;          // "HeadingDeg": rumbo de DayZ, 0 = norte, horario (adelante = (sin h, cos h) en x, z)
    [NonSerialized()]
    bool   m_TieneRumbo;
    [NonSerialized()]
    ref array<float> m_MirarA;   // "LookAt": [x, z], que mire hacia ese punto; gana sobre HeadingDeg

    string Completar(BZTRNodo n, string pre) {
        string def = "";
        m_TieneRumbo = n.Tiene("HeadingDeg");
        m_Rumbo = 0;
        if (m_TieneRumbo) m_Rumbo = n.Numero("HeadingDeg");
        m_Rumbo = m_Rumbo - 360 * Math.Floor(m_Rumbo / 360);
        m_MirarA = new array<float>();
        n.Numeros("LookAt", m_MirarA);
        if (m_MirarA.Count() != 0 && m_MirarA.Count() != 2) {
            def = def + " " + pre + "LookAt(" + m_MirarA.Count().ToString() + " numeros, van 2: se ignora)";
            m_MirarA.Clear();
        }
        if (!Waypoints) Waypoints = new array<ref BZTRWaypoint>();
        if (!n.Tiene("TimeS")) {
            TimeS = BZTRDef.TRADER_TIME_S;
            def = def + " " + pre + "TimeS";
        }
        if (TimeS < 0) TimeS = 0;
        MovementSpeed.ToUpper();
        if (MovementSpeed == "RUN") MovementSpeed = "JOG";
        if (MovementSpeed != "WALK" && MovementSpeed != "JOG") {
            if (MovementSpeed != "") def = def + " " + pre + "MovementSpeed(" + MovementSpeed + " no vale)";
            else def = def + " " + pre + "MovementSpeed";
            MovementSpeed = "WALK";
        }
        return def;
    }

    // La altura de cada puesto: la del terreno (el editor no la conoce). Devuelve el log.
    string AlturaDelTerreno() {
        string s = "";
        int i;
        BZTRWaypoint w;
        vector p;
        float y;
        for (i = 0; i < Waypoints.Count(); i++) {
            w = Waypoints.Get(i);
            if (!w) continue;
            p = w.Position;
            y = GetGame().SurfaceY(p[0], p[2]);
            p[1] = y;
            w.Position = p;
            s = s + " " + w.WayPointName + " (" + BZTRLog.Pos3(p) + ")";
        }
        return s;
    }

    int Velocidad() {
        if (MovementSpeed == "JOG") return eAIMovementSpeed.JOG;
        return eAIMovementSpeed.WALK;
    }

    // la direccion (mundo, en el plano) hacia la que mira estando en 'desde'. ok = false si el puesto no pide
    // orientacion (o LookAt cae encima de el). 'como' = de donde sale, para el log.
    vector Mirada(vector desde, out bool ok, out string como) {
        vector d;
        float h;
        ok = false;
        como = "";
        if (m_MirarA && m_MirarA.Count() == 2) {
            d = Vector(m_MirarA.Get(0) - desde[0], 0, m_MirarA.Get(1) - desde[2]);
            if (d.Length() >= 0.2) {
                ok = true;
                como = "LookAt " + BZTRLog.XZ(Vector(m_MirarA.Get(0), 0, m_MirarA.Get(1)));
                return d.Normalized();
            }
        }
        if (!m_TieneRumbo) return "0 0 1";
        h = m_Rumbo * Math.DEG2RAD;
        ok = true;
        como = "HeadingDeg " + Math.Round(m_Rumbo).ToString();
        return Vector(Math.Sin(h), 0, Math.Cos(h));
    }
}

// Stops[].Signals[]: una senal para que la parada se vea de lejos. La enciende el despliegue, se
// renueva mientras dura la parada y se borra al seguir (BZTRBalizas, BZTraderBalizas.c).
class BZTRStopSignal {
    // ⛔ "Type" y "Pos" se leen del texto a campos con otro nombre: Pos es una CLASE (5_Mission) y Type() es un metodo
    // de Class (lo heredan todas las clases). Mismo criterio que Clothing.
    [NonSerialized()]
    string m_Tipo;              // auto · smoke · roadflare · flaregun · fireworks · airdrop (Expansion Missions)
    [NonSerialized()]
    ref array<float> m_PosXZ;   // [x, z] absoluta, como los puestos; la altura, del terreno
    string Color;               // green · purple · red · white · yellow · blue · black (el humo; la roadflare es roja)
    float  EverySeconds;        // cada cuanto se renueva la que se apago; 0 = una sola vez
    float  StartDelayS;         // desde el despliegue

    string Completar(BZTRNodo n, string pre) {
        string def = "";
        m_Tipo = n.Texto("Type");
        m_Tipo.ToLower();
        if (!EsTipo(m_Tipo)) {
            if (m_Tipo != "") def = def + " " + pre + "Type(" + m_Tipo + " no vale)";
            else def = def + " " + pre + "Type";
            m_Tipo = BZTRDef.SIGNAL_TYPE;
        }
        m_PosXZ = new array<float>();
        n.Numeros("Pos", m_PosXZ);
        if (m_PosXZ.Count() < 2) def = def + " " + pre + "Pos(falta: no se enciende)";
        Color.ToLower();
        if (!EsColor(Color)) {
            if (Color != "") def = def + " " + pre + "Color(" + Color + " no vale)";
            else def = def + " " + pre + "Color";
            Color = BZTRDef.SIGNAL_COLOR;
        }
        if (!n.Tiene("EverySeconds")) {
            EverySeconds = BZTRDef.SIGNAL_EVERY_S;
            def = def + " " + pre + "EverySeconds";
        }
        if (EverySeconds < 0) EverySeconds = 0;
        if (!n.Tiene("StartDelayS")) {
            StartDelayS = BZTRDef.SIGNAL_START_DELAY_S;
            def = def + " " + pre + "StartDelayS";
        }
        if (StartDelayS < 0) StartDelayS = 0;
        return def;
    }

    static bool EsTipo(string t) {
        if (t == "auto" || t == "smoke" || t == "roadflare") return true;
        if (t == "flaregun" || t == "fireworks") return true;
        if (t == "airdrop") return true;
        return false;
    }

    static bool EsColor(string c) {
        if (c == "green" || c == "purple" || c == "red" || c == "white") return true;
        if (c == "yellow" || c == "blue" || c == "black") return true;
        return false;
    }

    // ¿Puede salir humo? (auto de dia, o smoke)
    bool UsaHumo() {
        return m_Tipo == "auto" || m_Tipo == "smoke";
    }

    bool TienePos() {
        return m_PosXZ && m_PosXZ.Count() >= 2;
    }

    // [x, z] del archivo con la altura del terreno; ok = false si no vino.
    vector PosMundo(out bool ok) {
        float x;
        float z;
        ok = TienePos();
        if (!ok) return "0 0 0";
        x = m_PosXZ.Get(0);
        z = m_PosXZ.Get(1);
        return Vector(x, GetGame().SurfaceY(x, z), z);
    }
}

// Stops[]: una marca 4 de la toma = una parada.
class BZTRStop {
    int    Mark;            // numero de marca 4 (1 = la primera de la toma)
    // [x, z] de la marca, para verificar contra la toma. La clave es "Pos" y Pos es una CLASE (5_Mission): mismo
    // criterio que Clothing, se lee del texto a un campo con otro nombre.
    [NonSerialized()]
    ref array<float> m_PosXZ;
    float  HeadingDeg;
    float  DistM;
    string Mode;            // always · wave_only · off
    float  WaveRadiusM;
    ref array<int> WhoGetsOff;
    float  BoardTimeoutS;
    ref array<ref BZTRStopTrader> Traders;
    ref array<ref BZTRStopSignal> Signals;   // opcional
    int    HornOnDepart;    // 1 = un toque de bocina antes de seguir de esta parada

    string Completar(BZTRNodo n, string pre, int idx) {
        string def = "";
        int i;
        BZTRStopTrader st;
        BZTRStopSignal sg;
        BZTRNodo nt;   // ⛔ el nodo en una local, nunca como temporal (ver BZTRServicio.Completar)
        string preT;
        if (!n.Tiene("Mark")) {
            Mark = idx + 1;
            def = def + " " + pre + "Mark";
        }
        m_PosXZ = new array<float>();
        n.Numeros("Pos", m_PosXZ);
        if (Mode != "always" && Mode != "wave_only" && Mode != "off") {
            if (Mode != "") def = def + " " + pre + "Mode(" + Mode + " no vale)";
            else def = def + " " + pre + "Mode";
            Mode = "always";
        }
        if (WaveRadiusM <= 0) WaveRadiusM = BZTRDef.STOP_WAVE_RADIUS_M;
        if (!WhoGetsOff) WhoGetsOff = new array<int>();
        if (!n.Tiene("BoardTimeoutS")) {
            BoardTimeoutS = BZTRDef.BOARD_TIMEOUT_S;
            def = def + " " + pre + "BoardTimeoutS";
        }
        if (BoardTimeoutS < BZTRDef.BOARD_TIMEOUT_MIN_S) BoardTimeoutS = BZTRDef.BOARD_TIMEOUT_MIN_S;
        if (!Traders) Traders = new array<ref BZTRStopTrader>();
        for (i = 0; i < Traders.Count(); i++) {
            st = Traders.Get(i);
            if (!st) continue;
            preT = pre + "Traders[" + i.ToString() + "].";
            nt = n.Elemento("Traders", i);
            def = def + st.Completar(nt, preT);
        }
        if (!Signals) Signals = new array<ref BZTRStopSignal>();
        for (i = 0; i < Signals.Count(); i++) {
            sg = Signals.Get(i);
            if (!sg) continue;
            preT = pre + "Signals[" + i.ToString() + "].";
            nt = n.Elemento("Signals", i);
            def = def + sg.Completar(nt, preT);
        }
        if (!n.Tiene("HornOnDepart")) {
            HornOnDepart = BZTRDef.HORN_ON_DEPART;
            def = def + " " + pre + "HornOnDepart";
        }
        if (HornOnDepart != 0) HornOnDepart = 1;
        return def;
    }

    BZTRStopTrader DeLaPlaza(int seat) {
        int i;
        BZTRStopTrader st;
        for (i = 0; i < Traders.Count(); i++) {
            st = Traders.Get(i);
            if (st && st.Seat == seat) return st;
        }
        return null;
    }

    bool Baja(int seat) {
        return WhoGetsOff.Find(seat) >= 0;
    }

    // La distancia (XZ) de la marca al waypoint mas lejano de un puesto; 0 si la marca no trae Pos.
    float PuestoMasLejos(BZTRStopTrader st) {
        bool ok;
        vector pe = PosEditor(ok);
        vector w;
        float d;
        float mayor = 0;
        int i;
        BZTRWaypoint wp;
        if (!ok || !st || !st.Waypoints) return 0;
        for (i = 0; i < st.Waypoints.Count(); i++) {
            wp = st.Waypoints.Get(i);
            if (!wp) continue;
            w = wp.Position;
            d = Math.Sqrt((w[0] - pe[0]) * (w[0] - pe[0]) + (w[2] - pe[2]) * (w[2] - pe[2]));
            if (d > mayor) mayor = d;
        }
        return mayor;
    }

    // la distancia (XZ) de la marca a una senal; 0 si falta alguna de las dos posiciones.
    float SenalLejos(BZTRStopSignal sg) {
        bool ok;
        vector pe = PosEditor(ok);
        float dx;
        float dz;
        if (!ok || !sg || !sg.TienePos()) return 0;
        dx = sg.m_PosXZ.Get(0) - pe[0];
        dz = sg.m_PosXZ.Get(1) - pe[2];
        return Math.Sqrt(dx * dx + dz * dz);
    }

    // Posicion de la marca segun el editor ([x, z]); ok = false si no vino.
    vector PosEditor(out bool ok) {
        ok = false;
        array<float> xz = m_PosXZ;
        if (!xz || xz.Count() < 2) return "0 0 0";
        ok = true;
        return Vector(xz.Get(0), 0, xz.Get(1));
    }
}

// OnRouteEnd{...}: que pasa al llegar al final (decision del admin, no del codigo).
class BZTROnRouteEnd {
    string Action;   // despawn · respawn · restart
    float  DelayS;   // respawn: a los DelayS se relanza
    float  WaitS;    // opcional: espera en la terminal antes (vende sentado / con jugadores cerca)

    string Completar(BZTRNodo n, string pre) {
        string def = "";
        if (Action != "despawn" && Action != "respawn" && Action != "restart") {
            if (Action != "") def = def + " " + pre + "Action(" + Action + " no vale)";
            else def = def + " " + pre + "Action";
            Action = "despawn";
        }
        if (DelayS < 0) DelayS = 0;
        if (!n.Tiene("WaitS")) WaitS = 0;
        if (WaitS < 0) WaitS = 0;
        return def;
    }
}

// Editor{...}: lo usa el editor para rearmar la sesion. El mod lo ignora (se declara para que el parser lo acepte).
class BZTREditor {
    int   Version;
    int   Seats;
    float RouteLengthM;
    ref array<float> Start;
    ref array<float> End;
    int   Example;
}

// UN SERVICIO: un vehiculo con su ruta, su chofer y sus comerciantes.
class BZTRServicio {
    int    Enable;
    string Name;
    string Route;          // archivo de ruta de AutoDrive, en $profile:BZ_AutoDrive/
    string VehicleClass;   // informativo: se compara con el hdr de la ruta
    string StopMode;       // marks · wave_only · marks_and_wave
    ref BZTRWave Wave;
    ref BZTRDriver Driver;
    ref BZTRTraderZone TraderZone;
    ref array<ref BZTRTrader> Traders;
    ref array<ref BZTRStop> Stops;
    ref BZTROnRouteEnd OnRouteEnd;
    ref BZTREditor Editor;
    // opcionales (tiempos de espera)
    float  StartDelayS;
    ref BZTRMarkers Markers;
    int    RespawnSeconds;
    float  PlayerRadiusM;
    float  MaxWithPlayersS;
    float  SeatedWaitS;
    float  PlayerHaltM;   // a cuantos metros de un jugador se frena y lo mira mientras vende; 0 = nunca
    float  MaxStallDistanceM;   // un puesto o una senal mas lejos de su marca se avisa (mercado armado: mas)
    // PlayersCanBoard ya no existe: los vehiculos van siempre cerrados a jugadores. Si el archivo lo trae, Avisos lo dice
    [NonSerialized()]
    bool   m_TraePlayersCanBoard;
    float  FrontPlayerWaitS;    // al seguir, con un jugador en el camino de adelante: espera que se corra, como mucho; 0 = no espera

    // Defaults por presencia sobre el texto del archivo. Devuelve los campos que salieron del default.
    // ⛔ cada BZTRNodo va a una variable LOCAL antes de pasarlo. Pasado como temporal
    // (s.Completar(raiz.Elemento("Stops", i), ...)), el nodo llego NULL a la linea donde BZTRStop.Completar lo usa
    // para su propio Elemento (VME 'NULL pointer to instance', bztraderconfig.c:351; en esa linea es
    // lo unico sin chequear): el Completar de la parada se corto y sus defaults no se aplicaron ni se informaron.
    // Por que murio el temporal no esta medido; una local es una referencia fuerte y no depende de eso.
    // Un validador del build corta si vuelve el patron; Asegurar() avisa si igual se corta una carga.
    string Completar(string texto, string archivo) {
        BZTRNodo raiz = new BZTRNodo(texto, texto.IndexOf("{"));
        BZTRNodo nodo;
        string def = "";
        string pre;
        int i;
        BZTRTrader t;
        BZTRStop s;
        if (!raiz.Tiene("Enable")) def = def + " Enable(0)";
        if (Name == "") Name = archivo;
        if (StopMode != "marks" && StopMode != "wave_only" && StopMode != "marks_and_wave") {
            if (StopMode != "") def = def + " StopMode(" + StopMode + " no vale)";
            else def = def + " StopMode";
            StopMode = "marks";
        }
        if (!Wave) Wave = new BZTRWave();
        nodo = raiz.Hijo("Wave");
        def = def + Wave.Completar(nodo, "Wave.");
        if (!Driver) Driver = new BZTRDriver();
        nodo = raiz.Hijo("Driver");
        def = def + Driver.Completar(nodo, "Driver.");
        if (!TraderZone) TraderZone = new BZTRTraderZone();
        nodo = raiz.Hijo("TraderZone");
        def = def + TraderZone.Completar(nodo, "TraderZone.");
        if (!Traders) Traders = new array<ref BZTRTrader>();
        for (i = 0; i < Traders.Count(); i++) {
            t = Traders.Get(i);
            if (!t) continue;
            pre = "Traders[" + i.ToString() + "].";
            nodo = raiz.Elemento("Traders", i);
            def = def + t.Completar(nodo, pre);
        }
        if (!Stops) Stops = new array<ref BZTRStop>();
        for (i = 0; i < Stops.Count(); i++) {
            s = Stops.Get(i);
            if (!s) continue;
            pre = "Stops[" + i.ToString() + "].";
            nodo = raiz.Elemento("Stops", i);
            def = def + s.Completar(nodo, pre, i);
        }
        if (!OnRouteEnd) OnRouteEnd = new BZTROnRouteEnd();
        nodo = raiz.Hijo("OnRouteEnd");
        def = def + OnRouteEnd.Completar(nodo, "OnRouteEnd.");
        if (!raiz.Tiene("StartDelayS")) {
            StartDelayS = BZTRDef.START_DELAY_S;
            def = def + " StartDelayS";
        }
        if (StartDelayS < 0) StartDelayS = 0;
        if (!Markers) Markers = new BZTRMarkers();
        nodo = raiz.Hijo("Markers");
        def = def + Markers.Completar(nodo, "Markers.", Name);
        if (!raiz.Tiene("RespawnSeconds")) {
            RespawnSeconds = BZTRDef.RESPAWN_S;
            def = def + " RespawnSeconds";
        }
        if (!raiz.Tiene("PlayerRadiusM")) {
            PlayerRadiusM = BZTRDef.PLAYER_RADIUS_M;
            def = def + " PlayerRadiusM";
        }
        if (PlayerRadiusM < 1) PlayerRadiusM = 1;
        if (!raiz.Tiene("MaxWithPlayersS")) {
            MaxWithPlayersS = BZTRDef.MAX_WITH_PLAYERS_S;
            def = def + " MaxWithPlayersS";
        }
        if (!raiz.Tiene("SeatedWaitS")) {
            SeatedWaitS = BZTRDef.SEATED_WAIT_S;
            def = def + " SeatedWaitS";
        }
        if (SeatedWaitS < 0) SeatedWaitS = 0;
        if (!raiz.Tiene("PlayerHaltM")) {
            PlayerHaltM = BZTRDef.PLAYER_HALT_M;
            def = def + " PlayerHaltM";
        }
        if (PlayerHaltM < 0) PlayerHaltM = 0;
        if (!raiz.Tiene("MaxStallDistanceM")) {
            MaxStallDistanceM = BZTRDef.MAX_STALL_DISTANCE_M;
            def = def + " MaxStallDistanceM";
        }
        if (MaxStallDistanceM <= 0) MaxStallDistanceM = BZTRDef.MAX_STALL_DISTANCE_M;
        m_TraePlayersCanBoard = raiz.Tiene("PlayersCanBoard");
        if (!raiz.Tiene("FrontPlayerWaitS")) {
            FrontPlayerWaitS = BZTRDef.FRONT_PLAYER_WAIT_S;
            def = def + " FrontPlayerWaitS";
        }
        if (FrontPlayerWaitS < 0) FrontPlayerWaitS = 0;
        return def;
    }

    // ⛔ una VME adentro de un Completar corta SOLO esa funcion; la carga sigue y el servicio corre con lo
    // que quedo a medias, sin aviso. Esto se corre despues de Completar: repara lo que un Completar siempre deja
    // puesto (objetos, listas, valores de enumeracion) y devuelve que tuvo que reparar ("" = la carga quedo entera).
    string Asegurar() {
        string r = "";
        string pre;
        int i;
        int j;
        BZTRTrader t;
        BZTRStop s;
        BZTRStopTrader st;
        BZTRStopSignal sg;
        BZTRNodo vacio = new BZTRNodo("", -1);
        if (!Wave) {
            Wave = new BZTRWave();
            Wave.Completar(vacio, "Wave.");
            r = r + " Wave";
        }
        if (!Driver) {
            Driver = new BZTRDriver();
            Driver.Completar(vacio, "Driver.");
            r = r + " Driver";
        }
        if (!TraderZone) {
            TraderZone = new BZTRTraderZone();
            TraderZone.Completar(vacio, "TraderZone.");
            r = r + " TraderZone";
        }
        if (!OnRouteEnd) {
            OnRouteEnd = new BZTROnRouteEnd();
            OnRouteEnd.Completar(vacio, "OnRouteEnd.");
            r = r + " OnRouteEnd";
        }
        if (!Markers) {
            Markers = new BZTRMarkers();
            Markers.Completar(vacio, "Markers.", Name);
            r = r + " Markers";
        }
        if (StopMode != "marks" && StopMode != "wave_only" && StopMode != "marks_and_wave") {
            StopMode = "marks";
            r = r + " StopMode";
        }
        if (!Traders) {
            Traders = new array<ref BZTRTrader>();
            r = r + " Traders";
        }
        for (i = 0; i < Traders.Count(); i++) {
            t = Traders.Get(i);
            if (!t) continue;
            pre = " Traders[" + i.ToString() + "].";
            if (!t.m_Ropa) {
                t.m_Ropa = new array<string>();
                r = r + pre + "Clothing";
            }
            if (t.Faction == "") t.Faction = BZTRDef.FACCION;   // se ignora (ver BZTRDef.FACCION)
            if (t.LootingBehaviour == "") {
                t.LootingBehaviour = BZTRDef.LOOTING;
                r = r + pre + "LootingBehaviour";
            }
        }
        if (!Stops) {
            Stops = new array<ref BZTRStop>();
            r = r + " Stops";
        }
        for (i = 0; i < Stops.Count(); i++) {
            s = Stops.Get(i);
            if (!s) continue;
            pre = " Stops[" + i.ToString() + "].";
            if (!s.m_PosXZ) {
                s.m_PosXZ = new array<float>();
                r = r + pre + "Pos";
            }
            if (s.Mode != "always" && s.Mode != "wave_only" && s.Mode != "off") {
                s.Mode = "always";
                r = r + pre + "Mode";
            }
            if (!s.WhoGetsOff) {
                s.WhoGetsOff = new array<int>();
                r = r + pre + "WhoGetsOff";
            }
            if (s.BoardTimeoutS < BZTRDef.BOARD_TIMEOUT_MIN_S) {
                s.BoardTimeoutS = BZTRDef.BOARD_TIMEOUT_MIN_S;
                r = r + pre + "BoardTimeoutS";
            }
            if (!s.Traders) {
                s.Traders = new array<ref BZTRStopTrader>();
                r = r + pre + "Traders";
            }
            for (j = 0; j < s.Traders.Count(); j++) {
                st = s.Traders.Get(j);
                if (!st) continue;
                if (!st.Waypoints) {
                    st.Waypoints = new array<ref BZTRWaypoint>();
                    r = r + pre + "Traders[" + j.ToString() + "].Waypoints";
                }
                if (st.MovementSpeed != "WALK" && st.MovementSpeed != "JOG") {
                    st.MovementSpeed = "WALK";
                    r = r + pre + "Traders[" + j.ToString() + "].MovementSpeed";
                }
                if (st.TimeS < 0) {
                    st.TimeS = 0;
                    r = r + pre + "Traders[" + j.ToString() + "].TimeS";
                }
                if (!st.m_MirarA) {
                    st.m_MirarA = new array<float>();
                    r = r + pre + "Traders[" + j.ToString() + "].LookAt";
                }
            }
            if (!s.Signals) {
                s.Signals = new array<ref BZTRStopSignal>();
                r = r + pre + "Signals";
            }
            for (j = 0; j < s.Signals.Count(); j++) {
                sg = s.Signals.Get(j);
                if (!sg) continue;
                if (!sg.m_PosXZ) {
                    sg.m_PosXZ = new array<float>();
                    r = r + pre + "Signals[" + j.ToString() + "].Pos";
                }
                if (!BZTRStopSignal.EsTipo(sg.m_Tipo)) {
                    sg.m_Tipo = BZTRDef.SIGNAL_TYPE;
                    r = r + pre + "Signals[" + j.ToString() + "].Type";
                }
                if (!BZTRStopSignal.EsColor(sg.Color)) {
                    sg.Color = BZTRDef.SIGNAL_COLOR;
                    r = r + pre + "Signals[" + j.ToString() + "].Color";
                }
            }
        }
        if (MaxStallDistanceM <= 0) {
            MaxStallDistanceM = BZTRDef.MAX_STALL_DISTANCE_M;
            r = r + " MaxStallDistanceM";
        }
        return r;
    }

    // "" si se puede lanzar; si no, por que no.
    string Invalido() {
        if (Route == "") return "falta Route (el archivo de ruta de AutoDrive)";
        if (!FileExist("$profile:BZ_AutoDrive/" + Route)) return "la ruta no existe: $profile:BZ_AutoDrive/" + Route;
        return "";
    }

    // Incoherencias que no impiden lanzar: se avisan (el editor y el mod tienen que hablar el mismo archivo).
    void Avisos(array<string> av) {
        int i;
        int j;
        int k;
        BZTRTrader t;
        BZTRStop s;
        BZTRStopTrader st;
        BZTRStopSignal sg;
        float lejos;
        string preS;
        bool traenInvencible = false;
        string tope = BZTRLog.Metros(MaxStallDistanceM);
        array<int> plazas = new array<int>();
        for (i = 0; i < Traders.Count(); i++) {
            t = Traders.Get(i);
            if (!t) continue;
            if (t.Seat < 0) continue;
            if (plazas.Find(t.Seat) >= 0) av.Insert("Traders: la plaza " + t.Seat.ToString() + " esta dos veces (se usa la primera)");
            plazas.Insert(t.Seat);
            if (t.Seat == 0 && Driver.IsTrader != 1) av.Insert("Traders: hay un comerciante en la plaza 0 pero Driver.IsTrader = 0 (no se engancha)");
            if (t.TraderFile == "") av.Insert("Traders[" + i.ToString() + "] (" + t.Name + "): falta TraderFile, no va a vender");
            if (t.m_TraeInvencible) traenInvencible = true;
        }
        if (traenInvencible) av.Insert("Invincible/CanBeLooted ya no se usan: los comerciantes de BZ_Trader son invencibles");
        if (Driver.IsTrader == 1 && plazas.Find(0) < 0) av.Insert("Driver.IsTrader = 1 pero ningun comerciante tiene Seat 0: el chofer no vende");
        // ★ en BZ_Trader maneja uno de los comerciantes. Un archivo viejo con IsTrader 0 sigue andando
        if (Driver.IsTrader != 1) av.Insert("Driver.IsTrader = 0: maneja el DriverClass de la ruta, que no vende. En BZ_Trader maneja uno de los comerciantes: elegilo en el editor (Quien maneja)");
        if (Wave.m_TraeWhoGetsOff) av.Insert("Wave.WhoGetsOff ya no se usa: en una parada por saludo nadie baja");
        if (m_TraePlayersCanBoard) av.Insert("PlayersCanBoard ya no se usa: los vehiculos de BZ_Trader van siempre cerrados a jugadores (nadie se sienta)");
        for (i = 0; i < Stops.Count(); i++) {
            s = Stops.Get(i);
            if (!s) continue;
            for (j = 0; j < s.WhoGetsOff.Count(); j++) {
                k = s.WhoGetsOff.Get(j);
                if (plazas.Find(k) < 0) av.Insert("Stops[Mark " + s.Mark.ToString() + "]: WhoGetsOff incluye la plaza " + k.ToString() + ", que no es de un comerciante (no baja)");
            }
            for (j = 0; j < s.Traders.Count(); j++) {
                st = s.Traders.Get(j);
                if (!st) continue;
                if (!s.Baja(st.Seat)) av.Insert("Stops[Mark " + s.Mark.ToString() + "]: hay puesto para la plaza " + st.Seat.ToString() + " pero no esta en WhoGetsOff");
                lejos = s.PuestoMasLejos(st);
                if (lejos > MaxStallDistanceM) av.Insert("Stops[Mark " + s.Mark.ToString() + "]: el puesto de la plaza " + st.Seat.ToString() + " esta a " + BZTRLog.Metros(lejos) + " de la marca (mas de MaxStallDistanceM " + tope + "): camina mucho y la zona crece");
            }
            // las senales
            for (j = 0; j < s.Signals.Count(); j++) {
                sg = s.Signals.Get(j);
                if (!sg) continue;
                preS = "Stops[Mark " + s.Mark.ToString() + "].Signals[" + j.ToString() + "]";
                if (j >= BZTRDef.SIGNALS_MAX) {
                    av.Insert("Stops[Mark " + s.Mark.ToString() + "]: " + s.Signals.Count().ToString() + " senales: se encienden las primeras " + BZTRDef.SIGNALS_MAX.ToString());
                    break;
                }
                if (!sg.TienePos()) {
                    av.Insert(preS + ": sin Pos [x, z], no se enciende");
                    continue;
                }
                lejos = s.SenalLejos(sg);
                if (lejos > MaxStallDistanceM) av.Insert(preS + ": esta a " + BZTRLog.Metros(lejos) + " de la marca (mas de MaxStallDistanceM " + tope + ")");
                if (sg.m_Tipo == "flaregun") av.Insert(preS + ": flaregun todavia no se dispara (el proyectil lo crea el motor al disparar un arma en manos): sale una roadflare");
#ifndef EXPANSIONMODMISSIONS
                if (sg.m_Tipo == "airdrop") av.Insert(preS + ": el humo de airdrop es de Expansion Missions, que no esta cargado: no se enciende");
#endif
                if (sg.Color == "blue" && sg.UsaHumo()) av.Insert(preS + ": no hay humo azul en la 1.30: sale violeta");
            }
        }
    }

    BZTRTrader TraderDeLaPlaza(int seat) {
        int i;
        BZTRTrader t;
        for (i = 0; i < Traders.Count(); i++) {
            t = Traders.Get(i);
            if (t && t.Seat == seat) return t;
        }
        return null;
    }

    BZTRStop StopDeLaMarca(int mark) {
        int i;
        BZTRStop s;
        for (i = 0; i < Stops.Count(); i++) {
            s = Stops.Get(i);
            if (s && s.Mark == mark) return s;
        }
        return null;
    }

    bool MarcasActivas() {
        return StopMode != "wave_only";
    }

    bool SenaLibre() {
        if (StopMode == "marks") return false;
        return Wave.Enabled == 1;
    }

    // donde toca bocina al salir: "marcas 1 3 y saludo" ("" = en ninguna)
    string Bocinas() {
        int i;
        BZTRStop st;
        string marcas = "";
        string r = "";
        for (i = 0; i < Stops.Count(); i++) {
            st = Stops.Get(i);
            if (st && st.HornOnDepart == 1) marcas = marcas + " " + st.Mark.ToString();
        }
        if (marcas != "") r = "marcas" + marcas;
        if (SenaLibre() && Wave.HornOnDepart == 1) {
            if (r != "") r = r + " y ";
            r = r + "saludo";
        }
        return r;
    }

    // las facciones que el archivo pide y no son BZTRDef.FACCION ("Observers"; "" = ninguna): se ignoran
    string FaccionesIgnoradas() {
        int i;
        BZTRTrader t;
        string r = "";
        for (i = 0; i < Traders.Count(); i++) {
            t = Traders.Get(i);
            if (!t || t.Faction == BZTRDef.FACCION) continue;
            if (r.IndexOf(t.Faction) >= 0) continue;
            if (r != "") r = r + ", ";
            r = r + t.Faction;
        }
        return r;
    }

    string Resumen() {
        string bocinas = Bocinas();
        string ignoradas = FaccionesIgnoradas();
        string s = "Route " + Route + " · StopMode " + StopMode + " · sena " + SenaLibre().ToString();
        s = s + " · chofer comerciante " + Driver.IsTrader.ToString() + " · " + Traders.Count().ToString() + " comerciante(s)";
        s = s + " · " + Stops.Count().ToString() + " parada(s) · OnRouteEnd " + OnRouteEnd.Action;
        if (bocinas != "") s = s + " · bocina al salir: " + bocinas;
        s = s + " · faccion " + BZTRDef.FACCION;
        if (ignoradas != "") s = s + " (el archivo dice " + ignoradas + ": se ignora)";
        if (TraderZone.Name != "") s = s + " · zona " + TraderZone.Name + " (sigue " + TraderZone.FollowsVehicle.ToString() + ", radio " + TraderZone.RadiusM.ToString() + " m)";
        return s;
    }

    // El archivo de ejemplo de una instalacion nueva: un servicio APAGADO, con el formato del editor.
    static BZTRServicio Ejemplo() {
        BZTRServicio c = new BZTRServicio();
        BZTRTrader t0 = new BZTRTrader();
        BZTRTrader t = new BZTRTrader();
        BZTRStop s = new BZTRStop();
        BZTRStopTrader st = new BZTRStopTrader();
        BZTRWaypoint w = new BZTRWaypoint();
        c.Enable = 0;
        c.Name = "EXAMPLE";
        c.Route = "BZBusRoute_EXAMPLE.json";
        c.VehicleClass = "CivilianSedan";
        c.StopMode = "marks";
        c.Wave = new BZTRWave();
        c.Wave.Enabled = 0;
        c.Wave.RadiusM = BZTRDef.WAVE_RADIUS_M;
        c.Wave.AheadOnly = 1;
        c.Wave.CooldownS = BZTRDef.WAVE_COOLDOWN_S;
        c.Wave.TimeS = BZTRDef.WAVE_TIME_S;
        c.Wave.MemoryS = BZTRDef.WAVE_MEMORY_S;
        c.Wave.MinGapM = BZTRDef.WAVE_MIN_GAP_M;
        c.Wave.MaxPerLap = BZTRDef.WAVE_MAX_PER_LAP;
        c.Wave.HornOnDepart = BZTRDef.HORN_ON_DEPART;
        c.Driver = new BZTRDriver();
        c.Driver.IsTrader = 1;   // maneja el comerciante de la plaza 0 (vende sentado)
        c.Driver.AllCanDrive = 1;
        c.TraderZone = new BZTRTraderZone();
        c.TraderZone.RadiusM = 0;
        c.Traders = new array<ref BZTRTrader>();
        t0.Seat = 0;
        t0.Name = "Trader";
        t0.TraderAIClass = "ExpansionTraderAIMirek";
        t0.Loadout = "SurvivorLoadout";
        t0.m_Ropa = new array<string>();
        t0.TraderFile = "Consumables";
        t0.Faction = BZTRDef.FACCION;
        c.Traders.Insert(t0);
        t.Seat = 1;
        t.Name = "Trader 1";
        t.TraderAIClass = "ExpansionTraderAIDenis";
        t.Loadout = "SurvivorLoadout";
        t.m_Ropa = new array<string>();
        t.TraderFile = "Consumables";
        t.Faction = BZTRDef.FACCION;
        c.Traders.Insert(t);
        c.Stops = new array<ref BZTRStop>();
        s.Mark = 1;
        s.m_PosXZ = new array<float>();
        s.Mode = "always";
        s.WhoGetsOff = new array<int>();
        s.WhoGetsOff.Insert(1);
        s.BoardTimeoutS = BZTRDef.BOARD_TIMEOUT_S;
        s.Traders = new array<ref BZTRStopTrader>();
        st.Seat = 1;
        st.Waypoints = new array<ref BZTRWaypoint>();
        w.WayPointName = "M1-1";
        st.Waypoints.Insert(w);
        st.TimeS = BZTRDef.TRADER_TIME_S;
        st.MovementSpeed = "WALK";
        s.Traders.Insert(st);
        s.Signals = new array<ref BZTRStopSignal>();
        s.HornOnDepart = BZTRDef.HORN_ON_DEPART;
        c.Stops.Insert(s);
        c.OnRouteEnd = new BZTROnRouteEnd();
        c.OnRouteEnd.Action = "despawn";
        c.StartDelayS = BZTRDef.START_DELAY_S;
        c.RespawnSeconds = BZTRDef.RESPAWN_S;
        c.PlayerRadiusM = BZTRDef.PLAYER_RADIUS_M;
        c.MaxWithPlayersS = BZTRDef.MAX_WITH_PLAYERS_S;
        c.SeatedWaitS = BZTRDef.SEATED_WAIT_S;
        c.PlayerHaltM = BZTRDef.PLAYER_HALT_M;
        c.MaxStallDistanceM = BZTRDef.MAX_STALL_DISTANCE_M;
        c.FrontPlayerWaitS = BZTRDef.FRONT_PLAYER_WAIT_S;
        // las plazas del vehiculo (el CivilianSedan: el chofer y 3 mas), para que el editor lo abra sin pedirlas
        c.Editor = new BZTREditor();
        c.Editor.Version = 1;
        c.Editor.Seats = 4;
        return c;
    }
}
