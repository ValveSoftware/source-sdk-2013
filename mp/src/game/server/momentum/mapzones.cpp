#include "cbase.h"
#include "mapzones.h"
#include "Timer.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "mom_triggers.h"

#include "tier0/memdbgon.h"


#define NO_LOOK -190.0f

CMapzone::~CMapzone()
{
    if (m_pos)
    {
        delete m_pos;
        m_pos = NULL;
    }
    if (m_rot)
    {
        delete m_rot;
        m_rot = NULL;
    }
    if (m_scaleMins)
    {
        delete m_scaleMins;
        m_scaleMins = NULL;
    }
    if (m_scaleMaxs)
    {
        delete m_scaleMaxs;
        m_scaleMaxs = NULL;
    }
}

CMapzone::CMapzone(const int pType, Vector* pPos, QAngle* pRot, Vector* pScaleMins,
    Vector* pScaleMaxs, const int pIndex, const bool pShouldStop, const bool pShouldTilt,
    const float pHoldTime, const bool pLimitSpeed,
    const float pMaxLeaveSpeed, const float flYaw,
    const string_t pLinkedEnt, const bool pCheckOnlyXY)
{
    m_type = pType;
    m_pos = pPos;
    m_rot = pRot;
    m_scaleMins = pScaleMins;
    m_scaleMaxs = pScaleMaxs;
    m_index = pIndex;
    m_shouldStopOnTeleport = pShouldStop;
    m_shouldResetAngles = pShouldTilt;
    m_holdTimeBeforeTeleport = pHoldTime;
    m_limitingspeed = pLimitSpeed;
    m_maxleavespeed = pMaxLeaveSpeed;
    m_yaw = flYaw;
    m_linkedent = pLinkedEnt;
    m_onlyxycheck = pCheckOnlyXY;
}

void CMapzone::SpawnZone()
{
    switch (m_type)
    {
    case MOMZONETYPE_START:
        m_trigger = (CTriggerTimerStart *) CreateEntityByName("trigger_momentum_timer_start");
        ((CTriggerTimerStart *) m_trigger)->SetIsLimitingSpeed(m_limitingspeed);
        ((CTriggerTimerStart *) m_trigger)->SetMaxLeaveSpeed(m_maxleavespeed);
        ((CTriggerTimerStart *) m_trigger)->SetIsLimitingSpeedOnlyXY(m_onlyxycheck);
        if ( m_yaw != NO_LOOK )
        {
            ((CTriggerTimerStart *) m_trigger)->SetHasLookAngles(true);
            ((CTriggerTimerStart *) m_trigger)->SetLookAngles(QAngle( 0, m_yaw, 0 ));
        }
        else
        {
            ((CTriggerTimerStart *) m_trigger)->SetHasLookAngles(false);
        }
        
        m_trigger->SetName(MAKE_STRING("Start Trigger"));
        g_Timer.SetStartTrigger((CTriggerTimerStart *) m_trigger);
        break;
    case MOMZONETYPE_CP:
        m_trigger = (CTriggerCheckpoint *) CreateEntityByName("trigger_momentum_timer_checkpoint");
        m_trigger->SetName(MAKE_STRING("Checkpoint Trigger"));
        ((CTriggerCheckpoint *) m_trigger)->SetCheckpointNumber(m_index);
        break;
    case MOMZONETYPE_STOP:
        m_trigger = (CTriggerTimerStop *) CreateEntityByName("trigger_momentum_timer_stop");
        m_trigger->SetName(MAKE_STRING("Ending Trigger"));
        break;
    case MOMZONETYPE_ONEHOP:
        m_trigger = (CTriggerOnehop *) CreateEntityByName("trigger_momentum_onehop");
        m_trigger->SetName(MAKE_STRING("Onehop Trigger"));
        m_trigger->m_target = m_linkedent;
        //((CTriggerOnehop *) m_trigger)->SetDestinationIndex(m_destinationIndex);
        //((CTriggerOnehop *) m_trigger)->SetDestinationName(m_linkedtrigger);
        ((CTriggerOnehop *) m_trigger)->SetHoldTeleportTime(m_holdTimeBeforeTeleport);
        ((CTriggerOnehop *) m_trigger)->SetShouldStopPlayer(m_shouldStopOnTeleport);
        ((CTriggerOnehop *) m_trigger)->SetShouldResetAngles(m_shouldResetAngles);
        break;
    case MOMZONETYPE_RESETONEHOP:
        m_trigger = (CTriggerResetOnehop *) CreateEntityByName("trigger_momentum_resetonehop");
        m_trigger->SetName(MAKE_STRING("ResetOnehop Trigger"));
        break;
    case MOMZONETYPE_CPTELE:
        m_trigger = (CTriggerTeleportCheckpoint *) CreateEntityByName("trigger_momentum_teleport_checkpoint");
        m_trigger->SetName(MAKE_STRING("TeleportToCheckpoint Trigger"));
        m_trigger->m_target = m_linkedent;
        //((CTriggerTeleportCheckpoint *)m_trigger)->SetDestinationCheckpointNumber(m_destinationIndex);
        //((CTriggerTeleportCheckpoint *)m_trigger)->SetDestinationCheckpointName(m_linkedtrigger);
        ((CTriggerTeleportCheckpoint *) m_trigger)->SetShouldStopPlayer(m_shouldStopOnTeleport);
        ((CTriggerTeleportCheckpoint *) m_trigger)->SetShouldResetAngles(m_shouldResetAngles);
        break;
    case MOMZONETYPE_MULTIHOP:
        m_trigger = (CTriggerOnehop *) CreateEntityByName("trigger_momentum_onehop");
        m_trigger->SetName(MAKE_STRING("Onehop Trigger"));
        m_trigger->m_target = m_linkedent;
        //((CTriggerMultihop *) m_trigger)->SetDestinationIndex(m_destinationIndex);
        //((CTriggerMultihop *) m_trigger)->SetDestinationName(m_linkedent);
        ((CTriggerMultihop *) m_trigger)->SetHoldTeleportTime(m_holdTimeBeforeTeleport);
        ((CTriggerMultihop *) m_trigger)->SetShouldStopPlayer(m_shouldStopOnTeleport);
        ((CTriggerMultihop *) m_trigger)->SetShouldResetAngles(m_shouldResetAngles);
        break;
    case MOMZONETYPE_STAGE:
        m_trigger = (CTriggerStage *) CreateEntityByName("trigger_momentum_timer_stage");
        m_trigger->SetName(MAKE_STRING("Stage Trigger"));
        ((CTriggerStage *) m_trigger)->SetStageNumber(m_index);
        break;
        //MOM_TODO: add trigger_momentum_teleport, and momentum_trigger_userinput
    default:
        break;
    }

    if (m_trigger)
    {
        m_trigger->Spawn();
        m_trigger->Activate();
        m_trigger->SetAbsOrigin(*m_pos);
        m_trigger->SetSize(*m_scaleMins, *m_scaleMaxs);
        m_trigger->SetAbsAngles(*m_rot);
        m_trigger->SetSolid(SOLID_BBOX);
    }
}

static void saveZonFile(const char* szMapName)
{
    KeyValues* zoneKV = new KeyValues(szMapName);
    CBaseEntity* pEnt = gEntList.FindEntityByClassname(NULL, "trigger_momentum_*");
    while (pEnt)
    {
        KeyValues* subKey = NULL;
        if (pEnt->ClassMatches("trigger_momentum_timer_start"))
        {
            CTriggerTimerStart* pTrigger = dynamic_cast<CTriggerTimerStart*>(pEnt);
            subKey = new KeyValues("start");
            if (pTrigger)
            {
                subKey->SetFloat("leavespeed", pTrigger->GetMaxLeaveSpeed());
                subKey->SetBool("limitingspeed", pTrigger->IsLimitingSpeed());
                subKey->SetBool("onlyxy", pTrigger->IsLimitingSpeedOnlyXY());
                if (pTrigger->GetHasLookAngles())
                    subKey->SetFloat("yaw", pTrigger->GetLookAngles()[YAW] );
            }
        }
        else if (pEnt->ClassMatches("trigger_momentum_timer_stop"))
        {
            subKey = new KeyValues("end");
        }
        else if (pEnt->ClassMatches("trigger_momentum_timer_checkpoint"))
        {
            CTriggerCheckpoint* pTrigger = dynamic_cast<CTriggerCheckpoint*>(pEnt);
            if (pTrigger)
            {
                subKey = new KeyValues("checkpoint");
                subKey->SetInt("number", pTrigger->GetCheckpointNumber());
            }
        }
        else if (pEnt->ClassMatches("trigger_momentum_onehop"))
        {
            CTriggerOnehop* pTrigger = dynamic_cast<CTriggerOnehop*>(pEnt);
            if (pTrigger)
            {
                subKey = new KeyValues("onehop");
                //subKey->SetInt("destination", pTrigger->GetDestinationIndex());
                subKey->SetBool("stop", pTrigger->ShouldStopPlayer());
                subKey->SetBool("resetang", pTrigger->ShouldResetAngles());
                subKey->SetFloat("hold", pTrigger->GetHoldTeleportTime());
                subKey->SetString("destinationname", pTrigger->m_target.ToCStr());
            }
        }
        else if (pEnt->ClassMatches("trigger_momentum_resetonehop"))
        {
            subKey = new KeyValues("resetonehop");
        }
        else if (pEnt->ClassMatches("trigger_momentum_teleport_checkpoint"))
        {

            CTriggerTeleportCheckpoint* pTrigger = dynamic_cast<CTriggerTeleportCheckpoint*>(pEnt);
            if (pTrigger)
            {
                subKey = new KeyValues("checkpoint_teleport");
                //subKey->SetInt("destination", pTrigger->GetDestinationCheckpointNumber());
                subKey->SetBool("stop", pTrigger->ShouldStopPlayer());
                subKey->SetBool("resetang", pTrigger->ShouldResetAngles());
                subKey->SetString("destinationname", pTrigger->m_target.ToCStr());
            }
        }
        else if (pEnt->ClassMatches("trigger_momentum_multihop"))
        {
            CTriggerMultihop* pTrigger = dynamic_cast<CTriggerMultihop*>(pEnt);
            if (pTrigger)
            {
                subKey = new KeyValues("multihop");
                //subKey->SetInt("destination", pTrigger->GetDestinationIndex());
                subKey->SetBool("stop", pTrigger->ShouldStopPlayer());
                subKey->SetFloat("hold", pTrigger->GetHoldTeleportTime());
                subKey->SetBool("resetang", pTrigger->ShouldResetAngles());
                subKey->SetString("destinationname", pTrigger->m_target.ToCStr());
            }
        }
        else if (pEnt->ClassMatches("trigger_momentum_timer_stage"))
        {
            CTriggerStage *pTrigger = dynamic_cast<CTriggerStage*>(pEnt);
            if (pTrigger)
            {
                subKey = new KeyValues("stage");
                subKey->SetInt("number", pTrigger->GetStageNumber());
            }
        }
        if (subKey)
        {
            subKey->SetFloat("xPos", pEnt->GetAbsOrigin().x);
            subKey->SetFloat("yPos", pEnt->GetAbsOrigin().y);
            subKey->SetFloat("zPos", pEnt->GetAbsOrigin().z);
            subKey->SetFloat("xRot", pEnt->GetAbsAngles().x);
            subKey->SetFloat("yRot", pEnt->GetAbsAngles().y);
            subKey->SetFloat("zRot", pEnt->GetAbsAngles().z);
            subKey->SetFloat("xScaleMins", pEnt->WorldAlignMins().x);
            subKey->SetFloat("yScaleMins", pEnt->WorldAlignMins().y);
            subKey->SetFloat("zScaleMins", pEnt->WorldAlignMins().z);
            subKey->SetFloat("xScaleMaxs", pEnt->WorldAlignMaxs().x);
            subKey->SetFloat("yScaleMaxs", pEnt->WorldAlignMaxs().y);
            subKey->SetFloat("zScaleMaxs", pEnt->WorldAlignMaxs().z);
            zoneKV->AddSubKey(subKey);
        }
        pEnt = gEntList.FindEntityByClassname(pEnt, "trigger_momentum_*");
    }
    if (zoneKV->GetFirstSubKey())//not empty 
    {
        char zoneFilePath[MAX_PATH];
        Q_strcpy(zoneFilePath, "maps/");
        Q_strcat(zoneFilePath, szMapName, MAX_PATH);
        Q_strncat(zoneFilePath, ".zon", MAX_PATH);
        zoneKV->SaveToFile(filesystem, zoneFilePath, "MOD");
        zoneKV->deleteThis();
    }
}

CMapzoneData::CMapzoneData(const char *szMapName)
{
    if (!LoadFromFile(szMapName))
    {
        Log("Unable to find map zones! Trying to create them...\n");
        saveZonFile(szMapName);//try making the zon file if the map has the entities
        LoadFromFile(szMapName);
    }
}

//MOM_TODO: Get rid of the following method and ConCommand
static void saveZonFile_f()
{
    saveZonFile(gpGlobals->mapname.ToCStr());
}

static ConCommand mom_generate_zone_file("mom_generate_zone_file", saveZonFile_f, "Generates a zone file.");

CMapzoneData::~CMapzoneData()
{
    if (!m_zones.IsEmpty())
    {
        m_zones.PurgeAndDeleteElements();
    }
}

bool CMapzoneData::MapZoneSpawned(CMapzone *mZone)
{
    bool toReturn = false;
    if (!mZone) return false;

    char name[128];
    if ( !ZoneTypeToClass( mZone->GetType(), name ) ) return false;


    CBaseEntity *pEnt = gEntList.FindEntityByClassname(NULL, name);
    while (pEnt)
    {
        if (pEnt->GetAbsOrigin() == *mZone->GetPosition()
            && pEnt->GetAbsAngles() == *mZone->GetRotation()
            && pEnt->WorldAlignMaxs() == *mZone->GetScaleMaxs()
            && pEnt->WorldAlignMins() == *mZone->GetScaleMins())
        {
            DevLog("Already found a %s spawned on the map! Not spawning it from zone file...\n", name);
            toReturn = true;
            break;
        }

        pEnt = gEntList.FindEntityByClassname(pEnt, name);
    }

    return toReturn;
}

void CMapzoneData::SpawnMapZones()
{
    int count = m_zones.Count();
    for (int i = 0; i < count; i++)
    {
        if (m_zones[i])
        {
            //if the zone already exists (placed in map by Hammer), don't spawn it
            if (!MapZoneSpawned(m_zones[i]))
                m_zones[i]->SpawnZone();
        }
    }
}

bool CMapzoneData::LoadFromFile(const char *szMapName)
{
    bool toReturn = false;
    char zoneFilePath[MAX_PATH];
    Q_strcpy(zoneFilePath, c_mapPath);
    Q_strcat(zoneFilePath, szMapName, MAX_PATH);
    Q_strncat(zoneFilePath, c_zoneFileEnding, MAX_PATH);
    DevLog("Looking for zone file: %s \n", zoneFilePath);
    KeyValues* zoneKV = new KeyValues(szMapName);
    if (zoneKV->LoadFromFile(filesystem, zoneFilePath, "MOD"))
    {
        // Go through checkpoints
        for (KeyValues *cp = zoneKV->GetFirstSubKey(); cp; cp = cp->GetNextKey())
        {
            // Load position information (will default to 0 if the keys don't exist)
            Vector* pos = new Vector(cp->GetFloat("xPos"), cp->GetFloat("yPos"), cp->GetFloat("zPos"));
            QAngle* rot = new QAngle(cp->GetFloat("xRot"), cp->GetFloat("yRot"), cp->GetFloat("zRot"));
            Vector* scaleMins = new Vector(cp->GetFloat("xScaleMins"), cp->GetFloat("yScaleMins"), cp->GetFloat("zScaleMins"));
            Vector* scaleMaxs = new Vector(cp->GetFloat("xScaleMaxs"), cp->GetFloat("yScaleMaxs"), cp->GetFloat("zScaleMaxs"));

            // Do specific things for different types of checkpoints
            // 0 = start, 1 = checkpoint, 2 = end, 3 = Onehop, 4 = OnehopReset, 5 = Checkpoint_teleport, 6 = Multihop, 7 = stage
            int zoneType = -1;
            int index = -1;
            bool shouldStop = false;
            bool shouldTilt = true;
            float holdTime = 1.0f;
            //int destinationIndex = -1;
            bool limitingspeed = true;
            bool checkonlyxy = true;
            float maxleavespeed = 290.0f;
            const char * linkedtrigger = NULL;

            float start_yaw = NO_LOOK;

            if (Q_strcmp(cp->GetName(), "start") == 0)
            {
                zoneType = MOMZONETYPE_START;
                limitingspeed = cp->GetBool("limitingspeed");
                maxleavespeed = cp->GetFloat("leavespeed");
                start_yaw = cp->GetFloat("yaw", NO_LOOK);
                checkonlyxy = cp->GetBool("onlyxy", true);
            }
            else if (Q_strcmp(cp->GetName(), "checkpoint") == 0)
            {
                zoneType = MOMZONETYPE_CP;
                index = cp->GetInt("number", -1);
            }
            else if (Q_strcmp(cp->GetName(), "end") == 0)
            {
                zoneType = MOMZONETYPE_STOP;
            }
            else if (Q_strcmp(cp->GetName(), "onehop") == 0)
            {
                zoneType = MOMZONETYPE_ONEHOP;
                shouldStop = cp->GetBool("stop", false);
                shouldTilt = cp->GetBool("resetang", true);
                holdTime = cp->GetFloat("hold", 1);
                //destinationIndex = cp->GetInt("destination", 1);
                linkedtrigger = cp->GetString("destinationname", NULL);
            }
            else if (Q_strcmp(cp->GetName(), "resetonehop") == 0)
            {
                zoneType = MOMZONETYPE_RESETONEHOP;
            }
            else if (Q_strcmp(cp->GetName(), "checkpoint_teleport") == 0)
            {
                zoneType = MOMZONETYPE_CPTELE;
                //destinationIndex = cp->GetInt("destination", -1);
                shouldStop = cp->GetBool("stop", false);
                shouldTilt = cp->GetBool("resetang", true);
                linkedtrigger = cp->GetString("destinationname", NULL);
            }
            else if (Q_strcmp(cp->GetName(), "multihop") == 0)
            {
                zoneType = MOMZONETYPE_MULTIHOP;
                shouldStop = cp->GetBool("stop", false);
                shouldTilt = cp->GetBool("resetang", true);
                holdTime = cp->GetFloat("hold", 1);
                //destinationIndex = cp->GetInt("destination", 1);
                linkedtrigger = cp->GetString("destinationname", NULL);
            }
            else if (Q_strcmp(cp->GetName(), "stage") == 0)
            {
                zoneType = MOMZONETYPE_STAGE;
                index = cp->GetInt("number", 0);
            }
            else
            {
                Warning("Error while reading zone file: Unknown mapzone type %s!\n", cp->GetName());
                continue;
            }

            // Add element
            m_zones.AddToTail(new CMapzone(zoneType, pos, rot, scaleMins, scaleMaxs, index, shouldStop, shouldTilt,
                holdTime, limitingspeed, maxleavespeed, start_yaw, MAKE_STRING(linkedtrigger),checkonlyxy));
        }
        DevLog("Successfully loaded map zone file %s!\n", zoneFilePath);
        toReturn = true;
    }
    zoneKV->deleteThis();
    return toReturn;
}

bool ZoneTypeToClass( int type, char *dest )
{
    switch (type)
    {
    case MOMZONETYPE_START:
        Q_strcpy(dest, "trigger_momentum_timer_start");
        return true;
    case MOMZONETYPE_CP:
        Q_strcpy(dest, "trigger_momentum_timer_checkpoint");
        return true;
    case MOMZONETYPE_STOP:
        Q_strcpy(dest, "trigger_momentum_timer_stop");
        return true;
    case MOMZONETYPE_ONEHOP:
        Q_strcpy(dest, "trigger_momentum_onehop");
        return true;
    case MOMZONETYPE_RESETONEHOP:
        Q_strcpy(dest, "trigger_momentum_timer_resetonehop");
        return true;
    case MOMZONETYPE_CPTELE:
        Q_strcpy(dest, "trigger_momentum_teleport_checkpoint");
        return true;
    case MOMZONETYPE_MULTIHOP:
        Q_strcpy(dest, "trigger_momentum_multihop");
        return true;
    case MOMZONETYPE_STAGE:
        Q_strcpy(dest, "trigger_momentum_timer_stage");
        return true;
    }

    return false;
}