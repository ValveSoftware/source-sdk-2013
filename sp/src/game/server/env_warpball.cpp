//=============== Source SDK License, All rights reserved. ===============//
//
// Purpose: Add a fancy teleportation effect with entity creation and a
//			console command that invokes it.
//
//========================================================================//
#include "cbase.h"
#include "datacache/imdlcache.h"
#include "tesla.h"
#include <vector>
#include <string>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define BEAM_MAT "sprites/physbeam.vmt"

class CWarpBall : public CBaseEntity
{
public:
	CWarpBall();
	virtual void Activate();
	CBaseEntity* CreateTesla();
	void StoreData(std::vector<std::string> params);

	DECLARE_CLASS(CWarpBall, CBaseEntity);
	DECLARE_DATADESC();

private:
	virtual void WarpThink();

	bool m_bOn;
	float m_flWarptime;
	float m_flWarpLifetime;
	CBaseEntity *m_hWarpTesla;
	std::vector<std::string> m_vData;
};
LINK_ENTITY_TO_CLASS(env_warpball, CWarpBall);

BEGIN_DATADESC(CWarpBall)
	DEFINE_THINKFUNC(WarpThink),
END_DATADESC()

//------------------------------------------------------------------------------
// Purpose: Create an entity of the given type
//------------------------------------------------------------------------------
CBaseEntity* Ent_Create(std::vector<std::string> params, const Vector* pos)
{
	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache(true);

	// Try to create entity
	CBaseEntity *entity = dynamic_cast< CBaseEntity * >(CreateEntityByName(params.at(0).c_str()));
	if (entity)
	{
		entity->Precache();
		entity->WorldAlignSize();

		// Pass in any additional parameters
		if (params.size() > 1)
			for (size_t i = 1; i < params.size(); i += 2)
			{
				const char *pKeyName = params.at(1).c_str();
				const char *pValue = params.at(i + 1).c_str();
				entity->KeyValue(pKeyName, pValue);
			}

		DispatchSpawn(entity);

		// Now attempt to drop into the world
		entity->Teleport(pos, NULL, NULL);
		entity->Activate();
	}
	CBaseEntity::SetAllowPrecache(allowPrecache);
	return entity;
}
void CC_Ent_Create_Portal(const CCommand& args)
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if (!pPlayer)
	{
		return;
	}

	// Parse any additional parameters
	std::vector<std::string> params;
	params.push_back(args[1]);
	Msg("%s\n", params.at(0).c_str());
	for (int i = 2; i + 1 < args.ArgC(); i += 2)
	{
		params.push_back(args[i]);
		params.push_back(args[i+1]);
	}

	CBaseEntity *entity = dynamic_cast< CBaseEntity * >(CreateEntityByName("env_warpball"));
	if (entity)
	{
		DispatchSpawn(entity);
		dynamic_cast<CWarpBall*>(entity)->StoreData(params);

		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors(&forward);
		UTIL_TraceLine(pPlayer->EyePosition(),
			pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH, MASK_SOLID,
			pPlayer, COLLISION_GROUP_NONE, &tr);
		if (tr.fraction != 1.0)
		{
			// Raise the end position up off the floor
			tr.endpos.z += 128;
			entity->Teleport(&tr.endpos, NULL, NULL);
		}
		entity->Activate();
	}
}
static ConCommand ent_portal_create("ent_create_portal", CC_Ent_Create_Portal,
	"Creates an entity of the given type where the player is looking with a teleportation effect. Additional parameters can be passed in the form: ent_create_portal <entity name> <param 1 name> <param 1>...<param N name> <param N>",
	FCVAR_GAMEDLL | FCVAR_CHEAT);
IntervalTimer;

CWarpBall::CWarpBall()
{
	m_bOn = true;
	m_flWarptime = gpGlobals->curtime + 1.5f;
	m_flWarpLifetime = gpGlobals->curtime + 3.0f;
	PrecacheMaterial(BEAM_MAT);
	m_hWarpTesla = nullptr;
}

void CWarpBall::Activate()
{
	BaseClass::Activate();
	SetThink(&CWarpBall::WarpThink);
	SetNextThink(gpGlobals->curtime);
	if (m_bOn)
		m_hWarpTesla = CreateTesla();
}

void CWarpBall::WarpThink()
{
	if (m_bOn)
	{
		if (m_flWarptime <= gpGlobals->curtime)
		{
			// This could be built from some KVs (e.g. EntToSpawn ).
			Ent_Create(m_vData, &GetLocalOrigin());
			m_bOn = false;
		}
	}
	if (m_flWarpLifetime <= gpGlobals->curtime)
	{
		m_hWarpTesla->SUB_Remove();
		m_hWarpTesla = nullptr;
		SUB_Remove();
	}
	else
		SetNextThink(gpGlobals->curtime + 0.05f);
}

CBaseEntity* CWarpBall::CreateTesla()
{
	if (!m_bOn)
		return NULL;

	bool allowPrecache = IsPrecacheAllowed();
	SetAllowPrecache(true);

	CBaseEntity *entity = dynamic_cast< CBaseEntity * >(CreateEntityByName("point_tesla"));
	if (entity)
	{
		entity->Precache();
		entity->KeyValue("m_bOn", true);
		entity->KeyValue("m_flRadius", 96);
		entity->KeyValue("m_SoundName", "DoSpark");
		entity->KeyValue("texture", BEAM_MAT);
		entity->KeyValue("interval_min", 0.10);
		entity->KeyValue("interval_max", 0.15);
		entity->KeyValue("lifetime_min", 0.10);
		entity->KeyValue("lifetime_max", 0.15);
		entity->KeyValue("m_Color", Vector(255, 255, 255)); // (No tinting) Without a color the beams are invisible!

		DispatchSpawn(entity);

		// Let's share space
		entity->Teleport(&GetLocalOrigin(), NULL, NULL);
		entity->Activate();
	}
	SetAllowPrecache(allowPrecache);

	return entity;
}

void CWarpBall::StoreData(std::vector<std::string> params)
{
	m_vData = params;
	// This could be parsed into some KVs (e.g. EntToSpawn ).
}
