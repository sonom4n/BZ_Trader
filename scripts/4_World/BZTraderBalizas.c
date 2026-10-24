// ============================================================================
//  LAS SENALES DE UNA PARADA: bengalas y humo de colores que se encienden cuando los comerciantes bajan a hacer la
//  parada, para que los jugadores la vean de lejos. Todo vanilla de la 1.30:
//    - humo de colores: M18SmokeGrenade_* y RDG2SmokeGrenade_Black (90 s de humo, EnergyManager del config);
//    - la Roadflare, roja (20 min encendida y 60 s de humo al final);
//    - los fuegos artificiales: FireworksLauncher (3 s de mecha y 16 tiros cada 3-3,35 s).
//  Y de Expansion (Missions): el humo rojo y la luz de un airdrop, que se ven de lejos (BZTRAirdrop, abajo).
//  Reglas:
//    - Se encienden al desplegar, cada una a su StartDelayS (Stops[].Signals[]).
//    - Cada EverySeconds se renueva la que se APAGO: nunca dos a la vez en el mismo lugar. Un EverySeconds mas corto
//      que lo que dura espera a que se apague; 0 = una sola vez.
//    - auto = humo de dia (con su Color) y roadflare de noche; se decide en cada encendido (World.IsNight).
//    - flaregun: el proyectil de la pistola lo crea el motor cuando dispara un arma en manos (TryFireWeapon). Hay un
//      Weapon.Fire(muzzle, pos, dir, speed) nativo que no usa nadie, sin probar sin dueno. Por ahora sale una
//      roadflare (Avisos lo dice al cargar).
//    - Al seguir, al retirar el vehiculo o al apagar el server se borran todas las que creo la parada.
//    - CreateObjectEx con ECE_NOPERSISTENCY_WORLD: nada queda guardado (como en AutoDrive). SetTakeable(false): las
//      acciones de tomar no las ofrecen (encendidas, ademas, vanilla no las deja entrar al inventario).
// ============================================================================
class BZTRBaliza {
    BZTRStopSignal m_Cfg;
    int      m_Num;          // 1 = la primera de la parada (para el log)
    EntityAI m_Obj;          // la encendida: puntero comun, si el motor la borra queda en null solo
    int      m_AirdropId = -1;   // el humo de airdrop: no es un objeto del server, es su id en los clientes
    string   m_Tipo;         // lo que se encendio: smoke · roadflare · fireworks · airdrop
    string   m_Que;          // para el log: "humo verde en x z"
    float    m_EncendidaT;
    float    m_ProxT;        // la proxima renovacion (< 0 = no se renueva)
    bool     m_Prendio;      // se la vio encendida al menos una vez
    int      m_Veces;
}

// ============================================================================
//  EL HUMO DE AIRDROP de Expansion (Missions) como senal de parada: la caja del CLIENTE de un airdrop
//  (ExpansionAirdropContainer_Client: el humo rojo y la luz de ExpansionAirdropContainerFX), creada en TODOS los
//  jugadores con el mismo RPC con que Expansion crea las suyas (ExpansionMissionModule, RPC_CreateAirdropContainer) y
//  borrada con el suyo (RPC_DeleteAirdropContainer).
//    - NO hay contenedor en el server: ni fisica, ni infectados, ni loot, ni marcador, ni patrullas, ni nada que se
//      guarde. La caja del cliente es un objeto local sin inventario: nadie la abre.
//    - Es un RPC a todos: no depende de la burbuja de red de los items (unos 150 m), se ve de lejos.
//    - El que entra durante la parada la recibe (EnviarA, desde el manager, con los datos del comercio).
//    - Enterrada: el punto "light" del modelo (de donde salen el humo y la luz) queda EMISOR_BAJO_M bajo el suelo. Las
//      alturas se miden una vez con el modelo como objeto estatico local (CreateStaticObjectUsingP3D): sin crear nada
//      de Expansion.
//    - Sin Expansion Missions (EXPANSIONMODMISSIONS) no hay: la senal dice por que y no se enciende.
// ============================================================================
class BZTRAirdropActivo {
    int    m_Id;
    vector m_Pos;
    vector m_Ori;
}

class BZTRAirdrop {
    static const string CLASE_CLIENTE = "ExpansionAirdropContainer_Client";
    static const float  TOPE_DEF_M    = 1.07;    // si no se pudo medir: la tapa, con el origen del modelo en la base
    static const float  EMISOR_BAJO_M = 0.02;    // el punto light, apenas bajo el suelo
    static const float  CAIDA_MS      = 4.5;     // la que manda Expansion (aca no cae: ya llego)
    static bool   s_Medido;
    static bool   s_Dicho;
    static float  s_Piso = 0;                    // la base de la caja, desde el origen del modelo
    static float  s_Tope = TOPE_DEF_M;           // la tapa, desde el origen
    static float  s_Luz = TOPE_DEF_M;            // el punto light, desde el origen
    static string s_Medida = "";
    static ref array<ref BZTRAirdropActivo> s_Activos = new array<ref BZTRAirdropActivo>();

    // Si se puede encender: Expansion Missions cargado. Sin el, la senal no se intenta (no es algo que se arregle solo).
    static bool Disponible() {
#ifdef EXPANSIONMODMISSIONS
        if (ExpansionMissionModule.s_Instance) return true;
#endif
        return false;
    }

    // Crea la caja con humo en todos los clientes. El id, o -1 (que = el porque). que = lo del log.
    static int Crear(vector p, out string que) {
        BZTRAirdropActivo a;
        float suelo;
        float baseY;        // donde va el origen del modelo
        float asoma;
        float bajo;
        float tapa;
        int id = -1;
        que = "";
#ifdef EXPANSIONMODMISSIONS
        if (!ExpansionMissionModule.s_Instance) {
            que = "humo de airdrop: el modulo de misiones de Expansion no esta cargado";
            return -1;
        }
        Medir();
        suelo = GetGame().SurfaceY(p[0], p[2]);
        baseY = suelo - s_Luz - EMISOR_BAJO_M;
        id = ExpansionAirdropContainerBase.s_Expansion_AirdropContainerNextID;
        ExpansionAirdropContainerBase.s_Expansion_AirdropContainerNextID = id + 1;
        a = new BZTRAirdropActivo();
        a.m_Id = id;
        a.m_Pos = Vector(p[0], baseY, p[2]);
        a.m_Ori = "0 0 0";
        s_Activos.Insert(a);
        EnviarCrear(a, null);
        bajo = suelo - baseY - s_Piso;
        asoma = baseY + s_Tope - suelo;
        tapa = suelo - baseY - s_Tope;
        que = "humo de airdrop (contenedor " + CLASE_CLIENTE + " id " + id.ToString();
        que = que + ", enterrado: la base a " + MetrosCm(bajo) + " bajo el suelo, el humo sale a " + Centimetros(EMISOR_BAJO_M) + " bajo el suelo";
        if (asoma > 0.005) que = que + ", la caja ASOMA " + Centimetros(asoma);
        else que = que + ", la tapa a " + Centimetros(tapa) + " bajo el suelo";
        que = que + ") en " + BZTRLog.XZ(p) + " · creado en los clientes";
        if (!s_Dicho) {
            s_Dicho = true;
            que = que + " · " + s_Medida;
        }
        return id;
#else
        que = "humo de airdrop: hace falta Expansion Missions (no esta)";
        return -1;
#endif
    }

    // Lo borra en todos los clientes (al seguir, al retirar, al apagar).
    static void Borrar(int id) {
        int i;
#ifdef EXPANSIONMODMISSIONS
        ExpansionScriptRPC rpc;
        if (ExpansionMissionModule.s_Instance) {
            rpc = ExpansionMissionModule.s_Instance.Expansion_CreateRPC("RPC_DeleteAirdropContainer");
            rpc.Write(id);
            rpc.Expansion_Send(true, null);
        }
#endif
        for (i = s_Activos.Count() - 1; i >= 0; i--) {
            if (s_Activos.Get(i).m_Id == id) s_Activos.Remove(i);
        }
    }

    // Al jugador que entra: los que estan encendidos (el cliente ignora un id que ya tiene). Cuantos.
    static int EnviarA(PlayerIdentity ident) {
        int i;
        if (!ident) return 0;
        for (i = 0; i < s_Activos.Count(); i++) EnviarCrear(s_Activos.Get(i), ident);
        return s_Activos.Count();
    }

    // el RPC de Expansion (ExpansionAirdropContainerBase.Expansion_SendCreateContainerOnClient escribe lo mismo):
    // ya sin paracaidas y con el humo prendido. ident null = a todos.
    private static void EnviarCrear(BZTRAirdropActivo a, PlayerIdentity ident) {
#ifdef EXPANSIONMODMISSIONS
        ExpansionScriptRPC rpc;
        string clase = CLASE_CLIENTE;
        float caida = CAIDA_MS;
        float viento = 0;
        bool humo = true;
        bool sinParacaidas = true;
        if (!ExpansionMissionModule.s_Instance) return;
        rpc = ExpansionMissionModule.s_Instance.Expansion_CreateRPC("RPC_CreateAirdropContainer");
        rpc.Write(a.m_Id);
        rpc.Write(clase);
        rpc.Write(a.m_Pos);
        rpc.Write(a.m_Ori);
        rpc.Write(caida);
        rpc.Write(viento);
        rpc.Write(humo);
        rpc.Write(sinParacaidas);
        rpc.Expansion_Send(true, ident);
#endif
    }

    // Una vez: la tapa y el punto light del modelo, con el modelo como objeto estatico LOCAL (no viaja a los clientes,
    // no es de Expansion) que se borra en el acto. Si no se puede, quedan los valores por defecto (TOPE_DEF_M).
    static void Medir() {
        string modelo = "";
        Object o;
        vector caja[2];
        vector luz;
        bool hayCaja;
        bool hayLuz;
        float alto;
        string linea;
        if (s_Medido) return;
        s_Medido = true;
        GetGame().ConfigGetText("CfgVehicles " + CLASE_CLIENTE + " model", modelo);
        // el config lo da con la barra inicial; los p3d sueltos se piden sin ella (como ObjectSpawner)
        if (modelo.IndexOf("\\") == 0 || modelo.IndexOf("/") == 0) modelo = modelo.Substring(1, modelo.Length() - 1);
        if (modelo == "") {
            s_Medida = "sin el model de " + CLASE_CLIENTE + " en CfgVehicles: tapa y humo a " + MetrosCm(TOPE_DEF_M) + " de la base (por defecto)";
            return;
        }
        o = GetGame().CreateStaticObjectUsingP3D(modelo, "0 3000 0", "0 0 0", 1.0, true);
        if (!o) {
            s_Medida = "no se pudo medir " + modelo + ": tapa y humo a " + MetrosCm(TOPE_DEF_M) + " de la base (por defecto)";
            return;
        }
        hayCaja = o.GetCollisionBox(caja);
        if (hayCaja) {
            s_Piso = caja[0][1];
            s_Tope = caja[1][1];
        }
        hayLuz = o.MemoryPointExists("light");
        s_Luz = s_Tope;
        if (hayLuz) {
            luz = o.GetMemoryPointPos("light");
            s_Luz = luz[1];
        }
        GetGame().ObjectDelete(o);
        alto = s_Tope - s_Piso;
        linea = "la caja (" + modelo + ")";
        if (hayCaja) linea = linea + " mide " + MetrosCm(alto) + " de alto: de " + MetrosCm(s_Piso) + " a " + MetrosCm(s_Tope) + " desde su origen";
        else linea = linea + " sin caja de colision: la tapa a " + MetrosCm(s_Tope) + " (por defecto)";
        linea = linea + "; el humo y la luz salen del punto light, a " + MetrosCm(s_Luz) + " desde su origen";
        if (!hayLuz) linea = linea + " (el modelo no tiene el punto light: se toma la tapa)";
        s_Medida = linea;
    }

    // al centimetro (BZTRLog.Metros redondea al decimetro, y el emisor esta a 2 cm)
    static string MetrosCm(float m) {
        float r = Math.Round(m * 100) / 100;
        return r.ToString() + " m";
    }

    static string Centimetros(float m) {
        float r = Math.Round(m * 100);
        return r.ToString() + " cm";
    }
}

class BZTRBalizas {
    static const float FUEGOS_DURAN_S = 75;   // 3 s de mecha + 16 tiros de 3-3,35 s (FireworksLauncher.c, 1.30)
    static const float RENOVAR_MIN_S  = 10;   // entre dos encendidos de la misma, como minimo
    static const float REINTENTO_S    = 30;   // si no se pudo crear, se reintenta a los 30 s

    private ref array<ref BZTRBaliza> m_Lista;
    private string m_Quien;
    private string m_Parada;

    void BZTRBalizas() {
        m_Lista = new array<ref BZTRBaliza>();
    }

    int Cuantas() {
        return m_Lista.Count();
    }

    // Al desplegar. Devuelve la linea del log sin el "PARADA (...): " ("" = la parada no tiene senales).
    string Encender(float now, BZTRStop s, string quien, string parada) {
        int i;
        int n;
        int prendidas = 0;
        int luego = 0;
        BZTRStopSignal sg;
        BZTRBaliza b;
        string falla;
        string lista = "";
        string listaLuego = "";
        string fallas = "";
        string sinPos = "";
        string sinMisiones = "";
        string linea;
        string quedaron = Apagar();
        if (quedaron != "") BZTRLog.Warn(quien, "PARADA (" + parada + "): " + quedaron + " de la parada anterior");
        m_Quien = quien;
        m_Parada = parada;
        if (!s || !s.Signals || s.Signals.Count() == 0) return "";
        n = s.Signals.Count();
        if (n > BZTRDef.SIGNALS_MAX) n = BZTRDef.SIGNALS_MAX;
        for (i = 0; i < n; i++) {
            sg = s.Signals.Get(i);
            if (!sg) continue;
            b = new BZTRBaliza();
            b.m_Cfg = sg;
            b.m_Num = i + 1;
            b.m_ProxT = now + sg.StartDelayS;
            if (!sg.TienePos()) {
                sinPos = sinPos + " " + b.m_Num.ToString();
                continue;
            }
            if (sg.m_Tipo == "airdrop" && !BZTRAirdrop.Disponible()) {
                sinMisiones = sinMisiones + " " + b.m_Num.ToString();
                continue;
            }
            m_Lista.Insert(b);
            if (sg.StartDelayS > 0) {
                luego = luego + 1;
                listaLuego = listaLuego + ", " + Nombre(sg.m_Tipo, sg.Color, false) + " a los " + BZTRLog.Seg(sg.StartDelayS);
                continue;
            }
            falla = Prender(b, now);
            if (falla != "") {
                b.m_ProxT = now + REINTENTO_S;
                fallas = fallas + " " + b.m_Num.ToString() + "(" + falla + ")";
                continue;
            }
            prendidas = prendidas + 1;
            lista = lista + ", " + b.m_Que;
        }
        linea = "señales: " + prendidas.ToString() + " encendidas";
        if (lista != "") linea = linea + " (" + lista.Substring(2, lista.Length() - 2) + ")";
        if (luego > 0) linea = linea + " · " + luego.ToString() + " despues (" + listaLuego.Substring(2, listaLuego.Length() - 2) + ")";
        linea = linea + " · " + Renovacion(s, n);
        if (fallas != "") linea = linea + " · NO SE CREARON:" + fallas + " (se reintenta a los " + BZTRLog.Seg(REINTENTO_S) + ")";
        if (sinPos != "") linea = linea + " · sin Pos (no se encienden):" + sinPos;
        if (sinMisiones != "") linea = linea + " · humo de airdrop sin Expansion Missions (no se encienden):" + sinMisiones;
        if (s.Signals.Count() > n) linea = linea + " · de " + s.Signals.Count().ToString() + " se encienden las primeras " + n.ToString();
        return linea;
    }

    // Mientras dura la parada (cada tick): renueva la que se apago y enciende las que tenian retardo.
    void Tick(float now) {
        int i;
        BZTRBaliza b;
        string falla;
        string linea;
        int vez;
        for (i = 0; i < m_Lista.Count(); i++) {
            b = m_Lista.Get(i);
            if (Activa(b, now)) {
                b.m_Prendio = true;
                continue;
            }
            if (b.m_ProxT < 0 || now < b.m_ProxT) continue;
            if (b.m_Veces > 0 && !b.m_Prendio) BZTRLog.Warn(m_Quien, "PARADA (" + m_Parada + "): señal " + b.m_Num.ToString() + " (" + b.m_Que + "): no se la vio encendida en " + BZTRLog.Seg(now - b.m_EncendidaT) + " (¿mojada? ¿la borro el CE?)");
            linea = "encendida a los " + BZTRLog.Seg(b.m_Cfg.StartDelayS);
            vez = b.m_Veces + 1;
            if (b.m_Veces > 0) linea = "renovada (vez " + vez.ToString() + "; la anterior, encendida hace " + BZTRLog.Seg(now - b.m_EncendidaT) + ", se apago)";
            Borrar(b);
            falla = Prender(b, now);
            if (falla != "") {
                b.m_ProxT = now + REINTENTO_S;
                BZTRLog.Warn(m_Quien, "PARADA (" + m_Parada + "): señal " + b.m_Num.ToString() + " NO SE CREO (" + falla + "): se reintenta a los " + BZTRLog.Seg(REINTENTO_S));
                continue;
            }
            BZTRLog.Info(m_Quien, "PARADA (" + m_Parada + "): señal " + b.m_Num.ToString() + " " + linea + ": " + b.m_Que);
        }
    }

    // Al seguir (o al retirar el vehiculo): las borra todas. Devuelve la linea del log ("" = no habia).
    string Apagar() {
        int i;
        int total = m_Lista.Count();
        int borradas = 0;
        int faltan;
        BZTRBaliza b;
        EntityAI obj;
        string linea;
        if (total == 0) return "";
        for (i = 0; i < total; i++) {
            b = m_Lista.Get(i);
            obj = b.m_Obj;
            if (obj && !obj.IsSetForDeletion()) borradas = borradas + 1;
            else if (b.m_AirdropId >= 0) borradas = borradas + 1;
            Borrar(b);
        }
        m_Lista.Clear();
        linea = "señales: " + borradas.ToString() + " apagadas y borradas";
        faltan = total - borradas;
        if (faltan > 0) linea = linea + " (" + faltan.ToString() + " ya no estaba(n) o no llego a encenderse)";
        return linea;
    }

    // ------------------------------------------------------------------------
    private string Prender(BZTRBaliza b, float now) {
        BZTRStopSignal sg = b.m_Cfg;
        string tipo = sg.m_Tipo;
        string clase;
        bool ok;
        bool noche = false;
        vector p = sg.PosMundo(ok);
        Object o;
        EntityAI e;
        ItemBase ib;
        FireworksLauncher fl;
        ComponentEnergyManager em;
        World mundo;
        if (!ok) return "sin Pos";
        if (tipo == "auto") {
            mundo = GetGame().GetWorld();
            noche = mundo.IsNight();
            tipo = "smoke";
            if (noche) tipo = "roadflare";
        }
        if (tipo == "flaregun") tipo = "roadflare";
        if (tipo == "airdrop") return PrenderAirdrop(b, p, now);
        clase = Clase(tipo, sg.Color);
        o = GetGame().CreateObjectEx(clase, p, ECE_PLACE_ON_SURFACE | ECE_NOPERSISTENCY_WORLD);
        e = EntityAI.Cast(o);
        if (!e) {
            if (o) GetGame().ObjectDelete(o);
            return "CreateObjectEx(" + clase + ") no devolvio un EntityAI";
        }
        b.m_Obj = e;
        b.m_Tipo = tipo;
        b.m_Que = Nombre(tipo, sg.Color, true) + " en " + BZTRLog.XZ(p);
        if (sg.m_Tipo == "auto") {
            if (noche) b.m_Que = b.m_Que + " (auto: de noche, " + Hora() + ")";
            else b.m_Que = b.m_Que + " (auto: de dia, " + Hora() + ")";
        }
        if (sg.m_Tipo == "flaregun") b.m_Que = b.m_Que + " (flaregun: roadflare)";
        b.m_EncendidaT = now;
        b.m_Prendio = false;
        b.m_Veces = b.m_Veces + 1;
        b.m_ProxT = -1;
        if (sg.EverySeconds > 0) b.m_ProxT = now + Math.Max(sg.EverySeconds, RENOVAR_MIN_S);
        ib = ItemBase.Cast(e);
        if (ib) ib.SetTakeable(false);
        if (tipo == "fireworks") {
            e.SetOrientation("0 0 0");   // parado: los tiros salen hacia arriba del modelo
            fl = FireworksLauncher.Cast(e);
            if (!fl) return clase + " no es un FireworksLauncher";
            fl.OnIgnitedThis(null);
            b.m_Prendio = true;
            return "";
        }
        em = e.GetCompEM();
        if (!em) return clase + " no tiene EnergyManager";
        em.SwitchOn();
        if (em.IsWorking()) b.m_Prendio = true;
        return "";
    }

    // el humo de airdrop: en los clientes, encendido hasta que se lo borra (no se apaga solo: no se renueva)
    private string PrenderAirdrop(BZTRBaliza b, vector p, float now) {
        string que;
        int id = BZTRAirdrop.Crear(p, que);
        if (id < 0) return que;
        b.m_Obj = null;
        b.m_AirdropId = id;
        b.m_Tipo = "airdrop";
        b.m_Que = que;
        b.m_EncendidaT = now;
        b.m_Prendio = true;
        b.m_Veces = b.m_Veces + 1;
        b.m_ProxT = -1;
        return "";
    }

    private bool Activa(BZTRBaliza b, float now) {
        ComponentEnergyManager em;
        EntityAI obj = b.m_Obj;
        if (b.m_Tipo == "airdrop") return b.m_AirdropId >= 0;
        if (!obj || obj.IsSetForDeletion()) return false;
        if (b.m_Tipo == "fireworks") return now - b.m_EncendidaT < FUEGOS_DURAN_S;
        em = obj.GetCompEM();
        if (!em) return false;
        return em.IsWorking();
    }

    private void Borrar(BZTRBaliza b) {
        EntityAI obj = b.m_Obj;
        if (obj && !obj.IsSetForDeletion()) GetGame().ObjectDelete(obj);
        b.m_Obj = null;
        if (b.m_AirdropId >= 0) BZTRAirdrop.Borrar(b.m_AirdropId);
        b.m_AirdropId = -1;
    }

    // "se renuevan cada 60 s" (todas iguales) o el detalle de cada una. El humo de airdrop no cuenta: no se renueva.
    private string Renovacion(BZTRStop s, int n) {
        int i;
        int k;
        BZTRStopSignal sg;
        float primera = -1;
        bool iguales = true;
        string det = "";
        string deAirdrop = "";
        for (i = 0; i < n; i++) {
            sg = s.Signals.Get(i);
            if (!sg || !sg.TienePos()) continue;
            k = i + 1;
            if (sg.m_Tipo == "airdrop") {
                deAirdrop = deAirdrop + " " + k.ToString();
                continue;
            }
            if (primera < 0) primera = sg.EverySeconds;
            if (sg.EverySeconds != primera) iguales = false;
            if (sg.EverySeconds > 0) det = det + " " + k.ToString() + ":" + BZTRLog.Seg(sg.EverySeconds);
            else det = det + " " + k.ToString() + ":una vez";
        }
        if (deAirdrop != "") deAirdrop = "humo de airdrop:" + deAirdrop + " hasta que sigue (no se renueva)";
        if (primera < 0 && deAirdrop != "") return deAirdrop;
        if (primera < 0) return "ninguna con Pos";
        if (deAirdrop != "") deAirdrop = " · " + deAirdrop;
        if (iguales && primera <= 0) return "una sola vez (no se renuevan)" + deAirdrop;
        if (iguales) return "se renuevan cada " + BZTRLog.Seg(primera) + " (la que se apago)" + deAirdrop;
        return "se renuevan (la que se apago) cada:" + det + deAirdrop;
    }

    static string Clase(string tipo, string color) {
        if (tipo == "roadflare") return "Roadflare";
        if (tipo == "fireworks") return "FireworksLauncher";
        if (tipo == "airdrop") return "ExpansionAirdropContainer_Client";
        if (color == "purple" || color == "blue") return "M18SmokeGrenade_Purple";   // no hay humo azul en la 1.30
        if (color == "red") return "M18SmokeGrenade_Red";
        if (color == "white") return "M18SmokeGrenade_White";
        if (color == "yellow") return "M18SmokeGrenade_Yellow";
        if (color == "black") return "RDG2SmokeGrenade_Black";
        return "M18SmokeGrenade_Green";
    }

    // El nombre para el log. creada = el tipo ya resuelto (auto -> humo o roadflare).
    static string Nombre(string tipo, string color, bool creada) {
        if (tipo == "roadflare" || tipo == "flaregun") return "roadflare";
        if (tipo == "fireworks") return "fuegos";
        if (tipo == "airdrop") return "humo de airdrop";
        if (tipo == "auto" && !creada) return "auto " + ColorTexto(color);
        return "humo " + ColorTexto(color);
    }

    static string ColorTexto(string color) {
        if (color == "purple") return "violeta";
        if (color == "blue") return "violeta (no hay azul)";
        if (color == "red") return "rojo";
        if (color == "white") return "blanco";
        if (color == "yellow") return "amarillo";
        if (color == "black") return "negro";
        return "verde";
    }

    // La hora del mundo (HH:MM), para leer el auto en el log.
    static string Hora() {
        int anio;
        int mes;
        int dia;
        int hh;
        int mm;
        string s;
        World mundo = GetGame().GetWorld();
        mundo.GetDate(anio, mes, dia, hh, mm);
        s = hh.ToString() + ":";
        if (mm < 10) s = s + "0";
        return s + mm.ToString();
    }
}
