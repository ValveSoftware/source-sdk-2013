#include "cbase.h"
#include "mapzones_edit.h"

#include "tier0/memdbgon.h"


ConVar mom_zone_edit("mom_zone_edit", "0", FCVAR_CHEAT, "Toggle zone editing.\n", true, 0, true, 1);
static ConVar mom_zone_ignorewarning("mom_zone_ignorewarning", "0", FCVAR_CHEAT, "Lets you create zones despite map already having start and end.\n", true, 0, true, 1);
static ConVar mom_zone_grid("mom_zone_grid", "8", FCVAR_CHEAT, "Set grid size. 0 to disable.", true, 0, false, 0);
static ConVar mom_zone_defzone("mom_zone_defzone", "start", FCVAR_CHEAT, "If no zone type is passed to mom_zone_mark, use this.\n");
static ConVar mom_zone_start_limitspdmethod("mom_zone_start_limitspdmethod", "1", FCVAR_CHEAT, "0 = Take into account player z-velocity, 1 = Ignore z-velocity.\n", true, 0, true, 1);
static ConVar mom_zone_stage_num("mom_zone_stage_num", "0", FCVAR_CHEAT, "Set stage number. Should start from 2. 0 to automatically find one.\n", true, 0, false, 0);
static ConVar mom_zone_start_maxleavespeed("mom_zone_start_maxleavespeed", "290", FCVAR_CHEAT, "Max leave speed. 0 to disable.\n", true, 0, false, 0);
//static ConVar mom_zone_cp_num( "mom_zone_cp_num", "0", FCVAR_CHEAT, "Checkpoint number. 0 to automatically find one." );


void CC_Mom_ZoneZoomIn()
{
    g_MapzoneEdit.DecreaseZoom((float) mom_zone_grid.GetInt());
}

static ConCommand mom_zone_zoomin("mom_zone_zoomin", CC_Mom_ZoneZoomIn, "Decrease reticle maximum distance.\n", FCVAR_CHEAT);


void CC_Mom_ZoneZoomOut()
{
    g_MapzoneEdit.IncreaseZoom((float) mom_zone_grid.GetInt());
}

static ConCommand mom_zone_zoomout("mom_zone_zoomout", CC_Mom_ZoneZoomOut, "Increase reticle maximum distance.\n", FCVAR_CHEAT);


void CC_Mom_ZoneDelete(const CCommand &args)
{
    if (!mom_zone_edit.GetBool()) return;


    if (args.ArgC() > 1)
    {
        DevMsg("Attempting to delete '%s'\n", args[1]);

        int entindex = atoi(args[1]);

        if (entindex != 0)
        {
            CBaseEntity *pEnt = CBaseEntity::Instance(INDEXENT(entindex));

            if (pEnt && g_MapzoneEdit.GetEntityZoneType(pEnt) != -1)
            {
                UTIL_Remove(pEnt);
            }
        }
        else
        {
            char szDelete[64];
            if (ZoneTypeToClass(g_MapzoneEdit.ShortNameToZoneType(args[1]), szDelete))
            {
                CBaseEntity *pEnt = gEntList.FindEntityByClassname(NULL, szDelete);
                while (pEnt)
                {
                    UTIL_Remove(pEnt);
                    pEnt = gEntList.FindEntityByClassname(pEnt, szDelete);
                }
            }
        }
    }
}

static ConCommand mom_zone_delete("mom_zone_delete", CC_Mom_ZoneDelete, "Delete zone types. Accepts start/stop/stage or an entity index.\n", FCVAR_CHEAT);


void CC_Mom_ZoneSetLook(const CCommand &args)
{
    if (!mom_zone_edit.GetBool()) return;

    CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
    if (!pPlayer) return;


    float yaw;

    if (args.ArgC() > 1)
    {
        yaw = atof(args[1]);
    }
    else
    {
        yaw = pPlayer->EyeAngles()[1];
    }

    CBaseEntity *pEnt = gEntList.FindEntityByClassname(NULL, "trigger_momentum_timer_start");
    CTriggerTimerStart *pStart;

    while (pEnt)
    {
        pStart = static_cast<CTriggerTimerStart *>(pEnt);

        if (pStart)
        {
            pStart->SetHasLookAngles(true);
            pStart->SetLookAngles(QAngle(0, yaw, 0));

            DevMsg("Set start zone angles to: %.1f, %.1f, %.1f\n", pStart->GetLookAngles()[0], pStart->GetLookAngles()[1], pStart->GetLookAngles()[2]);
        }

        pEnt = gEntList.FindEntityByClassname(pEnt, "trigger_momentum_timer_start");
    }
}

static ConCommand mom_zone_start_setlook("mom_zone_start_setlook", CC_Mom_ZoneSetLook, "Sets start zone teleport look angles. Will take yaw in degrees or use your angles if no arguments given.\n", FCVAR_CHEAT);


void CC_Mom_ZoneMark(const CCommand &args)
{
    if (!mom_zone_edit.GetBool()) return;

    CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

    if (!pPlayer) return;


    int zonetype = -1;

    if (g_MapzoneEdit.GetBuildStage() >= BUILDSTAGE_END)
    {
        if (args.ArgC() > 1)
        {
            zonetype = g_MapzoneEdit.ShortNameToZoneType(args[1]);
        }
        else
        {
            zonetype = g_MapzoneEdit.ShortNameToZoneType(mom_zone_defzone.GetString());
        }

        if (zonetype == MOMZONETYPE_START || zonetype == MOMZONETYPE_STOP)
        {
            // Count zones to make sure we don't create multiple instances.
            int startnum = 0;
            int endnum = 0;

            CBaseEntity *pEnt;

            pEnt = gEntList.FindEntityByClassname(NULL, "trigger_momentum_timer_start");
            while (pEnt)
            {
                startnum++;
                pEnt = gEntList.FindEntityByClassname(pEnt, "trigger_momentum_timer_start");
            }

            pEnt = gEntList.FindEntityByClassname(NULL, "trigger_momentum_timer_stop");
            while (pEnt)
            {
                endnum++;
                pEnt = gEntList.FindEntityByClassname(pEnt, "trigger_momentum_timer_stop");
            }

            DevMsg("Found %i starts and %i ends (previous)\n", startnum, endnum);

            if (!mom_zone_ignorewarning.GetBool() && startnum && endnum)
            {
                g_MapzoneEdit.SetBuildStage(BUILDSTAGE_NONE);

                ConMsg("Map already has a start and an end! Use mom_zone_defzone to set another type.\n");

                return;
            }

            //The user is trying to make multiple starts?
            if (zonetype == MOMZONETYPE_START)
            {
                 // Switch between start and end.
                 zonetype = (startnum <= endnum) ? MOMZONETYPE_START : MOMZONETYPE_STOP;
            }
            //else the zonetype can be STOP, allowing for multiple stop triggers to be created
        }
    }

    trace_t tr;
    Vector vecFwd;

    AngleVectors(pPlayer->EyeAngles(), &vecFwd);

    UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + vecFwd * g_MapzoneEdit.GetZoom(), MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &tr);


    g_MapzoneEdit.Build(&tr.endpos, zonetype);
}

static ConCommand mom_zone_mark("mom_zone_mark", CC_Mom_ZoneMark, "Starts building a zone.\n", FCVAR_CHEAT);


void CC_Mom_ZoneCancel()
{
    if (!mom_zone_edit.GetBool()) return;

    CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

    if (!pPlayer) return;


    g_MapzoneEdit.SetBuildStage(BUILDSTAGE_NONE);
}

static ConCommand mom_zone_cancel("mom_zone_cancel", CC_Mom_ZoneCancel, "Cancel the zone building.\n", FCVAR_CHEAT);


void CMapzoneEdit::Build(Vector *aimpos, int type, int forcestage)
{
    if (mom_zone_grid.GetInt() > 0)
        VectorSnapToGrid(aimpos, (float) mom_zone_grid.GetInt());


    switch ((forcestage != BUILDSTAGE_NONE) ? forcestage : ++m_nBuildStage)
    {
    case BUILDSTAGE_START:
        m_vecBuildStart = *aimpos;
        break;

    case BUILDSTAGE_END:
        m_vecBuildEnd = *aimpos;
        break;

    case BUILDSTAGE_HEIGHT:
    {
        char szClass[64];
        if (ZoneTypeToClass(type, szClass))
        {
            CBaseEntity *pEnt = CreateEntityByName(szClass);
            Vector vecOrigin, vecSize, vecMinsRel;
            int i;

            VectorMin(m_vecBuildStart, m_vecBuildEnd, vecMinsRel);
            VectorMax(m_vecBuildStart, m_vecBuildEnd, vecSize);

            for (i = 0; i < 3; i++)
                vecSize[i] = (vecSize[i] - vecMinsRel[i]) / 2.0f;

            for (i = 0; i < 3; i++)
                vecOrigin[i] = vecMinsRel[i] + vecSize[i];

            pEnt->Spawn();

            pEnt->SetAbsOrigin(vecOrigin);
            pEnt->SetSize(Vector(-vecSize.x, -vecSize.y, -vecSize.z), vecSize);
            pEnt->SetEffects(EF_NODRAW);
            pEnt->SetSolid(SOLID_BBOX);

            pEnt->Activate();

            SetZoneProps(pEnt);
        }
    }
    default:
        m_nBuildStage = BUILDSTAGE_NONE;
    }
}

void CMapzoneEdit::SetZoneProps(CBaseEntity *pEnt)
{
    CTriggerTimerStart *pStart = dynamic_cast<CTriggerTimerStart *>(pEnt);
    if (pStart)
    {
        if (mom_zone_start_maxleavespeed.GetFloat() > 0.0)
        {
            pStart->SetMaxLeaveSpeed(mom_zone_start_maxleavespeed.GetFloat());
            pStart->SetIsLimitingSpeed(true);
        }
        else
        {
            pStart->SetIsLimitingSpeed(false);
        }

        pStart->SetIsLimitingSpeedOnlyXY(mom_zone_start_limitspdmethod.GetBool());

        return;
    }

    CTriggerStage *pStage = dynamic_cast<CTriggerStage *>(pEnt);
    if (pStage)
    {
        if (mom_zone_stage_num.GetInt() > 0)
        {
            pStage->SetStageNumber(mom_zone_stage_num.GetInt());
        }
        else
        {
            int higheststage = 1;
            CTriggerStage *pTempStage;

            CBaseEntity *pTemp = gEntList.FindEntityByClassname(NULL, "trigger_momentum_timer_stage");
            while (pTemp)
            {
                pTempStage = static_cast<CTriggerStage *>(pTemp);

                if (pTempStage && pTempStage->GetStageNumber() > higheststage)
                {
                    higheststage = pTempStage->GetStageNumber();
                }

                pTemp = gEntList.FindEntityByClassname(pTemp, "trigger_momentum_timer_stage");
            }

            pStage->SetStageNumber(higheststage + 1);
        }

        return;
    }


    /*CTriggerCheckpoint *pCP = dynamic_cast<CTriggerCheckpoint *>( pEnt );
    if ( pCP )
    {
    if ( mom_zone_cpnum.GetInt() > 0 )
    {
    pCP->SetCheckpointNumber( mom_zone_cpnum.GetInt() );
    }
    else
    {
    int highestcp = 0;
    CTriggerCheckpoint *pTempCP;

    CBaseEntity *pTemp = gEntList.FindEntityByClassname( NULL, "trigger_momentum_timer_checkpoint" );
    while ( pTemp )
    {
    pTempCP = dynamic_cast<CTriggerCheckpoint *>( pTemp );

    if ( pTempCP && pTempCP->GetCheckpointNumber() > highestcp )
    {
    highestcp = pTempCP->GetCheckpointNumber();
    }

    pTemp = gEntList.FindEntityByClassname( pTemp, "trigger_momentum_timer_checkpoint" );
    }

    pStage->SetStageNumber( highestcp + 1 );
    }

    return;
    }*/
}

int CMapzoneEdit::GetEntityZoneType(CBaseEntity *pEnt)
{
    CTriggerTimerStart *pStart = dynamic_cast<CTriggerTimerStart *>(pEnt);
    if (pStart) return MOMZONETYPE_START;

    /*CTriggerTeleportCheckpoint *pCP = dynamic_cast<CTriggerTeleportCheckpoint *>( pEnt );
    if ( pCP ) return 1;*/

    CTriggerTimerStop *pStop = dynamic_cast<CTriggerTimerStop *>(pEnt);
    if (pStop) return MOMZONETYPE_STOP;

    CTriggerStage *pStage = dynamic_cast<CTriggerStage *>(pEnt);
    if (pStage) return MOMZONETYPE_STAGE;

    return -1;
}

void CMapzoneEdit::Update()
{
    if (mom_zone_edit.GetBool())
    {
        if (!m_bEditing)
        {
            m_nBuildStage = BUILDSTAGE_NONE;
            m_bEditing = true;
        }
    }
    else
    {
        if (m_bEditing)
        {
            m_nBuildStage = BUILDSTAGE_NONE;
            m_bEditing = false;
        }

        return;
    }

    CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

    if (!pPlayer) return;


    trace_t tr;
    Vector vecFwd;

    AngleVectors(pPlayer->EyeAngles(), &vecFwd);

    UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + vecFwd * m_flReticleDist, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

    Vector vecAim = tr.endpos;

    if (mom_zone_grid.GetInt() > 0)
        VectorSnapToGrid(&vecAim, (float) mom_zone_grid.GetInt());


    if (m_nBuildStage >= BUILDSTAGE_START)
    {
        Vector vecP2, vecP3, vecP4;

        if (m_nBuildStage >= BUILDSTAGE_END)
        {
            vecP3 = m_vecBuildEnd;
        }
        else
        {
            vecP3 = vecAim;
        }

        vecP3[2] = m_vecBuildStart[2];

        // Bottom
        vecP2[0] = m_vecBuildStart[0];
        vecP2[1] = vecP3[1];
        vecP2[2] = m_vecBuildStart[2];

        vecP4[0] = vecP3[0];
        vecP4[1] = m_vecBuildStart[1];
        vecP4[2] = m_vecBuildStart[2];

        DebugDrawLine(m_vecBuildStart, vecP2, 255, 255, 255, true, -1.0f);
        DebugDrawLine(vecP2, vecP3, 255, 255, 255, true, -1.0f);
        DebugDrawLine(vecP3, vecP4, 255, 255, 255, true, -1.0f);
        DebugDrawLine(vecP4, m_vecBuildStart, 255, 255, 255, true, -1.0f);

        if (m_nBuildStage >= BUILDSTAGE_END)
        {
            Vector vecP5, vecP6, vecP8;

            m_vecBuildEnd[2] = SnapToGrid(m_vecBuildStart[2] + GetZoneHeightToPlayer(pPlayer), (float) mom_zone_grid.GetInt());

            // Top
            vecP5 = m_vecBuildStart;
            vecP5.z = m_vecBuildEnd[2];

            vecP6 = vecP2;
            vecP6.z = m_vecBuildEnd[2];

            vecP8 = vecP4;
            vecP8.z = m_vecBuildEnd[2];

            DebugDrawLine(vecP5, vecP6, 255, 255, 255, true, -1.0f);
            DebugDrawLine(vecP6, m_vecBuildEnd, 255, 255, 255, true, -1.0f);
            DebugDrawLine(m_vecBuildEnd, vecP8, 255, 255, 255, true, -1.0f);
            DebugDrawLine(vecP8, vecP5, 255, 255, 255, true, -1.0f);

            // Bottom to top
            DebugDrawLine(m_vecBuildStart, vecP5, 255, 255, 255, true, -1.0f);
            DebugDrawLine(vecP2, vecP6, 255, 255, 255, true, -1.0f);
            DebugDrawLine(vecP3, m_vecBuildEnd, 255, 255, 255, true, -1.0f);
            DebugDrawLine(vecP4, vecP8, 255, 255, 255, true, -1.0f);
        }
    }

    // Draw surface normal. Makes it a bit easier to see where reticle is hitting.
    if (tr.DidHit())
    {
        DebugDrawLine(vecAim, vecAim + tr.plane.normal * 24.0f, 0, 0, 255, true, -1.0f);
    }

    DrawReticle(&vecAim, (mom_zone_grid.GetInt() > 0) ? ((float) mom_zone_grid.GetInt() / 2.0f) : 8.0f);
}

void CMapzoneEdit::VectorSnapToGrid(Vector *dest, float gridsize)
{
    dest->x = SnapToGrid(dest->x, gridsize);
    dest->y = SnapToGrid(dest->y, gridsize);
    dest->z = SnapToGrid(dest->z, gridsize);
}

float CMapzoneEdit::SnapToGrid(float fl, float gridsize)
{
    float closest;
    float dif;

    closest = fl - fmodf(fl, gridsize);

    dif = fl - closest;

    if (dif > (gridsize / 2.0f))
    {
        closest += gridsize;
    }
    else if (dif < (-gridsize / 2.0f))
    {
        closest -= gridsize;
    }

    return closest;
}

float CMapzoneEdit::GetZoneHeightToPlayer(CBasePlayer *pPlayer)
{
    // It's good enough.
    return pPlayer->GetAbsOrigin().DistTo(m_vecBuildStart) * tanf(DEG2RAD(-pPlayer->EyeAngles()[0])) + pPlayer->GetViewOffset()[2];
}

void CMapzoneEdit::DrawReticle(Vector *pos, float retsize)
{
    Vector p1, p2, p3, p4, p5, p6;

    p1.x = pos->x + retsize;
    p1.y = pos->y;
    p1.z = pos->z;

    p2.x = pos->x - retsize;
    p2.y = pos->y;
    p2.z = pos->z;

    p3.x = pos->x;
    p3.y = pos->y + retsize;
    p3.z = pos->z;

    p4.x = pos->x;
    p4.y = pos->y - retsize;
    p4.z = pos->z;

    p5.x = pos->x;
    p5.y = pos->y;
    p5.z = pos->z + retsize;

    p6.x = pos->x;
    p6.y = pos->y;
    p6.z = pos->z - retsize;

    DebugDrawLine(p1, p2, 255, 0, 0, true, -1.0f);
    DebugDrawLine(p3, p4, 255, 0, 0, true, -1.0f);
    DebugDrawLine(p5, p6, 255, 0, 0, true, -1.0f);
}

int CMapzoneEdit::ShortNameToZoneType(const char *in)
{
    if (Q_stricmp(in, "start") == 0)
    {
        return MOMZONETYPE_START;
    }
    else if (Q_stricmp(in, "end") == 0 || Q_stricmp(in, "stop") == 0)
    {
        return MOMZONETYPE_STOP;
    }
    else if (Q_stricmp(in, "stage") == 0)
    {
        return MOMZONETYPE_STAGE;
    }

    return -1;
}

void CMapzoneEdit::Reset()
{
    mom_zone_edit.SetValue(0);

    m_nBuildStage = BUILDSTAGE_NONE;
    m_bEditing = false;
}

CMapzoneEdit g_MapzoneEdit;