// ============================================================================
//  EL CASCO DEL VEHICULO: su caja, la puerta de cada plaza y el rodeo a pie.
//
//  Ejemplo, el APC del CarPack: solo las plazas 0 y 1 tienen puerta al costado; de la 2 a la 5
//  se entra y se sale por la compuerta de ATRAS. Un comerciante de la plaza 2 sale por atras, su puesto esta al costado, y eAI no
//  rodea el casco (el vehiculo no esta en su navmesh): sin ayuda, va y viene a 3 m del puesto.
//
//  Coordenadas del vehiculo, en el plano: x = a la derecha, z = adelante, en metros desde su origen
//  (GetDirectionAside y GetDirection, proyectados). La caja sale de GetCollisionBox (o ClippingInfo); sin ninguna, una
//  estimada con las entradas de las plazas.
//  - AL BAJAR se camina desde donde quedo parado (la SALIDA MEDIDA): adentro de la caja (el pasillo del
//    bus), derecho; afuera, con rodeo si el tramo cruza el casco.
//  - A LA VUELTA sube desde un punto afuera del casco (PuntoDeVuelta): su salida medida si quedo afuera; si la entrada
//    cae adentro, el borde mas cercano a la altura de la entrada; si no, el PUNTO DE APROXIMACION de su puerta (la
//    entrada llevada al borde de la caja por el lado al que da, APROX_M mas afuera).
//  - EL RODEO: si el tramo recto entre dos puntos cruza la caja (con ADENTRO_M de margen, el cuerpo del bot), el camino
//    pasa por una o dos esquinas de la caja agrandada en ESQUINA_M. Se elige el mas corto que no cruce.
// ============================================================================
class BZTRCasco {
    static const float ADENTRO_M  = 0.3;
    static const float ESQUINA_M  = 1.5;
    static const float APROX_M    = 1.0;
    static const float COMPARTIDA_M = 1.2;   // dos entradas a menos de esto son la misma puerta
    static const float AFUERA_M   = 1.5;     // un jugador que estaba adentro queda a esto del casco

    float  m_XMin;
    float  m_XMax;
    float  m_ZMin;
    float  m_ZMax;
    string m_Fuente;
    string m_Clase;

    void BZTRCasco(EntityAI veh) {
        Medir(veh);
    }

    private void Medir(EntityAI veh) {
        vector mm[2];
        vector a;
        vector b;
        bool hay = false;
        m_Fuente = "estimada";
        m_XMin = -1;
        m_XMax = 1;
        m_ZMin = -2.5;
        m_ZMax = 2.5;
        if (!veh) return;
        m_Clase = veh.GetType();
        if (veh.GetCollisionBox(mm)) {
            hay = true;
            m_Fuente = "GetCollisionBox";
        } else {
            veh.ClippingInfo(mm);
            hay = true;
            m_Fuente = "ClippingInfo";
        }
        if (hay) {
            a = mm[0];
            b = mm[1];
            if (b[0] - a[0] >= 0.5 && b[2] - a[2] >= 0.5 && b[0] - a[0] < 30 && b[2] - a[2] < 60) {
                m_XMin = a[0];
                m_XMax = b[0];
                m_ZMin = a[2];
                m_ZMax = b[2];
                return;
            }
            m_Fuente = "estimada (" + m_Fuente + " dio una caja que no sirve)";
        }
    }

    // Con las entradas de las plazas: si la caja salio estimada, que al menos las contenga.
    void IncluirEntradas(Transport tr) {
        int i;
        vector p;
        vector d;
        vector l;
        if (!tr || m_Fuente.IndexOf("estimada") != 0) return;
        for (i = 0; i < tr.CrewSize(); i++) {
            tr.CrewEntryWS(i, p, d);
            l = ALocal(tr, p);
            if (l[0] - 0.3 < m_XMin) m_XMin = l[0] - 0.3;
            if (l[0] + 0.3 > m_XMax) m_XMax = l[0] + 0.3;
            if (l[2] - 0.3 < m_ZMin) m_ZMin = l[2] - 0.3;
            if (l[2] + 0.3 > m_ZMax) m_ZMax = l[2] + 0.3;
        }
    }

    // ---- el marco del vehiculo (en el momento en que se llama: el vehiculo se mueve entre paradas)
    static vector Derecha(EntityAI veh) {
        vector v = veh.GetDirectionAside();
        v[1] = 0;
        return v.Normalized();
    }

    static vector Adelante(EntityAI veh) {
        vector v = veh.GetDirection();
        v[1] = 0;
        return v.Normalized();
    }

    // mundo -> (x a la derecha, 0, z adelante)
    static vector ALocal(EntityAI veh, vector w) {
        vector o = veh.GetPosition();
        vector der = Derecha(veh);
        vector ade = Adelante(veh);
        vector d = w - o;
        float x = d[0] * der[0] + d[2] * der[2];
        float z = d[0] * ade[0] + d[2] * ade[2];
        return Vector(x, 0, z);
    }

    // (x, z) del vehiculo -> mundo, al nivel del terreno
    static vector AMundo(EntityAI veh, float x, float z) {
        vector o = veh.GetPosition();
        vector der = Derecha(veh);
        vector ade = Adelante(veh);
        vector w = o + der * x + ade * z;
        w[1] = GetGame().SurfaceY(w[0], w[2]);
        return w;
    }

    // ---- la geometria, en coordenadas del vehiculo
    bool Dentro(vector l, float margen) {
        if (l[0] < m_XMin - margen || l[0] > m_XMax + margen) return false;
        if (l[2] < m_ZMin - margen || l[2] > m_ZMax + margen) return false;
        return true;
    }

    // ¿El tramo a-b (locales) entra a la caja agrandada en 'margen'? Liang-Barsky en el plano.
    bool Cruza(vector a, vector b, float margen) {
        float x0 = m_XMin - margen;
        float x1 = m_XMax + margen;
        float z0 = m_ZMin - margen;
        float z1 = m_ZMax + margen;
        float dx = b[0] - a[0];
        float dz = b[2] - a[2];
        float t0 = 0;
        float t1 = 1;
        if (!Recorte(-dx, a[0] - x0, t0, t1)) return false;
        if (!Recorte(dx, x1 - a[0], t0, t1)) return false;
        if (!Recorte(-dz, a[2] - z0, t0, t1)) return false;
        if (!Recorte(dz, z1 - a[2], t0, t1)) return false;
        return t1 - t0 > 0.0001;
    }

    private static bool Recorte(float p, float q, inout float t0, inout float t1) {
        float r;
        if (p == 0) return q >= 0;
        r = q / p;
        if (p < 0) {
            if (r > t1) return false;
            if (r > t0) t0 = r;
        } else {
            if (r < t0) return false;
            if (r < t1) t1 = r;
        }
        return true;
    }

    // La esquina i (0 adelante-izq, 1 adelante-der, 2 atras-der, 3 atras-izq) de la caja agrandada en ESQUINA_M.
    vector Esquina(int i) {
        float x = m_XMin - ESQUINA_M;
        float z = m_ZMax + ESQUINA_M;
        if (i == 1 || i == 2) x = m_XMax + ESQUINA_M;
        if (i == 2 || i == 3) z = m_ZMin - ESQUINA_M;
        return Vector(x, 0, z);
    }

    static float Largo(vector a, vector b) {
        float dx = b[0] - a[0];
        float dz = b[2] - a[2];
        return Math.Sqrt(dx * dx + dz * dz);
    }

    // El rodeo de a a b (locales): las esquinas a pasar, en orden (0, 1 o 2). Devuelve por donde ("" = derecho; "?" =
    // no hay camino que no cruce, por ejemplo con un extremo pegado al casco: se va derecho).
    string Rodeo(vector a, vector b, array<vector> esquinas) {
        int i;
        int j;
        int mejorI = -1;
        int mejorJ = -1;
        float mejor = 1000000;
        float largo;
        vector ci;
        vector cj;
        esquinas.Clear();
        if (!Cruza(a, b, ADENTRO_M)) return "";
        for (i = 0; i < 4; i++) {
            ci = Esquina(i);
            if (Cruza(a, ci, ADENTRO_M) || Cruza(ci, b, ADENTRO_M)) continue;
            largo = Largo(a, ci) + Largo(ci, b);
            if (largo >= mejor) continue;
            mejor = largo;
            mejorI = i;
            mejorJ = -1;
        }
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                if (j != (i + 1) % 4 && i != (j + 1) % 4) continue;   // solo esquinas vecinas
                ci = Esquina(i);
                cj = Esquina(j);
                if (Cruza(a, ci, ADENTRO_M) || Cruza(cj, b, ADENTRO_M)) continue;
                largo = Largo(a, ci) + Largo(ci, cj) + Largo(cj, b);
                if (largo >= mejor) continue;
                mejor = largo;
                mejorI = i;
                mejorJ = j;
            }
        }
        if (mejorI < 0) return "?";
        ci = Esquina(mejorI);
        esquinas.Insert(ci);
        if (mejorJ < 0) return Lado(mejorI, -1);
        cj = Esquina(mejorJ);
        esquinas.Insert(cj);
        return Lado(mejorI, mejorJ);
    }

    // Por donde pasa un rodeo: adelante (esquinas 0 y 1), atras (2 y 3), por la izquierda (0 y 3), por la derecha (1 y 2).
    private static string Lado(int i, int j) {
        bool adelante = (i == 0 || i == 1);
        bool derecha = (i == 1 || i == 2);
        if (j < 0) {
            if (adelante && derecha) return "adelante a la derecha";
            if (adelante) return "adelante a la izquierda";
            if (derecha) return "atras a la derecha";
            return "atras a la izquierda";
        }
        if ((i == 0 || i == 1) && (j == 0 || j == 1)) return "adelante";
        if ((i == 2 || i == 3) && (j == 2 || j == 3)) return "atras";
        if ((i == 0 || i == 3) && (j == 0 || j == 3)) return "la izquierda";
        return "la derecha";
    }

    // ---- las plazas
    // La entrada de la plaza (local) y a que lado da: el borde de la caja que tiene mas cerca, en proporcion a su medida
    // (en el APC las plazas 2-5 estan sobre el eje, cerca de atras: dan a la compuerta de atras).
    string LadoDe(vector l) {
        float cx = (m_XMin + m_XMax) / 2;
        float cz = (m_ZMin + m_ZMax) / 2;
        float hx = (m_XMax - m_XMin) / 2;
        float hz = (m_ZMax - m_ZMin) / 2;
        float sx;
        float sz;
        if (hx < 0.1) hx = 0.1;
        if (hz < 0.1) hz = 0.1;
        sx = Math.AbsFloat(l[0] - cx) / hx;
        sz = Math.AbsFloat(l[2] - cz) / hz;
        if (sx >= sz) {
            if (l[0] >= cx) return "derecha";
            return "izquierda";
        }
        if (l[2] >= cz) return "adelante";
        return "atras";
    }

    // El punto de aproximacion (local) de una entrada: sobre el borde de su lado, APROX_M afuera.
    vector Aproximacion(vector l, string lado) {
        if (lado == "derecha") return Vector(m_XMax + APROX_M, 0, l[2]);
        if (lado == "izquierda") return Vector(m_XMin - APROX_M, 0, l[2]);
        if (lado == "adelante") return Vector(l[0], 0, m_ZMax + APROX_M);
        return Vector(l[0], 0, m_ZMin - APROX_M);
    }

    vector EntradaLocal(Transport tr, int seat) {
        vector p;
        vector d;
        tr.CrewEntryWS(seat, p, d);
        return ALocal(tr, p);
    }

    // hasta donde llega el casco desde su origen (la esquina mas lejana), para buscar jugadores adentro.
    float Radio() {
        float x = Math.Max(Math.AbsFloat(m_XMin), Math.AbsFloat(m_XMax));
        float z = Math.Max(Math.AbsFloat(m_ZMin), Math.AbsFloat(m_ZMax));
        return Math.Sqrt(x * x + z * z);
    }

    // el punto afuera mas cercano a 'l' (local): sobre el borde mas cercano de la caja, AFUERA_M mas afuera y
    // a la misma altura del vehiculo que 'l'. 'lado' dice cual.
    vector AfueraMasCerca(vector l, out string lado) {
        float x = Math.Clamp(l[0], m_XMin, m_XMax);
        float z = Math.Clamp(l[2], m_ZMin, m_ZMax);
        float dDer = m_XMax - l[0];
        float dIzq = l[0] - m_XMin;
        float dAde = m_ZMax - l[2];
        float dAtr = l[2] - m_ZMin;
        float menor = Math.Min(Math.Min(dDer, dIzq), Math.Min(dAde, dAtr));
        if (menor == dDer) {
            lado = "al costado derecho";
            return Vector(m_XMax + AFUERA_M, 0, z);
        }
        if (menor == dIzq) {
            lado = "al costado izquierdo";
            return Vector(m_XMin - AFUERA_M, 0, z);
        }
        if (menor == dAde) {
            lado = "adelante";
            return Vector(x, 0, m_ZMax + AFUERA_M);
        }
        lado = "atras";
        return Vector(x, 0, m_ZMin - AFUERA_M);
    }

    // ★ el punto donde SUBE a la vuelta (local), en este orden:
    //   1. la salida medida, si quedo AFUERA de la caja (la compuerta de atras del APC): ahi estuvo parado al bajar;
    //   2. si la entrada cae ADENTRO de la caja (el bus: los 22 asientos sobre el pasillo), el borde del casco mas cercano
    //      a 'desde' (donde esta el comerciante), a la altura de la entrada y APROX_M afuera. Al pasillo no se llega a
    //      pie: en el bus quedaban a 2,1-2,3 m de la entrada, contra el costado;
    //   3. si no, la aproximacion de su puerta.
    // 'como' dice cual, para el log.
    vector PuntoDeVuelta(vector entrada, bool hayMedida, vector medida, bool medidaAdentro, vector desde, out string como) {
        array<vector> cand = new array<vector>();
        array<string> nombres = new array<string>();
        vector mejor;
        float dMejor = 1000000;
        float dc;
        int i;
        string lado;
        if (hayMedida && !medidaAdentro) {
            como = "su salida medida (" + LadoDe(medida) + ")";
            return medida;
        }
        if (Dentro(entrada, -ADENTRO_M)) {   // 0,3 m o mas adentro: sobre el borde (una puerta de auto) cuenta como afuera
            cand.Insert(Vector(m_XMax + APROX_M, 0, entrada[2]));
            nombres.Insert("el costado derecho");
            cand.Insert(Vector(m_XMin - APROX_M, 0, entrada[2]));
            nombres.Insert("el costado izquierdo");
            cand.Insert(Vector(entrada[0], 0, m_ZMax + APROX_M));
            nombres.Insert("el frente");
            cand.Insert(Vector(entrada[0], 0, m_ZMin - APROX_M));
            nombres.Insert("la cola");
            como = "?";
            for (i = 0; i < cand.Count(); i++) {
                dc = Largo(desde, cand.Get(i));
                if (dc >= dMejor) continue;
                dMejor = dc;
                mejor = cand.Get(i);
                como = nombres.Get(i) + " del casco, a la altura de su asiento (la entrada esta adentro)";
            }
            return mejor;
        }
        lado = LadoDe(entrada);
        como = "su puerta (" + lado + ")";
        return Aproximacion(entrada, lado);
    }

    // El camino a pie de 'desde' a 'hasta' (mundo) que rodea el casco: agrega a 'salida' las esquinas (mundo) y devuelve
    // por donde rodea ("" si va derecho).
    string Camino(Transport tr, vector desde, vector hasta, array<vector> salida) {
        array<vector> esq = new array<vector>();
        vector a = ALocal(tr, desde);
        vector b = ALocal(tr, hasta);
        vector c;
        vector w;
        int i;
        string lado = Rodeo(a, b, esq);
        for (i = 0; i < esq.Count(); i++) {
            c = esq.Get(i);
            w = AMundo(tr, c[0], c[2]);
            salida.Insert(w);
        }
        return lado;
    }

    // ¿Dos plazas comparten puerta? La misma animacion de puerta (DoorsTrunk) o entradas a menos de COMPARTIDA_M.
    static bool Comparten(Transport tr, int s1, int s2) {
        vector p1;
        vector p2;
        vector d;
        bool h1 = false;
        bool h2 = false;
        string a1 = "";
        string a2 = "";
        float dx;
        float dz;
        if (!tr || s1 == s2) return false;
        ExpansionFSMHelper.DoorAnimationSource(tr, s1, h1, a1);
        ExpansionFSMHelper.DoorAnimationSource(tr, s2, h2, a2);
        if (h1 && h2 && a1 != "" && a1 == a2) return true;
        tr.CrewEntryWS(s1, p1, d);
        tr.CrewEntryWS(s2, p2, d);
        dx = p1[0] - p2[0];
        dz = p1[2] - p2[2];
        return dx * dx + dz * dz <= COMPARTIDA_M * COMPARTIDA_M;
    }
}

// ============================================================================
//  EL ARCHIVO DE PLAZAS DE UNA CLASE DE VEHICULO, para el editor:
//  $profile:BZ_Trader/_plazas/<VehicleClass>.json, escrito la primera vez que el mod ve la clase en cada arranque.
//  Todo relativo al vehiculo: ...Right = a la derecha, ...Forward = adelante, ...Up = arriba, en metros desde su origen.
// ============================================================================
class BZTRPlazaArchivo {
    int    Seat;
    float  EntryRight;
    float  EntryForward;
    float  EntryUp;
    float  DirRight;
    float  DirForward;
    string DoorSide;
    float  ApproachRight;
    float  ApproachForward;
    string DoorAnimation;
    ref array<int> SharedWith;
    // donde quedo parado a pie la primera vez que bajo por esa plaza (medido; se conserva entre arranques)
    float  ExitRight;
    float  ExitForward;
    bool   ExitMeasured;
    bool   ExitInsideHull;    // cae 0,3 m o mas adentro de la caja (el pasillo del bus): desde ahi no se rodea
    // el ASIENTO (CrewTransform del motor): donde se sienta, no la puerta. El editor dibuja cada plaza ahi; un archivo
    // de antes no lo trae y se completa en el proximo lanzamiento de la clase
    float  SeatRight;
    float  SeatForward;
    float  SeatUp;
    bool   SeatMeasured;

    void BZTRPlazaArchivo() {
        SharedWith = new array<int>();
    }
}

class BZTRPlazasArchivo {
    int    Version;
    string VehicleClass;
    int    CrewSize;
    ref array<float> HullBox;      // [derecha min, derecha max, adelante min, adelante max]
    string HullBoxSource;
    ref array<ref BZTRPlazaArchivo> SeatDoors;

    void BZTRPlazasArchivo() {
        HullBox = new array<float>();
        SeatDoors = new array<ref BZTRPlazaArchivo>();
    }
}

class BZTRPlazas {
    static const string DIR = "$profile:BZ_Trader/_plazas";
    static ref array<string> s_Escritas;
    static ref map<string, ref BZTRPlazasArchivo> s_Archivos;   // lo escrito en este arranque, por clase
    static ref array<string> s_SalidasMedidas;                  // "clase|plaza" ya medidas en este arranque

    // ★ la salida medida de una plaza (local), la primera vez que baja en este arranque: se anota en el
    // archivo de la clase (para el editor). Devuelve la linea para el log ("" si ya estaba o no hay archivo).
    static string AnotarSalida(Transport tr, int seat, vector l, bool adentro) {
        BZTRPlazasArchivo f;
        BZTRPlazaArchivo p;
        BZTRPlazaArchivo q;
        string clase;
        string clave;
        string path;
        string err;
        string donde = "afuera de la caja";
        float x;
        float z;
        int i;
        if (!tr || !s_Archivos) return "";
        clase = tr.GetType();
        clave = clase + "|" + seat.ToString();
        if (!s_SalidasMedidas) s_SalidasMedidas = new array<string>();
        if (s_SalidasMedidas.Find(clave) >= 0) return "";
        s_SalidasMedidas.Insert(clave);
        if (!s_Archivos.Find(clase, f) || !f) return "";
        for (i = 0; i < f.SeatDoors.Count(); i++) {
            q = f.SeatDoors.Get(i);
            if (q && q.Seat == seat) p = q;
        }
        if (!p) return "";
        p.ExitRight = l[0];
        p.ExitForward = l[2];
        p.ExitMeasured = true;
        p.ExitInsideHull = adentro;
        if (adentro) donde = "ADENTRO de la caja (no se rodea desde ahi)";
        path = DIR + "/" + clase + ".json";
        if (!JsonFileLoader<BZTRPlazasArchivo>.SaveFile(path, f, err)) return "salida medida, pero no se pudo escribir " + path + ": " + err;
        x = Math.Round(l[0] * 10) / 10;
        z = Math.Round(l[2] * 10) / 10;
        return "salida medida: a la derecha " + x.ToString() + " m, adelante " + z.ToString() + " m, " + donde + " · anotada en " + path;
    }

    // Escribe el archivo de la clase si no se escribio en este arranque. Devuelve la linea para el log ("" si nada).
    static string Escribir(Transport tr, BZTRCasco casco) {
        BZTRPlazasArchivo f;
        BZTRPlazaArchivo p;
        vector w;
        vector d;
        vector l;
        vector ap;
        vector o;
        vector der;
        vector ade;
        string clase;
        string path;
        string err;
        string resumen = "";
        bool hay;
        string anim;
        int i;
        int j;
        int heredadas = 0;
        BZTRPlazasArchivo viejo;
        BZTRPlazaArchivo pv;
        vector asientoMat[4];
        vector asientoW;
        vector asientoL;
        if (!tr || !casco) return "";
        clase = tr.GetType();
        if (!s_Escritas) s_Escritas = new array<string>();
        if (s_Escritas.Find(clase) >= 0) return "";
        s_Escritas.Insert(clase);
        if (!FileExist(DIR)) MakeDirectory(DIR);
        f = new BZTRPlazasArchivo();
        f.Version = 1;
        f.VehicleClass = clase;
        f.CrewSize = tr.CrewSize();
        f.HullBox.Insert(casco.m_XMin);
        f.HullBox.Insert(casco.m_XMax);
        f.HullBox.Insert(casco.m_ZMin);
        f.HullBox.Insert(casco.m_ZMax);
        f.HullBoxSource = casco.m_Fuente;
        o = tr.GetPosition();
        der = BZTRCasco.Derecha(tr);
        ade = BZTRCasco.Adelante(tr);
        for (i = 0; i < tr.CrewSize(); i++) {
            tr.CrewEntryWS(i, w, d);
            l = BZTRCasco.ALocal(tr, w);
            p = new BZTRPlazaArchivo();
            p.Seat = i;
            p.EntryRight = l[0];
            p.EntryForward = l[2];
            p.EntryUp = w[1] - o[1];
            p.DirRight = d[0] * der[0] + d[2] * der[2];
            p.DirForward = d[0] * ade[0] + d[2] * ade[2];
            // donde se sienta (la entrada es la puerta: en un auto queda afuera; en el APC, la compuerta comun)
            tr.CrewTransformWS(i, asientoMat);
            asientoW = asientoMat[3];
            asientoL = BZTRCasco.ALocal(tr, asientoW);
            p.SeatRight = asientoL[0];
            p.SeatForward = asientoL[2];
            p.SeatUp = asientoW[1] - o[1];
            p.SeatMeasured = true;
            p.DoorSide = casco.LadoDe(l);
            ap = casco.Aproximacion(l, p.DoorSide);
            p.ApproachRight = ap[0];
            p.ApproachForward = ap[2];
            hay = false;
            anim = "";
            ExpansionFSMHelper.DoorAnimationSource(tr, i, hay, anim);
            if (hay) p.DoorAnimation = anim;
            for (j = 0; j < tr.CrewSize(); j++) {
                if (j != i && BZTRCasco.Comparten(tr, i, j)) p.SharedWith.Insert(j);
            }
            f.SeatDoors.Insert(p);
            resumen = resumen + " " + i.ToString() + ":" + p.DoorSide;
        }
        path = DIR + "/" + clase + ".json";
        // las salidas medidas en arranques anteriores se conservan (misma clase y mismas plazas)
        if (FileExist(path)) {
            viejo = new BZTRPlazasArchivo();
            if (JsonFileLoader<BZTRPlazasArchivo>.LoadFile(path, viejo, err) && viejo.VehicleClass == clase && viejo.CrewSize == f.CrewSize && viejo.SeatDoors) {
                for (i = 0; i < viejo.SeatDoors.Count(); i++) {
                    pv = viejo.SeatDoors.Get(i);
                    if (!pv || !pv.ExitMeasured || pv.Seat < 0 || pv.Seat >= f.SeatDoors.Count()) continue;
                    p = f.SeatDoors.Get(pv.Seat);
                    p.ExitRight = pv.ExitRight;
                    p.ExitForward = pv.ExitForward;
                    p.ExitMeasured = true;
                    p.ExitInsideHull = pv.ExitInsideHull;
                    heredadas = heredadas + 1;
                }
            }
        }
        if (!s_Archivos) s_Archivos = new map<string, ref BZTRPlazasArchivo>();
        s_Archivos.Set(clase, f);
        if (!JsonFileLoader<BZTRPlazasArchivo>.SaveFile(path, f, err)) return "no se pudo escribir " + path + ": " + err;
        resumen = "plazas de " + clase + " escritas en " + path + " (para el editor) · caja " + casco.m_Fuente + " · asientos medidos (CrewTransform) · puertas:" + resumen;
        if (heredadas > 0) resumen = resumen + " · " + heredadas.ToString() + " salida(s) medida(s) en arranques anteriores, conservadas";
        return resumen;
    }
}
