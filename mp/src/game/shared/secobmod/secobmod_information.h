//====================================================//
//  Source Engine: CoOperative Base Mod Information.  //
//====================================================//
//SecobMod__IFDEF_Fixes  These are mainly kept to show where if defined lines have had to be changed to ifdef for code to compile.
//SecobMod__IFDEF_Info   These are like the above but have information describing why the fix is in place.
//SecobMod__MiscFixes    These are small fixes to the code to allow things to run correctly.
//SecobMod__MiscFixes	   These are like the above but have information describing why the fix was required.
//SecobMod__Information  These are the main comment lines found in the source code explaining many things in detail.
//SecobMod__ChangeME!    These are lines of code that require changing for the game to run correctly once you start modifying the base.
//SecobMod__FixMe		   These are bugs in the code which require a fix before they'll work.


//================================================//
//  Co-Op Singleplayer Maps Information.          //
//================================================//
// To those wishing to make a co-op version of the Half-Life 2/Ep1/2 maps, Mulekick on the steam forums gave this advice for older maps:
// - This does mean that you will need to start using the Everything solution file, which means more work settings things up again -
//Basically brushes that block LOS from AI's won't work with older maps.
//To fix this:
//In public/bspflags.h, change the line starting with
//#define MASK_BLOCKLOS (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_BLOCKLOS)
//to:
//#define MASK_BLOCKLOS (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_BLOCKLOS|CONTENTS_OPAQUE)


//All OLD MOUNT CODE CLIENT/SERVER.
//=============================//
// Server Dynamic Mount Code.  //
//=============================//
/*
#ifdef SecobMod__USE_DYNAMIC_MOUNT_CODE	
//SecobMod__ChangeME!
//SecobMod__Information: This is the new dynamic GCF mounting code. Basically each map can be set to be either HL2, Ep1, Ep2 or (if you're feeling daring for your mod) Portal content enabled. If you enable Ep2, you lose some functionality that HL2 has, and vice-versa.
// as you can see. Mounting depends on a specific naming scheme for custom maps. These are SecobMod__hl2_<map name>, SecobMod__ep1_<mapname> and finally SecobMod__ep2_<mapname>
//REMEMBER ! ! ! ! To change SecobMod__ to whatever prefix your mod uses or you WILL crash ! ! !
//ALSO !!!!!!!!!!! Remember to change the mod folder search paths from seco7 to the name of your Mod folder. Otherwise you will have LOADS of problems on dynamically mounted maps!
	if ( !Q_strnicmp( gpGlobals->mapname.ToCStr(), "SecobMod__bkgd", 14 ))
	{
	//SecobMod__Information: Background maps use the static search paths for mounts. This is restrictive as to what's allowable in such maps but it's essential this not be changed due to main menu bugs.
	}
	else if ( !Q_strnicmp( gpGlobals->mapname.ToCStr(), "d1_", 3 )
	|| !Q_strnicmp( gpGlobals->mapname.ToCStr(), "d2_", 3 )
	|| !Q_strnicmp( gpGlobals->mapname.ToCStr(), "d3_", 3 )
	|| !Q_strnicmp( gpGlobals->mapname.ToCStr(), "dm_", 3 )
	|| !Q_strnicmp( gpGlobals->mapname.ToCStr(), "testchmb", 8 ) //Allow portal maps to load as HL2.
	|| !Q_strnicmp( gpGlobals->mapname.ToCStr(), "escape", 6 ) //Allow portal maps to load as HL2.
	|| !Q_strnicmp( gpGlobals->mapname.ToCStr(), "SecobMod__hl2_", 14 ) ) //Mounts required for HalfLife 2/Deathmatch content.
	{
		Msg("HALFLIFE 2 CONTENT IS BEING MOUNTED! \n");		
		filesystem->RemoveAllSearchPaths(); // We have to remove all search paths or the game gets confused about model vertex counts etc.
		//SecobMod__Information: Dedicated servers require different search paths to work.	
		filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2/bin", "EXECUTABLE_PATH");
		filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2", "PLATFORM");
		if (sv_dedicated.GetBool())
		{
			filesystem->AddSearchPath("mod_hl2mp", "MOD");
			filesystem->AddSearchPath("mod_hl2mp/bin", "GAMEBIN");
			filesystem->AddSearchPath("mod_hl2mp", "GAME");
		}
		else
		{
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "MOD");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/bin", "GAMEBIN");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "GAME");
		}
		filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/bin", "GAMEBIN");
		filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2", "GAME", PATH_ADD_TO_TAIL); //Add to the Tail
		filesystem->MountSteamContent(-220);  //Half-Life 2
		if (sv_dedicated.GetBool())
		{
			filesystem->AddSearchPath("mod_hl2mp", "DEFAULT_WRITE_PATH");
			filesystem->AddSearchPath("mod_hl2mp", "LOGDIR");
		}
		else
		{
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "DEFAULT_WRITE_PATH");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "LOGDIR");
		}
		filesystem->AddSearchPath("hl2mp", "GAME");
		filesystem->MountSteamContent(-320);  //Half-Life 2:Deathmatch
		filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/episodic", "GAME");
		filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2", "GAME");	
		
		#ifdef SecobMod__ENABLE_PORTAL_CONTENT_MOUNTING
		filesystem->AddSearchPath("portal", "GAME");
		filesystem->MountSteamContent(-400);  //Portal
		Msg("PORTAL CONTENT IS BEING MOUNTED! \n");
		#endif //SecobMod__ENABLE_PORTAL_CONTENT_MOUNTING
		
		sv_hl2_mount.SetValue(1);
		sv_ep1_mount.SetValue(0);
		sv_ep2_mount.SetValue(0);
		sv_failsafe_mount.SetValue(0);
	}
	else if ( !Q_strnicmp( gpGlobals->mapname.ToCStr(), "ep1_", 4 )
	|| !Q_strnicmp( gpGlobals->mapname.ToCStr(), "SecobMod__ep1_", 14 )	) //Mounts required for Episode 1 content.
	{
		Msg("EPISODE 1 CONTENT IS BEING MOUNTED! \n");
	
			//filesystem->RemoveAllSearchPaths(); // We have to remove all search paths or the game gets confused about model vertex counts etc.
			//engine->ServerCommand( "soundscape_flush\n" ); // We have to remove all sound paths or the game gets confused about where sounds are stored etc.
			
			filesystem->AddSearchPath("../../../steamapps/common/source sdk base 2013 multiplayer/", "BASE_PATH");
			filesystem->AddSearchPath("../../../steamapps/common/source sdk base 2013 multiplayer/bin/", "EXECUTABLE_PATH");
			filesystem->AddSearchPath("../../../steamapps/common/source sdk base 2013 multiplayer/platform/", "PLATFORM");
			
			
			filesystem->AddSearchPath("../../../steamapps/common/source sdk base 2013 multiplayer/hl2", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/source sdk base 2013 multiplayer/episodic/", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/source sdk base 2013 multiplayer/ep2/", "GAME");
			
			if (sv_dedicated.GetBool())
			{
				filesystem->AddSearchPath("mod_hl2mp", "MOD");
				filesystem->AddSearchPath("mod_hl2mp/bin", "GAMEBIN");
				filesystem->AddSearchPath("mod_hl2mp", "GAME");
				filesystem->AddSearchPath("mod_hl2mp", "DEFAULT_WRITE_PATH");
				filesystem->AddSearchPath("mod_hl2mp", "LOGDIR");
			}
			else
			{
				filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/", "GAME");
				filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/", "mod");
				filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/", "mod_write");
				filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/", "default_write_path");
				filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/bin/", "gamebin");
			}
			
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/hl2_sound_vo_english.vpk", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/hl2_pak.vpk", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/hl2_textures.vpk", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/hl2_sound_misc.vpk", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/hl2_misc.vpk", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/platform/platform_misc.vpk", "PLATFORM" );
			
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/episodic/", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2/", "GAME");
			
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/", "GAME");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/", "game_write");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/bin/", "gamebin");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/", "mod");
			
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/hl2mp_pak.vpk", "GAME");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/hl2mp_pak.vpk", "mod");

			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/episodic", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2", "GAME");

			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/episodic/ep1_pak.vpk", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/episodic/ep1_sound_vo_english.vpk", "GAME");
			
			
			//Old style layout for reference.
			filesystem->AddSearchPath("../ep2", "EXECUTABLE_PATH");
			filesystem->AddSearchPath("../ep2", "PLATFORM");
			filesystem->AddSearchPath("mod_hl2mp", "MOD");
			filesystem->AddSearchPath("source sdk base 2013 multiplayer/bin", "GAMEBIN");
			filesystem->AddSearchPath("mod_hl2mp", "GAME");
			filesystem->AddSearchPath("episodic", "GAMEBIN");
			filesystem->AddSearchPath("episodic", "GAME", PATH_ADD_TO_TAIL); //Add to the Tail
			filesystem->MountSteamContent(-380);  //Half-Life 2-Episode 1
			filesystem->AddSearchPath("hl2", "GAMEBIN");
			filesystem->AddSearchPath("hl2", "GAME", PATH_ADD_TO_TAIL); //Add to the Tail
			filesystem->MountSteamContent(-220);  //Half-Life 2
			filesystem->AddSearchPath("mod_hl2mp", "DEFAULT_WRITE_PATH");
			filesystem->AddSearchPath("mod_hl2mp", "LOGDIR");
			filesystem->AddSearchPath("hl2mp", "GAME");
			filesystem->MountSteamContent(-320);  //Half-Life 2:Deathmatch
			filesystem->AddSearchPath("ep2", "GAME");	
			
			#ifdef OBCO_ENABLE_PORTAL_CONTENT_MOUNTING
			filesystem->AddSearchPath("portal", "GAME");
			filesystem->MountSteamContent(-400);  //Portal
			Msg("PORTAL CONTENT IS BEING MOUNTED! \n");
			#endif //OBCO_ENABLE_PORTAL_CONTENT_MOUNTING
			

		
			//Default layout of gameinfo.txt's search paths.
			
			filesystem->AddSearchPath("../../../steamapps/common/source sdk base 2013 multiplayer/", "BASE_PATH");
			filesystem->AddSearchPath("../../../steamapps/common/source sdk base 2013 multiplayer/bin/", "EXECUTABLE_PATH");
			filesystem->AddSearchPath("../../../steamapps/common/source sdk base 2013 multiplayer/platform/", "PLATFORM");
			
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/", "GAME");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/", "mod");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/", "mod_write");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/", "default_write_path");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/bin/", "gamebin");
			
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/hl2_sound_vo_english.vpk", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/hl2_pak.vpk", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/hl2_textures.vpk", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/hl2_sound_misc.vpk", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/hl2_misc.vpk", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/platform/platform_misc.vpk", "PLATFORM" );
			
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/episodic/", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2/", "GAME");
			
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/", "GAME");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/", "game_write");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/bin/", "gamebin");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/", "mod");
			
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/hl2mp_pak.vpk", "GAME");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/hl2mp_pak.vpk", "mod");

			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/episodic", "GAME");
			filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2", "GAME");
			
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/", "LOGDIR");
			

	Msg ("These are the episodic search paths");
	 filesystem->PrintSearchPaths();
		
		#ifdef SecobMod__ENABLE_PORTAL_CONTENT_MOUNTING
		filesystem->AddSearchPath("portal", "GAME");
		filesystem->MountSteamContent(-400);  //Portal*/
		Msg("PORTAL CONTENT IS BEING MOUNTED! \n");
		#endif //SecobMod__ENABLE_PORTAL_CONTENT_MOUNTING
		
		sv_hl2_mount.SetValue(0);
		sv_ep1_mount.SetValue(1);
		sv_ep2_mount.SetValue(0);
		sv_failsafe_mount.SetValue(0);
	}
	else if ( !Q_strnicmp( gpGlobals->mapname.ToCStr(), "ep2_", 3 )
	|| !Q_strnicmp( gpGlobals->mapname.ToCStr(), "SecobMod__ep2_", 14 )) //Mounts required for Episode 2 content.
	{
		Msg("EPISODE 2 CONTENT IS BEING MOUNTED! \n");
		filesystem->RemoveAllSearchPaths(); // We have to remove all search paths or the game gets confused about model vertex counts etc.
		//SecobMod__Information: Dedicated servers require different search paths to work.
		filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2/bin", "EXECUTABLE_PATH");
		filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2", "PLATFORM");
		if (sv_dedicated.GetBool())
		{
			filesystem->AddSearchPath("mod_hl2mp", "MOD");
			filesystem->AddSearchPath("mod_hl2mp/bin", "GAMEBIN");
			filesystem->AddSearchPath("mod_hl2mp", "GAME");
		}
		else
		{
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "MOD");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/bin", "GAMEBIN");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "GAME");
		}
		filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2/bin", "GAMEBIN");
		filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2", "GAME", PATH_ADD_TO_TAIL); //Add to the Tail
		filesystem->MountSteamContent(-420);  //Half-Life 2-Episode 2
		filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/bin", "GAMEBIN");
		filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2", "GAME", PATH_ADD_TO_TAIL); //Add to the Tail
		filesystem->MountSteamContent(-220);  //Half-Life 2
		if (sv_dedicated.GetBool())
		{
			filesystem->AddSearchPath("mod_hl2mp", "DEFAULT_WRITE_PATH");
			filesystem->AddSearchPath("mod_hl2mp", "LOGDIR");
		}
		else
		{
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "DEFAULT_WRITE_PATH");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "LOGDIR");
		}
		filesystem->AddSearchPath("hl2mp", "GAME");
		filesystem->MountSteamContent(-320);  //Half-Life 2:Deathmatch
		filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/episodic", "GAME");
		filesystem->MountSteamContent(-380);  //Half-Life 2-Episode 1
		
		#ifdef SecobMod__ENABLE_PORTAL_CONTENT_MOUNTING
		filesystem->AddSearchPath("portal", "GAME");
		filesystem->MountSteamContent(-400);  //Portal*/
		Msg("PORTAL CONTENT IS BEING MOUNTED! \n");
		#endif //SecobMod__ENABLE_PORTAL_CONTENT_MOUNTING
		
		sv_hl2_mount.SetValue(0);
		sv_ep1_mount.SetValue(0);
		sv_ep2_mount.SetValue(1);
		sv_failsafe_mount.SetValue(0);
	}
	else 
	{
		Msg("WARNING ! FAIL-SAFE CONTENT IS BEING MOUNTED! MAP NAME NOT VALID FOR DYNAMIC GCF MOUNTING! FIX THIS OR CHANGE TO A DIFFERENT LEVEL ! \n");
		filesystem->RemoveAllSearchPaths(); // We have to remove all search paths or the game gets confused about model vertex counts etc.
		filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2/bin", "EXECUTABLE_PATH");
		filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2", "PLATFORM");
		if (sv_dedicated.GetBool())
		{
			filesystem->AddSearchPath("mod_hl2mp", "MOD");
			filesystem->AddSearchPath("mod_hl2mp/bin", "GAMEBIN");
			filesystem->AddSearchPath("mod_hl2mp", "GAME");
		}
		else
		{
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "MOD");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/bin", "GAMEBIN");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "GAME");
		}
		filesystem->AddSearchPath("hl2mp", "GAME");
		filesystem->MountSteamContent(-320);  //Half-Life 2:Deathmatch		
		if (sv_dedicated.GetBool())
		{
			filesystem->AddSearchPath("mod_hl2mp", "DEFAULT_WRITE_PATH");
			filesystem->AddSearchPath("mod_hl2mp", "LOGDIR");
		}
		else
		{
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "DEFAULT_WRITE_PATH");
			filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "LOGDIR");
		}
		sv_hl2_mount.SetValue(0);
		sv_ep1_mount.SetValue(0);
		sv_ep2_mount.SetValue(0);
		sv_failsafe_mount.SetValue(1);
		Msg("WARNING ! FAIL-SAFE CONTENT IS BEING MOUNTED! MAP NAME NOT VALID FOR DYNAMIC GCF MOUNTING! FIX THIS OR CHANGE TO A DIFFERENT LEVEL ! \n");
	}	
#endif //SecobMod__USE_DYNAMIC_MOUNT_CODE
*/

//=============================//
// Client Dynamic Mount Code.  //
//=============================//
/*
#ifdef SecobMod__USE_DYNAMIC_MOUNT_CODE
			//Why did I remove this section from Alpha 2 !
			// Obsidian: Always flush the mdlcache here, except when it will crash
			// Expression is a little complex, what it says is that if we're a listen server
			// and the localplayer is index 1, don't flush anything.
			//SecobMod__Information: If we don't flush the cache here strange things happen to the NPCs in dedicated server games. Example: The metropolice float (yes, in the AIR) round "matrix" style.
			if ( !(!sv_dedicated.GetBool() && (engine->GetLocalPlayer() == 1)) )
			{
			    Msg("CLIENT LEVEL PRE-INIT IS FLUSHING CACHE !");
				mdlcache->Flush();
				UncacheAllMaterials();
			}
	//SecobMod__ChangeME!		
	if (sv_hl2_mount.GetBool())
	{
	filesystem->RemoveAllSearchPaths(); // We have to remove all search paths or the game gets confused about model vertex counts etc.
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2/bin", "EXECUTABLE_PATH");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2", "PLATFORM");
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "MOD");
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/bin", "GAMEBIN");
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "GAME");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/bin", "GAMEBIN");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2", "GAME", PATH_ADD_TO_TAIL); //Add to the Tail
	filesystem->MountSteamContent(-220);  //Half-Life 2
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "DEFAULT_WRITE_PATH");
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "LOGDIR");
	filesystem->AddSearchPath("hl2mp", "GAME");
	filesystem->MountSteamContent(-320);  //Half-Life 2:Deathmatch
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/episodic", "GAME");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2", "GAME");	
	
	#ifdef SecobMod__ENABLE_PORTAL_CONTENT_MOUNTING
	filesystem->AddSearchPath("portal", "GAME");
	filesystem->MountSteamContent(-400);  //Portal
	#endif //SecobMod__ENABLE_PORTAL_CONTENT_MOUNTING
	}		
	//=================
	if (sv_ep1_mount.GetBool())
	{
	/*filesystem->RemoveAllSearchPaths(); // We have to remove all search paths or the game gets confused about model vertex counts etc.
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2/bin", "EXECUTABLE_PATH");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2", "PLATFORM");
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "MOD");
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/bin", "GAMEBIN");
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "GAME");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/episodic/bin", "GAMEBIN");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/episodic", "GAME", PATH_ADD_TO_TAIL); //Add to the Tail
	filesystem->MountSteamContent(-380);  //Half-Life 2-Episode 1
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/bin", "GAMEBIN");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2", "GAME");
	filesystem->MountSteamContent(-220);  //Half-Life 2
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "DEFAULT_WRITE_PATH");
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "LOGDIR");
	filesystem->AddSearchPath("hl2mp", "GAME");
	filesystem->MountSteamContent(-320);  //Half-Life 2:Deathmatch
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2", "GAME");	
	
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/episodic/", "GAME");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/episodic/ep1_sound_vo_english.vpk", "GAME");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/episodic/ep1_pak.vpk", "GAME");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/hl2_sound_vo_english.vpk", "GAME");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/hl2_pak.vpk", "GAME");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/hl2_textures.vpk", "GAME");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/hl2_sound_misc.vpk", "GAME");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/hl2_misc.vpk", "GAME");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/episodic/", "GAME");
	
	#ifdef SecobMod__ENABLE_PORTAL_CONTENT_MOUNTING
	filesystem->AddSearchPath("portal", "GAME");
	filesystem->MountSteamContent(-400);  //Portal
	#endif //SecobMod__ENABLE_PORTAL_CONTENT_MOUNTING
	}		
	//=================
	if (sv_ep2_mount.GetBool())
	{
	filesystem->RemoveAllSearchPaths(); // We have to remove all search paths or the game gets confused about model vertex counts etc.
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2/bin", "EXECUTABLE_PATH");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2", "PLATFORM");
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "MOD");
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/bin", "GAMEBIN");
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "GAME");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2/bin", "GAMEBIN");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2", "GAME", PATH_ADD_TO_TAIL); //Add to the Tail
	filesystem->MountSteamContent(-420);  //Half-Life 2-Episode 2
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2/bin", "GAMEBIN");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/hl2", "GAME");
	filesystem->MountSteamContent(-220);  //Half-Life 2
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "DEFAULT_WRITE_PATH");
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "LOGDIR");
	filesystem->AddSearchPath("hl2mp", "GAME");
	filesystem->MountSteamContent(-320);  //Half-Life 2:Deathmatch
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/episodic", "GAME");
	filesystem->MountSteamContent (-380);  //Half-Life 2-Episode 1
	
	#ifdef SecobMod__ENABLE_PORTAL_CONTENT_MOUNTING
	filesystem->AddSearchPath("portal", "GAME");
	filesystem->MountSteamContent(-400);  //Portal
	#endif //SecobMod__ENABLE_PORTAL_CONTENT_MOUNTING
	}			
	//=================
	if (sv_failsafe_mount.GetBool())
	{
	filesystem->RemoveAllSearchPaths(); // We have to remove all search paths or the game gets confused about model vertex counts etc.
	Msg("WARNING ! FAIL-SAFE CONTENT IS BEING MOUNTED! MAP NAME NOT VALID FOR DYNAMIC GCF MOUNTING! FIX THIS OR CHANGE TO A DIFFERENT LEVEL ! \n");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2/bin", "EXECUTABLE_PATH");
	filesystem->AddSearchPath("../../../steamapps/common/Half-Life 2/ep2", "PLATFORM");
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "MOD");
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp/bin", "GAMEBIN");
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "GAME");
	filesystem->AddSearchPath("hl2mp", "GAMEBIN");
	filesystem->AddSearchPath("hl2mp", "GAME");
	filesystem->MountSteamContent(-320);  //Half-Life 2:Deathmatch
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "DEFAULT_WRITE_PATH");
	filesystem->AddSearchPath("../../../steamapps/SourceMods/mod_hl2mp", "LOGDIR");
	Msg("WARNING ! FAIL-SAFE CONTENT IS BEING MOUNTED! MAP NAME NOT VALID FOR DYNAMIC GCF MOUNTING! FIX THIS OR CHANGE TO A DIFFERENT LEVEL ! \n");
	}
#endif //SecobMod__USE_DYNAMIC_MOUNT_CODE
*/


//=============================//
//    Static Mount Code.       //
//=============================//
/*#ifdef SecobMod__USE_STATIC_MOUNT_CODE
	//SecobMod__Information: This is our base game mount code. It relies on a text file to hold all the search paths/AppIDs which your mod requires. Make sure people know what you add to this, because if they don't have the
	// game that you choose to mount here, they could well crash to desktop. The file is in the root of the compiled modification folder. This is how we allow people who have just loaded the game to use map and changelevel commands
	// to any official valve map.
	  KeyValues *pkvMount = new KeyValues( "InstalledSourceGames" );
	   if ( pkvMount->LoadFromFile( filesystem, "InstalledSourceGames.txt" ) )
	   {
	      while ( pkvMount )
	      {
	         const char *pszMountName = pkvMount->GetName();
	         KeyValues *pkvSearchPath = pkvMount->FindKey( "searchpath" );
	         KeyValues *pkvAppID = pkvMount->FindKey( "appid" );
			 
	
	         if ( pszMountName && pkvSearchPath && pkvAppID )
	         {
	            const char *pszMountPath = pkvSearchPath->GetString();
	            int nMountID = pkvAppID->GetInt();
				
				//SecobMod__Information: If InstalledSourceGames.txt attempts to mount Portal, but Portal is disabled in the code - skip trying to mount Portal.
				#ifndef SecobMod__ENABLE_PORTAL_CONTENT_MOUNTING
				if (nMountID == -400)
				{
				break;
				}
				#endif //SecobMod__ENABLE_PORTAL_CONTENT_MOUNTING
		
	            filesystem->AddSearchPath(pszMountPath, "GAME");
	            filesystem->MountSteamContent(nMountID);
	         }
	
	         pkvMount = pkvMount->GetNextKey();
	      }
	   }
#endif //SecobMod__USE_STATIC_MOUNT_CODE*/

//=========================//
//  GameInfo.txt Backup.   //
//=========================//
/*
"GameInfo"
{
	game	"Source Engine Co-Operative Base Modification (2013)"
	title	"Source Engine Co-Operative Base Modification"
	title2	"(2013)"
	type multiplayer_only
	nomodels 0
	nohimodel 0
	nocrosshair 0
	hidden_maps
	{
		"test_speakers"		1
		"test_hardware"		1
	}


	FileSystem
	{
		SteamAppId				243750
		
		SearchPaths
		{
			//SecobMod__ Allow custom content by user .vpk packaged mods.
			game+mod			hl2mp/custom/*
			game+mod			hl2/custom/*

			//SecobMod__ Set the gameinfo.txt's location to that of the mod search path here.
			game+mod+mod_write+default_write_path		|gameinfo_path|.
			gamebin				|gameinfo_path|bin

			//SecobMod__ Load all the .vpk files that we want -EXCEPT- hl2mp files (which break the AI if they're enabled so early).
			game_lv				hl2/hl2_lv.vpk //Low Violence.
			
			//SecobMod__ Newest game first, so here ep2,ep1,hl2s multitude of .vpk files.			
			game				|all_source_engine_paths|ep2/ep2_sound_vo_english.vpk
			game				|all_source_engine_paths|ep2/ep2_pak.vpk
			game				|all_source_engine_paths|episodic/ep1_sound_vo_english.vpk
			game				|all_source_engine_paths|episodic/ep1_pak.vpk
			game				|all_source_engine_paths|hl2/hl2_sound_vo_english.vpk
			game				|all_source_engine_paths|hl2/hl2_pak.vpk
			game				|all_source_engine_paths|hl2/hl2_textures.vpk
			game				|all_source_engine_paths|hl2/hl2_sound_misc.vpk
			game				|all_source_engine_paths|hl2/hl2_misc.vpk
			platform			|all_source_engine_paths|platform/platform_misc.vpk
			
			//SecobMod__ The below lines allow the game to find the location of all singleplayer maps for playing. Remove the block if you don't want to be able to do this. Good for testing stuff though.
			game "|gameinfo_path|..\..\common\Half-Life 2\hl2" 
			game "|gameinfo_path|..\..\common\Half-Life 2\episodic"
			game "|gameinfo_path|..\..\common\Half-Life 2\ep2"


			//SecobMod__ Set the Source SDK 2013 Multiplayer (hl2mp) folder as the game search and write path.
			game+game_write		hl2mp

			//SecobMod__ Location of the Source SDK 2013 Multiplayer exe file.
			gamebin				hl2mp/bin

			//SecobMod__ Now we can mount in our game+mod files (hl2mp) without breaking stuff.
			game+mod			|all_source_engine_paths|hl2mp
			game+mod			hl2mp/hl2mp_pak.vpk
			game				|all_source_engine_paths|episodic
			game				|all_source_engine_paths|hl2
			platform			|all_source_engine_paths|platform
		}
	}
}
*/

//====================================================//
//  Complete list of modified files. 				  //
//====================================================//
/*
achievements_hlx.cpp 
activitylist.cpp 
ai_activity.cpp 
ai_activity.h 
ai_allymanager.cpp 
ai_basenpc.cpp 
ai_basenpc.h 
ai_basenpc_schedule.cpp 
ai_behavior_actbusy.cpp 
ai_behavior_fear.cpp 
ai_behavior_follow.cpp 
ai_behavior_lead.cpp 
ai_behavior_passenger.cpp 
ai_behavior_passenger.h 
ai_behavior_standoff.cpp 
ai_hint.cpp 
ai_moveprobe.cpp 
ai_planesolver.cpp 
ai_playerally.cpp 
ai_relationship.cpp 
ai_scriptconditions.cpp 
ai_scriptconditions.h 
ai_speech.cpp 
ai_utils.h 
antlion_maker.cpp 
baseanimating.h 
basecombatcharacter.cpp 
basecombatweapon_shared.cpp 
baseentity.cpp 
baseentity_shared.cpp 
baseflex.cpp 
basegrenade_shared.cpp 
baseplayer_shared.cpp 
baseviewmodel_shared.cpp 
baseviewmodel_shared.h 
baseviewport.cpp 
c_baseentity.cpp 
c_baseplayer.cpp 
c_baseplayer.h 
c_colorcorrection.cpp 
c_dynamiclight.cpp 
c_hl2mp_player.cpp 
c_hl2mp_player.h 
c_npc_advisor.cpp 
c_npc_antlionguard.cpp 
c_prop_scalable.cpp 
c_vehicle_airboat.cpp 
c_vehicle_jeep.cpp 
c_weapon_stunstick.cpp 
cdll_client_int.cpp 
classmenu.cpp 
client.cpp 
clientmode_shared.cpp 
colorcorrectionvolume.cpp 
combine_mine.cpp 
CommentarySystem.cpp 
effects.cpp 
EntityDissolve.cpp 
entitylist.cpp 
env_headcrabcanister.cpp 
env_zoom.cpp 
EnvHudHint.cpp 
EnvMessage.cpp 
ep2_gamestats.cpp 
func_break.cpp 
func_recharge.cpp 
func_tank.cpp 
func_tank.h 
fx_impact.cpp 
game.cpp 
gameinterface.cpp 
gameinterface.h 
gamemovement.cpp 
gamemovement.h 
gamerules.cpp 
gameweaponmanager.cpp 
geiger.cpp 
genericactor.cpp 
gib.cpp 
globals.cpp 
globals.h 
grenade_frag.cpp 
hl_gamemovement.cpp 
hl_gamemovement.h 
hl2_gamerules.cpp 
hl2_gamerules.h 
hl2_player.cpp 
hl2_usermessages.cpp 
hl2mp_client.cpp 
hl2mp_gamerules.cpp 
hl2mp_gamerules.h 
hl2mp_player.cpp 
hl2mp_player.h 
hl2mp_weapon_parse.cpp 
hl2mp_weapon_parse.h 
hud_ammo.cpp 
hud_basechat.cpp 
hud_battery.cpp 
hud_damageindicator.cpp 
hud_flashlight.cpp 
hud_health.cpp 
hud_locator.cpp 
hud_poisondamageindicator.cpp 
hud_posture.cpp 
hud_squadstatus.cpp 
hud_suitpower.cpp 
hud_zoom.cpp 
ilagcompensationmanager.h 
in_main.cpp 
item_battery.cpp 
item_dynamic_resupply.cpp 
item_suit.cpp 
iviewrender.h 
logicentities.cpp 
maprules.cpp 
message_entity.cpp 
monstermaker.cpp 
movevars_shared.cpp 
multiplay_gamerules.cpp 
multiplayer_animstate.cpp 
multiplayer_animstate.h 
npc_advisor.cpp 
npc_alyx_episodic.cpp 
npc_antlion.cpp 
npc_antlion.h 
npc_antlionguard.cpp 
npc_attackchopper.cpp 
npc_barnacle.cpp 
npc_barnacle.h 
npc_basescanner.cpp 
npc_BaseZombie.cpp 
npc_citizen17.cpp 
npc_citizen17.h 
npc_combine.cpp 
npc_combinedropship.cpp 
npc_combinegunship.cpp 
npc_combines.cpp 
npc_combines.h 
npc_dog.cpp 
npc_enemyfinder.cpp 
npc_headcrab.cpp 
npc_headcrab.h 
npc_metropolice.cpp 
npc_playercompanion.cpp 
npc_playercompanion.h 
npc_rollermine.cpp 
npc_scanner.cpp 
npc_strider.cpp 
npc_talker.cpp 
npc_turret.cpp 
npc_turret_floor.cpp 
npc_turret_floor.h 
npc_vortigaunt_episodic.cpp 
npc_zombine.cpp 
particlesystemquery.cpp 
physconstraint.cpp 
physics.cpp 
physics.h 
physics_npc_solver.cpp 
player.cpp 
player.h 
player_command.cpp 
player_lagcompensation.cpp 
player_pickup.cpp 
prop_combine_ball.cpp 
prop_thumper.cpp 
proto_sniper.cpp 
ragdoll_shared.cpp 
recipientfilter.cpp 
sceneentity.cpp 
script_intro.cpp 
shareddefs.h 
soundscape.cpp 
subs.cpp 
teamplayroundbased_gamerules.cpp 
test_stressentities.cpp 
trigger_portal.cpp 
triggers.cpp 
usercmd.cpp 
usercmd.h 
util.cpp 
util.h 
util_shared.cpp 
vehicle_airboat.cpp 
vehicle_crane.cpp 
vehicle_jeep.cpp 
vehicle_jeep.h 
vehicle_jeep_episodic.cpp 
vehicle_prisoner_pod.cpp 
vehicle_viewcontroller.cpp 
vguitextwindow.cpp 
view.cpp 
viewrender.h 
weapon_ar2.cpp 
weapon_ar2.h 
weapon_crossbow.cpp 
weapon_crowbar.cpp 
weapon_crowbar.h 
weapon_frag.cpp 
weapon_hl2mpbase.cpp 
weapon_hl2mpbase.h 
weapon_hl2mpbase_machinegun.h 
weapon_hl2mpbasebasebludgeon.h 
weapon_physcannon.cpp 
weapon_physcannon.h 
weapon_pistol.cpp 
weapon_rpg.cpp 
weapon_rpg.h 
weapon_selection.cpp 
weapon_shotgun.cpp 
weapon_slam.cpp 
weapon_smg1.cpp 
weapon_striderbuster.cpp 
weapon_stunstick.cpp*/