
#include "cbase.h"

#ifdef GAME_DLL
ConVar cvar_gstring_enable_entities( "gstring_enable_entities", "1" );
#else
ConVar cvar_gstring_enable_postprocessing( "gstring_enable_postprocessing", "1" );
ConVar cvar_gstring_enable_hud( "gstring_enable_hud", "1" );

ConVar cvar_gstring_drawdetailpropsfirst( "gstring_Drawdetailpropsfirst", "1", FCVAR_ARCHIVE );
ConVar cvar_gstring_drawbars( "gstring_DrawBars", "1", FCVAR_ARCHIVE );
ConVar cvar_gstring_drawfilmgrain( "gstring_DrawFilmGrain", "1", FCVAR_ARCHIVE );
ConVar cvar_gstring_drawvignette( "gstring_DrawVignette", "1", FCVAR_ARCHIVE );
ConVar cvar_gstring_drawgodrays( "gstring_DrawGodrays", "1", FCVAR_ARCHIVE );
ConVar cvar_gstring_drawexplosionblur( "gstring_DrawExplosionFX", "1", FCVAR_ARCHIVE );
ConVar cvar_gstring_drawmotionblur( "gstring_DrawMotionblur", "1", FCVAR_ARCHIVE );
ConVar cvar_gstring_drawscreenblur( "gstring_DrawScreenBlur", "1", FCVAR_ARCHIVE );
ConVar cvar_gstring_drawdreamblur( "gstring_DrawDreamBlur", "1", FCVAR_ARCHIVE );
ConVar cvar_gstring_drawlensflare( "gstring_DrawLensFlare", "1", FCVAR_ARCHIVE );
ConVar cvar_gstring_drawbloomflare( "gstring_DrawBloomFlare", "0", FCVAR_ARCHIVE );
ConVar cvar_gstring_drawwatereffects( "gstring_DrawWaterEffects", "1", FCVAR_ARCHIVE );

ConVar cvar_gstring_bars_scale( "gstring_Bars_scale", "0.14", FCVAR_ARCHIVE );
ConVar cvar_gstring_explosionfx_strength( "gstring_ExplosionFx_strength", "1.0", FCVAR_ARCHIVE );
ConVar cvar_gstring_motionblur_scale( "gstring_MotionBlur_scale", "0.5", FCVAR_ARCHIVE );
ConVar cvar_gstring_barsandgrain_zoom( "gstring_BarsAndGrain_zoom", "0.06", FCVAR_ARCHIVE );
ConVar cvar_gstring_bloomflare_strength( "gstring_BloomFlare_strength", "2.0", FCVAR_ARCHIVE );
ConVar cvar_gstring_desaturation_strength( "gstring_Desaturation_strength", "0.0", FCVAR_ARCHIVE );
ConVar cvar_gstring_filmgrain_strength( "gstring_FilmGrain_strength", "0.1", FCVAR_ARCHIVE );
ConVar cvar_gstring_vignette_strength( "gstring_Vignette_strength", "1.0", FCVAR_ARCHIVE );
ConVar cvar_gstring_vignette_range_min( "gstring_Vignette_range_min", "0.2", FCVAR_ARCHIVE );
ConVar cvar_gstring_vignette_range_max( "gstring_Vignette_range_max", "0.7", FCVAR_ARCHIVE );

ConVar cvar_gstring_debug_vguiparticles( "gstring_debug_VguiParticles", "0" );
ConVar cvar_gstring_nightvision_minlighting( "gstring_nightvision_minlighting", "0.5" );
#endif
