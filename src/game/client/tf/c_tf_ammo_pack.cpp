//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "c_tf_ammo_pack.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef _DEBUG
static ConVar tf_debug_weapontrail( "tf_debug_weapontrail", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
#endif // _DEBUG

// Network table.
IMPLEMENT_CLIENTCLASS_DT( C_TFAmmoPack, DT_AmmoPack, CTFAmmoPack )
	RecvPropVector( RECVINFO( m_vecInitialVelocity ) ),
	RecvPropFloat( RECVINFO_NAME( m_angNetworkAngles[0], m_angRotation[0] ) ),
	RecvPropFloat( RECVINFO_NAME( m_angNetworkAngles[1], m_angRotation[1] ) ),
	RecvPropFloat( RECVINFO_NAME( m_angNetworkAngles[2], m_angRotation[2] ) ),
END_RECV_TABLE()


C_TFAmmoPack::C_TFAmmoPack( void )
{
	m_nWorldModelIndex = 0;
}

C_TFAmmoPack::~C_TFAmmoPack( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
// Output : int
//-----------------------------------------------------------------------------
int C_TFAmmoPack::DrawModel( int flags )
{
#ifdef _DEBUG
	// Debug!
	if ( tf_debug_weapontrail.GetBool() )
	{
		Msg( "Ammo Pack:: Position: (%f %f %f), Velocity (%f %f %f)\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z, GetAbsVelocity().x, GetAbsVelocity().y, GetAbsVelocity().z );
		if ( debugoverlay )
		{
			debugoverlay->AddBoxOverlay( GetAbsOrigin(), Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), QAngle( 0, 0, 0 ), 255, 255, 0, 32, 5.0 );
		}
	}
#endif // _DEBUG

	return BaseClass::DrawModel( flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_TFAmmoPack::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

#ifdef _DEBUG
	// Debug!
	if ( tf_debug_weapontrail.GetBool() )
	{
		Msg( "AbsOrigin (%f %f %f), LocalOrigin(%f %f %f)\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z, GetLocalOrigin().x, GetLocalOrigin().y, GetLocalOrigin().z );
	}
#endif // _DEBUG

	if ( updateType == DATA_UPDATE_CREATED )
	{ 
#ifdef _DEBUG
		// Debug!
		if ( tf_debug_weapontrail.GetBool() )
		{
			Msg( "Origin (%f %f %f)\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		}
#endif // _DEBUG

		float flChangeTime = GetLastChangeTime( LATCH_SIMULATION_VAR );
		Vector vecCurOrigin = GetLocalOrigin();

		// Now stick our initial velocity into the interpolation history 
		CInterpolatedVar< Vector > &interpolator = GetOriginInterpolator();
		interpolator.ClearHistory();
		interpolator.AddToHead( flChangeTime - 0.15f, &vecCurOrigin, false );

		m_nWorldModelIndex = m_nModelIndex;
	}
}

int C_TFAmmoPack::GetWorldModelIndex( void )
{
	if ( m_nWorldModelIndex == 0 )
		return m_nModelIndex;

	if ( GameRules() )
	{
		const char *pBaseName = modelinfo->GetModelName( modelinfo->GetModel( m_nWorldModelIndex ) );
		const char *pTranslatedName = GameRules()->TranslateEffectForVisionFilter( "weapons", pBaseName );

		if ( pTranslatedName != pBaseName )
		{
			return modelinfo->GetModelIndex( pTranslatedName );
		}
	}

	return m_nWorldModelIndex;
}

void C_TFAmmoPack::ValidateModelIndex( void )
{
	m_nModelIndex = GetWorldModelIndex();

	BaseClass::ValidateModelIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : currentTime - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TFAmmoPack::Interpolate( float currentTime )
{
	return BaseClass::Interpolate( currentTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void C_TFAmmoPack::DisplayHintTo( C_BasePlayer *pPlayer )
{
	C_TFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
	if ( pTFPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		pTFPlayer->HintMessage( HINT_ENGINEER_PICKUP_METAL );
	}
	else
	{
		pTFPlayer->HintMessage( HINT_PICKUP_AMMO );
	}
}
