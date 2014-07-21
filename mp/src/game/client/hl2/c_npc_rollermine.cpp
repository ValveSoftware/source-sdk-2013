//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_ai_basenpc.h"
#include "iviewrender_beams.h"
#include "beam_shared.h"
#include "materialsystem/imaterial.h"
#include "model_types.h"
#include "clienteffectprecachesystem.h"
#include "beamdraw.h"

class C_RollerMine : public C_AI_BaseNPC
{
	DECLARE_CLASS( C_RollerMine, C_AI_BaseNPC );
public:
	DECLARE_CLIENTCLASS();

			C_RollerMine( void ) {}

	int		DrawModel( int flags );

	RenderGroup_t GetRenderGroup( void ) 
	{	
		if ( m_bIsOpen )
			return RENDER_GROUP_TRANSLUCENT_ENTITY;	
		else
			return RENDER_GROUP_OPAQUE_ENTITY;
	}

private:
	C_RollerMine( const C_RollerMine & ) {}

	bool	m_bIsOpen;
	float	m_flActiveTime;
	bool	m_bHackedByAlyx;
	bool	m_bPowerDown;
};

IMPLEMENT_CLIENTCLASS_DT( C_RollerMine, DT_RollerMine, CNPC_RollerMine )
	RecvPropInt( RECVINFO( m_bIsOpen ) ),
	RecvPropFloat( RECVINFO( m_flActiveTime ) ),
	RecvPropInt( RECVINFO( m_bHackedByAlyx ) ),
	RecvPropInt( RECVINFO( m_bPowerDown ) ),
END_RECV_TABLE()

#define	NUM_ATTACHMENTS	11

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
// Output : int
//-----------------------------------------------------------------------------
int C_RollerMine::DrawModel( int flags )
{
	if ( m_bIsOpen && m_flActiveTime <= gpGlobals->curtime )
	{
		float scale = random->RandomFloat( 4.0f, 6.0f );

		if ( gpGlobals->frametime != 0 )
		{
			// Inner beams
			BeamInfo_t beamInfo;

			beamInfo.m_vecStart = GetAbsOrigin();
			Vector	offset = RandomVector( -6*scale, 2*scale );

			offset += Vector(2,2,2) * scale;
			beamInfo.m_vecEnd = GetAbsOrigin() + offset;

			beamInfo.m_pStartEnt= cl_entitylist->GetEnt( BEAMENT_ENTITY( entindex() ) );
			beamInfo.m_pEndEnt	= beamInfo.m_pStartEnt;
			beamInfo.m_nStartAttachment = random->RandomInt( 0, NUM_ATTACHMENTS );
			beamInfo.m_nEndAttachment = random->RandomInt( 0, NUM_ATTACHMENTS );

			// Ensure we're not the same point
			if ( beamInfo.m_nStartAttachment == beamInfo.m_nEndAttachment )
			{
				int nextStep = ( random->RandomInt( 0, 1 ) ) ? 1 : -1;

				beamInfo.m_nEndAttachment = ( beamInfo.m_nStartAttachment + nextStep ) % NUM_ATTACHMENTS;
			}
			
			beamInfo.m_nType = TE_BEAMTESLA;
			beamInfo.m_pszModelName = "sprites/lgtning.vmt";
			beamInfo.m_flHaloScale = 0.0f;
			beamInfo.m_flLife = 0.1f;
			beamInfo.m_flWidth = random->RandomFloat( 2.0f, 4.0f );
			beamInfo.m_flEndWidth = random->RandomFloat( 0.0f, 1.0f );
			beamInfo.m_flFadeLength = 0.0f;
			beamInfo.m_flAmplitude = random->RandomFloat( 16, 32 );
			beamInfo.m_flBrightness = 255.0;
			beamInfo.m_flSpeed = 0.0;
			beamInfo.m_nStartFrame = 0.0;
			beamInfo.m_flFrameRate = 1.0f;

			if ( m_bPowerDown )
			{
				beamInfo.m_flRed = 255.0f;;
				beamInfo.m_flGreen = 64.0f;
				beamInfo.m_flBlue = 64.0f;
			}
			else if ( m_bHackedByAlyx )
			{
				beamInfo.m_flRed = 240.0f;;
				beamInfo.m_flGreen = 200.0f;
				beamInfo.m_flBlue = 80.0f;
			}
			else
			{
				beamInfo.m_flRed = 255.0f;;
				beamInfo.m_flGreen = 255.0f;
				beamInfo.m_flBlue = 255.0f;
			}

			beamInfo.m_nSegments = 4;
			beamInfo.m_bRenderable = true;
			beamInfo.m_nFlags = 0;
			
			beams->CreateBeamEntPoint( beamInfo );
			
			// Draw the halo
			float	color[3];

			if ( m_bHackedByAlyx )
			{
				color[0] = 0.25f;
				color[1] = 0.05f;
				color[2] = 0.0f;
			}
			else
			{
				color[0] = color[1] = color[2] = 0.15f;
			}

			IMaterial *pMaterial = materials->FindMaterial( "effects/rollerglow", NULL, false );

			CMatRenderContextPtr pRenderContext( materials );
			pRenderContext->Bind( pMaterial );
			DrawHalo( pMaterial, GetAbsOrigin(), random->RandomFloat( 6.0f*scale, 6.5f*scale ), color );

			if ( m_bPowerDown )
			{
				color[0] = random->RandomFloat( 0.80f, 1.00f );
				color[1] = random->RandomFloat( 0.10f, 0.25f );
				color[2] = 0.0f;
			}
			else if ( m_bHackedByAlyx )
			{
				color[0] = random->RandomFloat( 0.25f, 0.75f );
				color[1] = random->RandomFloat( 0.10f, 0.25f );
				color[2] = 0.0f;
			}
			else
			{
				color[0] = color[1] = color[2] = random->RandomFloat( 0.25f, 0.5f );
			}

			Vector attachOrigin;
			QAngle attachAngles;
			
			GetAttachment( beamInfo.m_nEndAttachment, attachOrigin, attachAngles );
			DrawHalo( pMaterial, attachOrigin, random->RandomFloat( 1.0f*scale, 1.5f*scale ), color );
			
			GetAttachment( beamInfo.m_nStartAttachment, attachOrigin, attachAngles );
			DrawHalo( pMaterial, attachOrigin, random->RandomFloat( 1.0f*scale, 1.5f*scale ), color );
		}
	}

	return BaseClass::DrawModel( flags );
}