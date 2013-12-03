//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_baseanimatingoverlay.h"
#include "bone_setup.h"
#include "tier0/vprof.h"
#include "engine/ivdebugoverlay.h"
#include "datacache/imdlcache.h"
#include "eventlist.h"

#include "dt_utlvector_recv.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar r_sequence_debug;

C_BaseAnimatingOverlay::C_BaseAnimatingOverlay()
{
	// FIXME: where does this initialization go now?
	//for ( int i=0; i < MAX_OVERLAYS; i++ )
	//{
	//	memset( &m_Layer[i], 0, sizeof(m_Layer[0]) );
	//	m_Layer[i].m_nOrder = MAX_OVERLAYS;
	//}

	// FIXME: where does this initialization go now?
	// AddVar( m_Layer, &m_iv_AnimOverlay, LATCH_ANIMATION_VAR );
}

#undef CBaseAnimatingOverlay



BEGIN_RECV_TABLE_NOBASE(CAnimationLayer, DT_Animationlayer)
	RecvPropInt(	RECVINFO_NAME(m_nSequence, m_nSequence)),
	RecvPropFloat(	RECVINFO_NAME(m_flCycle, m_flCycle)),
	RecvPropFloat(	RECVINFO_NAME(m_flPrevCycle, m_flPrevCycle)),
	RecvPropFloat(	RECVINFO_NAME(m_flWeight, m_flWeight)),
	RecvPropInt(	RECVINFO_NAME(m_nOrder, m_nOrder))
END_RECV_TABLE()

const char *s_m_iv_AnimOverlayNames[C_BaseAnimatingOverlay::MAX_OVERLAYS] =
{
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay00",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay01",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay02",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay03",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay04",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay05",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay06",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay07",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay08",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay09",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay10",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay11",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay12",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay13",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay14"
};

void ResizeAnimationLayerCallback( void *pStruct, int offsetToUtlVector, int len )
{
	C_BaseAnimatingOverlay *pEnt = (C_BaseAnimatingOverlay*)pStruct;
	CUtlVector < C_AnimationLayer > *pVec = &pEnt->m_AnimOverlay;
	CUtlVector< CInterpolatedVar< C_AnimationLayer > > *pVecIV = &pEnt->m_iv_AnimOverlay;
	
	Assert( (char*)pVec - (char*)pEnt == offsetToUtlVector );
	Assert( pVec->Count() == pVecIV->Count() );
	Assert( pVec->Count() <= C_BaseAnimatingOverlay::MAX_OVERLAYS );
	
	int diff = len - pVec->Count();

	

	if ( diff == 0 )
		return;

	// remove all entries
	for ( int i=0; i < pVec->Count(); i++ )
	{
		pEnt->RemoveVar( &pVec->Element( i ) );
	}

	// adjust vector sizes
	if ( diff > 0 )
	{
		pVec->AddMultipleToTail( diff );
		pVecIV->AddMultipleToTail( diff );
	}
	else
	{
		pVec->RemoveMultiple( len, -diff );
		pVecIV->RemoveMultiple( len, -diff );
	}

	// Rebind all the variables in the ent's list.
	for ( int i=0; i < len; i++ )
	{
		IInterpolatedVar *pWatcher = &pVecIV->Element( i );
		pWatcher->SetDebugName( s_m_iv_AnimOverlayNames[i] );
		pEnt->AddVar( &pVec->Element( i ), pWatcher, LATCH_ANIMATION_VAR, true );
	}
	// FIXME: need to set historical values of nOrder in pVecIV to MAX_OVERLAY
	
}


BEGIN_RECV_TABLE_NOBASE( C_BaseAnimatingOverlay, DT_OverlayVars )
	 RecvPropUtlVector( 
		RECVINFO_UTLVECTOR_SIZEFN( m_AnimOverlay, ResizeAnimationLayerCallback ), 
		C_BaseAnimatingOverlay::MAX_OVERLAYS,
		RecvPropDataTable(NULL, 0, 0, &REFERENCE_RECV_TABLE( DT_Animationlayer ) ) )
END_RECV_TABLE()


IMPLEMENT_CLIENTCLASS_DT( C_BaseAnimatingOverlay, DT_BaseAnimatingOverlay, CBaseAnimatingOverlay )
	RecvPropDataTable( "overlay_vars", 0, 0, &REFERENCE_RECV_TABLE( DT_OverlayVars ) )
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_BaseAnimatingOverlay )

/*
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[0][2].m_nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[0][2].m_flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[0][2].m_flPlaybackRate, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[0][2].m_flWeight, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[1][2].m_nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[1][2].m_flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[1][2].m_flPlaybackRate, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[1][2].m_flWeight, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[2][2].m_nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[2][2].m_flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[2][2].m_flPlaybackRate, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[2][2].m_flWeight, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[3][2].m_nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[3][2].m_flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[3][2].m_flPlaybackRate, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[3][2].m_flWeight, FIELD_FLOAT),
*/

END_PREDICTION_DATA()

C_AnimationLayer* C_BaseAnimatingOverlay::GetAnimOverlay( int i )
{
	Assert( i >= 0 && i < MAX_OVERLAYS );
	return &m_AnimOverlay[i];
}


void C_BaseAnimatingOverlay::SetNumAnimOverlays( int num )
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


int C_BaseAnimatingOverlay::GetNumAnimOverlays() const
{
	return m_AnimOverlay.Count();
}


void C_BaseAnimatingOverlay::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	BaseClass::GetRenderBounds( theMins, theMaxs );

	if ( !IsRagdoll() )
	{
		CStudioHdr *pStudioHdr = GetModelPtr();
		if ( !pStudioHdr || !pStudioHdr->SequencesAvailable() )
			return;

		int nSequences = pStudioHdr->GetNumSeq();

		int i;
		for (i = 0; i < m_AnimOverlay.Count(); i++)
		{
			if (m_AnimOverlay[i].m_flWeight > 0.0)
			{
				if ( m_AnimOverlay[i].m_nSequence >= nSequences )
				{
					continue;
				}

				mstudioseqdesc_t &seqdesc = pStudioHdr->pSeqdesc( m_AnimOverlay[i].m_nSequence );
				VectorMin( seqdesc.bbmin, theMins, theMins );
				VectorMax( seqdesc.bbmax, theMaxs, theMaxs );
			}
		}
	}
}



void C_BaseAnimatingOverlay::CheckForLayerChanges( CStudioHdr *hdr, float currentTime )
{
	CDisableRangeChecks disableRangeChecks;

	bool bLayersChanged = false;
	
	// FIXME: damn, there has to be a better way than this.
	int i;
	for (i = 0; i < m_iv_AnimOverlay.Count(); i++)
	{
		CDisableRangeChecks disableRangeChecks; 

		int iHead, iPrev1, iPrev2;
		m_iv_AnimOverlay[i].GetInterpolationInfo( currentTime, &iHead, &iPrev1, &iPrev2 );

		// fake up previous cycle values.
		float t0;
		C_AnimationLayer *pHead = m_iv_AnimOverlay[i].GetHistoryValue( iHead, t0 );
		// reset previous
		float t1;
		C_AnimationLayer *pPrev1 = m_iv_AnimOverlay[i].GetHistoryValue( iPrev1, t1 );
		// reset previous previous
		float t2;
		C_AnimationLayer *pPrev2 = m_iv_AnimOverlay[i].GetHistoryValue( iPrev2, t2 );

		if ( pHead && pPrev1 && pHead->m_nSequence != pPrev1->m_nSequence )
		{
			bLayersChanged = true;
	#if 1 // _DEBUG
			if (/* Q_stristr( hdr->pszName(), r_sequence_debug.GetString()) != NULL || */ r_sequence_debug.GetInt() == entindex())
			{
				DevMsgRT( "(%7.4f : %30s : %5.3f : %4.2f : %1d)\n", t0, hdr->pSeqdesc( pHead->m_nSequence ).pszLabel(),  (float)pHead->m_flCycle,  (float)pHead->m_flWeight, i );
				DevMsgRT( "(%7.4f : %30s : %5.3f : %4.2f : %1d)\n", t1, hdr->pSeqdesc( pPrev1->m_nSequence ).pszLabel(),  (float)pPrev1->m_flCycle, (float)pPrev1->m_flWeight, i );
				if (pPrev2)
					DevMsgRT( "(%7.4f : %30s : %5.3f : %4.2f : %1d)\n", t2, hdr->pSeqdesc( pPrev2->m_nSequence ).pszLabel(),  (float)pPrev2->m_flCycle,  (float)pPrev2->m_flWeight, i );
			}
	#endif

			if (pPrev1)
			{
				pPrev1->m_nSequence = pHead->m_nSequence;
				pPrev1->m_flCycle = pHead->m_flPrevCycle;
				pPrev1->m_flWeight = pHead->m_flWeight;
			}

			if (pPrev2)
			{
				float num = 0;
				if ( fabs( t0 - t1 ) > 0.001f )
					num = (t2 - t1) / (t0 - t1);

				pPrev2->m_nSequence = pHead->m_nSequence;
				float flTemp;
				if (IsSequenceLooping( hdr, pHead->m_nSequence ))
				{
					flTemp = LoopingLerp( num, (float)pHead->m_flPrevCycle, (float)pHead->m_flCycle );
				}
				else
				{
					flTemp = Lerp( num, (float)pHead->m_flPrevCycle, (float)pHead->m_flCycle );
				}
				pPrev2->m_flCycle = flTemp;
				pPrev2->m_flWeight = pHead->m_flWeight;
			}

			/*
			if (stricmp( r_seq_overlay_debug.GetString(), hdr->name ) == 0)
			{
				DevMsgRT( "(%30s %6.2f : %6.2f : %6.2f)\n", hdr->pSeqdesc( pHead->nSequence ).pszLabel(), (float)pPrev2->m_flCycle, (float)pPrev1->m_flCycle, (float)pHead->m_flCycle );
			}
			*/

			m_iv_AnimOverlay[i].SetLooping( IsSequenceLooping( hdr, pHead->m_nSequence ) );
			m_iv_AnimOverlay[i].Interpolate( currentTime );

			// reset event indexes
			m_flOverlayPrevEventCycle[i] = pHead->m_flPrevCycle - 0.01;
		}
	}

	if (bLayersChanged)
	{
		// render bounds may have changed
		UpdateVisibility();
	}
}



void C_BaseAnimatingOverlay::AccumulateLayers( IBoneSetup &boneSetup, Vector pos[], Quaternion q[], float currentTime )
{
	BaseClass::AccumulateLayers( boneSetup, pos, q, currentTime );
	int i;

	// resort the layers
	int layer[MAX_OVERLAYS];
	for (i = 0; i < MAX_OVERLAYS; i++)
	{
		layer[i] = MAX_OVERLAYS;
	}
	for (i = 0; i < m_AnimOverlay.Count(); i++)
	{
		if (m_AnimOverlay[i].m_nOrder < MAX_OVERLAYS)
		{
			/*
			Assert( layer[m_AnimOverlay[i].m_nOrder] == MAX_OVERLAYS );
			layer[m_AnimOverlay[i].m_nOrder] = i;
			*/
			// hacky code until initialization of new layers is finished
			if (layer[m_AnimOverlay[i].m_nOrder] != MAX_OVERLAYS)
			{
				m_AnimOverlay[i].m_nOrder = MAX_OVERLAYS;
			}
			else
			{
				layer[m_AnimOverlay[i].m_nOrder] = i;
			}
		}
	}

	CheckForLayerChanges( boneSetup.GetStudioHdr(), currentTime );

	int nSequences = boneSetup.GetStudioHdr()->GetNumSeq();

	// add in the overlay layers
	int j;
	for (j = 0; j < MAX_OVERLAYS; j++)
	{
		i = layer[ j ];
		if (i < m_AnimOverlay.Count())
		{
			if ( m_AnimOverlay[i].m_nSequence >= nSequences )
			{
				continue;
			}

			/*
			DevMsgRT( 1 , "%.3f  %.3f  %.3f\n", currentTime, fWeight, dadt );
			debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), -j - 1, 0, 
				"%2d(%s) : %6.2f : %6.2f", 
					m_AnimOverlay[i].m_nSequence,
					hdr->pSeqdesc( m_AnimOverlay[i].m_nSequence )->pszLabel(),
					m_AnimOverlay[i].m_flCycle, 
					m_AnimOverlay[i].m_flWeight
					);
			*/

			m_AnimOverlay[i].BlendWeight();

			float fWeight = m_AnimOverlay[i].m_flWeight;

			if (fWeight > 0)
			{
				// check to see if the sequence changed
				// FIXME: move this to somewhere more reasonable
				// do a nice spline interpolation of the values
				// if ( m_AnimOverlay[i].m_nSequence != m_iv_AnimOverlay.GetPrev( i )->nSequence )
				float fCycle = m_AnimOverlay[ i ].m_flCycle;

				fCycle = ClampCycle( fCycle, IsSequenceLooping( m_AnimOverlay[i].m_nSequence ) );

				if (fWeight > 1)
					fWeight = 1;

				boneSetup.AccumulatePose( pos, q, m_AnimOverlay[i].m_nSequence, fCycle, fWeight, currentTime, m_pIk );

#if 1 // _DEBUG
				if (/* Q_stristr( hdr->pszName(), r_sequence_debug.GetString()) != NULL || */ r_sequence_debug.GetInt() == entindex())
				{
					if (1)
					{
						DevMsgRT( "%8.4f : %30s : %5.3f : %4.2f : %1d\n", currentTime, boneSetup.GetStudioHdr()->pSeqdesc( m_AnimOverlay[i].m_nSequence ).pszLabel(), fCycle, fWeight, i );
					}
					else
					{
						int iHead, iPrev1, iPrev2;
						m_iv_AnimOverlay[i].GetInterpolationInfo( currentTime, &iHead, &iPrev1, &iPrev2 );

						// fake up previous cycle values.
						float t0;
						C_AnimationLayer *pHead = m_iv_AnimOverlay[i].GetHistoryValue( iHead, t0 );
						// reset previous
						float t1;
						C_AnimationLayer *pPrev1 = m_iv_AnimOverlay[i].GetHistoryValue( iPrev1, t1 );
						// reset previous previous
						float t2;
						C_AnimationLayer *pPrev2 = m_iv_AnimOverlay[i].GetHistoryValue( iPrev2, t2 );

						if ( pHead && pPrev1 && pPrev2 )
						{
							DevMsgRT( "%6.2f : %30s %6.2f (%6.2f:%6.2f:%6.2f) : %6.2f (%6.2f:%6.2f:%6.2f) : %1d\n", currentTime, boneSetup.GetStudioHdr()->pSeqdesc( m_AnimOverlay[i].m_nSequence ).pszLabel(), 
								fCycle, (float)pPrev2->m_flCycle, (float)pPrev1->m_flCycle, (float)pHead->m_flCycle,
								fWeight, (float)pPrev2->m_flWeight, (float)pPrev1->m_flWeight, (float)pHead->m_flWeight,
								i );
						}
						else
						{
							DevMsgRT( "%6.2f : %30s %6.2f : %6.2f : %1d\n", currentTime, boneSetup.GetStudioHdr()->pSeqdesc( m_AnimOverlay[i].m_nSequence ).pszLabel(), fCycle, fWeight, i );
						}

					}
				}
#endif

//#define DEBUG_TF2_OVERLAYS
#if defined( DEBUG_TF2_OVERLAYS )
				engine->Con_NPrintf( 10 + j, "%30s %6.2f : %6.2f : %1d", boneSetup.GetStudioHdr()->pSeqdesc( m_AnimOverlay[i].m_nSequence ).pszLabel(), fCycle, fWeight, i );
			}
			else
			{
				engine->Con_NPrintf( 10 + j, "%30s %6.2f : %6.2f : %1d", "            ", 0.f, 0.f, i );
#endif
			}
		}
#if defined( DEBUG_TF2_OVERLAYS )
		else
		{
			engine->Con_NPrintf( 10 + j, "%30s %6.2f : %6.2f : %1d", "            ", 0.f, 0.f, i );
		}
#endif
	}
}

void C_BaseAnimatingOverlay::DoAnimationEvents( CStudioHdr *pStudioHdr )
{
	if ( !pStudioHdr || !pStudioHdr->SequencesAvailable() )
		return;

	MDLCACHE_CRITICAL_SECTION();

	int nSequences = pStudioHdr->GetNumSeq();

	BaseClass::DoAnimationEvents( pStudioHdr );

	bool watch = false; // Q_strstr( hdr->name, "rifle" ) ? true : false;

	CheckForLayerChanges( pStudioHdr, gpGlobals->curtime ); // !!!

	int j;
	for (j = 0; j < m_AnimOverlay.Count(); j++)
	{
		if ( m_AnimOverlay[j].m_nSequence >= nSequences )
		{
			continue;
		}

		mstudioseqdesc_t &seqdesc = pStudioHdr->pSeqdesc( m_AnimOverlay[j].m_nSequence );
		if ( seqdesc.numevents == 0 )
			continue;

		// stalled?
		if (m_AnimOverlay[j].m_flCycle == m_flOverlayPrevEventCycle[j])
			continue;

		bool bLoopingSequence = IsSequenceLooping( m_AnimOverlay[j].m_nSequence );

		bool bLooped = false;

		//in client code, m_flOverlayPrevEventCycle is set to -1 when we first start an overlay, looping or not
		if ( bLoopingSequence &&
			m_flOverlayPrevEventCycle[j] > 0.0f &&
			m_AnimOverlay[j].m_flCycle <= m_flOverlayPrevEventCycle[j] )
		{
			if (m_flOverlayPrevEventCycle[j] - m_AnimOverlay[j].m_flCycle > 0.5)
			{
				bLooped = true;
			}
			else
			{
				// things have backed up, which is bad since it'll probably result in a hitch in the animation playback
				// but, don't play events again for the same time slice
				return;
			}
		}

		mstudioevent_t *pevent = seqdesc.pEvent( 0 );

		// This makes sure events that occur at the end of a sequence occur are
		// sent before events that occur at the beginning of a sequence.
		if (bLooped)
		{
			for (int i = 0; i < (int)seqdesc.numevents; i++)
			{
				// ignore all non-client-side events
				if ( pevent[i].type & AE_TYPE_NEWEVENTSYSTEM )
				{
					if ( !( pevent[i].type & AE_TYPE_CLIENT ) )
						 continue;
				}
				else if ( pevent[i].event < 5000 ) //Adrian - Support the old event system
					continue;
			
				if ( pevent[i].cycle <= m_flOverlayPrevEventCycle[j] )
					continue;
				
				if ( watch )
				{
					Msg( "%i FE %i Looped cycle %f, prev %f ev %f (time %.3f)\n",
						gpGlobals->tickcount,
						pevent[i].event,
						pevent[i].cycle,
						m_flOverlayPrevEventCycle[j],
						(float)m_AnimOverlay[j].m_flCycle,
						gpGlobals->curtime );
				}
					
					
				FireEvent( GetAbsOrigin(), GetAbsAngles(), pevent[ i ].event, pevent[ i ].pszOptions() );
			}

			// Necessary to get the next loop working
			m_flOverlayPrevEventCycle[j] = -0.01;
		}

		for (int i = 0; i < (int)seqdesc.numevents; i++)
		{
			if ( pevent[i].type & AE_TYPE_NEWEVENTSYSTEM )
			{
				if ( !( pevent[i].type & AE_TYPE_CLIENT ) )
					 continue;
			}
			else if ( pevent[i].event < 5000 ) //Adrian - Support the old event system
				continue;

			if ( (pevent[i].cycle > m_flOverlayPrevEventCycle[j] && pevent[i].cycle <= m_AnimOverlay[j].m_flCycle) )
			{
				if ( watch )
				{
					Msg( "%i (seq: %d) FE %i Normal cycle %f, prev %f ev %f (time %.3f)\n",
						gpGlobals->tickcount,
						m_AnimOverlay[j].m_nSequence.GetRaw(),
						pevent[i].event,
						pevent[i].cycle,
						m_flOverlayPrevEventCycle[j],
						(float)m_AnimOverlay[j].m_flCycle,
						gpGlobals->curtime );
				}

				FireEvent( GetAbsOrigin(), GetAbsAngles(), pevent[ i ].event, pevent[ i ].pszOptions() );
			}
		}

		m_flOverlayPrevEventCycle[j] = m_AnimOverlay[j].m_flCycle;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *C_BaseAnimatingOverlay::OnNewModel()
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	// Clear out animation layers
	for ( int i=0; i < m_AnimOverlay.Count(); i++ )
	{
		m_AnimOverlay[i].Reset();
		m_AnimOverlay[i].m_nOrder = MAX_OVERLAYS;
	}

	return hdr;
}
