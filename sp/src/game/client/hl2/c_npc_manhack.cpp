//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_ai_basenpc.h"
#include "soundenvelope.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_NPC_Manhack : public C_AI_BaseNPC
{
public:
	C_NPC_Manhack() {}

	DECLARE_CLASS( C_NPC_Manhack, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();
 	DECLARE_DATADESC();

	// Purpose: Start the manhack's engine sound.
	virtual void OnDataChanged( DataUpdateType_t type );
	virtual void UpdateOnRemove( void );
	virtual void OnRestore();

private:
	C_NPC_Manhack( const C_NPC_Manhack & );

	// Purpose: Start + stop the manhack's engine sound.
	void SoundInit( void );
	void SoundShutdown( void );

	CSoundPatch		*m_pEngineSound1;
	CSoundPatch		*m_pEngineSound2;
	CSoundPatch		*m_pBladeSound;

	int				m_nEnginePitch1;
	int				m_nEnginePitch2;
	float			m_flEnginePitch1Time;
	float			m_flEnginePitch2Time;
};


//-----------------------------------------------------------------------------
// Save/restore
//-----------------------------------------------------------------------------
BEGIN_DATADESC( C_NPC_Manhack )

//	DEFINE_SOUNDPATCH( m_pEngineSound1 ),
//	DEFINE_SOUNDPATCH( m_pEngineSound2 ),
//	DEFINE_SOUNDPATCH( m_pBladeSound ),

//	DEFINE_FIELD( m_nEnginePitch1, FIELD_INTEGER ),
//	DEFINE_FIELD( m_nEnginePitch2, FIELD_INTEGER ),
//	DEFINE_FIELD( m_flEnginePitch1Time, FIELD_FLOAT ),
//	DEFINE_FIELD( m_flEnginePitch2Time, FIELD_FLOAT ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT(C_NPC_Manhack, DT_NPC_Manhack, CNPC_Manhack)
	RecvPropIntWithMinusOneFlag(RECVINFO(m_nEnginePitch1)),
	RecvPropFloat(RECVINFO(m_flEnginePitch1Time)),
	RecvPropIntWithMinusOneFlag(RECVINFO(m_nEnginePitch2)),
	RecvPropFloat(RECVINFO(m_flEnginePitch2Time)),
END_RECV_TABLE()



//-----------------------------------------------------------------------------
// Purpose: Start the manhack's engine sound.
//-----------------------------------------------------------------------------
void C_NPC_Manhack::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if (( m_nEnginePitch1 < 0 ) || ( m_nEnginePitch2 < 0 ) )
	{
		SoundShutdown();
	}
	else
	{
		SoundInit();
		if ( m_pEngineSound1 && m_pEngineSound2 )
		{
			float dt = ( m_flEnginePitch1Time >= gpGlobals->curtime ) ? m_flEnginePitch1Time - gpGlobals->curtime : 0.0f;
			CSoundEnvelopeController::GetController().SoundChangePitch( m_pEngineSound1, m_nEnginePitch1, dt );
			dt = ( m_flEnginePitch2Time >= gpGlobals->curtime ) ? m_flEnginePitch2Time - gpGlobals->curtime : 0.0f;
			CSoundEnvelopeController::GetController().SoundChangePitch( m_pEngineSound2, m_nEnginePitch2, dt );
		}
	}
}


//-----------------------------------------------------------------------------
// Restore
//-----------------------------------------------------------------------------
void C_NPC_Manhack::OnRestore()
{
	BaseClass::OnRestore();
	SoundInit();
}


//-----------------------------------------------------------------------------
// Purpose: Start the manhack's engine sound.
//-----------------------------------------------------------------------------
void C_NPC_Manhack::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();
	SoundShutdown();
}


//-----------------------------------------------------------------------------
// Purpose: Start the manhack's engine sound.
//-----------------------------------------------------------------------------
void C_NPC_Manhack::SoundInit( void )
{
	if (( m_nEnginePitch1 < 0 ) || ( m_nEnginePitch2 < 0 ) )
		return;

	// play an engine start sound!!
	CPASAttenuationFilter filter( this );

	// Bring up the engine looping sound.
	if( !m_pEngineSound1 )
	{
		m_pEngineSound1 = CSoundEnvelopeController::GetController().SoundCreate( filter, entindex(), "NPC_Manhack.EngineSound1" );
		CSoundEnvelopeController::GetController().Play( m_pEngineSound1, 0.0, m_nEnginePitch1 );
		CSoundEnvelopeController::GetController().SoundChangeVolume( m_pEngineSound1, 0.7, 2.0 );
	}

	if( !m_pEngineSound2 )
	{
		m_pEngineSound2 = CSoundEnvelopeController::GetController().SoundCreate( filter, entindex(), "NPC_Manhack.EngineSound2" );
		CSoundEnvelopeController::GetController().Play( m_pEngineSound2, 0.0, m_nEnginePitch2 );
		CSoundEnvelopeController::GetController().SoundChangeVolume( m_pEngineSound2, 0.7, 2.0 );
	}

	if( !m_pBladeSound )
	{
		m_pBladeSound = CSoundEnvelopeController::GetController().SoundCreate( filter, entindex(), "NPC_Manhack.BladeSound" );
		CSoundEnvelopeController::GetController().Play( m_pBladeSound, 0.0, m_nEnginePitch1 );
		CSoundEnvelopeController::GetController().SoundChangeVolume( m_pBladeSound, 0.7, 2.0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_NPC_Manhack::SoundShutdown(void)
{
	// Kill the engine!
	if ( m_pEngineSound1 )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pEngineSound1 );
		m_pEngineSound1 = NULL;
	}

	// Kill the engine!
	if ( m_pEngineSound2 )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pEngineSound2 );
		m_pEngineSound2 = NULL;
	}

	// Kill the blade!
	if ( m_pBladeSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pBladeSound );
		m_pBladeSound = NULL;
	}
}

