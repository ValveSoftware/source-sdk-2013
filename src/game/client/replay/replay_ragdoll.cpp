//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TODO:
//    - Use a mempool
//    - Need to be able to gracefully turn replay ragdolls on/off
//
//----------------------------------------------------------------------------------------

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replay_ragdoll.h"
#include "tier1/mempool.h"
#include "debugoverlay_shared.h"
#include "filesystem.h"

//--------------------------------------------------------------------------------

static matrix3x4_t gs_BoneCache[ MAXSTUDIOBONES ];
static int gs_nBytesAllocated = 0;

//--------------------------------------------------------------------------------

void OnReplayCacheClientRagdollsCvarChange( IConVar *pVar, const char *pOldValue, float flOldValue )
{
	// TODO: need to be able to gracefully turn replay ragdolls on/off
}

//--------------------------------------------------------------------------------

static ConVar replay_ragdoll_dbg( "replay_ragdoll_dbg", "0", FCVAR_CLIENTDLL, "Display replay ragdoll debugging information." );
static ConVar replay_cache_client_ragdolls( "replay_cache_client_ragdolls", "0", FCVAR_CLIENTDLL, "Record ragdolls on the client during.", OnReplayCacheClientRagdollsCvarChange );

//--------------------------------------------------------------------------------

void DrawBones( matrix3x4_t const* pBones, int nNumBones, ragdoll_t const* pRagdoll,
			    int nRed, int nGreen, int nBlue, C_BaseAnimating* pBaseAnimating )
{
	Assert( pBones );
	Assert( pRagdoll );
	Assert( pBaseAnimating );

	Vector from, to;
	for ( int i = 0; i < nNumBones; ++i )
	{
//		debugoverlay->AddCoordFrameOverlay( pBones[ i ], 3.0f );

		int const iRagdollParentIndex = pRagdoll->list[ i ].parentIndex;
		if ( iRagdollParentIndex < 0 )
			continue;

		int iBoneIndex = pRagdoll->boneIndex[ i ];
		int iParentIndex = pRagdoll->boneIndex[ iRagdollParentIndex ];

		MatrixPosition( pBones[ iParentIndex ], from );
		MatrixPosition( pBones[ iBoneIndex   ], to );

		if ( debugoverlay )
		{
			debugoverlay->AddLineOverlay( from, to, nRed, nGreen, nBlue, true, 0.0f );
		}
	}
}

//--------------------------------------------------------------------------------

inline int GetServerTickCount()
{
	int nTick = TIME_TO_TICKS( engine->GetLastTimeStamp() );
	return nTick;
}

//--------------------------------------------------------------------------------

/*static*/ RagdollSimulationFrame_t* RagdollSimulationFrame_t::Alloc( int nNumBones )
{
	// TODO: use a mempool
	RagdollSimulationFrame_t* pNew = new RagdollSimulationFrame_t();
	pNew->pPositions = new Vector[ nNumBones ];
	pNew->pAngles = new QAngle[ nNumBones ];
	gs_nBytesAllocated += sizeof( pNew ) + nNumBones * ( sizeof( Vector ) + sizeof( QAngle ) );
	return pNew;
}

//--------------------------------------------------------------------------------

RagdollSimulationData_t::RagdollSimulationData_t( C_BaseAnimating* pEntity, int nStartTick, int nNumBones )
:	m_pEntity( pEntity ),
	m_nEntityIndex( -1 ),
	m_nStartTick( nStartTick ),
	m_nNumBones( nNumBones ),
	m_nDuration( -1 )
{
	if ( pEntity )
	{
		m_nEntityIndex = pEntity->entindex();
	}

	Assert( nNumBones >= 0 && nNumBones < MAXSTUDIOBONES );
}

bool _ComputeRagdollBones( const ragdoll_t *pRagdoll, matrix3x4_t &parentTransform, matrix3x4_t *pBones, Vector *pPositions, QAngle *pAngles )
{
	matrix3x4_t inverted, output;

#ifdef _DEBUG
	CBitVec<MAXSTUDIOBONES> vBonesComputed;
	vBonesComputed.ClearAll();
#endif

	for ( int i = 0; i < pRagdoll->listCount; ++i )
	{
		const ragdollelement_t& element = pRagdoll->list[ i ];

		// during restore if a model has changed since the file was saved, this could be NULL
		if ( !element.pObject )
			return false;

		int const boneIndex = pRagdoll->boneIndex[ i ];
		if ( boneIndex < 0 )
		{
			AssertMsg( 0, "Replay: No mapping for ragdoll bone\n" );
			return false;
		}

		// Get global transform and put it into the bone cache
		element.pObject->GetPositionMatrix( &pBones[ boneIndex ] );

		// Ensure a fixed translation from the parent (no stretching)
		if ( element.parentIndex >= 0 && !pRagdoll->allowStretch )
		{
			int parentIndex = pRagdoll->boneIndex[ element.parentIndex ];

#ifdef _DEBUG
			// Make sure we computed the parent already
			Assert( vBonesComputed.IsBitSet(parentIndex) );
#endif

			// overwrite the position from physics to force rigid attachment
			// NOTE: On the client we actually override this with the proper parent bone in each LOD
			Vector out;
			VectorTransform( element.originParentSpace, pBones[ parentIndex ], out );
			MatrixSetColumn( out, 3, pBones[ boneIndex ] );

			MatrixInvert( pBones[ parentIndex ], inverted );
		}
		else if ( element.parentIndex == - 1 )
		{
			// Decompose into parent space
			MatrixInvert( parentTransform, inverted );
		}

#ifdef _DEBUG
		vBonesComputed.Set( boneIndex, true );
#endif

		// Compute local transform and put into 'output'
 		ConcatTransforms( inverted, pBones[ boneIndex ], output );

		// Cache as Euler/position
 		MatrixAngles( output, pAngles[ i ], pPositions[ i ] );
	}
	return true;
}

void RagdollSimulationData_t::Record()
{
	Assert( m_pEntity->m_pRagdoll );

	// Allocate a frame
	RagdollSimulationFrame_t* pNewFrame = RagdollSimulationFrame_t::Alloc( m_nNumBones );
	if ( !pNewFrame )
		return;

	// Set the current tick
	pNewFrame->nTick = GetServerTickCount();

	// Add new frame to list of frames
	m_lstFrames.AddToTail( pNewFrame );

	// Compute parent transform
	matrix3x4_t parentTransform;
	Vector vRootPosition = m_pEntity->GetRenderOrigin();
	QAngle angRootAngles = m_pEntity->GetRenderAngles();
	AngleMatrix( angRootAngles, vRootPosition, parentTransform );

//	debugoverlay->AddCoordFrameOverlay( parentTransform, 100 );

	// Cache off root position/orientation
	pNewFrame->vRootPosition = vRootPosition;
	pNewFrame->angRootAngles = angRootAngles;

	// Compute actual ragdoll bones
	matrix3x4_t* pBones = gs_BoneCache;
	_ComputeRagdollBones( m_pEntity->m_pRagdoll->GetRagdoll(), parentTransform, pBones, pNewFrame->pPositions, pNewFrame->pAngles );

	// Draw bones
	if ( replay_ragdoll_dbg.GetBool() )
	{
		DrawBones( pBones, m_pEntity->m_pRagdoll->RagdollBoneCount(), m_pEntity->m_pRagdoll->GetRagdoll(), 255, 0, 0, m_pEntity );
	}
}

//--------------------------------------------------------------------------------

CReplayRagdollRecorder::CReplayRagdollRecorder()
:	m_bIsRecording(false)
{
}

CReplayRagdollRecorder::~CReplayRagdollRecorder()
{
}

/*static*/ CReplayRagdollRecorder& CReplayRagdollRecorder::Instance()
{
	static CReplayRagdollRecorder s_instance;
	return s_instance;
}

void CReplayRagdollRecorder::Init()
{
	Assert( !m_bIsRecording );
	m_bIsRecording = true;
	gs_nBytesAllocated = 0;
}

void CReplayRagdollRecorder::Shutdown()
{
	if ( !m_bIsRecording )
		return;

	m_lstRagdolls.PurgeAndDeleteElements();
	gs_nBytesAllocated = 0;

	// RemoveAll() purges, and there is no UnlinkAll() - is there an easier way to do this?
	Iterator_t i = m_lstRagdollsToRecord.Head();
	while ( i != m_lstRagdollsToRecord.InvalidIndex() )
	{
		m_lstRagdollsToRecord.Unlink( i );
		i = m_lstRagdollsToRecord.Head();
	}

	Assert( m_bIsRecording );
	m_bIsRecording = false;
}

void CReplayRagdollRecorder::AddEntry( C_BaseAnimating* pEntity, int nStartTick, int nNumBones )
{
	DevMsg( "Replay: Processing Ragdoll at time %d\n", nStartTick );

	Assert( pEntity );
	RagdollSimulationData_t* pNewEntry = new RagdollSimulationData_t( pEntity, nStartTick, nNumBones );
	gs_nBytesAllocated += sizeof( RagdollSimulationData_t );
	m_lstRagdolls.AddToTail( pNewEntry );

	// Also add to list of ragdolls to record
	m_lstRagdollsToRecord.AddToTail( pNewEntry );
}

void CReplayRagdollRecorder::StopRecordingRagdoll( C_BaseAnimating* pEntity )
{
	Assert( pEntity );

	// Find the entry in the recording list
	Iterator_t nIndex;
	if ( !FindEntryInRecordingList( pEntity, nIndex ) )
		return;

	StopRecordingRagdollAtIndex( nIndex );
}

void CReplayRagdollRecorder::StopRecordingRagdollAtIndex( Iterator_t nIndex )
{
	// No longer recording - compute duration
	RagdollSimulationData_t* pData = m_lstRagdollsToRecord[ nIndex ];

	// Does duration need to be set?
	if ( pData->m_nDuration < 0 )
	{
		pData->m_nDuration = GetServerTickCount() - pData->m_nStartTick;		Assert( pData->m_nDuration > 0 );
	}

	// Remove it from the recording list
	m_lstRagdollsToRecord.Unlink( nIndex );
}

void CReplayRagdollRecorder::StopRecordingSleepingRagdolls()
{
	Iterator_t i = m_lstRagdollsToRecord.Head();
	while ( i != m_lstRagdollsToRecord.InvalidIndex() )
	{
		if ( RagdollIsAsleep( *m_lstRagdollsToRecord[ i ]->m_pEntity->m_pRagdoll->GetRagdoll() ) )
		{
			DevMsg( "entity %d: Removing sleeping ragdoll\n", m_lstRagdollsToRecord[ i ]->m_nEntityIndex );

			StopRecordingRagdollAtIndex( i );
			i = m_lstRagdollsToRecord.Head();
		}
		else
		{
			i = m_lstRagdollsToRecord.Next( i );
		}
	}
}

bool CReplayRagdollRecorder::FindEntryInRecordingList( C_BaseAnimating* pEntity,
													   CReplayRagdollRecorder::Iterator_t& nOutIndex )
{
	// Find the entry
	FOR_EACH_LL( m_lstRagdollsToRecord, i )
	{
		if ( m_lstRagdollsToRecord[ i ]->m_pEntity == pEntity )
		{
			nOutIndex = i;
			return true;
		}
	}

	nOutIndex = m_lstRagdollsToRecord.InvalidIndex();
	return false;
}

void CReplayRagdollRecorder::Record()
{
	static ConVar* pReplayEnable = NULL;
	static bool bLookedForConvar = false;
	if ( bLookedForConvar )
	{
		pReplayEnable =  (ConVar*)cvar->FindVar( "replay_enable" );
		bLookedForConvar = true;
	}
	if ( !pReplayEnable || !pReplayEnable->GetInt() )
		return;

	if ( !replay_cache_client_ragdolls.GetInt() )
		return;

	FOR_EACH_LL( m_lstRagdollsToRecord, i )
	{
		Assert( m_lstRagdollsToRecord[ i ]->m_pEntity->IsRagdoll() );
		m_lstRagdollsToRecord[ i ]->Record();
	}
}

void CReplayRagdollRecorder::Think()
{
	if ( !IsRecording() )
		return;

	StopRecordingSleepingRagdolls();
	Record();

	PrintDebug();
}

void CReplayRagdollRecorder::PrintDebug()
{
	if ( !replay_ragdoll_dbg.GetInt() )
		return;

	int nLine = 0;

	// Print memory usage
	engine->Con_NPrintf( nLine++, "ragdolls: %.2f MB", gs_nBytesAllocated / 1048576.0f );

	// Print server time
	engine->Con_NPrintf( nLine++, "server time: %d", GetServerTickCount() );

	++nLine;  // Blank line

	// Print info about each ragdoll
	FOR_EACH_LL( m_lstRagdolls, i )
	{
		engine->Con_NPrintf( nLine++, "entity %d: start time=%d  duration=%d  num bones=%d", m_lstRagdolls[i]->m_nEntityIndex, m_lstRagdolls[i]->m_nStartTick, m_lstRagdolls[i]->m_nDuration, m_lstRagdolls[i]->m_nNumBones );
	}
}

void CReplayRagdollRecorder::CleanupStartupTicksAndDurations( int nStartTick )
{
	FOR_EACH_LL( m_lstRagdolls, i )
	{
		RagdollSimulationData_t* pRagdollData = m_lstRagdolls[ i ];

		// Offset start tick with start tick, sent over from server
		pRagdollData->m_nStartTick -= nStartTick;			Assert( pRagdollData->m_nStartTick >= 0 );

		// Setup duration
		pRagdollData->m_nDuration = GetServerTickCount() - nStartTick;		Assert( pRagdollData->m_nDuration > 0 );

		// Go through all frames and subtract the start tick
		FOR_EACH_LL( pRagdollData->m_lstFrames, j )
		{
			pRagdollData->m_lstFrames[ j ]->nTick -= nStartTick;
		}
	}
}

BEGIN_DMXELEMENT_UNPACK( RagdollSimulationData_t )
	DMXELEMENT_UNPACK_FIELD( "nEntityIndex", "0", int, m_nEntityIndex )
	DMXELEMENT_UNPACK_FIELD( "nStartTick", "0", int, m_nStartTick )
	DMXELEMENT_UNPACK_FIELD( "nDuration", "0", int, m_nDuration )
	DMXELEMENT_UNPACK_FIELD( "nNumBones", "0", int, m_nNumBones )
END_DMXELEMENT_UNPACK( RagdollSimulationData_t, s_RagdollSimulationDataUnpack )

bool CReplayRagdollRecorder::DumpRagdollsToDisk( char const* pFilename ) const
{
	MEM_ALLOC_CREDIT();
	DECLARE_DMX_CONTEXT();

	CDmxElement* pSimulations = CreateDmxElement( "Simulations" );
	CDmxElementModifyScope modify( pSimulations );

		int const nNumRagdolls = m_lstRagdolls.Count();

		pSimulations->SetValue( "iNumRagdolls", nNumRagdolls );

		CDmxAttribute* pRagdolls = pSimulations->AddAttribute( "ragdolls" );
		CUtlVector< CDmxElement* >& ragdolls = pRagdolls->GetArrayForEdit< CDmxElement* >();

	modify.Release();

	char name[32];

	FOR_EACH_LL( m_lstRagdolls, i )
	{
		RagdollSimulationData_t const* pData = m_lstRagdolls[ i ];

		// Make sure we've setup all durations properly
		Assert( pData->m_nDuration >= 0 );

		CDmxElement* pRagdoll = CreateDmxElement( "ragdoll" );
		ragdolls.AddToTail( pRagdoll );

		V_snprintf( name, sizeof(name), "ragdoll %lld", i );
		pRagdoll->SetValue( "name", name );

		CDmxElementModifyScope modifyClass( pRagdoll );

		pRagdoll->AddAttributesFromStructure( pData, s_RagdollSimulationDataUnpack );

		CDmxAttribute* pFrames = pRagdoll->AddAttribute( "frames" );
		CUtlVector< CDmxElement* >& frames = pFrames->GetArrayForEdit< CDmxElement* >();

		FOR_EACH_LL( pData->m_lstFrames, j )
		{
			CDmxElement* pFrame = CreateDmxElement( "frame" );
			frames.AddToTail( pFrame );

			V_snprintf( name, sizeof(name), "frame %lld", j );
			pFrame->SetValue( "name", name );

			// Store tick
			pFrame->SetValue( "tick", pData->m_lstFrames[ j ]->nTick );

			// Store root pos/orientation
			pFrame->SetValue( "root_pos"   , pData->m_lstFrames[ j ]->vRootPosition );
			pFrame->SetValue( "root_angles", pData->m_lstFrames[ j ]->angRootAngles );

			for ( int k = 0; k < pData->m_nNumBones; ++k )
			{
				CDmxAttribute* pPositions = pFrame->AddAttribute( "positions" );
				CUtlVector< Vector >& positions = pPositions->GetArrayForEdit< Vector >();

				CDmxAttribute* pAngles = pFrame->AddAttribute( "angles" );
				CUtlVector< QAngle >& angles = pAngles->GetArrayForEdit< QAngle >();

				positions.AddToTail( pData->m_lstFrames[ j ]->pPositions[ k ] );
				angles.AddToTail( pData->m_lstFrames[ j ]->pAngles[ k ] );
			}
		}
	}

	{
		MEM_ALLOC_CREDIT();
		CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
		if ( !SerializeDMX( buf, pSimulations, pFilename ) )
		{
			Warning( "Replay: Failed to write ragdoll cache, %s.\n", pFilename );
			return false;
		}

		// Write the file
		filesystem->WriteFile( pFilename, "MOD", buf );
	}

	CleanupDMX( pSimulations );

	Msg( "Replay: Cached ragdoll data.\n" );

	return true;
}

//--------------------------------------------------------------------------------

CReplayRagdollCache::CReplayRagdollCache()
:	m_bInit( false )
{
}

/*static*/ CReplayRagdollCache& CReplayRagdollCache::Instance()
{
	static CReplayRagdollCache s_instance;
	return s_instance;
}

bool CReplayRagdollCache::Init( char const* pFilename )
{
	Assert( !m_bInit );

	// Make sure valid filename
	if ( !pFilename || pFilename[0] == 0 )
		return false;

	DECLARE_DMX_CONTEXT();

	// Attempt to read from disk
	CDmxElement* pRagdolls = NULL;
	if ( !UnserializeDMX( pFilename, "MOD", true, &pRagdolls ) )
//	if ( !UnserializeDMX( pFilename, "GAME", false, &pRagdolls ) )
		return false;

	CUtlVector< CDmxElement* > const& ragdolls = pRagdolls->GetArray< CDmxElement* >( "ragdolls" );
	for ( int i = 0; i < ragdolls.Count(); ++i )
	{
		CDmxElement* pCurRagdollInput = ragdolls[ i ];

		// Create a new ragdoll entry and add to list
		RagdollSimulationData_t* pNewSimData = new RagdollSimulationData_t();
		m_lstRagdolls.AddToTail( pNewSimData );

		// Read
		pCurRagdollInput->UnpackIntoStructure( pNewSimData, sizeof( *pNewSimData ), s_RagdollSimulationDataUnpack );

		// NOTE: Entity ptr doesn't get linked up here because it doesn't necessarily exist at this point

		// Read frames
		CUtlVector< CDmxElement* > const& frames = pCurRagdollInput->GetArray< CDmxElement* >( "frames" );
		for ( int j = 0; j < frames.Count(); ++j )
		{
			CDmxElement* pCurFrameInput = frames[ j ];

			// Create a new frame and add it to list of frames
			RagdollSimulationFrame_t* pNewFrame = RagdollSimulationFrame_t::Alloc( pNewSimData->m_nNumBones );
			pNewSimData->m_lstFrames.AddToTail( pNewFrame );

			// Read tick
			pNewFrame->nTick = pCurFrameInput->GetValue( "tick", -1 );			Assert( pNewFrame->nTick != -1 );

			// Read root pos/orientation
			pNewFrame->vRootPosition = pCurFrameInput->GetValue( "root_pos"   , vec3_origin );
			pNewFrame->angRootAngles = pCurFrameInput->GetValue( "root_angles", vec3_angle );

			CUtlVector< Vector > const& positions = pCurFrameInput->GetArray< Vector >( "positions" );
			CUtlVector< QAngle > const& angles = pCurFrameInput->GetArray< QAngle >( "angles" );

			for ( int k = 0; k < pNewSimData->m_nNumBones; ++k )
			{
				pNewFrame->pPositions[ k ] = positions[ k ];
				pNewFrame->pAngles[ k ]    = angles[ k ];
			}
		}
	}


	// Cleanup
	CleanupDMX( pRagdolls );

	m_bInit = true;

	return true;
}

void CReplayRagdollCache::Shutdown()
{
	if ( !m_bInit )
		return;

	m_lstRagdolls.PurgeAndDeleteElements();
	m_bInit = false;
}

ConVar replay_ragdoll_blending( "replay_ragdoll_blending", "1", FCVAR_DEVELOPMENTONLY );
ConVar replay_ragdoll_tickoffset( "replay_ragdoll_tickoffset", "0", FCVAR_DEVELOPMENTONLY );

bool CReplayRagdollCache::GetFrame( C_BaseAnimating* pEntity, int nTick, bool* pBoneSimulated, CBoneAccessor* pBoneAccessor ) const
{
	nTick += replay_ragdoll_tickoffset.GetInt();

	Assert( pEntity );
	Assert( pBoneSimulated );
	Assert( pEntity->m_pRagdoll );

	// Find ragdoll for the given entity - will return NULL if nTick is out of the entry's time window
	const RagdollSimulationData_t* pRagdollEntry = FindRagdollEntry( pEntity, nTick );
	if ( !pRagdollEntry )
		return false;

	// Find frame for the given tick
	RagdollSimulationFrame_t* pFrame;
	RagdollSimulationFrame_t* pNextFrame;
	if ( !FindFrame( pFrame, pNextFrame, pRagdollEntry, nTick ) )
		return false;

	// Compute root transform
	matrix3x4_t rootTransform;
	float flInterpAmount = gpGlobals->interpolation_amount;
	if ( pNextFrame )
	{
		AngleMatrix(
			(const QAngle &)Lerp( flInterpAmount, pFrame->angRootAngles, pNextFrame->angRootAngles ),	// Actually does a slerp
			Lerp( flInterpAmount, pFrame->vRootPosition, pNextFrame->vRootPosition ),
			rootTransform
		);
	}
	else
	{
		AngleMatrix( pFrame->angRootAngles, pFrame->vRootPosition, rootTransform );
	}

	// Compute each bone
	ragdoll_t* pRagdoll = pEntity->m_pRagdoll->GetRagdoll();		Assert( pRagdoll );
	for ( int k = 0; k < pRagdoll->listCount; ++k )
	{
		int objectIndex = k;
		const ragdollelement_t& element = pRagdoll->list[ objectIndex ];

		int const boneIndex = pRagdoll->boneIndex[ objectIndex ];			Assert( boneIndex >= 0 );

		// Compute blended transform if possible
		matrix3x4_t localTransform;
		if ( pNextFrame && replay_ragdoll_blending.GetInt() )
		{
			// Get blended Eular angles - NOTE: The Lerp() here actually calls Lerp<QAngle>() which converts to quats and back
			flInterpAmount = gpGlobals->interpolation_amount;		Assert( flInterpAmount >= 0.0f && flInterpAmount <= 1.0f );
			AngleMatrix(
				(const QAngle &)Lerp( flInterpAmount, pFrame->pAngles   [ objectIndex ], pNextFrame->pAngles   [ objectIndex ] ), 
				Lerp( flInterpAmount, pFrame->pPositions[ objectIndex ], pNextFrame->pPositions[ objectIndex ] ),
				localTransform
			);
		}
		else
		{
			// Last frame
			AngleMatrix( pFrame->pAngles[ objectIndex ], pFrame->pPositions[ objectIndex ], localTransform );
		}

		matrix3x4_t& boneMatrix = pBoneAccessor->GetBoneForWrite( boneIndex );

		if ( element.parentIndex < 0 )
		{
			ConcatTransforms( rootTransform, localTransform, boneMatrix );
		}
		else
		{
			int parentBoneIndex = pRagdoll->boneIndex[ element.parentIndex ];		Assert( parentBoneIndex >= 0 );
			Assert( pBoneSimulated[ parentBoneIndex ] );
			matrix3x4_t const& parentMatrix = pBoneAccessor->GetBone( parentBoneIndex );
			ConcatTransforms( parentMatrix, localTransform, boneMatrix );
		}

		// Simulated this bone
		pBoneSimulated[ boneIndex ] = true;
	}

	if ( replay_ragdoll_dbg.GetBool() )
	{
		DrawBones( pBoneAccessor->GetBoneArrayForWrite(), pRagdollEntry->m_nNumBones, pRagdoll, 0, 0, 255, pEntity );
	}

	return true;
}

RagdollSimulationData_t* CReplayRagdollCache::FindRagdollEntry( C_BaseAnimating* pEntity, int nTick )
{
	Assert( pEntity );

	int const nEntIndex = pEntity->entindex();

	FOR_EACH_LL( m_lstRagdolls, i )
	{
		RagdollSimulationData_t* pRagdollData = m_lstRagdolls[ i ];

		// If not the right entity or the tick is out range, continue.
		if ( pRagdollData->m_nEntityIndex != nEntIndex )
			continue;

		// We've got the ragdoll, but only return it if nTick is in the window
		if ( nTick < pRagdollData->m_nStartTick ||
			 nTick > pRagdollData->m_nStartTick + pRagdollData->m_nDuration )
			return NULL;

		return pRagdollData;
	}

	return NULL;
}

bool CReplayRagdollCache::FindFrame( RagdollSimulationFrame_t*& pFrameOut, RagdollSimulationFrame_t*& pNextFrameOut,
									 const RagdollSimulationData_t* pRagdollEntry, int nTick )
{
	// Look for the appropriate frame
	FOR_EACH_LL( pRagdollEntry->m_lstFrames, j )
	{
		RagdollSimulationFrame_t* pFrame = pRagdollEntry->m_lstFrames[ j ];

		// Get next frame if possible
		int const nNext = pRagdollEntry->m_lstFrames.Next( j );
		RagdollSimulationFrame_t* pNextFrame =
			nNext == pRagdollEntry->m_lstFrames.InvalidIndex() ? NULL : pRagdollEntry->m_lstFrames[ nNext ];

		// Use this frame?
		if ( nTick >= pFrame->nTick &&
			( (pNextFrame && nTick <= pNextFrame->nTick) || !pNextFrame ) )		// Use the last frame if the tick is past the range of frames -
		{																		// this is the "sleeping" ragdoll frame
			pFrameOut     = pFrame;
			pNextFrameOut = pNextFrame;

			return true;
		}
	}

	pFrameOut     = NULL;
	pNextFrameOut = NULL;

	return false;
}

void CReplayRagdollCache::Think()
{
	// TODO: Add IsPlayingReplayDemo() to engine interface
	/*
	engine->Con_NPrintf( 8, "time: %d", engine->GetDemoPlaybackTick() );
	FOR_EACH_LL( m_lstRagdolls, i )
	{
		engine->Con_NPrintf( 10 + i, "entity %d: start time=%d  duration=%d  num bones=%d", m_lstRagdolls[i]->m_nEntityIndex, m_lstRagdolls[i]->m_nStartTick, m_lstRagdolls[i]->m_nDuration, m_lstRagdolls[i]->m_nNumBones );
	}
	*/
}

//--------------------------------------------------------------------------------

bool Replay_CacheRagdolls( const char* pFilename, int nStartTick )
{
	CReplayRagdollRecorder::Instance().CleanupStartupTicksAndDurations( nStartTick );
	return CReplayRagdollRecorder::Instance().DumpRagdollsToDisk( pFilename );
}

#endif
