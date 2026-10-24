// ============================================================================
//  LAS GRANADAS DE HUMO AL CERRAR EL SERVER. Medido en el RPT de un cierre con 3 humos encendidos
//  en una parada: "Destroying game" y 6 VME "NULL pointer to instance" en SmokeGrenadeBase.PlaySmokeParticle (vanilla,
//  smokegrenadebase.c:76), 3 ms ANTES de la linea nuestra "señales: 3 apagadas y borradas". Es nuestro propio borrado de
//  OnMissionFinish: en el cierre ObjectDelete corre EEDelete en el acto; el EnergyManager apaga el aparato que estaba
//  encendido (OnDeviceDestroyed -> SwitchOff -> OnWorkStop), la granada pasa al estado END y el motor despacha
//  OnVariablesSynchronized -> RefreshParticlesAndSounds -> PlaySmokeParticle, que pide ParticleManager.GetInstance(): en
//  un server dedicado es null SIEMPRE (GetInstance no lo crea si IsHeadlessOrDedicatedServer). Con el server andando ese
//  camino no se recorre (la sincronia va al cliente): por eso el mismo borrado en pleno juego, con el humo encendido, no
//  da VME. Borrarlas sin apagarlas, o apagarlas antes, pasa por el mismo camino.
//  El corte: sin gestor de particulas no hay particula que tocar. Alcanza a toda granada de humo del server, pero solo
//  agrega la guarda (el cliente no lleva este mod: es de server).
// ============================================================================
modded class SmokeGrenadeBase {
    override protected void PlaySmokeParticle() {
        if (!ParticleManager.GetInstance()) return;   // server dedicado (y el cierre): no hay particulas
        super.PlaySmokeParticle();
    }
}
