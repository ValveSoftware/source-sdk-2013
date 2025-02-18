//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "tier0/dbg.h"
#include "tier0/platform.h"
#include "mathlib/mathlib.h"
#include "tier0/tslist.h"
#include "tier1/utlmap.h"
#include "tier1/convar.h"

#include "bone_setup.h"

#include "con_nprint.h"
#include "cdll_int.h"
#include "globalvars_base.h"

#include "posedebugger.h"

#include "iclientnetworkable.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern IVEngineClient *engine;
extern CGlobalVarsBase *gpGlobals;

static ConVar ui_posedebug_fade_in_time( "ui_posedebug_fade_in_time", "0.2",
										 FCVAR_CHEAT | FCVAR_DONTRECORD,
										 "Time during which a new pose activity layer is shown in green in +posedebug UI" );
static ConVar ui_posedebug_fade_out_time( "ui_posedebug_fade_out_time", "0.8",
										  FCVAR_CHEAT | FCVAR_DONTRECORD,
										  "Time to keep a no longer active pose activity layer in red until removing it from +posedebug UI" );


//////////////////////////////////////////////////////////////////////////
//
// CPoseDebuggerStub : IPoseDebugger
// empty interface implementation
//
//////////////////////////////////////////////////////////////////////////

class CPoseDebuggerStub : public IPoseDebugger
{
public:
	virtual void StartBlending( IClientNetworkable *pEntity, const CStudioHdr *pStudioHdr ) { }
	virtual void AccumulatePose(
		const CStudioHdr *pStudioHdr,
		CIKContext *pIKContext,
		Vector pos[], 
		Quaternion q[], 
		int sequence, 
		float cycle,
		const float poseParameter[],
		int boneMask,
		float flWeight,
		float flTime
		) { }
};

static CPoseDebuggerStub s_PoseDebuggerStub;
IPoseDebugger *g_pPoseDebugger = &s_PoseDebuggerStub;

//////////////////////////////////////////////////////////////////////////
//
// CPoseDebuggerImpl : IPoseDebugger
// Purpose: Main implementation of the pose debugger
// Declaration
//
//////////////////////////////////////////////////////////////////////////

class ModelPoseDebugInfo
{
public:
	ModelPoseDebugInfo() : m_iEntNum( 0 ), m_iCurrentText( 0 ) { }


public:
	// Entity number
	int m_iEntNum;

	// Currently processed text
	int m_iCurrentText;

	// Info Text Flags
	enum InfoTextFlags
	{
		F_SEEN_THIS_FRAME = 1 << 0,
		F_SEEN_LAST_FRAME = 1 << 1,
	};

	struct InfoText
	{
		InfoText() { memset( this, 0, sizeof( *this ) ); }

		// Flags
		uint32 m_uiFlags;

		// Time seen
		float m_flTimeToLive, m_flTimeAlive;

		// Activity/label
		int m_iActivity;
		char m_chActivity[100];
		char m_chLabel[100];

		// Text
		char m_chTextLines[4][256];
		enum
		{
			MAX_TEXT_LINES = 4
		};
	};
	
	CCopyableUtlVector< InfoText > m_arrTxt;


public:
	// Add an info text
	void AddInfoText( InfoText *x, ModelPoseDebugInfo *pOld );

	// Lookup an info text
	InfoText *LookupInfoText( InfoText *x );

	// Print pending info text
	void PrintPendingInfoText( int &rnPosPrint );
};

void ModelPoseDebugInfo::AddInfoText( InfoText *x, ModelPoseDebugInfo *pOld )
{
	if ( x )
	{
		// Try to set the proper flags on the info text
		x->m_uiFlags &= ~F_SEEN_LAST_FRAME;
		x->m_uiFlags |= F_SEEN_THIS_FRAME;
	}

	// If we have smth to compare against
	if ( pOld )
	{
		// Search for the same activity/label in the other model pose debug info
		ModelPoseDebugInfo &o = *pOld;
		int k = o.m_iCurrentText;
		if ( x )
		{
			for ( ; k < o.m_arrTxt.Count(); ++ k )
			{
				InfoText &txt = o.m_arrTxt[k];
				if ( ( txt.m_uiFlags & F_SEEN_THIS_FRAME ) &&
					!stricmp( x->m_chActivity, txt.m_chActivity ) &&
					!stricmp( x->m_chLabel, txt.m_chLabel ) &&
					( x->m_iActivity == txt.m_iActivity ) )
				{
					x->m_flTimeAlive = txt.m_flTimeAlive;
					break;
				}
			}
		}
		else
		{
			k = o.m_arrTxt.Count();
		}

		// Range of finished activities
		int iFinishedRange[2] = { o.m_iCurrentText, k };

		// Check whether this is a new message
		if ( k == o.m_arrTxt.Count() )
		{
			if ( !x )
			{
				o.m_iCurrentText = k;
			}
			else
			{
				// Don't update the current when insertion happens and don't have finished commands
				iFinishedRange[1] = iFinishedRange[0];
			}
		}
		else
		{
			o.m_iCurrentText = k + 1;
			if ( x )
			{
				x->m_uiFlags |= F_SEEN_LAST_FRAME;
				x->m_flTimeAlive += gpGlobals->frametime;
			}
		}

		// Everything before finished
		for ( int iFinished = iFinishedRange[0]; iFinished < iFinishedRange[1]; ++ iFinished )
		{
			InfoText &txtFinished = o.m_arrTxt[ iFinished ];

			if ( txtFinished.m_uiFlags & F_SEEN_THIS_FRAME )
				txtFinished.m_uiFlags |= F_SEEN_LAST_FRAME;

			txtFinished.m_uiFlags &= ~F_SEEN_THIS_FRAME;

			txtFinished.m_flTimeToLive -= gpGlobals->frametime;
			txtFinished.m_flTimeAlive += gpGlobals->frametime;

			if ( txtFinished.m_flTimeToLive >= 0.0f )
				m_arrTxt.AddToTail( txtFinished );
		}
	}

	if ( x )
	{
		// Now add it to the array
		x->m_flTimeToLive = ui_posedebug_fade_out_time.GetFloat();
		m_arrTxt.AddToTail( *x );
	}
}

ModelPoseDebugInfo::InfoText * ModelPoseDebugInfo::LookupInfoText( InfoText *x )
{
	int k = m_iCurrentText;
	if ( x )
	{
		for ( ; k < m_arrTxt.Count(); ++ k )
		{
			InfoText &txt = m_arrTxt[k];
			if ( ( txt.m_uiFlags & F_SEEN_THIS_FRAME ) &&
				!stricmp( x->m_chActivity, txt.m_chActivity ) &&
				!stricmp( x->m_chLabel, txt.m_chLabel ) &&
				( x->m_iActivity == txt.m_iActivity ) )
			{
				return &txt;
			}
		}
	}
	return NULL;
}

void ModelPoseDebugInfo::PrintPendingInfoText( int &rnPosPrint )
{
	con_nprint_s nxPrn = { 0 };
	nxPrn.time_to_live = -1;
	nxPrn.color[0] = 1.0f, nxPrn.color[1] = 1.0f, nxPrn.color[2] = 1.0f;
	nxPrn.fixed_width_font = true;

	float const flFadeInTime = ui_posedebug_fade_in_time.GetFloat();
	float const flFadeOutTime = ui_posedebug_fade_out_time.GetFloat();

	// Now print all the accumulated spew
	for ( int k = m_iCurrentText; k < m_arrTxt.Count(); ++ k )
	{
		InfoText &prntxt = m_arrTxt[k];

		switch( prntxt.m_uiFlags & ( F_SEEN_LAST_FRAME | F_SEEN_THIS_FRAME ) )
		{
		case ( F_SEEN_LAST_FRAME | F_SEEN_THIS_FRAME ) :
			nxPrn.color[0] = 1.f;
			nxPrn.color[1] = 1.f;
			nxPrn.color[2] = 1.f;
			if ( prntxt.m_flTimeAlive > flFadeInTime )
				break;
			else
				NULL; // Fall-through to keep showing in green
		case F_SEEN_THIS_FRAME :
			if ( flFadeInTime > 0.f )
			{
				nxPrn.color[0] = 1.f * prntxt.m_flTimeAlive / flFadeInTime;
				nxPrn.color[1] = 1.f;
				nxPrn.color[2] = 1.f * prntxt.m_flTimeAlive / flFadeInTime;
			}
			else
			{
				nxPrn.color[0] = ( prntxt.m_flTimeAlive > 0.0f ) ? 1.f : 0.f;
				nxPrn.color[1] = 1.f;
				nxPrn.color[2] = ( prntxt.m_flTimeAlive > 0.0f ) ? 1.f : 0.f;
			}
			break;
		case F_SEEN_LAST_FRAME :
		case 0:
			if ( flFadeOutTime > 0.f )
				nxPrn.color[0] = 1.f * prntxt.m_flTimeToLive / flFadeOutTime;
			else
				nxPrn.color[0] = ( prntxt.m_flTimeToLive > 0.0f ) ? 1.f : 0.f;
			nxPrn.color[1] = 0.f;
			nxPrn.color[2] = 0.f;
			break;
		}

		nxPrn.index = ( rnPosPrint += 1 );
		engine->Con_NXPrintf( &nxPrn, "%s", prntxt.m_chTextLines[0] );

		for ( int iLine = 1; iLine < ModelPoseDebugInfo::InfoText::MAX_TEXT_LINES; ++ iLine)
		{
			if ( !prntxt.m_chTextLines[iLine][0] )
				break;

			nxPrn.index = ( rnPosPrint += 1 );
			engine->Con_NXPrintf( &nxPrn, "%s", prntxt.m_chTextLines[iLine] );
		}
	}

	m_iCurrentText = m_arrTxt.Count();
}



class CPoseDebuggerImpl : public IPoseDebugger
{
public:
	CPoseDebuggerImpl();
	~CPoseDebuggerImpl();

public:
	void ShowAllModels( bool bShow );
	void ShowModel( int iEntNum, bool bShow );
	bool IsModelShown( int iEntNum ) const;

public:
	virtual void StartBlending( IClientNetworkable *pEntity, const CStudioHdr *pStudioHdr );
	virtual void AccumulatePose(
		const CStudioHdr *pStudioHdr,
		CIKContext *pIKContext,
		Vector pos[], 
		Quaternion q[], 
		int sequence, 
		float cycle,
		const float poseParameter[],
		int boneMask,
		float flWeight,
		float flTime
		);

protected:
	typedef CUtlMap< CStudioHdr const *, ModelPoseDebugInfo > MapModel;
	MapModel m_mapModel, m_mapModelOld;
	int m_nPosPrint;

	CBitVec< MAX_EDICTS > m_uiMaskShowModels;

	CStudioHdr const *m_pLastModel;
};

static CPoseDebuggerImpl s_PoseDebuggerImpl;

//////////////////////////////////////////////////////////////////////////
//
// CPoseDebuggerImpl
// Implementation
//
//////////////////////////////////////////////////////////////////////////

CPoseDebuggerImpl::CPoseDebuggerImpl() :
	m_mapModel( DefLessFunc( CStudioHdr const * ) ),
	m_mapModelOld( DefLessFunc( CStudioHdr const * ) ),
	m_nPosPrint( 0 ),
	m_pLastModel( NULL )
{
	m_uiMaskShowModels.SetAll();
}

CPoseDebuggerImpl::~CPoseDebuggerImpl()
{
	// g_pPoseDebugger = &s_PoseDebuggerStub;
}

void CPoseDebuggerImpl::ShowAllModels( bool bShow )
{
	bShow ? m_uiMaskShowModels.SetAll() : m_uiMaskShowModels.ClearAll();
}

void CPoseDebuggerImpl::ShowModel( int iEntNum, bool bShow )
{
	Assert( iEntNum >= 0 && iEntNum < MAX_EDICTS );
	if ( iEntNum >= 0 && iEntNum < MAX_EDICTS )
		m_uiMaskShowModels.Set( iEntNum, bShow );
}

bool CPoseDebuggerImpl::IsModelShown( int iEntNum ) const
{
	Assert( iEntNum >= 0 && iEntNum < MAX_EDICTS );
	if ( iEntNum >= 0 && iEntNum < MAX_EDICTS )
		return m_uiMaskShowModels.IsBitSet( iEntNum );
	else
		return false;
}

void CPoseDebuggerImpl::StartBlending( IClientNetworkable *pEntity, const CStudioHdr *pStudioHdr )
{
//	virtualmodel_t const *pVMdl = pStudioHdr->GetVirtualModel();
// 	if ( !pVMdl )
// 		return;

	// If we are starting a new model then finalize the previous one
	if ( pStudioHdr != m_pLastModel && m_pLastModel )
	{
		MapModel::IndexType_t idx = m_mapModel.Find( m_pLastModel );
		if ( idx != m_mapModel.InvalidIndex() )
		{
			ModelPoseDebugInfo &mpi = m_mapModel.Element( idx );
			ModelPoseDebugInfo *pMpiOld = NULL;
			MapModel::IndexType_t idxMapModelOld = m_mapModelOld.Find( m_pLastModel );
			if ( idxMapModelOld != m_mapModelOld.InvalidIndex() )
			{
				pMpiOld = &m_mapModelOld.Element( idxMapModelOld );
			}
			mpi.AddInfoText( NULL, pMpiOld );
			mpi.PrintPendingInfoText( m_nPosPrint );
		}
	}
	m_pLastModel = pStudioHdr;

	// Go ahead with the new model
	studiohdr_t const *pRMdl = pStudioHdr->GetRenderHdr();
	if ( !pRMdl ||
		 !pRMdl->numincludemodels )
		return;

	// Entity number
	int iEntNum = pEntity->entindex();
	if ( !IsModelShown( iEntNum ) )
		return;

	// Check if we saw the model
	if ( m_mapModel.Find( pStudioHdr ) != m_mapModel.InvalidIndex() )
	{
		// Initialize the printing position
		m_nPosPrint = 9;

		// Swap the maps
		m_mapModelOld.RemoveAll();
		m_mapModelOld.Swap( m_mapModel );

		// Zero out the text on the old map
		for ( int k = m_mapModelOld.FirstInorder();
			  k != m_mapModelOld.InvalidIndex();
			  k = m_mapModelOld.NextInorder( k ) )
		{
			ModelPoseDebugInfo &mpi = m_mapModelOld[k];
			mpi.m_iCurrentText = 0;
		}
	}
	else
	{
		// Next model
		m_nPosPrint += 3;
	}

	ModelPoseDebugInfo mpi;
	mpi.m_iEntNum = iEntNum;
	m_mapModel.Insert( pStudioHdr, mpi );

	con_nprint_s nxPrn = { 0 };
	nxPrn.index = m_nPosPrint;
	nxPrn.time_to_live = -1;
	nxPrn.color[0] = 0.9f, nxPrn.color[1] = 1.0f, nxPrn.color[2] = 0.9f;
	nxPrn.fixed_width_font = false;

	engine->Con_NXPrintf( &nxPrn, "[ %2d  ]    Model: %s", iEntNum, pRMdl->pszName() );
	m_nPosPrint += 3;
}

void CPoseDebuggerImpl::AccumulatePose( const CStudioHdr *pStudioHdr, CIKContext *pIKContext,
									    Vector pos[], Quaternion q[], int sequence, float cycle,
										const float poseParameter[], int boneMask,
										float flWeight, float flTime )
{
//	virtualmodel_t const *pVMdl = pStudioHdr->GetVirtualModel();
// 	if ( !pVMdl )
// 		return;

	studiohdr_t const *pRMdl = pStudioHdr->GetRenderHdr();
	if ( !pRMdl ||
		 !pRMdl->numincludemodels )
		return;

	MapModel::IndexType_t idxMapModel = m_mapModel.Find( pStudioHdr );
	if ( idxMapModel == m_mapModel.InvalidIndex() )
		return;

	ModelPoseDebugInfo &mpi = m_mapModel.Element( idxMapModel );
	if ( !IsModelShown( mpi.m_iEntNum ) )
		return;

	ModelPoseDebugInfo *pMpiOld = NULL;
	MapModel::IndexType_t idxMapModelOld = m_mapModelOld.Find( pStudioHdr );
	if ( idxMapModelOld != m_mapModelOld.InvalidIndex() )
	{
		pMpiOld = &m_mapModelOld.Element( idxMapModelOld );
	}


	//
	// Actual processing
	//

	mstudioseqdesc_t	&seqdesc = ((CStudioHdr *)pStudioHdr)->pSeqdesc( sequence );

	if ( sequence >= pStudioHdr->GetNumSeq() )
	{
		sequence = 0;
		seqdesc = ((CStudioHdr *)pStudioHdr)->pSeqdesc( sequence );
	}

	enum
	{
		widthActivity	= 35,
		widthLayer		= 35,
		widthIks		= 60,
		widthPercent	= 6,
	};

	// Prepare the text
	char chBuffer[256];
	ModelPoseDebugInfo::InfoText txt;
	int numLines = 0;
	
	txt.m_iActivity = seqdesc.activity;
	sprintf( txt.m_chActivity, "%s", seqdesc.pszActivityName() );
	sprintf( txt.m_chLabel, "%s", seqdesc.pszLabel() );

	if ( !txt.m_chActivity[0] )
	{
		// Try to find the last seen activity and re-use it
		for ( int iLast = mpi.m_arrTxt.Count(); iLast --> 0; )
		{
			ModelPoseDebugInfo::InfoText &lastSeenTxt = mpi.m_arrTxt[iLast];
			if ( lastSeenTxt.m_uiFlags & ModelPoseDebugInfo::F_SEEN_THIS_FRAME &&
				 lastSeenTxt.m_chActivity[0] )
			{
				sprintf( txt.m_chActivity, "%s", lastSeenTxt.m_chActivity );
				break;
			}
		}
	}

	// The layer information
	ModelPoseDebugInfo::InfoText *pOldTxt = pMpiOld ? pMpiOld->LookupInfoText( &txt ) : NULL;
	sprintf( txt.m_chTextLines[numLines],
		"%-*s  %-*s  %*.2f  %*.1f/%-*d  %*.0f%% ",
		widthActivity,
		seqdesc.pszActivityName(),
		widthLayer,
		seqdesc.pszLabel(),
		7,
		pOldTxt ? pOldTxt->m_flTimeAlive : 0.f,
		5,
		cycle * ( ((CStudioHdr *)pStudioHdr)->pAnimdesc( seqdesc.anim( 0, 0 ) ).numframes - 1 ),
		3,
		((CStudioHdr *)pStudioHdr)->pAnimdesc( seqdesc.anim( 0, 0 ) ).numframes,
		widthPercent,
		flWeight * 100.0f
		);
	++ numLines;

	if ( seqdesc.numiklocks )
	{
		sprintf( chBuffer,
			"iklocks : %-2d : ",
			seqdesc.numiklocks );

		for ( int k = 0; k < seqdesc.numiklocks; ++ k )
		{
			mstudioiklock_t *plock = seqdesc.pIKLock( k );
			mstudioikchain_t *pchain = pStudioHdr->pIKChain( plock->chain );

			sprintf( chBuffer + strlen( chBuffer ), "%s ", pchain->pszName() );
			// plock->flPosWeight;
			// plock->flLocalQWeight;
		}

		sprintf( txt.m_chTextLines[numLines],
			"%-*s",
			widthIks,
			chBuffer
			);
		++ numLines;
	}

	if ( seqdesc.numikrules )
	{
		sprintf( chBuffer, "ikrules : %-2d",
			seqdesc.numikrules );

		sprintf( txt.m_chTextLines[numLines],
			"%-*s",
			widthIks,
			chBuffer
			);
		++ numLines;
	}


	// Now add the accumulated text into the container
	mpi.AddInfoText( &txt, pMpiOld );
	mpi.PrintPendingInfoText( m_nPosPrint );
}


//////////////////////////////////////////////////////////////////////////
//
// Con-commands
//
//////////////////////////////////////////////////////////////////////////

static void IN_PoseDebuggerStart( const CCommand &args )
{
	if ( args.ArgC() <= 1 )
	{
		// No args, enable all
		s_PoseDebuggerImpl.ShowAllModels( true );
	}
	else
	{
		// If explicitly showing the pose debugger when it was disabled
		if ( g_pPoseDebugger != &s_PoseDebuggerImpl )
		{
			s_PoseDebuggerImpl.ShowAllModels( false );
		}

		// Show only specific models
		for ( int k = 1; k < args.ArgC(); ++ k )
		{
			int iEntNum = atoi( args.Arg( k ) );
			s_PoseDebuggerImpl.ShowModel( iEntNum, true );
		}
	}

	g_pPoseDebugger = &s_PoseDebuggerImpl;
}

static void IN_PoseDebuggerEnd( const CCommand &args )
{
	if ( args.ArgC() <= 1 )
	{
		// No args, disable all
		s_PoseDebuggerImpl.ShowAllModels( false );

		// Set the stub pointer
		g_pPoseDebugger = &s_PoseDebuggerStub;
	}
	else
	{
		// Hide only specific models
		for ( int k = 1; k < args.ArgC(); ++ k )
		{
			int iEntNum = atoi( args.Arg( k ) );
			s_PoseDebuggerImpl.ShowModel( iEntNum, false );
		}
	}
}

static ConCommand posedebuggerstart( "+posedebug", IN_PoseDebuggerStart, "Turn on pose debugger or add ents to pose debugger UI", FCVAR_CHEAT );
static ConCommand posedebuggerend  ( "-posedebug", IN_PoseDebuggerEnd, "Turn off pose debugger or hide ents from pose debugger UI", FCVAR_CHEAT );
