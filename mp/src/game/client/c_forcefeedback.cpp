//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"
#include "forcefeedback.h"
#include "hud_macros.h"
#include "input.h"

#define FF_CLIENT_FLAG	0x8000

class FFParams
{
public:
	FORCEFEEDBACK_t	m_nEffectType;
	FFBaseParams_t	m_BaseParams;
};

struct FFEffectInfo_t
{
	FORCEFEEDBACK_t	effectType;
	char const		*name;
};
	
#define DECLARE_FFEFFECT( name )	{ name, #name }

static FFEffectInfo_t g_EffectTypes[] =
{
	DECLARE_FFEFFECT( FORCE_FEEDBACK_SHOT_SINGLE ),
	DECLARE_FFEFFECT( FORCE_FEEDBACK_SHOT_DOUBLE ),
	DECLARE_FFEFFECT( FORCE_FEEDBACK_TAKEDAMAGE ),
	DECLARE_FFEFFECT( FORCE_FEEDBACK_SCREENSHAKE ),
	DECLARE_FFEFFECT( FORCE_FEEDBACK_SKIDDING ),
	DECLARE_FFEFFECT( FORCE_FEEDBACK_BREAKING )
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : effect - 
// Output : char const
//-----------------------------------------------------------------------------
char const *NameForForceFeedbackEffect( FORCEFEEDBACK_t effect )
{
	int c = ARRAYSIZE( g_EffectTypes );
	if ( (int)effect < 0 || (int)effect >= c )
		return "???";

	const FFEffectInfo_t& info = g_EffectTypes[ (int)effect ];
	Assert( info.effectType == effect );
	return info.name;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : FORCEFEEDBACK_t
//-----------------------------------------------------------------------------
FORCEFEEDBACK_t ForceFeedbackEffectForName( const char *name )
{
	int c = ARRAYSIZE( g_EffectTypes );
	for ( int i = 0 ; i < c; ++i )
	{
		const FFEffectInfo_t& info = g_EffectTypes[ i ];

		if ( !Q_stricmp( info.name, name ) )
			return info.effectType;
	}

	return ( FORCEFEEDBACK_t )-1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CForceFeedback : public IForceFeedback, public CAutoGameSystem
{
public:
	virtual bool			Init();
	virtual void			Shutdown();

	// API
	virtual void			StopAllEffects( CBasePlayer *player );
	virtual void			StopEffect( CBasePlayer *player, FORCEFEEDBACK_t effect );
	virtual void			StartEffect( CBasePlayer *player, FORCEFEEDBACK_t effect, const FFBaseParams_t& params );

	virtual void			PauseAll( CBasePlayer *player );
	virtual void			ResumeAll( CBasePlayer *player );

	void					MsgFunc_ForceFeedback( bf_read &msg );

private:

	void					Internal_StopAllEffects();
	void					Internal_StopEffect( FORCEFEEDBACK_t effect );
	void					Internal_StartEffect( FORCEFEEDBACK_t, const FFBaseParams_t& params );
	void					Internal_PauseAll();
	void					Internal_ResumeAll();
};

static CForceFeedback g_ForceFeedbackSingleton;
IForceFeedback *forcefeedback = &g_ForceFeedbackSingleton;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &msg - 
//-----------------------------------------------------------------------------
void __MsgFunc_ForceFeedback( bf_read &msg )
{
	g_ForceFeedbackSingleton.MsgFunc_ForceFeedback( msg );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CForceFeedback::Init()
{
	HOOK_MESSAGE( ForceFeedback );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CForceFeedback::Shutdown()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
//-----------------------------------------------------------------------------
void CForceFeedback::StopAllEffects( CBasePlayer *player )
{
	if ( !player )
		return;

	Internal_StopAllEffects();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
//			effect - 
//-----------------------------------------------------------------------------
void CForceFeedback::StopEffect( CBasePlayer *player, FORCEFEEDBACK_t effect )
{
	if ( !player )
		return;

	Internal_StopEffect( effect );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
//			effect - 
//			params - 
//-----------------------------------------------------------------------------
void CForceFeedback::StartEffect( CBasePlayer *player, FORCEFEEDBACK_t effect, const FFBaseParams_t& params )
{
	if ( !player )
	{
		return;
	}

	Internal_StartEffect( effect, params );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
//-----------------------------------------------------------------------------
void CForceFeedback::PauseAll( CBasePlayer *player )
{
	if ( !player )
		return;

	Internal_PauseAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
//-----------------------------------------------------------------------------
void CForceFeedback::ResumeAll( CBasePlayer *player )
{
	if ( !player )
		return;

	Internal_ResumeAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CForceFeedback::Internal_StopAllEffects()
{
	input->ForceFeedback_StopAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : heffect - 
//-----------------------------------------------------------------------------
void CForceFeedback::Internal_StopEffect( FORCEFEEDBACK_t effect )
{
	input->ForceFeedback_Stop( effect );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : effect - 
//-----------------------------------------------------------------------------
void CForceFeedback::Internal_StartEffect( FORCEFEEDBACK_t effect, const FFBaseParams_t& params)
{
	char const *name = NameForForceFeedbackEffect( effect );
	Msg( "Starting FF effect '%s'\n", name );

	FFParams p;
	p.m_nEffectType = effect;
	p.m_BaseParams = params;

	input->ForceFeedback_Start( effect, params );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CForceFeedback::Internal_PauseAll()
{
	input->ForceFeedback_Pause();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CForceFeedback::Internal_ResumeAll()
{
	input->ForceFeedback_Resume();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszName - 
//			iSize - 
//			*pbuf - 
//-----------------------------------------------------------------------------
void CForceFeedback::MsgFunc_ForceFeedback( bf_read &msg )
{
	byte msgType = msg.ReadByte();

	switch ( msgType )
	{
	default:
		{
			Warning( "Bad parse in MsgFunc_ForceFeedback!\n" );
		}
		break;
	case FFMSG_STOPALL:
		{
			Internal_StopAllEffects();
		}
		break;
	case FFMSG_START:
		{
			FORCEFEEDBACK_t effectType = (FORCEFEEDBACK_t)msg.ReadByte();

			FFBaseParams_t params;
			params.m_flDirection = 360.0f * ( (byte)msg.ReadByte() / 255.0f );
			params.m_flDuration = (float)msg.ReadLong() / 1000.0f;
			params.m_flGain = ( (byte)msg.ReadByte() / 255.0f );
			params.m_nPriority = msg.ReadByte();
			params.m_bSolo = msg.ReadByte() == 0 ? false : true;

			if ( effectType >= 0 && effectType < NUM_FORCE_FEEDBACK_PRESETS )
			{
				Internal_StartEffect( effectType, params );
			}
			else
			{
				Warning( "Bad parse in MsgFunc_ForceFeedback, FFMSG_START (%i)!\n", effectType );
			}
		}
		break;
	case FFMSG_STOP:
		{
			FORCEFEEDBACK_t effectType = (FORCEFEEDBACK_t)msg.ReadByte();

			Internal_StopEffect( effectType );
		}
		break;
	case FFMSG_PAUSE:
		{
			Internal_PauseAll();
		}
		break;
	case FFMSG_RESUME:
		{
			Internal_ResumeAll();
		}
		break;
	}
}
