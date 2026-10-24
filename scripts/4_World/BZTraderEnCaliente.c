// ============================================================================
//  EN CALIENTE: lo del Market que se puede sumar sin reiniciar el server.
//
//  ★ UN COMERCIANTE DEL MARKET NUEVO (profiles\ExpansionMod\Traders\<archivo>.json escrito despues del arranque).
//  El Market lee los comerciantes solo al arrancar, y LoadTraderHost busca SOLO en esa cache (GetMarketTrader:
//  "Trader does not exist"). La API publica alcanza para sumarlo: ExpansionMarketTrader.Load(nombre) lo lee y lo
//  completa con sus categorias (Finalize), y ExpansionMarketSettings.AddMarketTrader lo pone en la cache. El cliente
//  arma el comerciante cuando le llegan los datos por RPC: no lo necesita de antes. Queda MEDIRLO en el juego.
//  ⛔ Las CATEGORIAS no: se cargan solo al arrancar y su lista es protected. Si el archivo nombra una categoria (o un
//  articulo suelto) que el Market no tiene, no se carga y el log dice que hace falta reiniciar.
//
//  La zona nueva sin reinicio esta en BZTRRuntime.BuscarZona (ExpansionMarketTraderZone.Load + SetTraderZone).
// ============================================================================

// Lo minimo de un archivo de comerciante del Market, para revisar que lo que nombra este cargado ANTES de llamar al
// Market (asi el error es el nuestro, claro, y no una cadena de errores del Finalize).
class BZTRComercianteCrudo {
    ref array<string> Categories;
    ref map<string, int> Items;
}

class BZTRMercado {
    static const float REINTENTO_S = 30;   // un archivo que fallo se vuelve a probar a los 30 s (el admin lo arregla)

    private static ref map<string, float>  s_FallaT;
    private static ref map<string, string> s_Falla;

    // "" si el comerciante del Market 'archivo' esta cargado o se pudo cargar ahora; si no, por que no.
    static string Asegurar(string archivo, string firma) {
        ExpansionMarketSettings ms;
        string clave = archivo;
        string falta;
        float now;
        float fallaT;
        if (archivo == "") return "";
        ms = GetExpansionSettings().GetMarket();
        if (!ms) return "el Market no esta cargado";
        if (ms.GetMarketTrader(archivo)) return "";
        if (!s_Falla) {
            s_Falla = new map<string, string>();
            s_FallaT = new map<string, float>();
        }
        clave.ToLower();
        now = GetGame().GetTickTime();
        if (s_FallaT.Find(clave, fallaT) && now - fallaT < REINTENTO_S) return s_Falla.Get(clave);
        falta = Cargar(ms, archivo, firma);
        if (falta == "") {
            s_FallaT.Remove(clave);
            s_Falla.Remove(clave);
            return "";
        }
        s_FallaT.Set(clave, now);
        s_Falla.Set(clave, falta);
        return falta;
    }

    private static string Cargar(ExpansionMarketSettings ms, string archivo, string firma) {
        // = EXPANSION_TRADER_FOLDER del Market (Expansion_Market_Constants.c): escrita, no nombrada (una constante global
        // de otro mod que no resuelve tumba el modulo entero, y el lint no la puede ver)
        string path = "$profile:ExpansionMod\\Traders\\" + archivo + ".json";
        string err;
        string cat;
        string faltanCat = "";
        string faltanArt = "";
        string clase;
        string linea;
        int i;
        int cuenta;
        array<string> partes = new array<string>();
        BZTRComercianteCrudo crudo = new BZTRComercianteCrudo();
        ExpansionMarketTrader t;
        if (!FileExist(path)) return "el comerciante del Market '" + archivo + "' no existe (" + path + "): no va a vender";
        if (!JsonFileLoader<BZTRComercianteCrudo>.LoadFile(path, crudo, err)) return "no se pudo leer " + path + ": " + err;
        if (crudo.Categories) {
            for (i = 0; i < crudo.Categories.Count(); i++) {
                cat = crudo.Categories.Get(i);
                partes.Clear();
                cat.Split(":", partes);
                if (partes.Count() > 0) cat = partes.Get(0);
                if (!ms.GetCategory(cat)) faltanCat = faltanCat + " " + cat;
            }
        }
        if (crudo.Items) {
            foreach (string suelto, int cantidad : crudo.Items) {
                clase = suelto;
                clase.ToLower();
                if (!ExpansionMarketCategory.GetGlobalItem(clase, false)) faltanArt = faltanArt + " " + clase;
            }
        }
        if (faltanCat != "") return "la categoria" + faltanCat + " no esta cargada en el Market: hace falta reiniciar el server (las categorias se leen solo al arrancar) · '" + archivo + "' no se carga";
        if (faltanArt != "") return "el articulo" + faltanArt + " no esta en ninguna categoria cargada del Market: hace falta reiniciar el server · '" + archivo + "' no se carga";
        t = ExpansionMarketTrader.Load(archivo);
        if (!t) return "el Market no pudo cargar '" + archivo + "' (ExpansionMarketTrader.Load devolvio null: ver el error del Market arriba)";
        ms.AddMarketTrader(t);
        cuenta = 0;
        if (t.Items) cuenta = t.Items.Count();
        linea = "comerciante del Market '" + archivo + "' cargado EN CALIENTE (no estaba al arrancar): ExpansionMarketTrader.Load + AddMarketTrader";
        linea = linea + " · " + cuenta.ToString() + " articulo(s) · si un jugador no ve 'Comerciar', hace falta reiniciar (a medir)";
        BZTRLog.Info(firma, linea);
        return "";
    }
}
