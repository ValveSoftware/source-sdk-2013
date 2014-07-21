//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Deals with singleton  
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#if !defined( CLIENTEFFECTPRECACHESYSTEM_H )
#define CLIENTEFFECTPRECACHESYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "commonmacros.h"
#include "utlvector.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterial.h"

//-----------------------------------------------------------------------------
// Interface to automated system for precaching materials
//-----------------------------------------------------------------------------
class IClientEffect
{
public:
	virtual void Cache( bool precache = true )	= 0;
};

//-----------------------------------------------------------------------------
// Responsible for managing precaching of particles
//-----------------------------------------------------------------------------

class CClientEffectPrecacheSystem : public IGameSystem
{
public:
	virtual char const *Name() { return "CCLientEffectPrecacheSystem"; }

	virtual bool	IsPerFrame() { return false; }

	// constructor, destructor
	CClientEffectPrecacheSystem() {}
	virtual ~CClientEffectPrecacheSystem() {}

	// Init, shutdown
	virtual bool Init() { return true; }
	virtual void PostInit() {}
	virtual void Shutdown();

	// Level init, shutdown
	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity() {}
	virtual void LevelShutdownPreEntity();
	virtual void LevelShutdownPostEntity();

	virtual void OnSave() {}
	virtual void OnRestore() {}
	virtual void SafeRemoveIfDesired() {}

	void Register( IClientEffect *effect );

protected:

	CUtlVector< IClientEffect * >	m_Effects;
};

//Singleton accessor
extern CClientEffectPrecacheSystem	*ClientEffectPrecacheSystem();

//-----------------------------------------------------------------------------
// Deals with automated registering and precaching of materials for effects
//-----------------------------------------------------------------------------

class CClientEffect : public IClientEffect
{
public:

	CClientEffect( void )
	{
		//Register with the main effect system
		ClientEffectPrecacheSystem()->Register( this );
	}

//-----------------------------------------------------------------------------
// Purpose: Precache a material by artificially incrementing its reference counter
// Input  : *materialName - name of the material
//		  : increment - whether to increment or decrement the reference counter
//-----------------------------------------------------------------------------

	inline void ReferenceMaterial( const char *materialName, bool increment = true )
	{
		IMaterial	*material = materials->FindMaterial( materialName, TEXTURE_GROUP_CLIENT_EFFECTS );
		if ( !IsErrorMaterial( material ) )
		{
			if ( increment )
			{
				material->IncrementReferenceCount();
			}
			else
			{
				material->DecrementReferenceCount();
			}
		}
	}
};

//Automatic precache macros

//Beginning
#define	CLIENTEFFECT_REGISTER_BEGIN( className )		\
namespace className {									\
class ClientEffectRegister : public CClientEffect		\
{														\
private:												\
	static const char *m_pszMaterials[];				\
public:													\
	void Cache( bool precache = true );					\
};														\
const char *ClientEffectRegister::m_pszMaterials[] = {

//Material definitions
#define	CLIENTEFFECT_MATERIAL( materialName )	materialName,

//End
#define	CLIENTEFFECT_REGISTER_END( )	};					\
void ClientEffectRegister::Cache( bool precache )			\
{															\
	for ( int i = 0; i < ARRAYSIZE( m_pszMaterials ); i++ )	\
	{														\
		ReferenceMaterial( m_pszMaterials[i], precache );	\
	}														\
}															\
ClientEffectRegister	register_ClientEffectRegister;		\
}

#define	CLIENTEFFECT_REGISTER_END_CONDITIONAL(condition )	};	\
void ClientEffectRegister::Cache( bool precache )				\
{																\
	if ( condition)												\
	{															\
		for ( int i = 0; i < ARRAYSIZE( m_pszMaterials ); i++ )	\
		{														\
			ReferenceMaterial( m_pszMaterials[i], precache );	\
		}														\
	}															\
}																\
ClientEffectRegister	register_ClientEffectRegister;			\
}

#endif	//CLIENTEFFECTPRECACHESYSTEM_H
