//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base class for all animating characters and objects.
//
//=============================================================================//

#include "cbase.h"
#include "baseanimating.h"
#include "animation.h"
#include "activitylist.h"
#include "studio.h"
#include "bone_setup.h"
#include "mathlib/mathlib.h"
#include "model_types.h"
#include "datacache/imdlcache.h"
#include "physics.h"
#include "ndebugoverlay.h"
#include "tier1/strtools.h"
#include "npcevent.h"
#include "isaverestore.h"
#include "KeyValues.h"
#include "tier0/vprof.h"
#include "EntityFlame.h"
#include "EntityDissolve.h"
#include "ai_basenpc.h"
#include "physics_prop_ragdoll.h"
#include "datacache/idatacache.h"
#include "smoke_trail.h"
#include "props.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar ai_sequence_debug( "ai_sequence_debug", "0" );

class CIKSaveRestoreOps : public CClassPtrSaveRestoreOps
{
	// save data type interface
	void Save( const SaveRestoreFieldInfo_t &fieldInfo, ISave *pSave )
	{
		Assert( fieldInfo.pTypeDesc->fieldSize == 1 );
		CIKContext **pIK = (CIKContext **)fieldInfo.pField;
		bool bHasIK = (*pIK) != 0;
		pSave->WriteBool( &bHasIK );
	}

	void Restore( const SaveRestoreFieldInfo_t &fieldInfo, IRestore *pRestore )
	{
		Assert( fieldInfo.pTypeDesc->fieldSize == 1 );
		CIKContext **pIK = (CIKContext **)fieldInfo.pField;

		bool bHasIK;
		pRestore->ReadBool( &bHasIK );
		*pIK = (bHasIK) ? new CIKContext : NULL;
	}
};

//-----------------------------------------------------------------------------
// Relative lighting entity
//-----------------------------------------------------------------------------
class CInfoLightingRelative : public CBaseEntity
{
public:
	DECLARE_CLASS( CInfoLightingRelative, CBaseEntity );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	virtual void Activate();
	virtual void SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );
	virtual int  UpdateTransmitState( void );

private:
	CNetworkHandle( CBaseEntity, m_hLightingLandmark );
	string_t		m_strLightingLandmark;
};

LINK_ENTITY_TO_CLASS( info_lighting_relative, CInfoLightingRelative );

BEGIN_DATADESC( CInfoLightingRelative )
	DEFINE_KEYFIELD( m_strLightingLandmark, FIELD_STRING, "LightingLandmark" ),
	DEFINE_FIELD( m_hLightingLandmark, FIELD_EHANDLE ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CInfoLightingRelative, DT_InfoLightingRelative)
	SendPropEHandle( SENDINFO( m_hLightingLandmark ) ),
END_SEND_TABLE()


//-----------------------------------------------------------------------------
// Activate!
//-----------------------------------------------------------------------------
void CInfoLightingRelative::Activate()
{
	BaseClass::Activate();
	if ( m_strLightingLandmark == NULL_STRING )
	{
		m_hLightingLandmark = NULL;
	}
	else
	{
		m_hLightingLandmark = gEntList.FindEntityByName( NULL, m_strLightingLandmark );
		if ( !m_hLightingLandmark )
		{
			DevWarning( "%s: Could not find lighting landmark '%s'!\n", GetClassname(), STRING( m_strLightingLandmark ) );
		}
		else
		{
			// Set a force transmit because we do not have a model.
			m_hLightingLandmark->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
		}
	}
}


//-----------------------------------------------------------------------------
// Force our lighting landmark to be transmitted
//-----------------------------------------------------------------------------
void CInfoLightingRelative::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	// Are we already marked for transmission?
	if ( pInfo->m_pTransmitEdict->Get( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo, bAlways );
	
	// Force our constraint entity to be sent too.
	if ( m_hLightingLandmark )
	{
		if ( m_hLightingLandmark->GetMoveParent() )
		{
			// Set a full check because we have a move parent.
			m_hLightingLandmark->SetTransmitState( FL_EDICT_FULLCHECK );
		}
		else
		{
			m_hLightingLandmark->SetTransmitState( FL_EDICT_ALWAYS );
		}

		m_hLightingLandmark->SetTransmit( pInfo, bAlways );
	}
}

//-----------------------------------------------------------------------------
// Purpose Force our lighting landmark to be transmitted
//-----------------------------------------------------------------------------
int CInfoLightingRelative::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

static CIKSaveRestoreOps s_IKSaveRestoreOp;


BEGIN_DATADESC( CBaseAnimating )

	DEFINE_FIELD( m_flGroundSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( m_flLastEventCheck, FIELD_TIME ),
	DEFINE_FIELD( m_bSequenceFinished, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bSequenceLoops, FIELD_BOOLEAN ),

//	DEFINE_FIELD( m_nForceBone, FIELD_INTEGER ),
//	DEFINE_FIELD( m_vecForce, FIELD_VECTOR ),

	DEFINE_INPUT( m_nSkin, FIELD_INTEGER, "skin" ),
	DEFINE_KEYFIELD( m_nBody, FIELD_INTEGER, "body" ),
	DEFINE_INPUT( m_nBody, FIELD_INTEGER, "SetBodyGroup" ),
	DEFINE_KEYFIELD( m_nHitboxSet, FIELD_INTEGER, "hitboxset" ),
	DEFINE_KEYFIELD( m_nSequence, FIELD_INTEGER, "sequence" ),
	DEFINE_ARRAY( m_flPoseParameter, FIELD_FLOAT, CBaseAnimating::NUM_POSEPAREMETERS ),
	DEFINE_ARRAY( m_flEncodedController,	FIELD_FLOAT, CBaseAnimating::NUM_BONECTRLS ),
	DEFINE_KEYFIELD( m_flPlaybackRate, FIELD_FLOAT, "playbackrate" ),
	DEFINE_KEYFIELD( m_flCycle, FIELD_FLOAT, "cycle" ),
//	DEFINE_FIELD( m_flIKGroundContactTime, FIELD_TIME ),
//	DEFINE_FIELD( m_flIKGroundMinHeight, FIELD_FLOAT ),
//	DEFINE_FIELD( m_flIKGroundMaxHeight, FIELD_FLOAT ),
//	DEFINE_FIELD( m_flEstIkFloor, FIELD_FLOAT ),
//	DEFINE_FIELD( m_flEstIkOffset, FIELD_FLOAT ),
//	DEFINE_FIELD( m_pStudioHdr, CStudioHdr ),
//	DEFINE_FIELD( m_StudioHdrInitLock, CThreadFastMutex ),
//	DEFINE_FIELD( m_BoneSetupMutex, CThreadFastMutex ),
	DEFINE_CUSTOM_FIELD( m_pIk, &s_IKSaveRestoreOp ),
	DEFINE_FIELD( m_iIKCounter, FIELD_INTEGER ),
	DEFINE_FIELD( m_bClientSideAnimation, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bClientSideFrameReset, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nNewSequenceParity, FIELD_INTEGER ),
	DEFINE_FIELD( m_nResetEventsParity, FIELD_INTEGER ),
	DEFINE_FIELD( m_nMuzzleFlashParity, FIELD_CHARACTER ),

	DEFINE_KEYFIELD( m_iszLightingOriginRelative, FIELD_STRING, "LightingOriginHack" ),
	DEFINE_KEYFIELD( m_iszLightingOrigin, FIELD_STRING, "LightingOrigin" ),

	DEFINE_FIELD( m_hLightingOrigin, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hLightingOriginRelative, FIELD_EHANDLE ),

	DEFINE_FIELD( m_flModelScale, FIELD_FLOAT ),
	DEFINE_FIELD( m_flDissolveStartTime, FIELD_TIME ),

 // DEFINE_FIELD( m_boneCacheHandle, memhandle_t ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Ignite", InputIgnite ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "IgniteLifetime", InputIgniteLifetime ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "IgniteNumHitboxFires", InputIgniteNumHitboxFires ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "IgniteHitboxFireScale", InputIgniteHitboxFireScale ),
	DEFINE_INPUTFUNC( FIELD_VOID, "BecomeRagdoll", InputBecomeRagdoll ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetLightingOriginHack", InputSetLightingOriginRelative ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetLightingOrigin", InputSetLightingOrigin ),
	DEFINE_OUTPUT( m_OnIgnite, "OnIgnite" ),

	DEFINE_INPUT( m_fadeMinDist, FIELD_FLOAT, "fademindist" ),
	DEFINE_INPUT( m_fadeMaxDist, FIELD_FLOAT, "fademaxdist" ),
	DEFINE_KEYFIELD( m_flFadeScale, FIELD_FLOAT, "fadescale" ),

	DEFINE_KEYFIELD( m_flModelScale, FIELD_FLOAT, "modelscale" ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetModelScale", InputSetModelScale ),

	DEFINE_FIELD( m_fBoneCacheFlags, FIELD_SHORT ),

	END_DATADESC()

// Sendtable for fields we don't want to send to clientside animating entities
BEGIN_SEND_TABLE_NOBASE( CBaseAnimating, DT_ServerAnimationData )
	// ANIMATION_CYCLE_BITS is defined in shareddefs.h
	SendPropFloat	(SENDINFO(m_flCycle),		ANIMATION_CYCLE_BITS, SPROP_CHANGES_OFTEN|SPROP_ROUNDDOWN,	0.0f,   1.0f)
END_SEND_TABLE()

void *SendProxy_ClientSideAnimation( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID );

// SendTable stuff.
IMPLEMENT_SERVERCLASS_ST(CBaseAnimating, DT_BaseAnimating)
	SendPropInt		( SENDINFO(m_nForceBone), 8, 0 ),
	SendPropVector	( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),

	SendPropInt		( SENDINFO(m_nSkin), ANIMATION_SKIN_BITS),
	SendPropInt		( SENDINFO(m_nBody), ANIMATION_BODY_BITS),

	SendPropInt		( SENDINFO(m_nHitboxSet),ANIMATION_HITBOXSET_BITS, SPROP_UNSIGNED ),

	SendPropFloat	( SENDINFO(m_flModelScale) ),

	SendPropArray3  ( SENDINFO_ARRAY3(m_flPoseParameter), SendPropFloat(SENDINFO_ARRAY(m_flPoseParameter), ANIMATION_POSEPARAMETER_BITS, 0, 0.0f, 1.0f ) ),
	
	SendPropInt		( SENDINFO(m_nSequence), ANIMATION_SEQUENCE_BITS, SPROP_UNSIGNED ),
	SendPropFloat	( SENDINFO(m_flPlaybackRate), ANIMATION_PLAYBACKRATE_BITS, SPROP_ROUNDUP, -4.0, 12.0f ), // NOTE: if this isn't a power of 2 than "1.0" can't be encoded correctly

	SendPropArray3 	(SENDINFO_ARRAY3(m_flEncodedController), SendPropFloat(SENDINFO_ARRAY(m_flEncodedController), 11, SPROP_ROUNDDOWN, 0.0f, 1.0f ) ),

	SendPropInt( SENDINFO( m_bClientSideAnimation ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_bClientSideFrameReset ), 1, SPROP_UNSIGNED ),

	SendPropInt( SENDINFO( m_nNewSequenceParity ), EF_PARITY_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nResetEventsParity ), EF_PARITY_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nMuzzleFlashParity ), EF_MUZZLEFLASH_BITS, SPROP_UNSIGNED ),

	SendPropEHandle( SENDINFO( m_hLightingOrigin ) ),
	SendPropEHandle( SENDINFO( m_hLightingOriginRelative ) ),

	SendPropDataTable( "serveranimdata", 0, &REFERENCE_SEND_TABLE( DT_ServerAnimationData ), SendProxy_ClientSideAnimation ),

	// Fading
	SendPropFloat( SENDINFO( m_fadeMinDist ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_fadeMaxDist ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_flFadeScale ), 0, SPROP_NOSCALE ),

END_SEND_TABLE()


CBaseAnimating::CBaseAnimating()
{
	m_vecForce.GetForModify().Init();
	m_nForceBone = 0;

	m_bResetSequenceInfoOnLoad = false;
	m_bClientSideAnimation = false;
	m_pIk = NULL;
	m_iIKCounter = 0;

	InitStepHeightAdjust();

	m_flModelScale = 1.0f;
	// initialize anim clock
	m_flAnimTime = gpGlobals->curtime;
	m_flPrevAnimTime = gpGlobals->curtime;
	m_nNewSequenceParity = 0;
	m_nResetEventsParity = 0;
	m_boneCacheHandle = 0;
	m_pStudioHdr = NULL;
	m_fadeMinDist = 0;
	m_fadeMaxDist = 0;
	m_flFadeScale = 0.0f;
	m_fBoneCacheFlags = 0;
}

CBaseAnimating::~CBaseAnimating()
{
	Studio_DestroyBoneCache( m_boneCacheHandle );
	delete m_pIk;
	UnlockStudioHdr();
	delete m_pStudioHdr;
}

void CBaseAnimating::Precache()
{
#if !defined( TF_DLL )
	// Anything derived from this class can potentially burn - true, but do we want it to!
	PrecacheParticleSystem( "burning_character" );
#endif

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Activate!
//-----------------------------------------------------------------------------
void CBaseAnimating::Activate()
{
	BaseClass::Activate();
	SetLightingOrigin( m_iszLightingOrigin );
	SetLightingOriginRelative( m_iszLightingOriginRelative );

	// Scaled physics objects (re)create their physics here
	if ( m_flModelScale != 1.0f && VPhysicsGetObject() )
	{	
		// sanity check to make sure 'm_flModelScale' is in sync with the 
		Assert( m_flModelScale > 0.0f );

		UTIL_CreateScaledPhysObject( this, m_flModelScale );
	}
}


//-----------------------------------------------------------------------------
// Force our lighting origin to be trasmitted
//-----------------------------------------------------------------------------
void CBaseAnimating::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	// Are we already marked for transmission?
	if ( pInfo->m_pTransmitEdict->Get( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo, bAlways );
	
	// Force our lighting entities to be sent too.
	if ( m_hLightingOrigin )
	{
		m_hLightingOrigin->SetTransmit( pInfo, bAlways );
	}
	if ( m_hLightingOriginRelative )
	{
		m_hLightingOriginRelative->SetTransmit( pInfo, bAlways );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CBaseAnimating::Restore( IRestore &restore )
{
	int result = BaseClass::Restore( restore );
	if ( m_flModelScale <= 0.0f )
		m_flModelScale = 1.0f;
	LockStudioHdr();
	return result;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseAnimating::OnRestore()
{
	BaseClass::OnRestore();

	if ( m_nSequence != -1 && GetModelPtr() && !IsValidSequence( m_nSequence ) )
		m_nSequence = 0;

	m_flEstIkFloor = GetLocalOrigin().z;
	PopulatePoseParameters();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseAnimating::Spawn()
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseAnimating::UseClientSideAnimation()
{
	m_bClientSideAnimation = true;
}

#define MAX_ANIMTIME_INTERVAL 0.2f

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CBaseAnimating::GetAnimTimeInterval( void ) const
{
	float flInterval;
	if (m_flAnimTime < gpGlobals->curtime)
	{
		// estimate what it'll be this frame
		flInterval = clamp( gpGlobals->curtime - m_flAnimTime, 0.f, MAX_ANIMTIME_INTERVAL );
	}
	else
	{
		// report actual
		flInterval = clamp( m_flAnimTime - m_flPrevAnimTime, 0.f, MAX_ANIMTIME_INTERVAL );
	}
	return flInterval;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimating::StudioFrameAdvanceInternal( CStudioHdr *pStudioHdr, float flCycleDelta )
{
	float flNewCycle = GetCycle() + flCycleDelta;
	if (flNewCycle < 0.0 || flNewCycle >= 1.0) 
	{
		if (m_bSequenceLoops)
		{
			flNewCycle -= (int)(flNewCycle);
		}
		else
		{
			flNewCycle = (flNewCycle < 0.0f) ? 0.0f : 1.0f;
		}
		m_bSequenceFinished = true;	// just in case it wasn't caught in GetEvents
	}
	else if (flNewCycle > GetLastVisibleCycle( pStudioHdr, GetSequence() ))
	{
		m_bSequenceFinished = true;
	}

	SetCycle( flNewCycle );

	/*
	if (!IsPlayer())
		Msg("%s %6.3f : %6.3f %6.3f (%.3f) %.3f\n", 
			GetClassname(), gpGlobals->curtime, 
			m_flAnimTime.Get(), m_flPrevAnimTime, flInterval, GetCycle() );
	*/
 
	m_flGroundSpeed = GetSequenceGroundSpeed( pStudioHdr, GetSequence() ) * GetModelScale();

	// Msg("%s : %s : %5.1f\n", GetClassname(), GetSequenceName( GetSequence() ), GetCycle() );
	InvalidatePhysicsRecursive( ANIMATION_CHANGED );

	InvalidateBoneCacheIfOlderThan( 0 );
}

void CBaseAnimating::InvalidateBoneCacheIfOlderThan( float deltaTime )
{
	CBoneCache *pcache = Studio_GetBoneCache( m_boneCacheHandle );
	if ( !pcache || !pcache->IsValid( gpGlobals->curtime, deltaTime ) )
	{
		InvalidateBoneCache();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimating::StudioFrameAdvanceManual( float flInterval )
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	if ( !pStudioHdr )
		return;

	m_flAnimTime = gpGlobals->curtime;
	m_flPrevAnimTime = m_flAnimTime - flInterval;
	float flCycleRate = GetSequenceCycleRate( pStudioHdr, GetSequence() ) * m_flPlaybackRate;
	StudioFrameAdvanceInternal( GetModelPtr(), flInterval * flCycleRate );
}


//=========================================================
// StudioFrameAdvance - advance the animation frame up some interval (default 0.1) into the future
//=========================================================
void CBaseAnimating::StudioFrameAdvance()
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	if ( !pStudioHdr || !pStudioHdr->SequencesAvailable() )
	{
		return;
	}

	if ( !m_flPrevAnimTime )
	{
		m_flPrevAnimTime = m_flAnimTime;
	}

	// Time since last animation
	float flInterval = gpGlobals->curtime - m_flAnimTime;
	flInterval = clamp( flInterval, 0.f, MAX_ANIMTIME_INTERVAL );

	//Msg( "%i %s interval %f\n", entindex(), GetClassname(), flInterval );
	if (flInterval <= 0.001f)
	{
		// Msg("%s : %s : %5.3f (skip)\n", GetClassname(), GetSequenceName( GetSequence() ), GetCycle() );
		return;
	}

	// Latch prev
	m_flPrevAnimTime = m_flAnimTime;
	// Set current
	m_flAnimTime = gpGlobals->curtime;

	// Drive cycle
	float flCycleRate = GetSequenceCycleRate( pStudioHdr, GetSequence() ) * m_flPlaybackRate;

	StudioFrameAdvanceInternal( pStudioHdr, flInterval * flCycleRate );

	if (ai_sequence_debug.GetBool() == true && m_debugOverlays & OVERLAY_NPC_SELECTED_BIT)
	{
		Msg("%5.2f : %s : %s : %5.3f\n", gpGlobals->curtime, GetClassname(), GetSequenceName( GetSequence() ), GetCycle() );
	}
}


//-----------------------------------------------------------------------------
// Set the relative lighting origin
//-----------------------------------------------------------------------------
void CBaseAnimating::SetLightingOriginRelative( string_t strLightingOriginRelative )
{
	if ( strLightingOriginRelative == NULL_STRING )
	{
		SetLightingOriginRelative( NULL );
	}
	else
	{
		CBaseEntity *pLightingOrigin = gEntList.FindEntityByName( NULL, strLightingOriginRelative );
		if ( !pLightingOrigin )
		{
			DevWarning( "%s: Could not find info_lighting_relative '%s'!\n", GetClassname(), STRING( strLightingOriginRelative ) );
			return;
		}
		else if ( !dynamic_cast<CInfoLightingRelative *>(pLightingOrigin) )
		{
			if( !pLightingOrigin )
			{
				DevWarning( "%s: Cannot find Lighting Origin named: %s\n", GetEntityName().ToCStr(), STRING(strLightingOriginRelative) );
			}
			else
			{
				DevWarning( "%s: Specified entity '%s' must be a info_lighting_relative!\n", 
					pLightingOrigin->GetClassname(), pLightingOrigin->GetEntityName().ToCStr() );
			}
			return;
		}

		SetLightingOriginRelative( pLightingOrigin );
	}

	// Save the name so that save/load will correctly restore it in Activate()
	m_iszLightingOriginRelative = strLightingOriginRelative;
}

//-----------------------------------------------------------------------------
// Set the lighting origin
//-----------------------------------------------------------------------------
void CBaseAnimating::SetLightingOrigin( string_t strLightingOrigin )
{
	if ( strLightingOrigin == NULL_STRING )
	{
		SetLightingOrigin( NULL );
	}
	else
	{
		CBaseEntity *pLightingOrigin = gEntList.FindEntityByName( NULL, strLightingOrigin );
		if ( !pLightingOrigin )
		{
			DevWarning( "%s: Could not find lighting origin entity named '%s'!\n", GetClassname(), STRING( strLightingOrigin ) );
			return;
		}
		else 
		{
			SetLightingOrigin( pLightingOrigin );
		}
	}

	// Save the name so that save/load will correctly restore it in Activate()
	m_iszLightingOrigin = strLightingOrigin;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CBaseAnimating::InputSetLightingOriginRelative( inputdata_t &inputdata )
{ 
	// Find our specified target
	string_t strLightingOriginRelative = MAKE_STRING( inputdata.value.String() );
	SetLightingOriginRelative( strLightingOriginRelative );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CBaseAnimating::InputSetLightingOrigin( inputdata_t &inputdata )
{ 
	// Find our specified target
	string_t strLightingOrigin = MAKE_STRING( inputdata.value.String() );
	SetLightingOrigin( strLightingOrigin );
}

//-----------------------------------------------------------------------------
// Purpose: SetModelScale input handler
//-----------------------------------------------------------------------------
void CBaseAnimating::InputSetModelScale( inputdata_t &inputdata )
{
	Vector vecScale;
	inputdata.value.Vector3D( vecScale );

	SetModelScale( vecScale.x, vecScale.y );
}


//=========================================================
// SelectWeightedSequence
//=========================================================
int CBaseAnimating::SelectWeightedSequence ( Activity activity )
{
	Assert( activity != ACT_INVALID );
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );
	return ::SelectWeightedSequence( GetModelPtr(), activity, GetSequence() );
}


int CBaseAnimating::SelectWeightedSequence ( Activity activity, int curSequence )
{
	Assert( activity != ACT_INVALID );
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );
	return ::SelectWeightedSequence( GetModelPtr(), activity, curSequence );
}

int CBaseAnimating::SelectWeightedSequenceFromModifiers( Activity activity, CUtlSymbol *pActivityModifiers, int iModifierCount )
{
	Assert( activity != ACT_INVALID );
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );
	return GetModelPtr()->SelectWeightedSequenceFromModifiers( activity, pActivityModifiers, iModifierCount );
}

//=========================================================
// ResetActivityIndexes
//=========================================================
void CBaseAnimating::ResetActivityIndexes ( void )
{
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );
	::ResetActivityIndexes( GetModelPtr() );
}

//=========================================================
// ResetEventIndexes
//=========================================================
void CBaseAnimating::ResetEventIndexes ( void )
{
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );
	::ResetEventIndexes( GetModelPtr() );
}

//=========================================================
// LookupHeaviestSequence
//
// Get sequence with highest 'weight' for this activity
//
//=========================================================
int CBaseAnimating::SelectHeaviestSequence ( Activity activity )
{
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );
	return ::SelectHeaviestSequence( GetModelPtr(), activity );
}


//-----------------------------------------------------------------------------
// Purpose: Looks up an activity by name.
// Input  : label - Name of the activity, ie "ACT_IDLE".
// Output : Returns the activity ID or ACT_INVALID.
//-----------------------------------------------------------------------------
int CBaseAnimating::LookupActivity( const char *label )
{
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );
	return ::LookupActivity( GetModelPtr(), label );
}

//=========================================================
//=========================================================
int CBaseAnimating::LookupSequence( const char *label )
{
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );
	return ::LookupSequence( GetModelPtr(), label );
}



//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
KeyValues *CBaseAnimating::GetSequenceKeyValues( int iSequence )
{
	const char *szText = Studio_GetKeyValueText( GetModelPtr(), iSequence );

	if (szText)
	{
		KeyValues *seqKeyValues = new KeyValues("");
		if ( seqKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), szText ) )
		{
			return seqKeyValues;
		}
		seqKeyValues->deleteThis();
	}
	return NULL;
}



//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : iSequence - 
//
// Output : float - 
//-----------------------------------------------------------------------------
float CBaseAnimating::GetSequenceMoveYaw( int iSequence )
{
	Vector				vecReturn;
	
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );
	::GetSequenceLinearMotion( GetModelPtr(), iSequence, GetPoseParameterArray(), &vecReturn );

	if (vecReturn.Length() > 0)
	{
		return UTIL_VecToYaw( vecReturn );
	}

	return NOMOTION;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : iSequence - 
//
// Output : float
//-----------------------------------------------------------------------------
float CBaseAnimating::GetSequenceMoveDist( CStudioHdr *pStudioHdr, int iSequence )
{
	Vector				vecReturn;
	
	::GetSequenceLinearMotion( pStudioHdr, iSequence, GetPoseParameterArray(), &vecReturn );

	return vecReturn.Length();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : iSequence - 
//			*pVec - 
//
//-----------------------------------------------------------------------------
void CBaseAnimating::GetSequenceLinearMotion( int iSequence, Vector *pVec )
{
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );
	::GetSequenceLinearMotion( GetModelPtr(), iSequence, GetPoseParameterArray(), pVec );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : iSequence - 
//
// Output : char
//-----------------------------------------------------------------------------
const char *CBaseAnimating::GetSequenceName( int iSequence )
{
	if( iSequence == -1 )
	{
		return "Not Found!";
	}

	if ( !GetModelPtr() )
		return "No model!";

	return ::GetSequenceName( GetModelPtr(), iSequence );
}
//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : iSequence - 
//
// Output : char
//-----------------------------------------------------------------------------
const char *CBaseAnimating::GetSequenceActivityName( int iSequence )
{
	if( iSequence == -1 )
	{
		return "Not Found!";
	}

	if ( !GetModelPtr() )
		return "No model!";

	return ::GetSequenceActivityName( GetModelPtr(), iSequence );
}

//-----------------------------------------------------------------------------
// Purpose: Make this a client-side simulated entity
// Input  : force - vector of force to be exerted in the physics simulation
//			forceBone - bone to exert force upon
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseAnimating::BecomeRagdollOnClient( const Vector &force )
{
	// If this character has a ragdoll animation, turn it over to the physics system
	if ( CanBecomeRagdoll() ) 
	{
		VPhysicsDestroyObject();
		AddSolidFlags( FSOLID_NOT_SOLID );
		m_nRenderFX = kRenderFxRagdoll;
		
		// Have to do this dance because m_vecForce is a network vector
		// and can't be sent to ClampRagdollForce as a Vector *
		Vector vecClampedForce;
		ClampRagdollForce( force, &vecClampedForce );
		m_vecForce = vecClampedForce;

		SetParent( NULL );

		AddFlag( FL_TRANSRAGDOLL );

		SetMoveType( MOVETYPE_NONE );
		//UTIL_SetSize( this, vec3_origin, vec3_origin );
		SetThink( NULL );
	
		SetNextThink( gpGlobals->curtime + 2.0f );
		//If we're here, then we can vanish safely
		SetThink( &CBaseEntity::SUB_Remove );

		// Remove our flame entity if it's attached to us
		CEntityFlame *pFireChild = dynamic_cast<CEntityFlame *>( GetEffectEntity() );
		if ( pFireChild )
		{
			pFireChild->SetThink( &CBaseEntity::SUB_Remove );
			pFireChild->SetNextThink( gpGlobals->curtime + 0.1f );
		}

		return true;
	}
	return false;
}

bool CBaseAnimating::IsRagdoll()
{
	return ( m_nRenderFX == kRenderFxRagdoll ) ? true : false;
}

bool CBaseAnimating::CanBecomeRagdoll( void ) 
{
	MDLCACHE_CRITICAL_SECTION();
	int ragdollSequence = SelectWeightedSequence( ACT_DIERAGDOLL );

	//Can't cause we don't have a ragdoll sequence.
	if ( ragdollSequence == ACTIVITY_NOT_AVAILABLE )
		 return false;
	
	if ( GetFlags() & FL_TRANSRAGDOLL )
		 return false;

	return true;
}

//=========================================================
//=========================================================
void CBaseAnimating::ResetSequenceInfo ( )
{
	if (GetSequence() == -1)
	{
		// This shouldn't happen.  Setting m_nSequence blindly is a horrible coding practice.
		SetSequence( 0 );
	}

	if ( IsDynamicModelLoading() )
	{
		m_bResetSequenceInfoOnLoad = true;
		return;
	}

	CStudioHdr *pStudioHdr = GetModelPtr();
	m_flGroundSpeed = GetSequenceGroundSpeed( pStudioHdr, GetSequence() ) * GetModelScale();
	m_bSequenceLoops = ((GetSequenceFlags( pStudioHdr, GetSequence() ) & STUDIO_LOOPING) != 0);
	// m_flAnimTime = gpGlobals->time;
	m_flPlaybackRate = 1.0;
	m_bSequenceFinished = false;
	m_flLastEventCheck = 0;

	m_nNewSequenceParity = ( m_nNewSequenceParity+1 ) & EF_PARITY_MASK;
	m_nResetEventsParity = ( m_nResetEventsParity+1 ) & EF_PARITY_MASK;

	// FIXME: why is this called here?  Nothing should have changed to make this nessesary
	if ( pStudioHdr )
	{
		SetEventIndexForSequence( pStudioHdr->pSeqdesc( GetSequence() ) );
	}
}

//=========================================================
//=========================================================
bool CBaseAnimating::IsValidSequence( int iSequence )
{
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );
	CStudioHdr* pstudiohdr = GetModelPtr( );
	if (iSequence < 0 || iSequence >= pstudiohdr->GetNumSeq())
	{
		return false;
	}
	return true;
}

//=========================================================
//=========================================================
void CBaseAnimating::SetSequence( int nSequence )
{
	Assert( nSequence == 0 || IsDynamicModelLoading() || ( GetModelPtr( ) && ( nSequence < GetModelPtr( )->GetNumSeq() ) && ( GetModelPtr( )->GetNumSeq() < (1 << ANIMATION_SEQUENCE_BITS) ) ) );
	m_nSequence = nSequence;
}

//=========================================================
//=========================================================
float CBaseAnimating::SequenceDuration( CStudioHdr *pStudioHdr, int iSequence )
{
	if ( !pStudioHdr )
	{
		DevWarning( 2, "CBaseAnimating::SequenceDuration( %d ) NULL pstudiohdr on %s!\n", iSequence, GetClassname() );
		return 0.1;
	}
	if ( !pStudioHdr->SequencesAvailable() )
	{
		return 0.1;
	}
	if (iSequence >= pStudioHdr->GetNumSeq() || iSequence < 0 )
	{
		DevWarning( 2, "CBaseAnimating::SequenceDuration( %d ) out of range\n", iSequence );
		return 0.1;
	}

	return Studio_Duration( pStudioHdr, iSequence, GetPoseParameterArray() );
}

float CBaseAnimating::GetSequenceCycleRate( CStudioHdr *pStudioHdr, int iSequence )
{
	float t = SequenceDuration( pStudioHdr, iSequence );

	if (t > 0.0f)
	{
		return 1.0f / t;
	}
	else
	{
		return 1.0f / 0.1f;
	}
}


float CBaseAnimating::GetLastVisibleCycle( CStudioHdr *pStudioHdr, int iSequence )
{
	if ( !pStudioHdr )
	{
		DevWarning( 2, "CBaseAnimating::LastVisibleCycle( %d ) NULL pstudiohdr on %s!\n", iSequence, GetClassname() );
		return 1.0;
	}

	if (!(GetSequenceFlags( pStudioHdr, iSequence ) & STUDIO_LOOPING))
	{
		return 1.0f - (pStudioHdr->pSeqdesc( iSequence ).fadeouttime) * GetSequenceCycleRate( iSequence ) * m_flPlaybackRate;
	}
	else
	{
		return 1.0;
	}
}


float CBaseAnimating::GetSequenceGroundSpeed( CStudioHdr *pStudioHdr, int iSequence )
{
	float t = SequenceDuration( pStudioHdr, iSequence );

	if (t > 0)
	{
		return ( GetSequenceMoveDist( pStudioHdr, iSequence ) / t );
	}
	else
	{
		return 0;
	}
}

float CBaseAnimating::GetIdealSpeed( ) const
{
	return m_flGroundSpeed;
}

float CBaseAnimating::GetIdealAccel( ) const
{
	// return ideal max velocity change over 1 second.
	// tuned for run-walk range of humans
	return GetIdealSpeed() + 50;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the given sequence has the anim event, false if not.
// Input  : nSequence - sequence number to check
//			nEvent - anim event number to look for
//-----------------------------------------------------------------------------
bool CBaseAnimating::HasAnimEvent( int nSequence, int nEvent )
{
	CStudioHdr *pstudiohdr = GetModelPtr();
	if ( !pstudiohdr )
	{
		return false;
	}

  	animevent_t event;

	int index = 0;
	while ( ( index = GetAnimationEvent( pstudiohdr, nSequence, &event, 0.0f, 1.0f, index ) ) != 0 )
	{
		if ( event.event == nEvent )
		{
			return true;
		}
	}

	return false;
}


//=========================================================
// DispatchAnimEvents
//=========================================================
void CBaseAnimating::DispatchAnimEvents ( CBaseAnimating *eventHandler )
{
	// don't fire events if the framerate is 0
	if (m_flPlaybackRate == 0.0)
		return;

	animevent_t	event;

	CStudioHdr *pstudiohdr = GetModelPtr( );

	if ( !pstudiohdr )
	{
		Assert(!"CBaseAnimating::DispatchAnimEvents: model missing");
		return;
	}

	if ( !pstudiohdr->SequencesAvailable() )
	{
		return;
	}

	// skip this altogether if there are no events
	if (pstudiohdr->pSeqdesc( GetSequence() ).numevents == 0)
	{
		return;
	}

	// look from when it last checked to some short time in the future	
	float flCycleRate = GetSequenceCycleRate( GetSequence() ) * m_flPlaybackRate;
	float flStart = m_flLastEventCheck;
	float flEnd = GetCycle();

	if (!m_bSequenceLoops && m_bSequenceFinished)
	{
		flEnd = 1.01f;
	}
	m_flLastEventCheck = flEnd;

	/*
	if (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT)
	{
		Msg( "%s:%s : checking %.2f %.2f (%d)\n", STRING(GetModelName()), pstudiohdr->pSeqdesc( GetSequence() ).pszLabel(), flStart, flEnd, m_bSequenceFinished );
	}
	*/

	// FIXME: does not handle negative framerates!
	int index = 0;
	while ( (index = GetAnimationEvent( pstudiohdr, GetSequence(), &event, flStart, flEnd, index ) ) != 0 )
	{
		event.pSource = this;
		// calc when this event should happen
		if (flCycleRate > 0.0)
		{
			float flCycle = event.cycle;
			if (flCycle > GetCycle())
			{
				flCycle = flCycle - 1.0;
			}
			event.eventtime = m_flAnimTime + (flCycle - GetCycle()) / flCycleRate + GetAnimTimeInterval();
		}

		/*
		if (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT)
		{
			Msg( "dispatch %i (%i) cycle %f event cycle %f cyclerate %f\n", 
				(int)(index - 1), 
				(int)event.event, 
				(float)GetCycle(), 
				(float)event.cycle, 
				(float)flCycleRate );
		}
		*/
		eventHandler->HandleAnimEvent( &event );

		// FAILSAFE:
		// If HandleAnimEvent has somehow reset my internal pointer
		// to CStudioHdr to something other than it was when we entered 
		// this function, we will crash on the next call to GetAnimationEvent
		// because pstudiohdr no longer points at something valid. 
		// So, catch this case, complain vigorously, and bail out of
		// the loop.
		CStudioHdr *pNowStudioHdr = GetModelPtr();
		if ( pNowStudioHdr != pstudiohdr )
		{
			AssertMsg2(false, "%s has changed its model while processing AnimEvents on sequence %d. Aborting dispatch.\n", GetDebugName(), GetSequence() );
			Warning( "%s has changed its model while processing AnimEvents on sequence %d. Aborting dispatch.\n", GetDebugName(), GetSequence() );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimating::HandleAnimEvent( animevent_t *pEvent )
{
	if ((pEvent->type & AE_TYPE_NEWEVENTSYSTEM) && (pEvent->type & AE_TYPE_SERVER))
	{
		if ( pEvent->event == AE_SV_PLAYSOUND )
		{
			EmitSound( pEvent->options );
			return;
		}
		else if ( pEvent->event == AE_RAGDOLL )
		{
			// Convert to ragdoll immediately
			BecomeRagdollOnClient( vec3_origin );
			return;
		}
#ifdef HL2_EPISODIC
		else if ( pEvent->event == AE_SV_DUSTTRAIL )
		{
			char szAttachment[128];
			float flDuration;
			float flSize;
			if (sscanf( pEvent->options, "%s %f %f", szAttachment, &flDuration, &flSize ) == 3)
			{
				CHandle<DustTrail>	hDustTrail;

				hDustTrail = DustTrail::CreateDustTrail();

				if( hDustTrail )
				{
					hDustTrail->m_SpawnRate = 4;    // Particles per second
					hDustTrail->m_ParticleLifetime = 1.5;   // Lifetime of each particle, In seconds
					hDustTrail->m_Color.Init(0.5f, 0.46f, 0.44f);
					hDustTrail->m_StartSize = flSize;
					hDustTrail->m_EndSize = hDustTrail->m_StartSize * 8;
					hDustTrail->m_SpawnRadius = 3;    // Each particle randomly offset from the center up to this many units
					hDustTrail->m_MinSpeed = 4;    // u/sec
					hDustTrail->m_MaxSpeed = 10;    // u/sec
					hDustTrail->m_Opacity = 0.5f;  
					hDustTrail->SetLifetime(flDuration);  // Lifetime of the spawner, in seconds
					hDustTrail->m_StopEmitTime = gpGlobals->curtime + flDuration;
					hDustTrail->SetParent( this, LookupAttachment( szAttachment ) );
					hDustTrail->SetLocalOrigin( vec3_origin );
				}
			}
			else
			{
				DevWarning( 1, "%s unable to parse AE_SV_DUSTTRAIL event \"%s\"\n", STRING( GetModelName() ), pEvent->options );
			}

			return;
		}
#endif
	}

	// Failed to find a handler
	const char *pName = EventList_NameForIndex( pEvent->event );
	if ( pName)
	{
		DevWarning( 1, "Unhandled animation event %s for %s\n", pName, GetClassname() );
	}
	else
	{
		DevWarning( 1, "Unhandled animation event %d for %s\n", pEvent->event, GetClassname() );
	}
}

// SetPoseParamater()

//=========================================================
//=========================================================
float CBaseAnimating::SetPoseParameter( CStudioHdr *pStudioHdr, const char *szName, float flValue )
{
	int poseParam = LookupPoseParameter( pStudioHdr, szName );
	AssertMsg2(poseParam >= 0, "SetPoseParameter called with invalid argument %s by %s", szName, GetDebugName());
	return SetPoseParameter( pStudioHdr, poseParam, flValue );
}

float CBaseAnimating::SetPoseParameter( CStudioHdr *pStudioHdr, int iParameter, float flValue )
{
	if ( !pStudioHdr )
	{
		return flValue;
	}

	if (iParameter >= 0)
	{
		float flNewValue;
		flValue = Studio_SetPoseParameter( pStudioHdr, iParameter, flValue, flNewValue );
		m_flPoseParameter.Set( iParameter, flNewValue );
	}

	return flValue;
}

//=========================================================
//=========================================================
float CBaseAnimating::GetPoseParameter( const char *szName )
{
	return GetPoseParameter( LookupPoseParameter( szName ) );
}

float CBaseAnimating::GetPoseParameter( int iParameter )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );

	if ( !pstudiohdr )
	{
		Assert(!"CBaseAnimating::GetPoseParameter: model missing");
		return 0.0;
	}

	if ( !pstudiohdr->SequencesAvailable() )
	{
		return 0;
	}

	if (iParameter >= 0)
	{
		return Studio_GetPoseParameter( pstudiohdr, iParameter, m_flPoseParameter[ iParameter ] );
	}

	return 0.0;
}

bool CBaseAnimating::GetPoseParameterRange( int index, float &minValue, float &maxValue )
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	if (pStudioHdr)
	{
		if (index >= 0 && index < pStudioHdr->GetNumPoseParameters())
		{
			const mstudioposeparamdesc_t &pose = pStudioHdr->pPoseParameter( index );
			minValue = pose.start;
			maxValue = pose.end;
			return true;
		}
	}
	minValue = 0.0f;
	maxValue = 1.0f;
	return false;
}

//=========================================================
//=========================================================
int CBaseAnimating::LookupPoseParameter( CStudioHdr *pStudioHdr, const char *szName )
{
	if ( !pStudioHdr )
		return 0;

	if ( !pStudioHdr->SequencesAvailable() )
	{
		return 0;
	}

	for (int i = 0; i < pStudioHdr->GetNumPoseParameters(); i++)
	{
		if (Q_stricmp( pStudioHdr->pPoseParameter( i ).pszName(), szName ) == 0)
		{
			return i;
		}
	}

	// AssertMsg( 0, UTIL_VarArgs( "poseparameter %s couldn't be mapped!!!\n", szName ) );
	return -1; // Error
}

//=========================================================
//=========================================================
bool CBaseAnimating::HasPoseParameter( int iSequence, const char *szName )
{
	int iParameter = LookupPoseParameter( szName );
	if (iParameter == -1)
	{
		return false;
	}

	return HasPoseParameter( iSequence, iParameter );
}

//=========================================================
//=========================================================
bool CBaseAnimating::HasPoseParameter( int iSequence, int iParameter )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );

	if ( !pstudiohdr )
	{
		return false;
	}

	if ( !pstudiohdr->SequencesAvailable() )
	{
		return false;
	}

	if (iSequence < 0 || iSequence >= pstudiohdr->GetNumSeq())
	{
		return false;
	}

	mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( iSequence );
	if (pstudiohdr->GetSharedPoseParameter( iSequence, seqdesc.paramindex[0] ) == iParameter || 
		pstudiohdr->GetSharedPoseParameter( iSequence, seqdesc.paramindex[1] ) == iParameter)
	{
		return true;
	}
	return false;
}


//=========================================================
// Each class that wants to use pose parameters should populate
// static variables in this entry point, rather than calling
// GetPoseParameter(const char*) every time you want to adjust
// an animation.
//
// Make sure to call BaseClass::PopulatePoseParameters() at 
// the *bottom* of your function.
//=========================================================
void	CBaseAnimating::PopulatePoseParameters( void )
{

}

//=========================================================
// Purpose: from input of 75% to 200% of maximum range, rescale smoothly from 75% to 100%
//=========================================================
float CBaseAnimating::EdgeLimitPoseParameter( int iParameter, float flValue, float flBase )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if ( !pstudiohdr )
	{
		return flValue;
	}

	if (iParameter < 0 || iParameter >= pstudiohdr->GetNumPoseParameters())
	{
		return flValue;
	}

	const mstudioposeparamdesc_t &Pose = pstudiohdr->pPoseParameter( iParameter );

	if (Pose.loop || Pose.start == Pose.end)
	{
		return flValue;
	}

	return RangeCompressor( flValue, Pose.start, Pose.end, flBase );
}


//-----------------------------------------------------------------------------
// Purpose: Returns index number of a given named bone
// Input  : name of a bone
// Output :	Bone index number or -1 if bone not found
//-----------------------------------------------------------------------------
int CBaseAnimating::LookupBone( const char *szName )
{
	const CStudioHdr *pStudioHdr = GetModelPtr();
	Assert( pStudioHdr );
	if ( !pStudioHdr )
		return -1;
	return Studio_BoneIndexByName( pStudioHdr, szName );
}


//=========================================================
//=========================================================
void CBaseAnimating::GetBonePosition ( int iBone, Vector &origin, QAngle &angles )
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
	{
		Assert(!"CBaseAnimating::GetBonePosition: model missing");
		return;
	}

	if (iBone < 0 || iBone >= pStudioHdr->numbones())
	{
		Assert(!"CBaseAnimating::GetBonePosition: invalid bone index");
		return;
	}

	matrix3x4_t bonetoworld;
	GetBoneTransform( iBone, bonetoworld );
	
	MatrixAngles( bonetoworld, angles, origin );
}



//=========================================================
//=========================================================

void CBaseAnimating::GetBoneTransform( int iBone, matrix3x4_t &pBoneToWorld )
{
	CStudioHdr *pStudioHdr = GetModelPtr( );

	if (!pStudioHdr)
	{
		Assert(!"CBaseAnimating::GetBoneTransform: model missing");
		return;
	}

	if (iBone < 0 || iBone >= pStudioHdr->numbones())
	{
		Assert(!"CBaseAnimating::GetBoneTransform: invalid bone index");
		return;
	}

	CBoneCache *pcache = GetBoneCache( );

	matrix3x4_t *pmatrix = pcache->GetCachedBone( iBone );

	if ( !pmatrix )
	{
		MatrixCopy( EntityToWorldTransform(), pBoneToWorld );
		return;
	}

	Assert( pmatrix );
	
	// FIXME
	MatrixCopy( *pmatrix, pBoneToWorld );
}

class CTraceFilterSkipNPCs : public CTraceFilterSimple
{
public:
	CTraceFilterSkipNPCs( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		if ( CTraceFilterSimple::ShouldHitEntity(pServerEntity, contentsMask) )
		{
			CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
			if ( pEntity->IsNPC() )
				return false;

			return true;
		}
		return false;
	}
};


//-----------------------------------------------------------------------------
// Purpose: Receives the clients IK floor position
//-----------------------------------------------------------------------------

void CBaseAnimating::SetIKGroundContactInfo( float minHeight, float maxHeight )
{
	m_flIKGroundContactTime = gpGlobals->curtime;
	m_flIKGroundMinHeight = minHeight;
	m_flIKGroundMaxHeight = maxHeight;
}

//-----------------------------------------------------------------------------
// Purpose: Initializes IK floor position
//-----------------------------------------------------------------------------

void CBaseAnimating::InitStepHeightAdjust( void )
{
	m_flIKGroundContactTime = 0;
	m_flIKGroundMinHeight = 0;
	m_flIKGroundMaxHeight = 0;

	// FIXME: not safe to call GetAbsOrigin here. Hierarchy might not be set up!
	m_flEstIkFloor = GetAbsOrigin().z;
	m_flEstIkOffset = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Interpolates client IK floor position and drops entity down so that the feet will reach
//-----------------------------------------------------------------------------

ConVar npc_height_adjust( "npc_height_adjust", "1", FCVAR_ARCHIVE, "Enable test mode for ik height adjustment" );

void CBaseAnimating::UpdateStepOrigin()
{
	if (!npc_height_adjust.GetBool())
	{
		m_flEstIkOffset = 0;
		m_flEstIkFloor = GetLocalOrigin().z;
		return;
	}

	/*
	if (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT)
	{
		Msg("%x : %x\n", GetMoveParent(), GetGroundEntity() );
	}
	*/

	if (m_flIKGroundContactTime > 0.2 && m_flIKGroundContactTime > gpGlobals->curtime - 0.2)
	{
		if ((GetFlags() & (FL_FLY | FL_SWIM)) == 0 && GetMoveParent() == NULL && GetGroundEntity() != NULL && !GetGroundEntity()->IsMoving())
		{
			Vector toAbs = GetAbsOrigin() - GetLocalOrigin();
			if (toAbs.z == 0.0)
			{
				CAI_BaseNPC *pNPC = MyNPCPointer();
				// FIXME:  There needs to be a default step height somewhere
				float height = 18.0f;
				if (pNPC)
				{
					height = pNPC->StepHeight();
				}

				// debounce floor location
				m_flEstIkFloor = m_flEstIkFloor * 0.2 + m_flIKGroundMinHeight * 0.8;

				// don't let heigth difference between min and max exceed step height
				float bias = clamp( (m_flIKGroundMaxHeight - m_flIKGroundMinHeight) - height, 0.f, height );
				// save off reasonable offset
				m_flEstIkOffset = clamp( m_flEstIkFloor - GetAbsOrigin().z, -height + bias, 0.0f );
				return;
			}
		}
	}

	// don't use floor offset, decay the value
	m_flEstIkOffset *= 0.5;
	m_flEstIkFloor = GetLocalOrigin().z;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the origin to use for model rendering
//-----------------------------------------------------------------------------

Vector CBaseAnimating::GetStepOrigin( void ) const 
{ 
	Vector tmp = GetLocalOrigin();
	tmp.z += m_flEstIkOffset;
	return tmp;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the origin to use for model rendering
//-----------------------------------------------------------------------------

QAngle CBaseAnimating::GetStepAngles( void ) const
{
	// TODO: Add in body lean
	return GetLocalAngles();
}

//-----------------------------------------------------------------------------
// Purpose: Find IK collisions with world
// Input  : 
// Output :	fills out m_pIk targets, calcs floor offset for rendering
//-----------------------------------------------------------------------------

void CBaseAnimating::CalculateIKLocks( float currentTime )
{
	if ( m_pIk )
	{
		Ray_t ray;
		CTraceFilterSkipNPCs traceFilter( this, GetCollisionGroup() );
		Vector up;
		GetVectors( NULL, NULL, &up );
		// FIXME: check number of slots?
		for (int i = 0; i < m_pIk->m_target.Count(); i++)
		{
			trace_t trace;
			CIKTarget *pTarget = &m_pIk->m_target[i];

			if (!pTarget->IsActive())
				continue;

			switch( pTarget->type )
			{
			case IK_GROUND:
				{
					Vector estGround;
					estGround = (pTarget->est.pos - GetAbsOrigin());
					estGround = estGround - (estGround * up) * up;
					estGround = GetAbsOrigin() + estGround + pTarget->est.floor * up;

					Vector p1, p2;
					VectorMA( estGround, pTarget->est.height, up, p1 );
					VectorMA( estGround, -pTarget->est.height, up, p2 );

					float r = MAX(pTarget->est.radius,1);

					// don't IK to other characters
					ray.Init( p1, p2, Vector(-r,-r,0), Vector(r,r,1) );
					enginetrace->TraceRay( ray, MASK_SOLID, &traceFilter, &trace );

					/*
					debugoverlay->AddBoxOverlay( p1, Vector(-r,-r,0), Vector(r,r,1), QAngle( 0, 0, 0 ), 255, 0, 0, 0, 1.0f );
					debugoverlay->AddBoxOverlay( trace.endpos, Vector(-r,-r,0), Vector(r,r,1), QAngle( 0, 0, 0 ), 255, 0, 0, 0, 1.0f );
					debugoverlay->AddLineOverlay( p1, trace.endpos, 255, 0, 0, 0, 1.0f );
					*/

					if (trace.startsolid)
					{
						ray.Init( pTarget->trace.hip, pTarget->est.pos, Vector(-r,-r,0), Vector(r,r,1) );

						enginetrace->TraceRay( ray, MASK_SOLID, &traceFilter, &trace );

						p1 = trace.endpos;
						VectorMA( p1, - pTarget->est.height, up, p2 );
						ray.Init( p1, p2, Vector(-r,-r,0), Vector(r,r,1) );

						enginetrace->TraceRay( ray, MASK_SOLID, &traceFilter, &trace );
					}

					if (!trace.startsolid)
					{
						if (trace.DidHitWorld())
						{
							pTarget->SetPosWithNormalOffset( trace.endpos, trace.plane.normal );
							pTarget->SetNormal( trace.plane.normal );
						}
						else
						{
							pTarget->SetPos( trace.endpos );
							pTarget->SetAngles( GetAbsAngles() );
						}

					}
				}
				break;
			case IK_ATTACHMENT:
				{
					// anything on the server?
				}
				break;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Clear out animation states that are invalidated with Teleport
//-----------------------------------------------------------------------------

void CBaseAnimating::Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity )
{
	BaseClass::Teleport( newPosition, newAngles, newVelocity );
	if (m_pIk)
	{
		m_pIk->ClearTargets( );
	}
	InitStepHeightAdjust();
}


//-----------------------------------------------------------------------------
// Purpose: build matrices first from the parent, then from the passed in arrays if the bone doesn't exist on the parent
//-----------------------------------------------------------------------------

void CBaseAnimating::BuildMatricesWithBoneMerge( 
	const CStudioHdr *pStudioHdr,
	const QAngle& angles, 
	const Vector& origin, 
	const Vector pos[MAXSTUDIOBONES],
	const Quaternion q[MAXSTUDIOBONES],
	matrix3x4_t bonetoworld[MAXSTUDIOBONES],
	CBaseAnimating *pParent,
	CBoneCache *pParentCache
	)
{
	CStudioHdr *fhdr = pParent->GetModelPtr();
	mstudiobone_t *pbones = pStudioHdr->pBone( 0 );

	matrix3x4_t rotationmatrix; // model to world transformation
	AngleMatrix( angles, origin, rotationmatrix);

	for ( int i=0; i < pStudioHdr->numbones(); i++ )
	{
		// Now find the bone in the parent entity.
		bool merged = false;
		int parentBoneIndex = Studio_BoneIndexByName( fhdr, pbones[i].pszName() );
		if ( parentBoneIndex >= 0 )
		{
			matrix3x4_t *pMat = pParentCache->GetCachedBone( parentBoneIndex );
			if ( pMat )
			{
				MatrixCopy( *pMat, bonetoworld[ i ] );
				merged = true;
			}
		}

		if ( !merged )
		{
			// If we get down here, then the bone wasn't merged.
			matrix3x4_t bonematrix;
			QuaternionMatrix( q[i], pos[i], bonematrix );

			if (pbones[i].parent == -1) 
			{
				ConcatTransforms (rotationmatrix, bonematrix, bonetoworld[i]);
			} 
			else 
			{
				ConcatTransforms (bonetoworld[pbones[i].parent], bonematrix, bonetoworld[i]);
			}
		}
	}
}

ConVar sv_pvsskipanimation( "sv_pvsskipanimation", "1", FCVAR_ARCHIVE, "Skips SetupBones when npc's are outside the PVS" );
ConVar ai_setupbones_debug( "ai_setupbones_debug", "0", 0, "Shows that bones that are setup every think" );




inline bool CBaseAnimating::CanSkipAnimation( void )
{
	if ( !sv_pvsskipanimation.GetBool() )
		return false;
		
	CAI_BaseNPC *pNPC = MyNPCPointer();
	if ( pNPC && !pNPC->HasCondition( COND_IN_PVS ) && ( m_fBoneCacheFlags & (BCF_NO_ANIMATION_SKIP | BCF_IS_IN_SPAWN) ) == false )
	{
		// If we have a player as a child, then we better setup our bones. If we don't,
		// the PVS will be screwy. 
		return !DoesHavePlayerChild();
	}
	else
	{	
		return false;
	}
}


void CBaseAnimating::SetupBones( matrix3x4_t *pBoneToWorld, int boneMask )
{
	AUTO_LOCK( m_BoneSetupMutex );
	
	VPROF_BUDGET( "CBaseAnimating::SetupBones", VPROF_BUDGETGROUP_SERVER_ANIM );
	
	MDLCACHE_CRITICAL_SECTION();

	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );

	CStudioHdr *pStudioHdr = GetModelPtr( );

	if(!pStudioHdr)
	{
		Assert(!"CBaseAnimating::GetSkeleton() without a model");
		return;
	}

	Assert( !IsEFlagSet( EFL_SETTING_UP_BONES ) );

	AddEFlags( EFL_SETTING_UP_BONES );

	Vector pos[MAXSTUDIOBONES];
	Quaternion q[MAXSTUDIOBONES];

	// adjust hit boxes based on IK driven offset
	Vector adjOrigin = GetAbsOrigin() + Vector( 0, 0, m_flEstIkOffset );

	if ( CanSkipAnimation() )
	{
		IBoneSetup boneSetup( pStudioHdr, boneMask, GetPoseParameterArray() );
		boneSetup.InitPose( pos, q );
		// Msg( "%.03f : %s:%s not in pvs\n", gpGlobals->curtime, GetClassname(), GetEntityName().ToCStr() );
	}
	else 
	{
		if ( m_pIk )
		{
			// FIXME: pass this into Studio_BuildMatrices to skip transforms
			CBoneBitList boneComputed;
			m_iIKCounter++;
			m_pIk->Init( pStudioHdr, GetAbsAngles(), adjOrigin, gpGlobals->curtime, m_iIKCounter, boneMask );
			GetSkeleton( pStudioHdr, pos, q, boneMask );

			m_pIk->UpdateTargets( pos, q, pBoneToWorld, boneComputed );
			CalculateIKLocks( gpGlobals->curtime );
			m_pIk->SolveDependencies( pos, q, pBoneToWorld, boneComputed );
		}
		else
		{
			// Msg( "%.03f : %s:%s\n", gpGlobals->curtime, GetClassname(), GetEntityName().ToCStr() );
			GetSkeleton( pStudioHdr, pos, q, boneMask );
		}
	}
	
	CBaseAnimating *pParent = dynamic_cast< CBaseAnimating* >( GetMoveParent() );
	if ( pParent )
	{
		// We're doing bone merging, so do special stuff here.
		CBoneCache *pParentCache = pParent->GetBoneCache();
		if ( pParentCache )
		{
			BuildMatricesWithBoneMerge( 
				pStudioHdr, 
				GetAbsAngles(), 
				adjOrigin, 
				pos, 
				q, 
				pBoneToWorld, 
				pParent, 
				pParentCache );
			
			RemoveEFlags( EFL_SETTING_UP_BONES );
			if (ai_setupbones_debug.GetBool())
			{
				DrawRawSkeleton( pBoneToWorld, boneMask, true, 0.11 );
			}
			return;
		}
	}

	Studio_BuildMatrices( 
		pStudioHdr, 
		GetAbsAngles(), 
		adjOrigin, 
		pos, 
		q, 
		-1,
		GetModelScale(), // Scaling
		pBoneToWorld,
		boneMask );

	if (ai_setupbones_debug.GetBool())
	{
		// Msg("%s:%s:%s (%x)\n", GetClassname(), GetDebugName(), STRING(GetModelName()), boneMask );
		DrawRawSkeleton( pBoneToWorld, boneMask, true, 0.11 );
	}
	RemoveEFlags( EFL_SETTING_UP_BONES );
}

//=========================================================
//=========================================================
int CBaseAnimating::GetNumBones ( void )
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if(pStudioHdr)
	{
		return pStudioHdr->numbones();
	}
	else
	{
		Assert(!"CBaseAnimating::GetNumBones: model missing");
		return 0;
	}
}


//=========================================================
//=========================================================

//-----------------------------------------------------------------------------
// Purpose: Returns index number of a given named attachment
// Input  : name of attachment
// Output :	attachment index number or -1 if attachment not found
//-----------------------------------------------------------------------------
int CBaseAnimating::LookupAttachment( const char *szName )
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
	{
		Assert(!"CBaseAnimating::LookupAttachment: model missing");
		return 0;
	}

	// The +1 is to make attachment indices be 1-based (namely 0 == invalid or unused attachment)
	return Studio_FindAttachment( pStudioHdr, szName ) + 1;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the world location and world angles of an attachment
// Input  : attachment name
// Output :	location and angles
//-----------------------------------------------------------------------------
bool CBaseAnimating::GetAttachment( const char *szName, Vector &absOrigin, QAngle &absAngles )
{																
	return GetAttachment( LookupAttachment( szName ), absOrigin, absAngles );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the world location and world angles of an attachment
// Input  : attachment index
// Output :	location and angles
//-----------------------------------------------------------------------------
bool CBaseAnimating::GetAttachment ( int iAttachment, Vector &absOrigin, QAngle &absAngles )
{
	matrix3x4_t attachmentToWorld;

	bool bRet = GetAttachment( iAttachment, attachmentToWorld );
	MatrixAngles( attachmentToWorld, absAngles, absOrigin );
	return bRet;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the world location and world angles of an attachment
// Input  : attachment index
// Output :	location and angles
//-----------------------------------------------------------------------------
bool CBaseAnimating::GetAttachment( int iAttachment, matrix3x4_t &attachmentToWorld )
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
	{
		MatrixCopy(EntityToWorldTransform(), attachmentToWorld);
		AssertOnce(!"CBaseAnimating::GetAttachment: model missing");
		return false;
	}

	if (iAttachment < 1 || iAttachment > pStudioHdr->GetNumAttachments())
	{
		MatrixCopy(EntityToWorldTransform(), attachmentToWorld);
//		Assert(!"CBaseAnimating::GetAttachment: invalid attachment index");
		return false;
	}

	const mstudioattachment_t &pattachment = pStudioHdr->pAttachment( iAttachment-1 );
	int iBone = pStudioHdr->GetAttachmentBone( iAttachment-1 );

	matrix3x4_t bonetoworld;
	GetBoneTransform( iBone, bonetoworld );
	if ( (pattachment.flags & ATTACHMENT_FLAG_WORLD_ALIGN) == 0 )
	{
		ConcatTransforms( bonetoworld, pattachment.local, attachmentToWorld ); 
	}
	else
	{
		Vector vecLocalBonePos, vecWorldBonePos;
		MatrixGetColumn( pattachment.local, 3, vecLocalBonePos );
		VectorTransform( vecLocalBonePos, bonetoworld, vecWorldBonePos );

		SetIdentityMatrix( attachmentToWorld );
		MatrixSetColumn( vecWorldBonePos, 3, attachmentToWorld );
	}

	return true;
}

// gets the bone for an attachment
int CBaseAnimating::GetAttachmentBone( int iAttachment )
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr || iAttachment < 1 || iAttachment > pStudioHdr->GetNumAttachments() )
	{
		AssertOnce(pStudioHdr && "CBaseAnimating::GetAttachment: model missing");
		return 0;
	}

	return pStudioHdr->GetAttachmentBone( iAttachment-1 );
}


//-----------------------------------------------------------------------------
// Purpose: Returns the world location of an attachment
// Input  : attachment index
// Output :	location and angles
//-----------------------------------------------------------------------------
bool CBaseAnimating::GetAttachment( const char *szName, Vector &absOrigin, Vector *forward, Vector *right, Vector *up )
{																
	return GetAttachment( LookupAttachment( szName ), absOrigin, forward, right, up );
}

bool CBaseAnimating::GetAttachment( int iAttachment, Vector &absOrigin, Vector *forward, Vector *right, Vector *up )
{
	matrix3x4_t attachmentToWorld;

	bool bRet = GetAttachment( iAttachment, attachmentToWorld );
	MatrixPosition( attachmentToWorld, absOrigin );
	if (forward)
	{
		MatrixGetColumn( attachmentToWorld, 0, forward );
	}
	if (right)
	{
		MatrixGetColumn( attachmentToWorld, 1, right );
	}
	if (up)
	{
		MatrixGetColumn( attachmentToWorld, 2, up );
	}
	return bRet;
}


//-----------------------------------------------------------------------------
// Returns the attachment in local space
//-----------------------------------------------------------------------------
bool CBaseAnimating::GetAttachmentLocal( const char *szName, Vector &origin, QAngle &angles )
{
	return GetAttachmentLocal( LookupAttachment( szName ), origin, angles );
}

bool CBaseAnimating::GetAttachmentLocal( int iAttachment, Vector &origin, QAngle &angles )
{
	matrix3x4_t attachmentToEntity;

	bool bRet = GetAttachmentLocal( iAttachment, attachmentToEntity );
	MatrixAngles( attachmentToEntity, angles, origin );
	return bRet;
}

bool CBaseAnimating::GetAttachmentLocal( int iAttachment, matrix3x4_t &attachmentToLocal )
{
	matrix3x4_t attachmentToWorld;
	bool bRet = GetAttachment(iAttachment, attachmentToWorld);
	matrix3x4_t worldToEntity;
	MatrixInvert( EntityToWorldTransform(), worldToEntity );
	ConcatTransforms( worldToEntity, attachmentToWorld, attachmentToLocal ); 
	return bRet;
}


//=========================================================
//=========================================================
void CBaseAnimating::GetEyeballs( Vector &origin, QAngle &angles )
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
	{
		Assert(!"CBaseAnimating::GetAttachment: model missing");
		return;
	}

	for (int iBodypart = 0; iBodypart < pStudioHdr->numbodyparts(); iBodypart++)
	{
		mstudiobodyparts_t *pBodypart = pStudioHdr->pBodypart( iBodypart );
		for (int iModel = 0; iModel < pBodypart->nummodels; iModel++)
		{
			mstudiomodel_t *pModel = pBodypart->pModel( iModel );
			for (int iEyeball = 0; iEyeball < pModel->numeyeballs; iEyeball++)
			{
				mstudioeyeball_t *pEyeball = pModel->pEyeball( iEyeball );
				matrix3x4_t bonetoworld;
				GetBoneTransform( pEyeball->bone, bonetoworld );
				VectorTransform( pEyeball->org, bonetoworld,  origin );
				MatrixAngles( bonetoworld, angles ); // ???
			}
		}
	}
}


//=========================================================
//=========================================================
int CBaseAnimating::FindTransitionSequence( int iCurrentSequence, int iGoalSequence, int *piDir )
{
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );

	if (piDir == NULL)
	{
		int iDir = 1;
		int sequence = ::FindTransitionSequence( GetModelPtr(), iCurrentSequence, iGoalSequence, &iDir );
		if (iDir != 1)
			return -1;
		else
			return sequence;
	}

	return ::FindTransitionSequence( GetModelPtr(), iCurrentSequence, iGoalSequence, piDir );
}


bool CBaseAnimating::GotoSequence( int iCurrentSequence, float flCurrentCycle, float flCurrentRate, int iGoalSequence, int &nNextSequence, float &flNextCycle, int &iNextDir )
{
	return ::GotoSequence( GetModelPtr(), iCurrentSequence, flCurrentCycle, flCurrentRate, iGoalSequence, nNextSequence, flNextCycle, iNextDir );
}


int CBaseAnimating::GetEntryNode( int iSequence )
{
	CStudioHdr *pstudiohdr = GetModelPtr();
	if (! pstudiohdr)
		return 0;

	return pstudiohdr->EntryNode( iSequence );
}


int CBaseAnimating::GetExitNode( int iSequence )
{
	CStudioHdr *pstudiohdr = GetModelPtr();
	if (! pstudiohdr)
		return 0;
	
	return pstudiohdr->ExitNode( iSequence );
}


//=========================================================
//=========================================================

void CBaseAnimating::SetBodygroup( int iGroup, int iValue )
{
	// SetBodygroup is not supported on pending dynamic models. Wait for it to load!
	// XXX TODO we could buffer up the group and value if we really needed to. -henryg
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );
	int newBody = m_nBody;
	::SetBodygroup( GetModelPtr( ), newBody, iGroup, iValue );
	m_nBody = newBody;
}

int CBaseAnimating::GetBodygroup( int iGroup )
{
	Assert( IsDynamicModelLoading() || GetModelPtr() );
	return IsDynamicModelLoading() ? 0 : ::GetBodygroup( GetModelPtr( ), m_nBody, iGroup );
}

const char *CBaseAnimating::GetBodygroupName( int iGroup )
{
	Assert( IsDynamicModelLoading() || GetModelPtr() );
	return IsDynamicModelLoading() ? "" : ::GetBodygroupName( GetModelPtr( ), iGroup );
}

int CBaseAnimating::FindBodygroupByName( const char *name )
{
	Assert( IsDynamicModelLoading() || GetModelPtr() );
	return IsDynamicModelLoading() ? -1 : ::FindBodygroupByName( GetModelPtr( ), name );
}

int CBaseAnimating::GetBodygroupCount( int iGroup )
{
	Assert( IsDynamicModelLoading() || GetModelPtr() );
	return IsDynamicModelLoading() ? 0 : ::GetBodygroupCount( GetModelPtr( ), iGroup );
}

int CBaseAnimating::GetNumBodyGroups( void )
{
	Assert( IsDynamicModelLoading() || GetModelPtr() );
	return IsDynamicModelLoading() ? 0 : ::GetNumBodyGroups( GetModelPtr( ) );
}

int CBaseAnimating::ExtractBbox( int sequence, Vector& mins, Vector& maxs )
{
	Assert( IsDynamicModelLoading() || GetModelPtr() );
	return IsDynamicModelLoading() ? 0 : ::ExtractBbox( GetModelPtr( ), sequence, mins, maxs );
}

//=========================================================
//=========================================================

void CBaseAnimating::SetSequenceBox( void )
{
	Vector mins, maxs;

	// Get sequence bbox
	if ( ExtractBbox( GetSequence(), mins, maxs ) )
	{
		// expand box for rotation
		// find min / max for rotations
		float yaw = GetLocalAngles().y * (M_PI / 180.0);
		
		Vector xvector, yvector;
		xvector.x = cos(yaw);
		xvector.y = sin(yaw);
		yvector.x = -sin(yaw);
		yvector.y = cos(yaw);
		Vector bounds[2];

		bounds[0] = mins;
		bounds[1] = maxs;
		
		Vector rmin( 9999, 9999, 9999 );
		Vector rmax( -9999, -9999, -9999 );
		Vector base, transformed;

		for (int i = 0; i <= 1; i++ )
		{
			base.x = bounds[i].x;
			for ( int j = 0; j <= 1; j++ )
			{
				base.y = bounds[j].y;
				for ( int k = 0; k <= 1; k++ )
				{
					base.z = bounds[k].z;
					
				// transform the point
					transformed.x = xvector.x*base.x + yvector.x*base.y;
					transformed.y = xvector.y*base.x + yvector.y*base.y;
					transformed.z = base.z;
					
					for ( int l = 0; l < 3; l++ )
					{
						if (transformed[l] < rmin[l])
							rmin[l] = transformed[l];
						if (transformed[l] > rmax[l])
							rmax[l] = transformed[l];
					}
				}
			}
		}
		rmin.z = 0;
		rmax.z = rmin.z + 1;
		UTIL_SetSize( this, rmin, rmax );
	}
}

//=========================================================
//=========================================================
int CBaseAnimating::RegisterPrivateActivity( const char *pszActivityName )
{
	return ActivityList_RegisterPrivateActivity( pszActivityName );
}

//-----------------------------------------------------------------------------
// Purpose: Notifies the console that this entity could not retrieve an
//			animation sequence for the specified activity. This probably means
//			there's a typo in the model QC file, or the sequence is missing
//			entirely.
//			
//
// Input  : iActivity - The activity that failed to resolve to a sequence.
//
//
// NOTE   :	IMPORTANT - Something needs to be done so that private activities
//			(which are allowed to collide in the activity list) remember each
//			entity that registered an activity there, and the activity name
//			each character registered.
//-----------------------------------------------------------------------------
void CBaseAnimating::ReportMissingActivity( int iActivity )
{
	Msg( "%s has no sequence for act:%s\n", GetClassname(), ActivityList_NameForIndex(iActivity) );
}


LocalFlexController_t CBaseAnimating::GetNumFlexControllers( void )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return LocalFlexController_t(0);

	return pstudiohdr->numflexcontrollers();
}


const char *CBaseAnimating::GetFlexDescFacs( int iFlexDesc )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return 0;

	mstudioflexdesc_t *pflexdesc = pstudiohdr->pFlexdesc( iFlexDesc );

	return pflexdesc->pszFACS( );
}

const char *CBaseAnimating::GetFlexControllerName( LocalFlexController_t iFlexController )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return 0;

	mstudioflexcontroller_t *pflexcontroller = pstudiohdr->pFlexcontroller( iFlexController );

	return pflexcontroller->pszName( );
}

const char *CBaseAnimating::GetFlexControllerType( LocalFlexController_t iFlexController )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return 0;

	mstudioflexcontroller_t *pflexcontroller = pstudiohdr->pFlexcontroller( iFlexController );

	return pflexcontroller->pszType( );
}

//-----------------------------------------------------------------------------
// Purpose: Converts the ground speed of the animating entity into a true velocity
// Output : Vector - velocity of the character at its current m_flGroundSpeed
//-----------------------------------------------------------------------------
Vector CBaseAnimating::GetGroundSpeedVelocity( void )
{
	CStudioHdr *pstudiohdr = GetModelPtr();
	if (!pstudiohdr)
		return vec3_origin;

	QAngle  vecAngles;
	Vector	vecVelocity;

	vecAngles.y = GetSequenceMoveYaw( GetSequence() );
	vecAngles.x = 0;
	vecAngles.z = 0;

	vecAngles.y += GetLocalAngles().y;

	AngleVectors( vecAngles, &vecVelocity );

	vecVelocity = vecVelocity * m_flGroundSpeed;

	return vecVelocity;
}


//-----------------------------------------------------------------------------
// Purpose:
// Output :
//-----------------------------------------------------------------------------
float CBaseAnimating::GetInstantaneousVelocity( float flInterval )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return 0;

	// FIXME: someone needs to check for last frame, etc.
	float flNextCycle = GetCycle() + flInterval * GetSequenceCycleRate( GetSequence() ) * m_flPlaybackRate;

	Vector vecVelocity;
	Studio_SeqVelocity( pstudiohdr, GetSequence(), flNextCycle, GetPoseParameterArray(), vecVelocity );
	vecVelocity *= m_flPlaybackRate;

	return vecVelocity.Length();
}



//-----------------------------------------------------------------------------
// Purpose:
// Output :
//-----------------------------------------------------------------------------
float CBaseAnimating::GetEntryVelocity( int iSequence )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return 0;

	Vector vecVelocity;
	Studio_SeqVelocity( pstudiohdr, iSequence, 0.0, GetPoseParameterArray(), vecVelocity );

	return vecVelocity.Length();
}

float CBaseAnimating::GetExitVelocity( int iSequence )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return 0;

	Vector vecVelocity;
	Studio_SeqVelocity( pstudiohdr, iSequence, 1.0, GetPoseParameterArray(), vecVelocity );

	return vecVelocity.Length();
}

//-----------------------------------------------------------------------------
// Purpose:
// Output :
//-----------------------------------------------------------------------------
bool CBaseAnimating::GetIntervalMovement( float flIntervalUsed, bool &bMoveSeqFinished, Vector &newPosition, QAngle &newAngles )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr || !pstudiohdr->SequencesAvailable())
		return false;

	float flComputedCycleRate = GetSequenceCycleRate( GetSequence() );
	
	float flNextCycle = GetCycle() + flIntervalUsed * flComputedCycleRate * m_flPlaybackRate;

	if ((!m_bSequenceLoops) && flNextCycle > 1.0)
	{
		flIntervalUsed = GetCycle() / (flComputedCycleRate * m_flPlaybackRate);
		flNextCycle = 1.0;
		bMoveSeqFinished = true;
	}
	else
	{
		bMoveSeqFinished = false;
	}

	Vector deltaPos;
	QAngle deltaAngles;

	if (Studio_SeqMovement( pstudiohdr, GetSequence(), GetCycle(), flNextCycle, GetPoseParameterArray(), deltaPos, deltaAngles ))
	{
		VectorYawRotate( deltaPos, GetLocalAngles().y, deltaPos );
		newPosition = GetLocalOrigin() + deltaPos;
		newAngles.Init();
		newAngles.y = GetLocalAngles().y + deltaAngles.y;
		return true;
	}
	else
	{
		newPosition = GetLocalOrigin();
		newAngles = GetLocalAngles();
		return false;
	}
}



//-----------------------------------------------------------------------------
// Purpose:
// Output :
//-----------------------------------------------------------------------------
bool CBaseAnimating::GetSequenceMovement( int nSequence, float fromCycle, float toCycle, Vector &deltaPosition, QAngle &deltaAngles )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return false;

	return Studio_SeqMovement( pstudiohdr, nSequence, fromCycle, toCycle, GetPoseParameterArray(), deltaPosition, deltaAngles );
}


//-----------------------------------------------------------------------------
// Purpose: find frame where they animation has moved a given distance.
// Output :
//-----------------------------------------------------------------------------
float CBaseAnimating::GetMovementFrame( float flDist )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return 0;

	float t = Studio_FindSeqDistance( pstudiohdr, GetSequence(), GetPoseParameterArray(), flDist );

	return t;
}


//-----------------------------------------------------------------------------
// Purpose: does a specific sequence have movement?
// Output :
//-----------------------------------------------------------------------------
bool CBaseAnimating::HasMovement( int iSequence )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return false;

	// FIXME: this needs to check to see if there are keys, and the object is walking
	Vector deltaPos;
	QAngle deltaAngles;
	if (Studio_SeqMovement( pstudiohdr, iSequence, 0.0f, 1.0f, GetPoseParameterArray(), deltaPos, deltaAngles ))
	{
		return true;
	}

	return false;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szModelName - 
//-----------------------------------------------------------------------------
void CBaseAnimating::SetModel( const char *szModelName )
{
	MDLCACHE_CRITICAL_SECTION();

	// delete exiting studio model container
	UnlockStudioHdr();
	delete m_pStudioHdr;
	m_pStudioHdr = NULL;
	
	if ( szModelName[0] )
	{
		int modelIndex = modelinfo->GetModelIndex( szModelName );
		const model_t *model = modelinfo->GetModel( modelIndex );
		if ( model && ( modelinfo->GetModelType( model ) != mod_studio ) )
		{
			Msg( "Setting CBaseAnimating to non-studio model %s  (type:%i)\n",	szModelName, modelinfo->GetModelType( model ) );
		}
	}

	if ( m_boneCacheHandle )
	{
		Studio_DestroyBoneCache( m_boneCacheHandle );
		m_boneCacheHandle = 0;
	}

	UTIL_SetModel( this, szModelName );

	InitBoneControllers( );
	SetSequence( 0 );
	
	PopulatePoseParameters();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CBaseAnimating::LockStudioHdr()
{
	AUTO_LOCK( m_StudioHdrInitLock );
	const model_t *mdl = GetModel();
	if (mdl)
	{
		MDLHandle_t hStudioHdr = modelinfo->GetCacheHandle( mdl );
		if ( hStudioHdr != MDLHANDLE_INVALID )
		{
			const studiohdr_t *pStudioHdr = mdlcache->LockStudioHdr( hStudioHdr );
			CStudioHdr *pStudioHdrContainer = NULL;
			if ( !m_pStudioHdr )
			{
				if ( pStudioHdr )
				{
					pStudioHdrContainer = new CStudioHdr;
					pStudioHdrContainer->Init( pStudioHdr, mdlcache );
				}
			}
			else
			{
				pStudioHdrContainer = m_pStudioHdr;
			}

			Assert( ( pStudioHdr == NULL && pStudioHdrContainer == NULL ) || pStudioHdrContainer->GetRenderHdr() == pStudioHdr );

			if ( pStudioHdrContainer && pStudioHdrContainer->GetVirtualModel() )
			{
				MDLHandle_t hVirtualModel = (MDLHandle_t)(int)(pStudioHdrContainer->GetRenderHdr()->virtualModel)&0xffff;
				mdlcache->LockStudioHdr( hVirtualModel );
			}
			m_pStudioHdr = pStudioHdrContainer; // must be last to ensure virtual model correctly set up
		}
	}
}

void CBaseAnimating::UnlockStudioHdr()
{
	if ( m_pStudioHdr )
	{
		const model_t *mdl = GetModel();
		if (mdl)
		{
			mdlcache->UnlockStudioHdr( modelinfo->GetCacheHandle( mdl ) );
			if ( m_pStudioHdr->GetVirtualModel() )
			{
				MDLHandle_t hVirtualModel = (MDLHandle_t)(int)(m_pStudioHdr->GetRenderHdr()->virtualModel)&0xffff;
				mdlcache->UnlockStudioHdr( hVirtualModel );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: return the index to the shared bone cache
// Output :
//-----------------------------------------------------------------------------
CBoneCache *CBaseAnimating::GetBoneCache( void )
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	Assert(pStudioHdr);

	CBoneCache *pcache = Studio_GetBoneCache( m_boneCacheHandle );
	int boneMask = BONE_USED_BY_HITBOX | BONE_USED_BY_ATTACHMENT;

	// TF queries these bones to position weapons when players are killed
#if defined( TF_DLL )
	boneMask |= BONE_USED_BY_BONE_MERGE;
#endif
	if ( pcache )
	{
		if ( pcache->IsValid( gpGlobals->curtime ) && (pcache->m_boneMask & boneMask) == boneMask && pcache->m_timeValid <= gpGlobals->curtime)
		{
			// Msg("%s:%s:%s (%x:%x:%8.4f) cache\n", GetClassname(), GetDebugName(), STRING(GetModelName()), boneMask, pcache->m_boneMask, pcache->m_timeValid );
			// in memory and still valid, use it!
			return pcache;
		}
		// in memory, but missing some of the bone masks
		if ( (pcache->m_boneMask & boneMask) != boneMask )
		{
			Studio_DestroyBoneCache( m_boneCacheHandle );
			m_boneCacheHandle = 0;
			pcache = NULL;
		}
	}

	matrix3x4_t bonetoworld[MAXSTUDIOBONES];
	SetupBones( bonetoworld, boneMask );

	if ( pcache )
	{
		// still in memory but out of date, refresh the bones.
		pcache->UpdateBones( bonetoworld, pStudioHdr->numbones(), gpGlobals->curtime );
	}
	else
	{
		bonecacheparams_t params;
		params.pStudioHdr = pStudioHdr;
		params.pBoneToWorld = bonetoworld;
		params.curtime = gpGlobals->curtime;
		params.boneMask = boneMask;

		m_boneCacheHandle = Studio_CreateBoneCache( params );
		pcache = Studio_GetBoneCache( m_boneCacheHandle );
	}
	Assert(pcache);
	return pcache;
}


void CBaseAnimating::InvalidateBoneCache( void )
{
	Studio_InvalidateBoneCache( m_boneCacheHandle );
}

bool CBaseAnimating::TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	// Return a special case for scaled physics objects
	if ( GetModelScale() != 1.0f )
	{
		IPhysicsObject *pPhysObject = VPhysicsGetObject();
		Vector vecPosition;
		QAngle vecAngles;
		pPhysObject->GetPosition( &vecPosition, &vecAngles );
		const CPhysCollide *pScaledCollide = pPhysObject->GetCollide();
		physcollision->TraceBox( ray, pScaledCollide, vecPosition, vecAngles, &tr );
		
		return tr.DidHit();
	}

	if ( IsSolidFlagSet( FSOLID_CUSTOMRAYTEST ))
	{
		if (!TestHitboxes( ray, fContentsMask, tr ))
			return true;

		return tr.DidHit();
	}

	// We shouldn't get here.
	Assert(0);
	return false;
}

bool CBaseAnimating::TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
	{
		Assert(!"CBaseAnimating::GetBonePosition: model missing");
		return false;
	}

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( m_nHitboxSet );
	if ( !set || !set->numhitboxes )
		return false;

	CBoneCache *pcache = GetBoneCache( );

	matrix3x4_t *hitboxbones[MAXSTUDIOBONES];
	pcache->ReadCachedBonePointers( hitboxbones, pStudioHdr->numbones() );

	if ( TraceToStudio( physprops, ray, pStudioHdr, set, hitboxbones, fContentsMask, GetAbsOrigin(), GetModelScale(), tr ) )
	{
		mstudiobbox_t *pbox = set->pHitbox( tr.hitbox );
		mstudiobone_t *pBone = pStudioHdr->pBone(pbox->bone);
		tr.surface.name = "**studio**";
		tr.surface.flags = SURF_HITBOX;
		tr.surface.surfaceProps = physprops->GetSurfaceIndex( pBone->pszSurfaceProp() );
	}
	return true;
}

void CBaseAnimating::InitBoneControllers ( void ) // FIXME: rename
{
	int i;

	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
		return;

	int nBoneControllerCount = pStudioHdr->numbonecontrollers();	
	if ( nBoneControllerCount > NUM_BONECTRLS )
	{
		nBoneControllerCount = NUM_BONECTRLS;

#ifdef _DEBUG
		Warning( "Model %s has too many bone controllers! (Max %d allowed)\n", pStudioHdr->pszName(), NUM_BONECTRLS );
#endif
	}

	for (i = 0; i < nBoneControllerCount; i++)
	{
		SetBoneController( i, 0.0 );
	}

	Assert( pStudioHdr->SequencesAvailable() );

	if ( pStudioHdr->SequencesAvailable() )
	{
		for (i = 0; i < pStudioHdr->GetNumPoseParameters(); i++)
		{
			SetPoseParameter( i, 0.0 );
		}
	}
}

//=========================================================
//=========================================================
float CBaseAnimating::SetBoneController ( int iController, float flValue )
{
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );

	CStudioHdr *pmodel = (CStudioHdr*)GetModelPtr();

	Assert(iController >= 0 && iController < NUM_BONECTRLS);

	float newValue;
	float retVal = Studio_SetController( pmodel, iController, flValue, newValue );
	m_flEncodedController.Set( iController, newValue );

	return retVal;
}

//=========================================================
//=========================================================
float CBaseAnimating::GetBoneController ( int iController )
{
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );

	CStudioHdr *pmodel = (CStudioHdr*)GetModelPtr();

	return Studio_GetController( pmodel, iController, m_flEncodedController[iController] );
}

//------------------------------------------------------------------------------
// Purpose : Returns velcocity of the NPC from it's animation.  
//			 If physically simulated gets velocity from physics object
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseAnimating::GetVelocity(Vector *vVelocity, AngularImpulse *vAngVelocity) 
{
	if ( GetMoveType() == MOVETYPE_VPHYSICS )
	{
		BaseClass::GetVelocity(vVelocity,vAngVelocity);
	}
	else if ( !(GetFlags() & FL_ONGROUND) )
	{
		BaseClass::GetVelocity(vVelocity,vAngVelocity);
	}
	else
	{
		if (vVelocity != NULL)
		{
			Vector	vRawVel;

			GetSequenceLinearMotion( GetSequence(), &vRawVel );

			// Build a rotation matrix from NPC orientation
			matrix3x4_t fRotateMatrix;
			AngleMatrix(GetLocalAngles(), fRotateMatrix);
			VectorRotate( vRawVel, fRotateMatrix, *vVelocity);
		}
		if (vAngVelocity != NULL)
		{
			QAngle tmp = GetLocalAngularVelocity();
			QAngleToAngularImpulse( tmp, *vAngVelocity );
		}
	}
}


//=========================================================
//=========================================================

void CBaseAnimating::GetSkeleton( CStudioHdr *pStudioHdr, Vector pos[], Quaternion q[], int boneMask )
{
	if(!pStudioHdr)
	{
		Assert(!"CBaseAnimating::GetSkeleton() without a model");
		return;
	}

	IBoneSetup boneSetup( pStudioHdr, boneMask, GetPoseParameterArray() );
	boneSetup.InitPose( pos, q );

	boneSetup.AccumulatePose( pos, q, GetSequence(), GetCycle(), 1.0, gpGlobals->curtime, m_pIk );

	if ( m_pIk )
	{
		CIKContext auto_ik;
		auto_ik.Init( pStudioHdr, GetAbsAngles(), GetAbsOrigin(), gpGlobals->curtime, 0, boneMask );
		boneSetup.CalcAutoplaySequences( pos, q, gpGlobals->curtime, &auto_ik );
	}
	else
	{
		boneSetup.CalcAutoplaySequences( pos, q, gpGlobals->curtime, NULL );
	}
	boneSetup.CalcBoneAdj( pos, q, GetEncodedControllerArray() );
}

int CBaseAnimating::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// ----------------
		// Print Look time
		// ----------------
		char tempstr[1024];
		Q_snprintf(tempstr, sizeof(tempstr), "Sequence: (%3d) %s",GetSequence(), GetSequenceName( GetSequence() ) );
		EntityText(text_offset,tempstr,0);
		text_offset++;
		const char *pActname = GetSequenceActivityName(GetSequence());
		if ( pActname && strlen(pActname) )
		{
			Q_snprintf(tempstr, sizeof(tempstr), "Activity %s", pActname );
			EntityText(text_offset,tempstr,0);
			text_offset++;
		}

		Q_snprintf(tempstr, sizeof(tempstr), "Cycle: %.5f (%.5f)", (float)GetCycle(), m_flAnimTime.Get() );
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}

	// Visualize attachment points
	if ( m_debugOverlays & OVERLAY_ATTACHMENTS_BIT )
	{	
		CStudioHdr *pStudioHdr = GetModelPtr();

		if ( pStudioHdr )
		{
			Vector	vecPos, vecForward, vecRight, vecUp;
			char tempstr[256];

			// Iterate all the stored attachments
			for ( int i = 1; i <= pStudioHdr->GetNumAttachments(); i++ )
			{
				GetAttachment( i, vecPos, &vecForward, &vecRight, &vecUp );

				// Red - forward, green - right, blue - up
				NDebugOverlay::Line( vecPos, vecPos + ( vecForward * 4.0f ), 255, 0, 0, true, 0.05f );
				NDebugOverlay::Line( vecPos, vecPos + ( vecRight * 4.0f ), 0, 255, 0, true, 0.05f );
				NDebugOverlay::Line( vecPos, vecPos + ( vecUp * 4.0f ), 0, 0, 255, true, 0.05f );
				
				Q_snprintf( tempstr, sizeof(tempstr), " < %s (%d)", pStudioHdr->pAttachment(i-1).pszName(), i );
				NDebugOverlay::Text( vecPos, tempstr, true, 0.05f );
			}
		}
	}

	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Force a clientside-animating entity to reset it's frame
//-----------------------------------------------------------------------------
void CBaseAnimating::ResetClientsideFrame( void )
{
	// TODO: Once we can chain MSG_ENTITY messages, use one of them
	m_bClientSideFrameReset = !(bool)m_bClientSideFrameReset;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the origin at which to play an inputted dispatcheffect 
//-----------------------------------------------------------------------------
void CBaseAnimating::GetInputDispatchEffectPosition( const char *sInputString, Vector &pOrigin, QAngle &pAngles )
{
	// See if there's a specified attachment point
	int iAttachment;
	if ( GetModelPtr() && sscanf( sInputString, "%d", &iAttachment ) )
	{
		if ( !GetAttachment( iAttachment, pOrigin, pAngles ) )
		{
			Msg( "ERROR: Mapmaker tried to spawn DispatchEffect %s, but %s has no attachment %d\n", 
				sInputString, STRING(GetModelName()), iAttachment );
		}
		return;
	}

	BaseClass::GetInputDispatchEffectPosition( sInputString, pOrigin, pAngles );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : setnum - 
//-----------------------------------------------------------------------------
void CBaseAnimating::SetHitboxSet( int setnum )
{
#ifdef _DEBUG
	CStudioHdr *pStudioHdr = GetModelPtr();
	if ( !pStudioHdr )
		return;

	if (setnum > pStudioHdr->numhitboxsets())
	{
		// Warn if an bogus hitbox set is being used....
		static bool s_bWarned = false;
		if (!s_bWarned)
		{
			Warning("Using bogus hitbox set in entity %s!\n", GetClassname() );
			s_bWarned = true;
		}
		setnum = 0;
	}
#endif

	m_nHitboxSet = setnum;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *setname - 
//-----------------------------------------------------------------------------
void CBaseAnimating::SetHitboxSetByName( const char *setname )
{
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );
	m_nHitboxSet = FindHitboxSetByName( GetModelPtr(), setname );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CBaseAnimating::GetHitboxSet( void )
{
	return m_nHitboxSet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *CBaseAnimating::GetHitboxSetName( void )
{
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );
	return ::GetHitboxSetName( GetModelPtr(), m_nHitboxSet );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CBaseAnimating::GetHitboxSetCount( void )
{
	AssertMsg( GetModelPtr(), "GetModelPtr NULL. %s", STRING(GetEntityName()) ? STRING(GetEntityName()) : "" );
	return ::GetHitboxSetCount( GetModelPtr() );
}

static Vector	hullcolor[8] = 
{
	Vector( 1.0, 1.0, 1.0 ),
	Vector( 1.0, 0.5, 0.5 ),
	Vector( 0.5, 1.0, 0.5 ),
	Vector( 1.0, 1.0, 0.5 ),
	Vector( 0.5, 0.5, 1.0 ),
	Vector( 1.0, 0.5, 1.0 ),
	Vector( 0.5, 1.0, 1.0 ),
	Vector( 1.0, 1.0, 1.0 )
};

//-----------------------------------------------------------------------------
// Purpose: Send the current hitboxes for this model to the client ( to compare with
//  r_drawentities 3 client side boxes ).
// WARNING:  This uses a ton of bandwidth, only use on a listen server
//-----------------------------------------------------------------------------
void CBaseAnimating::DrawServerHitboxes( float duration /*= 0.0f*/, bool monocolor /*= false*/  )
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	if ( !pStudioHdr )
		return;

	mstudiohitboxset_t *set =pStudioHdr->pHitboxSet( m_nHitboxSet );
	if ( !set )
		return;

	Vector position;
	QAngle angles;

	int r = 0;
	int g = 0;
	int b = 255;

	for ( int i = 0; i < set->numhitboxes; i++ )
	{
		mstudiobbox_t *pbox = set->pHitbox( i );

		GetBonePosition( pbox->bone, position, angles );

		if ( !monocolor )
		{
			int j = (pbox->group % 8);
			
			r = ( int ) ( 255.0f * hullcolor[j][0] );
			g = ( int ) ( 255.0f * hullcolor[j][1] );
			b = ( int ) ( 255.0f * hullcolor[j][2] );
		}

		NDebugOverlay::BoxAngles( position, pbox->bbmin * GetModelScale(), pbox->bbmax * GetModelScale(), angles, r, g, b, 0 ,duration );
	}
}


void CBaseAnimating::DrawRawSkeleton( matrix3x4_t boneToWorld[], int boneMask, bool noDepthTest, float duration, bool monocolor )
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	if ( !pStudioHdr )
		return;

	int i;
	int r = 255;
	int g = 255;
	int b = monocolor ? 255 : 0;
	

	for (i = 0; i < pStudioHdr->numbones(); i++)
	{
		if (pStudioHdr->pBone( i )->flags & boneMask)
		{
			Vector p1;
			MatrixPosition( boneToWorld[i], p1 );
			if ( pStudioHdr->pBone( i )->parent != -1 )
			{
				Vector p2;
				MatrixPosition( boneToWorld[pStudioHdr->pBone( i )->parent], p2 );
                NDebugOverlay::Line( p1, p2, r, g, b, noDepthTest, duration );
			}
		}
	}
}


int CBaseAnimating::GetHitboxBone( int hitboxIndex )
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	if ( pStudioHdr )
	{
		mstudiohitboxset_t *set =pStudioHdr->pHitboxSet( m_nHitboxSet );
		if ( set && hitboxIndex < set->numhitboxes )
		{
			return set->pHitbox( hitboxIndex )->bone;
		}
	}
	return 0;
}


//-----------------------------------------------------------------------------
// Computes a box that surrounds all hitboxes
//-----------------------------------------------------------------------------
bool CBaseAnimating::ComputeHitboxSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs )
{
	// Note that this currently should not be called during Relink because of IK.
	// The code below recomputes bones so as to get at the hitboxes,
	// which causes IK to trigger, which causes raycasts against the other entities to occur,
	// which is illegal to do while in the Relink phase.

	CStudioHdr *pStudioHdr = GetModelPtr();
	if (!pStudioHdr)
		return false;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( m_nHitboxSet );
	if ( !set || !set->numhitboxes )
		return false;

	CBoneCache *pCache = GetBoneCache();

	// Compute a box in world space that surrounds this entity
	pVecWorldMins->Init( FLT_MAX, FLT_MAX, FLT_MAX );
	pVecWorldMaxs->Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );

	Vector vecBoxAbsMins, vecBoxAbsMaxs;
	for ( int i = 0; i < set->numhitboxes; i++ )
	{
		mstudiobbox_t *pbox = set->pHitbox(i);
		matrix3x4_t *pMatrix = pCache->GetCachedBone(pbox->bone);

		if ( pMatrix )
		{
			TransformAABB( *pMatrix, pbox->bbmin * GetModelScale(), pbox->bbmax * GetModelScale(), vecBoxAbsMins, vecBoxAbsMaxs );
			VectorMin( *pVecWorldMins, vecBoxAbsMins, *pVecWorldMins );
			VectorMax( *pVecWorldMaxs, vecBoxAbsMaxs, *pVecWorldMaxs );
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Computes a box that surrounds all hitboxes, in entity space
//-----------------------------------------------------------------------------
bool CBaseAnimating::ComputeEntitySpaceHitboxSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs )
{
	// Note that this currently should not be called during position recomputation because of IK.
	// The code below recomputes bones so as to get at the hitboxes,
	// which causes IK to trigger, which causes raycasts against the other entities to occur,
	// which is illegal to do while in the computeabsposition phase.

	CStudioHdr *pStudioHdr = GetModelPtr();
	if (!pStudioHdr)
		return false;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( m_nHitboxSet );
	if ( !set || !set->numhitboxes )
		return false;

	CBoneCache *pCache = GetBoneCache();
	matrix3x4_t *hitboxbones[MAXSTUDIOBONES];
	pCache->ReadCachedBonePointers( hitboxbones, pStudioHdr->numbones() );

	// Compute a box in world space that surrounds this entity
	pVecWorldMins->Init( FLT_MAX, FLT_MAX, FLT_MAX );
	pVecWorldMaxs->Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );

	matrix3x4_t worldToEntity, boneToEntity;
	MatrixInvert( EntityToWorldTransform(), worldToEntity );

	Vector vecBoxAbsMins, vecBoxAbsMaxs;
	for ( int i = 0; i < set->numhitboxes; i++ )
	{
		mstudiobbox_t *pbox = set->pHitbox(i);

		ConcatTransforms( worldToEntity, *hitboxbones[pbox->bone], boneToEntity );
		TransformAABB( boneToEntity, pbox->bbmin, pbox->bbmax, vecBoxAbsMins, vecBoxAbsMaxs );
		VectorMin( *pVecWorldMins, vecBoxAbsMins, *pVecWorldMins );
		VectorMax( *pVecWorldMaxs, vecBoxAbsMaxs, *pVecWorldMaxs );
	}
	return true;
}


int CBaseAnimating::GetPhysicsBone( int boneIndex )
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	if ( pStudioHdr )
	{
		if ( boneIndex >= 0 && boneIndex < pStudioHdr->numbones() )
			return pStudioHdr->pBone( boneIndex )->physicsbone;
	}
	return 0;
}

bool CBaseAnimating::LookupHitbox( const char *szName, int& outSet, int& outBox )
{
	CStudioHdr* pHdr = GetModelPtr();

	outSet = -1;
	outBox = -1;

	if( !pHdr )
		return false;

	for( int set=0; set < pHdr->numhitboxsets(); set++ )
	{
		for( int i = 0; i < pHdr->iHitboxCount(set); i++ )
		{
			mstudiobbox_t* pBox = pHdr->pHitbox( i, set );
			
			if( !pBox )
				continue;
			
			const char* szBoxName = pBox->pszHitboxName();
			if( Q_stricmp( szBoxName, szName ) == 0 )
			{
				outSet = set;
				outBox = i;
				return true;
			}
		}
	}

	return false;
}

void CBaseAnimating::CopyAnimationDataFrom( CBaseAnimating *pSource )
{
	this->SetModelName( pSource->GetModelName() );
	this->SetModelIndex( pSource->GetModelIndex() );
	this->SetCycle( pSource->GetCycle() );
	this->SetEffects( pSource->GetEffects() );
	this->IncrementInterpolationFrame();
	this->SetSequence( pSource->GetSequence() );
	this->m_flAnimTime = pSource->m_flAnimTime;
	this->m_nBody = pSource->m_nBody;
	this->m_nSkin = pSource->m_nSkin;
	this->LockStudioHdr();
}

int CBaseAnimating::GetHitboxesFrontside( int *boxList, int boxMax, const Vector &normal, float dist )
{
	int count = 0;
	CStudioHdr *pStudioHdr = GetModelPtr();
	if ( pStudioHdr )
	{
		mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( m_nHitboxSet );
		if ( set )
		{
			matrix3x4_t matrix;
			for ( int b = 0; b < set->numhitboxes; b++ )
			{
				mstudiobbox_t *pbox = set->pHitbox( b );

				GetBoneTransform( pbox->bone, matrix );
				Vector center = (pbox->bbmax + pbox->bbmin) * 0.5;
				Vector centerWs;
				VectorTransform( center, matrix, centerWs );
				if ( DotProduct( centerWs, normal ) >= dist )
				{
					if ( count < boxMax )
					{
						boxList[count] = b;
						count++;
					}
				}
			}
		}
	}

	return count;
}

void CBaseAnimating::EnableServerIK()
{
	if (!m_pIk)
	{
		m_pIk = new CIKContext;
		m_iIKCounter = 0;
	}
}

void CBaseAnimating::DisableServerIK()
{
	delete m_pIk;
	m_pIk = NULL;
}

Activity CBaseAnimating::GetSequenceActivity( int iSequence )
{
	if( iSequence == -1 )
	{
		return ACT_INVALID;
	}

	if ( !GetModelPtr() )
		return ACT_INVALID;

	return (Activity)::GetSequenceActivity( GetModelPtr(), iSequence );
}

void CBaseAnimating::ModifyOrAppendCriteria( AI_CriteriaSet& set )
{
	BaseClass::ModifyOrAppendCriteria( set );

	// TODO
	// Append any animation state parameters here
}


void CBaseAnimating::DoMuzzleFlash()
{
	m_nMuzzleFlashParity = (m_nMuzzleFlashParity+1) & ((1 << EF_MUZZLEFLASH_BITS) - 1);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : scale - 
//-----------------------------------------------------------------------------
void CBaseAnimating::SetModelScale( float scale, float change_duration /*= 0.0f*/  )
{
	if ( change_duration > 0.0f )
	{
		ModelScale *mvs = ( ModelScale * )CreateDataObject( MODELSCALE );
		mvs->m_flModelScaleStart = m_flModelScale;
		mvs->m_flModelScaleGoal = scale;
		mvs->m_flModelScaleStartTime = gpGlobals->curtime;
		mvs->m_flModelScaleFinishTime = mvs->m_flModelScaleStartTime + change_duration;
		SetContextThink( &CBaseAnimating::UpdateModelScale, gpGlobals->curtime, "UpdateModelScaleThink" );
	}
	else
	{
		m_flModelScale = scale;
		RefreshCollisionBounds();

		if ( HasDataObjectType( MODELSCALE ) )
		{
			DestroyDataObject( MODELSCALE );
		}
	}
}

void CBaseAnimating::UpdateModelScale()
{
	ModelScale *mvs = ( ModelScale * )GetDataObject( MODELSCALE );
	if ( !mvs )
	{
		return;
	}

	float dt = mvs->m_flModelScaleFinishTime - mvs->m_flModelScaleStartTime;
	Assert( dt > 0.0f );

	float frac = ( gpGlobals->curtime - mvs->m_flModelScaleStartTime ) / dt;
	frac = clamp( frac, 0.0f, 1.0f );

	if ( gpGlobals->curtime >= mvs->m_flModelScaleFinishTime )
	{
		m_flModelScale = mvs->m_flModelScaleGoal;
		DestroyDataObject( MODELSCALE );
	}
	else
	{
		m_flModelScale = Lerp( frac, mvs->m_flModelScaleStart, mvs->m_flModelScaleGoal );
	}

	RefreshCollisionBounds();

	if ( frac < 1.f )
	{
		SetContextThink( &CBaseAnimating::UpdateModelScale, gpGlobals->curtime, "UpdateModelScaleThink" );
	}
}

void CBaseAnimating::RefreshCollisionBounds( void )
{
	CollisionProp()->RefreshScaledCollisionBounds();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseAnimating::Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner )
{
	if( IsOnFire() )
		return;

	bool bIsNPC = IsNPC();

	// Right now this prevents stuff we don't want to catch on fire from catching on fire.
	if( bNPCOnly && bIsNPC == false )
	{
		return;
	}

	if( bIsNPC == true && bCalledByLevelDesigner == false )
	{
		CAI_BaseNPC *pNPC = MyNPCPointer();

		if ( pNPC && pNPC->AllowedToIgnite() == false )
			 return;
	}

	CEntityFlame *pFlame = CEntityFlame::Create( this );
	if (pFlame)
	{
		pFlame->SetLifetime( flFlameLifetime );
		AddFlag( FL_ONFIRE );

		SetEffectEntity( pFlame );

		if ( flSize > 0.0f )
		{
			pFlame->SetSize( flSize );
		}
	}

	m_OnIgnite.FireOutput( this, this );
}

void CBaseAnimating::IgniteLifetime( float flFlameLifetime )
{
	if( !IsOnFire() )
		Ignite( 30, false, 0.0f, true );

	CEntityFlame *pFlame = dynamic_cast<CEntityFlame*>( GetEffectEntity() );

	if ( !pFlame )
		return;

	pFlame->SetLifetime( flFlameLifetime );
}

void CBaseAnimating::IgniteNumHitboxFires( int iNumHitBoxFires )
{
	if( !IsOnFire() )
		Ignite( 30, false, 0.0f, true );

	CEntityFlame *pFlame = dynamic_cast<CEntityFlame*>( GetEffectEntity() );

	if ( !pFlame )
		return;

	pFlame->SetNumHitboxFires( iNumHitBoxFires );
}

void CBaseAnimating::IgniteHitboxFireScale( float flHitboxFireScale )
{
	if( !IsOnFire() )
		Ignite( 30, false, 0.0f, true );

	CEntityFlame *pFlame = dynamic_cast<CEntityFlame*>( GetEffectEntity() );

	if ( !pFlame )
		return;

	pFlame->SetHitboxFireScale( flHitboxFireScale );
}

//-----------------------------------------------------------------------------
// Fades out!
//-----------------------------------------------------------------------------
bool CBaseAnimating::Dissolve( const char *pMaterialName, float flStartTime, bool bNPCOnly, int nDissolveType, Vector vDissolverOrigin, int iMagnitude )
{
	// Right now this prevents stuff we don't want to catch on fire from catching on fire.
	if( bNPCOnly && !(GetFlags() & FL_NPC) )
		return false;

	// Can't dissolve twice
	if ( IsDissolving() )
		return false;

	bool bRagdollCreated = false;
	CEntityDissolve *pDissolve = CEntityDissolve::Create( this, pMaterialName, flStartTime, nDissolveType, &bRagdollCreated );
	if (pDissolve)
	{
		SetEffectEntity( pDissolve );

		AddFlag( FL_DISSOLVING );
		m_flDissolveStartTime = flStartTime;
		pDissolve->SetDissolverOrigin( vDissolverOrigin );
		pDissolve->SetMagnitude( iMagnitude );
	}

	// if this is a ragdoll dissolving, fire an event
	if ( ( CLASS_NONE == Classify() ) && ( ClassMatches( "prop_ragdoll" ) ) )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "ragdoll_dissolved" );
		if ( event )
		{
			event->SetInt( "entindex", entindex() );
			gameeventmanager->FireEvent( event );
		}
	}

	return bRagdollCreated;
}


//-----------------------------------------------------------------------------
// Make a model look as though it's burning. 
//-----------------------------------------------------------------------------
void CBaseAnimating::Scorch( int rate, int floor )
{
	color32 color = GetRenderColor();

	if( color.r > floor )
		color.r -= rate;

	if( color.g > floor )
		color.g -= rate;

	if( color.b > floor )
		color.b -= rate;

	SetRenderColor( color.r, color.g, color.b );
}


void CBaseAnimating::ResetSequence(int nSequence)
{
	if (ai_sequence_debug.GetBool() == true && (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))
	{
		DevMsg("ResetSequence : %s: %s -> %s\n", GetClassname(), GetSequenceName(GetSequence()), GetSequenceName(nSequence));
	}
	
	if ( !SequenceLoops() )
	{
		SetCycle( 0 );
	}

	// Tracker 17868:  If the sequence number didn't actually change, but you call resetsequence info, it changes
	//  the newsequenceparity bit which causes the client to call m_flCycle.Reset() which causes a very slight 
	//  discontinuity in looping animations as they reset around to cycle 0.0.  This was causing the parentattached
	//  helmet on barney to hitch every time barney's idle cycled back around to its start.
	bool changed = nSequence != GetSequence() ? true : false;

	SetSequence( nSequence );
	if ( changed || !SequenceLoops() )
	{
		ResetSequenceInfo();
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseAnimating::InputIgnite( inputdata_t &inputdata )
{
	Ignite( 30, false, 0.0f, true );
}

void CBaseAnimating::InputIgniteLifetime( inputdata_t &inputdata )
{
	IgniteLifetime( inputdata.value.Float() );
}

void CBaseAnimating::InputIgniteNumHitboxFires( inputdata_t &inputdata )
{
	IgniteNumHitboxFires( inputdata.value.Int() );
}

void CBaseAnimating::InputIgniteHitboxFireScale( inputdata_t &inputdata )
{
	IgniteHitboxFireScale( inputdata.value.Float() );
}

void CBaseAnimating::InputBecomeRagdoll( inputdata_t &inputdata )
{
	BecomeRagdollOnClient( vec3_origin );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimating::SetFadeDistance( float minFadeDist, float maxFadeDist )
{
	m_fadeMinDist = minFadeDist;
	m_fadeMaxDist = maxFadeDist;
}

//-----------------------------------------------------------------------------
// Purpose: Async prefetches all anim data used by a particular sequence.  Returns true if all of the required data is memory resident
// Input  : iSequence - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseAnimating::PrefetchSequence( int iSequence )
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	if ( !pStudioHdr )
		return true;

	return Studio_PrefetchSequence( pStudioHdr, iSequence );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseAnimating::IsSequenceLooping( CStudioHdr *pStudioHdr, int iSequence )
{
	return (::GetSequenceFlags( pStudioHdr, iSequence ) & STUDIO_LOOPING) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: model-change notification. Fires on dynamic load completion as well
//-----------------------------------------------------------------------------
CStudioHdr *CBaseAnimating::OnNewModel()
{
	(void) BaseClass::OnNewModel();

	// TODO: if dynamic, validate m_Sequence and apply queued body group settings?
	if ( IsDynamicModelLoading() )
	{
		// Called while dynamic model still loading -> new model, clear deferred state
		m_bResetSequenceInfoOnLoad = false;
		return NULL;
	}

	CStudioHdr *hdr = GetModelPtr(); 

	if ( m_bResetSequenceInfoOnLoad )
	{
		m_bResetSequenceInfoOnLoad = false;
		ResetSequenceInfo();
	}

	return hdr;
}
