
#include "cbase.h"
#include "Gstring/cenv_postprocessing.h"

#ifdef CLIENT_DLL

#include "Gstring/gstring_postprocess.h"

#endif

#ifdef GAME_DLL

#define PPECTRLFLAGS_ENABLE_GODRAYS			0x01
#define PPECTRLFLAGS_ENABLE_BARS			0x02
#define PPECTRLFLAGS_DISABLE_BLOOMFLARE		0x04

#define TRANSITION_THINK_CONTEXT "PPCtrlTransThink"

#endif


CEnv_PostProcessingCtrl *g_pPPCtrl = NULL;


#ifdef GAME_DLL
BEGIN_DATADESC( CEnv_PostProcessingCtrl )

	DEFINE_KEYFIELD( m_vecColor_Godrays, FIELD_VECTOR, "godrayscolor" ),
	DEFINE_KEYFIELD( m_flIntensity_Godrays, FIELD_FLOAT, "godraysintensity" ),

	DEFINE_KEYFIELD( m_flIntensity_ScreenBlur, FIELD_FLOAT, "blurintensity" ),
	DEFINE_KEYFIELD( m_flScreenBlur_Goal, FIELD_FLOAT, "blurintensitygoal" ),
	DEFINE_INPUT( m_flIntensity_ScreenBlur, FIELD_FLOAT, "setblurintensity" ),
	DEFINE_INPUT( m_flScreenBlur_Goal, FIELD_FLOAT, "setblurintensitygoal" ),

	DEFINE_KEYFIELD( m_flIntensity_Dream, FIELD_FLOAT, "dreamintensity" ),
	DEFINE_KEYFIELD( m_flDream_Goal, FIELD_FLOAT, "dreamintensitygoal" ),
	DEFINE_INPUT( m_flIntensity_Dream, FIELD_FLOAT, "setdreamintensity" ),
	DEFINE_INPUT( m_flDream_Goal, FIELD_FLOAT, "setdreamintensitygoal" ),

	DEFINE_KEYFIELD( m_flTransition_Time, FIELD_FLOAT, "transitiontime" ),
	DEFINE_INPUT( m_flTransition_Time, FIELD_FLOAT, "settransitiontime" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "starttransition", InputStartTransition ),
	DEFINE_THINKFUNC( TransitionThink ),

	DEFINE_FIELD( m_bEnable_Godrays, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bEnable_Bars, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bEnable_Bloomflare, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_flCurTransition_StartTime, FIELD_TIME ),
	DEFINE_FIELD( m_flCurTransition_Duration, FIELD_FLOAT ),
	DEFINE_FIELD( m_flCurTransition_ScreenBlurGoal, FIELD_FLOAT ),
	DEFINE_FIELD( m_flCurTransition_DreamGoal, FIELD_FLOAT ),

	DEFINE_INPUTFUNC( FIELD_VECTOR, "godrayssetcolor", InputColorGodrays ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "godrayssetintensity", InputIntensityGodrays ),

	DEFINE_INPUTFUNC( FIELD_VOID, "godraysenable", InputEnableGodrays ),
	DEFINE_INPUTFUNC( FIELD_VOID, "godraysdisable", InputDisableGodrays ),

	DEFINE_INPUTFUNC( FIELD_VOID, "barsenable", InputEnableBars ),
	DEFINE_INPUTFUNC( FIELD_VOID, "barsdisable", InputDisableBars ),

	DEFINE_INPUTFUNC( FIELD_VOID, "bloomflareenable", InputEnableBloomflare ),
	DEFINE_INPUTFUNC( FIELD_VOID, "bloomflaredisable", InputDisableBloomflare ),

END_DATADESC()
#endif

IMPLEMENT_NETWORKCLASS_DT( CEnv_PostProcessingCtrl, CEnv_PostProcessingCtrl_DT )

#ifdef GAME_DLL
	SendPropBool( SENDINFO( m_bEnable_Godrays ) ),
	SendPropBool( SENDINFO( m_bEnable_Bars ) ),
	SendPropBool( SENDINFO( m_bEnable_Bloomflare ) ),
	SendPropVector( SENDINFO( m_vecColor_Godrays ) ),
	SendPropFloat( SENDINFO( m_flIntensity_Godrays ) ),
	SendPropFloat( SENDINFO( m_flIntensity_ScreenBlur ) ),
	SendPropFloat( SENDINFO( m_flIntensity_Dream ) ),
#else
	RecvPropBool( RECVINFO( m_bEnable_Godrays ) ),
	RecvPropBool( RECVINFO( m_bEnable_Bars ) ),
	RecvPropBool( RECVINFO( m_bEnable_Bloomflare ) ),
	RecvPropVector( RECVINFO( m_vecColor_Godrays ) ),
	RecvPropFloat( RECVINFO( m_flIntensity_Godrays ) ),
	RecvPropFloat( RECVINFO( m_flIntensity_ScreenBlur ) ),
	RecvPropFloat( RECVINFO( m_flIntensity_Dream ) ),
#endif

END_NETWORK_TABLE();

LINK_ENTITY_TO_CLASS( env_postprocessing_controller, CEnv_PostProcessingCtrl );

CEnv_PostProcessingCtrl::CEnv_PostProcessingCtrl()
{
	Assert( g_pPPCtrl == NULL );
	g_pPPCtrl = this;

#ifdef GAME_DLL
	m_bEnable_Godrays = true;
	m_bEnable_Bars = true;
	m_bEnable_Bloomflare = true;

	m_flCurTransition_StartTime = 0;
	m_flCurTransition_Duration = 0;
#endif
}

CEnv_PostProcessingCtrl::~CEnv_PostProcessingCtrl()
{
	Assert( g_pPPCtrl == this );
	g_pPPCtrl = NULL;
}

bool CEnv_PostProcessingCtrl::IsGodraysEnabled()
{
	return m_bEnable_Godrays;
}

bool CEnv_PostProcessingCtrl::IsBarsEnabled()
{
	return m_bEnable_Bars;
}

bool CEnv_PostProcessingCtrl::IsBloomflareEnabled()
{
	return m_bEnable_Bloomflare;
}

#ifdef GAME_DLL
void CEnv_PostProcessingCtrl::Spawn()
{
	BaseClass::Spawn();

	SetGodraysEnabled( HasSpawnFlags( PPECTRLFLAGS_ENABLE_GODRAYS ) );
	SetBarsEnabled( HasSpawnFlags( PPECTRLFLAGS_ENABLE_BARS ) );
	SetBloomflareEnabled( !HasSpawnFlags( PPECTRLFLAGS_DISABLE_BLOOMFLARE ) );
}

int CEnv_PostProcessingCtrl::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CEnv_PostProcessingCtrl::InputColorGodrays( inputdata_t &inputdata )
{
	Vector c;
	inputdata.value.Vector3D( c );

	m_vecColor_Godrays = c;
}

void CEnv_PostProcessingCtrl::InputIntensityGodrays( inputdata_t &inputdata )
{
	m_flIntensity_Godrays = inputdata.value.Float();
}

void CEnv_PostProcessingCtrl::SetGodraysEnabled( bool bEnabled )
{
	m_bEnable_Godrays = bEnabled;
}

void CEnv_PostProcessingCtrl::SetBarsEnabled( bool bEnabled )
{
	m_bEnable_Bars = bEnabled;
}

void CEnv_PostProcessingCtrl::SetBloomflareEnabled( bool bEnabled )
{
	m_bEnable_Bloomflare = bEnabled;
}

void CEnv_PostProcessingCtrl::InputEnableGodrays( inputdata_t &inputdata )
{
	SetGodraysEnabled( true );
}

void CEnv_PostProcessingCtrl::InputDisableGodrays( inputdata_t &inputdata )
{
	SetGodraysEnabled( false );
}

void CEnv_PostProcessingCtrl::InputEnableBars( inputdata_t &inputdata )
{
	SetBarsEnabled( true );
}

void CEnv_PostProcessingCtrl::InputDisableBars( inputdata_t &inputdata )
{
	SetBarsEnabled( false );
}

void CEnv_PostProcessingCtrl::InputEnableBloomflare( inputdata_t &inputdata )
{
	SetBloomflareEnabled( true );
}

void CEnv_PostProcessingCtrl::InputDisableBloomflare( inputdata_t &inputdata )
{
	SetBloomflareEnabled( false );
}

void CEnv_PostProcessingCtrl::InputStartTransition( inputdata_t &inputdata )
{
	SetContextThink( &CEnv_PostProcessingCtrl::TransitionThink,
		gpGlobals->curtime + 0.01f,
		TRANSITION_THINK_CONTEXT );

	m_flCurTransition_StartTime = gpGlobals->curtime;
	m_flCurTransition_Duration = m_flTransition_Time;

	m_flCurTransition_ScreenBlurOld = m_flIntensity_ScreenBlur;
	m_flCurTransition_ScreenBlurGoal = m_flScreenBlur_Goal;
	m_flCurTransition_DreamOld = m_flIntensity_Dream;
	m_flCurTransition_DreamGoal = m_flDream_Goal;
}

void CEnv_PostProcessingCtrl::TransitionThink()
{
	float fraction = ( gpGlobals->curtime - m_flCurTransition_StartTime ) / m_flCurTransition_Duration;
	fraction = clamp( fraction, 0, 1 );

	m_flIntensity_ScreenBlur = Lerp( fraction, m_flCurTransition_ScreenBlurOld, m_flCurTransition_ScreenBlurGoal );
	m_flIntensity_Dream = Lerp( fraction, m_flCurTransition_DreamOld, m_flCurTransition_DreamGoal );

	if ( fraction >= 1.0f )
	{
		SetContextThink( NULL, 0, TRANSITION_THINK_CONTEXT );
		return;
	}

	SetContextThink( &CEnv_PostProcessingCtrl::TransitionThink,
		gpGlobals->curtime + 0.01f,
		TRANSITION_THINK_CONTEXT );
}
#else
void CEnv_PostProcessingCtrl::OnDataChanged( DataUpdateType_t t )
{
	BaseClass::OnDataChanged( t );

	SetGodraysColor( m_vecColor_Godrays );
	SetGodraysIntensity( m_flIntensity_Godrays );
}
#endif