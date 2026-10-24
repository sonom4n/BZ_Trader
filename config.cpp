class CfgPatches {
    class BZ_Trader {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        // El comerciante ambulante une dos mods que no son suyos:
        //   - BZ_AutoDrive maneja el vehiculo (API publica del runner).
        //   - El Market de Expansion vende.
        // El comercio ambulante es idea de FUBAR (Traveling Trader Expansion). Las guardas de nuestros comerciantes
        // (BZTraderGuardas.c) estan adaptadas de TTE con el permiso de su autor: TTE no hace falta.
        // requiredAddons fija ademas el ORDEN DE CARGA: sin el, BZBusService / ExpansionTraderAIBase /
        // ExpansionMarketModule / ExpansionActionRecruitAI no resuelven. Navigation va por ExpansionMarkerModule (el
        // marcador).
        requiredAddons[] = {
            "DZ_Data",
            "BZ_AutoDrive",
            "DayZExpansion_Market_Scripts",
            "DayZExpansion_AI_Scripts",
            "DayZExpansion_Navigation_Scripts"
        };
    };
};

class CfgMods {
    class BZ_Trader {
        dir = "BZ_Trader";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "BZ_Trader — Traveling_Trader on Wheels";
        // Traveling trade: FUBAR, author of Traveling Trader Expansion
        // (https://steamcommunity.com/sharedfiles/filedetails/?id=3797172077). Las guardas de los comerciantes estan
        // adaptadas de TTE con el permiso de su autor.
        credits = "Sonom4n, Hiperhipo10, FUBAR (Traveling Trader Expansion)";
        author = "Sonom4n, Hiperhipo10";
        authorID = "0";
        version = "1.0.0";
        extra = 0;
        type = "mod";

        // Solo server en esta etapa: la logica vive en 4_World y el arranque en 5_Mission.
        // ⛔ Sin modded class de ExpansionMarketSettings: se carga a si misma con JsonFileLoader<ExpansionMarketSettings>
        // (this) y un modded rompe esa plantilla: el modulo Game del Market no compila. La zona se busca
        // con la API publica del Market (ver BZTRRuntime.BuscarZona).
        dependencies[] = { "World", "Mission" };

        class defs {
            class worldScriptModule {
                value = "";
                files[] = { "BZ_Trader/scripts/4_World" };
            };
            class missionScriptModule {
                value = "";
                files[] = { "BZ_Trader/scripts/5_Mission" };
            };
        };
    };
};
