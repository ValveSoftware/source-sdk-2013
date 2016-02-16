## ALPHA
- [x] PORT TO MP BRANCH

- [ ] Gameinterface.cpp (Server)
    - [ ] Delete comment on Motd file not being able to be loaded (L:1721) once we're on MP branch

- [ ] ClientScoreboardDialog.cpp (Client)
    - [ ] Get online data from the API
    - [x] Add friends leaderboard list
    - [x] Add format for online & friends leaderboard lists
    - [ ] Fill the lists with API data
    - [ ] Update rank for event runtime_posted
    - [x] Discuss update interval time
    - [ ] Consider adding a "100 tick" column
    - [x] Localize rank tokens
    - [ ] Make FindItemIDForPlayerIndex(int) return an ItemID for another person's time
    - [ ] Sort function for online times
    - [x] Fix bugs: Lines being chopped down & mapsummary not being set
    - [x] Discuss columns widths
    - [ ] De-hardcode the font used on size checking
    - [x] Find where to place friends leaderboards
    - [ ] A lot of variables are not necessary. Ensure which are and remove the rest
    - [ ] Use GetTextSize instead of calculating it per character
    
- [ ] MapSelector.cpp (Client)
    - [ ] Localize strings (remove ServerBrowser)
    - [x] Make the panel display from menu selection (edit GameMenu.res and do "engine ToggleMapsPanel")
    - [ ] Remove references to Server Browser
    - [ ] Cleanup classes to remove useless/commented code
    - [ ] Local Maps tab
        - [x] Header columns: Completed | Map name | Game type | Difficulty | Best time
        - [x] Populate with maps already on disk (parse .mom files?)
        - [ ] Parse .mom/zon/bsp files to fill the Dialog with information
    - [x] Change filters
        - [x] Map name
        - [x] Gametype (bhop/surf/etc)
        - [x] Difficulty 
        - [x] Linear/Staged        
    - [ ] "Online Maps" Tab
        - [ ] Apply filters to API searches
        - [ ] Parse data from API and apply to list
    - [ ] Start the selected map
        - [ ] Download a selected map, and its .zon and .mom file (if not exist)
        - [ ] Open a MapInfoDialog to show progress on download?
        - [x] Set the correct gamemode type, tickrate, etc based on .mom file
        
- [ ] MapInfoDialog.cpp (Client)
    - [ ] Localize (remove ServerBrowser)
    - [ ] Show information about the map
        - [ ] Local info
            - [ ] Local PBs
            - [ ] Link replays to each local time
            - [ ] Other local info/stats (how many total runs)
        - [ ] Online info
            - [ ] Top 10 times for the map
            - [ ] Link replays to each online time
            - [ ] Other online info/stats
    - [ ] Make the user able to start the map from the Dialog
        - [ ] Online maps need to be downloaded
        - [ ] Local maps simply start like normal
       
- [ ] The "mom"-ification and refactoring
    - [ ] mom_player (Server/Client)
         - [ ] Copy valuable snippets from HL2/CS Player classes
         - [x] Clean up the EntSelectSpawnPoint() method
    - [ ] mom_gamerules (Shared)
         - [ ] Follow the hl2_gamerules.cpp file for creation
         - [ ] Improve mom_spawn_with_weapons implementation
    - [x] mom_gamemovement.cpp (Shared)
        - [x] Implement rampboost fix by TotallyMehis
    - [ ] mom_usermessages.cpp (Shared)
        - [x] Remove the usermessages that aren't necessary
        - [ ] Remove even more messages
    - [ ] mom_client.cpp (Server)
        - [ ] Precache all necessary sounds/models for the mod
    - [ ] VPC Scripts
        - [ ] Add the new mom files to the proper VPC scripts
        - [ ] Verify & remove the files we deleted from the appropriate VPC scripts
    - [ ] Remove any and all unnecessary HL2/generic code that doesn't pertain to the mod
        - [x] Remove all ifdef (SIXENSE) code segments
        - [x] Remove all ifdef (XBOX/X360 etc) & IsX360() code segments
        - [x] Remove all ifdef (TF/PORTAL/DOD) code segments
        - [x] Remove all ifdef (HL2_EPISODIC/HL2_LOSTCOAST/HL2MP) code segments
        - [x] Remove all ifdef (HL1_DLL/CSPORT_DLL) code segments
        - [x] Remove all ifdef (INVASION_DLL/NEXT_BOT/TERROR/USES_ECON_ITEMS) code segments
        - [x] Inspect and cherry pick ifdef (CSTRIKE_DLL) code segments
        - [ ] Format (CTRL + K, CTRL + D) every class to follow the tabs->spaces and other spacing style for code
        - [ ] Remove any unused files
        	- [x] hud_squadstatus.cpp
            - [x] hud_posture.cpp
            - [x] hud_flashlight.cpp (the hud element)
            - [x] hud_battery.cpp
            - [ ] teamplayroundbased_gamerules.h/cpp
            - [ ] Entire hl2/ subfolder in client/server/shared
            - [x] Entire sdk/ subfolder in client/server/shared
        - [ ] Eventually undefine HL2_DLL and remove corresponding code

- [x] Creation of a shared (Client/Server) utils class with useful methods/data (gamemode, tickrate etc)
    - [x] Create global enumeration for gamemodes
    - [x] Store current gamemode on a global variable

- [ ] Gamemodes
    - [x] *BHOP* -> 100 tick, no stamina, autojump/no autojump (Cvar so people can manual scroll if they want)
    - [ ] *SURF* -> 66 (default)/100 , no stamina, autojump
    - [ ] *KZ* -> 100 tick, stamina, no autojump

- [ ] weapon_momentum_gun.cpp (Shared) and CS:S weapon entities
    - [x] Import CS:S weapon entities over
    - [x] Import extra CS:S weapon entities (Knife and Grenades)
    - [x] Cleanup CS:S weapon entities/reduce class clutter
    - [ ] Make the main gun toggleable (the player spawns with it, presses button to use/hide it)
    - [ ] Look into removing the crosshair? Customization?
    - [ ] Consider keeping the hud_ammo.cpp HUD element for displaying how many bullets the player has in the clip
	- [ ] Change model
	- [ ] Tone down or remove view-bob 

- [ ] timer.cpp (Client) 
    - [ ] Play effects (animations) for run states
    - [x] Move to bottom center (above speedometer)
    - [x] Utilize the .res file variables for position/color/etc
    - [x] Feed real data for the hud
    - [x] Have more info (checkpoints, current stage/total stages, etc)
    - [x] Only display relevant info
    - [ ] Act accordingly to gamemode
		- [ ] Add stamina settings for kz/scroll modes
    - [x] Implement Hud Messaging system to interact with Timer.cpp (server)
    - [x] Localization
    - [x] Discuss bufsize for strings taking intoa count localizations
    
- [ ] hud_cp_menu.cpp (Client) 
    - [x] Make creating a checkpoint stop your timer
    - [ ] Make checkpoints available for output to files
    - [ ] Consider local timer for routing
    - [ ] Consider KZ game mode basically requiring checkpoints
    - [ ] Extract underlying menu class and make hud_cp_menu create one
    - [x] Use .res files

- [ ] mom_triggers.cpp (Server)
    - [ ] Tweak limit speed method
    - [x] Implement Hud Messaging system to interact with timer.cpp (client)
    - [x] Add the option to define what angles should the player have after being teleported
    - [ ] Unify every spawnflag (So each one has an unique 'id') 
	- [ ] Bonus stages (either flags added to existing triggers or child entites)

- [ ] Timer.cpp (Server)
    - [ ] Add hash checking
    - [x] GetCPCount seems to return wrongly
    - [x] Are command flags needed?
    
- [x] tickset.cpp (Server) Make tick-setting crossplatform
    
- [x] In-game mapzone editor (Server/Client) allows for creation of zone files (on older CS maps) without using Hammer

- [ ] mapzones.cpp (Server)
    - [ ] Add support for the new custom triggers

- [x] mapzones_edit.cpp (Server)
	- [x] Change description for mom_zone_defmethod
	- [x] Improve methods of triggers (Probbably also find a better suitting name)
	- [ ] Improve binding (Add more build binds for things like mom_zone_defzone variations)

- [ ] Credits
	- [ ] Few values are set to default when the map ends, we have to set it to how the user had it before going to the credits
	- [ ] If the user disconnects before the credits ending, hud convar values will not be set back to how they were before
    
- [ ] GUI changes
	- [ ] Add practice mode state to timer VGUI panel 
	
## BETA
- [ ] Implement CEF
    - [ ] Create custom HTML HUD
    - [ ] Create custom HTML menu
    - [ ] Incorporate (precompiled/source?) into project  

- [ ] Consider importing Portal-like entities for non-euclidean geometry
    
- [ ] Replays
    - [ ] Use ghostingmod as an example of how to make/read the files
    - [ ] Make map selection/leaderboards be able to download replays

- [ ] Spectate system (like replays but streamed online)  
    - [ ] Use ghostingmod (server) as an example  
    - [ ] Racing system (like spectating but you can move around)
        - [ ] Synchronize start, notified ends
        - [ ] Allow disqualifications, drop outs, disconnections
	- [ ] Free play online ghosts (Similar to current bhop/surf servers. Community is important.)

- [ ] Global chat
    - [ ] Simple general and map chat via IRC or something
    
- [x] func_shootboost: (Potential entity that handles shootboosts (not needed))

- [ ] weapon_momentum_gun (Client/Server)
    - [ ] "Wire-frame" models for each gun override
    - [ ] Edited gun sounds that reference original sounds?

- [ ] Gamemodes (Ideas)
    - [ ] RocketJump -> 100 tick?, no stamina?, autojump?
	
## BETA+ (Official Release)
- [ ] Get greenlit

- [ ] Timer.cpp (Server)
    - [ ] Include the extra security measures
