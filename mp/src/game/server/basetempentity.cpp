//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "mathlib/mathlib.h"
#include "basetempentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_SERVERCLASS_ST_NOBASE(CBaseTempEntity, DT_BaseTempEntity)
END_SEND_TABLE()




// Global list of temp entity event classes
CBaseTempEntity *CBaseTempEntity::s_pTempEntities = NULL;

//-----------------------------------------------------------------------------
// Purpose: Returns head of list
// Output : CBaseTempEntity * -- head of list
//-----------------------------------------------------------------------------
CBaseTempEntity *CBaseTempEntity::GetList( void )
{
	return s_pTempEntities;
}

//-----------------------------------------------------------------------------
// Purpose: Creates temp entity, sets name, adds to global list
// Input  : *name - 
//-----------------------------------------------------------------------------
CBaseTempEntity::CBaseTempEntity( const char *name )
{
	m_pszName = name;
	Assert( m_pszName );

	// Add to list
	m_pNext			= s_pTempEntities;
	s_pTempEntities = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseTempEntity::~CBaseTempEntity( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get the name of this temp entity
// Output : const char *
//-----------------------------------------------------------------------------
const char *CBaseTempEntity::GetName( void )
{
	return m_pszName ? m_pszName : "Unnamed";
}

//-----------------------------------------------------------------------------
// Purpose: Get next temp ent in chain
// Output : CBaseTempEntity *
//-----------------------------------------------------------------------------
CBaseTempEntity *CBaseTempEntity::GetNext( void )
{
	return m_pNext;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTempEntity::Precache( void )
{
	// Nothing...
}

//-----------------------------------------------------------------------------
// Purpose: Default test implementation. Should only be called by derived classes
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CBaseTempEntity::Test( const Vector& current_origin, const QAngle& current_angles )
{
	Vector origin, forward;

	Msg( "%s\n", m_pszName );
	AngleVectors( current_angles, &forward );

	VectorMA( current_origin, 20, forward, origin );

	CBroadcastRecipientFilter filter;

	Create( filter, 0.0 );
}

//-----------------------------------------------------------------------------
// Purpose: Called at startup to allow temp entities to precache any models/sounds that they need
//-----------------------------------------------------------------------------
void CBaseTempEntity::PrecacheTempEnts( void )
{
	CBaseTempEntity *te = GetList();
	while ( te )
	{
		te->Precache();
		te = te->GetNext();
	}
}


void CBaseTempEntity::Create( IRecipientFilter& filter, float delay )
{
	// temp entities can't be reliable or part of the signon message, use real entities instead
	Assert( !filter.IsReliable() && !filter.IsInitMessage() );
	Assert( delay >= -1 && delay <= 1); // 1 second max delay

	engine->PlaybackTempEntity( filter, delay, 
		(void *)this, GetServerClass()->m_pTable, GetServerClass()->m_ClassID );
}
