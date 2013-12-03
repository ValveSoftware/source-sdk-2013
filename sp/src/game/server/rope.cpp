//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "rope.h"
#include "entitylist.h"
#include "rope_shared.h"
#include "sendproxy.h"
#include "rope_helpers.h"
#include "te_effect_dispatch.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//--------------------------------------------
// Rope Spawn Flags
//--------------------------------------------
#define SF_ROPE_RESIZE			1		// Automatically resize the rope

// -------------------------------------------------------------------------------- //
// Fun With Tables.
// -------------------------------------------------------------------------------- //

LINK_ENTITY_TO_CLASS( move_rope, CRopeKeyframe );
LINK_ENTITY_TO_CLASS( keyframe_rope, CRopeKeyframe );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CRopeKeyframe, DT_RopeKeyframe )
	SendPropEHandle(SENDINFO(m_hStartPoint)),
	SendPropEHandle(SENDINFO(m_hEndPoint)),
	SendPropInt( SENDINFO(m_iStartAttachment), 5, 0 ),
	SendPropInt( SENDINFO(m_iEndAttachment), 5, 0 ),
	
	SendPropInt( SENDINFO(m_Slack), 12 ),
	SendPropInt( SENDINFO(m_RopeLength), 15 ),
	SendPropInt( SENDINFO(m_fLockedPoints), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_RopeFlags), ROPE_NUMFLAGS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nSegments), 4, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO(m_bConstrainBetweenEndpoints) ),
	SendPropInt( SENDINFO(m_iRopeMaterialModelIndex), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_Subdiv), 4, SPROP_UNSIGNED ),

	SendPropFloat( SENDINFO(m_TextureScale), 10, 0, 0.1f, 10.0f ),
	SendPropFloat( SENDINFO(m_Width), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO(m_flScrollSpeed), 0, SPROP_NOSCALE ),

	SendPropVector(SENDINFO(m_vecOrigin), -1,  SPROP_COORD ),
	SendPropEHandle(SENDINFO_NAME(m_hMoveParent, moveparent) ),

	SendPropInt		(SENDINFO(m_iParentAttachment), NUM_PARENTATTACHMENT_BITS, SPROP_UNSIGNED),
END_SEND_TABLE()


BEGIN_DATADESC( CRopeKeyframe )

	DEFINE_FIELD( m_RopeFlags,		FIELD_INTEGER ),

	DEFINE_KEYFIELD( m_iNextLinkName,	FIELD_STRING,	"NextKey" ),
	DEFINE_KEYFIELD( m_Slack,			FIELD_INTEGER,	"Slack" ),
	DEFINE_KEYFIELD( m_Width,			FIELD_FLOAT,	"Width" ),
	DEFINE_KEYFIELD( m_TextureScale,		FIELD_FLOAT,	"TextureScale" ),
	DEFINE_FIELD( m_nSegments,		FIELD_INTEGER ),
	DEFINE_FIELD( m_bConstrainBetweenEndpoints,		FIELD_BOOLEAN ),

	DEFINE_FIELD( m_strRopeMaterialModel, FIELD_STRING ),
	DEFINE_FIELD( m_iRopeMaterialModelIndex, FIELD_MODELINDEX ),
	DEFINE_KEYFIELD( m_Subdiv,			FIELD_INTEGER,	"Subdiv" ),
	DEFINE_FIELD( m_RopeLength,		FIELD_INTEGER ),
	DEFINE_FIELD( m_fLockedPoints,	FIELD_INTEGER ),
	DEFINE_FIELD( m_bCreatedFromMapFile, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_flScrollSpeed,	FIELD_FLOAT,	"ScrollSpeed" ),

	DEFINE_FIELD( m_bStartPointValid, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bEndPointValid,	FIELD_BOOLEAN ),

	DEFINE_FIELD( m_hStartPoint,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_hEndPoint,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_iStartAttachment,	FIELD_SHORT ),
	DEFINE_FIELD( m_iEndAttachment,	FIELD_SHORT ),
	
	// Inputs
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"SetScrollSpeed",	InputSetScrollSpeed ),
	DEFINE_INPUTFUNC( FIELD_VECTOR,	"SetForce",			InputSetForce ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Break",			InputBreak ),

END_DATADESC()



// -------------------------------------------------------------------------------- //
// CRopeKeyframe implementation.
// -------------------------------------------------------------------------------- //

CRopeKeyframe::CRopeKeyframe()
{
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
	
	m_takedamage = DAMAGE_YES;

	m_iStartAttachment = m_iEndAttachment = 0;

	m_Slack = 0;
	m_Width = 2;
	m_TextureScale = 4;	// 4:1
	m_nSegments = 5;
	m_RopeLength = 20;
	m_fLockedPoints = (int) (ROPE_LOCK_START_POINT | ROPE_LOCK_END_POINT); // by default, both points are locked
	m_flScrollSpeed = 0;
	m_RopeFlags = ROPE_SIMULATE | ROPE_INITIAL_HANG;
	m_iRopeMaterialModelIndex = -1;
	m_Subdiv = 2;

	m_bCreatedFromMapFile = true;
}


CRopeKeyframe::~CRopeKeyframe()
{
	// Release transmit state ownership.
	SetStartPoint( NULL, 0 );
	SetEndPoint( NULL, 0 );
	SetParent( NULL, 0 );
}


void CRopeKeyframe::SetAttachmentPoint( CBaseHandle &hOutEnt, short &iOutAttachment, CBaseEntity *pEnt, int iAttachment )
{
	// Unforce our previously attached entity from transmitting.
	CBaseEntity *pCurEnt = gEntList.GetBaseEntity( hOutEnt );
	if ( pCurEnt && pCurEnt->edict() )
	{
		pCurEnt->DecrementTransmitStateOwnedCounter();
		pCurEnt->DispatchUpdateTransmitState();
	}
	
	hOutEnt = pEnt;
	iOutAttachment = iAttachment;

	// Force this entity to transmit.
	if ( pEnt )
	{
		pEnt->SetTransmitState( FL_EDICT_ALWAYS );
		pEnt->IncrementTransmitStateOwnedCounter();
	}

	EndpointsChanged();
}


void CRopeKeyframe::SetStartPoint( CBaseEntity *pStartPoint, int attachment )
{
	SetAttachmentPoint( m_hStartPoint.GetForModify(), m_iStartAttachment.GetForModify(), pStartPoint, attachment );
}

void CRopeKeyframe::SetEndPoint( CBaseEntity *pEndPoint, int attachment )
{
	SetAttachmentPoint( m_hEndPoint.GetForModify(), m_iEndAttachment.GetForModify(), pEndPoint, attachment );
}

void CRopeKeyframe::SetParent( CBaseEntity *pNewParent, int iAttachment )
{
	CBaseEntity *pCurParent = GetMoveParent();
	if ( pCurParent )
	{
		pCurParent->DecrementTransmitStateOwnedCounter();
		pCurParent->DispatchUpdateTransmitState();
	}

	// Make sure our move parent always transmits or we get asserts on the client.
	if ( pNewParent	)
	{
		pNewParent->IncrementTransmitStateOwnedCounter();
		pNewParent->SetTransmitState( FL_EDICT_ALWAYS );
	}

	BaseClass::SetParent( pNewParent, iAttachment );
}

void CRopeKeyframe::EnablePlayerWeaponAttach( bool bAttach )
{
	int newFlags = m_RopeFlags;
	if ( bAttach )
		newFlags |= ROPE_PLAYER_WPN_ATTACH;
	else
		newFlags &= ~ROPE_PLAYER_WPN_ATTACH;

	if ( newFlags != m_RopeFlags )
	{
		m_RopeFlags = newFlags;
	}
}


CRopeKeyframe* CRopeKeyframe::Create(
	CBaseEntity *pStartEnt,
	CBaseEntity *pEndEnt,
	int iStartAttachment,
	int iEndAttachment,
	int ropeWidth,
	const char *pMaterialName,
	int numSegments
	)
{
	CRopeKeyframe *pRet = (CRopeKeyframe*)CreateEntityByName( "keyframe_rope" );
	if( !pRet )
		return NULL;

	pRet->SetStartPoint( pStartEnt, iStartAttachment );
	pRet->SetEndPoint( pEndEnt, iEndAttachment );
	pRet->m_bCreatedFromMapFile = false;
	pRet->m_RopeFlags &= ~ROPE_INITIAL_HANG;

	pRet->Init();

	pRet->SetMaterial( pMaterialName );
	pRet->m_Width = ropeWidth;
	pRet->m_nSegments = clamp( numSegments, 2, ROPE_MAX_SEGMENTS );

	return pRet;
}


CRopeKeyframe* CRopeKeyframe::CreateWithSecondPointDetached(
	CBaseEntity *pStartEnt,
	int iStartAttachment,
	int ropeLength,
	int ropeWidth,
	const char *pMaterialName,
	int numSegments,
	bool bInitialHang
	)
{
	CRopeKeyframe *pRet = (CRopeKeyframe*)CreateEntityByName( "keyframe_rope" );
	if( !pRet )
		return NULL;

	pRet->SetStartPoint( pStartEnt, iStartAttachment );
	pRet->SetEndPoint( NULL, 0 );
	pRet->m_bCreatedFromMapFile = false;
	pRet->m_fLockedPoints.Set( ROPE_LOCK_START_POINT ); // Only attach the first point.

	if( !bInitialHang )
	{
		pRet->m_RopeFlags &= ~ROPE_INITIAL_HANG;
	}

	pRet->Init();

	pRet->SetMaterial( pMaterialName );
	pRet->m_RopeLength = ropeLength;
	pRet->m_Width = ropeWidth;
	pRet->m_nSegments = clamp( numSegments, 2, ROPE_MAX_SEGMENTS );

	return pRet;
}

void CRopeKeyframe::ActivateStartDirectionConstraints( bool bEnable )
{
	if (bEnable)
	{
		m_fLockedPoints.Set( m_fLockedPoints | ROPE_LOCK_START_DIRECTION ); 
	}
	else
	{
		m_fLockedPoints &= ~((int)ROPE_LOCK_START_DIRECTION); 
	}
}


void CRopeKeyframe::ActivateEndDirectionConstraints( bool bEnable )
{
	if (bEnable)
	{
		m_fLockedPoints.Set( m_fLockedPoints | ROPE_LOCK_END_DIRECTION ); 
	}
	else
	{
		m_fLockedPoints &= ~((int)ROPE_LOCK_END_DIRECTION); 
	}
}


void CRopeKeyframe::ShakeRopes( const Vector &vCenter, float flRadius, float flMagnitude )
{
	CEffectData shakeData;
	shakeData.m_vOrigin = vCenter;
	shakeData.m_flRadius = flRadius;
	shakeData.m_flMagnitude = flMagnitude;
	DispatchEffect( "ShakeRopes", shakeData );
}


bool CRopeKeyframe::SetupHangDistance( float flHangDist )
{
	CBaseEntity *pEnt1 = m_hStartPoint.Get();
	CBaseEntity *pEnt2 = m_hEndPoint.Get();
	if ( !pEnt1 || !pEnt2 )
		return false;

	// Calculate starting conditions so we can force it to hang down N inches.
	Vector v1 = pEnt1->GetAbsOrigin();
	if ( pEnt1->GetBaseAnimating() )
		pEnt1->GetBaseAnimating()->GetAttachment( m_iStartAttachment, v1 );
		
	Vector v2 = pEnt2->GetAbsOrigin();
	if ( pEnt2->GetBaseAnimating() )
		pEnt2->GetBaseAnimating()->GetAttachment( m_iEndAttachment, v2 );

	float flSlack, flLen;
	CalcRopeStartingConditions( v1, v2, ROPE_MAX_SEGMENTS, flHangDist, &flLen, &flSlack );

	m_RopeLength = (int)flLen;
	m_Slack = (int)flSlack;
	return true;
}


void CRopeKeyframe::Init()
{
	SetLocalAngles( vec3_angle );
	RecalculateLength();

	m_nSegments = clamp( (int) m_nSegments, 2, ROPE_MAX_SEGMENTS );

	UpdateBBox( true );

	m_bStartPointValid = (m_hStartPoint.Get() != NULL);
	m_bEndPointValid = (m_hEndPoint.Get() != NULL);
}


void CRopeKeyframe::Activate()
{
	BaseClass::Activate();
	
	if( !m_bCreatedFromMapFile )
		return;

	// Legacy support..
	if ( m_iRopeMaterialModelIndex == -1 )
		m_iRopeMaterialModelIndex = PrecacheModel( "cable/cable.vmt" );

	// Find the next entity in our chain.
	CBaseEntity *pEnt = gEntList.FindEntityByName( NULL, m_iNextLinkName );
	if( pEnt && pEnt->edict() )
	{
		SetEndPoint( pEnt );

		if( m_spawnflags & SF_ROPE_RESIZE )
			m_RopeFlags |= ROPE_RESIZE;
	}
	else
	{
		// If we're from the map file, and we don't have a target ent, and 
		// "Start Dangling" wasn't set, then this rope keyframe doesn't have
		// any rope coming out of it.
		if ( m_fLockedPoints & (int)ROPE_LOCK_END_POINT )
		{
			m_RopeFlags &= ~ROPE_SIMULATE;
		}
	}

	// By default, our start point is our own entity.
	SetStartPoint( this );

	// If we don't do this here, then when we save/load, we won't "own" the transmit 
	// state of our parent, so the client might get our entity without our parent entity.
	SetParent( GetParent(), GetParentAttachment() );

	EndpointsChanged();

	Init();
}

void CRopeKeyframe::EndpointsChanged()
{
	CBaseEntity *pStartEnt = m_hStartPoint.Get();
	if ( pStartEnt )
	{
		if ( (pStartEnt != this) || GetMoveParent() )
		{
			WatchPositionChanges( this, pStartEnt );
		}
	}
	CBaseEntity *pEndEnt = m_hEndPoint.Get();
	if ( pEndEnt )
	{
		if ( (pEndEnt != this) || GetMoveParent() )
		{
			WatchPositionChanges( this, pEndEnt );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Calculate the length of the rope
//-----------------------------------------------------------------------------
void CRopeKeyframe::RecalculateLength( void )
{
	// Get my entities
	if( m_hEndPoint.Get() )
	{
		CBaseEntity *pStartEnt = m_hStartPoint.Get();
		CBaseEntity *pEndEnt = m_hEndPoint.Get();

		// Set the length
		m_RopeLength = (int)( pStartEnt->GetAbsOrigin() - pEndEnt->GetAbsOrigin() ).Length();
	}
	else
	{
		m_RopeLength = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: This should remove the rope next time it reaches a resting state.
//			Right now only the client knows when it reaches a resting state, so
//			for now it just removes itself after a short time.
//-----------------------------------------------------------------------------
void CRopeKeyframe::DieAtNextRest( void )
{
	SetThink( &CBaseEntity::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 1.0f );
}


void CRopeKeyframe::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	if ( !pInfo->m_pTransmitEdict->Get( entindex() ) )
	{	
		BaseClass::SetTransmit( pInfo, bAlways );
	
		// Make sure our target ents are sent too.
		CBaseEntity *pEnt = m_hStartPoint;
		if ( pEnt )
			pEnt->SetTransmit( pInfo, bAlways );

		pEnt = m_hEndPoint;
		if ( pEnt )
			pEnt->SetTransmit( pInfo, bAlways );
	}
}


bool CRopeKeyframe::GetEndPointPos2( CBaseEntity *pAttached, int iAttachment, Vector &vPos )
{
	if( !pAttached )
		return false;

	if ( iAttachment > 0 )
	{
		CBaseAnimating *pAnim = pAttached->GetBaseAnimating();
		if ( pAnim )
		{
			if( !pAnim->GetAttachment( iAttachment, vPos ) )
				return false;
		}
		else
		{
			return false;
		}
	}
	else
	{
		vPos = pAttached->GetAbsOrigin();
	}

	return true;
}


bool CRopeKeyframe::GetEndPointPos( int iPt, Vector &v )
{
	if ( iPt == 0 )
		return GetEndPointPos2( m_hStartPoint, m_iStartAttachment, v );
	else
		return GetEndPointPos2( m_hEndPoint, m_iEndAttachment, v );
}


void CRopeKeyframe::UpdateBBox( bool bForceRelink )
{
	Vector v1, v2;
	Vector vMin, vMax;
	if ( GetEndPointPos( 0, v1 ) )
	{
		if ( GetEndPointPos( 1, v2 ) )
		{
			VectorMin( v1, v2, vMin );
			VectorMax( v1, v2, vMax );

			// Set our bounds to enclose both endpoints and relink.
			vMin -= GetAbsOrigin();
			vMax -= GetAbsOrigin();
		}
		else
		{
			vMin = vMax = v1 - GetAbsOrigin();
		}
	}
	else
	{
		vMin = vMax = Vector( 0, 0, 0 );
	}

	if ( WorldAlignMins() != vMin || WorldAlignMaxs() != vMax )
	{
		UTIL_SetSize( this, vMin, vMax );
	}
}

//------------------------------------------------------------------------------
// Purpose : Propagate force to each link in the rope.  Check for loops
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CRopeKeyframe::PropagateForce(CBaseEntity *pActivator, CBaseEntity *pCaller, CBaseEntity *pFirstLink, float x, float y, float z)
{
	EntityMessageBegin( this, true );
		WRITE_FLOAT( x );
		WRITE_FLOAT( y );
		WRITE_FLOAT( z );
	MessageEnd();

	// UNDONE: Doesn't deal with intermediate loops
	// Propagate to next segment
	CRopeKeyframe *pNextLink = dynamic_cast<CRopeKeyframe*>((CBaseEntity *)m_hEndPoint);
	if (pNextLink && pNextLink != pFirstLink)
	{
		pNextLink->PropagateForce(pActivator, pCaller, pFirstLink, x, y, z);
	}
}

//------------------------------------------------------------------------------
// Purpose: Set an instaneous force on the rope.
// Input  : Force vector.
//------------------------------------------------------------------------------
void CRopeKeyframe::InputSetForce( inputdata_t &inputdata )
{
	Vector vecForce;
	inputdata.value.Vector3D(vecForce);
	PropagateForce( inputdata.pActivator, inputdata.pCaller, this, vecForce.x, vecForce.y, vecForce.z );
}

//-----------------------------------------------------------------------------
// Purpose: Breaks the rope if able
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CRopeKeyframe::InputBreak( inputdata_t &inputdata )
{
	//Route through the damage code
	Break();
}

//-----------------------------------------------------------------------------
// Purpose: Breaks the rope
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRopeKeyframe::Break( void )
{
	DetachPoint( 0 );

	// Find whoever references us and detach us from them.
	// UNDONE: PERFORMANCE: This is very slow!!!
	CRopeKeyframe *pTest = NULL;
	pTest = gEntList.NextEntByClass( pTest );
	while ( pTest )
	{
		if( stricmp( STRING(pTest->m_iNextLinkName), STRING(GetEntityName()) ) == 0 )
		{
			pTest->DetachPoint( 1 );
		}
	
		pTest = gEntList.NextEntByClass( pTest );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRopeKeyframe::NotifyPositionChanged( CBaseEntity *pEntity )
{
	// Update our bbox?
	UpdateBBox( false );

	CBaseEntity *ents[2] = { m_hStartPoint.Get(), m_hEndPoint.Get() };
	if ( (m_RopeFlags & ROPE_RESIZE) && ents[0] && ents[0]->edict() && ents[1] && ents[1]->edict() )
	{
		int len = (int)( ents[0]->GetAbsOrigin() - ents[1]->GetAbsOrigin() ).Length() + m_Slack;
		if ( len != m_RopeLength )
		{
			m_RopeLength = len;
		}
	}

	// Figure out if our attachment points have gone away and make sure to update the client if they have.
	bool *pValid[2] = { &m_bStartPointValid, &m_bEndPointValid };
	for ( int i=0; i < 2; i++ )
	{
		bool bCurrentlyValid = ( ents[i] != NULL );
		if ( *pValid[i] != bCurrentlyValid )
		{
			*pValid[i] = bCurrentlyValid;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Take damage will break the rope
//-----------------------------------------------------------------------------
int CRopeKeyframe::OnTakeDamage( const CTakeDamageInfo &info )
{
	// Only allow this if it's been marked
	if( !(m_RopeFlags & ROPE_BREAKABLE) )
		return false;

	Break();
	return 0;
}


void CRopeKeyframe::Precache()
{
	m_iRopeMaterialModelIndex = PrecacheModel( STRING( m_strRopeMaterialModel ) );
	BaseClass::Precache();
}


void CRopeKeyframe::DetachPoint( int iPoint )
{
	Assert( iPoint == 0 || iPoint == 1 );
	
	m_fLockedPoints &= ~(1 << iPoint);
}


void CRopeKeyframe::EnableCollision()
{
	if( !( m_RopeFlags & ROPE_COLLIDE ) )
	{
		m_RopeFlags |= ROPE_COLLIDE;
	}
}

void CRopeKeyframe::EnableWind( bool bEnable )
{
	int flag = 0;
	if ( !bEnable )
		flag |= ROPE_NO_WIND;

	if ( (m_RopeFlags & ROPE_NO_WIND) != flag )
	{
		m_RopeFlags |= flag;
	}
}


bool CRopeKeyframe::KeyValue( const char *szKeyName, const char *szValue )
{
	if( stricmp( szKeyName, "Breakable" ) == 0 )
	{
		if( atoi( szValue ) == 1 )
			m_RopeFlags |= ROPE_BREAKABLE;
	}
	else if( stricmp( szKeyName, "Collide" ) == 0 )
	{
		if( atoi( szValue ) == 1 )
			m_RopeFlags |= ROPE_COLLIDE;
	}
	else if( stricmp( szKeyName, "Barbed" ) == 0 )
	{
		if( atoi( szValue ) == 1 )
			m_RopeFlags |= ROPE_BARBED;
	}
	else if( stricmp( szKeyName, "Dangling" ) == 0 )
	{
		if( atoi( szValue ) == 1 )
			m_fLockedPoints &= ~ROPE_LOCK_END_POINT; // detach our dest point
		
		return true;
	}
	else if( stricmp( szKeyName, "Type" ) == 0 )
	{
		int iType = atoi( szValue );
		if( iType == 0 )
			m_nSegments = ROPE_MAX_SEGMENTS;
		else if( iType == 1 )
			m_nSegments = ROPE_TYPE1_NUMSEGMENTS;
		else
			m_nSegments = ROPE_TYPE2_NUMSEGMENTS;
	}
	else if ( stricmp( szKeyName, "RopeShader" ) == 0 )
	{
		// Legacy support for the RopeShader parameter.
		int iShader = atoi( szValue );
		if ( iShader == 0 )
		{
			m_iRopeMaterialModelIndex = PrecacheModel( "cable/cable.vmt" );
		}
		else if ( iShader == 1 )
		{
			m_iRopeMaterialModelIndex = PrecacheModel( "cable/rope.vmt" );
		}
		else
		{
			m_iRopeMaterialModelIndex = PrecacheModel( "cable/chain.vmt" );
		}
	}
	else if ( stricmp( szKeyName, "RopeMaterial" ) == 0 )
	{
		// Make sure we have a vmt extension.
		if ( Q_stristr( szValue, ".vmt" ) )
		{
			SetMaterial( szValue );
		}
		else
		{
			char str[512];
			Q_snprintf( str, sizeof( str ), "%s.vmt", szValue );
			SetMaterial( str );
		}
	}
	else if ( stricmp( szKeyName, "NoWind" ) == 0 )
	{
		if ( atoi( szValue ) == 1 )
		{
			m_RopeFlags |= ROPE_NO_WIND;
		}
	}
	
	return BaseClass::KeyValue( szKeyName, szValue );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that sets the scroll speed.
//-----------------------------------------------------------------------------
void CRopeKeyframe::InputSetScrollSpeed( inputdata_t &inputdata )
{
	m_flScrollSpeed = inputdata.value.Float();
}


void CRopeKeyframe::SetMaterial( const char *pName )
{
	m_strRopeMaterialModel = AllocPooledString( pName );
	m_iRopeMaterialModelIndex = PrecacheModel( STRING( m_strRopeMaterialModel ) );
}

int CRopeKeyframe::UpdateTransmitState()
{
	// Certain entities like sprites and ropes are strewn throughout the level and they rarely change.
	// For these entities, it's more efficient to transmit them once and then always leave them on
	// the client. Otherwise, the server will have to send big bursts of data with the entity states
	// as they come in and out of the PVS.
	return SetTransmitState( FL_EDICT_ALWAYS );
}



		
