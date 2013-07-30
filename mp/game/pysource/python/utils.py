if isserver:
    from gameinterface import CSingleUserRecipientFilter
    
from _utils import *

# Tracer Flags
TRACER_FLAG_WHIZ = 0x0001
TRACER_FLAG_USEATTACHMENT = 0x0002

TRACER_DONT_USE_ATTACHMENT = -1

# To be used with UTIL_ClientPrintAll
HUD_PRINTNOTIFY = 1
HUD_PRINTCONSOLE = 2
HUD_PRINTTALK = 3
HUD_PRINTCENTER = 4

# UTIL_BloodSpray flags
FX_BLOODSPRAY_DROPS = 0x01
FX_BLOODSPRAY_GORE = 0x02
FX_BLOODSPRAY_CLOUD = 0x04
FX_BLOODSPRAY_ALL = 0xFF

if isserver:
    def ClientPrint(player, msg_dest, msg_name, param1, param2, param3, param4):
        if not player:
            return

        user = CSingleUserRecipientFilter(player)
        user.MakeReliable()

        UTIL_ClientPrintFilter(user, msg_dest, msg_name, param1, param2, param3, param4)
else:
    def ClientPrint(player, msg_dest, msg_name, param1, param2, param3, param4): pass
    
if isserver:
    def UTIL_GetPlayers():
        players = []
        for i in range(1, gpGlobals.maxClients+1):
            player = UTIL_PlayerByIndex(i)
            if not player or not player.IsConnected():
                continue   
            players.append(player)
        return players
else:
    def UTIL_GetPlayers():
        players = []
        for i in range(1, gpGlobals.maxClients+1):
            player = UTIL_PlayerByIndex(i)
            if not player:
                continue   
            players.append(player)
        return players