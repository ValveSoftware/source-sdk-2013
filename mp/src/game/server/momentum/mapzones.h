#ifndef MAPZONES_H
#define MAPZONES_H
#ifdef _WIN32
#pragma once
#endif

#include "filesystem.h"
#include "mom_triggers.h"

#define MOMZONETYPE_START       0
#define MOMZONETYPE_CP          1
#define MOMZONETYPE_STOP        2
#define MOMZONETYPE_ONEHOP      3
#define MOMZONETYPE_RESETONEHOP 4
#define MOMZONETYPE_CPTELE      5
#define MOMZONETYPE_MULTIHOP    6
#define MOMZONETYPE_STAGE       7

class CMapzone
{
public:
    CMapzone();
    CMapzone(const int, Vector*, QAngle*, Vector*, Vector*,
        const int, const bool, const bool, const float,
        const bool, const float, const float, const string_t, const bool);
    ~CMapzone();

    void SpawnZone();
    void RemoveZone();

    int GetType() { return m_type; }
    Vector* GetPosition() { return m_pos; }
    QAngle* GetRotation() { return m_rot; }
    Vector* GetScaleMins() { return m_scaleMins; }
    Vector* GetScaleMaxs() { return m_scaleMaxs; }

private:
    int m_type; // Zone type, look above
    int m_index; // Ignored when not a checkpoint
    bool m_shouldStopOnTeleport; // Stop player on teleport?
    bool m_shouldResetAngles; // Reset the player's angles?
    float m_holdTimeBeforeTeleport; // How much to wait for before teleporting
    // startTrigger
    bool m_limitingspeed; // Limit leave speed?
    bool m_onlyxycheck; // Only checking speed in XY?
    float m_maxleavespeed; // Max speed allowed
    float m_yaw; // Teleport yaw for start zone.
    string_t m_linkedent; // Entity name for teleporting to this entity (YESYES, It can be null!)
    Vector* m_pos;
    QAngle* m_rot;
    Vector* m_scaleMins;
    Vector* m_scaleMaxs;
    CBaseEntity* m_trigger;
};

class CMapzoneData
{
public:
    CMapzoneData(const char *szMapName);
    ~CMapzoneData();

    
    void SpawnMapZones();
    void RemoveMapZones();
    bool MapZoneSpawned(CMapzone*);
    bool LoadFromFile(const char*);

private:
    const char* c_mapPath = "maps/";
    const char* c_zoneFileEnding = ".zon";

    CUtlVector<CMapzone*> m_zones;
};

bool ZoneTypeToClass(int type, char *dest);

#endif