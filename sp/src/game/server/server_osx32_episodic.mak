NAME=server_episodic
SRCROOT=../..
TARGET_PLATFORM=osx32
TARGET_PLATFORM_EXT=
USE_VALVE_BINDIR=0
PWD := $(shell pwd)
# If no configuration is specified, "release" will be used.
ifeq "$(CFG)" ""
	CFG = release
endif

GCC_ExtraCompilerFlags=
GCC_ExtraLinkerFlags = 
SymbolVisibility = hidden
OptimizerLevel = -gdwarf-2 -g $(OptimizerLevel_CompilerSpecific)
SystemLibraries = -liconv 
DLL_EXT = .dylib
SYM_EXT = .dSYM
FORCEINCLUDES= 
ifeq "$(CFG)" "debug"
DEFINES += -DDEBUG -D_DEBUG -DGNUC -DPOSIX -D_OSX -DOSX -D_DARWIN_UNLIMITED_SELECT -DFD_SETSIZE=10240 -DQUICKTIME_VIDEO -DFORCE_QUICKTIME -DGL_GLEXT_PROTOTYPES -DDX_TO_GL_ABSTRACTION -DGAME_DLL -DVECTOR -DVERSION_SAFE_STEAM_API_INTERFACES -DPROTECTED_THINGS_ENABLE -Dsprintf=use_Q_snprintf_instead_of_sprintf -Dstrncpy=use_Q_strncpy_instead -D_snprintf=use_Q_snprintf_instead -DSWDS -DHL2_DLL -DHL2_EPISODIC -DUSES_SAVERESTORE -DVPCGAMECAPS=VALVE -DPROJECTDIR=/Users/joe/p4/ValveGames/rel/singleplayer/src/game/server -D_DLL_EXT=.dylib -DVPCGAME=valve -D_POSIX=1 
else
DEFINES += -DNDEBUG -DGNUC -DPOSIX -D_OSX -DOSX -D_DARWIN_UNLIMITED_SELECT -DFD_SETSIZE=10240 -DQUICKTIME_VIDEO -DFORCE_QUICKTIME -DGL_GLEXT_PROTOTYPES -DDX_TO_GL_ABSTRACTION -DGAME_DLL -DVECTOR -DVERSION_SAFE_STEAM_API_INTERFACES -DPROTECTED_THINGS_ENABLE -Dsprintf=use_Q_snprintf_instead_of_sprintf -Dstrncpy=use_Q_strncpy_instead -D_snprintf=use_Q_snprintf_instead -DSWDS -DHL2_DLL -DHL2_EPISODIC -DUSES_SAVERESTORE -DVPCGAMECAPS=VALVE -DPROJECTDIR=/Users/joe/p4/ValveGames/rel/singleplayer/src/game/server -D_DLL_EXT=.dylib -DVPCGAME=valve -D_POSIX=1 
endif
INCLUDEDIRS += ../../common ../../public ../../public/tier0 ../../public/tier1 ../../game/server/generated_proto_mod_episodic ../../thirdparty/protobuf-2.3.0/src ./ ../../game/shared ../../utils/common ../../game/shared/econ ../../game/server/NextBot ../../game/shared/hl2 ../../game/shared/episodic ./hl2 ./episodic 
CONFTYPE = dll
IMPORTLIBRARY = 
GAMEOUTPUTFILE = ../../../game/mod_episodic/bin/server.dylib
OUTPUTFILE=$(OBJ_DIR)/server.dylib
THIS_MAKEFILE = $(PWD)/server_osx32_episodic.mak
MAKEFILE_BASE = $(SRCROOT)/devtools/makefile_base_posix.mak


POSTBUILDCOMMAND = true



CPPFILES= \
    ../../common/language.cpp \
    ../../common/randoverride.cpp \
    ../../game/server/serverbenchmark_base.cpp \
    ../../game/server/vote_controller.cpp \
    ../../game/shared/achievementmgr.cpp \
    ../../game/shared/achievements_hlx.cpp \
    ../../game/shared/achievement_saverestore.cpp \
    ../../game/shared/activitylist.cpp \
    ../../game/shared/ammodef.cpp \
    ../../game/shared/animation.cpp \
    ../../game/shared/baseachievement.cpp \
    ../../game/shared/basecombatcharacter_shared.cpp \
    ../../game/shared/basecombatweapon_shared.cpp \
    ../../game/shared/baseentity_shared.cpp \
    ../../game/shared/basegrenade_shared.cpp \
    ../../game/shared/baseparticleentity.cpp \
    ../../game/shared/baseplayer_shared.cpp \
    ../../game/shared/baseviewmodel_shared.cpp \
    ../../game/shared/base_playeranimstate.cpp \
    ../../game/shared/beam_shared.cpp \
    ../../game/shared/collisionproperty.cpp \
    ../../game/shared/death_pose.cpp \
    ../../game/shared/debugoverlay_shared.cpp \
    ../../game/shared/decals.cpp \
    ../../game/shared/effect_dispatch_data.cpp \
    ../../game/shared/ehandle.cpp \
    ../../game/shared/entitylist_base.cpp \
    ../../game/shared/EntityParticleTrail_Shared.cpp \
    ../../game/shared/env_detail_controller.cpp \
    ../../game/shared/env_wind_shared.cpp \
    ../../game/shared/episodic/achievements_ep1.cpp \
    ../../game/shared/episodic/achievements_ep2.cpp \
    ../../game/shared/episodic/achievements_epx.cpp \
    ../../game/shared/eventlist.cpp \
    ../../game/shared/func_ladder.cpp \
    ../../game/shared/gamemovement.cpp \
    ../../game/shared/gamerules.cpp \
    ../../game/shared/gamerules_register.cpp \
    ../../game/shared/GameStats.cpp \
    ../../game/shared/gamestringpool.cpp \
    ../../game/shared/gamevars_shared.cpp \
    ../../game/shared/hintmessage.cpp \
    ../../game/shared/hintsystem.cpp \
    ../../game/shared/hl2/basehlcombatweapon_shared.cpp \
    ../../game/shared/hl2/env_headcrabcanister_shared.cpp \
    ../../game/shared/hl2/hl2_gamerules.cpp \
    ../../game/shared/hl2/hl2_usermessages.cpp \
    ../../game/shared/hl2/hl_gamemovement.cpp \
    ../../game/shared/hl2/survival_gamerules.cpp \
    ../../game/shared/igamesystem.cpp \
    ../../game/shared/interval.cpp \
    ../../game/shared/mapentities_shared.cpp \
    ../../game/shared/ModelSoundsCache.cpp \
    ../../game/shared/movevars_shared.cpp \
    ../../game/shared/mp_shareddefs.cpp \
    ../../game/shared/multiplay_gamerules.cpp \
    ../../game/shared/obstacle_pushaway.cpp \
    ../../game/shared/particlesystemquery.cpp \
    ../../game/shared/particle_parse.cpp \
    ../../game/shared/physics_main_shared.cpp \
    ../../game/shared/physics_saverestore.cpp \
    ../../game/shared/physics_shared.cpp \
    ../../game/shared/point_bonusmaps_accessor.cpp \
    ../../game/shared/point_posecontroller.cpp \
    ../../game/shared/precache_register.cpp \
    ../../game/shared/predictableid.cpp \
    ../../game/shared/props_shared.cpp \
    ../../game/shared/querycache.cpp \
    ../../game/shared/ragdoll_shared.cpp \
    ../../game/shared/rope_helpers.cpp \
    ../../game/shared/saverestore.cpp \
    ../../game/shared/SceneCache.cpp \
    ../../game/shared/sceneentity_shared.cpp \
    ../../game/shared/script_intro_shared.cpp \
    ../../game/shared/sequence_Transitioner.cpp \
    ../../game/shared/sheetsimulator.cpp \
    ../../game/shared/simtimer.cpp \
    ../../game/shared/singleplay_gamerules.cpp \
    ../../game/shared/SoundEmitterSystem.cpp \
    ../../game/shared/soundenvelope.cpp \
    ../../game/shared/Sprite.cpp \
    ../../game/shared/SpriteTrail.cpp \
    ../../game/shared/studio_shared.cpp \
    ../../game/shared/takedamageinfo.cpp \
    ../../game/shared/teamplay_gamerules.cpp \
    ../../game/shared/test_ehandle.cpp \
    ../../game/shared/usercmd.cpp \
    ../../game/shared/usermessages.cpp \
    ../../game/shared/util_shared.cpp \
    ../../game/shared/vehicle_viewblend_shared.cpp \
    ../../game/shared/voice_gamemgr.cpp \
    ../../game/shared/weapon_parse.cpp \
    ../../game/shared/weapon_parse_default.cpp \
    ../../game/shared/weapon_proficiency.cpp \
    ../../public/bone_setup.cpp \
    ../../public/collisionutils.cpp \
    ../../public/dt_send.cpp \
    ../../public/dt_utlvector_common.cpp \
    ../../public/dt_utlvector_send.cpp \
    ../../public/editor_sendcommand.cpp \
    ../../public/filesystem_helpers.cpp \
    ../../public/haptics/haptic_msgs.cpp \
    ../../public/interpolatortypes.cpp \
    ../../public/keyframe/keyframe.cpp \
    ../../public/map_utils.cpp \
    ../../public/networkvar.cpp \
    ../../public/registry.cpp \
    ../../public/rope_physics.cpp \
    ../../public/scratchpad3d.cpp \
    ../../public/ScratchPadUtils.cpp \
    ../../public/server_class.cpp \
    ../../public/simple_physics.cpp \
    ../../public/SoundParametersInternal.cpp \
    ../../public/stringregistry.cpp \
    ../../public/studio.cpp \
    ../../public/tier0/memoverride.cpp \
    ai_activity.cpp \
    ai_baseactor.cpp \
    ai_basehumanoid.cpp \
    ai_basenpc.cpp \
    ai_basenpc_flyer.cpp \
    ai_basenpc_flyer_new.cpp \
    ai_basenpc_movement.cpp \
    ai_basenpc_physicsflyer.cpp \
    ai_basenpc_schedule.cpp \
    ai_basenpc_squad.cpp \
    ai_behavior.cpp \
    ai_behavior_assault.cpp \
    ai_behavior_fear.cpp \
    ai_behavior_follow.cpp \
    ai_behavior_lead.cpp \
    ai_behavior_passenger.cpp \
    ai_behavior_rappel.cpp \
    ai_behavior_standoff.cpp \
    ai_blended_movement.cpp \
    ai_concommands.cpp \
    ai_condition.cpp \
    AI_Criteria.cpp \
    ai_default.cpp \
    ai_dynamiclink.cpp \
    ai_event.cpp \
    ai_eventresponse.cpp \
    ai_goalentity.cpp \
    ai_hint.cpp \
    ai_hull.cpp \
    ai_initutils.cpp \
    AI_Interest_Target.cpp \
    ai_link.cpp \
    ai_localnavigator.cpp \
    ai_looktarget.cpp \
    ai_memory.cpp \
    ai_motor.cpp \
    ai_moveprobe.cpp \
    ai_moveshoot.cpp \
    ai_movesolver.cpp \
    ai_namespaces.cpp \
    ai_navigator.cpp \
    ai_network.cpp \
    ai_networkmanager.cpp \
    ai_node.cpp \
    ai_pathfinder.cpp \
    ai_planesolver.cpp \
    ai_playerally.cpp \
    ai_relationship.cpp \
    AI_ResponseSystem.cpp \
    ai_route.cpp \
    ai_saverestore.cpp \
    ai_schedule.cpp \
    ai_scriptconditions.cpp \
    ai_senses.cpp \
    ai_sentence.cpp \
    ai_speech.cpp \
    ai_speechfilter.cpp \
    ai_squad.cpp \
    ai_squadslot.cpp \
    ai_tacticalservices.cpp \
    ai_task.cpp \
    ai_trackpather.cpp \
    ai_utils.cpp \
    ai_waypoint.cpp \
    baseanimating.cpp \
    BaseAnimatingOverlay.cpp \
    basebludgeonweapon.cpp \
    basecombatcharacter.cpp \
    basecombatweapon.cpp \
    baseentity.cpp \
    baseflex.cpp \
    basegrenade_concussion.cpp \
    basegrenade_contact.cpp \
    basegrenade_timed.cpp \
    basemultiplayerplayer.cpp \
    basetempentity.cpp \
    baseviewmodel.cpp \
    base_gameinterface.cpp \
    base_transmit_proxy.cpp \
    bitstring.cpp \
    bmodels.cpp \
    buttons.cpp \
    cbase.cpp \
    client.cpp \
    colorcorrection.cpp \
    colorcorrectionvolume.cpp \
    CommentarySystem.cpp \
    controlentities.cpp \
    cplane.cpp \
    CRagdollMagnet.cpp \
    damagemodifier.cpp \
    doors.cpp \
    dynamiclight.cpp \
    effects.cpp \
    EffectsServer.cpp \
    entityblocker.cpp \
    EntityDissolve.cpp \
    EntityFlame.cpp \
    entitylist.cpp \
    EntityParticleTrail.cpp \
    entity_tools_server.cpp \
    EnvBeam.cpp \
    EnvFade.cpp \
    EnvHudHint.cpp \
    EnvLaser.cpp \
    EnvMessage.cpp \
    envmicrophone.cpp \
    EnvShake.cpp \
    EnvSpark.cpp \
    env_debughistory.cpp \
    env_effectsscript.cpp \
    env_entity_maker.cpp \
    env_particlescript.cpp \
    env_player_surface_trigger.cpp \
    env_projectedtexture.cpp \
    env_screenoverlay.cpp \
    env_texturetoggle.cpp \
    env_tonemap_controller.cpp \
    env_zoom.cpp \
    episodic/ai_behavior_alyx_injured.cpp \
    episodic/ai_behavior_passenger_companion.cpp \
    episodic/ai_behavior_passenger_zombie.cpp \
    episodic/ep1_gamestats.cpp \
    episodic/ep2_gamestats.cpp \
    episodic/npc_advisor.cpp \
    episodic/npc_combine_cannon.cpp \
    episodic/npc_hunter.cpp \
    episodic/npc_magnusson.cpp \
    episodic/npc_puppet.cpp \
    episodic/prop_scalable.cpp \
    episodic/vehicle_jeep_episodic.cpp \
    episodic/weapon_striderbuster.cpp \
    EventLog.cpp \
    event_tempentity_tester.cpp \
    explode.cpp \
    filters.cpp \
    fire.cpp \
    fire_smoke.cpp \
    fish.cpp \
    fogcontroller.cpp \
    fourwheelvehiclephysics.cpp \
    func_areaportal.cpp \
    func_areaportalbase.cpp \
    func_areaportalwindow.cpp \
    func_break.cpp \
    func_breakablesurf.cpp \
    func_dust.cpp \
    func_ladder_endpoint.cpp \
    func_lod.cpp \
    func_movelinear.cpp \
    func_occluder.cpp \
    func_reflective_glass.cpp \
    func_smokevolume.cpp \
    game.cpp \
    gamedll_replay.cpp \
    gamehandle.cpp \
    gameinterface.cpp \
    GameStats_BasicStatsFunctions.cpp \
    gametrace_dll.cpp \
    gameweaponmanager.cpp \
    game_ui.cpp \
    genericactor.cpp \
    genericmonster.cpp \
    gib.cpp \
    globals.cpp \
    globalstate.cpp \
    grenadethrown.cpp \
    guntarget.cpp \
    hierarchy.cpp \
    hl2/ai_allymanager.cpp \
    hl2/ai_behavior_actbusy.cpp \
    hl2/ai_behavior_functank.cpp \
    hl2/ai_behavior_holster.cpp \
    hl2/ai_behavior_operator.cpp \
    hl2/ai_behavior_police.cpp \
    hl2/ai_goal_police.cpp \
    hl2/ai_spotlight.cpp \
    hl2/antlion_dust.cpp \
    hl2/antlion_maker.cpp \
    hl2/ar2_explosion.cpp \
    hl2/basehlcombatweapon.cpp \
    hl2/cbasehelicopter.cpp \
    hl2/cbasespriteprojectile.cpp \
    hl2/citadel_effects.cpp \
    hl2/combine_mine.cpp \
    hl2/env_alyxemp.cpp \
    hl2/env_headcrabcanister.cpp \
    hl2/env_speaker.cpp \
    hl2/env_starfield.cpp \
    hl2/func_bulletshield.cpp \
    hl2/Func_Monitor.cpp \
    hl2/func_recharge.cpp \
    hl2/func_tank.cpp \
    hl2/grenade_ar2.cpp \
    hl2/grenade_beam.cpp \
    hl2/grenade_bugbait.cpp \
    hl2/grenade_frag.cpp \
    hl2/grenade_homer.cpp \
    hl2/grenade_pathfollower.cpp \
    hl2/grenade_spit.cpp \
    hl2/hl2_ai_network.cpp \
    hl2/hl2_client.cpp \
    hl2/hl2_eventlog.cpp \
    hl2/hl2_player.cpp \
    hl2/hl2_playerlocaldata.cpp \
    hl2/hl2_triggers.cpp \
    hl2/hl_playermove.cpp \
    hl2/info_darknessmode_lightsource.cpp \
    hl2/info_teleporter_countdown.cpp \
    hl2/item_ammo.cpp \
    hl2/item_battery.cpp \
    hl2/item_dynamic_resupply.cpp \
    hl2/item_healthkit.cpp \
    hl2/item_itemcrate.cpp \
    hl2/item_suit.cpp \
    hl2/look_door.cpp \
    hl2/monster_dummy.cpp \
    hl2/npc_alyx_episodic.cpp \
    hl2/npc_antlion.cpp \
    hl2/npc_antliongrub.cpp \
    hl2/npc_antlionguard.cpp \
    hl2/npc_apcdriver.cpp \
    hl2/npc_attackchopper.cpp \
    hl2/npc_barnacle.cpp \
    hl2/npc_barney.cpp \
    hl2/npc_basescanner.cpp \
    hl2/npc_BaseZombie.cpp \
    hl2/npc_breen.cpp \
    hl2/npc_bullseye.cpp \
    hl2/npc_citizen17.cpp \
    hl2/npc_combine.cpp \
    hl2/npc_combinecamera.cpp \
    hl2/npc_combinedropship.cpp \
    hl2/npc_combinegunship.cpp \
    hl2/npc_combines.cpp \
    hl2/npc_cranedriver.cpp \
    hl2/npc_crow.cpp \
    hl2/npc_dog.cpp \
    hl2/npc_eli.cpp \
    hl2/npc_enemyfinder.cpp \
    hl2/npc_fastzombie.cpp \
    hl2/npc_fisherman.cpp \
    hl2/npc_gman.cpp \
    hl2/npc_headcrab.cpp \
    hl2/npc_ichthyosaur.cpp \
    hl2/npc_kleiner.cpp \
    hl2/npc_launcher.cpp \
    hl2/npc_manhack.cpp \
    hl2/npc_metropolice.cpp \
    hl2/npc_missiledefense.cpp \
    hl2/npc_monk.cpp \
    hl2/npc_mossman.cpp \
    hl2/npc_playercompanion.cpp \
    hl2/npc_PoisonZombie.cpp \
    hl2/npc_rollermine.cpp \
    hl2/npc_scanner.cpp \
    hl2/npc_stalker.cpp \
    hl2/npc_strider.cpp \
    hl2/npc_turret_ceiling.cpp \
    hl2/npc_turret_floor.cpp \
    hl2/npc_turret_ground.cpp \
    hl2/npc_vortigaunt_episodic.cpp \
    hl2/npc_zombie.cpp \
    hl2/npc_zombine.cpp \
    hl2/point_apc_controller.cpp \
    hl2/prop_combine_ball.cpp \
    hl2/prop_thumper.cpp \
    hl2/proto_sniper.cpp \
    hl2/rotorwash.cpp \
    hl2/script_intro.cpp \
    hl2/te_gaussexplosion.cpp \
    hl2/vehicle_airboat.cpp \
    hl2/vehicle_apc.cpp \
    hl2/vehicle_cannon.cpp \
    hl2/vehicle_crane.cpp \
    hl2/vehicle_jeep.cpp \
    hl2/vehicle_prisoner_pod.cpp \
    hl2/vehicle_viewcontroller.cpp \
    hl2/weapon_357.cpp \
    hl2/weapon_alyxgun.cpp \
    hl2/weapon_annabelle.cpp \
    hl2/weapon_ar2.cpp \
    hl2/weapon_bugbait.cpp \
    hl2/weapon_cguard.cpp \
    hl2/weapon_citizenpackage.cpp \
    hl2/weapon_crossbow.cpp \
    hl2/weapon_crowbar.cpp \
    hl2/weapon_flaregun.cpp \
    hl2/weapon_frag.cpp \
    hl2/weapon_physcannon.cpp \
    hl2/weapon_pistol.cpp \
    hl2/weapon_rpg.cpp \
    hl2/weapon_shotgun.cpp \
    hl2/weapon_smg1.cpp \
    hl2/weapon_stunstick.cpp \
    hltvdirector.cpp \
    h_ai.cpp \
    h_cycler.cpp \
    h_export.cpp \
    info_camera_link.cpp \
    info_overlay_accessor.cpp \
    init_factory.cpp \
    intermission.cpp \
    item_world.cpp \
    lightglow.cpp \
    lights.cpp \
    logicauto.cpp \
    logicentities.cpp \
    logicrelay.cpp \
    logic_achievement.cpp \
    logic_measure_movement.cpp \
    logic_navigation.cpp \
    mapentities.cpp \
    maprules.cpp \
    MaterialModifyControl.cpp \
    message_entity.cpp \
    modelentities.cpp \
    monstermaker.cpp \
    movehelper_server.cpp \
    movement.cpp \
    movie_explosion.cpp \
    ndebugoverlay.cpp \
    npc_talker.cpp \
    npc_vehicledriver.cpp \
    particle_fire.cpp \
    particle_light.cpp \
    particle_smokegrenade.cpp \
    particle_system.cpp \
    pathcorner.cpp \
    pathtrack.cpp \
    physconstraint.cpp \
    physics.cpp \
    physics_bone_follower.cpp \
    physics_cannister.cpp \
    physics_fx.cpp \
    physics_impact_damage.cpp \
    physics_main.cpp \
    physics_npc_solver.cpp \
    physics_prop_ragdoll.cpp \
    physobj.cpp \
    phys_controller.cpp \
    plasma.cpp \
    player.cpp \
    playerinfomanager.cpp \
    playerlocaldata.cpp \
    player_command.cpp \
    player_lagcompensation.cpp \
    player_pickup.cpp \
    player_resource.cpp \
    plugin_check.cpp \
    pointanglesensor.cpp \
    PointAngularVelocitySensor.cpp \
    pointhurt.cpp \
    pointteleport.cpp \
    point_camera.cpp \
    point_devshot_camera.cpp \
    point_playermoveconstraint.cpp \
    point_spotlight.cpp \
    point_template.cpp \
    props.cpp \
    RagdollBoogie.cpp \
    ragdoll_manager.cpp \
    recipientfilter.cpp \
    rope.cpp \
    saverestore_gamedll.cpp \
    sceneentity.cpp \
    scratchpad_gamedll_helpers.cpp \
    scripted.cpp \
    scriptedtarget.cpp \
    sendproxy.cpp \
    ServerNetworkProperty.cpp \
    shadowcontrol.cpp \
    SkyCamera.cpp \
    slideshow_display.cpp \
    smokestack.cpp \
    smoke_trail.cpp \
    sound.cpp \
    soundent.cpp \
    soundscape.cpp \
    soundscape_system.cpp \
    spotlightend.cpp \
    sprite_perfmonitor.cpp \
    stdafx.cpp \
    steamjet.cpp \
    subs.cpp \
    sun.cpp \
    tactical_mission.cpp \
    tanktrain.cpp \
    te.cpp \
    team.cpp \
    team_spawnpoint.cpp \
    TemplateEntities.cpp \
    tempmonster.cpp \
    tesla.cpp \
    testfunctions.cpp \
    testtraceline.cpp \
    test_proxytoggle.cpp \
    test_stressentities.cpp \
    textstatsmgr.cpp \
    te_armorricochet.cpp \
    te_basebeam.cpp \
    te_beamentpoint.cpp \
    te_beaments.cpp \
    te_beamfollow.cpp \
    te_beamlaser.cpp \
    te_beampoints.cpp \
    te_beamring.cpp \
    te_beamringpoint.cpp \
    te_beamspline.cpp \
    te_bloodsprite.cpp \
    te_bloodstream.cpp \
    te_breakmodel.cpp \
    te_bspdecal.cpp \
    te_bubbles.cpp \
    te_bubbletrail.cpp \
    te_clientprojectile.cpp \
    te_decal.cpp \
    te_dynamiclight.cpp \
    te_effect_dispatch.cpp \
    te_energysplash.cpp \
    te_explosion.cpp \
    te_fizz.cpp \
    te_footprintdecal.cpp \
    te_glassshatter.cpp \
    te_glowsprite.cpp \
    te_impact.cpp \
    te_killplayerattachments.cpp \
    te_largefunnel.cpp \
    te_muzzleflash.cpp \
    te_particlesystem.cpp \
    te_physicsprop.cpp \
    te_playerdecal.cpp \
    te_projecteddecal.cpp \
    te_showline.cpp \
    te_smoke.cpp \
    te_sparks.cpp \
    te_sprite.cpp \
    te_spritespray.cpp \
    te_worlddecal.cpp \
    timedeventmgr.cpp \
    toolframework_server.cpp \
    trains.cpp \
    triggers.cpp \
    util.cpp \
    variant_t.cpp \
    vehicle_base.cpp \
    vehicle_baseserver.cpp \
    vehicle_choreo_generic.cpp \
    vguiscreen.cpp \
    waterbullet.cpp \
    WaterLODControl.cpp \
    wcedit.cpp \
    weapon_cubemap.cpp \
    weight_button.cpp \
    world.cpp \


LIBFILES = \
    ../../lib/osx32/libtier0.dylib \
    ../../lib/osx32/libvstdlib.dylib \
    ../../lib/osx32/tier1.a \
    ../../lib/osx32/release/libprotobuf.a \
    ../../lib/osx32/choreoobjects.a \
    ../../lib/osx32/dmxloader.a \
    ../../lib/osx32/mathlib.a \
    ../../lib/osx32/particles.a \
    ../../lib/osx32/tier2.a \
    ../../lib/osx32/tier3.a \
    ../../lib/osx32/libsteam_api.dylib \


LIBFILENAMES = \
    ../../lib/osx32/choreoobjects.a \
    ../../lib/osx32/dmxloader.a \
    ../../lib/osx32/libsteam_api.dylib \
    ../../lib/osx32/libtier0.dylib \
    ../../lib/osx32/libvstdlib.dylib \
    ../../lib/osx32/mathlib.a \
    ../../lib/osx32/particles.a \
    ../../lib/osx32/release/libprotobuf.a \
    ../../lib/osx32/tier1.a \
    ../../lib/osx32/tier2.a \
    ../../lib/osx32/tier3.a \


# Include the base makefile now.
include $(SRCROOT)/devtools/makefile_base_posix.mak



OTHER_DEPENDENCIES = \


$(OBJ_DIR)/_other_deps.P : $(OTHER_DEPENDENCIES)
	$(GEN_OTHER_DEPS)

-include $(OBJ_DIR)/_other_deps.P



ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/language.P
endif

$(OBJ_DIR)/language.o : $(PWD)/../../common/language.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/randoverride.P
endif

$(OBJ_DIR)/randoverride.o : $(PWD)/../../common/randoverride.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/serverbenchmark_base.P
endif

$(OBJ_DIR)/serverbenchmark_base.o : $(PWD)/../../game/server/serverbenchmark_base.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/vote_controller.P
endif

$(OBJ_DIR)/vote_controller.o : $(PWD)/../../game/server/vote_controller.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/achievementmgr.P
endif

$(OBJ_DIR)/achievementmgr.o : $(PWD)/../../game/shared/achievementmgr.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/achievements_hlx.P
endif

$(OBJ_DIR)/achievements_hlx.o : $(PWD)/../../game/shared/achievements_hlx.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/achievement_saverestore.P
endif

$(OBJ_DIR)/achievement_saverestore.o : $(PWD)/../../game/shared/achievement_saverestore.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/activitylist.P
endif

$(OBJ_DIR)/activitylist.o : $(PWD)/../../game/shared/activitylist.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ammodef.P
endif

$(OBJ_DIR)/ammodef.o : $(PWD)/../../game/shared/ammodef.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/animation.P
endif

$(OBJ_DIR)/animation.o : $(PWD)/../../game/shared/animation.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/baseachievement.P
endif

$(OBJ_DIR)/baseachievement.o : $(PWD)/../../game/shared/baseachievement.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/basecombatcharacter_shared.P
endif

$(OBJ_DIR)/basecombatcharacter_shared.o : $(PWD)/../../game/shared/basecombatcharacter_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/basecombatweapon_shared.P
endif

$(OBJ_DIR)/basecombatweapon_shared.o : $(PWD)/../../game/shared/basecombatweapon_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/baseentity_shared.P
endif

$(OBJ_DIR)/baseentity_shared.o : $(PWD)/../../game/shared/baseentity_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/basegrenade_shared.P
endif

$(OBJ_DIR)/basegrenade_shared.o : $(PWD)/../../game/shared/basegrenade_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/baseparticleentity.P
endif

$(OBJ_DIR)/baseparticleentity.o : $(PWD)/../../game/shared/baseparticleentity.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/baseplayer_shared.P
endif

$(OBJ_DIR)/baseplayer_shared.o : $(PWD)/../../game/shared/baseplayer_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/baseviewmodel_shared.P
endif

$(OBJ_DIR)/baseviewmodel_shared.o : $(PWD)/../../game/shared/baseviewmodel_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/base_playeranimstate.P
endif

$(OBJ_DIR)/base_playeranimstate.o : $(PWD)/../../game/shared/base_playeranimstate.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/beam_shared.P
endif

$(OBJ_DIR)/beam_shared.o : $(PWD)/../../game/shared/beam_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/collisionproperty.P
endif

$(OBJ_DIR)/collisionproperty.o : $(PWD)/../../game/shared/collisionproperty.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/death_pose.P
endif

$(OBJ_DIR)/death_pose.o : $(PWD)/../../game/shared/death_pose.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/debugoverlay_shared.P
endif

$(OBJ_DIR)/debugoverlay_shared.o : $(PWD)/../../game/shared/debugoverlay_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/decals.P
endif

$(OBJ_DIR)/decals.o : $(PWD)/../../game/shared/decals.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/effect_dispatch_data.P
endif

$(OBJ_DIR)/effect_dispatch_data.o : $(PWD)/../../game/shared/effect_dispatch_data.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ehandle.P
endif

$(OBJ_DIR)/ehandle.o : $(PWD)/../../game/shared/ehandle.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/entitylist_base.P
endif

$(OBJ_DIR)/entitylist_base.o : $(PWD)/../../game/shared/entitylist_base.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/EntityParticleTrail_Shared.P
endif

$(OBJ_DIR)/EntityParticleTrail_Shared.o : $(PWD)/../../game/shared/EntityParticleTrail_Shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/env_detail_controller.P
endif

$(OBJ_DIR)/env_detail_controller.o : $(PWD)/../../game/shared/env_detail_controller.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/env_wind_shared.P
endif

$(OBJ_DIR)/env_wind_shared.o : $(PWD)/../../game/shared/env_wind_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/achievements_ep1.P
endif

$(OBJ_DIR)/achievements_ep1.o : $(PWD)/../../game/shared/episodic/achievements_ep1.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/achievements_ep2.P
endif

$(OBJ_DIR)/achievements_ep2.o : $(PWD)/../../game/shared/episodic/achievements_ep2.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/achievements_epx.P
endif

$(OBJ_DIR)/achievements_epx.o : $(PWD)/../../game/shared/episodic/achievements_epx.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/eventlist.P
endif

$(OBJ_DIR)/eventlist.o : $(PWD)/../../game/shared/eventlist.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/func_ladder.P
endif

$(OBJ_DIR)/func_ladder.o : $(PWD)/../../game/shared/func_ladder.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/gamemovement.P
endif

$(OBJ_DIR)/gamemovement.o : $(PWD)/../../game/shared/gamemovement.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/gamerules.P
endif

$(OBJ_DIR)/gamerules.o : $(PWD)/../../game/shared/gamerules.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/gamerules_register.P
endif

$(OBJ_DIR)/gamerules_register.o : $(PWD)/../../game/shared/gamerules_register.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/GameStats.P
endif

$(OBJ_DIR)/GameStats.o : $(PWD)/../../game/shared/GameStats.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/gamestringpool.P
endif

$(OBJ_DIR)/gamestringpool.o : $(PWD)/../../game/shared/gamestringpool.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/gamevars_shared.P
endif

$(OBJ_DIR)/gamevars_shared.o : $(PWD)/../../game/shared/gamevars_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/hintmessage.P
endif

$(OBJ_DIR)/hintmessage.o : $(PWD)/../../game/shared/hintmessage.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/hintsystem.P
endif

$(OBJ_DIR)/hintsystem.o : $(PWD)/../../game/shared/hintsystem.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/basehlcombatweapon_shared.P
endif

$(OBJ_DIR)/basehlcombatweapon_shared.o : $(PWD)/../../game/shared/hl2/basehlcombatweapon_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/env_headcrabcanister_shared.P
endif

$(OBJ_DIR)/env_headcrabcanister_shared.o : $(PWD)/../../game/shared/hl2/env_headcrabcanister_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/hl2_gamerules.P
endif

$(OBJ_DIR)/hl2_gamerules.o : $(PWD)/../../game/shared/hl2/hl2_gamerules.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/hl2_usermessages.P
endif

$(OBJ_DIR)/hl2_usermessages.o : $(PWD)/../../game/shared/hl2/hl2_usermessages.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/hl_gamemovement.P
endif

$(OBJ_DIR)/hl_gamemovement.o : $(PWD)/../../game/shared/hl2/hl_gamemovement.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/survival_gamerules.P
endif

$(OBJ_DIR)/survival_gamerules.o : $(PWD)/../../game/shared/hl2/survival_gamerules.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/igamesystem.P
endif

$(OBJ_DIR)/igamesystem.o : $(PWD)/../../game/shared/igamesystem.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/interval.P
endif

$(OBJ_DIR)/interval.o : $(PWD)/../../game/shared/interval.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/mapentities_shared.P
endif

$(OBJ_DIR)/mapentities_shared.o : $(PWD)/../../game/shared/mapentities_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ModelSoundsCache.P
endif

$(OBJ_DIR)/ModelSoundsCache.o : $(PWD)/../../game/shared/ModelSoundsCache.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/movevars_shared.P
endif

$(OBJ_DIR)/movevars_shared.o : $(PWD)/../../game/shared/movevars_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/mp_shareddefs.P
endif

$(OBJ_DIR)/mp_shareddefs.o : $(PWD)/../../game/shared/mp_shareddefs.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/multiplay_gamerules.P
endif

$(OBJ_DIR)/multiplay_gamerules.o : $(PWD)/../../game/shared/multiplay_gamerules.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/obstacle_pushaway.P
endif

$(OBJ_DIR)/obstacle_pushaway.o : $(PWD)/../../game/shared/obstacle_pushaway.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/particlesystemquery.P
endif

$(OBJ_DIR)/particlesystemquery.o : $(PWD)/../../game/shared/particlesystemquery.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/particle_parse.P
endif

$(OBJ_DIR)/particle_parse.o : $(PWD)/../../game/shared/particle_parse.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/physics_main_shared.P
endif

$(OBJ_DIR)/physics_main_shared.o : $(PWD)/../../game/shared/physics_main_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/physics_saverestore.P
endif

$(OBJ_DIR)/physics_saverestore.o : $(PWD)/../../game/shared/physics_saverestore.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/physics_shared.P
endif

$(OBJ_DIR)/physics_shared.o : $(PWD)/../../game/shared/physics_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/point_bonusmaps_accessor.P
endif

$(OBJ_DIR)/point_bonusmaps_accessor.o : $(PWD)/../../game/shared/point_bonusmaps_accessor.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/point_posecontroller.P
endif

$(OBJ_DIR)/point_posecontroller.o : $(PWD)/../../game/shared/point_posecontroller.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/precache_register.P
endif

$(OBJ_DIR)/precache_register.o : $(PWD)/../../game/shared/precache_register.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/predictableid.P
endif

$(OBJ_DIR)/predictableid.o : $(PWD)/../../game/shared/predictableid.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/props_shared.P
endif

$(OBJ_DIR)/props_shared.o : $(PWD)/../../game/shared/props_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/querycache.P
endif

$(OBJ_DIR)/querycache.o : $(PWD)/../../game/shared/querycache.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ragdoll_shared.P
endif

$(OBJ_DIR)/ragdoll_shared.o : $(PWD)/../../game/shared/ragdoll_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/rope_helpers.P
endif

$(OBJ_DIR)/rope_helpers.o : $(PWD)/../../game/shared/rope_helpers.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/saverestore.P
endif

$(OBJ_DIR)/saverestore.o : $(PWD)/../../game/shared/saverestore.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/SceneCache.P
endif

$(OBJ_DIR)/SceneCache.o : $(PWD)/../../game/shared/SceneCache.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/sceneentity_shared.P
endif

$(OBJ_DIR)/sceneentity_shared.o : $(PWD)/../../game/shared/sceneentity_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/script_intro_shared.P
endif

$(OBJ_DIR)/script_intro_shared.o : $(PWD)/../../game/shared/script_intro_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/sequence_Transitioner.P
endif

$(OBJ_DIR)/sequence_Transitioner.o : $(PWD)/../../game/shared/sequence_Transitioner.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/sheetsimulator.P
endif

$(OBJ_DIR)/sheetsimulator.o : $(PWD)/../../game/shared/sheetsimulator.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/simtimer.P
endif

$(OBJ_DIR)/simtimer.o : $(PWD)/../../game/shared/simtimer.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/singleplay_gamerules.P
endif

$(OBJ_DIR)/singleplay_gamerules.o : $(PWD)/../../game/shared/singleplay_gamerules.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/SoundEmitterSystem.P
endif

$(OBJ_DIR)/SoundEmitterSystem.o : $(PWD)/../../game/shared/SoundEmitterSystem.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/soundenvelope.P
endif

$(OBJ_DIR)/soundenvelope.o : $(PWD)/../../game/shared/soundenvelope.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/Sprite.P
endif

$(OBJ_DIR)/Sprite.o : $(PWD)/../../game/shared/Sprite.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/SpriteTrail.P
endif

$(OBJ_DIR)/SpriteTrail.o : $(PWD)/../../game/shared/SpriteTrail.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/studio_shared.P
endif

$(OBJ_DIR)/studio_shared.o : $(PWD)/../../game/shared/studio_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/takedamageinfo.P
endif

$(OBJ_DIR)/takedamageinfo.o : $(PWD)/../../game/shared/takedamageinfo.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/teamplay_gamerules.P
endif

$(OBJ_DIR)/teamplay_gamerules.o : $(PWD)/../../game/shared/teamplay_gamerules.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/test_ehandle.P
endif

$(OBJ_DIR)/test_ehandle.o : $(PWD)/../../game/shared/test_ehandle.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/usercmd.P
endif

$(OBJ_DIR)/usercmd.o : $(PWD)/../../game/shared/usercmd.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/usermessages.P
endif

$(OBJ_DIR)/usermessages.o : $(PWD)/../../game/shared/usermessages.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/util_shared.P
endif

$(OBJ_DIR)/util_shared.o : $(PWD)/../../game/shared/util_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/vehicle_viewblend_shared.P
endif

$(OBJ_DIR)/vehicle_viewblend_shared.o : $(PWD)/../../game/shared/vehicle_viewblend_shared.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/voice_gamemgr.P
endif

$(OBJ_DIR)/voice_gamemgr.o : $(PWD)/../../game/shared/voice_gamemgr.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_parse.P
endif

$(OBJ_DIR)/weapon_parse.o : $(PWD)/../../game/shared/weapon_parse.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_parse_default.P
endif

$(OBJ_DIR)/weapon_parse_default.o : $(PWD)/../../game/shared/weapon_parse_default.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_proficiency.P
endif

$(OBJ_DIR)/weapon_proficiency.o : $(PWD)/../../game/shared/weapon_proficiency.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/bone_setup.P
endif

$(OBJ_DIR)/bone_setup.o : $(PWD)/../../public/bone_setup.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/collisionutils.P
endif

$(OBJ_DIR)/collisionutils.o : $(PWD)/../../public/collisionutils.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/dt_send.P
endif

$(OBJ_DIR)/dt_send.o : $(PWD)/../../public/dt_send.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/dt_utlvector_common.P
endif

$(OBJ_DIR)/dt_utlvector_common.o : $(PWD)/../../public/dt_utlvector_common.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/dt_utlvector_send.P
endif

$(OBJ_DIR)/dt_utlvector_send.o : $(PWD)/../../public/dt_utlvector_send.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/editor_sendcommand.P
endif

$(OBJ_DIR)/editor_sendcommand.o : $(PWD)/../../public/editor_sendcommand.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/filesystem_helpers.P
endif

$(OBJ_DIR)/filesystem_helpers.o : $(PWD)/../../public/filesystem_helpers.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/haptic_msgs.P
endif

$(OBJ_DIR)/haptic_msgs.o : $(PWD)/../../public/haptics/haptic_msgs.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/interpolatortypes.P
endif

$(OBJ_DIR)/interpolatortypes.o : $(PWD)/../../public/interpolatortypes.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/keyframe.P
endif

$(OBJ_DIR)/keyframe.o : $(PWD)/../../public/keyframe/keyframe.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/map_utils.P
endif

$(OBJ_DIR)/map_utils.o : $(PWD)/../../public/map_utils.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/networkvar.P
endif

$(OBJ_DIR)/networkvar.o : $(PWD)/../../public/networkvar.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/registry.P
endif

$(OBJ_DIR)/registry.o : $(PWD)/../../public/registry.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/rope_physics.P
endif

$(OBJ_DIR)/rope_physics.o : $(PWD)/../../public/rope_physics.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/scratchpad3d.P
endif

$(OBJ_DIR)/scratchpad3d.o : $(PWD)/../../public/scratchpad3d.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ScratchPadUtils.P
endif

$(OBJ_DIR)/ScratchPadUtils.o : $(PWD)/../../public/ScratchPadUtils.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/server_class.P
endif

$(OBJ_DIR)/server_class.o : $(PWD)/../../public/server_class.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/simple_physics.P
endif

$(OBJ_DIR)/simple_physics.o : $(PWD)/../../public/simple_physics.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/SoundParametersInternal.P
endif

$(OBJ_DIR)/SoundParametersInternal.o : $(PWD)/../../public/SoundParametersInternal.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/stringregistry.P
endif

$(OBJ_DIR)/stringregistry.o : $(PWD)/../../public/stringregistry.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/studio.P
endif

$(OBJ_DIR)/studio.o : $(PWD)/../../public/studio.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/memoverride.P
endif

$(OBJ_DIR)/memoverride.o : $(PWD)/../../public/tier0/memoverride.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_activity.P
endif

$(OBJ_DIR)/ai_activity.o : $(PWD)/ai_activity.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_baseactor.P
endif

$(OBJ_DIR)/ai_baseactor.o : $(PWD)/ai_baseactor.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_basehumanoid.P
endif

$(OBJ_DIR)/ai_basehumanoid.o : $(PWD)/ai_basehumanoid.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_basenpc.P
endif

$(OBJ_DIR)/ai_basenpc.o : $(PWD)/ai_basenpc.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_basenpc_flyer.P
endif

$(OBJ_DIR)/ai_basenpc_flyer.o : $(PWD)/ai_basenpc_flyer.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_basenpc_flyer_new.P
endif

$(OBJ_DIR)/ai_basenpc_flyer_new.o : $(PWD)/ai_basenpc_flyer_new.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_basenpc_movement.P
endif

$(OBJ_DIR)/ai_basenpc_movement.o : $(PWD)/ai_basenpc_movement.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_basenpc_physicsflyer.P
endif

$(OBJ_DIR)/ai_basenpc_physicsflyer.o : $(PWD)/ai_basenpc_physicsflyer.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_basenpc_schedule.P
endif

$(OBJ_DIR)/ai_basenpc_schedule.o : $(PWD)/ai_basenpc_schedule.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_basenpc_squad.P
endif

$(OBJ_DIR)/ai_basenpc_squad.o : $(PWD)/ai_basenpc_squad.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_behavior.P
endif

$(OBJ_DIR)/ai_behavior.o : $(PWD)/ai_behavior.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_behavior_assault.P
endif

$(OBJ_DIR)/ai_behavior_assault.o : $(PWD)/ai_behavior_assault.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_behavior_fear.P
endif

$(OBJ_DIR)/ai_behavior_fear.o : $(PWD)/ai_behavior_fear.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_behavior_follow.P
endif

$(OBJ_DIR)/ai_behavior_follow.o : $(PWD)/ai_behavior_follow.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_behavior_lead.P
endif

$(OBJ_DIR)/ai_behavior_lead.o : $(PWD)/ai_behavior_lead.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_behavior_passenger.P
endif

$(OBJ_DIR)/ai_behavior_passenger.o : $(PWD)/ai_behavior_passenger.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_behavior_rappel.P
endif

$(OBJ_DIR)/ai_behavior_rappel.o : $(PWD)/ai_behavior_rappel.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_behavior_standoff.P
endif

$(OBJ_DIR)/ai_behavior_standoff.o : $(PWD)/ai_behavior_standoff.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_blended_movement.P
endif

$(OBJ_DIR)/ai_blended_movement.o : $(PWD)/ai_blended_movement.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_concommands.P
endif

$(OBJ_DIR)/ai_concommands.o : $(PWD)/ai_concommands.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_condition.P
endif

$(OBJ_DIR)/ai_condition.o : $(PWD)/ai_condition.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/AI_Criteria.P
endif

$(OBJ_DIR)/AI_Criteria.o : $(PWD)/AI_Criteria.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_default.P
endif

$(OBJ_DIR)/ai_default.o : $(PWD)/ai_default.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_dynamiclink.P
endif

$(OBJ_DIR)/ai_dynamiclink.o : $(PWD)/ai_dynamiclink.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_event.P
endif

$(OBJ_DIR)/ai_event.o : $(PWD)/ai_event.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_eventresponse.P
endif

$(OBJ_DIR)/ai_eventresponse.o : $(PWD)/ai_eventresponse.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_goalentity.P
endif

$(OBJ_DIR)/ai_goalentity.o : $(PWD)/ai_goalentity.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_hint.P
endif

$(OBJ_DIR)/ai_hint.o : $(PWD)/ai_hint.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_hull.P
endif

$(OBJ_DIR)/ai_hull.o : $(PWD)/ai_hull.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_initutils.P
endif

$(OBJ_DIR)/ai_initutils.o : $(PWD)/ai_initutils.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/AI_Interest_Target.P
endif

$(OBJ_DIR)/AI_Interest_Target.o : $(PWD)/AI_Interest_Target.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_link.P
endif

$(OBJ_DIR)/ai_link.o : $(PWD)/ai_link.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_localnavigator.P
endif

$(OBJ_DIR)/ai_localnavigator.o : $(PWD)/ai_localnavigator.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_looktarget.P
endif

$(OBJ_DIR)/ai_looktarget.o : $(PWD)/ai_looktarget.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_memory.P
endif

$(OBJ_DIR)/ai_memory.o : $(PWD)/ai_memory.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_motor.P
endif

$(OBJ_DIR)/ai_motor.o : $(PWD)/ai_motor.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_moveprobe.P
endif

$(OBJ_DIR)/ai_moveprobe.o : $(PWD)/ai_moveprobe.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_moveshoot.P
endif

$(OBJ_DIR)/ai_moveshoot.o : $(PWD)/ai_moveshoot.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_movesolver.P
endif

$(OBJ_DIR)/ai_movesolver.o : $(PWD)/ai_movesolver.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_namespaces.P
endif

$(OBJ_DIR)/ai_namespaces.o : $(PWD)/ai_namespaces.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_navigator.P
endif

$(OBJ_DIR)/ai_navigator.o : $(PWD)/ai_navigator.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_network.P
endif

$(OBJ_DIR)/ai_network.o : $(PWD)/ai_network.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_networkmanager.P
endif

$(OBJ_DIR)/ai_networkmanager.o : $(PWD)/ai_networkmanager.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_node.P
endif

$(OBJ_DIR)/ai_node.o : $(PWD)/ai_node.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_pathfinder.P
endif

$(OBJ_DIR)/ai_pathfinder.o : $(PWD)/ai_pathfinder.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_planesolver.P
endif

$(OBJ_DIR)/ai_planesolver.o : $(PWD)/ai_planesolver.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_playerally.P
endif

$(OBJ_DIR)/ai_playerally.o : $(PWD)/ai_playerally.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_relationship.P
endif

$(OBJ_DIR)/ai_relationship.o : $(PWD)/ai_relationship.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/AI_ResponseSystem.P
endif

$(OBJ_DIR)/AI_ResponseSystem.o : $(PWD)/AI_ResponseSystem.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_route.P
endif

$(OBJ_DIR)/ai_route.o : $(PWD)/ai_route.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_saverestore.P
endif

$(OBJ_DIR)/ai_saverestore.o : $(PWD)/ai_saverestore.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_schedule.P
endif

$(OBJ_DIR)/ai_schedule.o : $(PWD)/ai_schedule.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_scriptconditions.P
endif

$(OBJ_DIR)/ai_scriptconditions.o : $(PWD)/ai_scriptconditions.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_senses.P
endif

$(OBJ_DIR)/ai_senses.o : $(PWD)/ai_senses.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_sentence.P
endif

$(OBJ_DIR)/ai_sentence.o : $(PWD)/ai_sentence.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_speech.P
endif

$(OBJ_DIR)/ai_speech.o : $(PWD)/ai_speech.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_speechfilter.P
endif

$(OBJ_DIR)/ai_speechfilter.o : $(PWD)/ai_speechfilter.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_squad.P
endif

$(OBJ_DIR)/ai_squad.o : $(PWD)/ai_squad.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_squadslot.P
endif

$(OBJ_DIR)/ai_squadslot.o : $(PWD)/ai_squadslot.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_tacticalservices.P
endif

$(OBJ_DIR)/ai_tacticalservices.o : $(PWD)/ai_tacticalservices.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_task.P
endif

$(OBJ_DIR)/ai_task.o : $(PWD)/ai_task.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_trackpather.P
endif

$(OBJ_DIR)/ai_trackpather.o : $(PWD)/ai_trackpather.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_utils.P
endif

$(OBJ_DIR)/ai_utils.o : $(PWD)/ai_utils.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_waypoint.P
endif

$(OBJ_DIR)/ai_waypoint.o : $(PWD)/ai_waypoint.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/baseanimating.P
endif

$(OBJ_DIR)/baseanimating.o : $(PWD)/baseanimating.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/BaseAnimatingOverlay.P
endif

$(OBJ_DIR)/BaseAnimatingOverlay.o : $(PWD)/BaseAnimatingOverlay.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/basebludgeonweapon.P
endif

$(OBJ_DIR)/basebludgeonweapon.o : $(PWD)/basebludgeonweapon.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/basecombatcharacter.P
endif

$(OBJ_DIR)/basecombatcharacter.o : $(PWD)/basecombatcharacter.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/basecombatweapon.P
endif

$(OBJ_DIR)/basecombatweapon.o : $(PWD)/basecombatweapon.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/baseentity.P
endif

$(OBJ_DIR)/baseentity.o : $(PWD)/baseentity.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/baseflex.P
endif

$(OBJ_DIR)/baseflex.o : $(PWD)/baseflex.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/basegrenade_concussion.P
endif

$(OBJ_DIR)/basegrenade_concussion.o : $(PWD)/basegrenade_concussion.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/basegrenade_contact.P
endif

$(OBJ_DIR)/basegrenade_contact.o : $(PWD)/basegrenade_contact.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/basegrenade_timed.P
endif

$(OBJ_DIR)/basegrenade_timed.o : $(PWD)/basegrenade_timed.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/basemultiplayerplayer.P
endif

$(OBJ_DIR)/basemultiplayerplayer.o : $(PWD)/basemultiplayerplayer.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/basetempentity.P
endif

$(OBJ_DIR)/basetempentity.o : $(PWD)/basetempentity.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/baseviewmodel.P
endif

$(OBJ_DIR)/baseviewmodel.o : $(PWD)/baseviewmodel.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/base_gameinterface.P
endif

$(OBJ_DIR)/base_gameinterface.o : $(PWD)/base_gameinterface.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/base_transmit_proxy.P
endif

$(OBJ_DIR)/base_transmit_proxy.o : $(PWD)/base_transmit_proxy.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/bitstring.P
endif

$(OBJ_DIR)/bitstring.o : $(PWD)/bitstring.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/bmodels.P
endif

$(OBJ_DIR)/bmodels.o : $(PWD)/bmodels.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/buttons.P
endif

$(OBJ_DIR)/buttons.o : $(PWD)/buttons.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/cbase.P
endif

$(OBJ_DIR)/cbase.o : $(PWD)/cbase.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/client.P
endif

$(OBJ_DIR)/client.o : $(PWD)/client.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/colorcorrection.P
endif

$(OBJ_DIR)/colorcorrection.o : $(PWD)/colorcorrection.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/colorcorrectionvolume.P
endif

$(OBJ_DIR)/colorcorrectionvolume.o : $(PWD)/colorcorrectionvolume.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/CommentarySystem.P
endif

$(OBJ_DIR)/CommentarySystem.o : $(PWD)/CommentarySystem.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/controlentities.P
endif

$(OBJ_DIR)/controlentities.o : $(PWD)/controlentities.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/cplane.P
endif

$(OBJ_DIR)/cplane.o : $(PWD)/cplane.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/CRagdollMagnet.P
endif

$(OBJ_DIR)/CRagdollMagnet.o : $(PWD)/CRagdollMagnet.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/damagemodifier.P
endif

$(OBJ_DIR)/damagemodifier.o : $(PWD)/damagemodifier.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/doors.P
endif

$(OBJ_DIR)/doors.o : $(PWD)/doors.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/dynamiclight.P
endif

$(OBJ_DIR)/dynamiclight.o : $(PWD)/dynamiclight.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/effects.P
endif

$(OBJ_DIR)/effects.o : $(PWD)/effects.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/EffectsServer.P
endif

$(OBJ_DIR)/EffectsServer.o : $(PWD)/EffectsServer.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/entityblocker.P
endif

$(OBJ_DIR)/entityblocker.o : $(PWD)/entityblocker.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/EntityDissolve.P
endif

$(OBJ_DIR)/EntityDissolve.o : $(PWD)/EntityDissolve.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/EntityFlame.P
endif

$(OBJ_DIR)/EntityFlame.o : $(PWD)/EntityFlame.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/entitylist.P
endif

$(OBJ_DIR)/entitylist.o : $(PWD)/entitylist.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/EntityParticleTrail.P
endif

$(OBJ_DIR)/EntityParticleTrail.o : $(PWD)/EntityParticleTrail.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/entity_tools_server.P
endif

$(OBJ_DIR)/entity_tools_server.o : $(PWD)/entity_tools_server.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/EnvBeam.P
endif

$(OBJ_DIR)/EnvBeam.o : $(PWD)/EnvBeam.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/EnvFade.P
endif

$(OBJ_DIR)/EnvFade.o : $(PWD)/EnvFade.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/EnvHudHint.P
endif

$(OBJ_DIR)/EnvHudHint.o : $(PWD)/EnvHudHint.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/EnvLaser.P
endif

$(OBJ_DIR)/EnvLaser.o : $(PWD)/EnvLaser.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/EnvMessage.P
endif

$(OBJ_DIR)/EnvMessage.o : $(PWD)/EnvMessage.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/envmicrophone.P
endif

$(OBJ_DIR)/envmicrophone.o : $(PWD)/envmicrophone.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/EnvShake.P
endif

$(OBJ_DIR)/EnvShake.o : $(PWD)/EnvShake.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/EnvSpark.P
endif

$(OBJ_DIR)/EnvSpark.o : $(PWD)/EnvSpark.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/env_debughistory.P
endif

$(OBJ_DIR)/env_debughistory.o : $(PWD)/env_debughistory.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/env_effectsscript.P
endif

$(OBJ_DIR)/env_effectsscript.o : $(PWD)/env_effectsscript.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/env_entity_maker.P
endif

$(OBJ_DIR)/env_entity_maker.o : $(PWD)/env_entity_maker.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/env_particlescript.P
endif

$(OBJ_DIR)/env_particlescript.o : $(PWD)/env_particlescript.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/env_player_surface_trigger.P
endif

$(OBJ_DIR)/env_player_surface_trigger.o : $(PWD)/env_player_surface_trigger.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/env_projectedtexture.P
endif

$(OBJ_DIR)/env_projectedtexture.o : $(PWD)/env_projectedtexture.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/env_screenoverlay.P
endif

$(OBJ_DIR)/env_screenoverlay.o : $(PWD)/env_screenoverlay.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/env_texturetoggle.P
endif

$(OBJ_DIR)/env_texturetoggle.o : $(PWD)/env_texturetoggle.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/env_tonemap_controller.P
endif

$(OBJ_DIR)/env_tonemap_controller.o : $(PWD)/env_tonemap_controller.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/env_zoom.P
endif

$(OBJ_DIR)/env_zoom.o : $(PWD)/env_zoom.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_behavior_alyx_injured.P
endif

$(OBJ_DIR)/ai_behavior_alyx_injured.o : $(PWD)/episodic/ai_behavior_alyx_injured.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_behavior_passenger_companion.P
endif

$(OBJ_DIR)/ai_behavior_passenger_companion.o : $(PWD)/episodic/ai_behavior_passenger_companion.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_behavior_passenger_zombie.P
endif

$(OBJ_DIR)/ai_behavior_passenger_zombie.o : $(PWD)/episodic/ai_behavior_passenger_zombie.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ep1_gamestats.P
endif

$(OBJ_DIR)/ep1_gamestats.o : $(PWD)/episodic/ep1_gamestats.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ep2_gamestats.P
endif

$(OBJ_DIR)/ep2_gamestats.o : $(PWD)/episodic/ep2_gamestats.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_advisor.P
endif

$(OBJ_DIR)/npc_advisor.o : $(PWD)/episodic/npc_advisor.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_combine_cannon.P
endif

$(OBJ_DIR)/npc_combine_cannon.o : $(PWD)/episodic/npc_combine_cannon.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_hunter.P
endif

$(OBJ_DIR)/npc_hunter.o : $(PWD)/episodic/npc_hunter.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_magnusson.P
endif

$(OBJ_DIR)/npc_magnusson.o : $(PWD)/episodic/npc_magnusson.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_puppet.P
endif

$(OBJ_DIR)/npc_puppet.o : $(PWD)/episodic/npc_puppet.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/prop_scalable.P
endif

$(OBJ_DIR)/prop_scalable.o : $(PWD)/episodic/prop_scalable.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/vehicle_jeep_episodic.P
endif

$(OBJ_DIR)/vehicle_jeep_episodic.o : $(PWD)/episodic/vehicle_jeep_episodic.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_striderbuster.P
endif

$(OBJ_DIR)/weapon_striderbuster.o : $(PWD)/episodic/weapon_striderbuster.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/EventLog.P
endif

$(OBJ_DIR)/EventLog.o : $(PWD)/EventLog.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/event_tempentity_tester.P
endif

$(OBJ_DIR)/event_tempentity_tester.o : $(PWD)/event_tempentity_tester.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/explode.P
endif

$(OBJ_DIR)/explode.o : $(PWD)/explode.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/filters.P
endif

$(OBJ_DIR)/filters.o : $(PWD)/filters.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/fire.P
endif

$(OBJ_DIR)/fire.o : $(PWD)/fire.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/fire_smoke.P
endif

$(OBJ_DIR)/fire_smoke.o : $(PWD)/fire_smoke.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/fish.P
endif

$(OBJ_DIR)/fish.o : $(PWD)/fish.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/fogcontroller.P
endif

$(OBJ_DIR)/fogcontroller.o : $(PWD)/fogcontroller.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/fourwheelvehiclephysics.P
endif

$(OBJ_DIR)/fourwheelvehiclephysics.o : $(PWD)/fourwheelvehiclephysics.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/func_areaportal.P
endif

$(OBJ_DIR)/func_areaportal.o : $(PWD)/func_areaportal.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/func_areaportalbase.P
endif

$(OBJ_DIR)/func_areaportalbase.o : $(PWD)/func_areaportalbase.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/func_areaportalwindow.P
endif

$(OBJ_DIR)/func_areaportalwindow.o : $(PWD)/func_areaportalwindow.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/func_break.P
endif

$(OBJ_DIR)/func_break.o : $(PWD)/func_break.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/func_breakablesurf.P
endif

$(OBJ_DIR)/func_breakablesurf.o : $(PWD)/func_breakablesurf.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/func_dust.P
endif

$(OBJ_DIR)/func_dust.o : $(PWD)/func_dust.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/func_ladder_endpoint.P
endif

$(OBJ_DIR)/func_ladder_endpoint.o : $(PWD)/func_ladder_endpoint.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/func_lod.P
endif

$(OBJ_DIR)/func_lod.o : $(PWD)/func_lod.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/func_movelinear.P
endif

$(OBJ_DIR)/func_movelinear.o : $(PWD)/func_movelinear.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/func_occluder.P
endif

$(OBJ_DIR)/func_occluder.o : $(PWD)/func_occluder.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/func_reflective_glass.P
endif

$(OBJ_DIR)/func_reflective_glass.o : $(PWD)/func_reflective_glass.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/func_smokevolume.P
endif

$(OBJ_DIR)/func_smokevolume.o : $(PWD)/func_smokevolume.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/game.P
endif

$(OBJ_DIR)/game.o : $(PWD)/game.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/gamedll_replay.P
endif

$(OBJ_DIR)/gamedll_replay.o : $(PWD)/gamedll_replay.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/gamehandle.P
endif

$(OBJ_DIR)/gamehandle.o : $(PWD)/gamehandle.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/gameinterface.P
endif

$(OBJ_DIR)/gameinterface.o : $(PWD)/gameinterface.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/GameStats_BasicStatsFunctions.P
endif

$(OBJ_DIR)/GameStats_BasicStatsFunctions.o : $(PWD)/GameStats_BasicStatsFunctions.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/gametrace_dll.P
endif

$(OBJ_DIR)/gametrace_dll.o : $(PWD)/gametrace_dll.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/gameweaponmanager.P
endif

$(OBJ_DIR)/gameweaponmanager.o : $(PWD)/gameweaponmanager.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/game_ui.P
endif

$(OBJ_DIR)/game_ui.o : $(PWD)/game_ui.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/genericactor.P
endif

$(OBJ_DIR)/genericactor.o : $(PWD)/genericactor.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/genericmonster.P
endif

$(OBJ_DIR)/genericmonster.o : $(PWD)/genericmonster.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/gib.P
endif

$(OBJ_DIR)/gib.o : $(PWD)/gib.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/globals.P
endif

$(OBJ_DIR)/globals.o : $(PWD)/globals.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/globalstate.P
endif

$(OBJ_DIR)/globalstate.o : $(PWD)/globalstate.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/grenadethrown.P
endif

$(OBJ_DIR)/grenadethrown.o : $(PWD)/grenadethrown.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/guntarget.P
endif

$(OBJ_DIR)/guntarget.o : $(PWD)/guntarget.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/hierarchy.P
endif

$(OBJ_DIR)/hierarchy.o : $(PWD)/hierarchy.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_allymanager.P
endif

$(OBJ_DIR)/ai_allymanager.o : $(PWD)/hl2/ai_allymanager.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_behavior_actbusy.P
endif

$(OBJ_DIR)/ai_behavior_actbusy.o : $(PWD)/hl2/ai_behavior_actbusy.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_behavior_functank.P
endif

$(OBJ_DIR)/ai_behavior_functank.o : $(PWD)/hl2/ai_behavior_functank.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_behavior_holster.P
endif

$(OBJ_DIR)/ai_behavior_holster.o : $(PWD)/hl2/ai_behavior_holster.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_behavior_operator.P
endif

$(OBJ_DIR)/ai_behavior_operator.o : $(PWD)/hl2/ai_behavior_operator.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_behavior_police.P
endif

$(OBJ_DIR)/ai_behavior_police.o : $(PWD)/hl2/ai_behavior_police.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_goal_police.P
endif

$(OBJ_DIR)/ai_goal_police.o : $(PWD)/hl2/ai_goal_police.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ai_spotlight.P
endif

$(OBJ_DIR)/ai_spotlight.o : $(PWD)/hl2/ai_spotlight.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/antlion_dust.P
endif

$(OBJ_DIR)/antlion_dust.o : $(PWD)/hl2/antlion_dust.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/antlion_maker.P
endif

$(OBJ_DIR)/antlion_maker.o : $(PWD)/hl2/antlion_maker.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ar2_explosion.P
endif

$(OBJ_DIR)/ar2_explosion.o : $(PWD)/hl2/ar2_explosion.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/basehlcombatweapon.P
endif

$(OBJ_DIR)/basehlcombatweapon.o : $(PWD)/hl2/basehlcombatweapon.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/cbasehelicopter.P
endif

$(OBJ_DIR)/cbasehelicopter.o : $(PWD)/hl2/cbasehelicopter.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/cbasespriteprojectile.P
endif

$(OBJ_DIR)/cbasespriteprojectile.o : $(PWD)/hl2/cbasespriteprojectile.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/citadel_effects.P
endif

$(OBJ_DIR)/citadel_effects.o : $(PWD)/hl2/citadel_effects.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/combine_mine.P
endif

$(OBJ_DIR)/combine_mine.o : $(PWD)/hl2/combine_mine.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/env_alyxemp.P
endif

$(OBJ_DIR)/env_alyxemp.o : $(PWD)/hl2/env_alyxemp.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/env_headcrabcanister.P
endif

$(OBJ_DIR)/env_headcrabcanister.o : $(PWD)/hl2/env_headcrabcanister.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/env_speaker.P
endif

$(OBJ_DIR)/env_speaker.o : $(PWD)/hl2/env_speaker.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/env_starfield.P
endif

$(OBJ_DIR)/env_starfield.o : $(PWD)/hl2/env_starfield.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/func_bulletshield.P
endif

$(OBJ_DIR)/func_bulletshield.o : $(PWD)/hl2/func_bulletshield.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/Func_Monitor.P
endif

$(OBJ_DIR)/Func_Monitor.o : $(PWD)/hl2/Func_Monitor.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/func_recharge.P
endif

$(OBJ_DIR)/func_recharge.o : $(PWD)/hl2/func_recharge.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/func_tank.P
endif

$(OBJ_DIR)/func_tank.o : $(PWD)/hl2/func_tank.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/grenade_ar2.P
endif

$(OBJ_DIR)/grenade_ar2.o : $(PWD)/hl2/grenade_ar2.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/grenade_beam.P
endif

$(OBJ_DIR)/grenade_beam.o : $(PWD)/hl2/grenade_beam.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/grenade_bugbait.P
endif

$(OBJ_DIR)/grenade_bugbait.o : $(PWD)/hl2/grenade_bugbait.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/grenade_frag.P
endif

$(OBJ_DIR)/grenade_frag.o : $(PWD)/hl2/grenade_frag.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/grenade_homer.P
endif

$(OBJ_DIR)/grenade_homer.o : $(PWD)/hl2/grenade_homer.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/grenade_pathfollower.P
endif

$(OBJ_DIR)/grenade_pathfollower.o : $(PWD)/hl2/grenade_pathfollower.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/grenade_spit.P
endif

$(OBJ_DIR)/grenade_spit.o : $(PWD)/hl2/grenade_spit.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/hl2_ai_network.P
endif

$(OBJ_DIR)/hl2_ai_network.o : $(PWD)/hl2/hl2_ai_network.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/hl2_client.P
endif

$(OBJ_DIR)/hl2_client.o : $(PWD)/hl2/hl2_client.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/hl2_eventlog.P
endif

$(OBJ_DIR)/hl2_eventlog.o : $(PWD)/hl2/hl2_eventlog.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/hl2_player.P
endif

$(OBJ_DIR)/hl2_player.o : $(PWD)/hl2/hl2_player.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/hl2_playerlocaldata.P
endif

$(OBJ_DIR)/hl2_playerlocaldata.o : $(PWD)/hl2/hl2_playerlocaldata.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/hl2_triggers.P
endif

$(OBJ_DIR)/hl2_triggers.o : $(PWD)/hl2/hl2_triggers.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/hl_playermove.P
endif

$(OBJ_DIR)/hl_playermove.o : $(PWD)/hl2/hl_playermove.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/info_darknessmode_lightsource.P
endif

$(OBJ_DIR)/info_darknessmode_lightsource.o : $(PWD)/hl2/info_darknessmode_lightsource.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/info_teleporter_countdown.P
endif

$(OBJ_DIR)/info_teleporter_countdown.o : $(PWD)/hl2/info_teleporter_countdown.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/item_ammo.P
endif

$(OBJ_DIR)/item_ammo.o : $(PWD)/hl2/item_ammo.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/item_battery.P
endif

$(OBJ_DIR)/item_battery.o : $(PWD)/hl2/item_battery.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/item_dynamic_resupply.P
endif

$(OBJ_DIR)/item_dynamic_resupply.o : $(PWD)/hl2/item_dynamic_resupply.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/item_healthkit.P
endif

$(OBJ_DIR)/item_healthkit.o : $(PWD)/hl2/item_healthkit.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/item_itemcrate.P
endif

$(OBJ_DIR)/item_itemcrate.o : $(PWD)/hl2/item_itemcrate.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/item_suit.P
endif

$(OBJ_DIR)/item_suit.o : $(PWD)/hl2/item_suit.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/look_door.P
endif

$(OBJ_DIR)/look_door.o : $(PWD)/hl2/look_door.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/monster_dummy.P
endif

$(OBJ_DIR)/monster_dummy.o : $(PWD)/hl2/monster_dummy.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_alyx_episodic.P
endif

$(OBJ_DIR)/npc_alyx_episodic.o : $(PWD)/hl2/npc_alyx_episodic.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_antlion.P
endif

$(OBJ_DIR)/npc_antlion.o : $(PWD)/hl2/npc_antlion.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_antliongrub.P
endif

$(OBJ_DIR)/npc_antliongrub.o : $(PWD)/hl2/npc_antliongrub.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_antlionguard.P
endif

$(OBJ_DIR)/npc_antlionguard.o : $(PWD)/hl2/npc_antlionguard.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_apcdriver.P
endif

$(OBJ_DIR)/npc_apcdriver.o : $(PWD)/hl2/npc_apcdriver.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_attackchopper.P
endif

$(OBJ_DIR)/npc_attackchopper.o : $(PWD)/hl2/npc_attackchopper.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_barnacle.P
endif

$(OBJ_DIR)/npc_barnacle.o : $(PWD)/hl2/npc_barnacle.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_barney.P
endif

$(OBJ_DIR)/npc_barney.o : $(PWD)/hl2/npc_barney.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_basescanner.P
endif

$(OBJ_DIR)/npc_basescanner.o : $(PWD)/hl2/npc_basescanner.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_BaseZombie.P
endif

$(OBJ_DIR)/npc_BaseZombie.o : $(PWD)/hl2/npc_BaseZombie.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_breen.P
endif

$(OBJ_DIR)/npc_breen.o : $(PWD)/hl2/npc_breen.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_bullseye.P
endif

$(OBJ_DIR)/npc_bullseye.o : $(PWD)/hl2/npc_bullseye.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_citizen17.P
endif

$(OBJ_DIR)/npc_citizen17.o : $(PWD)/hl2/npc_citizen17.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_combine.P
endif

$(OBJ_DIR)/npc_combine.o : $(PWD)/hl2/npc_combine.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_combinecamera.P
endif

$(OBJ_DIR)/npc_combinecamera.o : $(PWD)/hl2/npc_combinecamera.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_combinedropship.P
endif

$(OBJ_DIR)/npc_combinedropship.o : $(PWD)/hl2/npc_combinedropship.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_combinegunship.P
endif

$(OBJ_DIR)/npc_combinegunship.o : $(PWD)/hl2/npc_combinegunship.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_combines.P
endif

$(OBJ_DIR)/npc_combines.o : $(PWD)/hl2/npc_combines.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_cranedriver.P
endif

$(OBJ_DIR)/npc_cranedriver.o : $(PWD)/hl2/npc_cranedriver.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_crow.P
endif

$(OBJ_DIR)/npc_crow.o : $(PWD)/hl2/npc_crow.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_dog.P
endif

$(OBJ_DIR)/npc_dog.o : $(PWD)/hl2/npc_dog.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_eli.P
endif

$(OBJ_DIR)/npc_eli.o : $(PWD)/hl2/npc_eli.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_enemyfinder.P
endif

$(OBJ_DIR)/npc_enemyfinder.o : $(PWD)/hl2/npc_enemyfinder.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_fastzombie.P
endif

$(OBJ_DIR)/npc_fastzombie.o : $(PWD)/hl2/npc_fastzombie.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_fisherman.P
endif

$(OBJ_DIR)/npc_fisherman.o : $(PWD)/hl2/npc_fisherman.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_gman.P
endif

$(OBJ_DIR)/npc_gman.o : $(PWD)/hl2/npc_gman.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_headcrab.P
endif

$(OBJ_DIR)/npc_headcrab.o : $(PWD)/hl2/npc_headcrab.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_ichthyosaur.P
endif

$(OBJ_DIR)/npc_ichthyosaur.o : $(PWD)/hl2/npc_ichthyosaur.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_kleiner.P
endif

$(OBJ_DIR)/npc_kleiner.o : $(PWD)/hl2/npc_kleiner.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_launcher.P
endif

$(OBJ_DIR)/npc_launcher.o : $(PWD)/hl2/npc_launcher.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_manhack.P
endif

$(OBJ_DIR)/npc_manhack.o : $(PWD)/hl2/npc_manhack.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_metropolice.P
endif

$(OBJ_DIR)/npc_metropolice.o : $(PWD)/hl2/npc_metropolice.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_missiledefense.P
endif

$(OBJ_DIR)/npc_missiledefense.o : $(PWD)/hl2/npc_missiledefense.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_monk.P
endif

$(OBJ_DIR)/npc_monk.o : $(PWD)/hl2/npc_monk.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_mossman.P
endif

$(OBJ_DIR)/npc_mossman.o : $(PWD)/hl2/npc_mossman.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_playercompanion.P
endif

$(OBJ_DIR)/npc_playercompanion.o : $(PWD)/hl2/npc_playercompanion.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_PoisonZombie.P
endif

$(OBJ_DIR)/npc_PoisonZombie.o : $(PWD)/hl2/npc_PoisonZombie.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_rollermine.P
endif

$(OBJ_DIR)/npc_rollermine.o : $(PWD)/hl2/npc_rollermine.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_scanner.P
endif

$(OBJ_DIR)/npc_scanner.o : $(PWD)/hl2/npc_scanner.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_stalker.P
endif

$(OBJ_DIR)/npc_stalker.o : $(PWD)/hl2/npc_stalker.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_strider.P
endif

$(OBJ_DIR)/npc_strider.o : $(PWD)/hl2/npc_strider.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_turret_ceiling.P
endif

$(OBJ_DIR)/npc_turret_ceiling.o : $(PWD)/hl2/npc_turret_ceiling.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_turret_floor.P
endif

$(OBJ_DIR)/npc_turret_floor.o : $(PWD)/hl2/npc_turret_floor.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_turret_ground.P
endif

$(OBJ_DIR)/npc_turret_ground.o : $(PWD)/hl2/npc_turret_ground.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_vortigaunt_episodic.P
endif

$(OBJ_DIR)/npc_vortigaunt_episodic.o : $(PWD)/hl2/npc_vortigaunt_episodic.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_zombie.P
endif

$(OBJ_DIR)/npc_zombie.o : $(PWD)/hl2/npc_zombie.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_zombine.P
endif

$(OBJ_DIR)/npc_zombine.o : $(PWD)/hl2/npc_zombine.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/point_apc_controller.P
endif

$(OBJ_DIR)/point_apc_controller.o : $(PWD)/hl2/point_apc_controller.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/prop_combine_ball.P
endif

$(OBJ_DIR)/prop_combine_ball.o : $(PWD)/hl2/prop_combine_ball.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/prop_thumper.P
endif

$(OBJ_DIR)/prop_thumper.o : $(PWD)/hl2/prop_thumper.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/proto_sniper.P
endif

$(OBJ_DIR)/proto_sniper.o : $(PWD)/hl2/proto_sniper.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/rotorwash.P
endif

$(OBJ_DIR)/rotorwash.o : $(PWD)/hl2/rotorwash.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/script_intro.P
endif

$(OBJ_DIR)/script_intro.o : $(PWD)/hl2/script_intro.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_gaussexplosion.P
endif

$(OBJ_DIR)/te_gaussexplosion.o : $(PWD)/hl2/te_gaussexplosion.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/vehicle_airboat.P
endif

$(OBJ_DIR)/vehicle_airboat.o : $(PWD)/hl2/vehicle_airboat.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/vehicle_apc.P
endif

$(OBJ_DIR)/vehicle_apc.o : $(PWD)/hl2/vehicle_apc.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/vehicle_cannon.P
endif

$(OBJ_DIR)/vehicle_cannon.o : $(PWD)/hl2/vehicle_cannon.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/vehicle_crane.P
endif

$(OBJ_DIR)/vehicle_crane.o : $(PWD)/hl2/vehicle_crane.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/vehicle_jeep.P
endif

$(OBJ_DIR)/vehicle_jeep.o : $(PWD)/hl2/vehicle_jeep.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/vehicle_prisoner_pod.P
endif

$(OBJ_DIR)/vehicle_prisoner_pod.o : $(PWD)/hl2/vehicle_prisoner_pod.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/vehicle_viewcontroller.P
endif

$(OBJ_DIR)/vehicle_viewcontroller.o : $(PWD)/hl2/vehicle_viewcontroller.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_357.P
endif

$(OBJ_DIR)/weapon_357.o : $(PWD)/hl2/weapon_357.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_alyxgun.P
endif

$(OBJ_DIR)/weapon_alyxgun.o : $(PWD)/hl2/weapon_alyxgun.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_annabelle.P
endif

$(OBJ_DIR)/weapon_annabelle.o : $(PWD)/hl2/weapon_annabelle.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_ar2.P
endif

$(OBJ_DIR)/weapon_ar2.o : $(PWD)/hl2/weapon_ar2.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_bugbait.P
endif

$(OBJ_DIR)/weapon_bugbait.o : $(PWD)/hl2/weapon_bugbait.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_cguard.P
endif

$(OBJ_DIR)/weapon_cguard.o : $(PWD)/hl2/weapon_cguard.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_citizenpackage.P
endif

$(OBJ_DIR)/weapon_citizenpackage.o : $(PWD)/hl2/weapon_citizenpackage.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_crossbow.P
endif

$(OBJ_DIR)/weapon_crossbow.o : $(PWD)/hl2/weapon_crossbow.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_crowbar.P
endif

$(OBJ_DIR)/weapon_crowbar.o : $(PWD)/hl2/weapon_crowbar.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_flaregun.P
endif

$(OBJ_DIR)/weapon_flaregun.o : $(PWD)/hl2/weapon_flaregun.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_frag.P
endif

$(OBJ_DIR)/weapon_frag.o : $(PWD)/hl2/weapon_frag.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_physcannon.P
endif

$(OBJ_DIR)/weapon_physcannon.o : $(PWD)/hl2/weapon_physcannon.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_pistol.P
endif

$(OBJ_DIR)/weapon_pistol.o : $(PWD)/hl2/weapon_pistol.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_rpg.P
endif

$(OBJ_DIR)/weapon_rpg.o : $(PWD)/hl2/weapon_rpg.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_shotgun.P
endif

$(OBJ_DIR)/weapon_shotgun.o : $(PWD)/hl2/weapon_shotgun.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_smg1.P
endif

$(OBJ_DIR)/weapon_smg1.o : $(PWD)/hl2/weapon_smg1.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_stunstick.P
endif

$(OBJ_DIR)/weapon_stunstick.o : $(PWD)/hl2/weapon_stunstick.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/hltvdirector.P
endif

$(OBJ_DIR)/hltvdirector.o : $(PWD)/hltvdirector.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/h_ai.P
endif

$(OBJ_DIR)/h_ai.o : $(PWD)/h_ai.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/h_cycler.P
endif

$(OBJ_DIR)/h_cycler.o : $(PWD)/h_cycler.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/h_export.P
endif

$(OBJ_DIR)/h_export.o : $(PWD)/h_export.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/info_camera_link.P
endif

$(OBJ_DIR)/info_camera_link.o : $(PWD)/info_camera_link.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/info_overlay_accessor.P
endif

$(OBJ_DIR)/info_overlay_accessor.o : $(PWD)/info_overlay_accessor.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/init_factory.P
endif

$(OBJ_DIR)/init_factory.o : $(PWD)/init_factory.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/intermission.P
endif

$(OBJ_DIR)/intermission.o : $(PWD)/intermission.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/item_world.P
endif

$(OBJ_DIR)/item_world.o : $(PWD)/item_world.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/lightglow.P
endif

$(OBJ_DIR)/lightglow.o : $(PWD)/lightglow.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/lights.P
endif

$(OBJ_DIR)/lights.o : $(PWD)/lights.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/logicauto.P
endif

$(OBJ_DIR)/logicauto.o : $(PWD)/logicauto.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/logicentities.P
endif

$(OBJ_DIR)/logicentities.o : $(PWD)/logicentities.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/logicrelay.P
endif

$(OBJ_DIR)/logicrelay.o : $(PWD)/logicrelay.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/logic_achievement.P
endif

$(OBJ_DIR)/logic_achievement.o : $(PWD)/logic_achievement.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/logic_measure_movement.P
endif

$(OBJ_DIR)/logic_measure_movement.o : $(PWD)/logic_measure_movement.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/logic_navigation.P
endif

$(OBJ_DIR)/logic_navigation.o : $(PWD)/logic_navigation.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/mapentities.P
endif

$(OBJ_DIR)/mapentities.o : $(PWD)/mapentities.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/maprules.P
endif

$(OBJ_DIR)/maprules.o : $(PWD)/maprules.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/MaterialModifyControl.P
endif

$(OBJ_DIR)/MaterialModifyControl.o : $(PWD)/MaterialModifyControl.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/message_entity.P
endif

$(OBJ_DIR)/message_entity.o : $(PWD)/message_entity.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/modelentities.P
endif

$(OBJ_DIR)/modelentities.o : $(PWD)/modelentities.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/monstermaker.P
endif

$(OBJ_DIR)/monstermaker.o : $(PWD)/monstermaker.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/movehelper_server.P
endif

$(OBJ_DIR)/movehelper_server.o : $(PWD)/movehelper_server.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/movement.P
endif

$(OBJ_DIR)/movement.o : $(PWD)/movement.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/movie_explosion.P
endif

$(OBJ_DIR)/movie_explosion.o : $(PWD)/movie_explosion.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ndebugoverlay.P
endif

$(OBJ_DIR)/ndebugoverlay.o : $(PWD)/ndebugoverlay.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_talker.P
endif

$(OBJ_DIR)/npc_talker.o : $(PWD)/npc_talker.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/npc_vehicledriver.P
endif

$(OBJ_DIR)/npc_vehicledriver.o : $(PWD)/npc_vehicledriver.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/particle_fire.P
endif

$(OBJ_DIR)/particle_fire.o : $(PWD)/particle_fire.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/particle_light.P
endif

$(OBJ_DIR)/particle_light.o : $(PWD)/particle_light.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/particle_smokegrenade.P
endif

$(OBJ_DIR)/particle_smokegrenade.o : $(PWD)/particle_smokegrenade.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/particle_system.P
endif

$(OBJ_DIR)/particle_system.o : $(PWD)/particle_system.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/pathcorner.P
endif

$(OBJ_DIR)/pathcorner.o : $(PWD)/pathcorner.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/pathtrack.P
endif

$(OBJ_DIR)/pathtrack.o : $(PWD)/pathtrack.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/physconstraint.P
endif

$(OBJ_DIR)/physconstraint.o : $(PWD)/physconstraint.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/physics.P
endif

$(OBJ_DIR)/physics.o : $(PWD)/physics.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/physics_bone_follower.P
endif

$(OBJ_DIR)/physics_bone_follower.o : $(PWD)/physics_bone_follower.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/physics_cannister.P
endif

$(OBJ_DIR)/physics_cannister.o : $(PWD)/physics_cannister.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/physics_fx.P
endif

$(OBJ_DIR)/physics_fx.o : $(PWD)/physics_fx.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/physics_impact_damage.P
endif

$(OBJ_DIR)/physics_impact_damage.o : $(PWD)/physics_impact_damage.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/physics_main.P
endif

$(OBJ_DIR)/physics_main.o : $(PWD)/physics_main.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/physics_npc_solver.P
endif

$(OBJ_DIR)/physics_npc_solver.o : $(PWD)/physics_npc_solver.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/physics_prop_ragdoll.P
endif

$(OBJ_DIR)/physics_prop_ragdoll.o : $(PWD)/physics_prop_ragdoll.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/physobj.P
endif

$(OBJ_DIR)/physobj.o : $(PWD)/physobj.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/phys_controller.P
endif

$(OBJ_DIR)/phys_controller.o : $(PWD)/phys_controller.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/plasma.P
endif

$(OBJ_DIR)/plasma.o : $(PWD)/plasma.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/player.P
endif

$(OBJ_DIR)/player.o : $(PWD)/player.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/playerinfomanager.P
endif

$(OBJ_DIR)/playerinfomanager.o : $(PWD)/playerinfomanager.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/playerlocaldata.P
endif

$(OBJ_DIR)/playerlocaldata.o : $(PWD)/playerlocaldata.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/player_command.P
endif

$(OBJ_DIR)/player_command.o : $(PWD)/player_command.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/player_lagcompensation.P
endif

$(OBJ_DIR)/player_lagcompensation.o : $(PWD)/player_lagcompensation.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/player_pickup.P
endif

$(OBJ_DIR)/player_pickup.o : $(PWD)/player_pickup.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/player_resource.P
endif

$(OBJ_DIR)/player_resource.o : $(PWD)/player_resource.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/plugin_check.P
endif

$(OBJ_DIR)/plugin_check.o : $(PWD)/plugin_check.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/pointanglesensor.P
endif

$(OBJ_DIR)/pointanglesensor.o : $(PWD)/pointanglesensor.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/PointAngularVelocitySensor.P
endif

$(OBJ_DIR)/PointAngularVelocitySensor.o : $(PWD)/PointAngularVelocitySensor.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/pointhurt.P
endif

$(OBJ_DIR)/pointhurt.o : $(PWD)/pointhurt.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/pointteleport.P
endif

$(OBJ_DIR)/pointteleport.o : $(PWD)/pointteleport.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/point_camera.P
endif

$(OBJ_DIR)/point_camera.o : $(PWD)/point_camera.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/point_devshot_camera.P
endif

$(OBJ_DIR)/point_devshot_camera.o : $(PWD)/point_devshot_camera.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/point_playermoveconstraint.P
endif

$(OBJ_DIR)/point_playermoveconstraint.o : $(PWD)/point_playermoveconstraint.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/point_spotlight.P
endif

$(OBJ_DIR)/point_spotlight.o : $(PWD)/point_spotlight.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/point_template.P
endif

$(OBJ_DIR)/point_template.o : $(PWD)/point_template.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/props.P
endif

$(OBJ_DIR)/props.o : $(PWD)/props.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/RagdollBoogie.P
endif

$(OBJ_DIR)/RagdollBoogie.o : $(PWD)/RagdollBoogie.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ragdoll_manager.P
endif

$(OBJ_DIR)/ragdoll_manager.o : $(PWD)/ragdoll_manager.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/recipientfilter.P
endif

$(OBJ_DIR)/recipientfilter.o : $(PWD)/recipientfilter.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/rope.P
endif

$(OBJ_DIR)/rope.o : $(PWD)/rope.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/saverestore_gamedll.P
endif

$(OBJ_DIR)/saverestore_gamedll.o : $(PWD)/saverestore_gamedll.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/sceneentity.P
endif

$(OBJ_DIR)/sceneentity.o : $(PWD)/sceneentity.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/scratchpad_gamedll_helpers.P
endif

$(OBJ_DIR)/scratchpad_gamedll_helpers.o : $(PWD)/scratchpad_gamedll_helpers.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/scripted.P
endif

$(OBJ_DIR)/scripted.o : $(PWD)/scripted.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/scriptedtarget.P
endif

$(OBJ_DIR)/scriptedtarget.o : $(PWD)/scriptedtarget.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/sendproxy.P
endif

$(OBJ_DIR)/sendproxy.o : $(PWD)/sendproxy.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ServerNetworkProperty.P
endif

$(OBJ_DIR)/ServerNetworkProperty.o : $(PWD)/ServerNetworkProperty.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/shadowcontrol.P
endif

$(OBJ_DIR)/shadowcontrol.o : $(PWD)/shadowcontrol.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/SkyCamera.P
endif

$(OBJ_DIR)/SkyCamera.o : $(PWD)/SkyCamera.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/slideshow_display.P
endif

$(OBJ_DIR)/slideshow_display.o : $(PWD)/slideshow_display.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/smokestack.P
endif

$(OBJ_DIR)/smokestack.o : $(PWD)/smokestack.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/smoke_trail.P
endif

$(OBJ_DIR)/smoke_trail.o : $(PWD)/smoke_trail.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/sound.P
endif

$(OBJ_DIR)/sound.o : $(PWD)/sound.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/soundent.P
endif

$(OBJ_DIR)/soundent.o : $(PWD)/soundent.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/soundscape.P
endif

$(OBJ_DIR)/soundscape.o : $(PWD)/soundscape.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/soundscape_system.P
endif

$(OBJ_DIR)/soundscape_system.o : $(PWD)/soundscape_system.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/spotlightend.P
endif

$(OBJ_DIR)/spotlightend.o : $(PWD)/spotlightend.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/sprite_perfmonitor.P
endif

$(OBJ_DIR)/sprite_perfmonitor.o : $(PWD)/sprite_perfmonitor.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/stdafx.P
endif

$(OBJ_DIR)/stdafx.o : $(PWD)/stdafx.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/steamjet.P
endif

$(OBJ_DIR)/steamjet.o : $(PWD)/steamjet.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/subs.P
endif

$(OBJ_DIR)/subs.o : $(PWD)/subs.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/sun.P
endif

$(OBJ_DIR)/sun.o : $(PWD)/sun.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/tactical_mission.P
endif

$(OBJ_DIR)/tactical_mission.o : $(PWD)/tactical_mission.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/tanktrain.P
endif

$(OBJ_DIR)/tanktrain.o : $(PWD)/tanktrain.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te.P
endif

$(OBJ_DIR)/te.o : $(PWD)/te.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/team.P
endif

$(OBJ_DIR)/team.o : $(PWD)/team.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/team_spawnpoint.P
endif

$(OBJ_DIR)/team_spawnpoint.o : $(PWD)/team_spawnpoint.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/TemplateEntities.P
endif

$(OBJ_DIR)/TemplateEntities.o : $(PWD)/TemplateEntities.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/tempmonster.P
endif

$(OBJ_DIR)/tempmonster.o : $(PWD)/tempmonster.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/tesla.P
endif

$(OBJ_DIR)/tesla.o : $(PWD)/tesla.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/testfunctions.P
endif

$(OBJ_DIR)/testfunctions.o : $(PWD)/testfunctions.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/testtraceline.P
endif

$(OBJ_DIR)/testtraceline.o : $(PWD)/testtraceline.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/test_proxytoggle.P
endif

$(OBJ_DIR)/test_proxytoggle.o : $(PWD)/test_proxytoggle.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/test_stressentities.P
endif

$(OBJ_DIR)/test_stressentities.o : $(PWD)/test_stressentities.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/textstatsmgr.P
endif

$(OBJ_DIR)/textstatsmgr.o : $(PWD)/textstatsmgr.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_armorricochet.P
endif

$(OBJ_DIR)/te_armorricochet.o : $(PWD)/te_armorricochet.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_basebeam.P
endif

$(OBJ_DIR)/te_basebeam.o : $(PWD)/te_basebeam.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_beamentpoint.P
endif

$(OBJ_DIR)/te_beamentpoint.o : $(PWD)/te_beamentpoint.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_beaments.P
endif

$(OBJ_DIR)/te_beaments.o : $(PWD)/te_beaments.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_beamfollow.P
endif

$(OBJ_DIR)/te_beamfollow.o : $(PWD)/te_beamfollow.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_beamlaser.P
endif

$(OBJ_DIR)/te_beamlaser.o : $(PWD)/te_beamlaser.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_beampoints.P
endif

$(OBJ_DIR)/te_beampoints.o : $(PWD)/te_beampoints.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_beamring.P
endif

$(OBJ_DIR)/te_beamring.o : $(PWD)/te_beamring.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_beamringpoint.P
endif

$(OBJ_DIR)/te_beamringpoint.o : $(PWD)/te_beamringpoint.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_beamspline.P
endif

$(OBJ_DIR)/te_beamspline.o : $(PWD)/te_beamspline.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_bloodsprite.P
endif

$(OBJ_DIR)/te_bloodsprite.o : $(PWD)/te_bloodsprite.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_bloodstream.P
endif

$(OBJ_DIR)/te_bloodstream.o : $(PWD)/te_bloodstream.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_breakmodel.P
endif

$(OBJ_DIR)/te_breakmodel.o : $(PWD)/te_breakmodel.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_bspdecal.P
endif

$(OBJ_DIR)/te_bspdecal.o : $(PWD)/te_bspdecal.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_bubbles.P
endif

$(OBJ_DIR)/te_bubbles.o : $(PWD)/te_bubbles.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_bubbletrail.P
endif

$(OBJ_DIR)/te_bubbletrail.o : $(PWD)/te_bubbletrail.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_clientprojectile.P
endif

$(OBJ_DIR)/te_clientprojectile.o : $(PWD)/te_clientprojectile.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_decal.P
endif

$(OBJ_DIR)/te_decal.o : $(PWD)/te_decal.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_dynamiclight.P
endif

$(OBJ_DIR)/te_dynamiclight.o : $(PWD)/te_dynamiclight.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_effect_dispatch.P
endif

$(OBJ_DIR)/te_effect_dispatch.o : $(PWD)/te_effect_dispatch.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_energysplash.P
endif

$(OBJ_DIR)/te_energysplash.o : $(PWD)/te_energysplash.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_explosion.P
endif

$(OBJ_DIR)/te_explosion.o : $(PWD)/te_explosion.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_fizz.P
endif

$(OBJ_DIR)/te_fizz.o : $(PWD)/te_fizz.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_footprintdecal.P
endif

$(OBJ_DIR)/te_footprintdecal.o : $(PWD)/te_footprintdecal.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_glassshatter.P
endif

$(OBJ_DIR)/te_glassshatter.o : $(PWD)/te_glassshatter.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_glowsprite.P
endif

$(OBJ_DIR)/te_glowsprite.o : $(PWD)/te_glowsprite.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_impact.P
endif

$(OBJ_DIR)/te_impact.o : $(PWD)/te_impact.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_killplayerattachments.P
endif

$(OBJ_DIR)/te_killplayerattachments.o : $(PWD)/te_killplayerattachments.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_largefunnel.P
endif

$(OBJ_DIR)/te_largefunnel.o : $(PWD)/te_largefunnel.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_muzzleflash.P
endif

$(OBJ_DIR)/te_muzzleflash.o : $(PWD)/te_muzzleflash.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_particlesystem.P
endif

$(OBJ_DIR)/te_particlesystem.o : $(PWD)/te_particlesystem.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_physicsprop.P
endif

$(OBJ_DIR)/te_physicsprop.o : $(PWD)/te_physicsprop.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_playerdecal.P
endif

$(OBJ_DIR)/te_playerdecal.o : $(PWD)/te_playerdecal.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_projecteddecal.P
endif

$(OBJ_DIR)/te_projecteddecal.o : $(PWD)/te_projecteddecal.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_showline.P
endif

$(OBJ_DIR)/te_showline.o : $(PWD)/te_showline.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_smoke.P
endif

$(OBJ_DIR)/te_smoke.o : $(PWD)/te_smoke.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_sparks.P
endif

$(OBJ_DIR)/te_sparks.o : $(PWD)/te_sparks.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_sprite.P
endif

$(OBJ_DIR)/te_sprite.o : $(PWD)/te_sprite.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_spritespray.P
endif

$(OBJ_DIR)/te_spritespray.o : $(PWD)/te_spritespray.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/te_worlddecal.P
endif

$(OBJ_DIR)/te_worlddecal.o : $(PWD)/te_worlddecal.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/timedeventmgr.P
endif

$(OBJ_DIR)/timedeventmgr.o : $(PWD)/timedeventmgr.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/toolframework_server.P
endif

$(OBJ_DIR)/toolframework_server.o : $(PWD)/toolframework_server.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/trains.P
endif

$(OBJ_DIR)/trains.o : $(PWD)/trains.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/triggers.P
endif

$(OBJ_DIR)/triggers.o : $(PWD)/triggers.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/util.P
endif

$(OBJ_DIR)/util.o : $(PWD)/util.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/variant_t.P
endif

$(OBJ_DIR)/variant_t.o : $(PWD)/variant_t.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/vehicle_base.P
endif

$(OBJ_DIR)/vehicle_base.o : $(PWD)/vehicle_base.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/vehicle_baseserver.P
endif

$(OBJ_DIR)/vehicle_baseserver.o : $(PWD)/vehicle_baseserver.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/vehicle_choreo_generic.P
endif

$(OBJ_DIR)/vehicle_choreo_generic.o : $(PWD)/vehicle_choreo_generic.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/vguiscreen.P
endif

$(OBJ_DIR)/vguiscreen.o : $(PWD)/vguiscreen.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/waterbullet.P
endif

$(OBJ_DIR)/waterbullet.o : $(PWD)/waterbullet.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/WaterLODControl.P
endif

$(OBJ_DIR)/WaterLODControl.o : $(PWD)/WaterLODControl.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/wcedit.P
endif

$(OBJ_DIR)/wcedit.o : $(PWD)/wcedit.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weapon_cubemap.P
endif

$(OBJ_DIR)/weapon_cubemap.o : $(PWD)/weapon_cubemap.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/weight_button.P
endif

$(OBJ_DIR)/weight_button.o : $(PWD)/weight_button.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/world.P
endif

$(OBJ_DIR)/world.o : $(PWD)/world.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

# Uncomment this, and set FILENAME to file you want built without optimizations enabled.
# $(OBJ_DIR)/FILENAME.o : CFLAGS := $(subst -O2,-O0,$(CFLAGS))

# Uncomment this to disable optimizations for the entire project.
# $(OBJ_DIR)/%.o : CFLAGS := $(subst -O2,-O0,$(CFLAGS))


