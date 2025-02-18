//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "func_ladder.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if !defined( CLIENT_DLL )
/*static*/ ConVar sv_showladders( "sv_showladders", "0", 0, "Show bbox and dismount points for all ladders (must be set before level load.)\n" );
#endif

CUtlVector< CFuncLadder * >	CFuncLadder::s_Ladders;
CUtlVector< CInfoLadderDismount* >	CInfoLadderDismount::s_Dismounts;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncLadder::CFuncLadder() :
	m_bDisabled( false )
{
	s_Ladders.AddToTail( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncLadder::~CFuncLadder()
{
	s_Ladders.FindAndRemove( this );
}

int CFuncLadder::GetLadderCount()
{
	return s_Ladders.Count();
}

CFuncLadder *CFuncLadder::GetLadder( int index )
{
	if ( index < 0 || index >= s_Ladders.Count() )
		return NULL;

	return s_Ladders[ index ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncLadder::Spawn()
{
	BaseClass::Spawn();

	// Entity is symbolid
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	SetCollisionGroup( COLLISION_GROUP_NONE );
	
	//AddFlag( FL_WORLDBRUSH );
	SetModelName( NULL_STRING );

	// Make entity invisible
	AddEffects( EF_NODRAW );
	// No model but should still network
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	Vector playerMins = VEC_HULL_MIN;
	Vector playerMaxs = VEC_HULL_MAX;

	// This will swap them if they are inverted
	SetEndPoints( m_vecPlayerMountPositionTop, m_vecPlayerMountPositionBottom );

#if !defined( CLIENT_DLL )
	trace_t bottomtrace, toptrace;
	UTIL_TraceHull( m_vecPlayerMountPositionBottom, m_vecPlayerMountPositionBottom, 
		playerMins, playerMaxs, MASK_PLAYERSOLID_BRUSHONLY, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &bottomtrace );
	UTIL_TraceHull( m_vecPlayerMountPositionTop, m_vecPlayerMountPositionTop, 
		playerMins, playerMaxs, MASK_PLAYERSOLID_BRUSHONLY, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &toptrace );

	if ( bottomtrace.startsolid || toptrace.startsolid )
	{
		if ( bottomtrace.startsolid )
		{
			DevMsg( 1, "Warning, funcladder with blocked bottom point (%.2f %.2f %.2f) stuck in (%s)\n",
				m_vecPlayerMountPositionBottom.GetX(),
				m_vecPlayerMountPositionBottom.GetY(),
				m_vecPlayerMountPositionBottom.GetZ(),
				bottomtrace.m_pEnt 
					? 
					UTIL_VarArgs( "%s/%s", bottomtrace.m_pEnt->GetClassname(), bottomtrace.m_pEnt->GetEntityName().ToCStr() ) 
					: 
					"NULL" );
		}
		if ( toptrace.startsolid )
		{
			DevMsg( 1, "Warning, funcladder with blocked top point (%.2f %.2f %.2f) stuck in (%s)\n",
				m_vecPlayerMountPositionTop.GetX(),
				m_vecPlayerMountPositionTop.GetY(),
				m_vecPlayerMountPositionTop.GetZ(),
				toptrace.m_pEnt 
					? 
					UTIL_VarArgs( "%s/%s", toptrace.m_pEnt->GetClassname(), toptrace.m_pEnt->GetEntityName().ToCStr() ) 
					: 
					"NULL" );
		}

		// Force geometry overlays on, but only if developer 2 is set...
		if ( developer.GetInt() > 1 )
		{
			m_debugOverlays |= OVERLAY_TEXT_BIT;
		}
	}

	m_vecPlayerMountPositionTop -= GetAbsOrigin();
	m_vecPlayerMountPositionBottom -= GetAbsOrigin();

	// Compute mins, maxs of points
	// 
	Vector mins( MAX_COORD_INTEGER, MAX_COORD_INTEGER, MAX_COORD_INTEGER );
	Vector maxs( -MAX_COORD_INTEGER, -MAX_COORD_INTEGER, -MAX_COORD_INTEGER );
	int i;
	for ( i = 0; i < 3; i++ )
	{
		if ( m_vecPlayerMountPositionBottom.m_Value[ i ] < mins[ i ] )
		{
			mins[ i ] = m_vecPlayerMountPositionBottom.m_Value[ i ];
		}
		if ( m_vecPlayerMountPositionBottom.m_Value[ i ] > maxs[ i ] )
		{
			maxs[ i ] = m_vecPlayerMountPositionBottom.m_Value[ i ];
		}
		if ( m_vecPlayerMountPositionTop.m_Value[ i ] < mins[ i ] )
		{
			mins[ i ] = m_vecPlayerMountPositionTop.m_Value[ i ];
		}
		if ( m_vecPlayerMountPositionTop.m_Value[ i ] > maxs[ i ] )
		{
			maxs[ i ] = m_vecPlayerMountPositionTop.m_Value[ i ];
		}
	}

	// Expand mins/maxs by player hull size
	mins += playerMins;
	maxs += playerMaxs;

	UTIL_SetSize( this, mins, maxs );

	m_bFakeLadder = HasSpawnFlags(SF_LADDER_DONTGETON);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Called after all entities have spawned or after reload from .sav file
//-----------------------------------------------------------------------------
void CFuncLadder::Activate()
{
	// Chain to base class
	BaseClass::Activate();

	// Re-hook up ladder dismount points
	SearchForDismountPoints();

#if !defined( CLIENT_DLL )
	// Show debugging UI if it's active
	if ( sv_showladders.GetBool() )
	{
		m_debugOverlays |= OVERLAY_TEXT_BIT;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncLadder::SearchForDismountPoints()
{
	CUtlVector< CInfoLadderDismountHandle > allNodes;

	Vector topPos;
	Vector bottomPos;

	GetTopPosition( topPos );
	GetBottomPosition( bottomPos );

	float dismount_radius = 100.0f;

	Vector vecBottomToTop = topPos - bottomPos;
	float ladderLength = VectorNormalize( vecBottomToTop );

	float recheck = 40.0f;

	// add both sets of nodes
	FindNearbyDismountPoints( topPos, dismount_radius, m_Dismounts );
	FindNearbyDismountPoints( bottomPos, dismount_radius, m_Dismounts );

	while ( 1 )
	{
		ladderLength -= recheck;
		if ( ladderLength <= 0.0f )
			break;
		bottomPos += recheck * vecBottomToTop;
		FindNearbyDismountPoints( bottomPos, dismount_radius, m_Dismounts );
	}
}

void CFuncLadder::SetEndPoints( const Vector& p1, const Vector& p2 )
{
	m_vecPlayerMountPositionTop = p1;
	m_vecPlayerMountPositionBottom = p2;

	if ( m_vecPlayerMountPositionBottom.GetZ() > m_vecPlayerMountPositionTop.GetZ() )
	{
		Vector temp = m_vecPlayerMountPositionBottom;
		m_vecPlayerMountPositionBottom = m_vecPlayerMountPositionTop;
		m_vecPlayerMountPositionTop = temp;
	}

#if !defined( CLIENT_DLL)
	Vector playerMins = VEC_HULL_MIN;
	Vector playerMaxs = VEC_HULL_MAX;

	trace_t result;
	UTIL_TraceHull( m_vecPlayerMountPositionTop + Vector( 0, 0, 4 ), m_vecPlayerMountPositionTop, 
		playerMins, playerMaxs, MASK_PLAYERSOLID_BRUSHONLY, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &result );

	if ( !result.startsolid )
	{
		m_vecPlayerMountPositionTop = result.endpos;
	}

	UTIL_TraceHull( m_vecPlayerMountPositionBottom + Vector( 0, 0, 4 ), m_vecPlayerMountPositionBottom, 
		playerMins, playerMaxs, MASK_PLAYERSOLID_BRUSHONLY, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &result );

	if ( !result.startsolid )
	{
		m_vecPlayerMountPositionBottom = result.endpos;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncLadder::DrawDebugGeometryOverlays()
{
#if !defined( CLIENT_DLL )

	BaseClass::DrawDebugGeometryOverlays();

	Vector playerMins = VEC_HULL_MIN;
	Vector playerMaxs  = VEC_HULL_MAX;

	Vector topPosition;
	Vector bottomPosition;

	GetTopPosition( topPosition );
	GetBottomPosition( bottomPosition );

	NDebugOverlay::Box( topPosition, playerMins, playerMaxs, 255,0,0,127, 0 );
	NDebugOverlay::Box( bottomPosition, playerMins, playerMaxs, 0,0,255,127, 0 );

	NDebugOverlay::EntityBounds(this, 200, 180, 63, 63, 0);

	trace_t bottomtrace;
	UTIL_TraceHull( m_vecPlayerMountPositionBottom, m_vecPlayerMountPositionBottom, 
		playerMins, playerMaxs, MASK_PLAYERSOLID_BRUSHONLY, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &bottomtrace );

	int c = m_Dismounts.Count();
	for ( int i = 0 ; i < c ; i++ )
	{
		CInfoLadderDismount *pt = m_Dismounts[ i ];
		if ( !pt )
			continue;

		NDebugOverlay::Box(pt->GetAbsOrigin(),Vector( -16, -16, 0 ), Vector( 16, 16, 8 ), 150,0,0, 63, 0);
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : org - 
//-----------------------------------------------------------------------------
void CFuncLadder::GetTopPosition( Vector& org )
{
	ComputeAbsPosition( m_vecPlayerMountPositionTop + GetLocalOrigin(), &org );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : org - 
//-----------------------------------------------------------------------------
void CFuncLadder::GetBottomPosition( Vector& org )
{
	ComputeAbsPosition( m_vecPlayerMountPositionBottom + GetLocalOrigin(), &org );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bottomToTopVec - 
//-----------------------------------------------------------------------------
void CFuncLadder::ComputeLadderDir( Vector& bottomToTopVec )
{
	Vector top;
	Vector bottom;

	GetTopPosition( top );
	GetBottomPosition( bottom );

	bottomToTopVec = top - bottom;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CFuncLadder::GetDismountCount() const
{
	return m_Dismounts.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : CInfoLadderDismountHandle
//-----------------------------------------------------------------------------
CInfoLadderDismount *CFuncLadder::GetDismount( int index_ )
{
	if ( index_ < 0 || index_ >= m_Dismounts.Count() )
		return NULL;
	return m_Dismounts[index_];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//			radius - 
//			list - 
//-----------------------------------------------------------------------------
void CFuncLadder::FindNearbyDismountPoints( const Vector& origin, float radius, CUtlVector< CInfoLadderDismountHandle >& list )
{
#ifdef CLIENT_DLL
	
	for ( int i = 0; i < CInfoLadderDismount::GetDismountCount(); i++ )
	{
		const float flRadiusSqr = radius * radius;

		CInfoLadderDismount *landingspot = CInfoLadderDismount::GetDismount( i );
		if ( landingspot->GetAbsOrigin().DistToSqr( origin ) >= flRadiusSqr )
			continue;
#else
	CBaseEntity *pEntity = NULL;
	while ( (pEntity = gEntList.FindEntityByClassnameWithin( pEntity, "info_ladder_dismount", origin, radius)) != NULL )
	{
		CInfoLadderDismount *landingspot = static_cast< CInfoLadderDismount * >( pEntity );
#endif
		Assert( landingspot );

		// TODO: Tell the client about this.
#ifdef GAME_DLL
		// If spot has a target, then if the target is not this ladder, don't add to our list.
		if ( landingspot->m_target != NULL_STRING )
		{
			if ( landingspot->GetNextTarget() != this )
			{
				continue;
			}
		}
#endif

		CInfoLadderDismountHandle handle;
		handle = landingspot;
		if ( list.Find( handle ) == list.InvalidIndex() )
		{
			list.AddToTail( handle  );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CFuncLadder::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CFuncLadder::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CFuncLadder::PlayerGotOn( CBasePlayer *pPlayer )
{
#if !defined( CLIENT_DLL )
	m_OnPlayerGotOnLadder.FireOutput(this, pPlayer);
	pPlayer->EmitSound( "Ladder.StepRight" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CFuncLadder::PlayerGotOff( CBasePlayer *pPlayer )
{
#if !defined( CLIENT_DLL )
	m_OnPlayerGotOffLadder.FireOutput(this, pPlayer);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFuncLadder::DontGetOnLadder( void ) const
{
	return m_bFakeLadder;
}

#if !defined(CLIENT_DLL)
const char *CFuncLadder::GetSurfacePropName()
{
	if ( !m_surfacePropName )
		return NULL;
	return m_surfacePropName.ToCStr();
}
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( FuncLadder, DT_FuncLadder );

BEGIN_NETWORK_TABLE( CFuncLadder, DT_FuncLadder )
#if !defined( CLIENT_DLL )
	SendPropVector( SENDINFO( m_vecPlayerMountPositionTop ), SPROP_COORD ),
	SendPropVector( SENDINFO( m_vecPlayerMountPositionBottom ), SPROP_COORD ),
	SendPropVector( SENDINFO( m_vecLadderDir ), SPROP_COORD ),
	SendPropBool( SENDINFO( m_bFakeLadder ) ),
//	SendPropStringT( SENDINFO(m_surfacePropName) ),
#else
	RecvPropVector( RECVINFO( m_vecPlayerMountPositionTop ) ),
	RecvPropVector( RECVINFO( m_vecPlayerMountPositionBottom )),
	RecvPropVector( RECVINFO( m_vecLadderDir )),
	RecvPropBool( RECVINFO( m_bFakeLadder ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( func_useableladder, CFuncLadder );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CFuncLadder )
	DEFINE_KEYFIELD( m_vecPlayerMountPositionTop,	FIELD_VECTOR, "point0" ),
	DEFINE_KEYFIELD( m_vecPlayerMountPositionBottom,	FIELD_VECTOR, "point1" ),

	DEFINE_FIELD( m_vecLadderDir, FIELD_VECTOR ),
	// DEFINE_FIELD( m_Dismounts, FIELD_UTLVECTOR ),

	DEFINE_FIELD( m_bFakeLadder, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_bDisabled,	FIELD_BOOLEAN,	"StartDisabled" ),

#if !defined( CLIENT_DLL )
	DEFINE_KEYFIELD( m_surfacePropName,FIELD_STRING,	"ladderSurfaceProperties" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_OUTPUT(	m_OnPlayerGotOnLadder,	"OnPlayerGotOnLadder" ),
	DEFINE_OUTPUT(	m_OnPlayerGotOffLadder,	"OnPlayerGotOffLadder" ),
#endif

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInfoLadderDismount::DrawDebugGeometryOverlays()
{
#if !defined( CLIENT_DLL )
	BaseClass::DrawDebugGeometryOverlays();

	if ( developer.GetBool() )
	{
		NDebugOverlay::Box( GetAbsOrigin(), Vector( -16, -16, 0 ), Vector( 16, 16, 8 ), 127, 127, 127, 127, 0 );
	}
#endif
}

#if defined( GAME_DLL )
int CFuncLadder::UpdateTransmitState()
{
	// transmit if in PVS for clientside prediction
	return SetTransmitState( FL_EDICT_PVSCHECK );
}
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( InfoLadderDismount, DT_InfoLadderDismount );

BEGIN_NETWORK_TABLE( CInfoLadderDismount, DT_InfoLadderDismount )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( info_ladder_dismount, CInfoLadderDismount );

CInfoLadderDismount::CInfoLadderDismount()
{
	s_Dismounts.AddToTail( this );
}

CInfoLadderDismount::~CInfoLadderDismount()
{
	s_Dismounts.FindAndRemove( this );
}

void CInfoLadderDismount::Spawn()
{
	BaseClass::Spawn();

	// Entity is symbolid
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	SetCollisionGroup( COLLISION_GROUP_NONE );

	//AddFlag( FL_WORLDBRUSH );
	SetModelName( NULL_STRING );

	// Make entity invisible
	AddEffects( EF_NODRAW );
	// No model but should still network
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}

/*static*/ int CInfoLadderDismount::GetDismountCount()
{
	return s_Dismounts.Count();
}
/*static*/ CInfoLadderDismount* CInfoLadderDismount::GetDismount( int index )
{
	return s_Dismounts[index];
}

#if defined(GAME_DLL)
int CInfoLadderDismount::UpdateTransmitState()
{
	// transmit if in PVS for clientside prediction
	return SetTransmitState( FL_EDICT_PVSCHECK );
}

const char *FuncLadder_GetSurfaceprops(CBaseEntity *pLadderEntity)
{
	CFuncLadder *pLadder = dynamic_cast<CFuncLadder *>(pLadderEntity);
	if ( pLadder )
	{
		if ( pLadder->GetSurfacePropName() )
			return pLadder->GetSurfacePropName();
	}
	return "ladder";
}
#endif
