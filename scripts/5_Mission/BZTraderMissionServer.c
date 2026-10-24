// Enganche al ciclo de vida del server. AutoDrive y el Market ya inicializaron en su propio
// OnInit (requiredAddons fija el orden de carga); el manager arranca unos segundos despues.
modded class MissionServer {
    override void OnInit() {
        super.OnInit();
        if (GetGame() && GetGame().IsServer()) {
            BZTRManager.Get().Arrancar();
        }
    }

    // ⚠ El cierre LIMPIO pasa por aca: se retiran los vehiculos para que la persistencia no los guarde.
    override void OnMissionFinish() {
        if (GetGame() && GetGame().IsServer()) {
            BZTRManager.Get().Apagar();
        }
        super.OnMissionFinish();
    }
}
