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
&FreeRegister( \$projPos );

;------------------------------------------------------------------------------
; Flesh area
;------------------------------------------------------------------------------
;	// Store the closest effect intensity
;	o.flDistanceToEffectCenter_flFresnelEffect.x = 9999.0f; // A very large distance
;	o.flDistanceToEffectCenter_flFresnelEffect.x = min( o.flDistanceToEffectCenter_flFresnelEffect.x, length( vWorldPosition.xyz - g_vEffectCenterOoRadius1.xyz ) * g_vEffectCenterOoRadius1.w );
;	o.flDistanceToEffectCenter_flFresnelEffect.x = min( o.flDistanceToEffectCenter_flFresnelEffect.x, length( vWorldPosition.xyz - g_vEffectCenterOoRadius2.xyz ) * g_vEffectCenterOoRadius2.w );
;	o.flDistanceToEffectCenter_flFresnelEffect.x = min( o.flDistanceToEffectCenter_flFresnelEffect.x, length( vWorldPosition.xyz - g_vEffectCenterOoRadius3.xyz ) * g_vEffectCenterOoRadius3.w );
;	o.flDistanceToEffectCenter_flFresnelEffect.x = min( o.flDistanceToEffectCenter_flFresnelEffect.x, length( vWorldPosition.xyz - g_vEffectCenterOoRadius4.xyz ) * g_vEffectCenterOoRadius4.w );

alloc $tmp1
alloc $flEffect

mov $flEffect, $cTwo
mad $flEffect, $flEffect, $cTwo, $cTwo

sub $tmp1.xyz, $worldPos, $SHADER_SPECIFIC_CONST_1
dp3 $tmp1.w, $tmp1, $tmp1
rsq $tmp1.w, $tmp1.w
rcp $tmp1.w, $tmp1.w
mul $tmp1.w, $tmp1.w, $SHADER_SPECIFIC_CONST_1.w
min $flEffect, $flEffect, $tmp1.w

sub $tmp1.xyz, $worldPos, $SHADER_SPECIFIC_CONST_2
dp3 $tmp1.w, $tmp1, $tmp1
rsq $tmp1.w, $tmp1.w
rcp $tmp1.w, $tmp1.w
mul $tmp1.w, $tmp1.w, $SHADER_SPECIFIC_CONST_2.w
min $flEffect, $flEffect, $tmp1.w

sub $tmp1.xyz, $worldPos, $SHADER_SPECIFIC_CONST_3
dp3 $tmp1.w, $tmp1, $tmp1
rsq $tmp1.w, $tmp1.w
rcp $tmp1.w, $tmp1.w
mul $tmp1.w, $tmp1.w, $SHADER_SPECIFIC_CONST_3.w
min $flEffect, $flEffect, $tmp1.w

sub $tmp1.xyz, $worldPos, $SHADER_SPECIFIC_CONST_4
dp3 $tmp1.w, $tmp1, $tmp1
rsq $tmp1.w, $tmp1.w
rcp $tmp1.w, $tmp1.w
mul $tmp1.w, $tmp1.w, $SHADER_SPECIFIC_CONST_4.w
min $flEffect, $flEffect, $tmp1.w

mov oD0, $flEffect

; float3 vWorldViewVector = normalize( vWorldPosition.xyz - cEyePos.xyz );
; o.flDistanceToEffectCenter_flFresnelEffect.y = pow( saturate( dot( -vWorldViewVector.xyz, vWorldNormal.xyz ) ), 1.5f );

sub $tmp1, $worldPos, $cEyePos
&Normalize( $tmp1 );
dp3 $tmp1, -$tmp1, $worldNormal
max $tmp1, $tmp1, $cZero
mul $tmp1, $tmp1, $tmp1
mov oD1, $tmp1

free $tmp1
free $flEffect

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

mov oT0.xy, $vTexCoord0

alloc $tmp2

dp4 $tmp2.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_0
dp4 $tmp2.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_1

add oT1.xy, $tmp2, $SHADER_SPECIFIC_CONST_4

free $tmp2

; YUCK!  This is to make texcoords continuous for mat_softwaretl
mov oT2, $cZero

&FreeRegister( \$worldPos );
&FreeRegister( \$worldNormal );
