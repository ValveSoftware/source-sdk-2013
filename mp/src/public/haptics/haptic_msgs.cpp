#include "cbase.h"

#ifdef CLIENT_DLL
	#include "tier3/tier3.h"
	#include "iviewrender.h"
	#include "inputsystem/iinputsystem.h"
	#include "vgui/IInputInternal.h"
	#include "c_basecombatweapon.h"
	#include "c_baseplayer.h"
	#include "hud_macros.h"
	#include "iclientvehicle.h"
	#include "c_prop_vehicle.h"
	#include "prediction.h"
	#include "activitylist.h"
#ifdef TERROR
	#include "ClientTerrorPlayer.h"
#endif
extern vgui::IInputInternal *g_InputInternal;
#else
	#include "usermessages.h"
#endif

#include "haptics/haptic_msgs.h"

void RegisterHapticMessages(void)
{
	usermessages->Register( "SPHapWeapEvent", 4 );
	usermessages->Register( "HapDmg", -1 );
	usermessages->Register( "HapPunch", -1 );
	usermessages->Register( "HapSetDrag", -1 );
	usermessages->Register( "HapSetConst", -1 );
	usermessages->Register( "HapMeleeContact", 0);
}

//-----------------------------------------------------------------------------
// Server
//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL

void HapticMsg_SendWeaponAnim( CBasePlayer *pPlayer, int iActivity )
{
	//Send the haptics message
	CSingleUserRecipientFilter user( pPlayer );
	user.MakeReliable();
	UserMessageBegin( user, "SPHapWeapEvent" );
	WRITE_LONG(iActivity);
	MessageEnd();
}

void HapticMsg_SetDrag(CBasePlayer* pPlayer, float drag)
{
	CSingleUserRecipientFilter user( pPlayer );
	user.MakeReliable();
	UserMessageBegin( user, "HapSetDrag" );
	WRITE_FLOAT(drag);
	MessageEnd();
}

void HapticMsg_SetConstantForce(CBasePlayer* pPlayer, Vector force)
{
	// portal does not network this.
	CSingleUserRecipientFilter user( pPlayer );
	user.MakeReliable();
	UserMessageBegin( user, "HapSetConst" );
		WRITE_SHORT(force.x);
		WRITE_SHORT(force.y);
		WRITE_SHORT(force.z);
	MessageEnd();
}

void HapticMsg_HapDmg(CBasePlayer* pPlayer, float pitch, float yaw, float dmg, float dmgType )
{
	CSingleUserRecipientFilter user(pPlayer);
	user.MakeReliable();
	UserMessageBegin(user,"HapDmg");

	WRITE_FLOAT(pitch);
	WRITE_FLOAT(yaw);
	WRITE_FLOAT(dmg);
	WRITE_LONG(dmgType);
	MessageEnd();
}

void HapticMsg_Punch(CBasePlayer* pPlayer, float x, float y, float z)
{
	CSingleUserRecipientFilter user(pPlayer);
	user.MakeReliable();
	UserMessageBegin(user,"HapPunch");

	WRITE_FLOAT(x);
	WRITE_FLOAT(y);
	WRITE_FLOAT(z);
	MessageEnd();
}

void HapticMsg_MeleeContact(CBasePlayer* pPlayer)
{
	CSingleUserRecipientFilter user(pPlayer);
	user.MakeReliable();
	UserMessageBegin(user,"HapMeleeContact");
	MessageEnd();
}

#endif //!CLIENT_DLL

//-----------------------------------------------------------------------------
// Client
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
void HookHapticMessages(void)
{
	HOOK_MESSAGE(SPHapWeapEvent);
	HOOK_MESSAGE(HapDmg);
	HOOK_MESSAGE(HapPunch);
	HOOK_MESSAGE(HapSetDrag);
	HOOK_MESSAGE(HapSetConst);
	HOOK_MESSAGE(HapMeleeContact);

}

// Defined in haptics_utils
#ifdef WIN32
void HapticsHandleMsg_HapSetDrag( float drag );
void HapticsHandleMsg_HapSetConst( Vector const &constant );
void HapticsHandleMsg_SPHapWeapEvent( int iActivity );
void HapticsHandleMsg_HapPunch( QAngle const &angle );
void HapticsHandleMsg_HapDmg( float pitch, float yaw, float damage, int damageType );
void HapticsHandleMsg_HapMeleeContact();
#endif // WIN32

void __MsgFunc_HapSetDrag( bf_read &msg )
{
#ifdef WIN32
	float drag = msg.ReadFloat();
	HapticsHandleMsg_HapSetDrag( drag );
#endif // WIN32
}

//Might be able to handle this better...
void __MsgFunc_HapSetConst( bf_read &msg )
{
#ifdef WIN32
	Vector constant;
	constant.x = msg.ReadShort();
	constant.y = msg.ReadShort();
	constant.z = msg.ReadShort();

	HapticsHandleMsg_HapSetConst( constant );
#endif // WIN32
}

void __MsgFunc_SPHapWeapEvent( bf_read &msg )
{
#ifdef WIN32
	int iActivity = msg.ReadLong();

	HapticsHandleMsg_SPHapWeapEvent( iActivity );
#endif // WIN32
}

void __MsgFunc_HapPunch( bf_read &msg )
{
#ifdef WIN32
	float x = msg.ReadFloat();
	float y = msg.ReadFloat();
	float z = msg.ReadFloat();

	HapticsHandleMsg_HapPunch( QAngle(x,y,z) );
#endif // WIN32
}

void __MsgFunc_HapDmg( bf_read &msg )
{
#ifdef WIN32
	float pitch = msg.ReadFloat();
	float yaw = msg.ReadFloat();
	float damage = msg.ReadFloat();
	int damageType = msg.ReadLong();

	HapticsHandleMsg_HapDmg( pitch, yaw, damage, damageType );
#endif // WIN32
}

void __MsgFunc_HapMeleeContact( bf_read &msg )
{
#ifdef WIN32
	HapticsHandleMsg_HapMeleeContact();
#endif // WIN32
}
#endif // CLIENT_DLL


