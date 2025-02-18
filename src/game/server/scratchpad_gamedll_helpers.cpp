//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "scratchpad_gamedll_helpers.h"
#include "iscratchpad3d.h"
#include "player.h"
#include "collisionproperty.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void ScratchPad_DrawWorldToScratchPad(
	IScratchPad3D *pPad,
	unsigned long flags )
{
	pPad->SetRenderState( IScratchPad3D::RS_FillMode, IScratchPad3D::FillMode_Wireframe );

	if ( flags & SPDRAWWORLD_DRAW_WORLD )
	{
		engine->DrawMapToScratchPad( pPad, 0 );
	}

	if ( flags & (SPDRAWWORLD_DRAW_PLAYERS | SPDRAWWORLD_DRAW_ENTITIES) )
	{
		CBaseEntity *pCur = gEntList.FirstEnt();
		while ( pCur )
		{
			bool bPlayer = ( dynamic_cast< CBasePlayer* >( pCur ) != 0 );
			if ( (bPlayer && !( flags & SPDRAWWORLD_DRAW_PLAYERS )) ||
				(!bPlayer && !( flags & SPDRAWWORLD_DRAW_ENTITIES )) )
			{
				pCur = gEntList.NextEnt( pCur );
				continue;
			}

			ScratchPad_DrawEntityToScratchPad( 
				pPad, 
				flags,
				pCur, 
				bPlayer ? Vector( 1.0, 0.5, 0 ) : Vector( 0.3, 0.3, 1.0 )
				);

			pCur = gEntList.NextEnt( pCur );
		}
	}
}


void ScratchPad_DrawEntityToScratchPad(
	IScratchPad3D *pPad,
	unsigned long flags,
	CBaseEntity *pEnt,
	const Vector &vColor )
{
	// Draw the entity's bbox [todo: draw OBBs here too].
	Vector mins, maxs;
	pEnt->CollisionProp()->WorldSpaceAABB( &mins, &maxs );

	pPad->DrawWireframeBox( mins, maxs, vColor );

	// Draw the edict's index or class?
	char str[512];
	str[0] = 0;
	if ( flags & SPDRAWWORLD_DRAW_EDICT_INDICES )
	{
		char tempStr[512];
		Q_snprintf( tempStr, sizeof( tempStr ), "edict: %d", pEnt->entindex() );
		Q_strncat( str, tempStr, sizeof( str ), COPY_ALL_CHARACTERS );
	}

	if ( flags & SPDRAWWORLD_DRAW_ENTITY_CLASSNAMES )
	{
		if ( str[0] != 0 )
			Q_strncat( str, ", ", sizeof( str ), COPY_ALL_CHARACTERS );

		char tempStr[512];
		Q_snprintf( tempStr, sizeof( tempStr ), "class: %s", pEnt->GetClassname() );
		Q_strncat( str, tempStr, sizeof( str ), COPY_ALL_CHARACTERS );
	}

	if ( str[0] != 0 )
	{
		CTextParams params;
		params.m_vPos = (mins + maxs) * 0.5f;
		params.m_bCentered = true;
		params.m_flLetterWidth = 2;
		pPad->DrawText( str, params );
	}
}


