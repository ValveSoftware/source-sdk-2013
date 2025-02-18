//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================//
#include "cbase.h"

#include "c_te_effect_dispatch.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "tf_shareddefs.h"
#include "c_rope.h"

// NOTE: Always include this last!
#include "tier0/memdbgon.h"


// Used for cycling so that they're sequencially ordered on each strand
int g_nHolidayLightColor = 0;

// 4 light colors
Color g_rgbaHolidayRed( 255, 0, 0, 255 );
Color g_rgbaHolidayYellow( 2, 110, 197, 255 );
Color g_rgbaHolidayGreen( 117, 193, 8, 255 );
Color g_rgbaHolidayBlue( 255, 151, 29, 255 );

Color *(rgbaHolidayLightColors[]) = { &g_rgbaHolidayRed, 
									  &g_rgbaHolidayYellow, 
									  &g_rgbaHolidayGreen, 
									  &g_rgbaHolidayBlue };


struct HolidayLightData_t
{
	Vector vOrigin;
	int nID;
	int nSubID;
	float fScale;
};


void CreateHolidayLight( const HolidayLightData_t &holidayLight );


class CHolidayLightManager : public CAutoGameSystemPerFrame
{
public:
	CHolidayLightManager( char const *name );

	// Methods of IGameSystem
	virtual void Update( float frametime );

	virtual void LevelInitPostEntity( void );
	virtual void LevelShutdownPreEntity();

	void AddHolidayLight( const CEffectData &data );

private:

	CUtlVector< HolidayLightData_t > m_PendingLightData;
};

CHolidayLightManager g_CHolidayLightManager( "CHolidayLightManager" );


CHolidayLightManager::CHolidayLightManager( char const *name ) : CAutoGameSystemPerFrame( name )
{
}

// Methods of IGameSystem
void CHolidayLightManager::Update( float frametime )
{
	for ( int i = 0; i < m_PendingLightData.Count(); ++i )
	{
		CreateHolidayLight( m_PendingLightData[ i ] );
	}

	m_PendingLightData.RemoveAll();
}

void CHolidayLightManager::LevelInitPostEntity( void )
{
	m_PendingLightData.RemoveAll();
}

void CHolidayLightManager::LevelShutdownPreEntity()
{
	m_PendingLightData.RemoveAll();
}

void CHolidayLightManager::AddHolidayLight( const CEffectData &data )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// Too far away?
	if ( pPlayer->GetAbsOrigin().DistTo( data.m_vOrigin ) > 2000.0f )
		return;

	// In the skybox?
	sky3dparams_t *pSky = &(pPlayer->m_Local.m_skybox3d);
	if ( pSky->origin->DistTo( data.m_vOrigin ) < 2000.0f )
		return;

	HolidayLightData_t newData;

	newData.vOrigin = data.m_vOrigin;

	// HACK: Use these ints to ID the light later
	newData.nID = data.m_nMaterial;
	newData.nSubID = data.m_nHitBox;

	// Skybox lights pass in a smaller scale
	newData.fScale = data.m_flScale;

	if ( m_PendingLightData.Count() < CTempEnts::MAX_TEMP_ENTITIES / 2 )
	{
		m_PendingLightData.AddToTail( newData );
	}
}


void TF_HolidayLightCallback( const CEffectData &data )
{
	g_CHolidayLightManager.AddHolidayLight( data );
}

void CreateHolidayLight( const HolidayLightData_t &holidayLight )
{
	int nHolidayLightStyle = RopeManager()->GetHolidayLightStyle();

	const model_t *pModel = ( nHolidayLightStyle == 0 ? engine->LoadModel( "effects/christmas_bulb.vmt" ) : engine->LoadModel( "effects/mtp_fluff.vmt" ) );
	if ( !pModel )
		return;

	Assert( pModel );

	C_LocalTempEntity *pTemp = tempents->FindTempEntByID( holidayLight.nID, holidayLight.nSubID );

	if ( !pTemp )
	{
		// Didn't find one with that ID, so make a new one!
		// Randomize the angle
		QAngle angOrientation = ( nHolidayLightStyle == 0 ? QAngle( 0.0f, 0.0f, RandomFloat( -180.0f, 180.0f ) ) : vec3_angle );
		pTemp = tempents->SpawnTempModel( pModel, holidayLight.vOrigin, angOrientation, vec3_origin, 2.0f, FTENT_NEVERDIE );
		if ( !pTemp )
		{
			return;
		}

		pTemp->clientIndex = 0;

		// HACK: Use these ints to ID the light later
		pTemp->m_nSkin = holidayLight.nID;
		pTemp->hitSound = holidayLight.nSubID;

		// Skybox lights pass in a smaller scale
		pTemp->m_flSpriteScale = holidayLight.fScale;

		// Smuggle the color index here
		pTemp->m_nHitboxSet = g_nHolidayLightColor;

		// Set the color
		pTemp->SetRenderColor( rgbaHolidayLightColors[ g_nHolidayLightColor ]->r(), 
							   rgbaHolidayLightColors[ g_nHolidayLightColor ]->g(), 
							   rgbaHolidayLightColors[ g_nHolidayLightColor ]->b(), 
							   rgbaHolidayLightColors[ g_nHolidayLightColor ]->a() );

		// Next color in the pattern
		g_nHolidayLightColor = ( g_nHolidayLightColor + 1 ) % ARRAYSIZE( rgbaHolidayLightColors );

		// Animate if needed
		pTemp->m_flFrameMax = modelinfo->GetModelFrameCount( pModel ) - 1;
		pTemp->flags = ( FTENT_SPRANIMATE | FTENT_SPRANIMATELOOP );
		pTemp->m_flFrameRate = 10;
	}
	else
	{
		// Update the position
		pTemp->SetAbsOrigin( holidayLight.vOrigin );

		// Every 10 light strands have a blink cycle
		if ( pTemp->m_nSkin % 5 == 0 )
		{
			// Magic! Basically this makes the on/off cycle of each color different and offsets it by the segment index.
			// That way it looks like a timed pattern but is also chaotic.
			int nCycle = ( pTemp->hitSound + static_cast< int >( gpGlobals->curtime * 2.0f ) ) % ( pTemp->m_nHitboxSet + ARRAYSIZE( rgbaHolidayLightColors ) + 1 );
			pTemp->SetRenderColorA( nCycle < ARRAYSIZE( rgbaHolidayLightColors ) ? 255 : 64 );
		}

		// Update the scale
		pTemp->m_flSpriteScale = holidayLight.fScale;

		// Extend it's life
		pTemp->die = gpGlobals->curtime + 2.0f;
	}
}

DECLARE_CLIENT_EFFECT( "TF_HolidayLight", TF_HolidayLightCallback );

void RopesHolidayLightColor( const CCommand &args )
{
	if ( args.ArgC() < 5 )
		return;

	int nLight = atoi( args[ 1 ] );

	if ( nLight < 0 || nLight >= ARRAYSIZE( rgbaHolidayLightColors ) )
		return;

 	rgbaHolidayLightColors[ nLight ]->SetColor( atoi( args[ 2 ] ), atoi( args[ 3 ] ), atoi( args[ 4 ] ) );
}
ConCommand r_ropes_holiday_light_color( "r_ropes_holiday_light_color", RopesHolidayLightColor, "Set each light's color: [light0-3] [r0-255] [g0-255] [b0-255]", FCVAR_NONE );