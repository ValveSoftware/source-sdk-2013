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

class ICustomWeaponDataLoader : public IEntityFactory
{
public:
	virtual const void* ParseDataFromWeaponFile(KeyValues* pKV) const = 0;
	virtual void ReleaseData(const void* pData) const = 0;
};

class ICustomWeapon
{
public:
	virtual void InitCustomWeaponFromData(const void* pData, const char *pszWeaponScript) = 0;
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
		ICustomWeaponDataLoader* pNewFactory;
		IEntityFactory* pOldFactory;
		const void* pData;
	} CustomClassName_t;
	CUtlDict<CustomClassName_t, unsigned short> m_ClassFactories;
};

CCustomWeaponSystem* CustomWeaponSystem();

CUtlDict< ICustomWeaponDataLoader*, unsigned short >& CustomWeaponsFactoryDictionary();

template <class T>
class CCustomWeaponEntityFactoryBase : public ICustomWeaponDataLoader
{
public:
	CCustomWeaponEntityFactoryBase(const char* pFactoryClass)
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

template <class Entity, class Data>
class CDefaultCustomWeaponEntityFactory : public CCustomWeaponEntityFactoryBase<Entity>
{
public:
	CDefaultCustomWeaponEntityFactory(const char *pFactoryClass) : CCustomWeaponEntityFactoryBase<Entity>(pFactoryClass)
	{}

	virtual const void* ParseDataFromWeaponFile(KeyValues* pKV) const
	{
		Data* pData = new Data;
		if (pData && pData->Parse(pKV))
			return pData;

		delete pData;
		return nullptr;
	}

	virtual void ReleaseData(const void* pData) const
	{
		delete pData;
	}
};

#define DEFINE_CUSTOM_WEAPON_FACTORY(factoryName, DLLClassName, DataStruct) \
	static CDefaultCustomWeaponEntityFactory<DLLClassName, DataStruct> custom_weapon_##factoryName##_factory( #factoryName );

#endif // !CUSTOM_WEAPON_FACTORY_H
