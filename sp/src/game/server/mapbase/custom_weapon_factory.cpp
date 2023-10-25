//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: The central manager of the custom weapons system.
// 
// Author: Peter Covington (petercov@outlook.com)
//
//==================================================================================//

#include "cbase.h"
#include "custom_weapon_factory.h"

#define GENERIC_MANIFEST_FILE "scripts/mapbase_default_manifest.txt"
#define AUTOLOADED_MANIFEST_FILE UTIL_VarArgs("maps/%s_manifest.txt", STRING(gpGlobals->mapname))
#define GLOBAL_WEAPONS_MANIFEST "scripts/custom_weapon_manifest.txt"

extern ConVar mapbase_load_default_manifest;

IMPLEMENT_PRIVATE_SYMBOLTYPE(CustomWeaponSymbol);

CCustomWeaponSystem::CCustomWeaponSystem() : CAutoGameSystem("CustomWeaponFactorySystem")
{
}

void CCustomWeaponSystem::LevelInitPreEntity()
{
	LoadCustomWeaponsManifest(GLOBAL_WEAPONS_MANIFEST);

	// Check for a generic "mapname_manifest.txt" file and load it.
	if (filesystem->FileExists(AUTOLOADED_MANIFEST_FILE, "GAME"))
	{
		AddManifestFile(AUTOLOADED_MANIFEST_FILE);
	}
	else
	{
		// Load the generic script instead.
		ParseGenericManifest();
	}
}

// Get a generic, hardcoded manifest with hardcoded names.
void CCustomWeaponSystem::ParseGenericManifest()
{
	if (!mapbase_load_default_manifest.GetBool())
		return;

	KeyValues* pKV = new KeyValues("DefaultManifest");
	pKV->LoadFromFile(filesystem, GENERIC_MANIFEST_FILE);

	AddManifestFile(pKV/*, true*/);

	pKV->deleteThis();
}

void CCustomWeaponSystem::AddManifestFile(const char* file)
{
	KeyValues* pKV = new KeyValues(file);
	if (!pKV->LoadFromFile(filesystem, file))
	{
		Warning("Mapbase Manifest: \"%s\" is unreadable or missing (can't load KV, check for syntax errors)\n", file);
		pKV->deleteThis();
		return;
	}

	CGMsg(1, CON_GROUP_MAPBASE_MISC, "===== Mapbase Manifest: Loading manifest file %s =====\n", file);

	AddManifestFile(pKV, false);

	CGMsg(1, CON_GROUP_MAPBASE_MISC, "==============================================================================\n");

	pKV->deleteThis();
}

void CCustomWeaponSystem::AddManifestFile(KeyValues* pKV, bool bDontWarn)
{
	KeyValues* pKey = pKV->FindKey("weapons");

	if (pKey)
	{
		char value[MAX_PATH];
		value[0] = '\0';

		// Parse %mapname%, etc.
		bool inparam = false;
		CUtlStringList outStrings;
		V_SplitString(pKey->GetString(), "%", outStrings);
		for (int i = 0; i < outStrings.Count(); i++)
		{
			if (inparam)
			{
				if (FStrEq(outStrings[i], "mapname"))
				{
					Q_strncat(value, STRING(gpGlobals->mapname), sizeof(value));
				}
				else if (FStrEq(outStrings[i], "language"))
				{
#ifdef CLIENT_DLL
					char uilanguage[64];
					engine->GetUILanguage(uilanguage, sizeof(uilanguage));
					Q_strncat(value, uilanguage, sizeof(value));
#else
					// Give up, use English
					Q_strncat(value, "english", sizeof(value));
#endif
				}
			}
			else
			{
				Q_strncat(value, outStrings[i], sizeof(value));
			}

			inparam = !inparam;
		}

		outStrings.PurgeAndDeleteElements();
		bDontWarn = pKV->GetBool("NoErrors", bDontWarn);

		LoadCustomWeaponsManifest(value, bDontWarn);
	}
}

#define Factory CustomWeaponsFactoryDictionary()
void CCustomWeaponSystem::LoadCustomWeaponsManifest(const char* file, bool bDontWarn)
{
	KeyValuesAD pKV("weapons_manifest");
	if (pKV->LoadFromFile(filesystem, file, "GAME"))
	{
		for (KeyValues *pkvWeapon = pKV->GetFirstValue(); pkvWeapon != nullptr; pkvWeapon = pkvWeapon->GetNextValue())
		{
			const char* pszClassname = pkvWeapon->GetName();
			KeyValuesAD pkvWeaponScript("WeaponData");
			if (pkvWeaponScript->LoadFromFile(filesystem, pkvWeapon->GetString(), "GAME"))
			{
				const char* pszFactory = pkvWeaponScript->GetString("custom_factory", nullptr);
				unsigned short FactoryIndex = Factory.Find(pszFactory);
				if (Factory.IsValidIndex(FactoryIndex))
				{
					auto* pFactory = Factory.Element(FactoryIndex);
					const void* pData = pFactory->ParseDataFromWeaponFile(pkvWeaponScript);
					if (!pData)
						continue;

					unsigned short ClassIndex = m_ClassFactories.Find(pszClassname);
					if (!m_ClassFactories.IsValidIndex(ClassIndex))
					{
						ClassIndex = m_ClassFactories.Insert(pszClassname);
						m_ClassFactories[ClassIndex].pOldFactory = EntityFactoryDictionary()->FindFactory(pszClassname);
					}
					else
					{
						Assert(m_ClassFactories[ClassIndex].pNewFactory);
						Assert(m_ClassFactories[ClassIndex].pData);

						m_ClassFactories[ClassIndex].pNewFactory->ReleaseData(m_ClassFactories[ClassIndex].pData);
					}

					m_ClassFactories[ClassIndex].sDataFile = pkvWeapon->GetString();
					m_ClassFactories[ClassIndex].pNewFactory = pFactory;
					m_ClassFactories[ClassIndex].pData = pData;
					EntityFactoryDictionary()->UninstallFactory(pszClassname);
					EntityFactoryDictionary()->InstallFactory(m_ClassFactories[ClassIndex].pNewFactory, pszClassname);
				}
			}
		}
	}
}
#undef Factory

void CCustomWeaponSystem::LevelShutdownPostEntity()
{
	for (unsigned short i = 0; i < m_ClassFactories.Count(); i++)
	{
		EntityFactoryDictionary()->UninstallFactory(m_ClassFactories.GetElementName(i));
		const CustomClassName_t& entry = m_ClassFactories.Element(i);
		if (entry.pOldFactory)
			EntityFactoryDictionary()->InstallFactory(entry.pOldFactory, m_ClassFactories.GetElementName(i));

		Assert(entry.pData);
		entry.pNewFactory->ReleaseData(entry.pData);
	}

	m_ClassFactories.Purge();
	g_CustomWeaponSymbolSymbolTable.RemoveAll();
}

void CCustomWeaponSystem::ParseWeapon(CBaseCombatWeapon* pWeapon, const char* pClassName)
{
	ICustomWeapon* pCustom = dynamic_cast<ICustomWeapon*> (pWeapon);
	if (!pCustom)
		return;

	unsigned short i = m_ClassFactories.Find(pClassName);
	if (!m_ClassFactories.IsValidIndex(i))
		return;

	pCustom->InitCustomWeaponFromData(m_ClassFactories[i].pData, m_ClassFactories[i].sDataFile.String());
}

CUtlDict<ICustomWeaponDataLoader*, unsigned short>& CustomWeaponsFactoryDictionary()
{
	static CUtlDict<ICustomWeaponDataLoader*, unsigned short> dict;
	return dict;
}

static CCustomWeaponSystem g_CustomWeaponsSystem;
CCustomWeaponSystem* CustomWeaponSystem()
{
	return &g_CustomWeaponsSystem;
}
