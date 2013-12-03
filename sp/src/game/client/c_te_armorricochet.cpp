//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "c_basetempentity.h"
#include "IEffects.h"
#include "tier1/KeyValues.h"
#include "tier0/vprof.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Armor Ricochet TE
//-----------------------------------------------------------------------------
class C_TEMetalSparks : public C_BaseTempEntity
{
public:
	DECLARE_CLIENTCLASS();

					C_TEMetalSparks( void );
	virtual			~C_TEMetalSparks( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

	virtual void	Precache( void );

public:
	Vector			m_vecPos;
	Vector			m_vecDir;

	const struct model_t *m_pModel;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEMetalSparks::C_TEMetalSparks( void )
{
	m_vecPos.Init();
	m_vecDir.Init();
	m_pModel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEMetalSparks::~C_TEMetalSparks( void )
{
}

void C_TEMetalSparks::Precache( void )
{
	//m_pModel = engine->LoadModel( "sprites/richo1.vmt" );
}


//-----------------------------------------------------------------------------
// Recording
//-----------------------------------------------------------------------------
static inline void RecordMetalSparks( const Vector &start, const Vector &direction )
{
	if ( !ToolsEnabled() )
		return;

	if ( clienttools->IsInRecordingMode() )
	{
		KeyValues *msg = new KeyValues( "TempEntity" );

 		msg->SetInt( "te", TE_METAL_SPARKS );
 		msg->SetString( "name", "TE_MetalSparks" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetFloat( "originx", start.x );
		msg->SetFloat( "originy", start.y );
		msg->SetFloat( "originz", start.z );
		msg->SetFloat( "directionx", direction.x );
		msg->SetFloat( "directiony", direction.y );
		msg->SetFloat( "directionz", direction.z );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEMetalSparks::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TEMetalSparks::PostDataUpdate" );

	g_pEffects->MetalSparks( m_vecPos, m_vecDir );
	RecordMetalSparks( m_vecPos, m_vecDir );
}

void TE_MetalSparks( IRecipientFilter& filter, float delay,
	const Vector* pos, const Vector* dir )
{
	g_pEffects->MetalSparks( *pos, *dir );
	RecordMetalSparks( *pos, *dir );
}

//-----------------------------------------------------------------------------
// Purpose: Armor Ricochet TE
//-----------------------------------------------------------------------------
class C_TEArmorRicochet : public C_TEMetalSparks
{
	DECLARE_CLASS( C_TEArmorRicochet, C_TEMetalSparks );
public:
	DECLARE_CLIENTCLASS();
	virtual void	PostDataUpdate( DataUpdateType_t updateType );
};


//-----------------------------------------------------------------------------
// Recording
//-----------------------------------------------------------------------------
static inline void RecordArmorRicochet( const Vector &start, const Vector &direction )
{
	if ( !ToolsEnabled() )
		return;

	if ( clienttools->IsInRecordingMode() )
	{
		KeyValues *msg = new KeyValues( "TempEntity" );

 		msg->SetInt( "te", TE_ARMOR_RICOCHET );
 		msg->SetString( "name", "TE_ArmorRicochet" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetFloat( "originx", start.x );
		msg->SetFloat( "originy", start.y );
		msg->SetFloat( "originz", start.z );
		msg->SetFloat( "directionx", direction.x );
		msg->SetFloat( "directiony", direction.y );
		msg->SetFloat( "directionz", direction.z );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Client side version of API
//-----------------------------------------------------------------------------
void TE_ArmorRicochet( IRecipientFilter& filter, float delay,
	const Vector* pos, const Vector* dir )
{
	g_pEffects->Ricochet( *pos, *dir );
	RecordArmorRicochet( *pos, *dir );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEArmorRicochet::PostDataUpdate( DataUpdateType_t updateType )
{
	g_pEffects->Ricochet( m_vecPos, m_vecDir );
	RecordArmorRicochet( m_vecPos, m_vecDir );
}

// Expose the TE to the engine.
IMPLEMENT_CLIENTCLASS_EVENT( C_TEMetalSparks, DT_TEMetalSparks, CTEMetalSparks );

BEGIN_RECV_TABLE_NOBASE(C_TEMetalSparks, DT_TEMetalSparks)
	RecvPropVector(RECVINFO(m_vecPos)),
	RecvPropVector(RECVINFO(m_vecDir)),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_EVENT( C_TEArmorRicochet, DT_TEArmorRicochet, CTEArmorRicochet );
BEGIN_RECV_TABLE(C_TEArmorRicochet, DT_TEArmorRicochet)
END_RECV_TABLE()
