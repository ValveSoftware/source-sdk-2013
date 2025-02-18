//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_tf_buff_banner.h"
#include "c_tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( TFBuffBanner, DT_TFBuffBanner )

BEGIN_NETWORK_TABLE( C_TFBuffBanner, DT_TFBuffBanner )
END_NETWORK_TABLE()


C_TFBuffBanner::C_TFBuffBanner()
{
	m_flDetachTime = 0.f;
	m_iBuffType = 0;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFBuffBanner::NotifyBoneAttached( C_BaseAnimating* attachTarget )
{
	if ( m_hBuffItem )
	{
		if ( attachTarget != m_hBuffItem->GetOwner() )
		{
			// We are being moved to a corpse. Let our associated buff item know.
			m_hBuffItem->SetBanner( NULL );
		}
		else
		{
			float flDuration = 10.f;	// 10 is default
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( attachTarget, flDuration, mod_buff_duration );
			m_flDetachTime = gpGlobals->curtime + flDuration;

			SetNextClientThink( CLIENT_THINK_ALWAYS );
		}
	}

	BaseClass::NotifyBoneAttached( attachTarget );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBuffBanner::ClientThink( void )
{
	BaseClass::ClientThink();

	// DO THIS AFTER BASECLASS::CLIENTTHINK
	if ( !m_pAttachedTo || !m_hBuffItem )
	{
		if ( m_hBuffItem )
		{
			m_hBuffItem->SetBanner( NULL );
		}
		Release();
		return;
	}

	// Parachute's never expire
	if ( m_iBuffType == EParachute )
	{
		m_flDetachTime = gpGlobals->curtime + 10.0f;
	}

	// Normal Banners
	if ( m_pAttachedTo )
	{	
		if ( gpGlobals->curtime > m_flDetachTime || !m_hBuffItem )
		{
			// Destroy us automatically after a period of time.
			if ( m_hBuffItem )
			{
				m_hBuffItem->SetBanner( NULL );
			}
			Release();
		}
		else if ( m_pAttachedTo->IsEffectActive( EF_NODRAW ) && !IsEffectActive( EF_NODRAW ) )
		{
			AddEffects( EF_NODRAW );
			UpdateVisibility();
		}
		else if ( !m_pAttachedTo->IsEffectActive( EF_NODRAW ) && IsEffectActive( EF_NODRAW ) && (m_pAttachedTo != C_BasePlayer::GetLocalPlayer()) )
		{
			RemoveEffects( EF_NODRAW );
			UpdateVisibility();
		}
	}
}