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
#include "basetempentity.h"
#include "te_effect_dispatch.h"
#include "networkstringtable_gamedll.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: This TE provides a simple interface to dispatch effects by name using DispatchEffect().
//-----------------------------------------------------------------------------
class CTEEffectDispatch : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEEffectDispatch, CBaseTempEntity );

					CTEEffectDispatch( const char *name );
	virtual			~CTEEffectDispatch( void );

	DECLARE_SERVERCLASS();

public:
	CEffectData m_EffectData;
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEEffectDispatch::CTEEffectDispatch( const char *name ) :
	CBaseTempEntity( name )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEEffectDispatch::~CTEEffectDispatch( void )
{
}

IMPLEMENT_SERVERCLASS_ST( CTEEffectDispatch, DT_TEEffectDispatch )

	SendPropDataTable( SENDINFO_DT( m_EffectData ), &REFERENCE_SEND_TABLE( DT_EffectData ) )

END_SEND_TABLE()


// Singleton to fire TEEffectDispatch objects
static CTEEffectDispatch g_TEEffectDispatch( "EffectDispatch" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TE_DispatchEffect( IRecipientFilter& filter, float delay, const Vector &pos, const char *pName, const CEffectData &data )
{
	// Copy the supplied effect data.
	g_TEEffectDispatch.m_EffectData = data;

	// Get the entry index in the string table.
	g_TEEffectDispatch.m_EffectData.m_iEffectName = g_pStringTableEffectDispatch->AddString( CBaseEntity::IsServer(), pName );

	// Send it to anyone who can see the effect's origin.
	g_TEEffectDispatch.Create( filter, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DispatchEffect( const char *pName, const CEffectData &data )
{
	CPASFilter filter( data.m_vOrigin );
	DispatchEffect( pName, data, filter );
}

void DispatchEffect( const char *pName, const CEffectData &data, CRecipientFilter &filter )
{
	te->DispatchEffect( filter, 0.0, data.m_vOrigin, pName, data );
}
