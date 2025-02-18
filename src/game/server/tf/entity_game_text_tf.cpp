//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "tf_gamerules.h"
#include "tf_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CTFHudNotify : public CPointEntity
{
public:
	DECLARE_CLASS( CTFHudNotify, CPointEntity );
	DECLARE_DATADESC();

	void InputDisplay( inputdata_t &inputdata );
	void Display( CBaseEntity *pActivator );

private:
	string_t m_iszMessage;
	string_t m_iszIcon;
	int m_iRecipientTeam;
	int m_iBackgroundTeam;
};

LINK_ENTITY_TO_CLASS( game_text_tf, CTFHudNotify );

BEGIN_DATADESC( CTFHudNotify )

DEFINE_KEYFIELD( m_iszMessage, FIELD_STRING, "message" ),
DEFINE_KEYFIELD( m_iszIcon, FIELD_STRING, "icon" ),
DEFINE_KEYFIELD( m_iRecipientTeam, FIELD_INTEGER, "display_to_team" ),
DEFINE_KEYFIELD( m_iBackgroundTeam, FIELD_INTEGER, "background" ),

// Inputs
DEFINE_INPUTFUNC( FIELD_VOID, "Display", InputDisplay ),

END_DATADESC()


void CTFHudNotify::InputDisplay( inputdata_t &inputdata )
{
	Display( inputdata.pActivator );
}

void CTFHudNotify::Display( CBaseEntity *pActivator )
{
	CBroadcastRecipientFilter filter;

	switch( m_iRecipientTeam )
	{
	case TF_TEAM_RED:
		filter.RemoveRecipientsByTeam( GetGlobalTeam(TF_TEAM_BLUE) );
		break;

	case TF_TEAM_BLUE:
		filter.RemoveRecipientsByTeam( GetGlobalTeam(TF_TEAM_RED) );
		break;
	}

	TFGameRules()->SendHudNotification( filter, STRING(m_iszMessage), STRING(m_iszIcon), m_iBackgroundTeam );
}
