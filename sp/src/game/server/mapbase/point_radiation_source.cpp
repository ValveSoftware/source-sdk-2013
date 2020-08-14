//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ====
//
// An entity that triggers the player's geiger counter.
// 
// Doesn't cause any actual damage. Should be parentable.
//
//=============================================================================

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



class CPointRadiationSource : public CPointEntity
{
public:
	DECLARE_CLASS( CPointRadiationSource, CPointEntity );
	DECLARE_DATADESC();

	void Spawn();

	void RadiationThink();

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	bool m_bTestPVS;
	float m_flRadius;
	float m_flIntensity = 1.0f;

	bool m_bDisabled;
};

BEGIN_DATADESC( CPointRadiationSource )

	// Function Pointers
	DEFINE_FUNCTION( RadiationThink ),

	// Fields
	DEFINE_KEYFIELD( m_bTestPVS, FIELD_BOOLEAN, "TestPVS" ),
	DEFINE_INPUT( m_flRadius, FIELD_FLOAT, "SetRadius" ),
	DEFINE_INPUT( m_flIntensity, FIELD_FLOAT, "SetIntensity" ),
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( point_radiation_source, CPointRadiationSource );


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CPointRadiationSource::Spawn( void )
{
	BaseClass::Spawn();

	if (!m_bDisabled)
	{
		SetThink( &CPointRadiationSource::RadiationThink );
		SetNextThink( gpGlobals->curtime + random->RandomFloat(0.0, 0.5) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointRadiationSource::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;

	SetThink( &CPointRadiationSource::RadiationThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointRadiationSource::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;

	SetThink( NULL );
	SetNextThink( TICK_NEVER_THINK );

	// Must update player
	//CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	//if (pPlayer)
	//{
	//	pPlayer->NotifyNearbyRadiationSource(1000);
	//}
}

//-----------------------------------------------------------------------------
// Purpose: Trigger hurt that causes radiation will do a radius check and set
//			the player's geiger counter level according to distance from center
//			of trigger.
//-----------------------------------------------------------------------------
void CPointRadiationSource::RadiationThink( void )
{
	CBasePlayer *pPlayer = NULL;
	if (m_bTestPVS)
	{
		CBaseEntity *pPVSPlayer = CBaseEntity::Instance(UTIL_FindClientInPVS(edict()));
		if (pPVSPlayer)
		{
			pPlayer = static_cast<CBasePlayer*>(pPVSPlayer);
		}
	}
	else
	{
		pPlayer = UTIL_GetLocalPlayer();
	}

	if (pPlayer)
	{
		// get range to player;
		float flRange = pPlayer->GetAbsOrigin().DistTo((GetAbsOrigin()));
		if (m_flRadius <= 0 || flRange < m_flRadius)
		{
			if (m_flIntensity == 0)
			{
				Warning("%s: INTENSITY IS ZERO!!! Can't notify of radiation\n", GetDebugName());
				return;
			}

			//flRange *= 3.0f;
			flRange /= m_flIntensity;
			pPlayer->NotifyNearbyRadiationSource(flRange);
		}
	}

	SetNextThink( gpGlobals->curtime + 0.25 );
}
