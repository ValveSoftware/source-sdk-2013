//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "ModelSoundsCache.h"
#include "studio.h"
#include "eventlist.h"
#include "scriptevent.h"

extern ISoundEmitterSystemBase *soundemitterbase;

CStudioHdr *ModelSoundsCache_LoadModel( char const *filename );
void ModelSoundsCache_PrecacheScriptSound( const char *soundname );
void ModelSoundsCache_FinishModel( CStudioHdr *hdr );
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *hdr - 
// Output : static void
//-----------------------------------------------------------------------------

void VerifySequenceIndex( CStudioHdr *pstudiohdr );

// HACK:  This must match the #define in cl_animevent.h in the client .dll code!!!
#define CL_EVENT_SOUND				5004
#define CL_EVENT_FOOTSTEP_LEFT		6004
#define CL_EVENT_FOOTSTEP_RIGHT		6005
#define CL_EVENT_MFOOTSTEP_LEFT		6006
#define CL_EVENT_MFOOTSTEP_RIGHT	6007


extern ISoundEmitterSystemBase *soundemitterbase;

CModelSoundsCache::CModelSoundsCache()
{
}

CModelSoundsCache::CModelSoundsCache( const CModelSoundsCache& src )
{
	sounds = src.sounds;
}

char const *CModelSoundsCache::GetSoundName( int index )
{
	return soundemitterbase->GetSoundName( sounds[ index ] );
}

void CModelSoundsCache::Save( CUtlBuffer& buf  )
{
	buf.PutShort( sounds.Count() );
	
	for ( int i = 0; i < sounds.Count(); ++i )
	{
		buf.PutString( GetSoundName( i ) );
	}
}

void CModelSoundsCache::Restore( CUtlBuffer& buf  )
{
	MEM_ALLOC_CREDIT();
	unsigned short c;

	c = (unsigned short)buf.GetShort();

	for ( int i = 0; i < c; ++i )
	{
		char soundname[ 512 ];

		buf.GetString( soundname );

		int idx = soundemitterbase->GetSoundIndex( soundname );
		if ( idx != -1 )
		{
			Assert( idx <= 65535 );
			if ( sounds.Find( idx ) == sounds.InvalidIndex() )
			{
				sounds.AddToTail( (unsigned short)idx );
			}
		}
	}
}

void CModelSoundsCache::Rebuild( char const *filename )
{
	sounds.RemoveAll();

	CStudioHdr *hdr = ModelSoundsCache_LoadModel( filename );

	if ( hdr )
	{
		// Precache all sounds referenced in animation events
		BuildAnimationEventSoundList( hdr, sounds );
		ModelSoundsCache_FinishModel( hdr );
	}
}

void CModelSoundsCache::PrecacheSoundList()
{
	for ( int i = 0; i < sounds.Count(); ++i )
	{
		ModelSoundsCache_PrecacheScriptSound( GetSoundName( i ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Static method
// Input  : sounds - 
//			*soundname - 
//-----------------------------------------------------------------------------
void CModelSoundsCache::FindOrAddScriptSound( CUtlVector< unsigned short >& sounds, char const *soundname )
{
	int soundindex = soundemitterbase->GetSoundIndex( soundname );
	if ( soundindex != -1 )
	{
		// Only add it once per model...
		if ( sounds.Find( soundindex ) == sounds.InvalidIndex() )
		{
			MEM_ALLOC_CREDIT();
			sounds.AddToTail( soundindex );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Static method
// Input  : *hdr - 
//			sounds - 
//-----------------------------------------------------------------------------
void CModelSoundsCache::BuildAnimationEventSoundList( CStudioHdr *hdr, CUtlVector< unsigned short >& sounds )
{
	Assert( hdr );
	
	// force animation event resolution!!!
	VerifySequenceIndex( hdr );

	// Find all animation events which fire off sound script entries...
	for ( int iSeq=0; iSeq < hdr->GetNumSeq(); iSeq++ )
	{
		mstudioseqdesc_t *pSeq = &hdr->pSeqdesc( iSeq );
		
		// Now read out all the sound events with their timing
		for ( int iEvent=0; iEvent < (int)pSeq->numevents; iEvent++ )
		{
			mstudioevent_t *pEvent = pSeq->pEvent( iEvent );
			
			switch ( pEvent->event )
			{
			default:
				{
					if ( pEvent->type & AE_TYPE_NEWEVENTSYSTEM )
					{
						if ( pEvent->event == AE_SV_PLAYSOUND )
						{
							FindOrAddScriptSound( sounds, pEvent->pszOptions() );
						}
					}
				}
				break;
			// Old-style client .dll animation event
			case CL_EVENT_SOUND:
				{
					FindOrAddScriptSound( sounds, pEvent->pszOptions() );
				}
				break;
			case CL_EVENT_FOOTSTEP_LEFT:
			case CL_EVENT_FOOTSTEP_RIGHT:
				{
					char soundname[256];
					char const *options = pEvent->pszOptions();
					if ( !options || !options[0] )
					{
						options = "NPC_CombineS";
					}

					Q_snprintf( soundname, 256, "%s.RunFootstepLeft", options );
					FindOrAddScriptSound( sounds, soundname );
					Q_snprintf( soundname, 256, "%s.RunFootstepRight", options );
					FindOrAddScriptSound( sounds, soundname );
					Q_snprintf( soundname, 256, "%s.FootstepLeft", options );
					FindOrAddScriptSound( sounds, soundname );
					Q_snprintf( soundname, 256, "%s.FootstepRight", options );
					FindOrAddScriptSound( sounds, soundname );
				}
				break;
			case AE_CL_PLAYSOUND:
				{
					if ( !( pEvent->type & AE_TYPE_CLIENT ) )
						break;

					if ( pEvent->pszOptions()[0] )
					{
						FindOrAddScriptSound( sounds, pEvent->pszOptions() );
					}
					else
					{
						Warning( "-- Error --:  empty soundname, .qc error on AE_CL_PLAYSOUND in model %s, sequence %s, animevent # %i\n", 
							hdr->pszName(), pSeq->pszLabel(), iEvent+1 );
					}
				}
				break;
			case SCRIPT_EVENT_SOUND:
				{
					FindOrAddScriptSound( sounds, pEvent->pszOptions() );
				}
				break;

			case SCRIPT_EVENT_SOUND_VOICE:
				{
					FindOrAddScriptSound( sounds, pEvent->pszOptions() );
				}
				break;
			}
		}
	}
}