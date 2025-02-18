//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include <KeyValues.h>
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/itexture.h"
#include "materialsystem/imaterialsystem.h"
#include "functionproxy.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );

//-----------------------------------------------------------------------------
// Returns the proximity of the player to the entity
//-----------------------------------------------------------------------------

class CPlayerProximityProxy : public CResultProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
	float	m_Factor;
};

bool CPlayerProximityProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	m_Factor = pKeyValues->GetFloat( "scale", 0.002 );
	return true;
}

void CPlayerProximityProxy::OnBind( void *pC_BaseEntity )
{
	if (!pC_BaseEntity)
		return;

	// Find the distance between the player and this entity....
	C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
	C_BaseEntity* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	Vector delta;
	VectorSubtract( pEntity->WorldSpaceCenter(), pPlayer->WorldSpaceCenter(), delta );

	Assert( m_pResult );
	SetFloatResult( delta.Length() * m_Factor );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CPlayerProximityProxy, IMaterialProxy, "PlayerProximity" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Returns true if the player's team matches that of the entity the proxy material is attached to
//-----------------------------------------------------------------------------

class CPlayerTeamMatchProxy : public CResultProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
};

bool CPlayerTeamMatchProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	return true;
}

void CPlayerTeamMatchProxy::OnBind( void *pC_BaseEntity )
{
	if (!pC_BaseEntity)
		return;

	// Find the distance between the player and this entity....
	C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
	C_BaseEntity* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	Assert( m_pResult );
	SetFloatResult( (pEntity->GetTeamNumber() == pPlayer->GetTeamNumber()) ? 1.0 : 0.0 );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CPlayerTeamMatchProxy, IMaterialProxy, "PlayerTeamMatch" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Returns the player view direction
//-----------------------------------------------------------------------------
class CPlayerViewProxy : public CResultProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
	float	m_Factor;
};

bool CPlayerViewProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	m_Factor = pKeyValues->GetFloat( "scale", 2 );
	return true;
}

void CPlayerViewProxy::OnBind( void *pC_BaseEntity )
{
	if (!pC_BaseEntity)
		return;

	// Find the view angle between the player and this entity....
	C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
	C_BaseEntity* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	Vector delta;
	VectorSubtract( pEntity->WorldSpaceCenter(), pPlayer->WorldSpaceCenter(), delta );
	VectorNormalize( delta );

	Vector forward;
	AngleVectors( pPlayer->GetAbsAngles(), &forward );

	Assert( m_pResult );
	SetFloatResult( DotProduct( forward, delta ) * m_Factor );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CPlayerViewProxy, IMaterialProxy, "PlayerView" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Returns the player speed
//-----------------------------------------------------------------------------
class CPlayerSpeedProxy : public CResultProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
	float	m_Factor;
};

bool CPlayerSpeedProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	m_Factor = pKeyValues->GetFloat( "scale", 0.005 );
	return true;
}

void CPlayerSpeedProxy::OnBind( void *pC_BaseEntity )
{
	// Find the player speed....
	C_BaseEntity* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	Assert( m_pResult );
	SetFloatResult( pPlayer->GetLocalVelocity().Length() * m_Factor );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CPlayerSpeedProxy, IMaterialProxy, "PlayerSpeed" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Returns the player position
//-----------------------------------------------------------------------------
class CPlayerPositionProxy : public CResultProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
	float	m_Factor;
};

bool CPlayerPositionProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;
	  
	m_Factor = pKeyValues->GetFloat( "scale", 0.005 );
	return true;
}

void CPlayerPositionProxy::OnBind( void *pC_BaseEntity )
{
	// Find the player speed....
	C_BaseEntity* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	// This is actually a vector...
	Assert( m_pResult );
	Vector res;
	VectorMultiply( pPlayer->WorldSpaceCenter(), m_Factor, res ); 
	m_pResult->SetVecValue( res.Base(), 3 );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CPlayerPositionProxy, IMaterialProxy, "PlayerPosition" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Returns the entity speed
//-----------------------------------------------------------------------------
class CEntitySpeedProxy : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity );
};

void CEntitySpeedProxy::OnBind( void *pC_BaseEntity )
{
	// Find the view angle between the player and this entity....
	if (!pC_BaseEntity)
		return;

	// Find the view angle between the player and this entity....
	C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );

	Assert( m_pResult );
	m_pResult->SetFloatValue( pEntity->GetLocalVelocity().Length() );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CEntitySpeedProxy, IMaterialProxy, "EntitySpeed" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Returns a random # from 0 - 1 specific to the entity it's applied to
//-----------------------------------------------------------------------------
class CEntityRandomProxy : public CResultProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
	CFloatInput	m_Factor;
};

bool CEntityRandomProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	if (!m_Factor.Init( pMaterial, pKeyValues, "scale", 1 ))
		return false;

	return true;
}

void CEntityRandomProxy::OnBind( void *pC_BaseEntity )
{
	// Find the view angle between the player and this entity....
	if (!pC_BaseEntity)
		return;

	// Find the view angle between the player and this entity....
	C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );

	Assert( m_pResult );
	m_pResult->SetFloatValue( pEntity->ProxyRandomValue() * m_Factor.GetFloat() );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CEntityRandomProxy, IMaterialProxy, "EntityRandom" IMATERIAL_PROXY_INTERFACE_VERSION );

#include "utlrbtree.h"

//-----------------------------------------------------------------------------
// Returns the player speed
//-----------------------------------------------------------------------------
class CPlayerLogoProxy : public IMaterialProxy
{
public:
	CPlayerLogoProxy();

	virtual bool Init( IMaterial* pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );
	virtual void Release()
	{
		if ( m_pDefaultTexture )
		{
			m_pDefaultTexture->DecrementReferenceCount();
		}

		int c = m_Logos.Count();
		int i;
		for ( i = 0; i < c ; i++ )
		{
			PlayerLogo *logo = &m_Logos[ i ];
			if( logo->texture )
			{
				logo->texture->DecrementReferenceCount();
			}
		}

		m_Logos.RemoveAll();
	}

	virtual IMaterial *GetMaterial();

protected:
	virtual void	OnLogoBindInternal( int playerindex );

private:
	IMaterialVar *m_pBaseTextureVar;

	struct PlayerLogo
	{
		unsigned int			crc;
		ITexture			*texture;
	};

	static bool LogoLessFunc( const PlayerLogo& src1, const PlayerLogo& src2 )
	{
		return src1.crc < src2.crc;
	}

	CUtlRBTree< PlayerLogo >	m_Logos;
	ITexture					*m_pDefaultTexture;

};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPlayerLogoProxy::CPlayerLogoProxy()
: m_Logos( 0, 0, LogoLessFunc )
{
	m_pDefaultTexture = NULL;
}

#define DEFAULT_DECAL_NAME "decals/YBlood1"

bool CPlayerLogoProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	bool found = false;
	m_pBaseTextureVar = pMaterial->FindVar( "$basetexture", &found );
	if ( !found )
		return false;

	m_pDefaultTexture = materials->FindTexture( DEFAULT_DECAL_NAME, TEXTURE_GROUP_DECAL );
	if ( IsErrorTexture( m_pDefaultTexture ) )
		return false;

	m_pDefaultTexture->IncrementReferenceCount();

	return true;
}

void CPlayerLogoProxy::OnBind( void *pC_BaseEntity )
{
	// Decal's are bound with the player index as the passed in paramter
	int playerindex = (int)(intp)pC_BaseEntity;

	if ( playerindex <= 0 )
		return;

	if ( playerindex > gpGlobals->maxClients )
		return;

	if ( !m_pBaseTextureVar )
		return;

	OnLogoBindInternal( playerindex );
}

void CPlayerLogoProxy::OnLogoBindInternal( int playerindex )
{
	// Find player
	player_info_t info;
	engine->GetPlayerInfo( playerindex, &info );

	if ( !info.customFiles[0] ) 
		return;

	// So we don't trash this too hard

	ITexture *texture = NULL;

	PlayerLogo logo;
	logo.crc = (unsigned int)info.customFiles[0];
	logo.texture = NULL;

	int lookup = m_Logos.Find( logo );
	if ( lookup == m_Logos.InvalidIndex() )
	{
		char crcfilename[ 512 ];
		char logohex[ 16 ];
		Q_binarytohex( (byte *)&info.customFiles[0], sizeof( info.customFiles[0] ), logohex, sizeof( logohex ) );

		Q_snprintf( crcfilename, sizeof( crcfilename ), "temp/%s", logohex );

		texture = materials->FindTexture( crcfilename, TEXTURE_GROUP_DECAL, false );
		if ( texture )
		{
			// Make sure it doesn't get flushed
			texture->IncrementReferenceCount();
			logo.texture = texture;
		}

		m_Logos.Insert( logo );
	}
	else
	{
		texture = m_Logos[ lookup ].texture;
	}

	if ( texture )
	{
		m_pBaseTextureVar->SetTextureValue( texture );
	}
	else if ( m_pDefaultTexture )
	{
		m_pBaseTextureVar->SetTextureValue( m_pDefaultTexture );
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

IMaterial *CPlayerLogoProxy::GetMaterial()
{
	return m_pBaseTextureVar->GetOwningMaterial();
}

EXPOSE_INTERFACE( CPlayerLogoProxy, IMaterialProxy, "PlayerLogo" IMATERIAL_PROXY_INTERFACE_VERSION );

/* @note Tom Bui: This is here for reference, but we don't want people to use it!
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CPlayerLogoOnModelProxy : public CPlayerLogoProxy
{
public:
	virtual void OnBind( void *pC_BaseEntity );
};

void CPlayerLogoOnModelProxy::OnBind( void *pC_BaseEntity )
{
	if ( pC_BaseEntity )
	{
		IClientRenderable *pRend = (IClientRenderable *)pC_BaseEntity;
		C_BaseEntity *pEntity = pRend->GetIClientUnknown()->GetBaseEntity();
		if ( pEntity )
		{
			if ( !pEntity->IsPlayer() )
			{
				pEntity = pEntity->GetRootMoveParent();
			}

			if ( pEntity && pEntity->IsPlayer() )
			{
				int iPlayerIndex = pEntity->entindex();

				OnLogoBindInternal( iPlayerIndex );
			}
		}
	}
}

EXPOSE_INTERFACE( CPlayerLogoOnModelProxy, IMaterialProxy, "PlayerLogoOnModel" IMATERIAL_PROXY_INTERFACE_VERSION );
*/