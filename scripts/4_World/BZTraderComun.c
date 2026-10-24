// ============================================================================
//  BZ_Trader — comerciantes ambulantes EN VEHICULO.
//
//  ★ QUIEN MANDA: AutoDrive maneja el vehiculo (freeze, unfreeze, despawn).
//  Desde que un bot se baja, es de BZ_Trader: bajar, caminar a su puesto, vender, volver
//  y sentarse. Abajo, cada comerciante camina como un caminante de Traveling Trader
//  Expansion (FUBAR): waypoints del grupo eAI y el alto a pocos metros de un jugador.
//
//  De AutoDrive se usa su API publica del runner y SentarEnPlaza (la unica receta correcta
//  para sentar a un bot). De Expansion, su comercio tal cual (LoadTrader, zonas, loadouts,
//  marcadores). Las guardas de nuestros comerciantes (BZTraderGuardas.c) estan adaptadas de TTE con el permiso de
//  su autor; TTE no hace falta. De AutoDrive y de Expansion no se copia codigo.
// ============================================================================

// Todo sale al RPT: una sola fuente, ordenada junto a las lineas de AutoDrive.
// Formato: [BZ_Trader] [<firma>] mensaje
class BZTRLog {
    static void Info(string quien, string msg) { Print("[BZ_Trader] [" + quien + "] " + msg); }
    static void Warn(string quien, string msg) { Print("[BZ_Trader][WARN] [" + quien + "] " + msg); }
    static void Err(string quien, string msg)  { Print("[BZ_Trader][ERROR] [" + quien + "] " + msg); }

    // Posicion en el mapa (X Z, metros enteros).
    static string XZ(vector p) {
        float x = Math.Round(p[0]);
        float z = Math.Round(p[2]);
        return x.ToString() + " " + z.ToString();
    }

    // Posicion con la altura (X Y Z, un decimal): para los puestos, que vienen sin altura del editor.
    static string Pos3(vector p) {
        float x = Math.Round(p[0] * 10) / 10;
        float y = Math.Round(p[1] * 10) / 10;
        float z = Math.Round(p[2] * 10) / 10;
        return x.ToString() + " " + y.ToString() + " " + z.ToString();
    }

    static string Seg(float s) {
        float r = Math.Round(s);
        return r.ToString() + " s";
    }

    static string Metros(float m) {
        float r = Math.Round(m * 10) / 10;
        return r.ToString() + " m";
    }
}

// Estado de un servicio. Siempre esta en exactamente uno.
class BZTREstado {
    static const int APAGADO   = 0;   // Enable 0 o config invalida: no se lanza nunca
    static const int ESPERANDO = 1;   // lanzamiento programado (arranque, reintento, vuelta nueva)
    static const int LANZADO   = 2;   // runner creado, sin vehiculo todavia: el wp0 esta ocupado y el runner espera
    static const int EN_RUTA   = 3;   // AutoDrive maneja
    static const int PARADA    = 4;   // HOLD en una marca, por una sena o por un fin de leg
    static const int TERMINAL  = 5;   // llego al final (o la ruta cerro): OnRouteEnd
    static const int MUERTO    = 6;   // el chofer murio (hoy todo es invencible: no deberia pasar)
    static const int FIN       = 7;   // no se relanza mas

    static string Nombre(int e) {
        if (e == APAGADO)   return "APAGADO";
        if (e == ESPERANDO) return "ESPERANDO";
        if (e == LANZADO)   return "LANZADO";
        if (e == EN_RUTA)   return "EN_RUTA";
        if (e == PARADA)    return "PARADA";
        if (e == TERMINAL)  return "TERMINAL";
        if (e == MUERTO)    return "MUERTO";
        if (e == FIN)       return "FIN";
        return "?";
    }
}

// ============================================================================
//  PRESENCIA DE CLAVES EN EL JSON.
//
//  JsonFileLoader deja en CERO todo objeto que crea el mismo (cada elemento de un array
//  y cada bloque anidado): no corre los inicializadores. Un campo AUSENTE y un 0 ESCRITO
//  son indistinguibles, y aca el 0 vale en varios campos. Por eso el default se decide por
//  PRESENCIA de la clave en el texto, no por el valor.
//
//  Lector minimo: sabe de objetos, arrays y strings con escapes. No valida el JSON: eso
//  ya lo hizo el JsonSerializer del motor, que corre antes. Trabaja con codigos ASCII
//  para no depender de escapes en literales.
// ============================================================================
class BZTRJson {
    static const int C_COMILLA   = 34;    // "
    static const int C_BARRA     = 92;    // barra invertida
    static const int C_LLAVE_A   = 123;   // {
    static const int C_LLAVE_C   = 125;   // }
    static const int C_CORCHETE_A = 91;   // [
    static const int C_CORCHETE_C = 93;   // ]
    static const int C_DOS_PUNTOS = 58;   // :
    static const int C_COMA      = 44;    // ,

    // El archivo entero como texto ("" si no existe o no se pudo abrir).
    static string LeerTexto(string path) {
        FileHandle f;
        string texto = "";
        if (!FileExist(path)) return "";
        f = OpenFile(path, FileMode.READ);
        if (f == 0) return "";
        ReadFile(f, texto, 1000000);
        CloseFile(f);
        return texto;
    }

    static int Ascii(string t, int i) {
        string c = t.Get(i);
        return c.ToAscii();
    }

    static bool EsBlanco(int a) {
        return a == 32 || a == 9 || a == 10 || a == 13;
    }

    static int SaltarBlancos(string t, int i) {
        int n = t.Length();
        while (i < n) {
            if (!EsBlanco(Ascii(t, i))) return i;
            i = i + 1;
        }
        return n;
    }

    // 'i' en la comilla que abre. Devuelve el indice siguiente a la comilla que cierra.
    static int FinDeString(string t, int i) {
        int n = t.Length();
        int j = i + 1;
        int a;
        while (j < n) {
            a = Ascii(t, j);
            if (a == C_BARRA) {
                j = j + 2;
                continue;
            }
            if (a == C_COMILLA) return j + 1;
            j = j + 1;
        }
        return n;
    }

    // 'i' en el primer caracter de un valor. Devuelve el indice siguiente al valor.
    static int FinDeValor(string t, int i) {
        int n = t.Length();
        int j = i;
        int a;
        int prof = 0;
        if (i >= n) return n;
        a = Ascii(t, i);
        if (a == C_COMILLA) return FinDeString(t, i);
        if (a == C_LLAVE_A || a == C_CORCHETE_A) {
            while (j < n) {
                a = Ascii(t, j);
                if (a == C_COMILLA) {
                    j = FinDeString(t, j);
                    continue;
                }
                if (a == C_LLAVE_A || a == C_CORCHETE_A) prof = prof + 1;
                if (a == C_LLAVE_C || a == C_CORCHETE_C) {
                    prof = prof - 1;
                    if (prof == 0) return j + 1;
                }
                j = j + 1;
            }
            return n;
        }
        // numero, true, false, null
        while (j < n) {
            a = Ascii(t, j);
            if (a == C_COMA || a == C_LLAVE_C || a == C_CORCHETE_C || EsBlanco(a)) return j;
            j = j + 1;
        }
        return n;
    }

    // 'ini' en la llave que abre un objeto. Devuelve el indice donde empieza el valor de 'clave'
    // en el PRIMER nivel de ese objeto, o -1 si no esta escrita.
    static int ValorDeClave(string t, int ini, string clave) {
        int n = t.Length();
        int i;
        int a;
        int finK;
        int finV;
        string k;
        if (ini < 0 || ini >= n) return -1;
        i = SaltarBlancos(t, ini);
        if (i >= n || Ascii(t, i) != C_LLAVE_A) return -1;
        i = i + 1;
        while (i < n) {
            i = SaltarBlancos(t, i);
            if (i >= n) return -1;
            a = Ascii(t, i);
            if (a == C_LLAVE_C) return -1;
            if (a == C_COMA) {
                i = i + 1;
                continue;
            }
            if (a != C_COMILLA) return -1;
            finK = FinDeString(t, i);
            if (finK - i - 2 < 0) return -1;   // string sin cerrar: JSON roto
            k = t.Substring(i + 1, finK - i - 2);
            i = SaltarBlancos(t, finK);
            if (i >= n || Ascii(t, i) != C_DOS_PUNTOS) return -1;
            i = SaltarBlancos(t, i + 1);
            if (k == clave) return i;
            finV = FinDeValor(t, i);
            if (finV <= i) finV = i + 1;
            i = finV;
        }
        return -1;
    }

    static bool Tiene(string t, int ini, string clave) {
        return ValorDeClave(t, ini, clave) >= 0;
    }

    // 'ini' en el corchete que abre un array. Carga en 'salida' donde empieza cada elemento.
    static void Elementos(string t, int ini, array<int> salida) {
        int n = t.Length();
        int i;
        int a;
        int finV;
        if (ini < 0 || ini >= n) return;
        i = SaltarBlancos(t, ini);
        if (i >= n || Ascii(t, i) != C_CORCHETE_A) return;
        i = i + 1;
        while (i < n) {
            i = SaltarBlancos(t, i);
            if (i >= n) return;
            a = Ascii(t, i);
            if (a == C_CORCHETE_C) return;
            if (a == C_COMA) {
                i = i + 1;
                continue;
            }
            salida.Insert(i);
            finV = FinDeValor(t, i);
            if (finV <= i) finV = i + 1;
            i = finV;
        }
    }

    // ¿El valor que empieza en 'i' es un objeto?
    static bool EsObjeto(string t, int i) {
        if (i < 0 || i >= t.Length()) return false;
        return Ascii(t, i) == C_LLAVE_A;
    }

    // El texto crudo del valor que empieza en 'i': un string sin sus comillas, un numero tal cual.
    static string ValorTexto(string t, int i) {
        int fin;
        if (i < 0 || i >= t.Length()) return "";
        fin = FinDeValor(t, i);
        if (Ascii(t, i) == C_COMILLA) {
            if (fin - i - 2 < 0) return "";
            return t.Substring(i + 1, fin - i - 2);
        }
        return t.Substring(i, fin - i);
    }

    static string TextoDeClave(string t, int ini, string clave) {
        int v = ValorDeClave(t, ini, clave);
        if (v < 0) return "";
        return ValorTexto(t, v);
    }

    static int EnteroDeClave(string t, int ini, string clave, int siFalta) {
        int v = ValorDeClave(t, ini, clave);
        string s;
        if (v < 0) return siFalta;
        s = ValorTexto(t, v);
        return s.ToInt();
    }
}

// Un objeto del JSON, para preguntar por la presencia de sus claves. i = -1: no esta escrito.
class BZTRNodo {
    string m_T;
    int    m_I;

    void BZTRNodo(string t, int i) {
        m_T = t;
        m_I = i;
    }

    bool Existe() {
        return BZTRJson.EsObjeto(m_T, m_I);
    }

    bool Tiene(string clave) {
        return BZTRJson.ValorDeClave(m_T, m_I, clave) >= 0;
    }

    BZTRNodo Hijo(string clave) {
        return new BZTRNodo(m_T, BZTRJson.ValorDeClave(m_T, m_I, clave));
    }

    // El elemento n (desde 0) del array 'clave'; no escrito = nodo vacio.
    BZTRNodo Elemento(string clave, int n) {
        array<int> elems = new array<int>();
        int v = BZTRJson.ValorDeClave(m_T, m_I, clave);
        if (v >= 0) BZTRJson.Elementos(m_T, v, elems);
        if (n >= 0 && n < elems.Count()) return new BZTRNodo(m_T, elems.Get(n));
        return new BZTRNodo(m_T, -1);
    }

    int Cuantos(string clave) {
        array<int> elems = new array<int>();
        int v = BZTRJson.ValorDeClave(m_T, m_I, clave);
        if (v >= 0) BZTRJson.Elementos(m_T, v, elems);
        return elems.Count();
    }

    // Un numero ("HeadingDeg": 90) leido del texto; 0 si no esta (preguntar antes con Tiene).
    float Numero(string clave) {
        string s = BZTRJson.TextoDeClave(m_T, m_I, clave);
        return s.ToFloat();
    }

    // Un string ("Type": "smoke") leido del texto; "" si no esta.
    string Texto(string clave) {
        return BZTRJson.TextoDeClave(m_T, m_I, clave);
    }

    // Un array de strings ("Clothing": ["TShirt_Beige", ...]) leido del texto.
    void Textos(string clave, array<string> salida) {
        array<int> elems = new array<int>();
        int v = BZTRJson.ValorDeClave(m_T, m_I, clave);
        int i;
        string s;
        if (v >= 0) BZTRJson.Elementos(m_T, v, elems);
        for (i = 0; i < elems.Count(); i++) {
            s = BZTRJson.ValorTexto(m_T, elems.Get(i));
            if (s != "") salida.Insert(s);
        }
    }

    // Un array de numeros ("Pos": [12812.5, 9021]) leido del texto.
    void Numeros(string clave, array<float> salida) {
        array<int> elems = new array<int>();
        int v = BZTRJson.ValorDeClave(m_T, m_I, clave);
        int i;
        string s;
        if (v >= 0) BZTRJson.Elementos(m_T, v, elems);
        for (i = 0; i < elems.Count(); i++) {
            s = BZTRJson.ValorTexto(m_T, elems.Get(i));
            salida.Insert(s.ToFloat());
        }
    }
}
