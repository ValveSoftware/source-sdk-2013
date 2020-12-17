//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ==========
//
// An entity that allows level designer control over the post-processing parameters.
//
//===================================================================================

#include "cbase.h"
#include "postprocesscontroller.h"
#include "entityinput.h"
#include "entityoutput.h"
#include "eventqueue.h"
#include "player.h"
#include "world.h"
#include "ndebugoverlay.h"
#include "triggers.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CPostProcessSystem s_PostProcessSystem( "PostProcessSystem" );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CPostProcessSystem *PostProcessSystem()
{
	return &s_PostProcessSystem;
}


LINK_ENTITY_TO_CLASS( postprocess_controller, CPostProcessController );

BEGIN_DATADESC( CPostProcessController )
	DEFINE_KEYFIELD( m_flPostProcessParameters[ PPPN_FADE_TIME ], FIELD_FLOAT,	"fadetime" ),
	DEFINE_KEYFIELD( m_flPostProcessParameters[ PPPN_LOCAL_CONTRAST_STRENGTH ], FIELD_FLOAT,	"localcontraststrength" ),
	DEFINE_KEYFIELD( m_flPostProcessParameters[ PPPN_LOCAL_CONTRAST_EDGE_STRENGTH ], FIELD_FLOAT,	"localcontrastedgestrength" ),
	DEFINE_KEYFIELD( m_flPostProcessParameters[ PPPN_VIGNETTE_START ], FIELD_TIME,	"vignettestart" ),
	DEFINE_KEYFIELD( m_flPostProcessParameters[ PPPN_VIGNETTE_END ], FIELD_TIME,	"vignetteend" ),
	DEFINE_KEYFIELD( m_flPostProcessParameters[ PPPN_VIGNETTE_BLUR_STRENGTH ], FIELD_FLOAT,	"vignetteblurstrength" ),
	DEFINE_KEYFIELD( m_flPostProcessParameters[ PPPN_FADE_TO_BLACK_STRENGTH ], FIELD_FLOAT,	"fadetoblackstrength" ),
	DEFINE_KEYFIELD( m_flPostProcessParameters[ PPPN_DEPTH_BLUR_FOCAL_DISTANCE ], FIELD_FLOAT, "depthblurfocaldistance" ),
	DEFINE_KEYFIELD( m_flPostProcessParameters[ PPPN_DEPTH_BLUR_STRENGTH ], FIELD_FLOAT, "depthblurstrength" ),
	DEFINE_KEYFIELD( m_flPostProcessParameters[ PPPN_SCREEN_BLUR_STRENGTH ], FIELD_FLOAT, "screenblurstrength" ),
	DEFINE_KEYFIELD( m_flPostProcessParameters[ PPPN_FILM_GRAIN_STRENGTH ], FIELD_FLOAT, "filmgrainstrength" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFadeTime", InputSetFadeTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetLocalContrastStrength", InputSetLocalContrastStrength ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetLocalContrastEdgeStrength", InputSetLocalContrastEdgeStrength ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetVignetteStart", InputSetVignetteStart ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetVignetteEnd", InputSetVignetteEnd ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetVignetteBlurStrength", InputSetVignetteBlurStrength ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFadeToBlackStrength", InputSetFadeToBlackStrength ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetDepthBlurFocalDistance", InputSetDepthBlurFocalDistance),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetDepthBlurStrength", InputSetDepthBlurStrength ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetScreenBlurStrength", InputSetScreenBlurStrength ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFilmGrainStrength", InputSetFilmGrainStrength ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CPostProcessController, DT_PostProcessController )
	SendPropArray3( SENDINFO_ARRAY3( m_flPostProcessParameters ), SendPropFloat( SENDINFO_ARRAY( m_flPostProcessParameters ) ) ),
	SendPropBool( SENDINFO(m_bMaster) ),
END_SEND_TABLE()


CPostProcessController::CPostProcessController()
{	
    m_bMaster = false;
}

CPostProcessController::~CPostProcessController()
{
}

void CPostProcessController::Spawn()
{
	BaseClass::Spawn();

	m_bMaster = IsMaster();
}

int CPostProcessController::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CPostProcessController::InputSetFadeTime( inputdata_t &inputdata )
{
	m_flPostProcessParameters.Set( PPPN_FADE_TIME, inputdata.value.Float() );
}

void CPostProcessController::InputSetLocalContrastStrength( inputdata_t &inputdata )
{
	m_flPostProcessParameters.Set( PPPN_LOCAL_CONTRAST_STRENGTH, inputdata.value.Float() );
}

void CPostProcessController::InputSetLocalContrastEdgeStrength( inputdata_t &inputdata )
{
	m_flPostProcessParameters.Set( PPPN_LOCAL_CONTRAST_EDGE_STRENGTH, inputdata.value.Float() );
}

void CPostProcessController::InputSetVignetteStart( inputdata_t &inputdata )
{
	m_flPostProcessParameters.Set( PPPN_VIGNETTE_START, inputdata.value.Float() );
}

void CPostProcessController::InputSetVignetteEnd( inputdata_t &inputdata )
{
	m_flPostProcessParameters.Set( PPPN_VIGNETTE_END, inputdata.value.Float() );
}

void CPostProcessController::InputSetVignetteBlurStrength( inputdata_t &inputdata )
{
	m_flPostProcessParameters.Set( PPPN_VIGNETTE_BLUR_STRENGTH, inputdata.value.Float() );
}

void CPostProcessController::InputSetFadeToBlackStrength( inputdata_t &inputdata )
{
	m_flPostProcessParameters.Set( PPPN_FADE_TO_BLACK_STRENGTH, inputdata.value.Float() );
}

void CPostProcessController::InputSetDepthBlurFocalDistance( inputdata_t &inputdata )
{
	m_flPostProcessParameters.Set( PPPN_DEPTH_BLUR_FOCAL_DISTANCE, inputdata.value.Float() );
}

void CPostProcessController::InputSetDepthBlurStrength( inputdata_t &inputdata )
{
	m_flPostProcessParameters.Set( PPPN_DEPTH_BLUR_STRENGTH, inputdata.value.Float() );
}

void CPostProcessController::InputSetScreenBlurStrength( inputdata_t &inputdata )
{
	m_flPostProcessParameters.Set( PPPN_SCREEN_BLUR_STRENGTH, inputdata.value.Float() );
}

void CPostProcessController::InputSetFilmGrainStrength( inputdata_t &inputdata )
{
	m_flPostProcessParameters.Set( PPPN_FILM_GRAIN_STRENGTH, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: Clear out the PostProcess controller.
//-----------------------------------------------------------------------------
void CPostProcessSystem::LevelInitPreEntity()
{
	m_hMasterController = nullptr;
	ListenForGameEvent( "round_start" );
}

//-----------------------------------------------------------------------------
// Purpose: Find the master controller.  If no controller is 
//			set as Master, use the first controller found.
//-----------------------------------------------------------------------------
void CPostProcessSystem::InitMasterController()
{
	CPostProcessController *pPostProcessController = nullptr;

	do
	{
		pPostProcessController = dynamic_cast<CPostProcessController*>( gEntList.FindEntityByClassname( pPostProcessController, "postprocess_controller" ) );
		if ( pPostProcessController )
		{
			if ( m_hMasterController.Get() == nullptr )
			{
				m_hMasterController = pPostProcessController;
			}
			else
			{
				if ( pPostProcessController->IsMaster() )
				{
					m_hMasterController = pPostProcessController;
				}
			}
		}
	} while ( pPostProcessController );
}

//-----------------------------------------------------------------------------
// Purpose: On a multiplayer map restart, re-find the master controller.
//-----------------------------------------------------------------------------
void CPostProcessSystem::FireGameEvent( IGameEvent *pEvent )
{
	InitMasterController();
}

//-----------------------------------------------------------------------------
// Purpose: On level load find the master PostProcess controller.  If no controller is 
//			set as Master, use the first PostProcess controller found.
//-----------------------------------------------------------------------------
void CPostProcessSystem::LevelInitPostEntity()
{
	InitMasterController();

	// HACK: Singleplayer games don't get a call to CBasePlayer::Spawn on level transitions.
	// CBasePlayer::Activate is called before this is called so that's too soon to set up the PostProcess controller.
	// We don't have a hook similar to Activate that happens after LevelInitPostEntity
	// is called, or we could just do this in the player itself.
	if ( gpGlobals->maxClients == 1 )
	{
		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
		if ( pPlayer && ( pPlayer->m_hPostProcessCtrl.Get() == nullptr ) )
		{
			pPlayer->InitPostProcessController();
		}
	}
}
