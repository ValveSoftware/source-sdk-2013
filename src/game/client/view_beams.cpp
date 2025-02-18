//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "iviewrender_beams.h"
#include "tempentity.h"
#include "beam_shared.h"
#include "ivieweffects.h"
#include "beamdraw.h"
#include "engine/ivdebugoverlay.h"
#include "engine/ivmodelinfo.h"
#include "view.h"
#include "fx.h"
#include "tier0/icommandline.h"
#include "tier0/vprof.h"
#include "c_pixel_visibility.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "viewrender.h"

#ifdef PORTAL
	#include "prop_portal_shared.h"
#endif

ConVar r_DrawBeams( "r_DrawBeams", "1", FCVAR_CHEAT, "0=Off, 1=Normal, 2=Wireframe" );

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool g_BeamCreationAllowed = false;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void SetBeamCreationAllowed( bool state )
{
	g_BeamCreationAllowed = state;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BeamCreationAllowed( void )
{
	return g_BeamCreationAllowed;
}

extern IViewEffects *vieweffects;

//-----------------------------------------------------------------------------
// Purpose: Implements beam rendering apis
//-----------------------------------------------------------------------------
class CViewRenderBeams : public IViewRenderBeams
{
// Construction
public:
						CViewRenderBeams( void );
	virtual				~CViewRenderBeams( void );

// Implement IViewRenderBeams
public:
	virtual	void		InitBeams( void );
	virtual	void		ShutdownBeams( void );
	virtual	void		ClearBeams( void );

	// Updates the state of the temp ent beams
	virtual void		UpdateTempEntBeams();

	virtual void		DrawBeam( C_Beam* pbeam, ITraceFilter *pEntityBeamTraceFilter = NULL );
	virtual void		DrawBeam( Beam_t *pbeam );

	virtual	void		KillDeadBeams( C_BaseEntity *pDeadEntity );

	virtual Beam_t		*CreateBeamEnts( BeamInfo_t &beamInfo );
	virtual Beam_t		*CreateBeamEntPoint( BeamInfo_t &beamInfo );
	virtual	Beam_t		*CreateBeamPoints( BeamInfo_t &beamInfo );
	virtual Beam_t		*CreateBeamRing( BeamInfo_t &beamInfo );
	virtual Beam_t		*CreateBeamRingPoint( BeamInfo_t &beamInfo );
	virtual Beam_t		*CreateBeamCirclePoints( BeamInfo_t &beamInfo );
	virtual Beam_t		*CreateBeamFollow( BeamInfo_t &beamInfo );

	virtual	void		CreateBeamEnts( int startEnt, int endEnt, int modelIndex, int haloIndex, float haloScale, 
							float life, float width, float endWidth, float fadeLength, float amplitude, 
							float brightness, float speed, int startFrame, 
							float framerate, float r, float g, float b, int type = -1 );
	virtual	void		CreateBeamEntPoint( int	nStartEntity, const Vector *pStart, int nEndEntity, const Vector* pEnd,
							int modelIndex, int haloIndex, float haloScale, 
							float life, float width, float endWidth, float fadeLength, float amplitude, 
							float brightness, float speed, int startFrame, 
							float framerate, float r, float g, float b );
	virtual	void		CreateBeamPoints( Vector& start, Vector& end, int modelIndex, int haloIndex, float haloScale,   
							float life, float width, float endWidth, float fadeLength, float amplitude, 
							float brightness, float speed, int startFrame, 
							float framerate, float r, float g, float b );
	virtual	void		CreateBeamRing( int startEnt, int endEnt, int modelIndex, int haloIndex, float haloScale,   
							float life, float width, float endWidth, float fadeLength, float amplitude, 
							float brightness, float speed, int startFrame, 
							float framerate, float r, float g, float b, int flags );
	virtual void		CreateBeamRingPoint( const Vector& center, float start_radius, float end_radius, int modelIndex, int haloIndex, float haloScale,   
							float life, float width, float m_nEndWidth, float m_nFadeLength, float amplitude, 
							float brightness, float speed, int startFrame, 
							float framerate, float r, float g, float b, int flags );
	virtual	void		CreateBeamCirclePoints( int type, Vector& start, Vector& end, 
							int modelIndex,  int haloIndex,  float haloScale, float life, float width, 
							float endWidth, float fadeLength, float amplitude, float brightness, float speed, 
							int startFrame, float framerate, float r, float g, float b );
	virtual	void		CreateBeamFollow( int startEnt, int modelIndex, int haloIndex, float haloScale,   
							float life, float width, float endWidth, float fadeLength, float r, float g, float b, 
							float brightness );

	virtual void		FreeBeam( Beam_t *pBeam ) { BeamFree( pBeam ); }
	virtual void		UpdateBeamInfo( Beam_t *pBeam, BeamInfo_t &beamInfo );

private:
	void					FreeDeadTrails( BeamTrail_t **trail );
	void					UpdateBeam( Beam_t *pbeam, float frametime );
	void					DrawBeamWithHalo( Beam_t* pbeam,int frame,int rendermode,float *color, float *srcColor, const model_t *sprite,const model_t *halosprite, float flHDRColorScale );
	void					DrawBeamFollow( const model_t* pSprite, Beam_t *pbeam, int frame, int rendermode, float frametime, const float* color, float flHDRColorScale = 1.0f );
	void					DrawLaser( Beam_t* pBeam, int frame, int rendermode, float* color, model_t const* sprite, model_t const* halosprite, float flHDRColorScale = 1.0f );
	void					DrawTesla( Beam_t* pBeam, int frame, int rendermode, float* color, model_t const* sprite, float flHDRColorScale = 1.0f );

	bool					RecomputeBeamEndpoints( Beam_t *pbeam );

	int						CullBeam( const Vector &start, const Vector &end, int pvsOnly );

	// Creation
	Beam_t					*CreateGenericBeam( BeamInfo_t &beamInfo );
	void					SetupBeam( Beam_t *pBeam, const BeamInfo_t &beamInfo );
	void					SetBeamAttributes( Beam_t *pBeam, const BeamInfo_t &beamInfo );
	
	// Memory Alloc/Free
	Beam_t*					BeamAlloc( bool bRenderable );
	void					BeamFree( Beam_t* pBeam );

// DATA
private:
	enum
	{

#ifndef _XBOX
		// default max # of particles at one time
		DEFAULT_PARTICLES	= 2048,
#else
		DEFAULT_PARTICLES   = 1024,
#endif

		// no fewer than this no matter what's on the command line
		MIN_PARTICLES		= 512,	

#ifndef _XBOX
		// Maximum length of the free list.
		BEAM_FREELIST_MAX	= 32
#else
		BEAM_FREELIST_MAX   = 4
#endif

	};

	Beam_t					*m_pActiveBeams;
	Beam_t					*m_pFreeBeams;
	int						m_nBeamFreeListLength;

	BeamTrail_t				*m_pBeamTrails;
	BeamTrail_t				*m_pActiveTrails;
	BeamTrail_t				*m_pFreeTrails;
	int						m_nNumBeamTrails;
};

// Expose interface to rest of client .dll
static CViewRenderBeams s_ViewRenderBeams;
IViewRenderBeams *beams = ( IViewRenderBeams * )&s_ViewRenderBeams;

CUniformRandomStream beamRandom;
//-----------------------------------------------------------------------------
// Global methods
//-----------------------------------------------------------------------------

//		freq2 += step * 0.1;
// Fractal noise generator, power of 2 wavelength
static void Noise( float *noise, int divs, float scale )
{
	int div2;
	
	div2 = divs >> 1;

	if ( divs < 2 )
		return;

	// Noise is normalized to +/- scale
	noise[ div2 ] = (noise[0] + noise[divs]) * 0.5 + scale * beamRandom.RandomFloat(-1, 1);
	if ( div2 > 1 )
	{
		Noise( &noise[div2], div2, scale * 0.5 );
		Noise( noise, div2, scale * 0.5 );
	}
}

static void SineNoise( float *noise, int divs )
{
	int i;
	float freq;
	float step = M_PI / (float)divs;

	freq = 0;
	for ( i = 0; i < divs; i++ )
	{
		noise[i] = sin( freq );
		freq += step;
	}
}

bool ComputeBeamEntPosition( C_BaseEntity *pEnt, int nAttachment, bool bInterpretAttachmentIndexAsHitboxIndex, Vector& pt )
{
	// NOTE: This will *leave* the pt at its current value, essential for
	// beam follow ents what want to stick around a little after their ent has died
	if (!pEnt)
		return false;

	if ( !bInterpretAttachmentIndexAsHitboxIndex )
	{
		QAngle angles;
		if ( pEnt->GetAttachment( nAttachment, pt, angles ) )
			return true;
	}
	else
	{
		C_BaseAnimating *pAnimating = pEnt->GetBaseAnimating();
		if ( pAnimating )
		{
			studiohdr_t *pStudioHdr = modelinfo->GetStudiomodel( pAnimating->GetModel() );
			if (pStudioHdr)
			{
				mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( pAnimating->GetHitboxSet() );
				if ( set && (set->numhitboxes >= nAttachment) && (nAttachment > 0) )
				{
					matrix3x4_t	*hitboxbones[MAXSTUDIOBONES];
					if ( pAnimating->HitboxToWorldTransforms( hitboxbones ) )
					{
						mstudiobbox_t *pHitbox = set->pHitbox( nAttachment - 1 );
						Vector vecViewPt = MainViewOrigin();
						Vector vecLocalViewPt;
						VectorITransform( vecViewPt, *hitboxbones[ pHitbox->bone ], vecLocalViewPt );

						Vector vecLocalClosestPt;
						CalcClosestPointOnAABB( pHitbox->bbmin, pHitbox->bbmax, vecLocalViewPt, vecLocalClosestPt );

						VectorTransform( vecLocalClosestPt, *hitboxbones[ pHitbox->bone ], pt );

//						MatrixGetColumn( *hitboxbones[ pHitbox->bone ], 3, pt );
						return true;
					}
				}
			}
		}
	}

	// Player origins are at their feet
	if ( pEnt->IsPlayer() )
	{
		pt = pEnt->WorldSpaceCenter();
	}
	else
	{
		VectorCopy( pEnt->GetRenderOrigin(), pt );
	}

	return true;
}


//-----------------------------------------------------------------------------
//
// Methods of Beam_t
//
//-----------------------------------------------------------------------------

Beam_t::Beam_t()
{
#ifdef PORTAL
	m_bDrawInMainRender = true;
	m_bDrawInPortalRender = true;
#endif

	Reset();
}


void Beam_t::Reset()
{
	m_Mins.Init(0,0,0);
	m_Maxs.Init(0,0,0);
	type = 0;
	flags = 0;
	trail = 0;
	m_hRenderHandle = INVALID_CLIENT_RENDER_HANDLE;
	m_bCalculatedNoise = false;
	m_queryHandleHalo = NULL;
	m_flHDRColorScale = 1.0f;
}


const Vector& Beam_t::GetRenderOrigin( void )
{
	if ((type == TE_BEAMRING) || (type == TE_BEAMRINGPOINT))
	{
		// return the center of the ring
		static Vector org;
		VectorMA( attachment[0], 0.5f, delta, org );
		return org;
	}

	return attachment[0];
}

const QAngle& Beam_t::GetRenderAngles( void )
{
	return vec3_angle;
}

const matrix3x4_t &Beam_t::RenderableToWorldTransform()
{
	static matrix3x4_t mat;
	SetIdentityMatrix( mat );
	PositionMatrix( GetRenderOrigin(), mat );
	return mat;
}


void Beam_t::GetRenderBounds( Vector& mins, Vector& maxs )
{
	VectorCopy( m_Mins, mins );
	VectorCopy( m_Maxs, maxs );
}


void Beam_t::ComputeBounds( )
{
	switch( type )
	{
	case TE_BEAMSPLINE:
		{
			// Here, we gotta look at all the attachments....
			Vector attachmentDelta;
			m_Mins.Init( 0,0,0 );
			m_Maxs.Init( 0,0,0 );

			for (int i=1; i < numAttachments; i++)
			{
				VectorSubtract( attachment[i], attachment[0], attachmentDelta );
				m_Mins = m_Mins.Min( attachmentDelta );
				m_Maxs = m_Maxs.Max( attachmentDelta );
			}
		}
		break;

	case TE_BEAMDISK:
	case TE_BEAMCYLINDER:
		{
			// FIXME: This isn't quite right for the cylinder

			// Here, delta[2] is the radius
			int radius = delta[2];
			m_Mins.Init( -radius, -radius, -radius );
			m_Maxs.Init( radius, radius, radius );
		}
		break;

	case TE_BEAMRING:
	case TE_BEAMRINGPOINT:
		{
			int radius = delta.Length() * 0.5f;
			m_Mins.Init( -radius, -radius, -radius );
			m_Maxs.Init( radius, radius, radius );
		}
		break;

	case TE_BEAMPOINTS:
	default:
		{
			// Just use the delta
			for (int i = 0; i < 3; ++i)
			{
				if (delta[i] > 0.0f)
				{
					m_Mins[i] = 0.0f;
					m_Maxs[i] = delta[i];
				}
				else
				{
					m_Mins[i] = delta[i];
					m_Maxs[i] = 0.0f;
				}
			}
		}
		break;
	}

	// Deal with beam follow
	Vector org = GetRenderOrigin();
	Vector followDelta;
	BeamTrail_t* pFollow = trail;
	while (pFollow)
	{
		VectorSubtract( pFollow->org, org, followDelta );
		m_Mins = m_Mins.Min( followDelta );
		m_Maxs = m_Maxs.Max( followDelta );

		pFollow = pFollow->next;
	}
}

bool Beam_t::ShouldDraw( void )
{
	return true;
}

bool Beam_t::IsTransparent( void )
{
	return true;
}

void Beam_t::ComputeFxBlend( )
{
	// Do nothing, they're always 255
}

int	Beam_t::GetFxBlend( )
{
	return 255;
}

extern bool g_bRenderingScreenshot;
extern ConVar r_drawviewmodel;

int Beam_t::DrawModel( int ignored )
{
#ifdef PORTAL
	if ( ( !g_pPortalRender->IsRenderingPortal() && !m_bDrawInMainRender ) || 
		( g_pPortalRender->IsRenderingPortal() && !m_bDrawInPortalRender ) )
	{
		return 0;
	}
#endif //#ifdef PORTAL

	// Tracker 16432:  If rendering a savegame screenshot don't draw beams 
	//   who have viewmodels as their attached entity
	if ( g_bRenderingScreenshot || !r_drawviewmodel.GetBool() )
	{
		// If the beam is attached
		for (int i=0;i<MAX_BEAM_ENTS;i++)
		{
			C_BaseViewModel *vm = dynamic_cast<C_BaseViewModel *>(entity[i].Get());
			if ( vm )
			{
				return 0;
			}
		}
	}

	s_ViewRenderBeams.DrawBeam( this );
	return 0;
}


//-----------------------------------------------------------------------------
//
// Implementation of CViewRenderBeams
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constructor, destructor: 
//-----------------------------------------------------------------------------

CViewRenderBeams::CViewRenderBeams( void ) : m_pBeamTrails(0)
{
	m_pFreeBeams = NULL;
	m_pActiveBeams = NULL;
	m_nBeamFreeListLength = 0;
}

CViewRenderBeams::~CViewRenderBeams( void )
{
	ClearBeams();
}


//-----------------------------------------------------------------------------
// Purpose: Initialize beam system and beam trails for follow beams
//-----------------------------------------------------------------------------
void CViewRenderBeams::InitBeams( void )
{
	int p = CommandLine()->ParmValue("-particles", -1);
	if ( p >= 0 )
	{
		m_nNumBeamTrails = MAX( p, MIN_PARTICLES );
	}
	else
	{
		m_nNumBeamTrails = DEFAULT_PARTICLES;
	}
	
	m_pBeamTrails = (BeamTrail_t *)new BeamTrail_t[ m_nNumBeamTrails ];
	Assert( m_pBeamTrails );

	// Clear them out
	ClearBeams();
}


//-----------------------------------------------------------------------------
// Purpose: Clear out all beams
//-----------------------------------------------------------------------------
void CViewRenderBeams::ClearBeams( void )
{
	Beam_t *next = NULL;
	for( ; m_pActiveBeams; m_pActiveBeams = next )
	{
		next = m_pActiveBeams->next;
		delete m_pActiveBeams;
	}

	for( ; m_pFreeBeams; m_pFreeBeams = next )
	{
		next = m_pFreeBeams->next;
		delete m_pFreeBeams;
	}

	m_nBeamFreeListLength = 0;

	if ( m_nNumBeamTrails )
	{
		// Also clear any particles used by beams
		m_pFreeTrails = &m_pBeamTrails[0];
		m_pActiveTrails = NULL;

		for (int i=0 ;i<m_nNumBeamTrails ; i++)
		{
			m_pBeamTrails[i].next = &m_pBeamTrails[i+1];
		}
		m_pBeamTrails[m_nNumBeamTrails-1].next = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Shut down beam system
//-----------------------------------------------------------------------------

void CViewRenderBeams::ShutdownBeams( void )
{
	if (m_pBeamTrails)
	{
		delete[] m_pBeamTrails;
		m_pActiveTrails = NULL;
		m_pBeamTrails = NULL;
		m_pFreeTrails = NULL;
		m_nNumBeamTrails = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Try and allocate a free beam
// Output : Beam_t
//-----------------------------------------------------------------------------
Beam_t *CViewRenderBeams::BeamAlloc( bool bRenderable )
{
	Beam_t*	pBeam = NULL; 

	if ( m_pFreeBeams )
	{
		pBeam = m_pFreeBeams;
		m_pFreeBeams  = pBeam->next;
		m_nBeamFreeListLength--;
	}
	else
	{
		pBeam = new Beam_t();
		if( !pBeam )
		{
			DevMsg( "ERROR: failed to alloc Beam_t!\n" );
			Assert( pBeam );
		}
	}
	pBeam->next		= m_pActiveBeams;
	m_pActiveBeams	= pBeam;

	if ( bRenderable )
	{
		// Hook it into the rendering system...
		ClientLeafSystem()->AddRenderable( pBeam, RENDER_GROUP_TRANSLUCENT_ENTITY );
	}
	else
	{
		pBeam->m_hRenderHandle = INVALID_CLIENT_RENDER_HANDLE;
	}

	return pBeam;
}

//-----------------------------------------------------------------------------
// Purpose: Free the beam.
//-----------------------------------------------------------------------------
void CViewRenderBeams::BeamFree( Beam_t* pBeam )
{
	// Free particles that have died off.
	FreeDeadTrails( &pBeam->trail );

	// Remove it from the rendering system...
	ClientLeafSystem()->RemoveRenderable( pBeam->m_hRenderHandle );

	// Clear us out
	pBeam->Reset();

	if( m_nBeamFreeListLength < BEAM_FREELIST_MAX )
	{
		m_nBeamFreeListLength++;

		// Now link into free list;
		pBeam->next = m_pFreeBeams;
		m_pFreeBeams = pBeam;
	}
	else
	{
		delete pBeam;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Iterates through active list and kills beams associated with deadEntity
// Input  : deadEntity - 
//-----------------------------------------------------------------------------
void CViewRenderBeams::KillDeadBeams( C_BaseEntity *pDeadEntity )
{
	Beam_t *pbeam;
	Beam_t *pnewlist;
	Beam_t *pnext;
	BeamTrail_t *pHead;  // Build a new list to replace m_pActiveBeams.

	pbeam    = m_pActiveBeams;  // Old list.
	pnewlist = NULL;           // New list.
	
	while (pbeam)
	{
		pnext = pbeam->next;
		if (pbeam->entity[0] != pDeadEntity )   // Link into new list.
		{
			pbeam->next = pnewlist;
			pnewlist = pbeam;

			pbeam = pnext;
			continue;
		}

		pbeam->flags &= ~(FBEAM_STARTENTITY | FBEAM_ENDENTITY);
		if ( pbeam->type != TE_BEAMFOLLOW )
		{
			// Die Die Die!
			pbeam->die = gpGlobals->curtime - 0.1;  

			// Kill off particles
			pHead = pbeam->trail;
			while (pHead)
			{
				pHead->die = gpGlobals->curtime - 0.1;
				pHead = pHead->next;
			}

			// Free the beam
			BeamFree( pbeam );
		}
		else
		{
			// Stay active
			pbeam->next = pnewlist;
			pnewlist = pbeam;
		}
		pbeam = pnext;
	}

	// We now have a new list with the bogus stuff released.
	m_pActiveBeams = pnewlist;
}

//-----------------------------------------------------------------------------
// Purpose: Fill in values to beam structure.
//   Input: pBeam -
//          beamInfo -
//-----------------------------------------------------------------------------
void CViewRenderBeams::SetupBeam( Beam_t *pBeam, const BeamInfo_t &beamInfo )
{
	const model_t *pSprite = modelinfo->GetModel( beamInfo.m_nModelIndex );
	if ( !pSprite )
		return;

	pBeam->type				= ( beamInfo.m_nType < 0 ) ? TE_BEAMPOINTS : beamInfo.m_nType;
	pBeam->modelIndex		= beamInfo.m_nModelIndex;
	pBeam->haloIndex		= beamInfo.m_nHaloIndex;
	pBeam->haloScale		= beamInfo.m_flHaloScale;
	pBeam->frame			= 0;
	pBeam->frameRate		= 0;
	pBeam->frameCount		= modelinfo->GetModelFrameCount( pSprite );
	pBeam->freq				= gpGlobals->curtime * beamInfo.m_flSpeed;
	pBeam->die				= gpGlobals->curtime + beamInfo.m_flLife;
	pBeam->width			= beamInfo.m_flWidth;
	pBeam->endWidth			= beamInfo.m_flEndWidth;
	pBeam->fadeLength		= beamInfo.m_flFadeLength;
	pBeam->amplitude		= beamInfo.m_flAmplitude;
	pBeam->brightness		= beamInfo.m_flBrightness;
	pBeam->speed			= beamInfo.m_flSpeed;
	pBeam->life				= beamInfo.m_flLife;
	pBeam->flags			= 0;

	VectorCopy( beamInfo.m_vecStart, pBeam->attachment[0] );
	VectorCopy( beamInfo.m_vecEnd, pBeam->attachment[1] );
	VectorSubtract( beamInfo.m_vecEnd, beamInfo.m_vecStart, pBeam->delta );
	Assert( pBeam->delta.IsValid() );

	if ( beamInfo.m_nSegments == -1 )
	{
		if ( pBeam->amplitude >= 0.50 )
		{
			pBeam->segments = VectorLength( pBeam->delta ) * 0.25 + 3; // one per 4 pixels
		}
		else
		{
			pBeam->segments = VectorLength( pBeam->delta ) * 0.075 + 3; // one per 16 pixels
		}
	}
	else
	{
		pBeam->segments = beamInfo.m_nSegments;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set beam color and frame data.
//	 Input: pBeam -
//          beamInfo -
//-----------------------------------------------------------------------------
void CViewRenderBeams::SetBeamAttributes( Beam_t *pBeam, const BeamInfo_t &beamInfo )
{
	pBeam->frame	 = ( float )beamInfo.m_nStartFrame;
	pBeam->frameRate = beamInfo.m_flFrameRate;
	pBeam->flags |= beamInfo.m_nFlags;

	pBeam->r = beamInfo.m_flRed;
	pBeam->g = beamInfo.m_flGreen;
	pBeam->b = beamInfo.m_flBlue;
}

//-----------------------------------------------------------------------------
// Purpose: Cull beam by bbox
// Input  : *start - 
//			*end - 
//			pvsOnly - 
// Output : int
//-----------------------------------------------------------------------------

int CViewRenderBeams::CullBeam( const Vector &start, const Vector &end, int pvsOnly )
{
	Vector mins, maxs;
	int i;

	for ( i = 0; i < 3; i++ )
	{
		if ( start[i] < end[i] )
		{
			mins[i] = start[i];
			maxs[i] = end[i];
		}
		else
		{
			mins[i] = end[i];
			maxs[i] = start[i];
		}
		
		// Don't let it be zero sized
		if ( mins[i] == maxs[i] )
		{
			maxs[i] += 1;
		}
	}

	// Check bbox
	if ( engine->IsBoxVisible( mins, maxs ) )
	{
		if ( pvsOnly || !engine->CullBox( mins, maxs ) )
		{
			// Beam is visible
			return 1;	
		}
	}

	// Beam is not visible
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Allocate and setup a generic beam.
//   Input: beamInfo -
//  Output: Beam_t
//-----------------------------------------------------------------------------
Beam_t *CViewRenderBeams::CreateGenericBeam( BeamInfo_t &beamInfo )
{
#if 0
	if ( BeamCreationAllowed() == false )
	{
		//NOTENOTE: If you've hit this, you may not add a beam where you have attempted to.
		//			Most often this means that you have added it in an entity's DrawModel function.
		//			Move this to the ClientThink function instead!

		DevMsg( "ERROR: Beam created too late in frame!\n" );
		Assert(0);
		return NULL;
	}
#endif

	Beam_t *pBeam = BeamAlloc( beamInfo.m_bRenderable );
	if ( !pBeam )
		return NULL;

	// In case we fail.
	pBeam->die = gpGlobals->curtime;

	// Need a valid model.
	if ( beamInfo.m_nModelIndex < 0 )
		return NULL;

	// Set it up
	SetupBeam( pBeam, beamInfo );

	return pBeam;	
}

//-----------------------------------------------------------------------------
// Purpose: Create a beam between two ents
// Input  : startEnt - 
//			endEnt - 
//			modelIndex - 
//			life - 
//			width - 
//			amplitude - 
//			brightness - 
//			speed - 
//			startFrame - 
//			framerate - 
//			BEAMENT_ENTITY(startEnt - 
// Output : Beam_t
//-----------------------------------------------------------------------------
void CViewRenderBeams::CreateBeamEnts( int startEnt, int endEnt, int modelIndex, 
	int haloIndex, float haloScale, float life, float width, float endWidth, 
	float fadeLength, float amplitude, float brightness, float speed, 
	int startFrame, float framerate, float r, float g, float b, int type )
{
	BeamInfo_t beamInfo;

	beamInfo.m_nType = type;
	beamInfo.m_pStartEnt = cl_entitylist->GetEnt( BEAMENT_ENTITY( startEnt ) );
	beamInfo.m_nStartAttachment = BEAMENT_ATTACHMENT( startEnt );
	beamInfo.m_pEndEnt = cl_entitylist->GetEnt( BEAMENT_ENTITY( endEnt ) );
	beamInfo.m_nEndAttachment = BEAMENT_ATTACHMENT( endEnt );
	beamInfo.m_nModelIndex = modelIndex;
	beamInfo.m_nHaloIndex = haloIndex;
	beamInfo.m_flHaloScale = haloScale;
	beamInfo.m_flLife = life;
	beamInfo.m_flWidth = width;
	beamInfo.m_flEndWidth = endWidth;
	beamInfo.m_flFadeLength = fadeLength;
	beamInfo.m_flAmplitude = amplitude;
	beamInfo.m_flBrightness = brightness;
	beamInfo.m_flSpeed = speed;
	beamInfo.m_nStartFrame = startFrame;
	beamInfo.m_flFrameRate = framerate;
	beamInfo.m_flRed = r;
	beamInfo.m_flGreen = g;
	beamInfo.m_flBlue = b;

	CreateBeamEnts( beamInfo );
}

//-----------------------------------------------------------------------------
// Purpose: Create a beam between two entities.
//-----------------------------------------------------------------------------
Beam_t *CViewRenderBeams::CreateBeamEnts( BeamInfo_t &beamInfo )
{
	// Don't start temporary beams out of the PVS
	if ( beamInfo.m_flLife != 0 && 
		 ( !beamInfo.m_pStartEnt || beamInfo.m_pStartEnt->GetModel() == NULL || 
		   !beamInfo.m_pEndEnt || beamInfo.m_pEndEnt->GetModel() == NULL) )
	{
		return NULL;
	}

	beamInfo.m_vecStart = vec3_origin;
	beamInfo.m_vecEnd = vec3_origin;

	Beam_t *pBeam = CreateGenericBeam( beamInfo );
	if ( !pBeam )
		return NULL;

	pBeam->type = ( beamInfo.m_nType < 0 ) ? TE_BEAMPOINTS : beamInfo.m_nType;
	pBeam->flags = FBEAM_STARTENTITY | FBEAM_ENDENTITY;

	pBeam->entity[0] = beamInfo.m_pStartEnt;
	pBeam->attachmentIndex[0] = beamInfo.m_nStartAttachment;
	pBeam->entity[1] = beamInfo.m_pEndEnt;
	pBeam->attachmentIndex[1] = beamInfo.m_nEndAttachment;

	// Attributes.
	SetBeamAttributes( pBeam, beamInfo );
	if ( beamInfo.m_flLife == 0 )
	{
		pBeam->flags |= FBEAM_FOREVER;
	}

	UpdateBeam( pBeam, 0 );

	return pBeam;
}

//-----------------------------------------------------------------------------
// Purpose: Creates a beam between an entity and a point
// Input  : startEnt - 
//			*end - 
//			modelIndex - 
//			life - 
//			width - 
//			amplitude - 
//			brightness - 
//			speed - 
//			startFrame - 
//			framerate - 
//			r - 
//			g - 
//			b - 
// Output : Beam_t
//-----------------------------------------------------------------------------
void CViewRenderBeams::CreateBeamEntPoint( int nStartEntity, const Vector *pStart, int nEndEntity, const Vector* pEnd,
										   int modelIndex, int haloIndex, float haloScale, float life,  float width, 
										   float endWidth, float fadeLength, float amplitude, float brightness, float speed, int startFrame, 
										   float framerate, float r, float g, float b )
{
	BeamInfo_t beamInfo;

	if ( nStartEntity <= 0 )
	{
		beamInfo.m_vecStart = pStart ? *pStart : vec3_origin;
		beamInfo.m_pStartEnt = NULL;
	}
	else
	{
		beamInfo.m_pStartEnt = cl_entitylist->GetEnt( BEAMENT_ENTITY( nStartEntity ) );
		beamInfo.m_nStartAttachment = BEAMENT_ATTACHMENT( nStartEntity );

		// Don't start beams out of the PVS
		if ( !beamInfo.m_pStartEnt )
			return;
	}

	if ( nEndEntity <= 0 )
	{
		beamInfo.m_vecEnd = pEnd ? *pEnd : vec3_origin;
		beamInfo.m_pEndEnt = NULL;
	}
	else
	{
		beamInfo.m_pEndEnt = cl_entitylist->GetEnt( BEAMENT_ENTITY( nEndEntity ) );
		beamInfo.m_nEndAttachment = BEAMENT_ATTACHMENT( nEndEntity );

		// Don't start beams out of the PVS
		if ( !beamInfo.m_pEndEnt )
			return;
	}

	beamInfo.m_nModelIndex = modelIndex;
	beamInfo.m_nHaloIndex = haloIndex;
	beamInfo.m_flHaloScale = haloScale;
	beamInfo.m_flLife = life;
	beamInfo.m_flWidth = width;
	beamInfo.m_flEndWidth = endWidth;
	beamInfo.m_flFadeLength = fadeLength;
	beamInfo.m_flAmplitude = amplitude;
	beamInfo.m_flBrightness = brightness;
	beamInfo.m_flSpeed = speed;
	beamInfo.m_nStartFrame = startFrame;
	beamInfo.m_flFrameRate = framerate;
	beamInfo.m_flRed = r;
	beamInfo.m_flGreen = g;
	beamInfo.m_flBlue = b;

	CreateBeamEntPoint( beamInfo );
}

//-----------------------------------------------------------------------------
// Purpose: Creates a beam between an entity and a point.
//-----------------------------------------------------------------------------
Beam_t *CViewRenderBeams::CreateBeamEntPoint( BeamInfo_t &beamInfo )
{
	if ( beamInfo.m_flLife != 0 )
	{
		if ( beamInfo.m_pStartEnt && beamInfo.m_pStartEnt->GetModel() == NULL )
			return NULL;

		if ( beamInfo.m_pEndEnt && beamInfo.m_pEndEnt->GetModel() == NULL )
			return NULL;
	}

	// Model index.
	if ( ( beamInfo.m_pszModelName ) && ( beamInfo.m_nModelIndex == -1 ) )
	{
		beamInfo.m_nModelIndex = modelinfo->GetModelIndex( beamInfo.m_pszModelName );
	}

	if ( ( beamInfo.m_pszHaloName ) && ( beamInfo.m_nHaloIndex == -1 ) )
	{
		beamInfo.m_nHaloIndex = modelinfo->GetModelIndex( beamInfo.m_pszHaloName );
	}

	Beam_t *pBeam = CreateGenericBeam( beamInfo );
	if ( !pBeam )
		return NULL;

	pBeam->type = TE_BEAMPOINTS;
	pBeam->flags = 0;

	if ( beamInfo.m_pStartEnt )
	{
		pBeam->flags |= FBEAM_STARTENTITY;
		pBeam->entity[0] = beamInfo.m_pStartEnt;
		pBeam->attachmentIndex[0] = beamInfo.m_nStartAttachment;
		beamInfo.m_vecStart = vec3_origin;
	}
	if ( beamInfo.m_pEndEnt )
	{
		pBeam->flags |= FBEAM_ENDENTITY;
		pBeam->entity[1] = beamInfo.m_pEndEnt;
		pBeam->attachmentIndex[1] = beamInfo.m_nEndAttachment;
		beamInfo.m_vecEnd = vec3_origin;
	}

	SetBeamAttributes( pBeam, beamInfo );
	if ( beamInfo.m_flLife == 0 )
	{
		pBeam->flags |= FBEAM_FOREVER;
	}

	UpdateBeam( pBeam, 0 );
	return pBeam;
}

//-----------------------------------------------------------------------------
// Purpose: Creates a beam between two points
// Input  : *start - 
//			*end - 
//			modelIndex - 
//			life - 
//			width - 
//			amplitude - 
//			brightness - 
//			speed - 
//			startFrame - 
//			framerate - 
//			r - 
//			g - 
//			b - 
// Output : Beam_t
//-----------------------------------------------------------------------------
void CViewRenderBeams::CreateBeamPoints( Vector& start, Vector& end, int modelIndex, int haloIndex, float haloScale, float life,  float width, 
										 float endWidth, float fadeLength,float amplitude, float brightness, float speed, int startFrame, 
										 float framerate, float r, float g, float b )
{
	BeamInfo_t beamInfo;
	
	beamInfo.m_vecStart = start;
	beamInfo.m_vecEnd = end;
	beamInfo.m_nModelIndex = modelIndex;
	beamInfo.m_nHaloIndex = haloIndex;
	beamInfo.m_flHaloScale = haloScale;
	beamInfo.m_flLife = life;
	beamInfo.m_flWidth = width;
	beamInfo.m_flEndWidth = endWidth;
	beamInfo.m_flFadeLength = fadeLength;
	beamInfo.m_flAmplitude = amplitude;
	beamInfo.m_flBrightness = brightness;
	beamInfo.m_flSpeed = speed;
	beamInfo.m_nStartFrame = startFrame;
	beamInfo.m_flFrameRate = framerate;
	beamInfo.m_flRed = r;
	beamInfo.m_flGreen = g;
	beamInfo.m_flBlue = b;

	CreateBeamPoints( beamInfo );
}

//-----------------------------------------------------------------------------
// Purpose: Creates a beam between two points.
//-----------------------------------------------------------------------------
Beam_t *CViewRenderBeams::CreateBeamPoints( BeamInfo_t &beamInfo )
{
	// Don't start temporary beams out of the PVS
	if ( beamInfo.m_flLife != 0 && !CullBeam( beamInfo.m_vecStart, beamInfo.m_vecEnd, 1 ) )
		return NULL;

	// Model index.
	if ( ( beamInfo.m_pszModelName ) && ( beamInfo.m_nModelIndex == -1 ) )
	{
		beamInfo.m_nModelIndex = modelinfo->GetModelIndex( beamInfo.m_pszModelName );
	}

	if ( ( beamInfo.m_pszHaloName ) && ( beamInfo.m_nHaloIndex == -1 ) )
	{
		beamInfo.m_nHaloIndex = modelinfo->GetModelIndex( beamInfo.m_pszHaloName );
	}

	// Create the new beam.
	Beam_t *pBeam = CreateGenericBeam( beamInfo );
	if ( !pBeam )
		return NULL;

	// Set beam initial state.
	SetBeamAttributes( pBeam, beamInfo );
	if ( beamInfo.m_flLife == 0 )
	{
		pBeam->flags |= FBEAM_FOREVER;
	}

	return pBeam;
}

//-----------------------------------------------------------------------------
// Purpose: Creates a circular beam between two points
// Input  : type - 
//			*start - 
//			*end - 
//			modelIndex - 
//			life - 
//			width - 
//			amplitude - 
//			brightness - 
//			speed - 
//			startFrame - 
//			framerate - 
//			r - 
//			g - 
//			b - 
// Output : Beam_t
//-----------------------------------------------------------------------------
void CViewRenderBeams::CreateBeamCirclePoints( int type, Vector& start, Vector& end, int modelIndex, int haloIndex, float haloScale, float life, float width, 
											   float endWidth, float fadeLength,float amplitude, float brightness, float speed, int startFrame, 
											   float framerate, float r, float g, float b )
{
	BeamInfo_t beamInfo;
	
	beamInfo.m_nType = type;
	beamInfo.m_vecStart = start;
	beamInfo.m_vecEnd = end;
	beamInfo.m_nModelIndex = modelIndex;
	beamInfo.m_nHaloIndex = haloIndex;
	beamInfo.m_flHaloScale = haloScale;
	beamInfo.m_flLife = life;
	beamInfo.m_flWidth = width;
	beamInfo.m_flEndWidth = endWidth;
	beamInfo.m_flFadeLength = fadeLength;
	beamInfo.m_flAmplitude = amplitude;
	beamInfo.m_flBrightness = brightness;
	beamInfo.m_flSpeed = speed;
	beamInfo.m_nStartFrame = startFrame;
	beamInfo.m_flFrameRate = framerate;
	beamInfo.m_flRed = r;
	beamInfo.m_flGreen = g;
	beamInfo.m_flBlue = b;

	CreateBeamCirclePoints( beamInfo );
}

//-----------------------------------------------------------------------------
// Purpose: Creates a circular beam between two points.
//-----------------------------------------------------------------------------
Beam_t *CViewRenderBeams::CreateBeamCirclePoints( BeamInfo_t &beamInfo )
{
	Beam_t *pBeam = CreateGenericBeam( beamInfo );
	if ( !pBeam )
		return NULL;

	pBeam->type = beamInfo.m_nType;

	SetBeamAttributes( pBeam, beamInfo );
	if ( beamInfo.m_flLife == 0 )
	{
		pBeam->flags |= FBEAM_FOREVER;
	}

	return pBeam;
}

//-----------------------------------------------------------------------------
// Purpose: Create a beam which follows an entity
// Input  : startEnt - 
//			modelIndex - 
//			life - 
//			width - 
//			r - 
//			g - 
//			b - 
//			brightness - 
// Output : Beam_t
//-----------------------------------------------------------------------------
void CViewRenderBeams::CreateBeamFollow( int startEnt, int modelIndex, int haloIndex, float haloScale, float life, float width, float endWidth, 
										 float fadeLength, float r, float g, float b, float brightness )
{
	BeamInfo_t beamInfo;

	beamInfo.m_pStartEnt = cl_entitylist->GetEnt( BEAMENT_ENTITY( startEnt ) );
	beamInfo.m_nStartAttachment = BEAMENT_ATTACHMENT( startEnt );
	beamInfo.m_nModelIndex = modelIndex;
	beamInfo.m_nHaloIndex = haloIndex;
	beamInfo.m_flHaloScale = haloScale;
	beamInfo.m_flLife = life;
	beamInfo.m_flWidth = width;
	beamInfo.m_flEndWidth = endWidth;
	beamInfo.m_flFadeLength = fadeLength;
	beamInfo.m_flBrightness = brightness;
	beamInfo.m_flRed = r;
	beamInfo.m_flGreen = g;
	beamInfo.m_flBlue = b;
	beamInfo.m_flAmplitude = life;

	CreateBeamFollow( beamInfo );
}

//-----------------------------------------------------------------------------
// Purpose: Create a beam which follows an entity.
//-----------------------------------------------------------------------------
Beam_t *CViewRenderBeams::CreateBeamFollow( BeamInfo_t &beamInfo )
{
	beamInfo.m_vecStart = vec3_origin;
	beamInfo.m_vecEnd = vec3_origin;
	beamInfo.m_flSpeed = 1.0f;
	Beam_t *pBeam = CreateGenericBeam( beamInfo );
	if ( !pBeam )
		return NULL;

	pBeam->type = TE_BEAMFOLLOW;
	pBeam->flags = FBEAM_STARTENTITY;
	pBeam->entity[0] = beamInfo.m_pStartEnt;
	pBeam->attachmentIndex[0] = beamInfo.m_nStartAttachment;

	beamInfo.m_flFrameRate = 1.0f;
	beamInfo.m_nStartFrame = 0;

	SetBeamAttributes( pBeam, beamInfo );

	UpdateBeam( pBeam, 0 );

	return pBeam;
}

//-----------------------------------------------------------------------------
// Purpose: Create a beam ring between two entities
// Input  : startEnt - 
//			endEnt - 
//			modelIndex - 
//			life - 
//			width - 
//			amplitude - 
//			brightness - 
//			speed - 
//			startFrame - 
//			framerate - 
//			startEnt - 
// Output : Beam_t
//-----------------------------------------------------------------------------
void CViewRenderBeams::CreateBeamRingPoint( const Vector& center, float start_radius, float end_radius, 
					   int modelIndex, int haloIndex, float haloScale, float life, float width, float endWidth, 
					   float fadeLength, float amplitude, float brightness, float speed, int startFrame, float framerate, 
					   float r, float g, float b, int nFlags )
{
	BeamInfo_t beamInfo;
	
	beamInfo.m_nModelIndex = modelIndex;
	beamInfo.m_nHaloIndex = haloIndex;
	beamInfo.m_flHaloScale = haloScale;
	beamInfo.m_flLife = life;
	beamInfo.m_flWidth = width;
	beamInfo.m_flEndWidth = endWidth;
	beamInfo.m_flFadeLength = fadeLength;
	beamInfo.m_flAmplitude = amplitude;
	beamInfo.m_flBrightness = brightness;
	beamInfo.m_flSpeed = speed;
	beamInfo.m_nStartFrame = startFrame;
	beamInfo.m_flFrameRate = framerate;
	beamInfo.m_flRed = r;
	beamInfo.m_flGreen = g;
	beamInfo.m_flBlue = b;
	beamInfo.m_vecCenter = center;
	beamInfo.m_flStartRadius = start_radius;
	beamInfo.m_flEndRadius = end_radius;
	beamInfo.m_nFlags = nFlags;

	CreateBeamRingPoint( beamInfo );
}

//-----------------------------------------------------------------------------
// Purpose: Create a beam ring between two entities
//   Input: beamInfo -
//-----------------------------------------------------------------------------
Beam_t *CViewRenderBeams::CreateBeamRingPoint( BeamInfo_t &beamInfo )
{
	// ??
	Vector endpos = beamInfo.m_vecCenter;

	beamInfo.m_vecStart = beamInfo.m_vecCenter;
	beamInfo.m_vecEnd = beamInfo.m_vecCenter;

	Beam_t *pBeam = CreateGenericBeam( beamInfo );
	if ( !pBeam )
		return NULL;

	pBeam->type = TE_BEAMRINGPOINT;
	pBeam->start_radius = beamInfo.m_flStartRadius;
	pBeam->end_radius = beamInfo.m_flEndRadius;
	pBeam->attachment[2] = beamInfo.m_vecCenter;

	SetBeamAttributes( pBeam, beamInfo );
	if ( beamInfo.m_flLife == 0 )
	{
		pBeam->flags |= FBEAM_FOREVER;
	}

	return pBeam;
}

//-----------------------------------------------------------------------------
// Purpose: Create a beam ring between two entities
// Input  : startEnt - 
//			endEnt - 
//			modelIndex - 
//			life - 
//			width - 
//			amplitude - 
//			brightness - 
//			speed - 
//			startFrame - 
//			framerate - 
//			startEnt - 
// Output : Beam_t
//-----------------------------------------------------------------------------
void CViewRenderBeams::CreateBeamRing( int startEnt, int endEnt, int modelIndex, int haloIndex, float haloScale, float life, float width, float endWidth, float fadeLength, 
					   float amplitude, float brightness, float speed, int startFrame, float framerate, 
					   float r, float g, float b, int flags )
{
	BeamInfo_t beamInfo;

	beamInfo.m_pStartEnt = cl_entitylist->GetEnt( BEAMENT_ENTITY( startEnt ) );
	beamInfo.m_nStartAttachment = BEAMENT_ATTACHMENT( startEnt );
	beamInfo.m_pEndEnt = cl_entitylist->GetEnt( BEAMENT_ENTITY( endEnt ) );
	beamInfo.m_nEndAttachment = BEAMENT_ATTACHMENT( endEnt );
	beamInfo.m_nModelIndex = modelIndex;
	beamInfo.m_nHaloIndex = haloIndex;
	beamInfo.m_flHaloScale = haloScale;
	beamInfo.m_flLife = life;
	beamInfo.m_flWidth = width;
	beamInfo.m_flEndWidth = endWidth;
	beamInfo.m_flFadeLength = fadeLength;
	beamInfo.m_flAmplitude = amplitude;
	beamInfo.m_flBrightness = brightness;
	beamInfo.m_flSpeed = speed;
	beamInfo.m_nStartFrame = startFrame;
	beamInfo.m_flFrameRate = framerate;
	beamInfo.m_flRed = r;
	beamInfo.m_flGreen = g;
	beamInfo.m_flBlue = b;
	beamInfo.m_nFlags = flags;

	CreateBeamRing( beamInfo );
}

//-----------------------------------------------------------------------------
// Purpose: Create a beam ring between two entities.
//   Input: beamInfo -
//-----------------------------------------------------------------------------
Beam_t *CViewRenderBeams::CreateBeamRing( BeamInfo_t &beamInfo )
{
	// Don't start temporary beams out of the PVS
	if ( beamInfo.m_flLife != 0 && 
		 ( !beamInfo.m_pStartEnt || beamInfo.m_pStartEnt->GetModel() == NULL || 
		   !beamInfo.m_pEndEnt || beamInfo.m_pEndEnt->GetModel() == NULL ) )
	{
		return NULL;
	}

	beamInfo.m_vecStart = vec3_origin;
	beamInfo.m_vecEnd = vec3_origin;
	Beam_t *pBeam = CreateGenericBeam( beamInfo );
	if ( !pBeam )
		return NULL;

	pBeam->type = TE_BEAMRING;
	pBeam->flags = FBEAM_STARTENTITY | FBEAM_ENDENTITY;
	pBeam->entity[0] = beamInfo.m_pStartEnt;
	pBeam->attachmentIndex[0] = beamInfo.m_nStartAttachment;
	pBeam->entity[1] = beamInfo.m_pEndEnt;
	pBeam->attachmentIndex[1] = beamInfo.m_nEndAttachment;

	SetBeamAttributes( pBeam, beamInfo );
	if ( beamInfo.m_flLife == 0 )
	{
		pBeam->flags |= FBEAM_FOREVER;
	}

	UpdateBeam( pBeam, 0 );

	return pBeam;
}

//-----------------------------------------------------------------------------
// Purpose: Free dead trails associated with beam
// Input  : **ppparticles - 
//-----------------------------------------------------------------------------
void CViewRenderBeams::FreeDeadTrails( BeamTrail_t **trail )
{
	BeamTrail_t *kill;
	BeamTrail_t *p;

	// kill all the ones hanging direcly off the base pointer
	for ( ;; ) 
	{
		kill = *trail;
		if (kill && kill->die < gpGlobals->curtime)
		{
			*trail = kill->next;
			kill->next = m_pFreeTrails;
			m_pFreeTrails = kill;
			continue;
		}
		break;
	}

	// kill off all the others
	for (p=*trail ; p ; p=p->next)
	{
		for ( ;; )
		{
			kill = p->next;
			if (kill && kill->die < gpGlobals->curtime)
			{
				p->next = kill->next;
				kill->next = m_pFreeTrails;
				m_pFreeTrails = kill;
				continue;
			}
			break;
		}
	}
}



//-----------------------------------------------------------------------------
// Updates beam state
//-----------------------------------------------------------------------------
void CViewRenderBeams::UpdateBeam( Beam_t *pbeam, float frametime )
{
	if ( pbeam->modelIndex < 0 )
	{
		pbeam->die = gpGlobals->curtime;
		return;
	}

	// if we are paused, force random numbers used by noise to generate the same value every frame
	if ( frametime == 0.0f )
	{
		beamRandom.SetSeed( (int)gpGlobals->curtime );
	}

	// If FBEAM_ONLYNOISEONCE is set, we don't want to move once we've first calculated noise
	if ( !(pbeam->flags & FBEAM_ONLYNOISEONCE ) )
	{
		pbeam->freq += frametime;
	}
	else
	{
		pbeam->freq += frametime * beamRandom.RandomFloat(1,2);
	}

	// OPTIMIZE: Do this every frame?
	// UNDONE: Do this differentially somehow?
	// Generate fractal noise
	pbeam->rgNoise[0] = 0;
	pbeam->rgNoise[NOISE_DIVISIONS] = 0;
	if ( pbeam->amplitude != 0 )
	{
		if ( !(pbeam->flags & FBEAM_ONLYNOISEONCE ) || !pbeam->m_bCalculatedNoise )
		{
			if ( pbeam->flags & FBEAM_SINENOISE )
			{
				SineNoise( pbeam->rgNoise, NOISE_DIVISIONS );
			}
			else
			{
				Noise( pbeam->rgNoise, NOISE_DIVISIONS, 1.0 );
			}

			pbeam->m_bCalculatedNoise = true;
		}
	}

	// update end points
	if ( pbeam->flags & (FBEAM_STARTENTITY|FBEAM_ENDENTITY) )
	{
		// Makes sure attachment[0] + attachment[1] are valid
		if (!RecomputeBeamEndpoints( pbeam ))
			return;

		// Compute segments from the new endpoints
		VectorSubtract( pbeam->attachment[1], pbeam->attachment[0], pbeam->delta );
		if ( pbeam->amplitude >= 0.50 )
			pbeam->segments = VectorLength( pbeam->delta ) * 0.25 + 3; // one per 4 pixels
		else
			pbeam->segments = VectorLength( pbeam->delta ) * 0.075 + 3; // one per 16 pixels
	}

	// Get position data for spline beam
	switch ( pbeam->type )
	{
	case TE_BEAMSPLINE:
		{
			// Why isn't attachment[0] being computed?
			for (int i=1; i < pbeam->numAttachments; i++)
			{
				if (!ComputeBeamEntPosition( pbeam->entity[i], pbeam->attachmentIndex[i], (pbeam->flags & FBEAM_USE_HITBOXES) != 0, pbeam->attachment[i] ))
				{
					// This should never happen, but if for some reason the attachment doesn't exist, 
					// as a safety measure copy in the location of the previous attachment point (rather than bailing)
					VectorCopy( pbeam->attachment[i-1], pbeam->attachment[i] );
				}
			}
		}
		break;

	case TE_BEAMRINGPOINT:
		{
			// 
			float dr = pbeam->end_radius - pbeam->start_radius;
			if ( dr != 0.0f )
			{
				float frac = 1.0f;
				// Go some portion of the way there based on life
				float remaining = pbeam->die - gpGlobals->curtime;
				if ( remaining < pbeam->life && pbeam->life > 0.0f )
				{
					frac = remaining / pbeam->life;
				}
				frac = MIN( 1.0f, frac );
				frac = MAX( 0.0f, frac );

				frac = 1.0f - frac;

				// Start pos
				Vector endpos = pbeam->attachment[ 2 ];
				endpos.x += ( pbeam->start_radius + frac * dr ) / 2.0f;
				Vector startpos = pbeam->attachment[ 2 ];
				startpos.x -= ( pbeam->start_radius + frac * dr ) / 2.0f;

				pbeam->attachment[ 0 ] = startpos;
				pbeam->attachment[ 1 ] = endpos;

				VectorSubtract( pbeam->attachment[1], pbeam->attachment[0], pbeam->delta );
				if (pbeam->amplitude >= 0.50)
					pbeam->segments = VectorLength( pbeam->delta ) * 0.25 + 3; // one per 4 pixels
				else
					pbeam->segments = VectorLength( pbeam->delta ) * 0.075 + 3; // one per 16 pixels

			}
		}
		break;

	case TE_BEAMPOINTS:
		// UNDONE: Build culling volumes for other types of beams
		if ( !CullBeam( pbeam->attachment[0], pbeam->attachment[1], 0 ) )
			return;
		break;
	}

	// update life cycle
	pbeam->t = pbeam->freq + (pbeam->die - gpGlobals->curtime);
	if (pbeam->t != 0)
	{
		pbeam->t = pbeam->freq / pbeam->t;
	}
	else
	{
		pbeam->t = 1.0f;
	}

	// ------------------------------------------
	// check for zero fadeLength (means no fade)
	// ------------------------------------------
	if (pbeam->fadeLength == 0)
	{
		Assert( pbeam->delta.IsValid() );
		pbeam->fadeLength = pbeam->delta.Length();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Update beams created by temp entity system
//-----------------------------------------------------------------------------
void CViewRenderBeams::UpdateTempEntBeams( void )
{
	VPROF_("UpdateTempEntBeams", 2, VPROF_BUDGETGROUP_CLIENT_SIM, false, BUDGETFLAG_CLIENT);
	if ( !m_pActiveBeams )
		return;

	// Get frame time
	float frametime = gpGlobals->frametime;

	if ( frametime == 0.0f )
		return;

	// Draw temporary entity beams
	Beam_t* pPrev = 0;
	Beam_t* pNext;
	for ( Beam_t* pBeam = m_pActiveBeams; pBeam ; pBeam = pNext )
	{
		// Need to store the next one since we may delete this one
		pNext = pBeam->next;

		// Retire old beams
		if ( !(pBeam->flags & FBEAM_FOREVER) && 
			pBeam->die <= gpGlobals->curtime )
		{
			// Reset links
			if ( pPrev )
			{
				pPrev->next = pNext;
			}
			else
			{
				m_pActiveBeams = pNext;
			}

			// Free the beam
			BeamFree( pBeam );

			pBeam = NULL;
			continue;
		}

		// Update beam state
		UpdateBeam( pBeam, frametime );

		// Compute bounds for the beam
		pBeam->ComputeBounds();

		// Indicates the beam moved
		if ( pBeam->m_hRenderHandle != INVALID_CLIENT_RENDER_HANDLE )
		{
			ClientLeafSystem()->RenderableChanged( pBeam->m_hRenderHandle );
		}

		pPrev = pBeam;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draw helper for beam follow beams
// Input  : *pbeam - 
//			frametime - 
//			*color - 
//-----------------------------------------------------------------------------
void CViewRenderBeams::DrawBeamFollow( const model_t* pSprite, Beam_t *pbeam, 
	int frame, int rendermode, float frametime, const float* color, float flHDRColorScale )
{
	BeamTrail_t		*particles;
	BeamTrail_t		*pnew;
	float			div;
	Vector			delta;
	
	Vector			screenLast;
	Vector			screen;

	FreeDeadTrails( &pbeam->trail );

	particles = pbeam->trail;
	pnew = NULL;

	div = 0;
	if ( pbeam->flags & FBEAM_STARTENTITY )
	{
		if (particles)
		{
			VectorSubtract( particles->org, pbeam->attachment[0], delta );
			div = VectorLength( delta );
			if (div >= 32 && m_pFreeTrails)
			{
				pnew = m_pFreeTrails;
				m_pFreeTrails = pnew->next;
			}
		}
		else if (m_pFreeTrails)
		{
			pnew = m_pFreeTrails;
			m_pFreeTrails = pnew->next;
			div = 0;
		}
	}

	if (pnew)
	{
		VectorCopy( pbeam->attachment[0], pnew->org );
		pnew->die = gpGlobals->curtime + pbeam->amplitude;
		VectorCopy( vec3_origin, pnew->vel );

		pbeam->die = gpGlobals->curtime + pbeam->amplitude;
		pnew->next = particles;
		pbeam->trail = pnew;
		particles = pnew;
	}

	if (!particles)
	{
		return;
	}
	if (!pnew && div != 0)
	{
		if ( debugoverlay )
		{
			VectorCopy( pbeam->attachment[0], delta );
			debugoverlay->ScreenPosition( pbeam->attachment[0], screenLast );
			debugoverlay->ScreenPosition( particles->org, screen );
		}
	}
	else if (particles && particles->next)
	{
		if ( debugoverlay )
		{
			VectorCopy( particles->org, delta );
			debugoverlay->ScreenPosition( particles->org, screenLast );
			debugoverlay->ScreenPosition( particles->next->org, screen );
		}
		particles = particles->next;
	}
	else
	{
		return;
	}

	// Draw it
	::DrawBeamFollow( pSprite, pbeam->trail, frame, rendermode, delta, screen, screenLast, 
		pbeam->die, pbeam->attachment[0], pbeam->flags, pbeam->width, 
		pbeam->amplitude, pbeam->freq, (float*)color );
	
	// Drift popcorn trail if there is a velocity
	particles = pbeam->trail;
	while (particles)
	{
		VectorMA( particles->org, frametime, particles->vel, particles->org );
		particles = particles->next;
	}
}

//------------------------------------------------------------------------------
// Purpose : Draw beam with a halo
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CViewRenderBeams::DrawBeamWithHalo(	Beam_t*			pbeam, 
											int				frame,
											int				rendermode,
											float*			color,
											float*			srcColor,
											const model_t	*sprite,
											const model_t	*halosprite,
											float			flHDRColorScale )
{
	Vector beamDir = pbeam->attachment[1] - pbeam->attachment[0];
	VectorNormalize( beamDir );
	
	Vector localDir = CurrentViewOrigin() - pbeam->attachment[0];
	VectorNormalize( localDir );
	
	float dotpr = DotProduct( beamDir, localDir );
	float fade;

	if ( dotpr < 0.0f )
	{
		fade = 0;
	}
	else
	{
		fade = dotpr * 2.0f;
	}

	float	distToLine;
	Vector	out;

	// Find out how close we are to the "line" of the spotlight
	CalcClosestPointOnLine( CurrentViewOrigin(), pbeam->attachment[0], pbeam->attachment[0] + ( beamDir * 2 ), out, &distToLine );

	distToLine = ( CurrentViewOrigin() - out ).Length();

	float scaleColor[3];
	float dotScale = 1.0f;
	
	// Use beam width
	float distThreshold = pbeam->width * 4.0f;

	if ( distToLine < distThreshold )
	{
		dotScale = RemapVal( distToLine, distThreshold, pbeam->width, 1.0f, 0.0f );
		dotScale = clamp( dotScale, 0.f, 1.f );
	}

	scaleColor[0] = color[0] * dotScale;
	scaleColor[1] = color[1] * dotScale;
	scaleColor[2] = color[2] * dotScale;

	if( pbeam->flags & FBEAM_HALOBEAM )
	{
		DrawSegs( NOISE_DIVISIONS, pbeam->rgNoise, sprite, frame, rendermode, pbeam->attachment[0],
			pbeam->delta, pbeam->width, pbeam->endWidth, pbeam->amplitude, pbeam->freq, pbeam->speed,
			pbeam->segments, pbeam->flags, scaleColor, pbeam->fadeLength, flHDRColorScale );
	}
	else
	{
		// Draw primary beam just shy of its end so it doesn't clip
		DrawSegs( NOISE_DIVISIONS, pbeam->rgNoise, sprite, frame, rendermode, pbeam->attachment[0], 
			pbeam->delta, pbeam->width, pbeam->width, pbeam->amplitude, pbeam->freq, pbeam->speed, 
			2, pbeam->flags, scaleColor, pbeam->fadeLength, flHDRColorScale );
	}

	Vector vSource = pbeam->attachment[0];

	pixelvis_queryparams_t params;
	params.Init( vSource, pbeam->m_haloProxySize );

	float haloFractionVisible = PixelVisibility_FractionVisible( params, pbeam->m_queryHandleHalo );
	if ( fade && haloFractionVisible > 0.0f )
	{
		//NOTENOTE: This is kinda funky when moving away and to the backside -- jdw
		float haloScale = RemapVal( distToLine, distThreshold, pbeam->width*0.5f, 1.0f, 2.0f );

		haloScale = clamp( haloScale, 1.0f, 2.0f );

		haloScale *= pbeam->haloScale;
		
		float colorFade = fade*fade;
		colorFade = clamp( colorFade, 0.f, 1.f );

		float haloColor[3];
		VectorScale( srcColor, colorFade * haloFractionVisible, haloColor );

		BeamDrawHalo( halosprite, frame, kRenderGlow, vSource, haloScale, haloColor, flHDRColorScale );
	}
}


//------------------------------------------------------------------------------
// Purpose : Draw a beam based upon the viewpoint
//------------------------------------------------------------------------------
void CViewRenderBeams::DrawLaser( Beam_t *pbeam, int frame, int rendermode, float *color, const model_t *sprite, const model_t *halosprite, float flHDRColorScale )
{
	float	color2[3];
	VectorCopy( color, color2 );

	Vector vecForward;
	Vector	beamDir	= pbeam->attachment[1] - pbeam->attachment[0];
	VectorNormalize( beamDir );
	AngleVectors( CurrentViewAngles(), &vecForward );
	float flDot = DotProduct(beamDir, vecForward);

	// abort if the player's looking along it away from the source
	if ( flDot > 0 )
	{
		return;
	}
	else
	{
		// Fade the beam if the player's not looking at the source
		float flFade = pow( flDot, 10 );

		// Fade the beam based on the player's proximity to the beam
		Vector localDir = CurrentViewOrigin() - pbeam->attachment[0];
		flDot = DotProduct( beamDir, localDir );
		Vector vecProjection = flDot * beamDir;
		float flDistance = ( localDir - vecProjection ).Length();

		if ( flDistance > 30 )
		{
			flDistance = 1 - ((flDistance - 30) / 64);
			if ( flDistance <= 0 )
			{
				flFade = 0;
			}
			else
			{
				flFade *= pow( flDistance, 3 );
			}
		}

		if (flFade < (1.0f / 255.0f))
			return;

		VectorScale( color2, flFade, color2 );

		//engine->Con_NPrintf( 6, "Fade: %f", flFade );
		//engine->Con_NPrintf( 7, "Dist: %f", flDistance );
	}

	DrawSegs( NOISE_DIVISIONS, pbeam->rgNoise, sprite, frame, rendermode, pbeam->attachment[0], pbeam->delta, pbeam->width, pbeam->endWidth, pbeam->amplitude, pbeam->freq, pbeam->speed, pbeam->segments, pbeam->flags, color2, pbeam->fadeLength);
}

//------------------------------------------------------------------------------
// Purpose : Draw a fibrous tesla beam
//------------------------------------------------------------------------------
void CViewRenderBeams::DrawTesla( Beam_t *pbeam, int frame, int rendermode, float *color, const model_t *sprite, float flHDRColorScale )
{
	DrawTeslaSegs( NOISE_DIVISIONS, pbeam->rgNoise, sprite, frame, rendermode, pbeam->attachment[0], pbeam->delta, pbeam->width, pbeam->endWidth, pbeam->amplitude, pbeam->freq, pbeam->speed, pbeam->segments, pbeam->flags, color, pbeam->fadeLength, flHDRColorScale );
}

//-----------------------------------------------------------------------------
// Purpose: Draw all beam entities
// Input  : *pbeam - 
//			frametime - 
//-----------------------------------------------------------------------------
void CViewRenderBeams::DrawBeam( Beam_t *pbeam )
{
	Assert( pbeam->delta.IsValid() );

	if ( !r_DrawBeams.GetInt() )
		return;

	// Don't draw really short beams
	if (pbeam->delta.Length() < 0.1)
	{
		return;
	}

	const model_t	*sprite;
	const model_t	*halosprite = NULL;

	if ( pbeam->modelIndex < 0 )
	{
		pbeam->die = gpGlobals->curtime;
		return;
	}
	
	sprite = modelinfo->GetModel( pbeam->modelIndex );
	if ( !sprite )
	{
		return;
	}
	
	halosprite = modelinfo->GetModel( pbeam->haloIndex );

	int frame = ( ( int )( pbeam->frame + gpGlobals->curtime * pbeam->frameRate) % pbeam->frameCount );
	int rendermode = ( pbeam->flags & FBEAM_SOLID ) ? kRenderNormal : kRenderTransAdd;

	// set color
	float srcColor[3];
	float color[3];

	srcColor[0] = pbeam->r;
	srcColor[1] = pbeam->g;
	srcColor[2] = pbeam->b;
	if ( pbeam->flags & FBEAM_FADEIN )
	{
		VectorScale( srcColor, pbeam->t, color );
	}
	else if ( pbeam->flags & FBEAM_FADEOUT )
	{
		VectorScale( srcColor, ( 1.0f - pbeam->t ), color );
	}
	else
	{
		VectorCopy( srcColor, color );
	}

	VectorScale( color, (1/255.0), color );
	VectorCopy( color, srcColor );
	VectorScale( color, ((float)pbeam->brightness / 255.0), color );

	switch( pbeam->type )
	{
	case TE_BEAMDISK:
		DrawDisk( NOISE_DIVISIONS, pbeam->rgNoise, sprite, frame, rendermode, 
			pbeam->attachment[0], pbeam->delta, pbeam->width, pbeam->amplitude, 
			pbeam->freq, pbeam->speed, pbeam->segments, color, pbeam->m_flHDRColorScale );
		break;

	case TE_BEAMCYLINDER:
		DrawCylinder( NOISE_DIVISIONS, pbeam->rgNoise, sprite, frame, rendermode, 
			pbeam->attachment[0], pbeam->delta, pbeam->width, pbeam->amplitude, 
			pbeam->freq, pbeam->speed, pbeam->segments, color, pbeam->m_flHDRColorScale );
		break;

	case TE_BEAMPOINTS:
		if (halosprite)
		{	
			DrawBeamWithHalo( pbeam, frame, rendermode, color, srcColor, sprite, halosprite, pbeam->m_flHDRColorScale );
		}
		else
		{
			DrawSegs( NOISE_DIVISIONS, pbeam->rgNoise, sprite, frame, rendermode, 
				pbeam->attachment[0], pbeam->delta, pbeam->width, pbeam->endWidth, 
				pbeam->amplitude, pbeam->freq, pbeam->speed, pbeam->segments, 
				pbeam->flags, color, pbeam->fadeLength, pbeam->m_flHDRColorScale );
		}
		break;

	case TE_BEAMFOLLOW:
		DrawBeamFollow( sprite, pbeam, frame, rendermode, gpGlobals->frametime, color, pbeam->m_flHDRColorScale );
		break;

	case TE_BEAMRING:
	case TE_BEAMRINGPOINT:
		DrawRing( NOISE_DIVISIONS, pbeam->rgNoise, Noise, sprite, frame, rendermode, 
			pbeam->attachment[0], pbeam->delta, pbeam->width, pbeam->amplitude, 
			pbeam->freq, pbeam->speed, pbeam->segments, color, pbeam->m_flHDRColorScale );
		break;

	case TE_BEAMSPLINE:
		DrawSplineSegs( NOISE_DIVISIONS, pbeam->rgNoise, sprite, halosprite,
			pbeam->haloScale, frame, rendermode, pbeam->numAttachments,
			pbeam->attachment, pbeam->width, pbeam->endWidth, pbeam->amplitude,
			pbeam->freq, pbeam->speed, pbeam->segments, pbeam->flags, color, pbeam->fadeLength, pbeam->m_flHDRColorScale );
		break;

	case TE_BEAMLASER:
		DrawLaser( pbeam, frame, rendermode, color, sprite, halosprite, pbeam->m_flHDRColorScale );
		break;

	case TE_BEAMTESLA:
		DrawTesla( pbeam, frame, rendermode, color, sprite, pbeam->m_flHDRColorScale );
		break;

	default:
		DevWarning( 1, "CViewRenderBeams::DrawBeam:  Unknown beam type %i\n", pbeam->type );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update the beam 
//-----------------------------------------------------------------------------
void CViewRenderBeams::UpdateBeamInfo( Beam_t *pBeam, BeamInfo_t &beamInfo )
{
	pBeam->attachment[0] = beamInfo.m_vecStart;
	pBeam->attachment[1] = beamInfo.m_vecEnd;
	pBeam->delta = beamInfo.m_vecEnd - beamInfo.m_vecStart;

	Assert( pBeam->delta.IsValid() );

	SetBeamAttributes( pBeam, beamInfo );
}


//-----------------------------------------------------------------------------
// Recomputes beam endpoints..
//-----------------------------------------------------------------------------
bool CViewRenderBeams::RecomputeBeamEndpoints( Beam_t *pbeam )
{
	if ( pbeam->flags & FBEAM_STARTENTITY )
	{
		if (ComputeBeamEntPosition( pbeam->entity[0], pbeam->attachmentIndex[0], (pbeam->flags & FBEAM_USE_HITBOXES) != 0, pbeam->attachment[0] ))
		{
			pbeam->flags |= FBEAM_STARTVISIBLE;
		}
		else if (! (pbeam->flags & FBEAM_FOREVER))
		{
			pbeam->flags &= ~(FBEAM_STARTENTITY);
		}
		else
		{
			// DevWarning( 1,"can't find start entity\n");
//			return false;
		}

		// If we've never seen the start entity, don't display
		if ( !(pbeam->flags & FBEAM_STARTVISIBLE) )
			return false;
	}

	if ( pbeam->flags & FBEAM_ENDENTITY )
	{
		if (ComputeBeamEntPosition( pbeam->entity[1], pbeam->attachmentIndex[1], (pbeam->flags & FBEAM_USE_HITBOXES) != 0, pbeam->attachment[1] ))
		{
			pbeam->flags |= FBEAM_ENDVISIBLE;
		}
		else if (! (pbeam->flags & FBEAM_FOREVER))
		{
			pbeam->flags &= ~(FBEAM_ENDENTITY);
			pbeam->die = gpGlobals->curtime;
			return false;
		}
		else
		{
			return false;
		}

		// If we've never seen the end entity, don't display
		if ( !(pbeam->flags & FBEAM_ENDVISIBLE) )
			return false;
	}

	return true;
}

#ifdef PORTAL
	bool bBeamDrawingThroughPortal = false;
#endif

//-----------------------------------------------------------------------------
// Draws a single beam
//-----------------------------------------------------------------------------
void CViewRenderBeams::DrawBeam( C_Beam* pbeam, ITraceFilter *pEntityBeamTraceFilter )
{
	Beam_t beam;

	// Set up the beam.
	int beamType = pbeam->GetType();

	BeamInfo_t beamInfo;
	beamInfo.m_vecStart = pbeam->GetAbsStartPos();
	beamInfo.m_vecEnd = pbeam->GetAbsEndPos();
	beamInfo.m_pStartEnt = beamInfo.m_pEndEnt = NULL;
	beamInfo.m_nModelIndex = pbeam->GetModelIndex();
	beamInfo.m_nHaloIndex = pbeam->m_nHaloIndex;
	beamInfo.m_flHaloScale = pbeam->m_fHaloScale;
	beamInfo.m_flLife = 0;
	beamInfo.m_flWidth = pbeam->GetWidth();
	beamInfo.m_flEndWidth = pbeam->GetEndWidth();
	beamInfo.m_flFadeLength = pbeam->GetFadeLength();
	beamInfo.m_flAmplitude = pbeam->GetNoise();
	beamInfo.m_flBrightness = pbeam->GetFxBlend();
	beamInfo.m_flSpeed = pbeam->GetScrollRate();

#ifdef PORTAL	// Beams need to recursively draw through portals
	// Trace to see if we've intersected a portal
	float fEndFraction;
	Ray_t rayBeam;

	bool bIsReversed = ( pbeam->GetBeamFlags() & FBEAM_REVERSED ) != 0x0;

	Vector vRayStartPoint, vRayEndPoint;

	vRayStartPoint = beamInfo.m_vecStart;
	vRayEndPoint = beamInfo.m_vecEnd;

	if ( beamType == BEAM_ENTPOINT || beamType == BEAM_ENTS || beamType == BEAM_LASER )
	{
		ComputeBeamEntPosition( pbeam->m_hAttachEntity[0], pbeam->m_nAttachIndex[0], false, vRayStartPoint );
		ComputeBeamEntPosition( pbeam->m_hAttachEntity[1], pbeam->m_nAttachIndex[1], false, vRayEndPoint );
	}

	if ( !bIsReversed )
		rayBeam.Init( vRayStartPoint, vRayEndPoint );
	else
		rayBeam.Init( vRayEndPoint, vRayStartPoint );

	CBaseEntity *pStartEntity = pbeam->GetStartEntityPtr();

	CTraceFilterSkipClassname traceFilter( pStartEntity, "prop_energy_ball", COLLISION_GROUP_NONE );

	if ( !pEntityBeamTraceFilter && pStartEntity )
		pEntityBeamTraceFilter = pStartEntity->GetBeamTraceFilter();

	CTraceFilterChain traceFilterChain( &traceFilter, pEntityBeamTraceFilter );

	C_Prop_Portal *pPortal = UTIL_Portal_TraceRay_Beam( rayBeam, MASK_SHOT, &traceFilterChain, &fEndFraction );

	// Get the point that we hit a portal or wall
	Vector vEndPoint = rayBeam.m_Start + rayBeam.m_Delta * fEndFraction;

	if ( pPortal )
	{
		// Prevent infinite recursion by lower the brightness each call
		int iOldBrightness = pbeam->GetBrightness();

		if ( iOldBrightness > 16 )
		{
			// Remember the old values of the beam before changing it for the next call
			Vector vOldStart = pbeam->GetAbsStartPos();
			Vector vOldEnd = pbeam->GetAbsEndPos();
			//float fOldWidth = pbeam->GetEndWidth();
			C_BaseEntity *pOldStartEntity = pbeam->GetStartEntityPtr();
			C_BaseEntity *pOldEndEntity = pbeam->GetEndEntityPtr();
			int iOldStartAttachment = pbeam->GetStartAttachment();
			int iOldEndAttachment = pbeam->GetEndAttachment();
			int iOldType = pbeam->GetType();

			// Get the transformed positions of the sub beam in the other portal's space
			Vector vTransformedStart, vTransformedEnd;
			VMatrix matThisToLinked = pPortal->MatrixThisToLinked();
			UTIL_Portal_PointTransform( matThisToLinked, vEndPoint, vTransformedStart );
			UTIL_Portal_PointTransform( matThisToLinked, rayBeam.m_Start + rayBeam.m_Delta, vTransformedEnd );

			// Set up the sub beam for the next call
			pbeam->SetBrightness( iOldBrightness - 16 );
			if ( bIsReversed )
				pbeam->PointsInit( vTransformedEnd, vTransformedStart );
			else
				pbeam->PointsInit( vTransformedStart, vTransformedEnd );
			if ( bIsReversed )
				pbeam->SetEndWidth( pbeam->GetWidth() );
			pbeam->SetStartEntity( pPortal->m_hLinkedPortal );

			// Draw the sub beam
			bBeamDrawingThroughPortal = true;
			DrawBeam( pbeam, pEntityBeamTraceFilter );
			bBeamDrawingThroughPortal = true;

			// Restore the original values
			pbeam->SetBrightness( iOldBrightness );
			pbeam->SetStartPos( vOldStart );
			pbeam->SetEndPos( vOldEnd );
			//if ( bIsReversed )
			//	pbeam->SetEndWidth( fOldWidth );
			if ( pOldStartEntity )
				pbeam->SetStartEntity( pOldStartEntity );
			if ( pOldEndEntity )
				pbeam->SetEndEntity( pOldEndEntity );
			pbeam->SetStartAttachment( iOldStartAttachment );
			pbeam->SetEndAttachment( iOldEndAttachment );
			pbeam->SetType( iOldType );

			// Doesn't use a hallow or taper the beam because we recursed
			beamInfo.m_nHaloIndex = 0;
			if ( !bIsReversed )
				beamInfo.m_flEndWidth = beamInfo.m_flWidth;
		}
	}

	// Clip to the traced end point (portal or wall)
	if ( bBeamDrawingThroughPortal )
	{
		if ( bIsReversed )
			beamInfo.m_vecStart = vEndPoint;
		else
			beamInfo.m_vecEnd = vEndPoint;
	}

	bBeamDrawingThroughPortal = false;
#endif

	SetupBeam( &beam, beamInfo );

	beamInfo.m_nStartFrame = pbeam->m_fStartFrame;
	beamInfo.m_flFrameRate = pbeam->m_flFrameRate;
	beamInfo.m_flRed = pbeam->m_clrRender->r;
	beamInfo.m_flGreen = pbeam->m_clrRender->g;
	beamInfo.m_flBlue = pbeam->m_clrRender->b;

	SetBeamAttributes( &beam, beamInfo );

	if ( pbeam->m_nHaloIndex > 0 )
	{
		// HACKHACK: heuristic to estimate proxy size.  Revisit this!
		float size = 1.0f + (pbeam->m_fHaloScale * pbeam->m_fWidth / pbeam->m_fEndWidth);
		size = clamp( size, 1.0f, 8.0f );
		beam.m_queryHandleHalo = &pbeam->m_queryHandleHalo;
		beam.m_haloProxySize = size;
	}
	else
	{
		beam.m_queryHandleHalo = NULL;
	}

	// Handle code from relinking.
	switch( beamType )
	{
		case BEAM_ENTS:
		{
			beam.type			= TE_BEAMPOINTS;
			beam.flags			= FBEAM_STARTENTITY | FBEAM_ENDENTITY;
			beam.entity[0]		= pbeam->m_hAttachEntity[0];
			beam.attachmentIndex[0]	= pbeam->m_nAttachIndex[0];
			beam.entity[1]		= pbeam->m_hAttachEntity[1];
			beam.attachmentIndex[1]	= pbeam->m_nAttachIndex[1];
			beam.numAttachments	= pbeam->m_nNumBeamEnts;
			break;
		}
		case BEAM_LASER:
		{
			beam.type			= TE_BEAMLASER;
			beam.flags			= FBEAM_STARTENTITY | FBEAM_ENDENTITY;
			beam.entity[0]		= pbeam->m_hAttachEntity[0];
			beam.attachmentIndex[0]	= pbeam->m_nAttachIndex[0];
			beam.entity[1]		= pbeam->m_hAttachEntity[1];
			beam.attachmentIndex[1]	= pbeam->m_nAttachIndex[1];
			beam.numAttachments	= pbeam->m_nNumBeamEnts;
			break;
		}
		case BEAM_SPLINE:
		{
			beam.type			= TE_BEAMSPLINE;
			beam.flags			= FBEAM_STARTENTITY | FBEAM_ENDENTITY;
			beam.numAttachments	= pbeam->m_nNumBeamEnts;
			for (int i=0;i<beam.numAttachments;i++)
			{
				beam.entity[i]		= pbeam->m_hAttachEntity[i];
				beam.attachmentIndex[i]	= pbeam->m_nAttachIndex[i];
			}
			break;
		}
		case BEAM_ENTPOINT:
		{
			beam.type			= TE_BEAMPOINTS;
			beam.flags = 0;
			beam.entity[0]		= pbeam->m_hAttachEntity[0];
			beam.attachmentIndex[0]	= pbeam->m_nAttachIndex[0];
			beam.entity[1]		= pbeam->m_hAttachEntity[1];
			beam.attachmentIndex[1]	= pbeam->m_nAttachIndex[1];
			if ( beam.entity[0].Get() )
			{
				beam.flags |= FBEAM_STARTENTITY;
			}
			if ( beam.entity[1].Get() )
			{
				beam.flags |= FBEAM_ENDENTITY;
			}
			beam.numAttachments	= pbeam->m_nNumBeamEnts;
			break;
		}
		case BEAM_POINTS:
			// Already set up
			break;
	}

	beam.flags |= pbeam->GetBeamFlags() & (FBEAM_SINENOISE|FBEAM_SOLID|FBEAM_SHADEIN|FBEAM_SHADEOUT|FBEAM_NOTILE);

	if ( beam.entity[0] )
	{
		// don't draw viewmodel effects in reflections
		if ( CurrentViewID() == VIEW_REFLECTION )
		{
			int group = beam.entity[0]->GetRenderGroup();
			if (group == RENDER_GROUP_VIEW_MODEL_TRANSLUCENT || group == RENDER_GROUP_VIEW_MODEL_OPAQUE)
				return;
		}
	}

	beam.m_flHDRColorScale = pbeam->GetHDRColorScale();

	// Draw it
	UpdateBeam( &beam, gpGlobals->frametime );
	DrawBeam( &beam );
}
