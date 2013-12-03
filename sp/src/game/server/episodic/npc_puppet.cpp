//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: NPC Puppet
//
//=============================================================================

#include "cbase.h"
#include "ai_basenpc.h"

// Must be the last file included
#include "memdbgon.h"

class CNPC_Puppet : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_Puppet, CAI_BaseNPC );
public:

	virtual void Spawn( void );
	virtual void Precache( void );

	void	InputSetAnimationTarget( inputdata_t &inputdata );

private:
	
	string_t	m_sAnimTargetname;
	string_t	m_sAnimAttachmentName;

	CNetworkVar( EHANDLE, m_hAnimationTarget );	// NPC that will drive what animation we're playing
	CNetworkVar( int, m_nTargetAttachment );	// Attachment point to match to on the target

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
};

LINK_ENTITY_TO_CLASS( npc_puppet, CNPC_Puppet );

BEGIN_DATADESC( CNPC_Puppet )
	DEFINE_KEYFIELD( m_sAnimTargetname, FIELD_STRING,	"animationtarget" ),
	DEFINE_KEYFIELD( m_sAnimAttachmentName, FIELD_STRING,	"attachmentname" ),

	DEFINE_FIELD( m_nTargetAttachment, FIELD_INTEGER ),
	DEFINE_FIELD( m_hAnimationTarget, FIELD_EHANDLE ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetAnimationTarget", InputSetAnimationTarget ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CNPC_Puppet, DT_NPC_Puppet )
	SendPropEHandle( SENDINFO( m_hAnimationTarget ) ),
	SendPropInt( SENDINFO( m_nTargetAttachment) ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Puppet::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel( STRING( GetModelName() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Puppet::Spawn( void )
{
	BaseClass::Spawn();

	Precache();

	SetModel( STRING( GetModelName() ) );
	
	NPCInit();

	SetHealth( 100 );

	// Find our animation target
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, m_sAnimTargetname );
	m_hAnimationTarget = pTarget;
	if ( pTarget )
	{
		CBaseAnimating *pAnimating = pTarget->GetBaseAnimating();
		if ( pAnimating )
		{
			m_nTargetAttachment = pAnimating->LookupAttachment( STRING( m_sAnimAttachmentName ) );
		}
	}

	// Always be scripted
	SetInAScript( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Puppet::InputSetAnimationTarget( inputdata_t &inputdata )
{
	// Take the new name
	m_sAnimTargetname = MAKE_STRING( inputdata.value.String() );

	// Find our animation target
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, m_sAnimTargetname );
	if ( pTarget == NULL )
	{
		Warning("Failed to find animation target %s for npc_puppet (%s)\n", STRING( m_sAnimTargetname ), STRING( GetEntityName() ) );
		return;
	}
	
	m_hAnimationTarget = pTarget;
	
	CBaseAnimating *pAnimating = pTarget->GetBaseAnimating();
	if ( pAnimating )
	{
		// Cache off our target attachment
		m_nTargetAttachment = pAnimating->LookupAttachment( STRING( m_sAnimAttachmentName ) );
	}

	// Stuff us at the owner's core for visibility reasons
	SetParent( pTarget );
	SetLocalOrigin( vec3_origin );
	SetLocalAngles( vec3_angle );
}
