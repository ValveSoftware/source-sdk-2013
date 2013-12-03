//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ar2_explosion.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define AR2EXPLOSION_ENTITYNAME	"ar2explosion"


IMPLEMENT_SERVERCLASS_ST(AR2Explosion, DT_AR2Explosion)
	SendPropString( SENDINFO( m_szMaterialName ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(ar2explosion, AR2Explosion);


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( AR2Explosion )

	DEFINE_AUTO_ARRAY( m_szMaterialName, FIELD_CHARACTER ),

END_DATADESC()


AR2Explosion* AR2Explosion::CreateAR2Explosion(const Vector &pos)
{
	CBaseEntity *pEnt = CreateEntityByName(AR2EXPLOSION_ENTITYNAME);
	if(pEnt)
	{
		AR2Explosion *pEffect = dynamic_cast<AR2Explosion*>(pEnt);
		if(pEffect && pEffect->edict())
		{
			pEffect->SetLocalOrigin( pos );
			pEffect->Activate();
			return pEffect;
		}
		else
		{
			UTIL_Remove(pEnt);
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// A lightweight entity for level-designer placed AR2 explosions.
//-----------------------------------------------------------------------------
class CEnvAR2Explosion : public CPointEntity
{
public:
	DECLARE_CLASS( CEnvAR2Explosion, CPointEntity );

	void Spawn( void );

	// Input handlers
	void InputExplode( inputdata_t &inputdata );

	DECLARE_DATADESC();

private:

	string_t m_iszMaterialName;
};


BEGIN_DATADESC( CEnvAR2Explosion )
	DEFINE_INPUTFUNC(FIELD_VOID, "Explode", InputExplode),
	DEFINE_KEYFIELD(m_iszMaterialName, FIELD_STRING, "material"),
END_DATADESC()

LINK_ENTITY_TO_CLASS( env_ar2explosion, CEnvAR2Explosion );


//-----------------------------------------------------------------------------
// Purpose: So you can see where this function begins and the last one ends.
//-----------------------------------------------------------------------------
void CEnvAR2Explosion::Spawn( void )
{ 
	Precache();

	SetSolid( SOLID_NONE );
	AddEffects( EF_NODRAW );

	SetMoveType( MOVETYPE_NONE );
}


//-----------------------------------------------------------------------------
// Purpose: Creates the explosion effect.
//-----------------------------------------------------------------------------
void CEnvAR2Explosion::InputExplode( inputdata_t &inputdata )
{
	AR2Explosion *pExplosion = AR2Explosion::CreateAR2Explosion(GetAbsOrigin());
	if (pExplosion)
	{
		pExplosion->SetLifetime( 10 );
		if (m_iszMaterialName != NULL_STRING)
		{
			pExplosion->SetMaterialName(STRING(m_iszMaterialName));
		}
	}
}
