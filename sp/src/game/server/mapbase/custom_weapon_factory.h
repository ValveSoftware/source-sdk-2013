//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: See custom_weapon_factory.cpp
// 
// Author: Peter Covington (petercov@outlook.com)
//
//==================================================================================//

#ifndef CUSTOM_WEAPON_FACTORY_H
#define CUSTOM_WEAPON_FACTORY_H
#pragma once
#include "utldict.h"
#include "utlsymbol.h"

DECLARE_PRIVATE_SYMBOLTYPE(CustomWeaponSymbol);

CUtlDict< IEntityFactory*, unsigned short >& CustomWeaponsFactoryDictionary();

class ICustomWeapon
{
public:
	virtual void ParseCustomFromWeaponFile(const char* pFileName) = 0;
};

class CCustomWeaponSystem : public CAutoGameSystem
{
public:
	CCustomWeaponSystem();

	// Level init, shutdown
	virtual void LevelInitPreEntity();
	virtual void LevelShutdownPostEntity();

	void ParseWeapon(CBaseCombatWeapon* pWeapon, const char* pClassName);

private:
	void ParseGenericManifest();
	void AddManifestFile(const char* file);
	void AddManifestFile(KeyValues* pKV, bool bDontWarn = false);
	void LoadCustomWeaponsManifest(const char* file, bool bDontWarn = false);

	typedef struct CustomClassName_s
	{
		CustomWeaponSymbol sDataFile;
		IEntityFactory* pNewFactory;
		IEntityFactory* pOldFactory;
	} CustomClassName_t;
	CUtlDict<CustomClassName_t, unsigned short> m_ClassFactories;
};

CCustomWeaponSystem* CustomWeaponSystem();

template <class T>
class CCustomWeaponEntityFactory : public IEntityFactory
{
public:
	CCustomWeaponEntityFactory(const char* pFactoryClass)
	{
		CustomWeaponsFactoryDictionary().Insert(pFactoryClass, this);
	}

	IServerNetworkable* Create(const char* pClassName)
	{
		T* pEnt = _CreateEntityTemplate((T*)NULL, pClassName);
		CustomWeaponSystem()->ParseWeapon(pEnt, pClassName);
		return pEnt->NetworkProp();
	}

	void Destroy(IServerNetworkable* pNetworkable)
	{
		if (pNetworkable)
		{
			pNetworkable->Release();
		}
	}

	virtual size_t GetEntitySize()
	{
		return sizeof(T);
	}
};

#define DEFINE_CUSTOM_WEAPON_FACTORY(factoryName, DLLClassName) \
	static CCustomWeaponEntityFactory<DLLClassName> custom_weapon_##factoryName##_factory( #factoryName );

#endif // !CUSTOM_WEAPON_FACTORY_H
