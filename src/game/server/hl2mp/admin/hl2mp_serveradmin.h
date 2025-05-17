#ifndef HL2MP_SERVERADMIN_H
#define HL2MP_SERVERADMIN_H

#include "cbase.h"

// admin permission flags
#define ADMIN_UNDEFINED      'a'
#define ADMIN_GENERIC        'b' // minimum flag required for admin detection
#define ADMIN_KICK           'c' // kicking function
#define ADMIN_BAN            'd' // banning function -- DOES NOT INCLUDE ADDBAN, WHICH NEEDS RCON
#define ADMIN_UNBAN          'e' // to remove a ban in banned_user.cfg -- only is useful for permanent bans
#define ADMIN_SLAY           'f' // to slap and slay
#define ADMIN_CHANGEMAP      'g' // to change levels
#define ADMIN_CVAR           'h' // for sa cvar, accessing any cvar, even those behind FCVAR_DEVELOPMENTONLY
#define ADMIN_CONFIG         'i' // for executing cfg files
#define ADMIN_CHAT           'j' // for chat specific commands (say, gag, mute, etc.)

// UNUSED: ADMIN_VOTE is now unused because cl_showpluginmessages is defaulted to 0 and on some Linux distros, 
// top menus outright don't work despite no errors and all the calls suggesting it should pop up on the client's screen. 
// A rework of the top menus API will be done to test a potential fix. For now, this permission is unused.
#define ADMIN_VOTE           'k' // for votes (votekick, voteban, votemute, votespec, etc.)
#define ADMIN_PASSWORD       'l' // for sv_password. Combines with CVAR permission to allow `sa cvar sv_password` usage.
#define ADMIN_RCON           'm' // bypasses the need for an rcon password, 
                                 // use in-game for issuing commands 
                                 // as if they were typed in the server console
#define ADMIN_CHEATS         'n' // primarily for sv_cheats
#define ADMIN_ROOT           'z' // all permissions, immunity for kick, ban, gag & mute

enum SpecialTarget
{
    TARGET_INVALID = -1,
    TARGET_ALL,
    TARGET_BLUE,
    TARGET_RED,
    TARGET_ALL_BUT_ME,
    TARGET_ME,
    TARGET_ALIVE,
    TARGET_DEAD,
    TARGET_BOTS,
    TARGET_HUMANS
};

enum AdminReplySource
{
    ADMIN_REPLY_SERVER_CONSOLE,
    ADMIN_REPLY_CONSOLE,
    ADMIN_REPLY_CHAT
};


extern CUtlMap<const char *, SpecialTarget> g_SpecialTargets;
extern CUtlMap<int, bool> g_PlayerCommandSourceMap;

struct AdminData_t
{
    CUtlString flags;  // Permissions flags string

    AdminData_t() {}
    AdminData_t( const char *szFlags ) : flags( szFlags ) {}
};

class CHL2MP_Admin
{
public:
    CHL2MP_Admin();
    ~CHL2MP_Admin();

    void Initialize( const char *steamID, const char *permissions );
    bool HasPermission( char flag ) const;

    const char *GetSteamID() const { return m_steamID; }

    static void InitAdminSystem();
    static CUtlMap<CUtlString, AdminData_t> &GetAdminMap();
    static bool ParseAdminFile( const char *filename, CUtlMap<CUtlString, AdminData_t> &outAdminMap );
    static void SaveAdminCache();  // For persisting updated list if needed
    static bool IsPlayerAdmin( CBasePlayer *pPlayer, const char *requiredFlags );
    static void ClearAllAdmins();
    static CHL2MP_Admin *GetAdmin( const char *steamID );
    // static void RegisterAdminCommands();
    static void AddAdmin( const char *steamID, const char *permissions );
    bool FindSpecialTargetGroup( const char *targetSpecifier );
    void ResetSpecialTargetGroup();
    static void CheckChatText( char *p, int bufsize );
    static void LogAction( CBasePlayer *pAdmin, CBasePlayer *pTarget, const char *action, const char *details = "", const char *groupTarget = nullptr );

    bool IsAllPlayers() const { return bAll; }
    bool IsAllBluePlayers() const { return bBlue; }
    bool IsAllRedPlayers() const { return bRed; }
    bool IsAllButMePlayers() const { return bAllButMe; }
    bool IsMe() const { return bMe; }
    bool IsAllAlivePlayers() const { return bAlive; }
    bool IsAllDeadPlayers() const { return bDead; }
    bool IsAllBotsPlayers() const { return bBots; }
    bool IsAllHumanPlayers() const { return bHumans; }

    void TargetAllPlayers( bool enabled ) { bAll = enabled; }
    void TargetAllBlue( bool enabled ) { bBlue = enabled; }
    void TargetAllRed( bool enabled ) { bRed = enabled; }
    void TargetAllButMe( bool enabled ) { bAllButMe = enabled; }
    void TargetMe( bool enabled ) { bMe = enabled; }
    void TargetAllPlayersAlive( bool enabled ) { bAlive = enabled; }
    void TargetAllPlayersDead( bool enabled ) { bDead = enabled; }
    void TargetAllBots( bool enabled ) { bBots = enabled; }
    void TargetAllHumans( bool enabled ) { bHumans = enabled; }

private:
    const char *m_steamID;
    const char *m_permissions;

    // special group target
    bool bAll;
    bool bBlue;
    bool bRed;
    bool bAllButMe;
    bool bMe;
    bool bAlive;
    bool bDead;
    bool bBots;
    bool bHumans;

    static bool bIsListenServerMsg;
};

extern CUtlVector<CHL2MP_Admin *> g_AdminList;

extern CHL2MP_Admin *g_pHL2MPAdmin;

inline CHL2MP_Admin *HL2MPAdmin()
{
    return static_cast< CHL2MP_Admin * >( g_pHL2MPAdmin );
}

#endif // HL2MP_SERVERADMIN_H