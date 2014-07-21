//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include "cbase.h"
#include "animation.h"
#include "studio.h"
#include "bone_setup.h"
#include "ai_basenpc.h"
#include "npcevent.h"

#include "saverestore_utlvector.h"
#include "dt_utlvector_send.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar ai_sequence_debug;


BEGIN_SIMPLE_DATADESC( CAnimationLayer )

//	DEFINE_FIELD( m_pOwnerEntity, CBaseAnimatingOverlay ),
	DEFINE_FIELD( m_fFlags, FIELD_INTEGER ),
	DEFINE_FIELD( m_bSequenceFinished, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bLooping, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( m_flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( m_flPrevCycle, FIELD_FLOAT ),
	DEFINE_FIELD( m_flPlaybackRate, FIELD_FLOAT),
	DEFINE_FIELD( m_flWeight, FIELD_FLOAT),
	DEFINE_FIELD( m_flBlendIn, FIELD_FLOAT ),
	DEFINE_FIELD( m_flBlendOut, FIELD_FLOAT ),
	DEFINE_FIELD( m_flKillRate, FIELD_FLOAT ),
	DEFINE_FIELD( m_flKillDelay, FIELD_FLOAT ),
	DEFINE_CUSTOM_FIELD( m_nActivity, ActivityDataOps() ),
	DEFINE_FIELD( m_nPriority, FIELD_INTEGER ),
	DEFINE_FIELD( m_nOrder, FIELD_INTEGER ),
	DEFINE_FIELD( m_flLastEventCheck, FIELD_FLOAT ),
	DEFINE_FIELD( m_flLastAccess, FIELD_TIME ),
	DEFINE_FIELD( m_flLayerAnimtime, FIELD_FLOAT ),
	DEFINE_FIELD( m_flLayerFadeOuttime, FIELD_FLOAT ),

END_DATADESC()


BEGIN_DATADESC( CBaseAnimatingOverlay )

	DEFINE_UTLVECTOR( m_AnimOverlay, FIELD_EMBEDDED ),

	// DEFINE_FIELD( m_nActiveLayers, FIELD_INTEGER ),
	// DEFINE_FIELD( m_nActiveBaseLayers, FIELD_INTEGER ),

END_DATADESC()


#define ORDER_BITS			4
#define WEIGHT_BITS			8

BEGIN_SEND_TABLE_NOBASE(CAnimationLayer, DT_Animationlayer)
	SendPropInt		(SENDINFO(m_nSequence),		ANIMATION_SEQUENCE_BITS,SPROP_UNSIGNED),
	SendPropFloat	(SENDINFO(m_flCycle),		ANIMATION_CYCLE_BITS,	SPROP_ROUNDDOWN,	0.0f,   1.0f),
	SendPropFloat	(SENDINFO(m_flPrevCycle),	ANIMATION_CYCLE_BITS,	SPROP_ROUNDDOWN,	0.0f,   1.0f),
	SendPropFloat	(SENDINFO(m_flWeight),		WEIGHT_BITS,			0,	0.0f,	1.0f),
	SendPropInt		(SENDINFO(m_nOrder),		ORDER_BITS,				SPROP_UNSIGNED),
END_SEND_TABLE()


BEGIN_SEND_TABLE_NOBASE( CBaseAnimatingOverlay, DT_OverlayVars )
	SendPropUtlVector( 
		SENDINFO_UTLVECTOR( m_AnimOverlay ),
		CBaseAnimatingOverlay::MAX_OVERLAYS, // max elements
		SendPropDataTable( NULL, 0, &REFERENCE_SEND_TABLE( DT_Animationlayer ) )  )
END_SEND_TABLE()


IMPLEMENT_SERVERCLASS_ST( CBaseAnimatingOverlay, DT_BaseAnimatingOverlay )
	// These are in their own separate data table so CCSPlayer can exclude all of these.
	SendPropDataTable( "overlay_vars", 0, &REFERENCE_SEND_TABLE( DT_OverlayVars ) )
END_SEND_TABLE()




CAnimationLayer::CAnimationLayer( )
{
	Init( NULL );
}


void CAnimationLayer::Init( CBaseAnimatingOverlay *pOverlay )
{
	m_pOwnerEntity = pOverlay;
	m_fFlags = 0;
	m_flWeight = 0;
	m_flCycle = 0;
	m_flPrevCycle = 0;
	m_bSequenceFinished = false;
	m_nActivity = ACT_INVALID;
	m_nSequence = 0;
	m_nPriority = 0;
	m_nOrder.Set( CBaseAnimatingOverlay::MAX_OVERLAYS );

	m_flBlendIn = 0.0;
	m_flBlendOut = 0.0;

	m_flKillRate = 100.0;
	m_flKillDelay = 0.0;
	m_flPlaybackRate = 1.0;
	m_flLastEventCheck = 0.0;
	m_flLastAccess = gpGlobals->curtime;
	m_flLayerAnimtime = 0;
	m_flLayerFadeOuttime = 0;
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------

void CAnimationLayer::StudioFrameAdvance( float flInterval, CBaseAnimating *pOwner )
{
	float flCycleRate = pOwner->GetSequenceCycleRate( m_nSequence );

	m_flPrevCycle = m_flCycle;
	m_flCycle += flInterval * flCycleRate * m_flPlaybackRate;

	if (m_flCycle < 0.0)
	{
		if (m_bLooping)
		{
			m_flCycle -= (int)(m_flCycle);
		}
		else
		{
			m_flCycle = 0;
		}
	}
	else if (m_flCycle >= 1.0) 
	{
		m_bSequenceFinished = true;

		if (m_bLooping)
		{
			m_flCycle -= (int)(m_flCycle);
		}
		else
		{
			m_flCycle = 1.0;
		}
	}

	if (IsAutoramp())
	{
		m_flWeight = 1;
	
		// blend in?
		if ( m_flBlendIn != 0.0f )
		{
			if (m_flCycle < m_flBlendIn)
			{
				m_flWeight = m_flCycle / m_flBlendIn;
			}
		}
		
		// blend out?
		if ( m_flBlendOut != 0.0f )
		{
			if (m_flCycle > 1.0 - m_flBlendOut)
			{
				m_flWeight = (1.0 - m_flCycle) / m_flBlendOut;
			}
		}

		m_flWeight = 3.0 * m_flWeight * m_flWeight - 2.0 * m_flWeight * m_flWeight * m_flWeight;
		if (m_nSequence == 0)
			m_flWeight = 0;
	}
}

//------------------------------------------------------------------------------

bool CAnimationLayer::IsAbandoned( void )
{ 
	if (IsActive() && !IsAutokill() && !IsKillMe() && m_flLastAccess > 0.0 && (gpGlobals->curtime - m_flLastAccess > 0.2)) 
		return true; 
	else 
		return false;
}

void CAnimationLayer::MarkActive( void )
{ 
	m_flLastAccess = gpGlobals->curtime;
}

//------------------------------------------------------------------------------

void CBaseAnimatingOverlay::VerifyOrder( void )
{
#ifdef _DEBUG
	int i, j;
	// test sorting of the layers
	int layer[MAX_OVERLAYS];
	int maxOrder = -1;
	for (i = 0; i < MAX_OVERLAYS; i++)
	{
		layer[i] = MAX_OVERLAYS;
	}
	for (i = 0; i < m_AnimOverlay.Count(); i++)
	{
		if (m_AnimOverlay[ i ].m_nOrder < MAX_OVERLAYS)
		{
			j = m_AnimOverlay[ i ].m_nOrder;
			Assert( layer[j] == MAX_OVERLAYS );
			layer[j] = i;
			if (j > maxOrder)
				maxOrder = j;
		}
	}

	// make sure they're sequential
	// Aim layers are allowed to have gaps, and we are moving aim blending to server
//	for ( i = 0; i <= maxOrder; i++ )
//	{
//		Assert( layer[i] != MAX_OVERLAYS);
//	}

	/*
	for ( i = 0; i < MAX_OVERLAYS; i++ )
	{
		int j = layer[i];
		if (j != MAX_OVERLAYS)
		{
			char tempstr[512];
			Q_snprintf( tempstr, sizeof( tempstr ),"%d : %d :%.2f :%d:%d:%.1f", 
				j, 
				m_AnimOverlay[ j ].m_nSequence, 
				m_AnimOverlay[ j ].m_flWeight,
				m_AnimOverlay[ j ].IsActive(),
				m_AnimOverlay[ j ].IsKillMe(),
				m_AnimOverlay[ j ].m_flKillDelay
				);
			EntityText( i, tempstr, 0.1 );
		}
	}
	*/
#endif
}


//------------------------------------------------------------------------------
// Purpose : advance the animation frame up to the current time
//			 if an flInterval is passed in, only advance animation that number of seconds
// Input   :
// Output  :
//------------------------------------------------------------------------------

void CBaseAnimatingOverlay::StudioFrameAdvance ()
{
	float flAdvance = GetAnimTimeInterval();

	VerifyOrder();

	BaseClass::StudioFrameAdvance();

	for ( int i = 0; i < m_AnimOverlay.Count(); i++ )
	{
		CAnimationLayer *pLayer = &m_AnimOverlay[i];
		
		if (pLayer->IsActive())
		{
			// Assert( !m_AnimOverlay[ i ].IsAbandoned() );
			if (pLayer->IsKillMe())
			{
				if (pLayer->m_flKillDelay > 0)
				{
					pLayer->m_flKillDelay -= flAdvance;
					pLayer->m_flKillDelay = clamp( 	pLayer->m_flKillDelay, 0.0f, 1.0f );
				}
				else if (pLayer->m_flWeight != 0.0f)
				{
					// give it at least one frame advance cycle to propagate 0.0 to client
					pLayer->m_flWeight -= pLayer->m_flKillRate * flAdvance;
					pLayer->m_flWeight = clamp( (float) pLayer->m_flWeight, 0.0f, 1.0f );
				}
				else
				{
					// shift the other layers down in order
					if (ai_sequence_debug.GetBool() == true && m_debugOverlays & OVERLAY_NPC_SELECTED_BIT)
					{
						Msg("removing %d (%d): %s : %5.3f (%.3f)\n", i, pLayer->m_nOrder.Get(), GetSequenceName( pLayer->m_nSequence ), pLayer->m_flCycle.Get(), pLayer->m_flWeight.Get() );
					}
					FastRemoveLayer( i );
					// needs at least one thing cycle dead to trigger sequence change
					pLayer->Dying();
					continue;
				}
			}

			pLayer->StudioFrameAdvance( flAdvance, this );
			if ( pLayer->m_bSequenceFinished && (pLayer->IsAutokill()) )
			{
				pLayer->m_flWeight = 0.0f;
				pLayer->KillMe();
			}
		}
		else if (pLayer->IsDying())
		{
			pLayer->Dead();	
		}
		else if (pLayer->m_flWeight > 0.0)
		{
			// Now that the server blends, it is turning off layers all the time.  Having a weight left over
			// when you're no longer marked as active is now harmless and commonplace.  Just clean up.
			pLayer->Init( this );
			pLayer->Dying();
		}
	}

	if (ai_sequence_debug.GetBool() == true && m_debugOverlays & OVERLAY_NPC_SELECTED_BIT)
	{
		for ( int i = 0; i < m_AnimOverlay.Count(); i++ )
		{
			if (m_AnimOverlay[ i ].IsActive())
			{
				/*
				if (m_AnimOverlay[ i ].IsAbandoned())
				{
					Msg(" %d abandoned %.2f (%.2f)\n", i, gpGlobals->curtime, m_AnimOverlay[ i ].m_flLastAccess );
				}
				*/
				Msg(" %d (%d): %s : %5.3f (%.3f)\n", i, m_AnimOverlay[ i ].m_nOrder.Get(), GetSequenceName( m_AnimOverlay[ i ].m_nSequence ), m_AnimOverlay[ i ].m_flCycle.Get(), m_AnimOverlay[ i ].m_flWeight.Get() );
			}
		}
	}

	VerifyOrder();
}



//=========================================================
// DispatchAnimEvents
//=========================================================
void CBaseAnimatingOverlay::DispatchAnimEvents ( CBaseAnimating *eventHandler )
{
	BaseClass::DispatchAnimEvents( eventHandler );

	for ( int i = 0; i < m_AnimOverlay.Count(); i++ )
	{
		if (m_AnimOverlay[ i ].IsActive())
		{
			m_AnimOverlay[ i ].DispatchAnimEvents( eventHandler, this );
		}
	}
}

void CAnimationLayer::DispatchAnimEvents( CBaseAnimating *eventHandler, CBaseAnimating *pOwner )
{
  	animevent_t	event;

	CStudioHdr *pstudiohdr = pOwner->GetModelPtr( );

	if ( !pstudiohdr )
	{
		Assert(!"CBaseAnimating::DispatchAnimEvents: model missing");
		return;
	}

	if ( !pstudiohdr->SequencesAvailable() )
	{
		return;
	}

	if ( m_nSequence >= pstudiohdr->GetNumSeq() )
		return;
	
	// don't fire if here are no events
	if ( pstudiohdr->pSeqdesc( m_nSequence ).numevents == 0 )
	{
		return;
	}

	// look from when it last checked to some short time in the future	
	float flCycleRate = pOwner->GetSequenceCycleRate( m_nSequence ) * m_flPlaybackRate;
	float flStart = m_flLastEventCheck;
	float flEnd = m_flCycle;

	if (!m_bLooping)
	{
		// fire off events early
		float flLastVisibleCycle = 1.0f - (pstudiohdr->pSeqdesc( m_nSequence ).fadeouttime) * flCycleRate;
		if (flEnd >= flLastVisibleCycle || flEnd < 0.0) 
		{
			m_bSequenceFinished = true;
			flEnd = 1.0f;
		}
	}
	m_flLastEventCheck = flEnd;

	/*
	if (pOwner->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT)
	{
		Msg( "%s:%s : checking %.2f %.2f (%d)\n", STRING(pOwner->GetModelName()), pstudiohdr->pSeqdesc( m_nSequence ).pszLabel(), flStart, flEnd, m_bSequenceFinished );
	}
	*/

	// FIXME: does not handle negative framerates!
	int index = 0;
	while ( (index = GetAnimationEvent( pstudiohdr, m_nSequence, &event, flStart, flEnd, index ) ) != 0 )
	{
		event.pSource = pOwner;
		// calc when this event should happen
		if (flCycleRate > 0.0)
		{
			float flCycle = event.cycle;
			if (flCycle > m_flCycle)
			{
				flCycle = flCycle - 1.0;
			}
			event.eventtime = pOwner->m_flAnimTime + (flCycle - m_flCycle) / flCycleRate + pOwner->GetAnimTimeInterval();
		}

		// Msg( "dispatch %d (%d : %.2f)\n", index - 1, event.event, event.eventtime );
		eventHandler->HandleAnimEvent( &event );
	}
}



void CBaseAnimatingOverlay::GetSkeleton( CStudioHdr *pStudioHdr, Vector pos[], Quaternion q[], int boneMask )
{
	if(!pStudioHdr)
	{
		Assert(!"CBaseAnimating::GetSkeleton() without a model");
		return;
	}

	if (!pStudioHdr->SequencesAvailable())
	{
		return;
	}

	IBoneSetup boneSetup( pStudioHdr, boneMask, GetPoseParameterArray() );
	boneSetup.InitPose( pos, q );

	boneSetup.AccumulatePose( pos, q, GetSequence(), GetCycle(), 1.0, gpGlobals->curtime, m_pIk );

	// sort the layers
	int layer[MAX_OVERLAYS] = {};
	int i;
	for (i = 0; i < m_AnimOverlay.Count(); i++)
	{
		layer[i] = MAX_OVERLAYS;
	}
	for (i = 0; i < m_AnimOverlay.Count(); i++)
	{
		CAnimationLayer &pLayer = m_AnimOverlay[i];
		if( (pLayer.m_flWeight > 0) && pLayer.IsActive() && pLayer.m_nOrder >= 0 && pLayer.m_nOrder < m_AnimOverlay.Count())
		{
			layer[pLayer.m_nOrder] = i;
		}
	}
	for (i = 0; i < m_AnimOverlay.Count(); i++)
	{
		if (layer[i] >= 0 && layer[i] < m_AnimOverlay.Count())
		{
			CAnimationLayer &pLayer = m_AnimOverlay[layer[i]];
			// UNDONE: Is it correct to use overlay weight for IK too?
			boneSetup.AccumulatePose( pos, q, pLayer.m_nSequence, pLayer.m_flCycle, pLayer.m_flWeight, gpGlobals->curtime, m_pIk );
		}
	}

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



//-----------------------------------------------------------------------------
// Purpose: zero's out all non-restore safe fields
// Output :
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::OnRestore( )
{
	int i;

	// force order of unused layers to current MAX_OVERLAYS
	// and Tracker 48843 (Alyx skating after restore) restore the owner entity ptr (otherwise the network layer won't get NetworkStateChanged signals until the layer is re-Init()'ed
	for (i = 0; i < m_AnimOverlay.Count(); i++)
	{
		m_AnimOverlay[i].m_pOwnerEntity = this;

		if ( !m_AnimOverlay[i].IsActive())
		{
			m_AnimOverlay[i].m_nOrder.Set( MAX_OVERLAYS );
		}
	}

	// get rid of all layers that shouldn't be restored
	for (i = 0; i < m_AnimOverlay.Count(); i++)
	{
		if ( ( m_AnimOverlay[i].IsActive() && (m_AnimOverlay[i].m_fFlags & ANIM_LAYER_DONTRESTORE) ) ||
			 ( GetModelPtr() && !IsValidSequence(m_AnimOverlay[i].m_nSequence) ) )
		{
			FastRemoveLayer( i );
		}
	}

	BaseClass::OnRestore();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CBaseAnimatingOverlay::AddGestureSequence( int sequence, bool autokill /*= true*/ )
{
	int i = AddLayeredSequence( sequence, 0 );
	// No room?
	if ( IsValidLayer( i ) )
	{
		SetLayerAutokill( i, autokill );
	}

	return i;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CBaseAnimatingOverlay::AddGestureSequence( int nSequence, float flDuration, bool autokill /*= true*/ )
{
	int iLayer = AddGestureSequence( nSequence, autokill );
	Assert( iLayer != -1 );

	if (iLayer >= 0 && flDuration > 0)
	{
		m_AnimOverlay[iLayer].m_flPlaybackRate = SequenceDuration( nSequence ) / flDuration;
	}
	return iLayer;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CBaseAnimatingOverlay::AddGesture( Activity activity, bool autokill /*= true*/ )
{
	if ( IsPlayingGesture( activity ) )
	{
		return FindGestureLayer( activity );
	}

	int seq = SelectWeightedSequence( activity );
	if ( seq <= 0 )
	{
		const char *actname = CAI_BaseNPC::GetActivityName( activity );
		DevMsg( "CBaseAnimatingOverlay::AddGesture:  model %s missing activity %s\n", STRING(GetModelName()), actname );
		return -1;
	}

	int i = AddGestureSequence( seq, autokill );
	Assert( i != -1 );
	if ( i != -1 )
	{
		m_AnimOverlay[ i ].m_nActivity = activity;
	}

	return i;
}


int CBaseAnimatingOverlay::AddGesture( Activity activity, float flDuration, bool autokill /*= true*/ )
{
	int iLayer = AddGesture( activity, autokill );
	SetLayerDuration( iLayer, flDuration );

	return iLayer;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------

void CBaseAnimatingOverlay::SetLayerDuration( int iLayer, float flDuration )
{
	if (IsValidLayer( iLayer ) && flDuration > 0)
	{
		m_AnimOverlay[iLayer].m_flPlaybackRate = SequenceDuration( m_AnimOverlay[iLayer].m_nSequence ) / flDuration;
	}
}



//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------

float CBaseAnimatingOverlay::GetLayerDuration( int iLayer )
{
	if (IsValidLayer( iLayer ))
	{
		if (m_AnimOverlay[iLayer].m_flPlaybackRate != 0.0f)
		{
			return (1.0 - m_AnimOverlay[iLayer].m_flCycle) * SequenceDuration( m_AnimOverlay[iLayer].m_nSequence ) / m_AnimOverlay[iLayer].m_flPlaybackRate;
		}
		return SequenceDuration( m_AnimOverlay[iLayer].m_nSequence );
	}
	return 0.0;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int	CBaseAnimatingOverlay::AddLayeredSequence( int sequence, int iPriority )
{
	int i = AllocateLayer( iPriority );
	// No room?
	if ( IsValidLayer( i ) )
	{
		m_AnimOverlay[i].m_flCycle = 0;
		m_AnimOverlay[i].m_flPrevCycle = 0;
		m_AnimOverlay[i].m_flPlaybackRate = 1.0;
		m_AnimOverlay[i].m_nActivity = ACT_INVALID;
		m_AnimOverlay[i].m_nSequence = sequence;
		m_AnimOverlay[i].m_flWeight = 1.0f;
		m_AnimOverlay[i].m_flBlendIn = 0.0f;
		m_AnimOverlay[i].m_flBlendOut = 0.0f;
		m_AnimOverlay[i].m_bSequenceFinished = false;
		m_AnimOverlay[i].m_flLastEventCheck = 0;
		m_AnimOverlay[i].m_bLooping = ((GetSequenceFlags( GetModelPtr(), sequence ) & STUDIO_LOOPING) != 0);
		if (ai_sequence_debug.GetBool() == true && m_debugOverlays & OVERLAY_NPC_SELECTED_BIT)
		{
			Msg("%5.3f : adding %d (%d): %s : %5.3f (%.3f)\n", gpGlobals->curtime, i, m_AnimOverlay[ i ].m_nOrder.Get(), GetSequenceName( m_AnimOverlay[ i ].m_nSequence ), m_AnimOverlay[ i ].m_flCycle.Get(), m_AnimOverlay[ i ].m_flWeight.Get() );
		}
	}

	return i;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
bool CBaseAnimatingOverlay::IsValidLayer( int iLayer )
{
	return (iLayer >= 0 && iLayer < m_AnimOverlay.Count() && m_AnimOverlay[iLayer].IsActive());
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CBaseAnimatingOverlay::AllocateLayer( int iPriority )
{
	int i;

	// look for an open slot and for existing layers that are lower priority
	int iNewOrder = 0;
	int iOpenLayer = -1;
	int iNumOpen = 0;
	for (i = 0; i < m_AnimOverlay.Count(); i++)
	{
		if ( m_AnimOverlay[i].IsActive() )
		{
			if (m_AnimOverlay[i].m_nPriority <= iPriority)
			{
				iNewOrder = MAX( iNewOrder, m_AnimOverlay[i].m_nOrder + 1 );
			}
		}
		else if (m_AnimOverlay[ i ].IsDying())
		{
			// skip
		}
		else if (iOpenLayer == -1)
		{
			iOpenLayer = i;
		}
		else
		{
			iNumOpen++;
		}
	}

	if (iOpenLayer == -1)
	{
		if (m_AnimOverlay.Count() >= MAX_OVERLAYS)
		{
			return -1;
		}

		iOpenLayer = m_AnimOverlay.AddToTail();
		m_AnimOverlay[iOpenLayer].Init( this );
	}

	// make sure there's always an empty unused layer so that history slots will be available on the client when it is used
	if (iNumOpen == 0)
	{
		if (m_AnimOverlay.Count() < MAX_OVERLAYS)
		{
			i = m_AnimOverlay.AddToTail();
			m_AnimOverlay[i].Init( this );
		}
	}

	for (i = 0; i < m_AnimOverlay.Count(); i++)
	{
		if ( m_AnimOverlay[i].m_nOrder >= iNewOrder && m_AnimOverlay[i].m_nOrder < MAX_OVERLAYS)
		{
			m_AnimOverlay[i].m_nOrder++;
		}
	}

	m_AnimOverlay[iOpenLayer].m_fFlags = ANIM_LAYER_ACTIVE;
	m_AnimOverlay[iOpenLayer].m_nOrder = iNewOrder;
	m_AnimOverlay[iOpenLayer].m_nPriority = iPriority;

	m_AnimOverlay[iOpenLayer].MarkActive();
	VerifyOrder();

	return iOpenLayer;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerPriority( int iLayer, int iPriority )
{
	if (!IsValidLayer( iLayer ))
	{
		return;
	}

	if (m_AnimOverlay[iLayer].m_nPriority == iPriority)
	{
		return;
	}

	// look for an open slot and for existing layers that are lower priority
	int i;
	for (i = 0; i < m_AnimOverlay.Count(); i++)
	{
		if ( m_AnimOverlay[i].IsActive() )
		{
			if (m_AnimOverlay[i].m_nOrder > m_AnimOverlay[iLayer].m_nOrder)
			{
				m_AnimOverlay[i].m_nOrder--;
			}
		}
	}

	int iNewOrder = 0;
	for (i = 0; i < m_AnimOverlay.Count(); i++)
	{
		if ( i != iLayer && m_AnimOverlay[i].IsActive() )
		{
			if (m_AnimOverlay[i].m_nPriority <= iPriority)
			{
				iNewOrder = MAX( iNewOrder, m_AnimOverlay[i].m_nOrder + 1 );
			}
		}
	}

	for (i = 0; i < m_AnimOverlay.Count(); i++)
	{
		if ( i != iLayer && m_AnimOverlay[i].IsActive() )
		{
			if ( m_AnimOverlay[i].m_nOrder >= iNewOrder)
			{
				m_AnimOverlay[i].m_nOrder++;
			}
		}
	}

	m_AnimOverlay[iLayer].m_nOrder = iNewOrder;
	m_AnimOverlay[iLayer].m_nPriority = iPriority;
	m_AnimOverlay[iLayer].MarkActive( );

	VerifyOrder();

	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
//-----------------------------------------------------------------------------
int	CBaseAnimatingOverlay::FindGestureLayer( Activity activity )
{
	for (int i = 0; i < m_AnimOverlay.Count(); i++)
	{
		if ( !(m_AnimOverlay[i].IsActive()) )
			continue;

		if ( m_AnimOverlay[i].IsKillMe() )
			continue;

		if ( m_AnimOverlay[i].m_nActivity == ACT_INVALID )
			continue;

		if ( m_AnimOverlay[i].m_nActivity == activity )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseAnimatingOverlay::IsPlayingGesture( Activity activity )
{
	return FindGestureLayer( activity ) != -1 ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::RestartGesture( Activity activity, bool addifmissing /*=true*/, bool autokill /*=true*/ )
{
	int idx = FindGestureLayer( activity );
	if ( idx == -1 )
	{
		if ( addifmissing )
		{
			AddGesture( activity, autokill );
		}
		return;
	}

	m_AnimOverlay[ idx ].m_flCycle = 0.0f;
	m_AnimOverlay[ idx ].m_flPrevCycle = 0.0f;
	m_AnimOverlay[ idx ].m_flLastEventCheck = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::RemoveGesture( Activity activity )
{
	int iLayer = FindGestureLayer( activity );
	if ( iLayer == -1 )
		return;

	RemoveLayer( iLayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::RemoveAllGestures( void )
{
	for (int i = 0; i < m_AnimOverlay.Count(); i++)
	{
		RemoveLayer( i );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerCycle( int iLayer, float flCycle )
{
	if (!IsValidLayer( iLayer ))
		return;

	if (!m_AnimOverlay[iLayer].m_bLooping)
	{
		flCycle = clamp( flCycle, 0.0f, 1.0f );
	}
	m_AnimOverlay[iLayer].m_flCycle = flCycle;
	m_AnimOverlay[iLayer].MarkActive( );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerCycle( int iLayer, float flCycle, float flPrevCycle )
{
	if (!IsValidLayer( iLayer ))
		return;

	if (!m_AnimOverlay[iLayer].m_bLooping)
	{
		flCycle = clamp( flCycle, 0.0f, 1.0f );
		flPrevCycle = clamp( flPrevCycle, 0.0f, 1.0f );
	}
	m_AnimOverlay[iLayer].m_flCycle = flCycle;
	m_AnimOverlay[iLayer].m_flPrevCycle = flPrevCycle;
	m_AnimOverlay[iLayer].m_flLastEventCheck = flPrevCycle;
	m_AnimOverlay[iLayer].MarkActive( );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CBaseAnimatingOverlay::GetLayerCycle( int iLayer )
{
	if (!IsValidLayer( iLayer ))
		return 0.0;

	return m_AnimOverlay[iLayer].m_flCycle;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerPlaybackRate( int iLayer, float flPlaybackRate )
{
	if (!IsValidLayer( iLayer ))
		return;

	Assert( flPlaybackRate > -1.0 && flPlaybackRate < 40.0);

	m_AnimOverlay[iLayer].m_flPlaybackRate = flPlaybackRate;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerWeight( int iLayer, float flWeight )
{
	if (!IsValidLayer( iLayer ))
		return;

	flWeight = clamp( flWeight, 0.0f, 1.0f );
	m_AnimOverlay[iLayer].m_flWeight = flWeight;
	m_AnimOverlay[iLayer].MarkActive( );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CBaseAnimatingOverlay::GetLayerWeight( int iLayer )
{
	if (!IsValidLayer( iLayer ))
		return 0.0;

	return m_AnimOverlay[iLayer].m_flWeight;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerBlendIn( int iLayer, float flBlendIn )
{
	if (!IsValidLayer( iLayer ))
		return;

	m_AnimOverlay[iLayer].m_flBlendIn = flBlendIn;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerBlendOut( int iLayer, float flBlendOut )
{
	if (!IsValidLayer( iLayer ))
		return;

	m_AnimOverlay[iLayer].m_flBlendOut = flBlendOut;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerAutokill( int iLayer, bool bAutokill )
{
	if (!IsValidLayer( iLayer ))
		return;

	if (bAutokill)
	{
		m_AnimOverlay[iLayer].m_fFlags |= ANIM_LAYER_AUTOKILL;
	}
	else
	{
		m_AnimOverlay[iLayer].m_fFlags &= ~ANIM_LAYER_AUTOKILL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerLooping( int iLayer, bool bLooping )
{
	if (!IsValidLayer( iLayer ))
		return;

	m_AnimOverlay[iLayer].m_bLooping = bLooping;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::SetLayerNoRestore( int iLayer, bool bNoRestore )
{
	if (!IsValidLayer( iLayer ))
		return;

	if (bNoRestore)
	{
		m_AnimOverlay[iLayer].m_fFlags |= ANIM_LAYER_DONTRESTORE;
	}
	else
	{
		m_AnimOverlay[iLayer].m_fFlags &= ~ANIM_LAYER_DONTRESTORE;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CBaseAnimatingOverlay::GetLayerActivity( int iLayer )
{
	if (!IsValidLayer( iLayer ))
	{
		return ACT_INVALID;
	}
		
	return m_AnimOverlay[iLayer].m_nActivity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseAnimatingOverlay::GetLayerSequence( int iLayer )
{
	if (!IsValidLayer( iLayer ))
	{
		return ACT_INVALID;
	}
		
	return m_AnimOverlay[iLayer].m_nSequence;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimatingOverlay::RemoveLayer( int iLayer, float flKillRate, float flKillDelay )
{
	if (!IsValidLayer( iLayer ))
		return;

	if (flKillRate > 0)
	{
		m_AnimOverlay[iLayer].m_flKillRate = m_AnimOverlay[iLayer].m_flWeight / flKillRate;
	}
	else
	{
		m_AnimOverlay[iLayer].m_flKillRate = 100;
	}

	m_AnimOverlay[iLayer].m_flKillDelay = flKillDelay;

	m_AnimOverlay[iLayer].KillMe();
}

void CBaseAnimatingOverlay::FastRemoveLayer( int iLayer )
{
	if (!IsValidLayer( iLayer ))
		return;

	// shift the other layers down in order
	for (int j = 0; j < m_AnimOverlay.Count(); j++ )
	{
		if ((m_AnimOverlay[ j ].IsActive()) && m_AnimOverlay[ j ].m_nOrder > m_AnimOverlay[ iLayer ].m_nOrder)
		{
			m_AnimOverlay[ j ].m_nOrder--;
		}
	}
	m_AnimOverlay[ iLayer ].Init( this );

	VerifyOrder();
}

CAnimationLayer *CBaseAnimatingOverlay::GetAnimOverlay( int iIndex )
{
	iIndex = clamp( iIndex, 0, m_AnimOverlay.Count()-1 );

	return &m_AnimOverlay[iIndex];
}


void CBaseAnimatingOverlay::SetNumAnimOverlays( int num )
{
	if ( m_AnimOverlay.Count() < num )
	{
		m_AnimOverlay.AddMultipleToTail( num - m_AnimOverlay.Count() );
	}
	else if ( m_AnimOverlay.Count() > num )
	{
		m_AnimOverlay.RemoveMultiple( num, m_AnimOverlay.Count() - num );
	}
}

bool CBaseAnimatingOverlay::HasActiveLayer( void )
{
	for (int j = 0; j < m_AnimOverlay.Count(); j++ )
	{
		if ( m_AnimOverlay[ j ].IsActive() )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
