//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Simple entity to transmit message to the client
//
//=============================================================================//

#include "cbase.h"

class CGameMessageEntity : public CLogicalEntity
{
public:
	DECLARE_CLASS( CGameMessageEntity, CLogicalEntity );
	DECLARE_DATADESC();

	CGameMessageEntity( void ) {};

	void InputDisplayMessage( inputdata_t &data );

	string_t	m_strText;
};

LINK_ENTITY_TO_CLASS( logic_game_message, CGameMessageEntity );

BEGIN_DATADESC( CGameMessageEntity )

	DEFINE_KEYFIELD( m_strText, FIELD_STRING, "text" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "DisplayMessage", InputDisplayMessage ),

END_DATADESC()

void CGameMessageEntity::InputDisplayMessage( inputdata_t &data )
{
	// Only send this message the local player	
	CSingleUserRecipientFilter user( UTIL_PlayerByIndex(1) );
	user.MakeReliable();

	// Start the message block
	UserMessageBegin( user, "GameMessage" );

		// Send our text to the client
		WRITE_STRING( STRING( m_strText ) );

	// End the message block
	MessageEnd();
}
