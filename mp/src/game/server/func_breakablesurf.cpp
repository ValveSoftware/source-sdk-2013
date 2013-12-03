//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A planar textured surface that breaks into increasingly smaller fragments
//			as it takes damage. Undamaged pieces remain attached to the world
//			until they are damaged. Used for window panes.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ndebugoverlay.h"
#include "filters.h"
#include "player.h"
#include "func_breakablesurf.h"
#include "shattersurfacetypes.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "globals.h"
#include "physics_impact_damage.h"
#include "te_effect_dispatch.h"

//=============================================================================
// HPE_BEGIN
// [dwenger] Necessary for stats tracking
//=============================================================================
#include "gamestats.h"
//=============================================================================
// HPE_END
//=============================================================================

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Spawn flags
#define SF_BREAKABLESURF_CRACK_DECALS				0x00000001
#define SF_BREAKABLESURF_DAMAGE_FROM_HELD_OBJECTS	0x00000002

//#############################################################################
//  > CWindowPane
//#############################################################################
#define WINDOW_PANEL_SIZE			12
#define WINDOW_SMALL_SHARD_SIZE		4
#define WINDOW_LARGE_SHARD_SIZE		7
#define WINDOW_MAX_SUPPORT			6.75
#define WINDOW_BREAK_SUPPORT		0.20

#define WINDOW_PANE_BROKEN			-1
#define WINDOW_PANE_HEALTHY			1

// Also defined in WC
#define QUAD_ERR_NONE				0
#define QUAD_ERR_MULT_FACES			1
#define QUAD_ERR_NOT_QUAD			2

//
// func_breakable - bmodel that breaks into pieces after taking damage
//
LINK_ENTITY_TO_CLASS( window_pane, CWindowPane );
BEGIN_DATADESC( CWindowPane )

	// Function Pointers
	DEFINE_FUNCTION( Die ),
	DEFINE_FUNCTION( PaneTouch ),

END_DATADESC()


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWindowPane::Spawn( void )
{
    Precache( );    
  
	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLYGRAVITY );
	m_takedamage = DAMAGE_YES;
 	
	SetCollisionGroup( COLLISION_GROUP_BREAKABLE_GLASS ); 

	SetModel( "models/brokenglass_piece.mdl" );//set size and link into world.
}

void CWindowPane::Precache( void )
{
	PrecacheModel( "models/brokenglass_piece.mdl" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pOther - 
//-----------------------------------------------------------------------------
void CWindowPane::PaneTouch( CBaseEntity *pOther )
{
	if (pOther &&
		pOther->GetCollisionGroup() != COLLISION_GROUP_BREAKABLE_GLASS)
	{
		Die();
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWindowPane::Die( void )
{
	Vector flForce = -1 * GetAbsVelocity();

	CPASFilter filter( GetAbsOrigin() );
	te->ShatterSurface( filter, 0.0,
					 &GetAbsOrigin(), &GetAbsAngles(), 
					 &GetAbsVelocity(), &GetAbsOrigin(),
					 WINDOW_PANEL_SIZE, WINDOW_PANEL_SIZE,WINDOW_SMALL_SHARD_SIZE,SHATTERSURFACE_GLASS,
					 255,255,255,255,255,255);

	UTIL_Remove(this);
}

///------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CWindowPane* CWindowPane::CreateWindowPane( const Vector &vecOrigin, const QAngle &vecAngles )
{
	CWindowPane *pGlass = (CWindowPane*)CreateEntityByName( "window_pane" );
	if ( !pGlass )
	{
		Msg( "NULL Ent in CreateWindowPane!\n" );
		return NULL;
	}

	if ( pGlass->edict() )
	{
		pGlass->SetLocalOrigin( vecOrigin );
		pGlass->SetLocalAngles( vecAngles );
		pGlass->Spawn();
		pGlass->SetTouch(&CWindowPane::PaneTouch);
		pGlass->SetLocalAngularVelocity( RandomAngle(-50,50) );
		pGlass->m_nBody = random->RandomInt(0,2);
	}
	return pGlass;
}


//####################################################################################
//	> CBreakableSurface
//####################################################################################
LINK_ENTITY_TO_CLASS( func_breakable_surf, CBreakableSurface );

BEGIN_DATADESC( CBreakableSurface )

	DEFINE_KEYFIELD( m_nSurfaceType,		FIELD_INTEGER,	"surfacetype"),
	DEFINE_KEYFIELD( m_nFragility,		FIELD_INTEGER,	"fragility"),
	DEFINE_KEYFIELD( m_vLLVertex,		FIELD_VECTOR,  "lowerleft" ),
	DEFINE_KEYFIELD( m_vULVertex,		FIELD_VECTOR,  "upperleft" ),
	DEFINE_KEYFIELD( m_vLRVertex,		FIELD_VECTOR,  "lowerright" ),
	DEFINE_KEYFIELD( m_vURVertex,		FIELD_VECTOR,  "upperright" ),
	DEFINE_KEYFIELD( m_nQuadError,		FIELD_INTEGER, "error" ),

	DEFINE_FIELD( m_nNumWide,			FIELD_INTEGER),	
	DEFINE_FIELD( m_nNumHigh,			FIELD_INTEGER),	
	DEFINE_FIELD( m_flPanelWidth,		FIELD_FLOAT),	
	DEFINE_FIELD( m_flPanelHeight,	FIELD_FLOAT),	
	DEFINE_FIELD( m_vNormal,			FIELD_VECTOR),	
	DEFINE_FIELD( m_vCorner,			FIELD_POSITION_VECTOR),	
	DEFINE_FIELD( m_bIsBroken,		FIELD_BOOLEAN),	
	DEFINE_FIELD( m_nNumBrokenPanes,	FIELD_INTEGER),	
	
	// UNDONE: How to load save this?  Need a way to update
	//		   the client about the state of the window upon load...
	//			We should use client-side save/load to fix this problem.
	DEFINE_AUTO_ARRAY2D( m_flSupport,	FIELD_FLOAT),	
	DEFINE_ARRAY( m_RawPanelBitVec, FIELD_BOOLEAN, MAX_NUM_PANELS*MAX_NUM_PANELS ),

	// Function Pointers
	DEFINE_THINKFUNC( BreakThink ),
	DEFINE_ENTITYFUNC( SurfaceTouch ),

	DEFINE_INPUTFUNC( FIELD_VECTOR,	"Shatter", InputShatter ),

	// DEFINE_FIELD( m_ForceUpdateClientData, CBitVec < MAX_PLAYERS > ),  // No need to save/restore this, it's just a temporary flag field
END_DATADESC()


IMPLEMENT_SERVERCLASS_ST(CBreakableSurface, DT_BreakableSurface)
	SendPropInt(SENDINFO(m_nNumWide), 8,  SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nNumHigh), 8, SPROP_UNSIGNED),
	SendPropFloat(SENDINFO(m_flPanelWidth), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_flPanelHeight), 0, SPROP_NOSCALE),
	SendPropVector(SENDINFO(m_vNormal), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vCorner), -1, SPROP_COORD),
	SendPropInt(SENDINFO(m_bIsBroken), 1, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nSurfaceType), 2, SPROP_UNSIGNED),
	SendPropArray3(SENDINFO_ARRAY3(m_RawPanelBitVec), SendPropInt( SENDINFO_ARRAY( m_RawPanelBitVec ), 1, SPROP_UNSIGNED ) ),
END_SEND_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBreakableSurface::Precache(void)
{
	UTIL_PrecacheOther( "window_pane" );

	// Load the edge types and styles for the specific surface type
	if (m_nSurfaceType == SHATTERSURFACE_TILE)
	{
		PrecacheMaterial( "models/brokentile/tilebroken_03a" );
		PrecacheMaterial( "models/brokentile/tilebroken_03b" );
		PrecacheMaterial( "models/brokentile/tilebroken_03c" );
		PrecacheMaterial( "models/brokentile/tilebroken_03d" );

		PrecacheMaterial( "models/brokentile/tilebroken_02a" );
		PrecacheMaterial( "models/brokentile/tilebroken_02b" );
		PrecacheMaterial( "models/brokentile/tilebroken_02c" );
		PrecacheMaterial( "models/brokentile/tilebroken_02d" );

		PrecacheMaterial( "models/brokentile/tilebroken_01a" );
		PrecacheMaterial( "models/brokentile/tilebroken_01b" );
		PrecacheMaterial( "models/brokentile/tilebroken_01c" );
		PrecacheMaterial( "models/brokentile/tilebroken_01d" );
	}
	else
	{
		PrecacheMaterial( "models/brokenglass/glassbroken_solid" );
		PrecacheMaterial( "models/brokenglass/glassbroken_01a" );
		PrecacheMaterial( "models/brokenglass/glassbroken_01b" );
		PrecacheMaterial( "models/brokenglass/glassbroken_01c" );
		PrecacheMaterial( "models/brokenglass/glassbroken_01d" );
		PrecacheMaterial( "models/brokenglass/glassbroken_02a" );
		PrecacheMaterial( "models/brokenglass/glassbroken_02b" );
		PrecacheMaterial( "models/brokenglass/glassbroken_02c" );
		PrecacheMaterial( "models/brokenglass/glassbroken_02d" );
		PrecacheMaterial( "models/brokenglass/glassbroken_03a" );
		PrecacheMaterial( "models/brokenglass/glassbroken_03b" );
		PrecacheMaterial( "models/brokenglass/glassbroken_03c" );
		PrecacheMaterial( "models/brokenglass/glassbroken_03d" );
	}

	BaseClass::Precache();
}


//------------------------------------------------------------------------------
// Purpose : Window has been touched.  Break out pieces based on touching
//			 entity's bounding box
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBreakableSurface::SurfaceTouch( CBaseEntity *pOther )
{
	// If tile only break if object is moving fast
	if (m_nSurfaceType == SHATTERSURFACE_TILE)
	{
		Vector vVel;
		pOther->GetVelocity( &vVel, NULL );
		if (vVel.Length() < 500)
		{
			return;
		}
	}

	// Find nearest point on plane for max
	Vector vecAbsMins, vecAbsMaxs;
	pOther->CollisionProp()->WorldSpaceAABB( &vecAbsMins, &vecAbsMaxs );
	Vector vToPlane		= (vecAbsMaxs - m_vCorner);
	float  vDistToPlane = DotProduct(m_vNormal,vToPlane);
	Vector vTouchPos	= vecAbsMaxs + vDistToPlane*m_vNormal;

	float flMinsWidth,flMinsHeight;
	PanePos(vTouchPos, &flMinsWidth, &flMinsHeight);

	// Find nearest point on plane for mins
	vToPlane		= (vecAbsMins - m_vCorner);
	vDistToPlane = DotProduct(m_vNormal,vToPlane);
	vTouchPos	= vecAbsMins + vDistToPlane*m_vNormal;

	float flMaxsWidth,flMaxsHeight;
	PanePos(vTouchPos, &flMaxsWidth, &flMaxsHeight);

	int nMinWidth = Floor2Int(MAX(0,		MIN(flMinsWidth,flMaxsWidth)));
	int nMaxWidth = Ceil2Int(MIN(m_nNumWide,MAX(flMinsWidth,flMaxsWidth)));

	int nMinHeight = Floor2Int(MAX(0,		MIN(flMinsHeight,flMaxsHeight)));
	int nMaxHeight = Ceil2Int(MIN(m_nNumHigh,MAX(flMinsHeight,flMaxsHeight)));

	Vector vHitVel;
	pOther->GetVelocity( &vHitVel, NULL );

	// Move faster then penetrating object so can see shards
	vHitVel *= 5;

	// If I'm not broken yet, break me
	if ( !m_bIsBroken )
	{
		Die( pOther, vHitVel );
	}

	for (int height=nMinHeight;height<nMaxHeight;height++)
	{
		// Randomly break the one before so it doesn't look square
		if (random->RandomInt(0,1))
		{
			ShatterPane(nMinWidth-1, height,vHitVel,pOther->GetLocalOrigin());
		}
		for (int width=nMinWidth;width<nMaxWidth;width++)
		{
			ShatterPane(width, height,vHitVel,pOther->GetLocalOrigin());
		}
		// Randomly break the one after so it doesn't look square
		if (random->RandomInt(0,1))
		{
			ShatterPane(nMaxWidth+1, height,vHitVel,pOther->GetLocalOrigin());
		}
	}
}

//------------------------------------------------------------------------------
// Purpose : Only take damage in trace attack
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CBreakableSurface::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !m_bIsBroken && info.GetDamageType() == DMG_CRUSH )
	{
		// physics will kill me now
		Die( info.GetAttacker(), info.GetDamageForce() );
		return 0;
	}

	if ( m_nSurfaceType == SHATTERSURFACE_GLASS && info.GetDamageType() & DMG_BLAST )
	{
		Vector vecDir = info.GetInflictor()->GetAbsOrigin() - WorldSpaceCenter();
		VectorNormalize( vecDir );
		Die( info.GetAttacker(), vecDir );
		return 0;
	}

	// Accept slash damage, too. Manhacks and such.
	if ( m_nSurfaceType == SHATTERSURFACE_GLASS && (info.GetDamageType() & DMG_SLASH) )
	{
		Die( info.GetAttacker(), info.GetDamageForce() );
		return 0;
	}
	

	return 0;
}


//------------------------------------------------------------------------------
// Purpose: Accepts damage and breaks if health drops below zero.
//------------------------------------------------------------------------------
void CBreakableSurface::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
    //=============================================================================
    // HPE_BEGIN:
    // [dwenger] Window break stat tracking
    //=============================================================================

    // Make sure this pane has not already been shattered
    bool bWasBroken = m_bIsBroken;

    //=============================================================================
    // HPE_END
    //=============================================================================

	// Decrease health
	m_iHealth -= info.GetDamage();
	m_OnHealthChanged.Set( m_iHealth, info.GetAttacker(), this );

	// If I'm not broken yet, break me
	if (!m_bIsBroken )
	{
		Vector vSurfDir = ptr->endpos - ptr->startpos;
		Die( info.GetAttacker(), vSurfDir );
	}

	if (info.GetDamageType() & (DMG_BULLET | DMG_CLUB))
	{
		// Figure out which panel has taken the damage and break it
		float flWidth,flHeight;
		PanePos(ptr->endpos,&flWidth,&flHeight);
		int nWidth  = flWidth;
		int nHeight = flHeight;
		
		if ( ShatterPane(nWidth, nHeight,vecDir*500,ptr->endpos) )
		{
            //=============================================================================
            // HPE_BEGIN:
            // [dwenger] Window break stat tracking
            //=============================================================================

            CBasePlayer* pAttacker = ToBasePlayer(info.GetAttacker());
            if ( ( pAttacker ) && ( !bWasBroken ) )
            {
                gamestats->Event_WindowShattered( pAttacker );
            }

            //=============================================================================
            // HPE_END
            //=============================================================================

			// Do an impact hit
			CEffectData	data;

			data.m_vNormal = ptr->plane.normal;
			data.m_vOrigin = ptr->endpos;

			CPASFilter filter( data.m_vOrigin );

			// client cannot trace against triggers
			filter.SetIgnorePredictionCull( true );

			te->DispatchEffect( filter, 0.0, data.m_vOrigin, "GlassImpact", data );
		}

		if (m_nSurfaceType == SHATTERSURFACE_GLASS)
		{
			// Break nearby panes if damages was near pane edge
			float flWRem = flWidth  - nWidth;
			float flHRem = flHeight - nHeight;

			if (flWRem > 0.8 && nWidth != m_nNumWide-1)
			{
				ShatterPane(nWidth+1, nHeight,vecDir*500,ptr->endpos);
			}
			else if (flWRem < 0.2 && nWidth != 0)
			{
				ShatterPane(nWidth-1, nHeight,vecDir*500,ptr->endpos);
			}
			if (flHRem > 0.8 && nHeight != m_nNumHigh-1)
			{
				ShatterPane(nWidth, nHeight+1,vecDir*500,ptr->endpos);
			}
			else if (flHRem < 0.2 && nHeight != 0)
			{
				ShatterPane(nWidth, nHeight-1,vecDir*500,ptr->endpos);
			}

			// Occasionally break the pane above me
			if (random->RandomInt(0,1)==0)
			{
				ShatterPane(nWidth, nHeight+1,vecDir*1000,ptr->endpos);
				// Occasionally break the pane above that
				if (random->RandomInt(0,1)==0)
				{
					ShatterPane(nWidth, nHeight+2,vecDir*1000,ptr->endpos);
				}		
			}
		}
	}
	else if (info.GetDamageType() & (DMG_SONIC | DMG_BLAST))
	{
		// ----------------------------------------
		// If it's tile blow out nearby tiles
		// ----------------------------------------
		if (m_nSurfaceType == SHATTERSURFACE_TILE)
		{
			// Figure out which panel has taken the damage and break it
			float flWidth,flHeight;
			if (info.GetAttacker())
			{
				PanePos(info.GetAttacker()->GetAbsOrigin(),&flWidth,&flHeight);
			}
			else
			{
				PanePos(ptr->endpos,&flWidth,&flHeight);
			}
			int nWidth  = flWidth;
			int nHeight = flHeight;

			// Blow out a roughly circular patch of tile with some randomness
			for (int width =nWidth-4;width<nWidth+4;width++)
			{
				for (int height =nHeight-4;height<nHeight+4;height++)
				{
					if ((abs(nWidth-width)+abs(nHeight-height))<random->RandomInt(2,5))
					{
						ShatterPane(width, height,vecDir*500,ptr->endpos);
					}
				}
			}
		}
		// ----------------------------------------
		// If it's glass blow out the whole window
		// ----------------------------------------
		else
		{
            //=============================================================================
            // HPE_BEGIN:
            // [pfreese] Window break stat tracking
            //=============================================================================

            CBasePlayer* pAttacker = ToBasePlayer(info.GetAttacker());
            if ( ( pAttacker ) && ( !bWasBroken ) )
            {
                gamestats->Event_WindowShattered( pAttacker );
            }

            //=============================================================================
            // HPE_END
            //=============================================================================

			float flDot = DotProduct(m_vNormal,vecDir);

#ifdef CSTRIKE_DLL
			float damageMultiplier = info.GetDamage();
#else
			float damageMultiplier = 1.0f;
#endif

			Vector vBlastDir;
			if (flDot > 0)
			{
				vBlastDir = damageMultiplier * 3000 * m_vNormal;
			}
			else
			{
				vBlastDir = damageMultiplier * -3000 * m_vNormal;
			}

			// Has the window already been destroyed?
			if (m_nNumBrokenPanes >= m_nNumWide*m_nNumHigh)
			{
				return;
			}
			// ---------------------------------------------------------------
			// If less than 10% of my panels have been broken, blow me 
			// up in one large glass shatter
			// ---------------------------------------------------------------
			else if ( m_nNumBrokenPanes < 0.1*(m_nNumWide*m_nNumHigh))
			{
				QAngle vAngles;
				VectorAngles(-1*m_vNormal,vAngles);

				CreateShards(m_vCorner, vAngles,vBlastDir, ptr->endpos, 
							m_nNumWide*m_flPanelWidth, m_nNumHigh*m_flPanelHeight, WINDOW_LARGE_SHARD_SIZE);
			}
			// ---------------------------------------------------------------
			// Otherwise break in the longest vertical strips possible
			// (to cut down on the network bandwidth)
			// ---------------------------------------------------------------
			else
			{
				QAngle vAngles;
				VectorAngles(-1*m_vNormal,vAngles);
				Vector vWidthDir,vHeightDir;
				AngleVectors(vAngles,NULL,&vWidthDir,&vHeightDir);

				for (int width=0;width<m_nNumWide;width++)
				{
					int height;
					int nHCount = 0;
					for ( height=0;height<m_nNumHigh;height++)
					{
						// Keep count of how many panes
						if (!IsBroken(width,height))
						{
							nHCount++;
						}
						// Shatter the strip and start counting again
						else if (nHCount > 0)
						{
							Vector vBreakPos = m_vCorner + 
													(width*vWidthDir*m_flPanelWidth) + 
													((height-nHCount)*vHeightDir*m_flPanelHeight);

							CreateShards(vBreakPos, vAngles,
								 vBlastDir,	  ptr->endpos,
								 m_flPanelWidth, nHCount*m_flPanelHeight,
								 WINDOW_LARGE_SHARD_SIZE);

							nHCount = 0;
						}
					}
					if (nHCount)
					{
						Vector vBreakPos = m_vCorner + 
												(width*vWidthDir*m_flPanelWidth) + 
												((height-nHCount)*vHeightDir*m_flPanelHeight);

						CreateShards(vBreakPos, vAngles,
								 vBlastDir,	  ptr->endpos,
								 m_flPanelWidth,nHCount*m_flPanelHeight,
								 WINDOW_LARGE_SHARD_SIZE);
					}
				}
			}

			BreakAllPanes();
		}
	}
}

//------------------------------------------------------------------------------
// Purpose: Break into panels
// Input  : pBreaker - 
//			vDir - 
//-----------------------------------------------------------------------------
void CBreakableSurface::Die( CBaseEntity *pBreaker, const Vector &vAttackDir )
{
	if ( m_bIsBroken )
		return;

	// Play a break sound
	PhysBreakSound( this, VPhysicsGetObject(), GetAbsOrigin() );

	m_bIsBroken = true;
	m_iHealth = 0.0f;

	if (pBreaker)
	{
		m_OnBreak.FireOutput( pBreaker, this );
	}
	else
	{
		m_OnBreak.FireOutput( this, this );
	}

	float flDir = -1;

	if ( vAttackDir.LengthSqr() > 0.001 )
	{
		float flDot = DotProduct( m_vNormal, vAttackDir );
		if (flDot < 0)
		{
			m_vLLVertex += m_vNormal;
			m_vLRVertex += m_vNormal;
			m_vULVertex += m_vNormal;
			m_vURVertex += m_vNormal;
			m_vNormal	*= -1;
			flDir		=   1;
		}
	}

	// -------------------------------------------------------
	// The surface has two sides, when we are killed pick 
	// the side that the damage came from 
	// -------------------------------------------------------
	Vector vWidth		= m_vLLVertex - m_vLRVertex;
	Vector vHeight		= m_vLLVertex - m_vULVertex;
	CrossProduct( vWidth, vHeight, m_vNormal.GetForModify() );
	VectorNormalize(m_vNormal.GetForModify());

	// ---------------------------------------------------
	//  Make sure width and height are oriented correctly
	// ---------------------------------------------------
	QAngle vAngles;
	VectorAngles(-1*m_vNormal,vAngles);
	Vector vWidthDir,vHeightDir;
	AngleVectors(vAngles,NULL,&vWidthDir,&vHeightDir);

	float flWDist = DotProduct(vWidthDir,vWidth);
	if (fabs(flWDist)<0.5)
	{
		Vector vSaveHeight	= vHeight;
		vHeight				= vWidth * flDir;
		vWidth				= vSaveHeight * flDir;
	}

	// -------------------------------------------------
	// Find which corner to use
	// -------------------------------------------------
	bool bLeft  = (DotProduct(vWidthDir,vWidth)   < 0);
	bool bLower = (DotProduct(vHeightDir,vHeight) < 0);
	if (bLeft)
	{
		m_vCorner = bLower ? m_vLLVertex : m_vULVertex;
	}
	else 
	{
		m_vCorner = bLower ? m_vLRVertex : m_vURVertex;
	}

	// -------------------------------------------------
	//  Calculate the number of panels
	// -------------------------------------------------
	float flWidth		= vWidth.Length();
	float flHeight		= vHeight.Length();
	m_nNumWide			= flWidth  / WINDOW_PANEL_SIZE;
	m_nNumHigh			= flHeight / WINDOW_PANEL_SIZE;

	// If to many panels make panel size bigger
	if (m_nNumWide > MAX_NUM_PANELS) m_nNumWide = MAX_NUM_PANELS;
	if (m_nNumHigh > MAX_NUM_PANELS) m_nNumHigh = MAX_NUM_PANELS;

	m_flPanelWidth	= flWidth  / m_nNumWide;
	m_flPanelHeight	= flHeight / m_nNumHigh;
	
	// Initialize panels
	for (int w=0;w<MAX_NUM_PANELS;w++)
	{ 
		for (int h=0;h<MAX_NUM_PANELS;h++)
		{
			SetSupport( w, h, WINDOW_PANE_HEALTHY );
		}
	}

	// Reset onground flags for any entity that may 
	// have been standing on me
	ResetOnGroundFlags();

	VPhysicsDestroyObject();
	AddSolidFlags( FSOLID_TRIGGER );
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetTouch(&CBreakableSurface::SurfaceTouch);
}

//------------------------------------------------------------------------------
// Purpose: Set an instaneous force on the rope.
// Input  : Force vector.
//------------------------------------------------------------------------------
void CBreakableSurface::InputShatter( inputdata_t &inputdata )
{
	Vector vecShatterInfo;
	inputdata.value.Vector3D(vecShatterInfo);

	if (!m_bIsBroken)
	{
		Die( NULL, vec3_origin );
	}

	// Figure out which panel has taken the damage and break it
	float flCenterX = vecShatterInfo.x * m_nNumWide;
	float flCenterY = vecShatterInfo.y * m_nNumHigh;

	// Bah: m_flPanelWidth is the width of a single panel
	int nMinX = (int)(flCenterX - vecShatterInfo.z / m_flPanelWidth);
	int nMaxX = (int)(flCenterX + vecShatterInfo.z / m_flPanelWidth) + 1;
	if (nMinX < 0)
		nMinX = 0;
	if (nMaxX > m_nNumWide)
		nMaxX = m_nNumWide;

	int nMinY = (int)(flCenterY - vecShatterInfo.z / m_flPanelHeight);
	int nMaxY = (int)(flCenterY + vecShatterInfo.z / m_flPanelHeight) + 1;

	if (nMinY < 0)
		nMinY = 0;
	if (nMaxY > m_nNumHigh)
		nMaxY = m_nNumHigh;

	QAngle vAngles;
	VectorAngles(-1*m_vNormal,vAngles);
	Vector vWidthDir,vHeightDir;
	AngleVectors(vAngles,NULL,&vWidthDir,&vHeightDir);

	// Blow out a roughly circular of tile with some randomness
	Vector2D vecActualCenter( flCenterX * m_flPanelWidth, flCenterY * m_flPanelHeight ); 
	for (int width = nMinX; width < nMaxX; width++)
	{
		for (int height = nMinY; height < nMaxY; height++)
		{
			Vector2D pt( (width + 0.5f) * m_flPanelWidth, (height + 0.5f) * m_flPanelWidth );
			if ( pt.DistToSqr(vecActualCenter) <= vecShatterInfo.z * vecShatterInfo.z )
			{
				Vector vBreakPos	= m_vCorner + 
									(width*vWidthDir*m_flPanelWidth) + 
									(height*vHeightDir*m_flPanelHeight);

				ShatterPane( width, height, m_vNormal * 500, vBreakPos );
			}
		}
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBreakableSurface::Event_Killed( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType )
{
	return;
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CBreakableSurface::IsBroken(int nWidth, int nHeight)
{
	if (nWidth  < 0  || nWidth  >= m_nNumWide) return true;
	if (nHeight < 0 || nHeight >= m_nNumHigh)  return true;

	return (m_flSupport[nWidth][nHeight]==WINDOW_PANE_BROKEN);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : w - 
//			h - 
//			support - 
//-----------------------------------------------------------------------------
void CBreakableSurface::SetSupport( int w, int h, float support )
{
	m_flSupport[ w ][ h ] = support;

	int offset = w + h * m_nNumWide;

	bool prevval = m_RawPanelBitVec.Get( offset );
	bool curval = prevval;

	if ( support < 0.0f )
	{
		curval = false;
	}
	else
	{
		curval = true;
	}
	if ( curval != prevval )
	{
		m_RawPanelBitVec.Set( offset, curval );
		m_RawPanelBitVec.GetForModify( offset );
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
float CBreakableSurface::GetSupport(int nWidth, int nHeight)
{
	return MAX(0,m_flSupport[nWidth][nHeight]);
}

//------------------------------------------------------------------------------
// Purpose : Return the structural support for this pane.  Assumes window
//			 is upright.  Still works for windows parallel to the ground
//			 but simulation isn't quite as good
// Input   :
// Output  :
//------------------------------------------------------------------------------
float CBreakableSurface::RecalcSupport(int nWidth, int nHeight)
{
	// Always has some support. Zero signifies that it has been broken
	float flSupport = 0.01;

	// ------------
	// Top support
	// ------------
	if (nHeight == m_nNumHigh-1)
	{
		flSupport += 1.0;
	}
	else
	{
		flSupport += GetSupport(nWidth,nHeight+1);	 
	}

	// ------------
	// Bottom Support
	// ------------
	if (nHeight == 0)
	{
		flSupport += 1.25;
	}
	else
	{
 		flSupport += 1.25 * GetSupport(nWidth,nHeight-1);
	}

	// ------------
	// Left Support
	// ------------
	if (nWidth == 0)
	{
		flSupport += 1.0;
	}
	else
	{
		flSupport += GetSupport(nWidth-1,nHeight);
	}

	// --------------
	// Right Support
	// --------------
	if (nWidth == m_nNumWide-1)
	{
		flSupport += 1.0;
	}
	else
	{
		flSupport += GetSupport(nWidth+1,nHeight);
	}

	// --------------------
	// Bottom Left Support
	// --------------------
	if (nHeight == 0 || nWidth == 0)
	{
		flSupport += 1.0;
	}
	else
	{
		flSupport += GetSupport(nWidth-1,nHeight-1);
	}

	// ---------------------
	// Bottom Right Support
	// ---------------------
	if (nHeight == 0 || nWidth == m_nNumWide-1)
	{
		flSupport += 1.0;
	}
	else
	{
		flSupport += GetSupport(nWidth+1,nHeight-1);
	}

	// -----------------
	// Top Right Support
	// -----------------
	if (nHeight == m_nNumHigh-1 || nWidth == m_nNumWide-1)
	{
		flSupport += 0.25;
	}
	else
	{
		flSupport += 0.25 * GetSupport(nWidth+1,nHeight+1);
	}

	// -----------------
	// Top Left Support
	// -----------------
	if (nHeight == m_nNumHigh-1 || nWidth == 0)
	{
		flSupport += 0.25;
	}
	else
	{
		flSupport += 0.25 * GetSupport(nWidth-1,nHeight+1);
	}
	
	return flSupport;
}


//------------------------------------------------------------------------------
// Purpose : Itterate through the panels and make sure none have become
//			 unstable
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBreakableSurface::BreakThink(void)
{
	// Don't calculate support if I'm tile
	if (m_nSurfaceType == SHATTERSURFACE_TILE)
	{
		return;
	}

	// -----------------------
	// Recalculate all support
	// -----------------------
	int w;
	float flSupport[MAX_NUM_PANELS][MAX_NUM_PANELS];
	for (w=0;w<m_nNumWide;w++)
	{
		for (int h=0;h<m_nNumHigh;h++)
		{
			if (!IsBroken(w,h))
			{
				flSupport[w][h] = RecalcSupport(w,h);
			}
		}
	}

	// ----------------------------------------------------
	//  Set support and break inadequately supported panes
	// ----------------------------------------------------
	float flBreakValue = WINDOW_BREAK_SUPPORT*(m_nFragility/100.0);
	for (w=0;w<m_nNumWide;w++)
	{
		for (int h=0;h<m_nNumHigh;h++)
		{
			if (!IsBroken(w,h))
			{
				SetSupport( w, h, flSupport[w][h]/WINDOW_MAX_SUPPORT );
				if (m_flSupport[w][h] < flBreakValue)
				{
					// Occasionaly drop a pane
					if (random->RandomInt(0,1))
					{
						DropPane(w,h);
					}
					// Otherwise just shatter the glass
					else
					{
						ShatterPane(w,h,vec3_origin,vec3_origin);
					}
					SetNextThink( gpGlobals->curtime );
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
// Purpose : Given a 3D position on the window in space return the height and
//			 width of the position from the window's corner
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBreakableSurface::PanePos(const Vector &vPos, float *flWidth, float *flHeight)
{
	Vector vAttackVec = vPos - m_vCorner;
	QAngle vAngles;
	VectorAngles(-1*m_vNormal,vAngles);
	Vector vWidthDir,vHeightDir;
	AngleVectors(vAngles,NULL,&vWidthDir,&vHeightDir);
	float flWDist = DotProduct(vWidthDir,vAttackVec);
	float flHDist = DotProduct(vHeightDir,vAttackVec);

	// Figure out which quadrent I'm in
	*flWidth  = flWDist/m_flPanelWidth;
	*flHeight = flHDist/m_flPanelHeight;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBreakableSurface::BreakAllPanes(void)
{
	// Now tell the client all the panes have been broken
	for (int width=0;width<m_nNumWide;width++)
	{
		for (int height=0;height<m_nNumHigh;height++)
		{
			//SetSupport( width, height, WINDOW_PANE_BROKEN );

			BreakPane(width,height);
		}
	}

	m_nNumBrokenPanes = m_nNumWide*m_nNumHigh;
}

//------------------------------------------------------------------------------
// Purpose : Drop a window pane entity
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBreakableSurface::BreakPane(int nWidth, int nHeight)
{
	// Check parameter range
	if (nWidth < 0  || nWidth  >= m_nNumWide) return;
	if (nHeight < 0 || nHeight >= m_nNumHigh) return;

	// Count how many panes have been broken or dropped
	m_nNumBrokenPanes++;
	SetSupport( nWidth, nHeight, WINDOW_PANE_BROKEN );

	SetThink(&CBreakableSurface::BreakThink);
	SetNextThink( gpGlobals->curtime );
}

//------------------------------------------------------------------------------
// Purpose : Drop a window pane entity
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBreakableSurface::DropPane(int nWidth, int nHeight)
{
	// Check parameter range
	if (nWidth < 0  || nWidth  >= m_nNumWide) return;
	if (nHeight < 0 || nHeight >= m_nNumHigh) return;

	if (!IsBroken(nWidth,nHeight))
	{
		BreakPane(nWidth,nHeight);

		QAngle vAngles;
		VectorAngles(-1*m_vNormal,vAngles);
		
		Vector vWidthDir,vHeightDir;
		AngleVectors(vAngles,NULL,&vWidthDir,&vHeightDir);
		Vector vBreakPos	= m_vCorner + 
								(nWidth*vWidthDir*m_flPanelWidth) + 
								(nHeight*vHeightDir*m_flPanelHeight);

		CreateShards(vBreakPos, vAngles, vec3_origin, vec3_origin,
						WINDOW_PANEL_SIZE,	WINDOW_PANEL_SIZE,
						WINDOW_SMALL_SHARD_SIZE);

		DamageSound();

		CWindowPane *pPane = CWindowPane::CreateWindowPane(vBreakPos, vAngles);
		if (pPane)
		{
			pPane->SetLocalAngularVelocity( RandomAngle(-120,120) );
		}
	}
}

void CBreakableSurface::CreateShards(const Vector &vBreakPos, const QAngle &vAngles,
									 const Vector &vForce,	  const Vector &vForcePos,
									 float flWidth,			  float flHeight,
									 int   nShardSize)
{
	Vector	vAdjustedBreakPos = vBreakPos;
	Vector	vAdjustedForce	 = vForce;
	int		front_r,front_g,front_b;
	int		back_r,back_g,back_b;


	// UNDONE: For now hardcode these colors.  Later when used by more textures
	//         we'll automate this process or expose the colors in WC
	if (m_nSurfaceType == SHATTERSURFACE_TILE)
	{
		// If tile shoot shards back from the shattered surface and offset slightly 
		// from the surface.
		vAdjustedBreakPos  -=  8*m_vNormal; 
		vAdjustedForce		= -0.75*vForce;
		front_r				= 89;
		front_g				= 120;
		front_b				= 83;
		back_r				= 99;
		back_g				= 76;
		back_b				= 21;
	}
	else
	{
		front_r				= 255;
		front_g				= 255;
		front_b				= 255;
		back_r				= 255;
		back_g				= 255;
		back_b				= 255;
	}
	
	CPASFilter filter( vAdjustedBreakPos );
	te->ShatterSurface(filter, 0.0,
		&vAdjustedBreakPos, &vAngles, 
		&vAdjustedForce, &vForcePos, 
		flWidth, flHeight,WINDOW_SMALL_SHARD_SIZE,m_nSurfaceType,
		front_r,front_g,front_b,back_r,back_g,back_b);//4);
}

//------------------------------------------------------------------------------
// Purpose : Break a panel
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CBreakableSurface::ShatterPane(int nWidth, int nHeight, const Vector &vForce, const Vector &vForcePos)
{
	// Check parameter range
	if (nWidth < 0  || nWidth  >= m_nNumWide) return false;
	if (nHeight < 0 || nHeight >= m_nNumHigh) return false;

	if ( IsBroken(nWidth,nHeight) )
		return false;

	BreakPane(nWidth,nHeight);

	QAngle vAngles;
	VectorAngles(-1*m_vNormal,vAngles);
	Vector vWidthDir,vHeightDir;
	AngleVectors(vAngles,NULL,&vWidthDir,&vHeightDir);
	Vector vBreakPos	= m_vCorner + 
						(nWidth*vWidthDir*m_flPanelWidth) + 
						(nHeight*vHeightDir*m_flPanelHeight);

	CreateShards(vBreakPos, vAngles,vForce,	vForcePos, m_flPanelWidth, m_flPanelHeight, WINDOW_SMALL_SHARD_SIZE);

	DamageSound();
	return true;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBreakableSurface::Spawn(void)
{
	BaseClass::Spawn();
	SetCollisionGroup( COLLISION_GROUP_BREAKABLE_GLASS ); 
	m_bIsBroken = false;

	if (m_nQuadError == QUAD_ERR_MULT_FACES)
	{
		Warning("Rejecting func_breakablesurf.  Has multiple faces that aren't NODRAW.\n");
		UTIL_Remove(this);
	}
	else if (m_nQuadError == QUAD_ERR_NOT_QUAD)
	{
		Warning("Rejecting func_breakablesurf.  Drawn face isn't a quad.\n");
		UTIL_Remove(this);
	}

	int materialCount = modelinfo->GetModelMaterialCount( const_cast<model_t*>(GetModel()) );
	if( materialCount != 1 )
	{
		Warning( "Encountered func_breakablesurf that has a material applied to more than one surface!\n" );
		UTIL_Remove(this);
	}

	// Get at the first material; even if there are more than one.
	IMaterial* pMaterial;
	modelinfo->GetModelMaterials( const_cast<model_t*>(GetModel()), 1, &pMaterial );

	// The material should point to a cracked version of itself
	bool foundVar;
	IMaterialVar* pCrackName = pMaterial->FindVar( "$crackmaterial", &foundVar, false );
	if (foundVar)
	{
		PrecacheMaterial( pCrackName->GetStringValue() );
	}

	// Init the Panel bit vector to all true. ( no panes are broken )
	int bitVecLength = MAX_NUM_PANELS * MAX_NUM_PANELS;
	
	for( int i=0;i<bitVecLength;i++ )
	{
		m_RawPanelBitVec.Set( i, true );
	}
}

void CBreakableSurface::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	if ( !m_bIsBroken )
	{
		int damageType = 0;
		string_t iszDamageTable = ( ( m_nSurfaceType == SHATTERSURFACE_GLASS ) ? ( "glass" ) : ( NULL_STRING ) );
		bool bDamageFromHeldObjects = ( ( m_spawnflags & SF_BREAKABLESURF_DAMAGE_FROM_HELD_OBJECTS ) != 0 );
		float damage = CalculateDefaultPhysicsDamage( index, pEvent, 1.0, false, damageType, iszDamageTable, bDamageFromHeldObjects );

		if ( damage > 10 )
		{
			// HACKHACK: Reset mass to get correct collision response for the object breaking this
			pEvent->pObjects[index]->SetMass( 2.0f );

			Vector normal, damagePos;
			pEvent->pInternalData->GetSurfaceNormal( normal );
			if ( index == 0 )
			{
				normal *= -1.0f;
			}
			pEvent->pInternalData->GetContactPoint( damagePos );
			int otherIndex = !index;
			CBaseEntity *pInflictor = pEvent->pEntities[otherIndex];
			CTakeDamageInfo info( pInflictor, pInflictor, normal, damagePos, damage, damageType );
			PhysCallbackDamage( this, info, *pEvent, index );
		}
		else if ( damage > 0 )
		{
			if ( m_spawnflags & SF_BREAKABLESURF_CRACK_DECALS )
			{

				Vector normal, damagePos;
				pEvent->pInternalData->GetSurfaceNormal( normal );
				if ( index == 0 )
				{
					normal *= -1.0f;
				}
				pEvent->pInternalData->GetContactPoint( damagePos );

				trace_t tr;
				UTIL_TraceLine ( damagePos - normal, damagePos + normal, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

				// Only place decals and draw effects if we hit something valid
				if ( tr.m_pEnt && tr.m_pEnt == this )
				{
					// Build the impact data
					CEffectData data;
					data.m_vOrigin = tr.endpos;
					data.m_vStart = tr.startpos;
					data.m_nSurfaceProp = tr.surface.surfaceProps;
					data.m_nDamageType = DMG_CLUB;
					data.m_nHitBox = tr.hitbox;
					data.m_nEntIndex = entindex();

					// Send it on its way
					DispatchEffect( "Impact", data );
				}
			}
		}
	}
	BaseClass::VPhysicsCollision( index, pEvent );
}

