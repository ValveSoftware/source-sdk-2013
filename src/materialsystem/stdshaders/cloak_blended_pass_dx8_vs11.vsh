# DYNAMIC: "SKINNING"				"0..1"

vs.1.1
#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending 
;------------------------------------------------------------------------------
&AllocateRegister( \$worldPos );
&AllocateRegister( \$worldNormal );
&AllocateRegister( \$projPos );

&SkinPositionAndNormal( $worldPos, $worldNormal );

if( $SKINNING == 1 )
{
	&Normalize( $worldNormal );
}

;------------------------------------------------------------------------------
; Transform the position from world to view space
;------------------------------------------------------------------------------
dp4 $projPos.x, $worldPos, $cViewProj0
dp4 $projPos.y, $worldPos, $cViewProj1
dp4 $projPos.z, $worldPos, $cViewProj2
dp4 $projPos.w, $worldPos, $cViewProj3

mov oPos, $projPos

;------------------------------------------------------------------------------
; Fog - don't bother with water fog for intro effects
;------------------------------------------------------------------------------
&DepthFog( $projPos, "oFog" );

;------------------------------------------------------------------------------
; Refract uv's (Code copied from predator.vsh)
;------------------------------------------------------------------------------
; NOTE: projPos isn't projPos after this point. :)

&AllocateRegister( \$projNormal );

; only do X and Y since that's all we care about
dp3 $projNormal.x, $worldNormal, $cViewProj0
dp3 $projNormal.y, $worldNormal, $cViewProj1

; divide by z
rcp $projPos.w, $projPos.w
mul $projPos.xy, $projPos.w, $projPos.xy

; map from -1..1 to 0..1
mad $projPos.xy, $projPos.xy, $cHalf, $cHalf

; tweak with the texcoords based on the normal and $refractionamount
mad $projPos.xy, $projNormal.xy, -$SHADER_SPECIFIC_CONST_4.xy, $projPos.xy		; FIXME

; invert y
add $projPos.y, $cOne, -$projPos.y

; hack scale for nvidia (Power of two texcoords are screwed.)
mul oT0.xy, $projPos.xy, $SHADER_SPECIFIC_CONST_5.xy

; YUCK!  This is to make texcoords continuous for mat_softwaretl
mov oT2, $cZero

&FreeRegister( \$projPos );
&FreeRegister( \$projNormal );

;------------------------------------------------------------------------------
; Refract mask
;------------------------------------------------------------------------------

; // float flFresnel = 1.0f - saturate( dot( i.vWorldNormal.xyz, normalize( -i.vWorldViewVector.xyz ) ) );
&AllocateRegister( \$flFresnel );
&AllocateRegister( \$tmp1 );

sub $flFresnel, $worldPos, $cEyePos
&Normalize( $flFresnel );
dp3 $flFresnel, -$flFresnel, $worldNormal
max $flFresnel, $flFresnel, $cZero
sub $flFresnel, $cOne, $flFresnel

; // float flCloakLerpFactor = saturate( lerp( 1.0f, flFresnel - 1.35f, saturate( g_flCloakFactor ) ) );
&AllocateRegister( \$flCloakLerpFactor );

sub $flCloakLerpFactor, $flFresnel, $SHADER_SPECIFIC_CONST_3.x ; // flFresnel - 1.35f
mov $tmp1, $cOne
sub $flCloakLerpFactor, $flCloakLerpFactor, $tmp1
mad $flCloakLerpFactor, $flCloakLerpFactor, $SHADER_SPECIFIC_CONST_3.y, $tmp1
max $flCloakLerpFactor, $flCloakLerpFactor, $cZero
min $flCloakLerpFactor, $flCloakLerpFactor, $cOne

; // flCloakLerpFactor = 1.0f - smoothstep( 0.4f, 0.425f, flCloakLerpFactor );
sub $flCloakLerpFactor, $flCloakLerpFactor, $SHADER_SPECIFIC_CONST_3.z
mul $flCloakLerpFactor, $flCloakLerpFactor, $SHADER_SPECIFIC_CONST_3.w
sub $flCloakLerpFactor, $cOne, $flCloakLerpFactor

mov oD0, $flCloakLerpFactor

&FreeRegister( \$tmp1 );
&FreeRegister( \$flFresnel );
&FreeRegister( \$flCloakLerpFactor );

&FreeRegister( \$worldPos );
&FreeRegister( \$worldNormal );
