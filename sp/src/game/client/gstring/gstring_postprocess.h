#ifndef G_STRING_POST_PROCESS_H
#define G_STRING_POST_PROCESS_H

void PerformScenePostProcessHack();

float GetSceneFadeScalar();

void DrawBarsAndGrain( int x, int y, int w, int h );

void SetGodraysColor( Vector col = Vector( 1, 1, 1 ) );
void SetGodraysIntensity( float i = 1.0f );
bool ShouldDrawGodrays();
void DrawGodrays();

void QueueExplosionBlur( Vector origin, float lifetime = 2.0f );
void DrawExplosionBlur();

void DrawMotionBlur();

void DrawScreenGaussianBlur();

void DrawDreamBlur();

void DrawBloomFlare();

void DrawDesaturation();

void SetNightvisionParams( float flBlackFade, float flNightvisionAmount, float flOverbright );
void DrawNightvision();
float GetNightvisionMinLighting();

void DrawHurtFX();

void ResetEffects();

#endif