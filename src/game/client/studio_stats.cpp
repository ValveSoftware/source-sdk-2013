//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "vgui/IInput.h"
#include "materialsystem/imaterialvar.h"
#include <vgui_controls/EditablePanel.h>
#include <mathlib/mathlib.h>
#include "view.h"
#include "studio_stats.h"
#include "coordsize.h"
#include "collisionutils.h"

enum
{
	RSTUDIOSTATMODE_NONE = 0,
	RSTUDIOSTATMODE_HELDWEAPON,
	RSTUDIOSTATMODE_VIEWMODEL,
	RSTUDIOSTATMODE_VIEWMODEL_ATTACHMENT,
};

IClientRenderable	*g_pStudioStatsEntity = NULL;
static ConVar	r_studio_stats( "r_studio_stats", "0", FCVAR_CHEAT );
static ConVar	r_studio_stats_lock( "r_studio_stats_lock", "0", FCVAR_CHEAT, "Lock the current studio stats entity selection" );
static ConVar	r_studio_stats_mode( "r_studio_stats_mode", "0", FCVAR_CHEAT, "Sets a mode for r_studio_stats. Modes are as follows:\n\t0 = Entity under your crosshair\n\t1 = Weapon held by player under your crosshair\n\t2 = Your viewmodel\n\t3 = The first entity attached to your viewmodel" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CStudioStatsEnumerator : public IPartitionEnumerator
{
public:
	CStudioStatsEnumerator( Ray_t& shot )
	{
		m_rayShot = shot;
		m_bHit = false;
	}

	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		trace_t tr;
		enginetrace->ClipRayToEntity( m_rayShot, MASK_SHOT, pHandleEntity, &tr );

		if ( tr.fraction < 1.0 )
		{
			ICollideable *pCollideable = enginetrace->GetCollideable( pHandleEntity );
			IClientUnknown *pUnk = pCollideable->GetIClientUnknown();
			if ( pUnk )
			{
				g_pStudioStatsEntity = pUnk->GetClientRenderable();

				if ( g_pStudioStatsEntity )
				{
					m_bHit = true;
					return ITERATION_STOP;
				}
			}
		}

		return ITERATION_CONTINUE;
	}

	bool Hit( void ) const { return m_bHit; }

private:
	Ray_t			m_rayShot;
	bool			m_bHit;
};


void StudioStats_FindClosestEntity( CClientRenderablesList *pClientRenderablesList )
{
	if ( r_studio_stats_lock.GetBool() )
		return;

	g_pStudioStatsEntity = NULL;
	if ( r_studio_stats.GetBool() == false )
		return;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	int iStudioStatMode = r_studio_stats_mode.GetInt();

	if ( iStudioStatMode == RSTUDIOSTATMODE_VIEWMODEL )
	{
		g_pStudioStatsEntity = pPlayer->GetViewModel();
		return;
	}
	if ( iStudioStatMode == RSTUDIOSTATMODE_VIEWMODEL_ATTACHMENT )
	{
		C_BaseEntity *pVM = pPlayer->GetViewModel();
		if ( pVM )
		{
			g_pStudioStatsEntity = pVM->FirstMoveChild();
		}
		return;
	}

	trace_t tr;
	Vector vecStart, vecEnd;
	VectorMA( MainViewOrigin(), MAX_TRACE_LENGTH, MainViewForward(), vecEnd );
	VectorMA( MainViewOrigin(), 10,   MainViewForward(), vecStart );

	Ray_t shotRay;
	shotRay.Init( vecStart, vecEnd );
	CStudioStatsEnumerator studioEnum( shotRay );
	::partition->EnumerateElementsAlongRay( PARTITION_ALL_CLIENT_EDICTS, shotRay, false, &studioEnum );

	if ( g_pStudioStatsEntity )
	{
		C_BaseEntity *pEntity = g_pStudioStatsEntity->GetIClientUnknown()->GetBaseEntity();

		if ( pEntity && ( pEntity != C_BasePlayer::GetLocalPlayer() ) )
		{
			switch ( iStudioStatMode )
			{
			default:
			case RSTUDIOSTATMODE_NONE:
				g_pStudioStatsEntity = pEntity->GetClientRenderable();
				break;

			case RSTUDIOSTATMODE_HELDWEAPON:
				{
					C_BasePlayer *pTargetPlayer = ToBasePlayer( pEntity );
					if ( pTargetPlayer && pTargetPlayer->GetActiveWeapon() )
					{
						g_pStudioStatsEntity = pTargetPlayer->GetActiveWeapon()->GetClientRenderable();
					}
				}
				break;
			}
		}
	}
}
