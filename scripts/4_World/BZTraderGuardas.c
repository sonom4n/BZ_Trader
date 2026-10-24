// Adaptado de Traveling Trader Expansion (FUBAR), con permiso de su autor.
// ============================================================================
//  LAS GUARDAS DE NUESTROS COMERCIANTES: un jugador no los esposa, no los recluta, no los cura ni les da de comer.
//
//  Las de TTE (Workshop 3797172077) alcanzan a TODO comerciante IA de Expansion; estas, solo a los de BZ_Trader: los
//  que se engancharon a un servicio (el registro de BZTRManos). Los comerciantes fijos de Expansion y los de otros mods
//  no cambian.
//    - CCTMan es la condicion de objetivo de las acciones vanilla sobre otro jugador: esposar (las dos), mordaza,
//      capucha, RCP, pulso, dar de comer o de beber, sangre, suero, vendas, inyecciones, entablillar, coser, afeitar...
//      Una sola guarda las cubre todas. "Comerciar" no la usa (su condicion es la del cursor): la venta no cambia.
//    - Reclutar: Expansion ya no recluta a un pasivo, pero AutoDrive le saca el pasivo a un tripulante al sentarlo (y
//      BZTRBot se lo vuelve a poner). Esta guarda no depende de eso.
//    - Sin guarda propia: despedir y abrir el inventario de un bot piden que el jugador este en su grupo (solo despues
//      de reclutarlo); lo de un comerciante muerto no aplica (son invencibles).
//  BZ_Trader es un mod de server: estas guardas corren en el server. El cliente puede ofrecer la accion (su condicion
//  no las ve), y el server la rechaza antes de que empiece: estas acciones esperan la confirmacion del server.
//
//  LOS ASIENTOS DE NUESTROS VEHICULOS: ningun jugador se sienta, en ninguna plaza (ni al volante ni de pasajero). El
//  cierre de Expansion (FORCEDLOCKED, BZTRRuntime.CerrarAJugadores) ya lo niega; estas guardas son el respaldo para un
//  server sin el modulo Vehicles de Expansion, donde el cierre no existe: subir (ActionGetInTransport) y cambiarse de
//  plaza (ActionSwitchSeats, y la de Expansion, que la hereda) dan false para un jugador en un vehiculo de un servicio.
//  Nuestros comerciantes y el chofer no usan acciones (los sienta el mod), y otros bots no cambian.
// ============================================================================
class BZTRGuardas {
    // los vehiculos de los servicios (el runtime anota el suyo en cada tick). Punteros comunes: el motor los borra al
    // retirar el vehiculo y quedan en null (se limpian al mirar)
    private static ref array<Object> s_Vehiculos;

    static void AnotarVehiculo(Object veh) {
        if (!veh) return;
        if (!s_Vehiculos) s_Vehiculos = new array<Object>();
        if (s_Vehiculos.Find(veh) < 0) s_Vehiculos.Insert(veh);
    }

    // el botin (el chofer murio): el vehiculo deja de ser de un servicio
    static void QuitarVehiculo(Object veh) {
        if (!veh || !s_Vehiculos) return;
        s_Vehiculos.RemoveItem(veh);
    }

    static bool VehiculoNuestro(Object veh) {
        int i;
        if (!veh || !s_Vehiculos) return false;
        for (i = s_Vehiculos.Count() - 1; i >= 0; i--) {
            if (!s_Vehiculos.Get(i)) s_Vehiculos.Remove(i);
        }
        return s_Vehiculos.Find(veh) >= 0;
    }

    // ¿Un jugador quiere sentarse en una plaza de un vehiculo nuestro? (los bots no: los nuestros no usan la accion)
    static bool PlazaNegada(PlayerBase player, ActionTarget target) {
        if (!player || !target || eAIBase.Cast(player)) return false;
        return VehiculoNuestro(target.GetObject());
    }

    // ¿El objetivo es uno de nuestros comerciantes?
    static bool Nuestro(ActionTarget target) {
        eAIBase bzComerciante;
        if (!target) return false;
        bzComerciante = eAIBase.Cast(target.GetObject());
        if (!bzComerciante) return false;
        return BZTRManos.EsNuestro(bzComerciante);
    }
}

modded class CCTMan {
    override bool Can(PlayerBase player, ActionTarget target) {
        if (BZTRGuardas.Nuestro(target)) return false;
        return super.Can(player, target);
    }
}

modded class ExpansionActionRecruitAI {
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item) {
        if (BZTRGuardas.Nuestro(target)) return false;
        return super.ActionCondition(player, target, item);
    }
}

modded class ActionGetInTransport {
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item) {
        if (BZTRGuardas.PlazaNegada(player, target)) return false;
        return super.ActionCondition(player, target, item);
    }
}

modded class ActionSwitchSeats {
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item) {
        if (BZTRGuardas.PlazaNegada(player, target)) return false;
        return super.ActionCondition(player, target, item);
    }
}
