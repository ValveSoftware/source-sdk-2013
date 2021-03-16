//===================================================================================================================================================

               Mapbase - Source 2013
	https://github.com/mapbase-source/source-sdk-2013
		https://www.moddb.com/mods/mapbase
	
//===================================================================================================================================================

This repository contains code from Mapbase, a modification of the Source 2013 SDK which serves as a combined package
of general-purpose improvements, fixes, and utility features for mods.

Mapbase's main content in this repository may include:

- 80+ custom entities (new logic entities, filters, etc.)
- Hundreds of Inputs/Outputs/KeyValues additions and modifications
- Custom SDK_ shaders with improvements adapted from Alien Swarm SDK code, including projected texture fixes and radial fog
- Custom VScript implementation based on public Squirrel API and Alien Swarm SDK interfaces/descriptions
- Additional gameplay control for Half-Life 2 mods, including grenade features for more NPCs and singleplayer respawning
- More map-specific capabilities for maps unable to branch into their own mods, e.g. MP mods or map compilations
- View rendering changes for drawing 3D skyboxes and RT-based entities
- Countless other fixes and improvements

//===================================================================================================================================================

Mapbase is an open-source project and its contents can be distributed and used at the discretion of its users. However, this project represents many parts of
the Source modding community packaged into a whole, so credit is taken very seriously.

	Up-to-date information about Mapbase content usage and credit are addressed in this article on Mapbase's wiki:
		https://github.com/mapbase-source/source-sdk-2013/wiki/Using-Mapbase-Content

//===================================================================================================================================================

>>>>>>>> EXTERNAL CONTENT USED IN MAPBASE <<<<<<<<

The Mapbase project is a combination of original code from its creators, code contributed by other Source modders, and code borrowed from open-source articles
and repositories (especially ones which are specifically published as free source code). One of Mapbase's goals is to make the most popular fixes and the most obscure
or complicated code changes accessible and easy to use for level designers and other kinds of Source modders who would otherwise have no idea how to implement them.

*** DISCLAIMER: Mapbase has a strict no-leak-content policy and only allows content created directly by contributors or content originating from open-source repositories.
If you believe any content in Mapbase originates from any leak or unauthorized source (from Valve or otherwise), please contact Blixibon immediately.
Mapbase is intended to be usable by everyone, including licensed Source projects and Steam mods. ***

-- The Alien Swarm SDK was used to backport features and code from newer branches of Source into a Source 2013/Half-Life 2 environment.
-- Mapbase also implements some of Tony Sergi's code changes from the Source 2007 SDK codebase. Both SDKs are publicly distributed by Valve and are available on Steam.

Some of the features backported from the Alien Swarm SDK (e.g. game instructor, particle rain) require assets from later versions of Source in order to work properly.
The required assets have been backported from Alien Swarm and Left 4 Dead for the release build. They are not available in the code repository.

Here's a list of Mapbase's other known external code sources:

- https://github.com/95Navigator/insolence-2013 (Initial custom shader code and projected texture improvements; also used to implement ASW SDK particle precipitation code)
- https://github.com/Biohazard90/g-string_2013 (Custom shadow filters, included indirectly via Insolence repo)
- https://github.com/KyleGospo/City-17-Episode-One-Source (Brush phong and projected texture changes, included indirectly via Insolence repo)
- https://github.com/DownFall-Team/DownFall (Multiple skybox code and fix for ent_fire delay not using floats; Also used as a guide to port certain Alien Swarm SDK changes to Source 2013,
including radial fog, rope code, and treesway)
- https://github.com/momentum-mod/game (Used as a guide to port postprocess_controller and env_dof_controller to Source 2013)
- https://github.com/DeathByNukes/source-sdk-2013 (VBSP manifest fixes)
- https://github.com/entropy-zero/source-sdk-2013 (skill_changed game event)

//---------------------------------------------------------------------------------------------------------------------------------------------------

Valve Developer Community (VDC) sources:

- https://developer.valvesoftware.com/wiki/Dynamic_RTT_shadow_angles_in_Source_2007 (Dynamic RTT shadow angles by Saul Rennison)
- https://developer.valvesoftware.com/wiki/Parallax_Corrected_Cubemaps (Parallax corrected cubemaps implementation from Brian Charles)
- https://developer.valvesoftware.com/wiki/Adding_the_Game_Instructor (ASW SDK game instructor adapted to Source 2013 by Kolesias123; was implemented based on a translated article)
- https://developer.valvesoftware.com/wiki/Brush_ladders (Functional func_ladders in Source 2013)
- https://developer.valvesoftware.com/wiki/CAreaPortalOneWay (func_areaportal_oneway)
- https://developer.valvesoftware.com/wiki/Implementing_Discord_RPC (Discord RPC implementation; Mapbase has its own framework originally based on this article)
- https://developer.valvesoftware.com/wiki/Rain_splashes (NOTE: This code is not completely used in Mapbase, but may still exist in its codebase)
- https://developer.valvesoftware.com/wiki/Hand_Viewmodels (NOTE: This code is not completely supported by default because Mapbase does not yet have weapon viewmodels which support
interchangeable arms; this may change in the future)

- https://developer.valvesoftware.com/wiki/General_SDK_Snippets_%26_Fixes (Various snippets on the article, including the dropship gun fix)
- https://developer.valvesoftware.com/wiki/Memory_Leak_Fixes (Most of these snippets were applied in later SDK updates, but some were not and have been added to Mapbase)
- https://developer.valvesoftware.com/wiki/Env_projectedtexture/fixes (Misc. env_projectedtexture fixes; Some of these are superceded by Alien Swarm-based changes and not used)
- https://developer.valvesoftware.com/wiki/Scenes.image (Original raw VCD file support; Code was improved for Mapbase and the article was later updated with it)
- https://developer.valvesoftware.com/wiki/Extending_Prop_Sphere (prop_sphere customization)
- https://developer.valvesoftware.com/wiki/TF2_Glow_Effect_(2013_SDK) (Glow effect)
- https://developer.valvesoftware.com/wiki/CFuncMoveLinear_ParentingFix (func_movelinear parenting fix; Code was improved for Mapbase and the article was later updated with it)
- https://developer.valvesoftware.com/wiki/Viewmodel_Prediction_Fix (Viewmodel prediction fix)
- https://developer.valvesoftware.com/wiki/Owner#Collisions_with_owner (FSOLID_COLLIDE_WITH_OWNER flag)
- https://developer.valvesoftware.com/wiki/Npc_clawscanner#Strider_Scout_Issue (npc_clawscanner strider scout fix)
- https://developer.valvesoftware.com/wiki/Ambient_generic:_stop_and_toggle_fix (Fixes for stopping/toggling ambient_generic)
- https://developer.valvesoftware.com/wiki/Func_clip_vphysics ("Start Disabled" keyvalue fix)
- https://developer.valvesoftware.com/wiki/Importing_CSS_Weapons_Into_HL2 (CS:S viewmodel chirality)

//---------------------------------------------------------------------------------------------------------------------------------------------------

Direct contributions:

- https://github.com/mapbase-source/source-sdk-2013/pull/5 (Custom VScript implementation by ReDucTor; was placed into feature branch before being merged in a subsequent PR)
- https://github.com/mapbase-source/source-sdk-2013/pull/3 ("playvideo" command playback fix from Avantate)
- https://github.com/mapbase-source/source-sdk-2013/pull/21 (Various GCC/Linux fixes from z33ky)
- https://github.com/mapbase-source/source-sdk-2013/pull/60 (Adjustment by RoyaleNoir to one of Saul's VDC changes)
- https://github.com/mapbase-source/source-sdk-2013/pull/84 (CS:S viewmodel chirality from 1upD)
- Demo autorecord code provided by Klems
- cc_emit crash fix provided by 1upD
- Custom HL2 ammo crate models created by Rara (Textures created by Blixibon; This is asset-based and, aside from the SLAM crate, not reflected in the code)
- Combine lock hardware on door01_left.mdl created by Kralich (This is asset-based and not reflected in the code)
- npc_vehicledriver fixes provided by CrAzY
- npc_combine cover behavior patches provided by iohnnyboy
- logic_playmovie icon created by URAKOLOUY5 (This is asset-based and not reflected in the code)

== Contributions from samisalreadytaken:
=-- https://github.com/mapbase-source/source-sdk-2013/pull/47 (VScript utility/consistency changes)
=-- https://github.com/mapbase-source/source-sdk-2013/pull/59 (New VScript functions and singletons based on API documentation in later Source/Source 2 games)
=-- https://github.com/mapbase-source/source-sdk-2013/pull/80 (More VScript changes, including support for extremely flexible client/server messaging)
=-- https://github.com/mapbase-source/source-sdk-2013/pull/105 (VScript fixes and optimizations, Vector class extensions, custom convars/commands)

//---------------------------------------------------------------------------------------------------------------------------------------------------

Other sources:

- Various code from Source SDK 2013 pull requests on the GitHub repository (https://github.com/ValveSoftware/source-sdk-2013/pulls):
-- https://github.com/ValveSoftware/source-sdk-2013/pull/441 (Smooth scrape sound oversight fix)
-- https://github.com/ValveSoftware/source-sdk-2013/pull/436 (VRAD debug counter fix + thread bump)
-- https://github.com/ValveSoftware/source-sdk-2013/pull/416 (Ragdoll null pointer dereference fix)
-- https://github.com/ValveSoftware/source-sdk-2013/pull/401 (func_rot_button "Starts locked" flag fix)
-- https://github.com/ValveSoftware/source-sdk-2013/pull/391 (VBSP func_detail smoothing group fix)
-- https://github.com/ValveSoftware/source-sdk-2013/pull/362 (npc_manhack npc_maker fix; Adjusted for formatting and save/restore in Mapbase)
-- https://github.com/Petercov/Source-PlusPlus/commit/ecdf50c48cd31dec4dbdb7fea2d0780e7f0dd8ec (used as a guide for porting the Alien Swarm SDK response system)
- https://github.com/momentum-mod/game/blob/1d066180b3bf74830c51e6914d46c40b0bea1fc2/mp/src/game/server/player.cpp#L6543 (spec_goto fix)
- Poison zombie barnacle crash fix implemented based on a snippet from HL2: Plus posted by Agent Agrimar on Discord (Mapbase makes the barnacle recognize it as poison just like poison headcrabs)
- https://gamebanana.com/skins/172192 (Airboat handling fix; This is asset-based and not reflected in the code)
- Vortigaunt LOS fix originally created by dky.tehkingd.u for HL2:CE
- https://combineoverwiki.net/wiki/File:Combine_main_symbol.svg ("icon_combine" instructor icon in "materials/vgui/hud/gameinstructor_hl2_1"; This is asset-based and not reflected in the code)

//---------------------------------------------------------------------------------------------------------------------------------------------------

If there is anything missing from this list, please contact Blixibon.

//===================================================================================================================================================

Aside from the content list above, Mapbase has more descriptive and up-to-date credits on this wiki article:
https://github.com/mapbase-source/source-sdk-2013/wiki/Mapbase-Credits

//===================================================================================================================================================

Please see the Source SDK 2013 license below:



               SOURCE 1 SDK LICENSE

Source SDK Copyright(c) Valve Corp.  

THIS DOCUMENT DESCRIBES A CONTRACT BETWEEN YOU AND VALVE 
CORPORATION ("Valve").  PLEASE READ IT BEFORE DOWNLOADING OR USING 
THE SOURCE ENGINE SDK ("SDK"). BY DOWNLOADING AND/OR USING THE 
SOURCE ENGINE SDK YOU ACCEPT THIS LICENSE. IF YOU DO NOT AGREE TO 
THE TERMS OF THIS LICENSE PLEASE DON’T DOWNLOAD OR USE THE SDK.  

  You may, free of charge, download and use the SDK to develop a modified Valve game 
running on the Source engine.  You may distribute your modified Valve game in source and 
object code form, but only for free. Terms of use for Valve games are found in the Steam 
Subscriber Agreement located here: http://store.steampowered.com/subscriber_agreement/ 

  You may copy, modify, and distribute the SDK and any modifications you make to the 
SDK in source and object code form, but only for free.  Any distribution of this SDK must 
include this LICENSE file and thirdpartylegalnotices.txt.  
 
  Any distribution of the SDK or a substantial portion of the SDK must include the above 
copyright notice and the following: 

    DISCLAIMER OF WARRANTIES.  THE SOURCE SDK AND ANY 
    OTHER MATERIAL DOWNLOADED BY LICENSEE IS PROVIDED 
    "AS IS".  VALVE AND ITS SUPPLIERS DISCLAIM ALL 
    WARRANTIES WITH RESPECT TO THE SDK, EITHER EXPRESS 
    OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED 
    WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, 
    TITLE AND FITNESS FOR A PARTICULAR PURPOSE.  

    LIMITATION OF LIABILITY.  IN NO EVENT SHALL VALVE OR 
    ITS SUPPLIERS BE LIABLE FOR ANY SPECIAL, INCIDENTAL, 
    INDIRECT, OR CONSEQUENTIAL DAMAGES WHATSOEVER 
    (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF 
    BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF 
    BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS) 
    ARISING OUT OF THE USE OF OR INABILITY TO USE THE 
    ENGINE AND/OR THE SDK, EVEN IF VALVE HAS BEEN 
    ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.  
 
       
If you would like to use the SDK for a commercial purpose, please contact Valve at 
sourceengine@valvesoftware.com.
