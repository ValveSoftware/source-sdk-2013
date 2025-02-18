#ifndef HAPTIC_MSGS_H
#define HAPTIC_MSGS_H

void RegisterHapticMessages(void);

//-----------------------------------------------------------------------------
// Server
//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL
	void HapticMsg_SendWeaponAnim( CBasePlayer *pPlayer, int iActivity );
	void HapticMsg_SetDrag(CBasePlayer* pPlayer, float drag);
	void HapticMsg_SetConstantForce(CBasePlayer* pPlayer, Vector force);
	void HapticMsg_HapDmg(CBasePlayer* pPlayer, float pitch, float yaw, float dmg, float dmgType );
	void HapticMsg_Punch(CBasePlayer* pPlayer, float x, float y, float z);
	void HapticMsg_MeleeContact(CBasePlayer* pPlayer);
#endif // !CLIENT_DLL

//-----------------------------------------------------------------------------
// Client
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
	void HookHapticMessages(void);

	void __MsgFunc_SPHapWeapEvent( bf_read &HapticMsg );
	void __MsgFunc_HapDmg( bf_read &HapticMsg );
	void __MsgFunc_HapSetConst( bf_read &HapticMsg );
	void __MsgFunc_HapPunch( bf_read &HapticMsg );
	void __MsgFunc_HapGeneric( bf_read &HapticMsg );
	void __MsgFunc_HapSetDrag( bf_read &HapticMsg );
	void __MsgFunc_HapSetDrag( bf_read &HapticMsg );
	void __MsgFunc_HapMeleeContact( bf_read &HapticMsg );
#endif // CLIENT_DLL

#endif // HAPTIC_MSGS_H
