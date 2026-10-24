// ============================================================================
//  LAS MANOS VACIAS: un comerciante no anda con objetos en las manos (los loadouts les ponen palas y picos)
//
//  De donde sale: los loadouts de fabrica de Expansion (SurvivorLoadout: un arma de mano al azar en la ranura Melee y, en
//  algunos conjuntos, una pistola directo en las manos). Y eAI saca a la mano un arma (de fuego o de mano) cada vez que un
//  bot tiene las manos vacias: la transicion a TakeItemToHands (eAIState_TakeItemToHands.Guard) no mira si es pasivo.
//
//  - Para NUESTROS comerciantes esa transicion falla (el modded de abajo). A Boris y a los bots de otros mods, nada.
//  - Lo que ya tengan en la mano lo guarda BZTRBot.VaciarManos en cada tick: a la espalda o al hombro, al inventario, o se
//    borra si no entra (un comerciante matable sigue teniendo su botin).
// ============================================================================
class BZTRManos {
    // nuestros comerciantes, anotados al engancharlos: tambien es el registro de las guardas (BZTraderGuardas.c)
    static ref array<eAIBase> s_Bots;   // punteros comunes: el motor los anula al borrar el bot

    static void Registrar(eAIBase x) {
        int i;
        if (!x) return;
        if (!s_Bots) s_Bots = new array<eAIBase>();
        for (i = s_Bots.Count() - 1; i >= 0; i--) {
            if (!s_Bots.Get(i)) s_Bots.Remove(i);
        }
        if (s_Bots.Find(x) < 0) s_Bots.Insert(x);
    }

    static bool EsNuestro(eAIBase x) {
        if (!x || !s_Bots) return false;
        return s_Bots.Find(x) >= 0;
    }
}

modded class eAIState_TakeItemToHands {
    override int Guard() {
        if (BZTRManos.EsNuestro(unit)) return eAITransition.FAIL;
        return super.Guard();
    }
}
