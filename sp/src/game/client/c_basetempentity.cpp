//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Core Temp Entity client implementation.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basetempentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS(C_BaseTempEntity, DT_BaseTempEntity, CBaseTempEntity);

BEGIN_RECV_TABLE_NOBASE(C_BaseTempEntity, DT_BaseTempEntity)
END_RECV_TABLE()


// Global list of temp entity classes
C_BaseTempEntity *C_BaseTempEntity::s_pTempEntities = NULL;

// Global list of dynamic temp entities
C_BaseTempEntity *C_BaseTempEntity::s_pDynamicEntities = NULL;

//-----------------------------------------------------------------------------
// Purpose: Returns head of list
// Output : CBaseTempEntity * -- head of list
//-----------------------------------------------------------------------------
C_BaseTempEntity *C_BaseTempEntity::GetDynamicList( void )
{
	return s_pDynamicEntities;
}

//-----------------------------------------------------------------------------
// Purpose: Returns head of list
// Output : CBaseTempEntity * -- head of list
//-----------------------------------------------------------------------------
C_BaseTempEntity *C_BaseTempEntity::GetList( void )
{
	return s_pTempEntities;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
C_BaseTempEntity::C_BaseTempEntity( void )
{
	// Add to list
	m_pNext			= s_pTempEntities;
	s_pTempEntities = this;
	
	m_pNextDynamic = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
C_BaseTempEntity::~C_BaseTempEntity( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get next temp ent in chain
// Output : CBaseTempEntity *
//-----------------------------------------------------------------------------
C_BaseTempEntity *C_BaseTempEntity::GetNext( void )
{
	return m_pNext;
}

//-----------------------------------------------------------------------------
// Purpose: Get next temp ent in chain
// Output : CBaseTempEntity *
//-----------------------------------------------------------------------------
C_BaseTempEntity *C_BaseTempEntity::GetNextDynamic( void )
{
	return m_pNextDynamic;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseTempEntity::Precache( void )
{
	// Nothing...
}

//-----------------------------------------------------------------------------
// Purpose: Called at startup to allow temp entities to precache any models/sounds that they need
//-----------------------------------------------------------------------------
void C_BaseTempEntity::PrecacheTempEnts( void )
{
	C_BaseTempEntity *te = GetList();
	while ( te )
	{
		te->Precache();
		te = te->GetNext();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called at startup and level load to clear out leftover temp entities
//-----------------------------------------------------------------------------
void C_BaseTempEntity::ClearDynamicTempEnts( void )
{
	C_BaseTempEntity *next;
	C_BaseTempEntity *te = s_pDynamicEntities;
	while ( te )
	{
		next = te->GetNextDynamic();
		delete te;
		te = next;
	}

	s_pDynamicEntities = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Called at startup and level load to clear out leftover temp entities
//-----------------------------------------------------------------------------
void C_BaseTempEntity::CheckDynamicTempEnts( void )
{
	C_BaseTempEntity *next, *newlist = NULL;
	C_BaseTempEntity *te = s_pDynamicEntities;
	while ( te )
	{
		next = te->GetNextDynamic();
		if ( te->ShouldDestroy() )
		{
			delete te;
		}
		else
		{
			te->m_pNextDynamic = newlist;
			newlist = te;
		}
		te = next;
	}

	s_pDynamicEntities = newlist;
}

//-----------------------------------------------------------------------------
// Purpose: Dynamic/non-singleton temp entities are initialized by
//  calling into here.  They should be added to a list of C_BaseTempEntities so
//  that their memory can be deallocated appropriately.
// Input  : *pEnt - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseTempEntity::Init( int entnum, int iSerialNum )
{
	if ( entnum != -1 )
	{
		Assert( 0 );
	}

	// Link into dynamic entity list
	m_pNextDynamic = s_pDynamicEntities;
	s_pDynamicEntities = this;

	return true;
}


void C_BaseTempEntity::Release()
{
	Assert( !"C_BaseTempEntity::Release should never be called" );
}


void C_BaseTempEntity::NotifyShouldTransmit( ShouldTransmitState_t state )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_BaseTempEntity::PreDataUpdate( DataUpdateType_t updateType )
{
	// TE's may or may not implement this
}


int C_BaseTempEntity::entindex( void ) const { Assert( 0 ); return 0; }
void C_BaseTempEntity::PostDataUpdate( DataUpdateType_t updateType ) { Assert( 0 ); }
void C_BaseTempEntity::OnPreDataChanged( DataUpdateType_t updateType ) { Assert( 0 ); }
void C_BaseTempEntity::OnDataChanged( DataUpdateType_t updateType ) { Assert( 0 ); }
void C_BaseTempEntity::SetDormant( bool bDormant ) { Assert( 0 ); }
bool C_BaseTempEntity::IsDormant( void ) { Assert( 0 ); return false; };
void C_BaseTempEntity::ReceiveMessage( int classID, bf_read &msg ) { Assert( 0 ); }
void C_BaseTempEntity::SetDestroyedOnRecreateEntities( void ) { Assert(0); }

void* C_BaseTempEntity::GetDataTableBasePtr()
{
	return this;
}

