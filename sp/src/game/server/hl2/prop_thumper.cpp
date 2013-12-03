//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the big scary boom-boom machine Antlions fear.
//
//=============================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "npcevent.h"
#include "rotorwash.h"
#include "soundenvelope.h"
#include "engine/IEngineSound.h"
#include "npc_antlion.h"
#include "te_effect_dispatch.h"

#if HL2_EPISODIC
#define THUMPER_RADIUS m_iEffectRadius // this const is only used inside the thumper anyway
#else
#define THUMPER_RADIUS 1000
#endif


#define STATE_CHANGE_MODIFIER 0.02f
#define THUMPER_SOUND_DURATION 1.5f
#define THUMPER_MIN_SCALE 128
#define THUMPER_MODEL_NAME "models/props_combine/CombineThumper002.mdl"


ConVar thumper_show_radius("thumper_show_radius","0",FCVAR_CHEAT,"If true, advisor will use her custom impact damage table.");


class CPropThumper : public CBaseAnimating
{
public:
	DECLARE_CLASS( CPropThumper, CBaseAnimating );
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void Think ( void );
	virtual void HandleAnimEvent( animevent_t *pEvent );
	virtual void StopLoopingSounds( void );

	void	InputDisable( inputdata_t &inputdata );
	void	InputEnable( inputdata_t &inputdata );

	void	InitMotorSound( void );

	void	HandleState( void );

	void	Thump( void );

private:
	
	bool m_bEnabled;
	int m_iHammerAttachment;
	CSoundPatch* m_sndMotor;
	EHANDLE m_hRepellantEnt;
	int m_iDustScale;
	
	COutputEvent	m_OnThumped;	// Fired when thumper goes off

#if HL2_EPISODIC
	int m_iEffectRadius;
#endif
};

LINK_ENTITY_TO_CLASS( prop_thumper, CPropThumper );

//-----------------------------------------------------------------------------
// Save/load 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CPropThumper )
	DEFINE_FIELD( m_bEnabled, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hRepellantEnt, FIELD_EHANDLE ),
	DEFINE_FIELD( m_iHammerAttachment, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_iDustScale, FIELD_INTEGER, "dustscale" ),
#if HL2_EPISODIC
	DEFINE_KEYFIELD( m_iEffectRadius, FIELD_INTEGER, "EffectRadius" ),
#endif
	DEFINE_SOUNDPATCH( m_sndMotor ),
	DEFINE_THINKFUNC( Think ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),

	DEFINE_OUTPUT( m_OnThumped, "OnThumped" ),
END_DATADESC()

void CPropThumper::Spawn( void )
{
	char *szModel = (char *)STRING( GetModelName() );
	if (!szModel || !*szModel)
	{
		szModel = THUMPER_MODEL_NAME;
		SetModelName( AllocPooledString(szModel) );
	}

	Precache();
	SetModel( szModel );

	SetSolid( SOLID_VPHYSICS );
	SetMoveType( MOVETYPE_NONE );
	VPhysicsInitStatic();

	BaseClass::Spawn();

	m_bEnabled = true;

	SetThink( &CPropThumper::Think );
	SetNextThink( gpGlobals->curtime + 1.0f );

	int iSequence = SelectHeaviestSequence ( ACT_IDLE );

	if ( iSequence != ACT_INVALID )
	{
		 SetSequence( iSequence );
		 ResetSequenceInfo();

		 //Do this so we get the nice ramp-up effect.
		 m_flPlaybackRate = random->RandomFloat( 0.0f, 1.0f);
	}

	m_iHammerAttachment = LookupAttachment( "hammer" );
	
	CAntlionRepellant *pRepellant = (CAntlionRepellant*)CreateEntityByName( "point_antlion_repellant" );

	if ( pRepellant )
	{
		pRepellant->Spawn();
		pRepellant->SetAbsOrigin( GetAbsOrigin() );
		pRepellant->SetRadius( THUMPER_RADIUS );

		m_hRepellantEnt = pRepellant;
	}

	if ( m_iDustScale == 0 )
		 m_iDustScale = THUMPER_MIN_SCALE;

#if HL2_EPISODIC
	if ( m_iEffectRadius == 0 )
		m_iEffectRadius = 1000;
#endif

}

void CPropThumper::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( STRING( GetModelName() ) );
	PrecacheScriptSound( "coast.thumper_hit" );
	PrecacheScriptSound( "coast.thumper_ambient" );
	PrecacheScriptSound( "coast.thumper_dust" );
	PrecacheScriptSound( "coast.thumper_startup" );
	PrecacheScriptSound( "coast.thumper_shutdown" );
	PrecacheScriptSound( "coast.thumper_large_hit" );
}

void CPropThumper::InitMotorSound( void )
{
	CPASAttenuationFilter filter( this );
	m_sndMotor = (CSoundEnvelopeController::GetController()).SoundCreate( filter, entindex(), CHAN_STATIC, "coast.thumper_ambient" , ATTN_NORM );

	if ( m_sndMotor )
	{
		(CSoundEnvelopeController::GetController()).Play( m_sndMotor, 1.0f, 100 );
	}
}

void CPropThumper::HandleState( void )
{
	if ( m_bEnabled == false )
	{
		 m_flPlaybackRate = MAX( m_flPlaybackRate - STATE_CHANGE_MODIFIER, 0.0f );
	}
	else
	{
 		 m_flPlaybackRate = MIN( m_flPlaybackRate + STATE_CHANGE_MODIFIER, 1.0f );
	}

	(CSoundEnvelopeController::GetController()).Play( m_sndMotor, 1.0f, m_flPlaybackRate * 100 );
}

void CPropThumper::Think( void )
{
	StudioFrameAdvance();
	DispatchAnimEvents( this );
	SetNextThink( gpGlobals->curtime + 0.1 );

	if ( m_sndMotor == NULL )
		 InitMotorSound();
	else
		 HandleState();
}

void CPropThumper::Thump ( void )
{
	if ( m_iHammerAttachment != -1 )
	{
		Vector vOrigin;
		GetAttachment( m_iHammerAttachment, vOrigin );

		CEffectData	data;

		data.m_nEntIndex = entindex();
		data.m_vOrigin = vOrigin;
		data.m_flScale = m_iDustScale * m_flPlaybackRate;
		DispatchEffect( "ThumperDust", data );
		UTIL_ScreenShake( vOrigin, 10.0 * m_flPlaybackRate, m_flPlaybackRate, m_flPlaybackRate / 2, THUMPER_RADIUS * m_flPlaybackRate, SHAKE_START, false );
	}

	EmitSound( "coast.thumper_dust" );
	CSoundEnt::InsertSound ( SOUND_THUMPER, GetAbsOrigin(), THUMPER_RADIUS * m_flPlaybackRate, THUMPER_SOUND_DURATION, this );

	if ( thumper_show_radius.GetBool() )
	{
		NDebugOverlay::Box( GetAbsOrigin(), Vector(-THUMPER_RADIUS, -THUMPER_RADIUS, -THUMPER_RADIUS), Vector(THUMPER_RADIUS, THUMPER_RADIUS, THUMPER_RADIUS), 
			255, 64, 64, 255, THUMPER_SOUND_DURATION );
	}

	if ( m_flPlaybackRate < 0.7f )
		 return;

	if( m_iDustScale == THUMPER_MIN_SCALE )
		 EmitSound( "coast.thumper_hit" );
	else
		 EmitSound( "coast.thumper_large_hit" );

	// Signal that we've thumped
	m_OnThumped.FireOutput( this, this );
}

void CPropThumper::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_THUMPER_THUMP )
	{
		Thump();
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}


//-----------------------------------------------------------------------------
// Shuts down sounds
//-----------------------------------------------------------------------------
void CPropThumper::StopLoopingSounds( void )
{
	if ( m_sndMotor != NULL )
	{
		 (CSoundEnvelopeController::GetController()).SoundDestroy( m_sndMotor );
		 m_sndMotor = NULL;
	}

	BaseClass::StopLoopingSounds();
}

void CPropThumper::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;
	
	EmitSound( "coast.thumper_shutdown" );
	
	if ( m_hRepellantEnt )
	{
		variant_t emptyVariant;
		m_hRepellantEnt->AcceptInput( "Disable", this, this, emptyVariant, 0 );
	}
}

void CPropThumper::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;

	EmitSound( "coast.thumper_startup" );

	if ( m_hRepellantEnt )
	{
		variant_t emptyVariant;
		m_hRepellantEnt->AcceptInput( "Enable", this, this, emptyVariant, 0 );
	}
}
