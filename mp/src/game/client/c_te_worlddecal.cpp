//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "c_basetempentity.h"
#include "iefx.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"
#include "fx.h"
#include "decals.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: World Decal TE
//-----------------------------------------------------------------------------
class C_TEWorldDecal : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEWorldDecal, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEWorldDecal( void );
	virtual			~C_TEWorldDecal( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

	virtual void	Precache( void );

public:
	Vector			m_vecOrigin;
	int				m_nIndex;
};


//-----------------------------------------------------------------------------
// Networking 
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEWorldDecal, DT_TEWorldDecal, CTEWorldDecal)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropInt( RECVINFO(m_nIndex)),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
C_TEWorldDecal::C_TEWorldDecal( void )
{
	m_vecOrigin.Init();
	m_nIndex = 0;
}

C_TEWorldDecal::~C_TEWorldDecal( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEWorldDecal::Precache( void )
{
}


//-----------------------------------------------------------------------------
// Shared code
//-----------------------------------------------------------------------------
static inline void RecordWorldDecal( const Vector *pos, int index )
{
	if ( !ToolsEnabled() )
		return;

	if ( clienttools->IsInRecordingMode() )
	{
		KeyValues *msg = new KeyValues( "TempEntity" );

 		msg->SetInt( "te", TE_WORLD_DECAL );
 		msg->SetString( "name", "TE_WorldDecal" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetFloat( "originx", pos->x );
		msg->SetFloat( "originy", pos->y );
		msg->SetFloat( "originz", pos->z );
		msg->SetString( "decalname", effects->Draw_DecalNameFromIndex( index ) );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEWorldDecal::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TEWorldDecal::PostDataUpdate" );

	if ( r_decals.GetInt() )
	{
		C_BaseEntity *ent = cl_entitylist->GetEnt( 0 );
		if ( ent )
		{
			bool bNoBlood = UTIL_IsLowViolence();
			bool bIsBlood = false;

			if ( bNoBlood )
			{
				const char *pchDecalName = decalsystem->GetDecalNameForIndex( m_nIndex );
				if ( pchDecalName && V_stristr( pchDecalName, "blood" ) )
				{
					bIsBlood = true;
				}
			}

			if ( !( bNoBlood && bIsBlood ) )
			{
				effects->DecalShoot( m_nIndex, 0, ent->GetModel(), ent->GetAbsOrigin(), ent->GetAbsAngles(), m_vecOrigin, 0, 0 );
			}
		}
	}
	RecordWorldDecal( &m_vecOrigin, m_nIndex );
}


//-----------------------------------------------------------------------------
// Client-side effects
//-----------------------------------------------------------------------------
void TE_WorldDecal( IRecipientFilter& filter, float delay, const Vector* pos, int index )
{
	if ( r_decals.GetInt() )
	{
		C_BaseEntity *ent = cl_entitylist->GetEnt( 0 );
		if ( ent )
		{
			effects->DecalShoot( index, 0, ent->GetModel(), ent->GetAbsOrigin(), ent->GetAbsAngles(), *pos, 0, 0 );
		}
	}
	RecordWorldDecal( pos, index );
}


void TE_WorldDecal( IRecipientFilter& filter, float delay, KeyValues *pKeyValues )
{
	Vector vecOrigin;
	vecOrigin.x = pKeyValues->GetFloat( "originx" );
	vecOrigin.y = pKeyValues->GetFloat( "originy" );
	vecOrigin.z = pKeyValues->GetFloat( "originz" );
	const char *pDecalName = pKeyValues->GetString( "decalname" );

	TE_WorldDecal( filter, 0.0f, &vecOrigin, effects->Draw_DecalIndexFromName( (char*)pDecalName ) );
}
