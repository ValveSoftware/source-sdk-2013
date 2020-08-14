//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"


class CEnvCloseCaption : public CBaseEntity
{
	DECLARE_CLASS( CEnvCloseCaption, CBaseEntity );
public:

	bool AllPlayers() { return true; }

	void InputSend( inputdata_t &inputdata );

	//bool m_bCustom;
	int m_iFlags;
	float m_flDuration;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( env_closecaption, CEnvCloseCaption );

BEGIN_DATADESC( CEnvCloseCaption )

	//DEFINE_KEYFIELD( m_bCustom, FIELD_BOOLEAN, "custom" ),
	DEFINE_KEYFIELD( m_iFlags, FIELD_INTEGER, "flags" ),
	DEFINE_INPUT( m_flDuration, FIELD_FLOAT, "SetDuration" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_STRING, "Send", InputSend ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvCloseCaption::InputSend( inputdata_t &inputdata )
{
	char szCC[512];
	Q_strncpy(szCC, inputdata.value.String(), sizeof(szCC));

	byte byteflags = m_iFlags;
	if ( AllPlayers() )
	{
		CReliableBroadcastRecipientFilter user;
		UserMessageBegin( user, "CloseCaption" );
			WRITE_STRING( szCC );
			WRITE_SHORT( MIN( 255, (int)( m_flDuration * 10.0f ) ) ),
			WRITE_BYTE( byteflags ),
		MessageEnd();
	}
	else
	{
		CBaseEntity *pPlayer = NULL;

		if ( inputdata.pActivator && inputdata.pActivator->IsPlayer() )
		{
			pPlayer = inputdata.pActivator;
		}
		else
		{
			pPlayer = UTIL_GetLocalPlayer();
		}

		if ( !pPlayer || !pPlayer->IsNetClient() )
			return;

		CSingleUserRecipientFilter user( (CBasePlayer *)pPlayer );
		user.MakeReliable();
		UserMessageBegin( user, "CloseCaption" );
			WRITE_STRING( szCC );
			WRITE_SHORT( MIN( 255, (int)( m_flDuration * 10.0f ) ) ),
			WRITE_BYTE( byteflags ),
		MessageEnd();
	}
}
