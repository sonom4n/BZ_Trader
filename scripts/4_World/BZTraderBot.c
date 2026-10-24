// ============================================================================
//  UN COMERCIANTE DEL SERVICIO (una plaza del vehiculo).
//
//  Arriba es de AutoDrive (la Crew de la ruta: la crea, la sube y no arranca hasta que
//  esta toda sentada). Desde que se baja en una parada es de BZ_Trader:
//    BAJANDO    Notify_Transport(null, -1) + puerta + GetOutVehicle + GRUPO PROPIO
//    CAMINANDO  waypoints de su puesto en su grupo (el esquema de TTE), ONCE
//    VENDIENDO  el alto de TTE: quieto y mirando a quien se acerque; minimo TimeS, mas
//               tiempo mientras haya jugadores cerca, con tope
//    VOLVIENDO  waypoint a SU puerta (CrewEntryWS) y, a 2 m, sube ANIMADO (SentarEnPlaza)
//    SUBIENDO   hasta que termina la animacion de subida
//  Rescate por teleport (SentarEnPlaza con teleport) si no llega a la puerta en
//  BoardTimeoutS o no termina de subir. Toda fase tiene tope.
//
//  ⛔ EL GRUPO PROPIO NO ES OPCIONAL: en el grupo de Boris (lider sentado), eAI vuelve a
//  subir al bot solo (eAIState_Vehicles). Es lo que le paso a la escolta de guard_stop.
//  ⛔ La formacion va en NONE al bajar y en IN recien con el bot a pie (con IN puesto antes,
//  la escolta volvia al asiento a los 3,6 s: FormacionGuardia de AutoDrive).
//  El chofer comerciante (plaza 0) NO cambia de grupo: es el lider del grupo del runner y
//  AutoDrive lo usa. Se le cambian los waypoints (SetNextWaypoint le deja el de la ruta).
//  ★ al lanzar, el que se encuentra por clase sale del grupo de Boris (Apartar) y se
//  sienta un tick despues de verlo; a pie, su grupo tiene siempre DOS waypoints (Despierto) para
//  que eAI no lo duerma; sentado, su puerta se cierra y se sostiene cerrada.
// ============================================================================
class BZTRFase {
    static const int SENTADO   = 0;   // en su plaza (o todavia sin enganchar)
    static const int BAJANDO   = 1;
    static const int CAMINANDO = 2;
    static const int VENDIENDO = 3;
    static const int VOLVIENDO = 4;
    static const int SUBIENDO  = 5;
    static const int TELEPORT  = 6;   // sentado por teleport: se confirma que quedo en la plaza
    static const int PERDIDO   = 7;   // no se lo pudo volver a sentar: la parada sigue sin el

    static string Nombre(int f) {
        if (f == SENTADO)   return "SENTADO";
        if (f == BAJANDO)   return "BAJANDO";
        if (f == CAMINANDO) return "CAMINANDO";
        if (f == VENDIENDO) return "VENDIENDO";
        if (f == VOLVIENDO) return "VOLVIENDO";
        if (f == SUBIENDO)  return "SUBIENDO";
        if (f == TELEPORT)  return "TELEPORT";
        if (f == PERDIDO)   return "PERDIDO";
        return "?";
    }
}

// Jugadores vivos, conscientes y con identidad a <= radio de un punto (los bots no cuentan).
class BZTRJugadores {
    static int Cerca(vector p, float radio, array<Man> salida) {
        array<Man> js = new array<Man>();
        PlayerBase pb;
        int n = 0;
        int i;
        if (p[0] == 0 && p[2] == 0) return 0;
        GetGame().GetPlayers(js);
        for (i = 0; i < js.Count(); i++) {
            pb = PlayerBase.Cast(js.Get(i));
            if (!pb) continue;
            if (!pb.GetIdentity()) continue;
            if (!pb.IsAlive()) continue;
            if (pb.IsUnconscious()) continue;
            if (vector.Distance(pb.GetPosition(), p) > radio) continue;
            n = n + 1;
            if (salida) salida.Insert(pb);
        }
        return n;
    }

    // ★ Los jugadores A BORDO de un vehiculo, de cualquier clase: sentados en una plaza (CrewMember), sentados sin
    // figurar en ninguna (su comando de vehiculo o su padre es el vehiculo: lo que deja el teleport de vanilla al
    // pisarle la plaza a alguien), parados ARRIBA (enganchados por el motor a lo que pisan: la caja de un camion, el
    // techo, el piso del bus) y parados ADENTRO de la caja del casco sin enganche (saltando, o soltados por la
    // velocidad; sin casco no se mira). Los bots no cuentan. Devuelve cuantos; los anota en sentados y parados.
    static int ABordo(Transport tr, BZTRCasco casco, array<PlayerBase> sentados, array<PlayerBase> parados) {
        array<Man> js = new array<Man>();
        PlayerBase pb;
        HumanCommandVehicle hcv;
        vector l;
        float dy;
        int i;
        if (!tr) return 0;
        GetGame().GetPlayers(js);
        for (i = 0; i < js.Count(); i++) {
            pb = PlayerBase.Cast(js.Get(i));
            if (!pb || eAIBase.Cast(pb) || !pb.GetIdentity()) continue;
            hcv = pb.GetCommand_Vehicle();
            if (pb.GetParent() == tr || EnPlaza(tr, pb) || (hcv && hcv.GetTransport() == tr)) {
                sentados.Insert(pb);
                continue;
            }
            if (Enganchado(tr, pb)) {
                parados.Insert(pb);
                continue;
            }
            if (!casco || pb.IsInTransport()) continue;
            l = BZTRCasco.ALocal(tr, pb.GetPosition());
            dy = pb.GetPosition()[1] - tr.GetPosition()[1];
            if (casco.Dentro(l, 0) && dy >= -1 && dy <= 4) parados.Insert(pb);
        }
        return sentados.Count() + parados.Count();
    }

    // ★ ¿Parado ARRIBA del vehiculo? El motor engancha a un jugador a lo que pisa (Human.PhysicsGetLinkedEntity; su
    // GetParent sigue vacio), en cualquier clase: es el enganche que usa Expansion (Expansion_GetParent). Tambien lo que
    // tiene debajo (PhysicsGetFloorEntity). Lo que pisa puede ser una pieza del vehiculo: se sube por los padres.
    static bool Enganchado(Transport tr, Human h) {
        if (!tr || !h) return false;
        if (EsDelVehiculo(tr, h.PhysicsGetLinkedEntity())) return true;
        return EsDelVehiculo(tr, h.PhysicsGetFloorEntity());
    }

    // el vehiculo, o algo que cuelga de el (hasta 4 niveles)
    static bool EsDelVehiculo(Transport tr, IEntity pieza) {
        int k;
        for (k = 0; k < 4 && pieza; k++) {
            if (pieza == tr) return true;
            pieza = pieza.GetParent();
        }
        return false;
    }

    // ¿Esta sentado en alguna plaza del vehiculo (la tabla de tripulacion)?
    static bool EnPlaza(Transport tr, Man m) {
        int i;
        if (!tr || !m) return false;
        for (i = 0; i < tr.CrewSize(); i++) {
            if (tr.CrewMember(i) == m) return true;
        }
        return false;
    }

    // "nombre1, nombre2" (para el log y el aviso)
    static string Nombres(array<PlayerBase> js) {
        string s = "";
        int i;
        PlayerBase pb;
        for (i = 0; i < js.Count(); i++) {
            pb = js.Get(i);
            if (!pb || !pb.GetIdentity()) continue;
            if (s != "") s = s + ", ";
            s = s + pb.GetIdentity().GetName();
        }
        return s;
    }

    // distancia horizontal al cuadrado (la puerta esta a la altura de la manija, no del piso)
    static float DistXZ2(vector a, vector b) {
        float dx = a[0] - b[0];
        float dz = a[2] - b[2];
        return dx * dx + dz * dz;
    }
}

class BZTRBot {
    static const float BAJAR_MAX_S           = 8;
    static const float LLEGADA_M             = 2.0;   // cerca del puesto: llego si ademas ya esta quieto (una pared, un kiosco)
    static const float LLEGADA_CERCA_M       = 1.0;   // ... o a esto del puesto, aunque todavia camine
    static const float LLEGADA_QUIETO_MS     = 0.3;   // "quieto": menos que esto, en m/s
    static const float SIN_MOVERSE_S         = 20;
    static const float MOVIO_M               = 0.5;
    static const float CAMINO_MAX_S          = 120;
    static const float PUERTA_M2             = 4.0;   // 2 m en XZ: el umbral de StepCrewBoarding de AutoDrive
    static const float SUBIR_MAX_S           = 10;
    static const float TELEPORT_CONFIRMA_S   = 5;
    static const float PUERTA_SOSTENER_S     = 5;     // sentado: su puerta se sostiene cerrada
    static const float APROX_LLEGADA_M2      = 1.44;  // llego a su punto de aproximacion (1,2 m en XZ)
    static const float TURNO_S               = 2;     // en una puerta compartida, entre uno que se sienta y el siguiente
    static const float PLAZA_LOG_S           = 30;    // y cada cuanto lo dice el log
    static const float DERIVA_M              = 1.5;   // vendiendo, apartado mas que esto del punto donde empezo: vuelve
    static const float DERIVA_VUELTO_M       = 1.2;   // y de vuelta a menos que esto (eAI da el waypoint por alcanzado a 1 m), se frena y mira su rumbo
    static const float DERIVA_VOLVER_S       = 15;    // si en este tiempo no volvio, vende donde esta

    ref BZTRTrader m_Cfg;
    int      m_Seat;
    bool     m_Chofer;
    eAIBase  m_Bot;            // el bot enganchado (puntero comun: lo borra el motor)
    bool     m_Enganchado;
    int      m_Enganches;
    bool     m_VestirPendiente;
    float    m_VestirT;
    eAIGroup m_Grupo;          // su grupo propio (crew); puntero comun, como AutoDrive
    int      m_FaccionVeces;   // en esta vuelta, cuantas veces hubo que ponerle la faccion a su grupo (ver Facciones)
    bool     m_PorClase;       // al lanzar se lo encontro por clase y se lo sento por teleport
    float    m_PorClaseT;
    bool     m_AvisoClase;
    // el teleport en el cuadro en que AutoDrive lo crea lo deja fantasma; se lo sienta un tick despues de verlo
    bool     m_SentarPendiente;
    float    m_VistoT;
    bool     m_VistoAviso;
    // los intentos de volver a sentarlo antes de que el vehiculo siga
    int      m_Intentos;
    float    m_IntentoT;
    // lo que Pacificar le borro la ultima vez ("" = nada), para el log de quien lo llamo
    string   m_Pacificado;
    // el casco del vehiculo (lo mide el runtime al lanzar; puntero comun) y la puerta de su plaza
    ref BZTRCasco m_Casco;   // compartido con el runtime (datos fijos de este vehiculo)
    int      m_GrupoPuerta;    // la plaza mas chica de las que comparten su puerta (la suya si no comparte)
    int      m_PuertaOcupada;  // la plaza que esta usando su puerta compartida ahora (-1 = libre); la pone el runtime
    float    m_SentadoT;       // cuando quedo sentado por ultima vez
    bool     m_TieneAprox;
    vector   m_Aprox;          // su punto de aproximacion en esta parada (mundo)
    string   m_LadoPuerta;     // por donde sube a la vuelta (para el log)
    bool     m_TurnoAvisado;
    // la salida MEDIDA en esta parada (donde quedo parado a pie al bajar, en coordenadas del vehiculo)
    bool     m_TieneSalida;
    vector   m_SalidaLocal;
    bool     m_SalidaAdentro;  // adentro de la caja (el pasillo del bus): va derecho, sin rodeo
    // el rumbo de su puesto mientras vende (OverrideTurnTarget de eAI)
    bool     m_Rumbo;
    float    m_RumboYaw;       // en el angulo de eAI (VectorToAngles de la direccion)
    float    m_RumboDeg;       // el mismo, como rumbo de DayZ (0 = norte, horario), para el log
    string   m_RumboComo;
    float    m_RumboT;         // desde cuando lo sostiene (-1 = se repuso tras el alto: se toma en el tick)
    int      m_RumboMedidas;   // 0 falta +3 s · 1 falta +15 s · 2 listo
    // lo que se le saco de las manos (punteros comunes) y lo que eAI le volvio a sacar (se avisa una vez)
    ref array<ItemBase> m_Guardados;
    ref array<ItemBase> m_Revueltos;
    // el estado de su plaza desde que se lo sienta
    string   m_NoBajo;         // por que Bajar devolvio false (EstadoPlaza en ese momento)
    float    m_MedirT0;        // > 0: se mide a +1 s y a +5 s de sentarlo
    int      m_Medidas;        // 0 falta +1 s · 1 falta +5 s

    // la parada en curso
    int      m_Fase;
    float    m_FaseT0;
    float    m_BajoT;
    ref BZTRStopTrader m_Parada;   // su puesto en esta parada (null = vende donde bajo)
    float    m_TiempoMin;
    float    m_Tope;
    float    m_RadioJug;
    float    m_AltoM;          // PlayerHaltM del servicio (0 = no se frena frente a un jugador)
    float    m_BoardTimeoutS;
    int      m_Velocidad;
    vector   m_Puesto;
    vector   m_UltPos;
    float    m_MovioT;
    float    m_VendeT0;
    bool     m_Alto;
    bool     m_Frenado;        // el freno de eAI puesto por nosotros (eAI_SetHalt): vendiendo, no se traslada
    float    m_DerivaT0;       // desde cuando vuelve caminando a donde vende (se aparto)
    ref array<Man> m_Mirando;
    string   m_ComoSubio;
    // la puerta de su plaza. ⛔ HAY QUE CERRARLA APENAS ESTA A PIE: el bot que subio solo al spawn
    // (seguidor de Boris) queda en el estado "Vehicles" de eAI, y ese estado NO termina mientras la puerta de su
    // plaza siga abierta ("Wait until AI has left the vehicle and closed the door", eAIState_Vehicles.OnUpdateEx).
    // Con la puerta abierta el bot no pasa a Idle ni recorre waypoints: quieto al lado del auto, que es lo que se vio.
    bool     m_TienePuerta;
    string   m_PuertaSrc;
    float    m_CerrarHasta;
    // diagnostico
    vector   m_Objetivo;
    // su plaza ocupada por otro (un jugador): el aviso al que la ocupa y la linea del log
    float    m_PlazaLogT;
    // vendiendo: el punto donde empezo (su waypoint, en HALT) y si se aparto de el (Deriva)
    vector   m_PuntoVenta;
    bool     m_Derivado;
    int      m_Derivas;

    void BZTRBot(BZTRTrader cfg) {
        m_Cfg = cfg;
        m_Seat = cfg.Seat;
        m_Chofer = (cfg.Seat == 0);
        m_Fase = BZTRFase.SENTADO;
        m_Mirando = new array<Man>();
        m_Guardados = new array<ItemBase>();
        m_Revueltos = new array<ItemBase>();
    }

    string Quien() {
        return "plaza " + m_Seat.ToString() + " (" + m_Cfg.Name + ")";
    }

    // Vehiculo nuevo (relanzamiento): los bots son otros.
    void Reiniciar() {
        m_Bot = null;
        m_PlazaLogT = 0;
        m_Derivado = false;
        m_Derivas = 0;
        m_Enganchado = false;
        m_VestirPendiente = false;
        m_Grupo = null;
        m_FaccionVeces = 0;
        m_PorClase = false;
        m_PorClaseT = 0;
        m_AvisoClase = false;
        m_SentarPendiente = false;
        m_VistoT = 0;
        m_VistoAviso = false;
        m_Intentos = 0;
        m_IntentoT = 0;
        m_Pacificado = "";
        m_Casco = null;
        m_GrupoPuerta = m_Seat;
        m_PuertaOcupada = -1;
        m_SentadoT = 0;
        m_TieneAprox = false;
        m_LadoPuerta = "";
        m_TurnoAvisado = false;
        m_TieneSalida = false;
        m_SalidaAdentro = false;
        m_Rumbo = false;
        m_RumboMedidas = 0;
        m_Guardados.Clear();
        m_Revueltos.Clear();
        m_CerrarHasta = 0;
        m_NoBajo = "";
        m_MedirT0 = 0;
        m_Medidas = 0;
        if (m_Frenado) Frenar(false);
        m_Fase = BZTRFase.SENTADO;
        m_Parada = null;
        m_Alto = false;
        m_Mirando.Clear();
    }

    // ★ las manos vacias, en cualquier fase. Lo que tenga en la mano va a la ranura de la espalda (Melee)
    // o del hombro (Shoulder) si esta libre, si no al inventario, y si no entra se borra. Con eAI_TakeItemToLocation, el
    // mismo movimiento que usa eAI (arregla el estado de las manos y la sincronia). Devuelve la linea para el log: la
    // primera vez de cada objeto, y si eAI lo vuelve a sacar (eso diria que el corte de BZTraderManos.c no alcanza).
    string VaciarManos() {
        HumanInventory invManos;
        GameInventory invBot;
        EntityAI cosa;
        ItemBase item;
        InventoryLocation dst;
        string clase;
        string destino = "";
        string linea;
        int slot;
        bool volvio;
        if (!m_Bot || !m_Bot.IsAlive()) return "";
        invManos = m_Bot.GetHumanInventory();
        invBot = m_Bot.GetInventory();
        cosa = invManos.GetEntityInHands();
        item = ItemBase.Cast(cosa);
        if (!item) return "";
        clase = item.GetType();
        volvio = m_Guardados.Find(item) >= 0;
        dst = new InventoryLocation();
        slot = InventorySlots.GetSlotIdFromString("Melee");
        if (invBot.CanAddAttachmentEx(item, slot)) {
            dst.SetAttachment(m_Bot, item, slot);
            destino = "a la espalda (Melee)";
        }
        if (destino == "") {
            slot = InventorySlots.GetSlotIdFromString("Shoulder");
            if (invBot.CanAddAttachmentEx(item, slot)) {
                dst.SetAttachment(m_Bot, item, slot);
                destino = "al hombro (Shoulder)";
            }
        }
        if (destino == "" && invBot.FindFreeLocationFor(item, FindInventoryLocationType.CARGO, dst)) destino = "al inventario";
        if (destino != "" && m_Bot.eAI_TakeItemToLocation(item, dst)) {
            if (!volvio) m_Guardados.Insert(item);
        } else {
            destino = "BORRADO (no entra en el inventario)";
            GetGame().ObjectDelete(item);
        }
        linea = "manos vacias · " + clase + " " + destino;   // «al hombro», «a la espalda», «al inventario»
        if (!volvio) return linea;
        if (m_Revueltos.Find(item) >= 0) return "";
        m_Revueltos.Insert(item);
        return "eAI le VOLVIO a sacar " + clase + " a la mano (el corte no alcanza) · " + linea;
    }

    bool Terminado() {
        return m_Fase == BZTRFase.SENTADO || m_Fase == BZTRFase.PERDIDO;
    }

    bool Abajo() {
        return m_Fase != BZTRFase.SENTADO && m_Fase != BZTRFase.PERDIDO;
    }

    // ¿Esta sentado en su plaza (y no subiendo)?
    bool EnSuPlaza(Transport tr) {
        HumanCommandVehicle vc;
        if (!m_Bot || !tr) return false;
        if (tr.CrewMember(m_Seat) != m_Bot) return false;
        vc = m_Bot.GetCommand_Vehicle();
        if (!vc) return false;
        return !vc.IsGettingIn();
    }

    // FANTASMA: anotado en una plaza y con el vehiculo de padre, pero sin el comando de
    // vehiculo. No baja en la marca y viaja metido en la carroceria.
    bool EsFantasma(Transport tr) {
        if (!m_Bot || !tr || !m_Bot.IsAlive()) return false;
        if (m_Bot.GetParent() != tr) return false;
        return m_Bot.GetCommand_Vehicle() == null;
    }

    // ★ el vehiculo no sale hasta que todos estan sentados de verdad. SENTADO DE VERDAD = adentro de ESTE
    // vehiculo, con el comando de vehiculo y sin estar subiendo (la condicion que necesita Bajar). CrewMember == el no
    // alcanza: un fantasma lo cumple, y por eso el muster de AutoDrive y el "embarcados" de la parada lo contaban.
    bool SentadoDeVerdad(Transport tr) {
        HumanCommandVehicle vc;
        if (!m_Bot || !tr || !m_Bot.IsAlive()) return false;
        if (m_Bot.GetParent() != tr) return false;
        vc = m_Bot.GetCommand_Vehicle();
        if (!vc) return false;
        return !vc.IsGettingIn();
    }

    // Por que no esta sentado de verdad (para el log de la regla).
    string PorQueNoSentado(Transport tr) {
        HumanCommandVehicle vc;
        string s;
        if (!m_Bot) return "sin comerciante";
        if (!m_Bot.IsAlive()) return "muerto";
        s = "cmd " + m_Bot.GetCurrentCommandID().ToString() + ", FSM " + EstadoFsm();
        if (EsFantasma(tr)) return "FANTASMA, " + s;
        vc = m_Bot.GetCommand_Vehicle();
        if (vc && vc.IsGettingIn()) return "subiendo";
        if (!m_Bot.IsInTransport()) return "a pie, " + s;
        return "en otro vehiculo, " + s;
    }

    // ★ EL FANTASMA SE VUELVE A SENTAR SIN MOVERLO. Esta anotado en su plaza y es hijo del vehiculo, sin el
    // comando de vehiculo: el mismo estado que un inconsciente sentado. Vanilla lo vuelve a sentar al despertar con
    // StartCommand_Vehicle(tr, plaza, tipo de asiento, TRUE) (DayZPlayerImplement.CommandHandler, 1.30: :2391-2397), sin
    // animacion de subida. Lo mismo aca, mas el SetVehicleType de la receta de siempre.
    // ⛔ No con SentarEnPlaza(teleport): su SetPosition(tr.GetPosition()) es para un bot a pie, y este es hijo del vehiculo.
    // Devuelve que se hizo (para el log).
    private string ResentarFantasma(Transport tr) {
        HumanCommandVehicle vc;
        int plaza;
        int tipo;
        plaza = tr.CrewMemberIndex(m_Bot);
        if (plaza < 0) return "FANTASMA sin plaza anotada (hijo del vehiculo y fuera de la tripulacion): no se lo puede volver a sentar";
        m_Bot.Notify_Transport(null, -1);
        tipo = tr.GetSeatAnimationType(plaza);
        vc = m_Bot.StartCommand_Vehicle(tr, plaza, tipo, true);
        if (!vc) return "FANTASMA en la plaza " + plaza.ToString() + ": StartCommand_Vehicle(..., true) NO devolvio el comando";
        vc.SetVehicleType(tr.GetAnimInstance());
        return "FANTASMA en la plaza " + plaza.ToString() + ": se lo vuelve a sentar sin moverlo (StartCommand_Vehicle con tipo " + tipo.ToString() + " y true, el de vanilla al despertar sentado)";
    }

    // ★ vuelve a sentar al que no esta sentado de verdad. El fantasma, sin moverlo; el que esta a pie, por
    // teleport a SU plaza (en una parada, por su fase TELEPORT, que se confirma sola; al lanzar, directo, como la
    // busqueda por clase). "" si no hay nada que hacer todavia (subiendo, muerto).
    string Reembarcar(float now, Transport tr, EntityAI bus, BZBusService r, bool enParada) {
        HumanCommandVehicle vc;
        if (!m_Bot || !m_Bot.IsAlive() || !tr) return "";
        if (EsFantasma(tr)) {
            if (m_Fase == BZTRFase.PERDIDO || m_Fase == BZTRFase.TELEPORT) m_Fase = BZTRFase.SENTADO;
            return ResentarFantasma(tr);
        }
        vc = m_Bot.GetCommand_Vehicle();
        if (vc && vc.IsGettingIn()) return "";
        if (m_Bot.IsInTransport()) return "";
        if (enParada) return Teleport(now, tr, bus, r, "no estaba sentado de verdad (a pie)");
        if (!SentarCon(r, tr, true, "BZ_Trader: " + m_Cfg.Name + " no estaba sentado (a pie)")) return PlazaOcupada(now, tr);
        return "a pie: se lo sienta en SU plaza por teleport";
    }

    // ★ AL LANZAR, el comerciante encontrado por clase sale del grupo de Boris ANTES de
    // sentarse y queda en su grupo propio, quieto (formacion NONE, sin waypoints). Sentado asi, la FSM de eAI no tiene a
    // quien seguir (FollowFormation pide IN y otro lider), no recorre waypoints y no entra al estado Vehicles (GuardEx
    // pide IN y un lider que no sea el): queda en Idle, y la subida nativa de AutoDrive (Via A) no lo toma mientras
    // espera su tick. Notify_Transport(null, -1): ningun pedido de subida viejo.
    // ⛔ Nunca a un bot que YA esta sentado en el estado Vehicles: lider de si mismo, ahi se baja solo
    // (Vehicles.xml, Sitting -> OpenVehicleDoor). Por eso esto es solo para el que todavia esta a pie.
    void Apartar() {
        eAIGroup g;
        if (!m_Bot || m_Chofer) return;
        if (m_Bot.IsInTransport()) return;
        m_Bot.Notify_Transport(null, -1);
        if (!m_Grupo) m_Grupo = eAIGroup.CreateGroup(eAIFaction.Create(BZTRDef.FACCION));
        if (m_Bot.GetGroup() != m_Grupo) m_Bot.SetGroup(m_Grupo);
        g = m_Bot.GetGroup();
        if (g) {
            g.ClearWaypoints();
            g.SetFormationState(eAIGroupFormationState.NONE);
        }
        m_Pacificado = Pacificar();   // fuera del grupo Passive de Boris, pasivo por su cuenta
    }

    // ★ CON EL SERVER VACIO eAI DUERME AL BOT QUIETO. eAIState_Dormant entra sin jugadores a la
    // vista, con el bot caminando (cmd de movimiento), sin moverse y con su waypoint alcanzado, y le apaga la simulacion
    // (DisableSimulation). Dormido no camina, no vuelve y no procesa el pedido de subida. Medido con el server vacio:
    // un comerciante se durmio a 3,5 m de su puesto sin dar un paso; otro, dormido en el puesto, no volvio a la puerta en 20 s;
    // una subida animada no arranco en 10 s. Las dos terminaron en teleport.
    // La guarda de Dormant FALLA con mas de un waypoint en el grupo ("Moving to waypoint"), y la FSM se sigue
    // actualizando dormida (ExpansionWorld): con dos, no se duerme y el que ya dormia se despierta en su tick siguiente.
    // Mientras esta a pie, su grupo tiene siempre dos: el ultimo, repetido (con ONCE termina en el mismo punto).
    private void Despierto(eAIGroup g) {
        array<vector> wps;
        vector ultimo;
        if (!g) return;
        wps = g.GetWaypoints();
        if (!wps || wps.Count() != 1) return;
        ultimo = wps.Get(0);
        g.AddWaypoint(ultimo);
    }

    // ★ EL COMERCIANTE VA PASIVO. eAI_IsPassive() tambien es true si la FACCION de su grupo es pasiva, y el
    // grupo de Boris es "Passive" (AutoDrive): ahi nunca juntaba blancos. Desde Apartar esta en un grupo propio
    // con la faccion del archivo (Observers), que no es pasiva, y eAI le da por blanco cualquier objeto del piso que le
    // sirva (LootingBehaviour: CLOTHING | UPGRADE | FOOD | BANDAGES). Un objeto vale amenaza hasta 1 (0,4 a 60 m,
    // eAIItemTargetInformation.CalculateThreat), y con >= 0,4 entra a Fighting y Positioning camina HACIA el objeto
    // (medido: 27 s en Fighting/Positioning, a 36 m de su puesto, sin enemigos en el server).
    // Pasivo, eAI_UpdateTargets sale en la primera linea (eAIBase.c:2966): no junta blancos, ni objetos ni enemigos.
    // TraversingWaypoints no mira la pasividad (Boris, pasivo, camina a su puerta). Los blancos que ya tenia se borran.
    // Devuelve que se borro ("" si nada).
    string Pacificar() {
        int n;
        int i;
        int borrados;
        int guarda = 0;
        eAITarget t;
        string primero;
        if (!m_Bot) return "";
        m_Bot.eAI_SetPassive(true);
        n = m_Bot.TargetCount();
        if (n == 0) return "";
        primero = BlancoTexto();
        // ⛔ los RUIDOS no se borran. eAI_RemoveTarget, al borrar un ruido, hace encoger los hombros al bot una
        // de cada tres veces (el PlayEmote al bajar), y un ruido de un pasivo no pasa de ~0,15 de amenaza
        // (eAIBase, la conversion de ruido para pasivos y observers): nunca llega al 0,4 de Fighting.
        borrados = 0;
        i = m_Bot.TargetCount() - 1;
        while (i >= 0 && guarda < 50) {
            guarda = guarda + 1;
            t = m_Bot.GetTarget(i);
            if (t && !t.IsNoise()) {
                m_Bot.eAI_RemoveTarget(t);
                borrados = borrados + 1;
            }
            i = i - 1;
        }
        if (borrados == 0) return "";
        return "pasivo: se le borraron " + borrados.ToString() + " de " + n.ToString() + " blanco(s) de eAI, los ruidos quedan (" + primero + ")";
    }

    // Sentarlo con la receta de AutoDrive y dejarlo pasivo: SentarEnPlaza pone eAI_SetPassive(plaza == 0), o sea que a
    // un tripulante lo deja NO pasivo.
    // ⛔ Nunca sobre una plaza OCUPADA por otro (un jugador que igual llego a sentarse: un server sin el modulo Vehicles y la guarda esquivada): la receta de vanilla
    // (StartCommand_Vehicle) le pisa la plaza en la tabla del vehiculo, y el jugador queda sentado sin figurar en
    // ninguna: el retiro de AutoDrive no lo cuenta y borra el vehiculo con el adentro (en la 1.30 se cae el server). El
    // comerciante espera al lado y al que la ocupa se le avisa (PlazaOcupada). true = se lo sento (o se pidio la subida).
    bool SentarCon(BZBusService r, Transport tr, bool teleport, string motivo) {
        if (!m_Bot || !r || !tr) return false;
        if (OcupanteDePlaza(tr)) return false;
        r.SentarEnPlaza(m_Bot, tr, m_Seat, teleport, motivo);
        m_Bot.eAI_SetPassive(true);
        return true;
    }

    // quien ocupa SU plaza y no es el (un jugador, otro bot, un cuerpo), o null
    Human OcupanteDePlaza(Transport tr) {
        Human h;
        if (!tr || m_Seat < 0 || m_Seat >= tr.CrewSize()) return null;
        h = tr.CrewMember(m_Seat);
        if (!h || h == m_Bot) return null;
        return h;
    }

    // La plaza esta ocupada: se espera al lado, sin mensaje al jugador. Devuelve la linea del log (la primera vez y cada
    // PLAZA_LOG_S; "" si no toca).
    string PlazaOcupada(float now, Transport tr) {
        Human h = OcupanteDePlaza(tr);
        PlayerBase pb = PlayerBase.Cast(h);
        string quien = "otro";
        if (!h) return "";
        if (pb && pb.GetIdentity() && !eAIBase.Cast(pb)) quien = "el jugador " + pb.GetIdentity().GetName();
        if (m_PlazaLogT > 0 && now - m_PlazaLogT < PLAZA_LOG_S) return "";
        m_PlazaLogT = now;
        return "su plaza " + m_Seat.ToString() + " esta OCUPADA por " + quien + ": no se lo sienta encima (espera al lado)";
    }

    // el primer blanco de eAI y la amenaza que siente (contra que "pelea"). Para el diag por segundo.
    string BlancoTexto() {
        eAITarget t;
        EntityAI e;
        string que = "entidad";
        string clase = "?";
        float propia;
        float del;
        int n;
        if (!m_Bot) return "sin bot";
        propia = Math.Round(m_Bot.GetThreatToSelf() * 100) / 100;
        n = m_Bot.TargetCount();
        if (n == 0) return "blancos 0 · amenaza " + propia.ToString() + " · pasivo " + m_Bot.eAI_IsPassive().ToString();
        t = m_Bot.GetTarget(0);
        if (!t) return "blancos " + n.ToString() + " (el primero nulo) · amenaza " + propia.ToString();
        e = t.GetEntity();
        if (e) clase = e.GetType();
        if (t.IsItem()) que = "objeto";
        else if (t.IsPlayer()) que = "jugador";
        else if (t.IsZombie()) que = "infectado";
        else if (t.IsCreature()) que = "animal";
        else if (t.IsNoise()) que = "ruido";
        else if (t.IsVehicle()) que = "vehiculo";
        else if (t.IsGroup()) que = "grupo";
        else if (t.IsDoor()) que = "puerta";
        else if (t.IsHazard()) que = "peligro";
        del = Math.Round(t.GetThreat() * 100) / 100;
        return "blancos " + n.ToString() + " · el primero: " + que + " " + clase + " a " + BZTRLog.Metros(t.GetDistance()) + ", amenaza " + del.ToString() + " · amenaza propia " + propia.ToString() + " · pasivo " + m_Bot.eAI_IsPassive().ToString();
    }

    // ¿esta usando su puerta en este momento? Subiendo, o recien sentado (TURNO_S): el siguiente espera.
    bool UsaSuPuerta(float now) {
        if (m_Fase == BZTRFase.SUBIENDO) return true;
        if (m_Fase == BZTRFase.SENTADO && m_SentadoT > 0 && now - m_SentadoT < TURNO_S) return true;
        return false;
    }

    // Sentado: su puerta, cerrada y sostenida unos segundos (si no, un bot sube y la puerta queda abierta).
    void SostenerPuerta(float now, EntityAI bus) {
        if (m_Fase != BZTRFase.SENTADO) return;
        if (now < m_CerrarHasta) CerrarPuerta(bus);
    }

    // ------------------------------------------------------------------------
    //  LA ROPA: Loadout (ExpansionHumanLoadout) o Clothing[] a mano (como DressDriver de AutoDrive).
    //  AutoDrive ya vistio a la Crew con su loadout: se saca lo que tiene y se viste en el tick siguiente,
    //  porque el Delete de DayZ es diferido y la ranura sigue ocupada en este cuadro.
    // ------------------------------------------------------------------------
    void Desvestir(float now) {
        int i;
        int n;
        EntityAI a;
        EntityAI mano;
        if (!m_Bot) return;
        n = m_Bot.GetInventory().AttachmentCount();
        for (i = n - 1; i >= 0; i--) {
            a = m_Bot.GetInventory().GetAttachmentFromIndex(i);
            if (a) a.Delete();
        }
        mano = m_Bot.GetHumanInventory().GetEntityInHands();
        if (mano) mano.Delete();
        m_VestirPendiente = true;
        m_VestirT = now + 1;
    }

    // "" si no toca; si toca, que se hizo (para el log).
    string VestirSiToca(float now) {
        int i;
        string s;
        bool ok;
        if (!m_VestirPendiente || now < m_VestirT || !m_Bot) return "";
        m_VestirPendiente = false;
        if (m_Cfg.Loadout != "") {
            ok = ExpansionHumanLoadout.Apply(m_Bot, m_Cfg.Loadout, false);
            return "vestido con el loadout " + m_Cfg.Loadout + " (ok " + ok.ToString() + ")";
        }
        s = "";
        for (i = 0; i < m_Cfg.m_Ropa.Count(); i++) {
            m_Bot.GetInventory().CreateAttachment(m_Cfg.m_Ropa.Get(i));
            s = s + " " + m_Cfg.m_Ropa.Get(i);
        }
        return "vestido a medida:" + s;
    }

    // ------------------------------------------------------------------------
    //  LA PARADA
    // ------------------------------------------------------------------------
    // Baja al bot. false si no esta sentado en condiciones de bajar (se queda arriba).
    bool Bajar(float now, EntityAI bus, BZTRStopTrader st, float tiempoMin, float tope, float radioJug, float boardTimeoutS, float altoM) {
        HumanCommandVehicle vc;
        int seat;
        bool hayPuerta = false;
        string puerta = "";
        ExpansionVehicle exv;
        eAIGroup g;
        m_NoBajo = "";
        if (!m_Bot || !m_Bot.IsAlive()) {
            m_NoBajo = EstadoPlaza(Transport.Cast(bus));
            return false;
        }
        vc = m_Bot.GetCommand_Vehicle();
        if (!vc || vc.IsGettingIn()) {
            m_NoBajo = EstadoPlaza(Transport.Cast(bus));
            return false;
        }
        seat = vc.GetVehicleSeat();
        m_Bot.Notify_Transport(null, -1);          // ningun pedido de subida pendiente puede re-sentarlo
        // ⛔ NO eAI_SetPassive(false). Aca decia "a pie camina": no hace falta (TraversingWaypoints no mira la
        // pasividad) y no pasivo, en su grupo propio, eAI lo manda a buscar objetos del piso (Fighting/Positioning).
        ExpansionFSMHelper.DoorAnimationSource(bus, seat, hayPuerta, puerta);
        m_TienePuerta = (hayPuerta && puerta != "");
        m_PuertaSrc = puerta;
        m_CerrarHasta = 0;
        if (m_TienePuerta) bus.SetAnimationPhase(puerta, 1.0);
        vc.GetOutVehicle();
        if (ExpansionVehicle.Get(exv, bus)) exv.ReserveSeat(seat, null);
        m_Bot.LookAtDirection("0 0 1");
        if (!m_Chofer) {
            if (!m_Grupo) m_Grupo = eAIGroup.CreateGroup(eAIFaction.Create(BZTRDef.FACCION));
            // ⚠ solo si todavia no esta en el: SetGroup saca al bot del grupo anterior y lo BORRA si queda vacio,
            // y en la segunda parada el anterior ES el suyo.
            if (m_Bot.GetGroup() != m_Grupo) m_Bot.SetGroup(m_Grupo);
        }
        g = m_Bot.GetGroup();
        if (g) {
            g.ClearWaypoints();
            // el segundo entre bajarse y empezar a caminar, sin waypoints, eAI lo dormia (medido: Dormant en
            // t 0). Dos en su lugar lo mantienen despierto; con formacion NONE no lo mueven.
            // el que maneja tambien (en el grupo del runner: durante el HOLD AutoDrive no le escribe
            // waypoints, y sin ninguno eAI lo dormiria)
            g.AddWaypoint(m_Bot.GetPosition());
            Despierto(g);
            if (!m_Chofer) g.SetFormationState(eAIGroupFormationState.NONE);
        }
        m_Pacificado = Pacificar();   // a pie, en su grupo propio, pasivo (sin blancos de eAI)
        // abajo sigue invencible (antes, con Invincible 0, al bajar se le habilitaba el dano)
        m_Parada = st;
        m_TiempoMin = tiempoMin;
        m_Tope = tope;
        if (m_Tope < m_TiempoMin) m_Tope = m_TiempoMin;
        m_RadioJug = radioJug;
        m_AltoM = altoM;
        m_BoardTimeoutS = boardTimeoutS;
        m_Velocidad = eAIMovementSpeed.WALK;
        if (st) m_Velocidad = st.Velocidad();
        m_Fase = BZTRFase.BAJANDO;
        m_FaseT0 = now;
        m_BajoT = now;
        m_ComoSubio = "";
        return true;
    }

    // Una fase por tick. Devuelve una linea para el log cuando cambia algo ("" si nada).
    string Tick(float now, Transport tr, EntityAI bus, BZBusService r) {
        if (m_Fase == BZTRFase.SENTADO || m_Fase == BZTRFase.PERDIDO) return "";
        if (!m_Bot || !m_Bot.IsAlive()) {
            m_Fase = BZTRFase.PERDIDO;
            return "ya no existe o murio: la parada sigue sin el";
        }
        // el freno es solo de VENDIENDO: con el freno puesto no camina a ningun lado (la vuelta, la parada siguiente)
        if (m_Frenado && m_Fase != BZTRFase.VENDIENDO) Frenar(false);
        // los primeros segundos a pie, la puerta se sostiene cerrada (la FSM de eAI la puede reabrir al terminar su
        // propia bajada); a la vuelta se abre a proposito, asi que ahi no.
        if (now < m_CerrarHasta && m_Fase != BZTRFase.VOLVIENDO && m_Fase != BZTRFase.SUBIENDO) CerrarPuerta(bus);
        if (m_Fase == BZTRFase.BAJANDO) return TickBajando(now, bus);
        if (m_Fase == BZTRFase.CAMINANDO) return TickCaminando(now);
        if (m_Fase == BZTRFase.VENDIENDO) return TickVendiendo(now, tr);
        if (m_Fase == BZTRFase.VOLVIENDO) return TickVolviendo(now, tr, bus, r);
        if (m_Fase == BZTRFase.SUBIENDO) return TickSubiendo(now, tr, bus, r);
        if (m_Fase == BZTRFase.TELEPORT) return TickTeleport(now, bus);
        return "";
    }

    private void CerrarPuerta(EntityAI bus) {
        if (!bus || !m_TienePuerta) return;
        if (bus.GetAnimationPhase(m_PuertaSrc) > 0.5) bus.SetAnimationPhase(m_PuertaSrc, 0.0);
    }

    private string TickBajando(float now, EntityAI bus) {
        if (!m_Bot.IsInTransport()) {
            CerrarPuerta(bus);
            m_CerrarHasta = now + 5;
            return EmpezarACaminar(now, bus);
        }
        if (now - m_FaseT0 <= BAJAR_MAX_S) return "";
        // no se pudo bajar: sigue sentado (su plaza esta ocupada, la parada no lo espera)
        m_Fase = BZTRFase.SENTADO;
        m_ComoSubio = "no se bajo";
        return "no termino de bajar en " + BZTRLog.Seg(BAJAR_MAX_S) + ": se queda arriba";
    }

    private string EmpezarACaminar(float now, EntityAI bus) {
        eAIGroup g = m_Bot.GetGroup();
        Transport tr = Transport.Cast(bus);
        int i;
        int n = 0;
        BZTRWaypoint w;
        string salida = "";
        string medida = MedirSalida(tr);
        m_Bot.SetMovementSpeedLimits(m_Velocidad, m_Velocidad);
        if (g) {
            if (!m_Chofer) g.SetFormationState(eAIGroupFormationState.IN);
            g.ClearWaypoints();
        }
        if (m_Parada && g) {
            for (i = 0; i < m_Parada.Waypoints.Count(); i++) {
                w = m_Parada.Waypoints.Get(i);
                if (!w) continue;
                if (n == 0) salida = Salida(g, tr, w.Position);   // antes del primer puesto
                g.AddWaypoint(w.Position);
                m_Puesto = w.Position;
                n = n + 1;
            }
        }
        if (n == 0) {
            if (g) {
                g.AddWaypoint(m_Bot.GetPosition());
                Despierto(g);
                g.SetWaypointBehaviour(eAIWaypointBehavior.HALT);
            }
            RecalcularCamino();
            m_Objetivo = m_Bot.GetPosition();
            return EmpezarAVender(now, "a pie en " + BZTRLog.Seg(now - m_BajoT) + ": vende donde bajo (sin puesto)" + medida);
        }
        Despierto(g);
        g.SetWaypointBehaviour(eAIWaypointBehavior.ONCE);
        RecalcularCamino();
        m_Objetivo = m_Puesto;
        m_Fase = BZTRFase.CAMINANDO;
        m_FaseT0 = now;
        m_UltPos = m_Bot.GetPosition();
        m_MovioT = now;
        return "a pie en " + BZTRLog.Seg(now - m_BajoT) + ": camina a su puesto " + BZTRLog.Pos3(m_Puesto) + " (" + BZTRLog.Metros(vector.Distance(m_Bot.GetPosition(), m_Puesto)) + ", " + n.ToString() + " waypoint(s))" + salida + medida;
    }

    // ★ AL BAJAR camina desde donde quedo parado (la salida medida). Si esta ADENTRO de la caja (el pasillo
    // del bus), derecho al puesto (asi anda en el bus). Si esta AFUERA y el tramo hasta el puesto cruza el casco
    // (la compuerta de atras del APC con el puesto al costado), por la esquina mas corta. Texto para el log.
    private string Salida(eAIGroup g, Transport tr, vector destino) {
        array<vector> esquinas = new array<vector>();
        vector c;
        string lado;
        string rodeo;
        int i;
        m_TieneAprox = false;
        if (!m_Casco || !tr || !g || !m_TieneSalida) return "";
        if (m_SalidaAdentro) return " · bajo adentro del casco: derecho al puesto, sin rodeo";
        lado = m_Casco.LadoDe(m_SalidaLocal);
        rodeo = m_Casco.Camino(tr, m_Bot.GetPosition(), destino, esquinas);
        for (i = 0; i < esquinas.Count(); i++) {
            c = esquinas.Get(i);
            g.AddWaypoint(c);
        }
        if (rodeo == "") return " · bajo afuera (" + lado + "), derecho al puesto";
        if (rodeo == "?") return " · bajo afuera (" + lado + "); el puesto esta pegado al casco: va derecho";
        return " · bajo afuera (" + lado + ") y camina a su puesto rodeando el vehiculo por " + rodeo + " (" + esquinas.Count().ToString() + " esquina(s))";
    }

    // donde quedo parado a pie, relativo al vehiculo; la primera vez de cada plaza en el arranque se anota en
    // _plazas (para el editor). Devuelve el texto para el log.
    private string MedirSalida(Transport tr) {
        string linea;
        string donde = "afuera";
        float x;
        float z;
        m_TieneSalida = false;
        if (!m_Casco || !tr || !m_Bot) return "";
        m_SalidaLocal = BZTRCasco.ALocal(tr, m_Bot.GetPosition());
        m_SalidaAdentro = m_Casco.Dentro(m_SalidaLocal, -BZTRCasco.ADENTRO_M);   // 0,3 m o mas adentro de la caja
        m_TieneSalida = true;
        linea = BZTRPlazas.AnotarSalida(tr, m_Seat, m_SalidaLocal, m_SalidaAdentro);
        if (linea != "") return " · " + linea;
        if (m_SalidaAdentro) donde = "adentro";
        x = Math.Round(m_SalidaLocal[0] * 10) / 10;
        z = Math.Round(m_SalidaLocal[2] * 10) / 10;
        return " · salida a la derecha " + x.ToString() + " m, adelante " + z.ToString() + " m (" + donde + " de la caja)";
    }

    private string TickCaminando(float now) {
        vector p = m_Bot.GetPosition();
        float d = Math.Sqrt(BZTRJugadores.DistXZ2(p, m_Puesto));
        if (d <= LLEGADA_CERCA_M) return EmpezarAVender(now, "llego a su puesto en " + BZTRLog.Seg(now - m_FaseT0));
        if (d <= LLEGADA_M && GetVelocity(m_Bot).Length() < LLEGADA_QUIETO_MS) return EmpezarAVender(now, "llego a " + BZTRLog.Metros(d) + " de su puesto y ahi se quedo quieto (una pared, un kiosco) en " + BZTRLog.Seg(now - m_FaseT0));
        if (vector.Distance(p, m_UltPos) > MOVIO_M) {
            m_UltPos = p;
            m_MovioT = now;
        }
        if (now - m_MovioT > SIN_MOVERSE_S) return EmpezarAVender(now, "no se movio en " + BZTRLog.Seg(SIN_MOVERSE_S) + ": vende donde esta, a " + BZTRLog.Metros(d) + " del puesto");
        if (now - m_FaseT0 > CAMINO_MAX_S) return EmpezarAVender(now, "TOPE de " + BZTRLog.Seg(CAMINO_MAX_S) + " caminando: vende donde esta, a " + BZTRLog.Metros(d) + " del puesto");
        return "";
    }

    // ★ VENDIENDO, QUIETO DONDE EMPEZO: GIRA SIN TRASLADARSE. Lo que lo hacia caminar derecho por su rumbo (eAI):
    //   - con el giro pedido (OverrideTurnTarget), eAIBase llama en cada update a eAICommandMove.SetTurnTarget(rumbo,
    //     true); forzado, el comando no recalcula hacia donde mirar mientras camina (ShouldRecalculateTurnTarget) y la
    //     marcha es hacia adelante;
    //   - la velocidad la decide el comando por la distancia al waypoint: si el bot no quedo "en posicion final" (llego
    //     caminando y se paso, un empujon, el estado Interacting) camina, y con el cuerpo clavado en el rumbo se va por el;
    //   - OverrideTurnTarget(false) NO limpia el forzado del comando (solo deja de pedirlo) y mientras camina nadie lo
    //     limpia: soltar el giro no lo frenaba (SoltarGiro).
    // La receta: el freno de eAI (Frenar: eAI_SetHalt, el limite de velocidad en 0) en cada tick de VENDIENDO; con el
    // freno no se traslada y el giro forzado lo da vuelta en el lugar. Su waypoint es el punto donde esta, en HALT (dos
    // iguales: despierto), y Deriva vigila que no se aparte.
    private void FijarPuntoVenta() {
        eAIGroup g = m_Bot.GetGroup();
        m_PuntoVenta = m_Bot.GetPosition();
        m_Derivado = false;
        Frenar(true);
        if (!g) return;
        g.ClearWaypoints();
        g.AddWaypoint(m_PuntoVenta);
        Despierto(g);
        g.SetWaypointBehaviour(eAIWaypointBehavior.HALT);
    }

    // ★ LA GUARDA, en cada tick (cuantas veces haga falta). Frenado, solo se aparta si lo empujan: a mas de DERIVA_M se le
    // sueltan el freno y el giro (de verdad: SoltarGiro) y eAI lo lleva caminando de vuelta a su waypoint; a menos de
    // DERIVA_VUELTO_M se frena otra vez y mira su rumbo. Si no volvio en DERIVA_VOLVER_S, vende donde esta. Devuelve la
    // linea del log ("" si nada).
    private string Deriva(float now) {
        float d = Math.Sqrt(BZTRJugadores.DistXZ2(m_Bot.GetPosition(), m_PuntoVenta));
        if (!m_Derivado) {
            Frenar(true);   // en cada tick: eAI lo suelta solo al manejar una puerta
            if (d <= DERIVA_M) return "";
            m_Derivado = true;
            m_DerivaT0 = now;
            m_Derivas = m_Derivas + 1;
            SoltarRumbo();
            Frenar(false);
            return "VENDIENDO se aparto " + BZTRLog.Metros(d) + " de donde vende: se le sueltan el freno y el giro y vuelve caminando (" + m_Derivas.ToString() + " vez/veces en esta parada · " + GrupoTexto() + ")";
        }
        if (d <= DERIVA_VUELTO_M) {
            m_Derivado = false;
            Frenar(true);
            return "volvio a donde vende en " + BZTRLog.Seg(now - m_DerivaT0) + ": frenado otra vez" + Orientar(now);
        }
        if (now - m_DerivaT0 < DERIVA_VOLVER_S) return "";
        FijarPuntoVenta();
        return "no volvio a donde vende en " + BZTRLog.Seg(DERIVA_VOLVER_S) + " (esta a " + BZTRLog.Metros(d) + "): vende donde esta, frenado" + Orientar(now);
    }

    private string EmpezarAVender(float now, string motivo) {
        string alto = "sin alto frente a jugadores (PlayerHaltM 0)";
        m_Fase = BZTRFase.VENDIENDO;
        m_FaseT0 = now;
        m_VendeT0 = now;
        m_Derivas = 0;
        FijarPuntoVenta();
        if (m_AltoM > 0) alto = "se frena y mira a un jugador a <= " + BZTRLog.Metros(m_AltoM);
        return motivo + " · vende minimo " + BZTRLog.Seg(m_TiempoMin) + ", tope " + BZTRLog.Seg(m_Tope) + " con jugadores a <= " + BZTRLog.Metros(m_RadioJug) + " · " + alto + Orientar(now);
    }

    // ★ se da vuelta hacia el rumbo de su puesto (HeadingDeg o LookAt) y lo SOSTIENE mientras vende.
    // eAI: OverrideTurnTarget gana sobre la mirada, el apuntar y la pose de quieto (eAIBase.c, la orden de giro del
    // comando de caminar, forzada). El alto frente a un jugador lo suelta; al irse el jugador se repone.
    private string Orientar(float now) {
        bool ok;
        string como;
        vector d;
        vector ang;
        m_Rumbo = false;
        m_RumboMedidas = 0;
        if (!m_Parada || !m_Bot) return "";
        d = m_Parada.Mirada(m_Bot.GetPosition(), ok, como);
        if (!ok) return "";
        ang = d.VectorToAngles();
        m_RumboYaw = ang[0];
        m_RumboDeg = RumboDe(d);
        m_RumboComo = como;
        m_RumboT = now;
        m_Rumbo = true;
        m_Bot.OverrideTurnTarget(true, m_RumboYaw);
        return " · se da vuelta a rumbo " + Math.Round(m_RumboDeg).ToString() + " (" + como + ")";
    }

    private void SoltarRumbo() {
        if (m_Rumbo && m_Bot) SoltarGiro();
        m_Rumbo = false;
    }

    // ★ Suelta el giro forzado DE VERDAD. OverrideTurnTarget(false) solo deja de pedirlo: el comando de caminar de eAI se
    // queda con su ultimo SetTurnTarget(rumbo, true) y, mientras camina, nadie lo limpia (no recalcula hacia donde mirar):
    // seguiria con el cuerpo en el rumbo viejo, caminando hacia adelante. Se le pone su orientacion actual, sin forzar.
    private void SoltarGiro() {
        eAICommandMove cmdMov;
        if (!m_Bot) return;
        m_Bot.OverrideTurnTarget(false, 0);
        cmdMov = m_Bot.m_eAI_CommandMove;
        if (cmdMov) cmdMov.SetTurnTarget(m_Bot.GetOrientation()[0], false);
    }

    // ★ El freno de eAI (eAI_SetHalt: el limite de velocidad del comando de caminar en 0): quieto aunque eAI quiera ir a
    // otro lado, y el giro lo da vuelta en el lugar. Solo en VENDIENDO; el Tick lo suelta en cualquier otra fase.
    private void Frenar(bool si) {
        m_Frenado = si;
        if (m_Bot) m_Bot.eAI_SetHalt(si);
    }

    // El rumbo de DayZ de una direccion (0 = norte, horario, 0-360).
    static float RumboDe(vector d) {
        float r = Math.Atan2(d[0], d[2]) * Math.RAD2DEG;
        if (r < 0) r = r + 360;
        return r;
    }

    // Lo que se aparta su rumbo real del pedido, en grados (-180 a 180).
    private float DifRumbo() {
        vector dr = m_Bot.GetDirection();
        float dif;
        dr[1] = 0;
        dif = RumboDe(dr) - m_RumboDeg;
        dif = dif - 360 * Math.Round(dif / 360);
        return dif;
    }

    // La medicion del giro (a +3 s y a +15 s de darse vuelta, sin alto): el rumbo real contra el pedido.
    private string MedirRumbo(float now) {
        string cuando;
        float dif;
        vector dr;
        if (!m_Rumbo || m_Alto || m_RumboMedidas >= 2) return "";
        if (m_RumboT < 0) m_RumboT = now;
        if (m_RumboMedidas == 0 && now - m_RumboT < 3) return "";
        if (m_RumboMedidas == 1 && now - m_RumboT < 15) return "";
        cuando = "a los 3 s";
        if (m_RumboMedidas == 1) cuando = "a los 15 s (sostenido)";
        m_RumboMedidas = m_RumboMedidas + 1;
        dif = DifRumbo();
        dr = m_Bot.GetDirection();
        dr[1] = 0;
        return "rumbo " + cuando + ": real " + Math.Round(RumboDe(dr)).ToString() + ", pedido " + Math.Round(m_RumboDeg).ToString() + " (" + m_RumboComo + ") · diferencia " + Math.Round(dif).ToString();
    }

    // El alto de TTE, mientras VENDE (es la unica fase que lo llama): con jugadores a <= m_AltoM, quieto y mirandolos;
    // sin nadie, suelta el override. PlayerHaltM 0 = sin alto (la secuencia fija). Al caminar no
    // hay alto: ni nuestro ni de TTE (el de TTE es de sus caminantes, TTE_SpawnManager).
    private void Alto() {
        array<Man> cerca = new array<Man>();
        int i;
        Man m;
        if (m_AltoM <= 0) {
            if (m_Alto || m_Mirando.Count() > 0) SoltarAlto();
            return;
        }
        BZTRJugadores.Cerca(m_Bot.GetPosition(), m_AltoM, cerca);
        if (cerca.Count() > 0) {
            if (!m_Alto && m_Rumbo) SoltarGiro();   // mira al jugador
            m_Alto = true;
            m_Bot.OverrideMovementSpeed(true, eAIMovementSpeed.STATIC);
            m_Bot.OverrideTargetPosition(m_Bot.GetPosition());
            for (i = 0; i < cerca.Count(); i++) {
                m = cerca.Get(i);
                if (m_Mirando.Find(m) >= 0) continue;
                m_Bot.eAI_AddInteractingPlayer(m);
                m_Mirando.Insert(m);
            }
        }
        for (i = m_Mirando.Count() - 1; i >= 0; i--) {
            m = m_Mirando.Get(i);
            if (m && cerca.Find(m) >= 0) continue;
            if (m) m_Bot.eAI_RemoveInteractingPlayer(m);
            m_Mirando.Remove(i);
        }
        if (cerca.Count() == 0 && m_Alto) SoltarAlto();
    }

    private void SoltarAlto() {
        int i;
        Man m;
        m_Alto = false;
        if (!m_Bot) return;
        m_Bot.OverrideMovementSpeed(false, 0);
        for (i = m_Mirando.Count() - 1; i >= 0; i--) {
            m = m_Mirando.Get(i);
            if (m) m_Bot.eAI_RemoveInteractingPlayer(m);
        }
        m_Mirando.Clear();
        // se fue el jugador: vuelve a su rumbo (se mide otra vez)
        if (m_Rumbo && m_Fase == BZTRFase.VENDIENDO) {
            m_Bot.OverrideTurnTarget(true, m_RumboYaw);
            m_RumboT = -1;
            m_RumboMedidas = 0;
        }
    }

    private string TickVendiendo(float now, Transport tr) {
        float t = now - m_VendeT0;
        int n;
        string med;
        Alto();
        med = Deriva(now);
        if (med == "") med = MedirRumbo(now);
        n = BZTRJugadores.Cerca(m_Bot.GetPosition(), m_RadioJug, null);
        if (t < m_TiempoMin) return med;
        if (n == 0) return Volver(now, tr, "vendio " + BZTRLog.Seg(t) + " y no hay jugadores cerca");
        if (t >= m_Tope) return Volver(now, tr, "TOPE de " + BZTRLog.Seg(m_Tope) + " con " + n.ToString() + " jugador(es) cerca");
        return med;
    }

    private string Volver(float now, Transport tr, string motivo) {
        eAIGroup g = m_Bot.GetGroup();
        vector puerta;
        vector dir;
        vector piso;
        float yPiso;
        array<vector> esquinas = new array<vector>();
        vector c;
        string rodeo = "";
        string texto;
        string como = "";
        vector ap;
        int i;
        SoltarRumbo();
        SoltarAlto();
        Frenar(false);
        tr.CrewEntryWS(m_Seat, puerta, dir);
        yPiso = GetGame().SurfaceY(puerta[0], puerta[2]);
        piso = puerta;
        piso[1] = yPiso;
        m_TieneAprox = false;
        m_TurnoAvisado = false;
        if (g) {
            if (!m_Chofer) g.SetFormationState(eAIGroupFormationState.IN);
            g.ClearWaypoints();
            if (m_Casco) {
                // al punto donde sube (BZTRCasco.PuntoDeVuelta: su salida medida si quedo afuera, el borde del
                // casco si la entrada esta adentro, o su puerta), rodeando el casco si hace falta; ahi sube animado
                ap = m_Casco.PuntoDeVuelta(m_Casco.EntradaLocal(tr, m_Seat), m_TieneSalida, m_SalidaLocal, m_SalidaAdentro, BZTRCasco.ALocal(tr, m_Bot.GetPosition()), como);
                m_Aprox = BZTRCasco.AMundo(tr, ap[0], ap[2]);
                m_TieneAprox = true;
                m_LadoPuerta = como;
                rodeo = m_Casco.Camino(tr, m_Bot.GetPosition(), m_Aprox, esquinas);
                for (i = 0; i < esquinas.Count(); i++) {
                    c = esquinas.Get(i);
                    g.AddWaypoint(c);
                }
                g.AddWaypoint(m_Aprox);
                piso = m_Aprox;
            } else {
                g.AddWaypoint(piso);
            }
            Despierto(g);
            g.SetWaypointBehaviour(eAIWaypointBehavior.ONCE);
        }
        m_Bot.SetMovementSpeedLimits(m_Velocidad, m_Velocidad);
        RecalcularCamino();
        m_Objetivo = piso;
        m_Fase = BZTRFase.VOLVIENDO;
        m_FaseT0 = now;
        if (m_TieneAprox) {
            texto = " · vuelve a subir por " + m_LadoPuerta + " (" + BZTRLog.Metros(vector.Distance(m_Bot.GetPosition(), m_Aprox)) + "";
            if (rodeo != "" && rodeo != "?") texto = texto + ", rodeando el vehiculo por " + rodeo;
            return motivo + texto + ", rescate por teleport a los " + BZTRLog.Seg(m_BoardTimeoutS) + ")";
        }
        return motivo + " · vuelve a su puerta (" + BZTRLog.Metros(vector.Distance(m_Bot.GetPosition(), piso)) + ", rescate por teleport a los " + BZTRLog.Seg(m_BoardTimeoutS) + ")";
    }

    private string TickVolviendo(float now, Transport tr, EntityAI bus, BZBusService r) {
        vector puerta;
        vector dir;
        bool hayPuerta = false;
        string anim = "";
        eAIGroup g;
        float tardo = now - m_FaseT0;
        bool llego;
        string donde = "a su puerta";
        tr.CrewEntryWS(m_Seat, puerta, dir);
        llego = BZTRJugadores.DistXZ2(m_Bot.GetPosition(), puerta) <= PUERTA_M2;
        // con el casco medido, sube desde su punto de aproximacion (en el APC, la entrada de atras esta dentro
        // del casco: hasta ahi no se llega caminando)
        if (!llego && m_TieneAprox && BZTRJugadores.DistXZ2(m_Bot.GetPosition(), m_Aprox) <= APROX_LLEGADA_M2) {
            llego = true;
            donde = "al punto de subida (" + m_LadoPuerta + ", a " + BZTRLog.Metros(Math.Sqrt(BZTRJugadores.DistXZ2(m_Bot.GetPosition(), puerta))) + " de la entrada)";
        }
        // una puerta compartida (la compuerta del APC), de a uno: espera a que el otro termine de subir
        if (llego && m_PuertaOcupada >= 0) {
            if (tardo > m_BoardTimeoutS) return Teleport(now, tr, bus, r, "no le toco el turno de su puerta en " + BZTRLog.Seg(m_BoardTimeoutS));
            if (m_TurnoAvisado) return "";
            m_TurnoAvisado = true;
            return "llego " + donde + ": espera su turno, la puerta la esta usando la plaza " + m_PuertaOcupada.ToString();
        }
        // su plaza ocupada por otro (un jugador): espera al lado, sin sentarse encima, y se le avisa al que la ocupa
        if (llego && OcupanteDePlaza(tr)) return PlazaOcupada(now, tr);
        if (llego) {
            ExpansionFSMHelper.DoorAnimationSource(bus, m_Seat, hayPuerta, anim);
            if (hayPuerta && anim != "") bus.SetAnimationPhase(anim, 1.0);
            m_Bot.LookAtDirection("0 0 1");
            m_Bot.BzSacarCorrea();   // AutoDrive (BZEaiCorrea.c): ningun "volver a la puerta" viejo le pisa el destino
            SentarCon(r, tr, false, "BZ_Trader: " + m_Cfg.Name + " llego a su puerta");
            // ⛔ los waypoints de la puerta NO se borran hasta que este sentado. Sin waypoints eAI lo duerme
            // y dormido no procesa el pedido de subida (medido: "no termino de subir en 10 s", FSM Dormant). Estan en
            // la puerta misma: no lo alejan. Se borran en Sentado.
            g = m_Bot.GetGroup();
            Despierto(g);
            m_Fase = BZTRFase.SUBIENDO;
            m_FaseT0 = now;
            return "llego " + donde + " en " + BZTRLog.Seg(tardo) + ": sube ANIMADO";
        }
        if (tardo > m_BoardTimeoutS) return Teleport(now, tr, bus, r, "no llego a su puerta en " + BZTRLog.Seg(m_BoardTimeoutS));
        return "";
    }

    private string TickSubiendo(float now, Transport tr, EntityAI bus, BZBusService r) {
        HumanCommandVehicle vc = m_Bot.GetCommand_Vehicle();
        if (vc && !vc.IsGettingIn()) return Sentado(now, bus, "animado");
        if (now - m_FaseT0 > SUBIR_MAX_S) return Teleport(now, tr, bus, r, "no termino de subir en " + BZTRLog.Seg(SUBIR_MAX_S));
        return "";
    }

    private string Teleport(float now, Transport tr, EntityAI bus, BZBusService r, string motivo) {
        eAIGroup g = m_Bot.GetGroup();
        // ⛔ el teleport sobre una plaza ocupada le pisa la plaza al que esta sentado (ver SentarCon): espera
        if (OcupanteDePlaza(tr)) return PlazaOcupada(now, tr);
        if (g) g.ClearWaypoints();
        SoltarRumbo();
        SoltarAlto();
        SentarCon(r, tr, true, "BZ_Trader: " + m_Cfg.Name + " " + motivo);
        m_Fase = BZTRFase.TELEPORT;
        m_FaseT0 = now;
        return motivo + ": se lo sienta por TELEPORT";
    }

    private string TickTeleport(float now, EntityAI bus) {
        if (m_Bot.GetCommand_Vehicle()) return Sentado(now, bus, "teleport");
        if (now - m_FaseT0 <= TELEPORT_CONFIRMA_S) return "";
        m_Fase = BZTRFase.PERDIDO;
        return "el teleport no lo dejo en la plaza en " + BZTRLog.Seg(TELEPORT_CONFIRMA_S) + ": PERDIDO (la parada sigue sin el)";
    }

    // ★ la puerta de su plaza se cierra al quedar SENTADO, animado o por teleport (antes el
    // teleport no la cerraba), y se sostiene cerrada PUERTA_SOSTENER_S (SostenerPuerta, desde el runtime). El grupo
    // queda quieto: sin waypoints y, el que no maneja, en formacion NONE (sentado no se duerme: no camina).
    private string Sentado(float now, EntityAI bus, string como) {
        bool hayPuerta = false;
        string anim = "";
        string puerta = "sin puerta";
        eAIGroup g;
        if (bus) {
            ExpansionFSMHelper.DoorAnimationSource(bus, m_Seat, hayPuerta, anim);
            m_TienePuerta = (hayPuerta && anim != "");
            m_PuertaSrc = anim;
            if (m_TienePuerta) {
                bus.SetAnimationPhase(anim, 0.0);
                m_CerrarHasta = now + PUERTA_SOSTENER_S;
                puerta = "puerta cerrada";
            }
        }
        g = m_Bot.GetGroup();
        if (g) {
            g.ClearWaypoints();
            if (!m_Chofer) g.SetFormationState(eAIGroupFormationState.NONE);
        }
        m_Fase = BZTRFase.SENTADO;
        m_ComoSubio = como;
        m_SentadoT = now;
        return "SENTADO (" + como + ") a los " + BZTRLog.Seg(now - m_BajoT) + " de haber bajado · " + puerta;
    }

    // La receta de caminata validada en AutoDrive (crew y chofer): despues de cambiar los waypoints,
    // GetPathFinding().ForceRecalculate(true). Sin eso el camino puede quedar el viejo y el bot no arranca.
    private void RecalcularCamino() {
        if (!m_Bot) return;
        if (m_Bot.GetPathFinding()) m_Bot.GetPathFinding().ForceRecalculate(true);
    }

    static string NombreFormacion(int f) {
        if (f == 0) return "NONE";
        if (f == 1) return "IN";
        if (f == 2) return "FLANK";
        return f.ToString();
    }

    // El estado de la FSM de eAI: "Estado" o "Estado/Subestado" ("?" si no hay).
    string EstadoFsm() {
        string fsm = "?";
        string sub = "";
        eAIFSM maquina;
        ExpansionState estadoFsm;
        ExpansionFSM maquinaSub;
        ExpansionState estadoSub;
        if (!m_Bot) return fsm;
        maquina = m_Bot.GetFSM();
        if (!maquina) return fsm;
        estadoFsm = maquina.GetState();
        if (!estadoFsm) return fsm;
        fsm = estadoFsm.GetName();
        maquinaSub = estadoFsm.GetSubFSM();
        if (maquinaSub) {
            estadoSub = maquinaSub.GetState();
            if (estadoSub) sub = "/" + estadoSub.GetName();
        }
        return fsm + sub;
    }

    // "grupo lider=el|otro, formacion X, wp N MODO (x z · x z)" (o "sin grupo"): los waypoints que sigue eAI y como
    // (ONCE, HALT, LOOP...); hasta 3, con el indice en curso.
    string GrupoTexto() {
        eAIGroup g;
        array<vector> wps;
        string lider = "lider=otro";
        string puntos = "";
        int nWp = 0;
        int i;
        if (!m_Bot) return "sin bot";
        g = m_Bot.GetGroup();
        if (!g) return "sin grupo";
        wps = g.GetWaypoints();
        if (wps) nWp = wps.Count();
        for (i = 0; i < nWp && i < 3; i++) {
            if (puntos != "") puntos = puntos + " · ";
            puntos = puntos + BZTRLog.XZ(wps.Get(i));
        }
        if (g.GetFormationLeader() == m_Bot) lider = "lider=el";
        return "grupo " + lider + ", formacion " + NombreFormacion(g.GetFormationState()) + ", wp " + nWp.ToString() + " " + NombreModoWp(g.GetWaypointBehaviour()) + " idx " + g.m_CurrentWaypointIndex.ToString() + " (" + puntos + ")";
    }

    static string NombreModoWp(int m) {
        if (m == eAIWaypointBehavior.HALT) return "HALT";
        if (m == eAIWaypointBehavior.LOOP) return "LOOP";
        if (m == eAIWaypointBehavior.ALTERNATE) return "ALTERNATE";
        if (m == eAIWaypointBehavior.ONCE) return "ONCE";
        if (m == eAIWaypointBehavior.ROAMING) return "ROAMING";
        return "modo " + m.ToString();
    }

    // ★ el estado de SU plaza, para ver por que Bajar dice "no estaba sentado" (GetCommand_Vehicle null o
    // IsGettingIn) y si el que la ocupa es el. Se loguea tras el teleport del lanzamiento, a +1 s, a +5 s, tras
    // vestirse, antes del HOLD, al congelar y en cada "no baja".
    string EstadoPlaza(Transport tr) {
        HumanCommandVehicle vc;
        string s;
        string comando = "vc null";
        int indice = -1;
        int cmd;
        bool esEl = false;
        bool padre = false;
        if (!m_Bot) return "m_Bot null (nadie enganchado)";
        if (!m_Bot.IsAlive()) return "muerto";
        cmd = m_Bot.GetCurrentCommandID();
        vc = m_Bot.GetCommand_Vehicle();
        if (vc) {
            comando = "vc ok, IsGettingIn " + vc.IsGettingIn().ToString() + ", IsGettingOut " + vc.IsGettingOut().ToString();
            comando = comando + ", GetVehicleSeat " + vc.GetVehicleSeat().ToString();
        }
        if (tr) {
            esEl = (tr.CrewMember(m_Seat) == m_Bot);
            indice = tr.CrewMemberIndex(m_Bot);
            padre = (m_Bot.GetParent() == tr);
        }
        s = "cmd " + cmd.ToString() + " · " + comando + " · CrewMember(" + m_Seat.ToString() + ")==bot " + esEl.ToString();
        s = s + " · CrewMemberIndex " + indice.ToString() + " · IsInTransport " + m_Bot.IsInTransport().ToString() + " · padre=vehiculo " + padre.ToString();
        s = s + " · FSM " + EstadoFsm() + " · " + GrupoTexto() + " · " + BlancoTexto();
        return s;
    }

    // DIAGNOSTICO, una linea por tick mientras esta abajo: donde esta, cuanto le falta, si se mueve,
    // en que comando y en que estado de la FSM de eAI esta, su grupo y la puerta. Si no camina, esto dice por que.
    string Diagnostico(float now, EntityAI bus) {
        string s;
        string volviendo = "?";
        string puerta = "sin puerta";
        vector p;
        vector vel;
        float d = 0;
        bool bzVuelve = false;
        int okVuelve;
        if (!Abajo() || !m_Bot) return "";
        p = m_Bot.GetPosition();
        vel = GetVelocity(m_Bot);
        if (m_Objetivo[0] != 0 || m_Objetivo[2] != 0) d = Math.Sqrt(BZTRJugadores.DistXZ2(p, m_Objetivo));
        okVuelve = EnScript.GetClassVar(m_Bot, "m_BzVolviendo", 0, bzVuelve);
        volviendo = bzVuelve.ToString() + " (GetClassVar " + okVuelve.ToString() + ")";
        if (m_TienePuerta && bus) puerta = "puerta " + bus.GetAnimationPhase(m_PuertaSrc).ToString();
        s = BZTRFase.Nombre(m_Fase) + " t " + BZTRLog.Seg(now - m_FaseT0) + " · " + BZTRLog.Pos3(p) + " · al objetivo " + BZTRLog.Metros(d);
        s = s + " · vel " + BZTRLog.Metros(vel.Length()) + "/s · enVehiculo " + m_Bot.IsInTransport().ToString() + " · cmd " + m_Bot.GetCurrentCommandID().ToString();
        s = s + " · FSM " + EstadoFsm() + " · " + GrupoTexto();
        s = s + " · " + puerta + " · BzVolviendo " + volviendo + " · " + BlancoTexto();
        if (m_Rumbo) s = s + " · rumbo pedido " + Math.Round(m_RumboDeg).ToString() + ", diferencia " + Math.Round(DifRumbo()).ToString();
        if (m_Fase == BZTRFase.VENDIENDO) s = s + " · frenado " + m_Frenado.ToString() + " (eAI " + m_Bot.m_eAI_Halt.ToString() + ") · vuelve caminando " + m_Derivado.ToString();
        return s;
    }

    // El tope duro de la parada: al que siga abajo, teleport.
    string Forzar(float now, Transport tr, EntityAI bus, BZBusService r) {
        if (!Abajo() || m_Fase == BZTRFase.TELEPORT) return "";
        if (!m_Bot || !m_Bot.IsAlive()) {
            m_Fase = BZTRFase.PERDIDO;
            return "ya no existe o murio";
        }
        return Teleport(now, tr, bus, r, "TOPE de la parada (fase " + BZTRFase.Nombre(m_Fase) + ")");
    }
}
