//
// Engineer Seams — Header-Only Interfaces (Task T2.1)
//
// Purpose:
// - Provide lightweight, header-only seam interfaces for future guarded
//   Engineer behavior (placement, hint bias, teleporter, maintenance).
// - No includes, no implementations elsewhere, no wiring in this phase.
//
// Toggle Policy:
// - Default: OFF. This header provides only no-op stubs in this phase.
// - Enablement (local/testing only):
//   - Example 1 (preferred): pass a build flag
//       -DTF_BOT_ENGINEER_SEAMS=1
//   - Example 2 (temporary): locally change the macro below to 1
//       #define TF_BOT_ENGINEER_SEAMS 1
//     (Not recommended for commits; use -D for reproducibility.)
// - Note: No wiring yet. Calls will be gated in Task T2.3.
// - Future work: runtime CVars for tuning will arrive in later phases.
//
// Design:
// - Forward declarations only; inline stubs return conservative baseline values
//   (0.0f, 0, false) to ensure zero behavior change if accidentally included.
// - Follows Valve style of minimalism and zero runtime cost.

#ifndef TF_BOT_ENGINEER_SEAMS_H
#define TF_BOT_ENGINEER_SEAMS_H

#ifndef TF_BOT_ENGINEER_SEAMS
#define TF_BOT_ENGINEER_SEAMS 1
#endif

class CTFPlayer;
class CBaseEntity;
class CBaseObject;
class Vector;

// Placement heuristics
inline float Seam_DesiredPlacementRange( const CTFPlayer *eng, const CBaseEntity *objective )
{
	(void)eng; (void)objective;
	return 0.0f;
}

inline float Seam_HeightBias( const CTFPlayer *eng, const Vector &pos )
{
	(void)eng; (void)pos;
	return 0.0f;
}

inline float Seam_NestSpacingMin( const CTFPlayer *eng )
{
	(void)eng;
	return 0.0f;
}

// Hint awareness
inline float Seam_HintBias( const CTFPlayer *eng, const CBaseEntity *hint )
{
	(void)eng; (void)hint;
	return 0.0f;
}

// Teleporter validation
inline bool Seam_TeleporterIsValid( const CBaseObject *entrance, const CBaseObject *exit )
{
	(void)entrance; (void)exit;
	return false;
}

inline bool Seam_TeleporterShouldRedeploy( const CBaseObject *entrance, const CBaseObject *exit, float travelDelta )
{
	(void)entrance; (void)exit; (void)travelDelta;
	return false;
}

// Maintenance policy
inline bool Seam_ShouldRepairFirst( const CBaseObject *obj, int engMetal )
{
	(void)obj; (void)engMetal;
	return false;
}

inline int Seam_TargetUpgradeLevel( const CBaseObject *obj )
{
	(void)obj;
	return 0;
}

inline int Seam_MetalReserve( const CTFPlayer *eng )
{
	(void)eng;
	return 0;
}

#endif // TF_BOT_ENGINEER_SEAMS_H
