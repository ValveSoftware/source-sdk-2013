//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "forcefeedback.h"
#include "igamesystem.h"

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
};

static CForceFeedback g_ForceFeedbackSingleton;
IForceFeedback *forcefeedback = &g_ForceFeedbackSingleton;

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CForceFeedback::Init()
{
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

	CSingleUserRecipientFilter user( player );

	UserMessageBegin( user, "ForceFeedback" );

		WRITE_BYTE( FFMSG_STOPALL ); // Reset effects

	MessageEnd();
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

	CSingleUserRecipientFilter user( player );

	UserMessageBegin( user, "ForceFeedback" );

		WRITE_BYTE( FFMSG_STOP ); // Reset effect
		WRITE_BYTE( effect );

	MessageEnd();
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

	CSingleUserRecipientFilter user( player );

	UserMessageBegin( user, "ForceFeedback" );

		WRITE_BYTE( FFMSG_START ); // Reset effects
		WRITE_BYTE( effect );

		// encode direction as a byte
		int dir = (int)( ( params.m_flDirection / 360.0f ) * 255.0f );
		WRITE_BYTE( dir );

		// encode duration as a signed int
		int duration = (int)params.m_flDuration * 1000.0f;
		WRITE_LONG( duration );
		
		// encode gain as a byte
		byte gain = (byte)clamp( params.m_flGain * 255.0f, 0.0f, 255.0f );

		WRITE_BYTE( gain );
		WRITE_BYTE( params.m_nPriority );
		WRITE_BYTE( params.m_bSolo ? 1 : 0 );

	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
//-----------------------------------------------------------------------------
void CForceFeedback::PauseAll( CBasePlayer *player )
{
	if ( !player )
		return;

	CSingleUserRecipientFilter user( player );

	UserMessageBegin( user, "ForceFeedback" );

		WRITE_BYTE( FFMSG_PAUSE ); // Pause effects

	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
//-----------------------------------------------------------------------------
void CForceFeedback::ResumeAll( CBasePlayer *player )
{
	if ( !player )
		return;

	CSingleUserRecipientFilter user( player );

	UserMessageBegin( user, "ForceFeedback" );

		WRITE_BYTE( FFMSG_RESUME ); // Resume effects

	MessageEnd();
}