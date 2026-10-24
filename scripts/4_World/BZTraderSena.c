// ============================================================================
//  LA SENA: el gesto de saludo de un jugador.
//
//  PlayEmote corre tambien en el server: registra el gesto en el log de admin (m_AdminLog
//  existe solo en el server) y fija m_CurrentGestureID (EmoteManager.c vanilla, 4_World).
//  Aca solo se avisa al manager; si ese saludo es una sena para algun servicio lo decide el
//  manager (a pie, adelante, cerca, con el vehiculo andando, fuera de una parada).
//  AutoDrive tambien tiene un modded EmoteManager (un getter): no pisa PlayEmote.
// ============================================================================
modded class EmoteManager {
    override bool PlayEmote(int id) {
        bool r = super.PlayEmote(id);
        if (id == EmoteConstants.ID_EMOTE_GREETING && m_Player && GetGame() && GetGame().IsServer()) {
            BZTRManager.Get().Saludo(m_Player);
        }
        return r;
    }
}
