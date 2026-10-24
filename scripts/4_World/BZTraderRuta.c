// ============================================================================
//  LAS MARCAS 4 DE LA TOMA.
//
//  La tecla 4 al grabar NO crea una parada isStop: crea un EVENTO en el _hdr.json de la
//  ruta, y AutoDrive solo frena solo en isStop. Las marcas las lee BZ_Trader y pide la
//  parada el mismo (HoldRunner unos wp antes).
//
//  Criterio (el mismo del visor de eventos de AutoDrive y del editor): una marca es un
//  evento con disparador "wp_reached" o "waypoint", o sin disparador; su wp es trigger.wp
//  o, si falta, el wp del evento; 0 < wp < ultimo; sin repetidos; en orden. Si la toma no
//  trae ningun evento, las filas con stop = 1 del _wp.csv menos la ultima (el final).
// ============================================================================
class BZTRMarca {
    int    Numero;   // 1 = la primera marca de la toma
    int    Wp;
    vector Donde;
}

class BZTRRuta {
    static const string DIR = "$profile:BZ_AutoDrive/";

    // "BZBusRoute_X.json" -> "BZBusRoute_X"
    static string Base(string ruta) {
        string b = ruta;
        int n = b.Length();
        string fin;
        if (n > 5) {
            fin = b.Substring(n - 5, 5);
            fin.ToLower();
            if (fin == ".json") b = b.Substring(0, n - 5);
        }
        return b;
    }

    // Las filas del _wp.csv: posicion y columna stop (3). Mismo criterio que LoadWaypointsCSV: 14+ columnas.
    static void LeerWp(string ruta, array<vector> pos, array<bool> stop) {
        string path = DIR + Base(ruta) + "_wp.csv";
        FileHandle f;
        string linea;
        array<string> c = new array<string>();
        int guarda = 0;
        if (!FileExist(path)) return;
        f = OpenFile(path, FileMode.READ);
        if (f == 0) return;
        while (FGets(f, linea) >= 0 && guarda < 200000) {
            guarda = guarda + 1;
            if (linea == "") continue;
            c.Clear();
            linea.Split(",", c);
            if (c.Count() < 14) continue;
            pos.Insert(Vector(c.Get(0).ToFloat(), c.Get(1).ToFloat(), c.Get(2).ToFloat()));
            stop.Insert(c.Get(3) == "1");
        }
        CloseFile(f);
    }

    // Las marcas de la toma, en orden. 'origen' dice de donde salieron (para el log).
    static void LeerMarcas(string ruta, array<vector> pos, array<ref BZTRMarca> marcas, out string origen) {
        string hdrPath = DIR + Base(ruta) + "_hdr.json";
        string texto = BZTRJson.LeerTexto(hdrPath);
        array<bool> stop = new array<bool>();
        array<int> eventos = new array<int>();
        array<int> wps = new array<int>();
        int raiz;
        int ev;
        int el;
        int trig;
        int w;
        int i;
        int n;
        string tipo;
        BZTRMarca m;
        LeerWp(ruta, pos, stop);
        n = pos.Count();
        origen = "";
        if (texto == "") {
            origen = "sin _hdr.json (" + hdrPath + ")";
        } else {
            raiz = texto.IndexOf("{");
            ev = BZTRJson.ValorDeClave(texto, raiz, "Events");
            if (ev >= 0) BZTRJson.Elementos(texto, ev, eventos);
            for (i = 0; i < eventos.Count(); i++) {
                el = eventos.Get(i);
                if (!BZTRJson.EsObjeto(texto, el)) continue;
                trig = BZTRJson.ValorDeClave(texto, el, "trigger");
                tipo = "wp_reached";
                w = 0;
                if (BZTRJson.EsObjeto(texto, trig)) {
                    tipo = BZTRJson.TextoDeClave(texto, trig, "type");
                    w = BZTRJson.EnteroDeClave(texto, trig, "wp", 0);
                }
                if (tipo != "" && tipo != "wp_reached" && tipo != "waypoint") continue;
                if (w <= 0) w = BZTRJson.EnteroDeClave(texto, el, "wp", 0);
                if (w <= 0 || w >= n - 1) continue;
                if (wps.Find(w) >= 0) continue;
                wps.Insert(w);
            }
            origen = "_hdr.json Events (" + eventos.Count().ToString() + " evento(s))";
        }
        if (wps.Count() == 0) {
            for (i = 0; i < n - 1; i++) {
                if (stop.Get(i)) wps.Insert(i);
            }
            if (wps.Count() > 0) origen = "filas stop = 1 del _wp.csv (la toma no trae eventos de marca)";
        }
        wps.Sort();
        for (i = 0; i < wps.Count(); i++) {
            w = wps.Get(i);
            m = new BZTRMarca();
            m.Numero = i + 1;
            m.Wp = w;
            if (w >= 0 && w < n) m.Donde = pos.Get(w);
            marcas.Insert(m);
        }
    }

    // La clase del vehiculo segun el hdr ("" si no se pudo leer).
    static string ClaseDelVehiculo(string ruta) {
        string texto = BZTRJson.LeerTexto(DIR + Base(ruta) + "_hdr.json");
        if (texto == "") return "";
        return BZTRJson.TextoDeClave(texto, texto.IndexOf("{"), "VehicleClass");
    }
}
