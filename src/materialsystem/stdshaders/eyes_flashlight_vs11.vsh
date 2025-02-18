vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"
# DYNAMIC: "SKINNING"				"0..1"

#include "macros.vsh"

local( $worldPos, $worldNormal, $projPos );

alloc $worldPos
alloc $projPos


&SkinPosition( $worldPos );

;------------------------------------------------------------------------------
; Transform the position from world to view space
;------------------------------------------------------------------------------
dp4 $projPos.x, $worldPos, $cViewProj0
dp4 $projPos.y, $worldPos, $cViewProj1
dp4 $projPos.z, $worldPos, $cViewProj2
dp4 $projPos.w, $worldPos, $cViewProj3

;------------------------------------------------------------------------------
; Normal is based on vertex position 
;------------------------------------------------------------------------------
&AllocateRegister( \$worldNormal );
&AllocateRegister( \$normalDotUp );

sub $worldNormal, $worldPos, $SHADER_SPECIFIC_CONST_6		; Normal = (Pos - Eye origin)
dp3 $normalDotUp, $worldNormal, $SHADER_SPECIFIC_CONST_7		; Normal -= 0.5f * (Normal dot Eye Up) * Eye Up
mul $normalDotUp, $normalDotUp, $cHalf
mad $worldNormal, -$normalDotUp, $SHADER_SPECIFIC_CONST_7, $worldNormal

&FreeRegister( \$normalDotUp );

; normalize the normal
&Normalize( $worldNormal );

mov oPos, $projPos

;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------
&CalcFog( $worldPos, $projPos );

; base tex coords
mov oT1.xy, $vTexCoord0

; spotlight texcoords
dp4 oT0.x, $worldPos, $SHADER_SPECIFIC_CONST_1
dp4 oT0.y, $worldPos, $SHADER_SPECIFIC_CONST_2
dp4 oT0.z, $worldPos, $SHADER_SPECIFIC_CONST_3
dp4 oT0.w, $worldPos, $SHADER_SPECIFIC_CONST_4

local( $worldPosToLightVector, $distFactors );

alloc $worldPosToLightVector

sub $worldPosToLightVector, $SHADER_SPECIFIC_CONST_0.xyz, $worldPos

local( $distatten );
alloc $distatten
; $distatten = [ 1, 1/dist, 1/distsquared ]

; dist squared
dp3 $distatten.z, $worldPosToLightVector, $worldPosToLightVector

; oodist
rsq $distatten.y, $distatten.z

mov $distatten.x, $cOne

local( $dist );
alloc $dist
mul $dist.x, $distatten.z, $distatten.y

rcp $distatten.z, $distatten.z ; 1/distsquared

local( $endFalloffFactor );
alloc $endFalloffFactor

; ( dist - farZ )
sub $endFalloffFactor.x, $dist.x, $SHADER_SPECIFIC_CONST_5.w
; 1 / ( (0.6f * farZ) - farZ)
mul $endFalloffFactor, $endFalloffFactor.x, $SHADER_SPECIFIC_CONST_0.w
max $endFalloffFactor, $endFalloffFactor, $cZero
min $endFalloffFactor, $endFalloffFactor, $cOne

local( $vertAtten );
alloc $vertAtten
dp3 $vertAtten, $distatten, $SHADER_SPECIFIC_CONST_5
mul $vertAtten, $vertAtten, $endFalloffFactor

; Normalize L
&Normalize( $worldPosToLightVector );

; N.L
dp3 $worldNormal, $worldNormal, $worldPosToLightVector

; Modulate distance attenuation with N.L
mul oD0, $vertAtten, $worldNormal

; iris
dp4 oT3.x, $SHADER_SPECIFIC_CONST_8, $worldPos
dp4 oT3.y, $SHADER_SPECIFIC_CONST_9, $worldPos

free $dist
free $endFalloffFactor
free $worldPos
free $worldNormal
free $projPos
free $worldPosToLightVector
free $distatten
free $vertAtten
