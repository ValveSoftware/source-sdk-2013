//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's C_BaseCombatCharacter entity
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basecombatcharacter.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined( CBaseCombatCharacter )
#undef CBaseCombatCharacter	
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter::C_BaseCombatCharacter()
{
	for ( int i=0; i < m_iAmmo.Count(); i++ )
	{
		m_iAmmo.Set( i, 0 );
	}

#ifdef GLOWS_ENABLE
	m_pGlowEffect = NULL;
	m_bGlowEnabled = false;
	m_bOldGlowEnabled = false;
#endif // GLOWS_ENABLE
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter::~C_BaseCombatCharacter()
{
#ifdef GLOWS_ENABLE
	DestroyGlowEffect();
#endif // GLOWS_ENABLE
}

/*
//-----------------------------------------------------------------------------
// Purpose: Returns the amount of ammunition of the specified type the character's carrying
//-----------------------------------------------------------------------------
int	C_BaseCombatCharacter::GetAmmoCount( char *szName ) const
{
	return GetAmmoCount( g_pGameRules->GetAmmoDef()->Index(szName) );
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

#ifdef GLOWS_ENABLE
	m_bOldGlowEnabled = m_bGlowEnabled;
#endif // GLOWS_ENABLE
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

#ifdef GLOWS_ENABLE
	if ( m_bOldGlowEnabled != m_bGlowEnabled )
	{
		UpdateGlowEffect();
	}
#endif // GLOWS_ENABLE
}

//-----------------------------------------------------------------------------
// Purpose: Overload our muzzle flash and send it to any actively held weapon
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::DoMuzzleFlash()
{
	// Our weapon takes our muzzle flash command
	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon )
	{
		pWeapon->DoMuzzleFlash();
		//NOTENOTE: We do not chain to the base here
	}
	else
	{
		BaseClass::DoMuzzleFlash();
	}
}

#ifdef GLOWS_ENABLE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::GetGlowEffectColor( float *r, float *g, float *b )
{
	*r = 0.76f;
	*g = 0.76f;
	*b = 0.76f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::UpdateGlowEffect( void )
{
	// destroy the existing effect
	if ( m_pGlowEffect )
	{
		DestroyGlowEffect();
	}

	// create a new effect
	if ( m_bGlowEnabled )
	{
		float r, g, b;
		GetGlowEffectColor( &r, &g, &b );

		m_pGlowEffect = new CGlowObject( this, Vector( r, g, b ), 1.0, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::DestroyGlowEffect( void )
{
	if ( m_pGlowEffect )
	{
		delete m_pGlowEffect;
		m_pGlowEffect = NULL;
	}
}
#endif // GLOWS_ENABLE

IMPLEMENT_CLIENTCLASS(C_BaseCombatCharacter, DT_BaseCombatCharacter, CBaseCombatCharacter);

// Only send active weapon index to local player
BEGIN_RECV_TABLE_NOBASE( C_BaseCombatCharacter, DT_BCCLocalPlayerExclusive )
	RecvPropTime( RECVINFO( m_flNextAttack ) ),
END_RECV_TABLE();


BEGIN_RECV_TABLE(C_BaseCombatCharacter, DT_BaseCombatCharacter)
	RecvPropDataTable( "bcc_localdata", 0, 0, &REFERENCE_RECV_TABLE(DT_BCCLocalPlayerExclusive) ),
	RecvPropEHandle( RECVINFO( m_hActiveWeapon ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_hMyWeapons), RecvPropEHandle( RECVINFO( m_hMyWeapons[0] ) ) ),
#ifdef GLOWS_ENABLE
	RecvPropBool( RECVINFO( m_bGlowEnabled ) ),
#endif // GLOWS_ENABLE

#ifdef INVASION_CLIENT_DLL
	RecvPropInt( RECVINFO( m_iPowerups ) ),
#endif

END_RECV_TABLE()


BEGIN_PREDICTION_DATA( C_BaseCombatCharacter )

	DEFINE_PRED_ARRAY( m_iAmmo, FIELD_INTEGER,  MAX_AMMO_TYPES, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flNextAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_hActiveWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_ARRAY( m_hMyWeapons, FIELD_EHANDLE, MAX_WEAPONS, FTYPEDESC_INSENDTABLE ),

END_PREDICTION_DATA()
