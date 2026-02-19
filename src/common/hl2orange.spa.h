////////////////////////////////////////////////////////////////////
//
// hl2orange.spa.h
//
// Auto-generated on Thursday, 13 September 2007 at 16:59:17
// XLAST project version 1.0.402.0
// SPA Compiler version 2.0.6274.0
//
////////////////////////////////////////////////////////////////////

#ifndef __THE_ORANGE_BOX_SPA_H__
#define __THE_ORANGE_BOX_SPA_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// Title info
//

#define TITLEID_THE_ORANGE_BOX                      0x4541080F

//
// Context ids
//
// These values are passed as the dwContextId to XUserSetContext.
//

#define CONTEXT_CHAPTER_HL2                         0
#define CONTEXT_SCENARIO                            1
#define CONTEXT_GAME                                2
#define CONTEXT_CHAPTER_EP1                         3
#define CONTEXT_CHAPTER_EP2                         4
#define CONTEXT_CHAPTER_PORTAL                      5

//
// Context values
//
// These values are passed as the dwContextValue to XUserSetContext.
//

// Values for CONTEXT_CHAPTER_HL2

#define CONTEXT_CHAPTER_HL2_POINT_INSERTION         0
#define CONTEXT_CHAPTER_HL2_A_RED_LETTER_DAY        1
#define CONTEXT_CHAPTER_HL2_ROUTE_KANAL             2
#define CONTEXT_CHAPTER_HL2_WATER_HAZARD            3
#define CONTEXT_CHAPTER_HL2_BLACK_MESA_EAST         4
#define CONTEXT_CHAPTER_HL2_RAVENHOLM               5
#define CONTEXT_CHAPTER_HL2_HIGHWAY_17              6
#define CONTEXT_CHAPTER_HL2_SANDTRAPS               7
#define CONTEXT_CHAPTER_HL2_NOVA_PROSPEKT           8
#define CONTEXT_CHAPTER_HL2_ENTANGLEMENT            9
#define CONTEXT_CHAPTER_HL2_ANTICITIZEN_ONE         10
#define CONTEXT_CHAPTER_HL2_FOLLOW_FREEMAN          11
#define CONTEXT_CHAPTER_HL2_OUR_BENEFACTORS         12
#define CONTEXT_CHAPTER_HL2_DARK_ENERGY             13

// Values for CONTEXT_SCENARIO

#define CONTEXT_SCENARIO_CTF_2FORT                  0
#define CONTEXT_SCENARIO_CP_DUSTBOWL                1
#define CONTEXT_SCENARIO_CP_GRANARY                 2
#define CONTEXT_SCENARIO_CP_WELL                    3
#define CONTEXT_SCENARIO_CP_GRAVELPIT               4
#define CONTEXT_SCENARIO_TC_HYDRO                   5
#define CONTEXT_SCENARIO_CTF_CLOAK                  6
#define CONTEXT_SCENARIO_CP_CLOAK                   7

// Values for CONTEXT_GAME

#define CONTEXT_GAME_GAME_HALF_LIFE_2               0
#define CONTEXT_GAME_GAME_EPISODE_ONE               1
#define CONTEXT_GAME_GAME_EPISODE_TWO               2
#define CONTEXT_GAME_GAME_PORTAL                    3
#define CONTEXT_GAME_GAME_TEAM_FORTRESS             4

// Values for CONTEXT_CHAPTER_EP1

#define CONTEXT_CHAPTER_EP1_UNDUE_ALARM             0
#define CONTEXT_CHAPTER_EP1_DIRECT_INTERVENTION     1
#define CONTEXT_CHAPTER_EP1_LOWLIFE                 2
#define CONTEXT_CHAPTER_EP1_URBAN_FLIGHT            3
#define CONTEXT_CHAPTER_EP1_EXIT_17                 4

// Values for CONTEXT_CHAPTER_EP2

#define CONTEXT_CHAPTER_EP2_TO_THE_WHITE_FOREST     0
#define CONTEXT_CHAPTER_EP2_THIS_VORTAL_COIL        1
#define CONTEXT_CHAPTER_EP2_FREEMAN_PONTIFEX        2
#define CONTEXT_CHAPTER_EP2_RIDING_SHOTGUN          3
#define CONTEXT_CHAPTER_EP2_UNDER_THE_RADAR         4
#define CONTEXT_CHAPTER_EP2_OUR_MUTUAL_FIEND        5
#define CONTEXT_CHAPTER_EP2_T_MINUS_ONE             6

// Values for CONTEXT_CHAPTER_PORTAL

#define CONTEXT_CHAPTER_PORTAL_TESTCHAMBER_00       0
#define CONTEXT_CHAPTER_PORTAL_TESTCHAMBER_04       1
#define CONTEXT_CHAPTER_PORTAL_TESTCHAMBER_08       2
#define CONTEXT_CHAPTER_PORTAL_TESTCHAMBER_10       3
#define CONTEXT_CHAPTER_PORTAL_TESTCHAMBER_13       4
#define CONTEXT_CHAPTER_PORTAL_TESTCHAMBER_14       5
#define CONTEXT_CHAPTER_PORTAL_TESTCHAMBER_15       6
#define CONTEXT_CHAPTER_PORTAL_TESTCHAMBER_16       7
#define CONTEXT_CHAPTER_PORTAL_TESTCHAMBER_17       8
#define CONTEXT_CHAPTER_PORTAL_TESTCHAMBER_18       9
#define CONTEXT_CHAPTER_PORTAL_TESTCHAMBER_19       10

// Values for X_CONTEXT_PRESENCE

#define CONTEXT_PRESENCE_TF_CP                      0
#define CONTEXT_PRESENCE_TF_CTF_LOSING              1
#define CONTEXT_PRESENCE_TF_CTF_TIED                2
#define CONTEXT_PRESENCE_TF_CTF_WINNING             3
#define CONTEXT_PRESENCE_APPCHOOSER                 4
#define CONTEXT_PRESENCE_MENU                       5
#define CONTEXT_PRESENCE_EP1_INGAME                 6
#define CONTEXT_PRESENCE_HL2_INGAME                 7
#define CONTEXT_PRESENCE_EP2_INGAME                 8
#define CONTEXT_PRESENCE_PORTAL_INGAME              9
#define CONTEXT_PRESENCE_COMMENTARY                 10
#define CONTEXT_PRESENCE_IDLE                       11

// Values for X_CONTEXT_GAME_MODE

#define CONTEXT_GAME_MODE_MULTIPLAYER               0
#define CONTEXT_GAME_MODE_SINGLEPLAYER              1

//
// Property ids
//
// These values are passed as the dwPropertyId value to XUserSetProperty
// and as the dwPropertyId value in the XUSER_PROPERTY structure.
//

#define PROPERTY_CAPS_OWNED                         0x10000000
#define PROPERTY_CAPS_TOTAL                         0x10000001
#define PROPERTY_PLAYER_TEAM_SCORE                  0x10000002
#define PROPERTY_OPPONENT_TEAM_SCORE                0x10000003
#define PROPERTY_FLAG_CAPTURE_LIMIT                 0x1000000B
#define PROPERTY_NUMBER_OF_ROUNDS                   0x1000000C
#define PROPERTY_GAME_SIZE                          0x1000000D
#define PROPERTY_AUTOBALANCE                        0x1000000E
#define PROPERTY_PRIVATE_SLOTS                      0x1000000F
#define PROPERTY_MAX_GAME_TIME                      0x10000010
#define PROPERTY_NUMBER_OF_KILLS                    0x10000011
#define PROPERTY_DAMAGE_DEALT                       0x10000012
#define PROPERTY_PLAY_TIME                          0x10000013
#define PROPERTY_POINT_CAPTURES                     0x10000014
#define PROPERTY_POINT_DEFENSES                     0x10000015
#define PROPERTY_DOMINATIONS                        0x10000016
#define PROPERTY_REVENGE                            0x10000017
#define PROPERTY_BUILDINGS_DESTROYED                0x10000019
#define PROPERTY_HEADSHOTS                          0x1000001A
#define PROPERTY_HEALTH_POINTS_HEALED               0x1000001B
#define PROPERTY_INVULNS                            0x1000001C
#define PROPERTY_KILL_ASSISTS                       0x1000001D
#define PROPERTY_BACKSTABS                          0x1000001E
#define PROPERTY_HEALTH_POINTS_LEACHED              0x1000001F
#define PROPERTY_BUILDINGS_BUILT                    0x10000020
#define PROPERTY_SENTRY_KILLS                       0x10000021
#define PROPERTY_TELEPORTS                          0x10000022
#define PROPERTY_KILLS                              0x10000023
#define PROPERTY_NUMBER_OF_TEAMS                    0x10000025
#define PROPERTY_TEAM_RED                           0x10000026
#define PROPERTY_TEAM_BLUE                          0x10000027
#define PROPERTY_TEAM_SPECTATOR                     0x10000028
#define PROPERTY_TEAM                               0x10000029
#define PROPERTY_WIN_LIMIT                          0x1000002A
#define PROPERTY_RANKING_TEST                       0x2000000A
#define PROPERTY_POINTS_SCORED                      0x20000018

//
// Stats view ids
//
// These are used in the dwViewId member of the XUSER_STATS_SPEC structure
// passed to the XUserReadStats* and XUserCreateStatsEnumerator* functions.
//

// Skill leaderboards for ranked game modes

#define STATS_VIEW_SKILL_RANKED_MULTIPLAYER         0xFFFF0000
#define STATS_VIEW_SKILL_RANKED_SINGLEPLAYER        0xFFFF0001

// Skill leaderboards for unranked (standard) game modes

#define STATS_VIEW_SKILL_STANDARD_MULTIPLAYER       0xFFFE0000
#define STATS_VIEW_SKILL_STANDARD_SINGLEPLAYER      0xFFFE0001

// Title defined leaderboards

#define STATS_VIEW_PLAYER_MAX_UNRANKED              1
#define STATS_VIEW_PLAYER_MAX_RANKED                2

//
// Stats view column ids
//
// These ids are used to read columns of stats views.  They are specified in
// the rgwColumnIds array of the XUSER_STATS_SPEC structure.  Rank, rating
// and gamertag are not retrieved as custom columns and so are not included
// in the following definitions.  They can be retrieved from each row's
// header (e.g., pStatsResults->pViews[x].pRows[y].dwRank, etc.).
//

// Column ids for PLAYER_MAX_UNRANKED

#define STATS_COLUMN_PLAYER_MAX_UNRANKED_POINTS_SCORED 2
#define STATS_COLUMN_PLAYER_MAX_UNRANKED_KILLS      3
#define STATS_COLUMN_PLAYER_MAX_UNRANKED_POINTS_CAPPED 1
#define STATS_COLUMN_PLAYER_MAX_UNRANKED_DAMAGE_DEALT 4
#define STATS_COLUMN_PLAYER_MAX_UNRANKED_PLAY_TIME  5
#define STATS_COLUMN_PLAYER_MAX_UNRANKED_POINT_DEFENSES 6
#define STATS_COLUMN_PLAYER_MAX_UNRANKED_DOMINATIONS 7
#define STATS_COLUMN_PLAYER_MAX_UNRANKED_REVENGE    8
#define STATS_COLUMN_PLAYER_MAX_UNRANKED_BUILDINGS_DESTROYED 9
#define STATS_COLUMN_PLAYER_MAX_UNRANKED_HEADSHOTS  10
#define STATS_COLUMN_PLAYER_MAX_UNRANKED_HEALTH_POINTS_HEALED 11
#define STATS_COLUMN_PLAYER_MAX_UNRANKED_INVULNS    12
#define STATS_COLUMN_PLAYER_MAX_UNRANKED_KILL_ASSISTS 13
#define STATS_COLUMN_PLAYER_MAX_UNRANKED_BACKSTABS  14
#define STATS_COLUMN_PLAYER_MAX_UNRANKED_HEALTH_POINTS_LEACHED 15
#define STATS_COLUMN_PLAYER_MAX_UNRANKED_BUILDINGS_BUILT 16
#define STATS_COLUMN_PLAYER_MAX_UNRANKED_SENTRY_KILLS 17
#define STATS_COLUMN_PLAYER_MAX_UNRANKED_TELEPORTS  18

// Column ids for PLAYER_MAX_RANKED

#define STATS_COLUMN_PLAYER_MAX_RANKED_POINTS_SCORED 2

//
// Matchmaking queries
//
// These values are passed as the dwProcedureIndex parameter to
// XSessionSearch to indicate which matchmaking query to run.
//

#define SESSION_MATCH_QUERY_PLAYER_MATCH            0

//
// Gamer pictures
//
// These ids are passed as the dwPictureId parameter to XUserAwardGamerTile.
//



#ifdef __cplusplus
}
#endif

#endif // __THE_ORANGE_BOX_SPA_H__


