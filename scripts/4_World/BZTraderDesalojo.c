// ============================================================================
//  BZ_Trader - el DESALOJO: nunca se retira un vehiculo con un jugador a bordo.
//
//  DayZ 1.30: borrar un vehiculo con un Man sentado tumba el server, y el retiro de AutoDrive cuenta solo las plazas
//  (CrewMember). Antes de cada UnregisterRunner, BZ_Trader mira quien esta a bordo, en cualquier clase
//  (BZTRJugadores.ABordo): sentado en una plaza, sentado sin figurar en ninguna, parado ARRIBA (enganchado a lo que
//  pisa: la caja de un camion, el techo, el piso del bus) y parado adentro de la caja del casco.
//  Si hay alguno, el runner NO se desregistra: queda registrado y frenado (HoldRunner) en esta lista, y cerrado (nadie
//  mas sube). Al parado se lo pone en el piso, al costado (BZTRBajada: si vuelve a subir o no se lo pudo mover, de nuevo
//  cada REBAJAR_S), y se retira en el tick siguiente; al sentado se lo espera (desde la 1.28 el server no lo puede bajar
//  de un asiento). Sin mensajes al jugador: solo el log. Recien con el vehiculo vacio de jugadores se lo desregistra. El
//  servicio sigue como si se hubiera retirado: relanza o termina con un vehiculo nuevo. La lista es de todo el mod (la
//  tickea el manager): un servicio que se reemplaza o termina no deja su vehiculo colgado.
// ============================================================================
class BZTRDesalojo {
    static const float REBAJAR_S = 20;   // al parado que vuelve a subir (o que no se pudo mover), de nuevo cada esto
    static const float LOG_S   = 60;     // cada cuanto se repite la linea del log
    static const float TOPE_S  = 600;    // a los 10 min se dice una vez: el vehiculo se queda hasta que se bajen

    private static ref array<ref BZTRDesalojo> s_Lista;

    BZBusService   m_Runner;
    ref BZTRCasco  m_Casco;
    ref BZTRBajada m_Bajada;
    string         m_Firma;
    string         m_Motivo;
    float          m_T0;
    float          m_LogT;
    bool           m_TopeDicho;

    // Antes de un UnregisterRunner. false = no hay jugadores a bordo (se puede retirar ya); true = los hay: el runner
    // queda en la lista, frenado y registrado, y se desregistra solo cuando este vacio.
    static bool Pedir(BZBusService r, BZTRCasco cascoServicio, string firma, string motivo) {
        Transport tr;
        BZTRCasco casco = cascoServicio;   // un parametro no es referencia fuerte: el new de abajo va a esta local
        array<PlayerBase> sentados = new array<PlayerBase>();
        array<PlayerBase> parados = new array<PlayerBase>();
        BZTRDesalojo d;
        string cierre;
        float now = GetGame().GetTickTime();
        if (!r) return false;
        tr = Transport.Cast(r.GetBus());
        if (!tr) return false;
        // sin el casco del servicio (todavia no medido), uno medido ahora: para mirar adentro y para el costado
        if (!casco) {
            casco = new BZTRCasco(tr);
            casco.IncluirEntradas(tr);
        }
        if (BZTRJugadores.ABordo(tr, casco, sentados, parados) == 0) return false;
        if (!s_Lista) s_Lista = new array<ref BZTRDesalojo>();
        d = new BZTRDesalojo();
        d.m_Runner = r;
        d.m_Casco = casco;
        d.m_Bajada = new BZTRBajada();
        d.m_Firma = firma;
        d.m_Motivo = motivo;
        d.m_T0 = now;
        d.m_LogT = now;
        s_Lista.Insert(d);
        r.HoldRunner();
        cierre = Cerrar(tr);
        d.m_Bajada.Bajar(tr, casco, parados, now, REBAJAR_S, firma, "desalojo");
        BZTRLog.Warn(firma, motivo + ": " + JugadoresTexto(sentados, parados) + " a bordo -> el vehiculo NO se retira: queda frenado y registrado y se retira recien vacio (nunca con alguien adentro)" + cierre);
        return true;
    }

    // Desde el tick del manager (1 s).
    static void Tick() {
        int i;
        BZTRDesalojo d;
        float now = GetGame().GetTickTime();
        if (!s_Lista) return;
        for (i = s_Lista.Count() - 1; i >= 0; i--) {
            d = s_Lista.Get(i);
            if (!d || d.Paso(now)) s_Lista.Remove(i);
        }
    }

    static int Pendientes() {
        if (!s_Lista) return 0;
        return s_Lista.Count();
    }

    // El cierre del server: no se espera ni se borra nada con jugadores a bordo. AutoDrive crea el vehiculo sin
    // persistencia: no vuelve al arrancar.
    static void Apagar() {
        int i;
        BZTRDesalojo d;
        if (!s_Lista) return;
        for (i = 0; i < s_Lista.Count(); i++) {
            d = s_Lista.Get(i);
            if (d) BZTRLog.Warn(d.m_Firma, "cierre del server con jugador(es) a bordo: el vehiculo no se retira (" + d.m_Motivo + "); no persiste");
        }
        s_Lista.Clear();
    }

    // true = terminado (desregistrado, o el runner ya no estaba): sale de la lista.
    private bool Paso(float now) {
        Transport tr;
        array<PlayerBase> sentados = new array<PlayerBase>();
        array<PlayerBase> parados = new array<PlayerBase>();
        array<ref BZBusService> rs = BZBusService.GetRunners();
        if (!m_Runner || !rs || rs.Find(m_Runner) < 0) {
            BZTRLog.Info(m_Firma, "desalojo: el runner ya no esta en el registro de AutoDrive: nada que desregistrar (" + m_Motivo + ")");
            return true;
        }
        tr = Transport.Cast(m_Runner.GetBus());
        if (tr) BZTRJugadores.ABordo(tr, m_Casco, sentados, parados);
        if (sentados.Count() + parados.Count() == 0) {
            BZBusService.UnregisterRunner(m_Runner);
            BZTRLog.Info(m_Firma, "desalojo: vacio de jugadores tras " + BZTRLog.Seg(now - m_T0) + " -> UnregisterRunner hecho (" + m_Motivo + ")");
            return true;
        }
        m_Runner.HoldRunner();
        if (parados.Count() > 0) m_Bajada.Bajar(tr, m_Casco, parados, now, REBAJAR_S, m_Firma, "desalojo");
        if (now - m_LogT >= LOG_S) {
            m_LogT = now;
            BZTRLog.Warn(m_Firma, "desalojo: sigue(n) a bordo tras " + BZTRLog.Seg(now - m_T0) + ": " + JugadoresTexto(sentados, parados) + " (" + m_Motivo + ")");
        }
        if (!m_TopeDicho && now - m_T0 >= TOPE_S) {
            m_TopeDicho = true;
            BZTRLog.Err(m_Firma, "desalojo: a los " + BZTRLog.Seg(TOPE_S) + " sigue(n) a bordo " + JugadoresTexto(sentados, parados) + ": el vehiculo se queda frenado y registrado hasta que se bajen (nunca se borra con alguien adentro)");
        }
        return false;
    }

    // ★ Mientras espera, nadie mas sube: el cierre de Expansion (FORCEDLOCKED) niega subir a una plaza. Bajarse y
    // cambiarse de asiento no lo miran (ActionGetOutTransport y ActionSwitchSeats de Expansion): no encierra al que esta.
    // Devuelve el tramo para el log.
    private static string Cerrar(Transport tr) {
        ExpansionVehicle exv;
        if (!tr || !ExpansionVehicle.Get(exv, tr) || !exv) return " · no es un vehiculo de Expansion: no se lo puede cerrar";
        if (!exv.IsLocked()) exv.SetLockState(ExpansionVehicleLockState.FORCEDLOCKED);
        if (!exv.IsLocked()) return " · el cierre no quedo puesto (falta el modulo Vehicles de Expansion?)";
        return " · cerrado (FORCEDLOCKED): nadie mas sube, bajarse se puede";
    }

    private static string JugadoresTexto(array<PlayerBase> sentados, array<PlayerBase> parados) {
        string s = "";
        if (sentados.Count() > 0) s = sentados.Count().ToString() + " sentado(s) (" + BZTRJugadores.Nombres(sentados) + ")";
        if (parados.Count() > 0) {
            if (s != "") s = s + " y ";
            s = s + parados.Count().ToString() + " parado(s) arriba o adentro (" + BZTRJugadores.Nombres(parados) + ")";
        }
        return s;
    }
}

// ★ Poner en el piso, al costado (el borde mas cercano del casco, BZTRCasco.AfueraMasCerca, al nivel del terreno), a
// los parados arriba o adentro de un vehiculo que se retira. A cada uno como mucho una vez cada 'cadaS': si vuelve a
// subir, o si no se lo pudo mover, se repite; mientras tanto el desalojo espera. Sin mensaje al jugador: el log.
class BZTRBajada {
    ref array<PlayerBase> m_Movidos;
    ref array<float>      m_MovidosT;

    void BZTRBajada() {
        m_Movidos = new array<PlayerBase>();
        m_MovidosT = new array<float>();
    }

    void Limpiar() {
        m_Movidos.Clear();
        m_MovidosT.Clear();
    }

    // Devuelve cuantos movio ahora.
    int Bajar(Transport tr, BZTRCasco casco, array<PlayerBase> parados, float now, float cadaS, string firma, string quien) {
        int i;
        int idx;
        int movidos = 0;
        PlayerBase pb;
        vector antes;
        vector afuera;
        vector destino;
        string lado;
        string nombre;
        string otraVez;
        string linea;
        if (!tr || !casco || !parados) return 0;
        for (i = 0; i < parados.Count(); i++) {
            pb = parados.Get(i);
            if (!pb) continue;
            idx = m_Movidos.Find(pb);
            if (idx >= 0 && now - m_MovidosT.Get(idx) < cadaS) continue;
            otraVez = "";
            if (idx >= 0) otraVez = " (otra vez: volvio a subir o no se lo pudo mover)";
            antes = pb.GetPosition();
            afuera = casco.AfueraMasCerca(BZTRCasco.ALocal(tr, antes), lado);
            destino = BZTRCasco.AMundo(tr, afuera[0], afuera[2]);
            pb.SetPosition(destino);
            if (idx >= 0) {
                m_MovidosT.Set(idx, now);
            } else {
                m_Movidos.Insert(pb);
                m_MovidosT.Insert(now);
            }
            movidos = movidos + 1;
            nombre = "?";
            if (pb.GetIdentity()) nombre = pb.GetIdentity().GetName();
            linea = quien + ": " + nombre + " parado arriba o adentro de " + tr.GetType() + ": se lo pone en el piso, al costado";
            linea = linea + " (de " + BZTRLog.XZ(antes) + " a " + BZTRLog.XZ(destino) + ", " + lado + ")" + otraVez;
            BZTRLog.Info(firma, linea);
        }
        return movidos;
    }
}
