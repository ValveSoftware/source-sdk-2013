vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"
# DYNAMIC: "SKINNING"				"0..1"
# STATIC: "TEETH"					"0..1"

#include "macros.vsh"

local( $worldPos, $worldNormal, $projPos );

alloc $worldPos
alloc $worldNormal
alloc $projPos

if( 0 )
{
	; NOTE: Don't do this optimization anymore since it would mean a gazillion combos
	; Special case for static prop lighting.  We can go directly from 
	; world to proj space for position, with the exception of z, which 
	; is needed for fogging *if* height fog is enabled.

	; NOTE: We don't use this path if $envmap is defined since we need
	; worldpos for envmapping.
	dp4 $projPos.x, $vPos, $cModelViewProj0
	dp4 $projPos.y, $vPos, $cModelViewProj1
	dp4 $projPos.z, $vPos, $cModelViewProj2
	dp4 $projPos.w, $vPos, $cModelViewProj3
	; normal
	dp3 $worldNormal.x, $vNormal, $cModel0
	dp3 $worldNormal.y, $vNormal, $cModel1
	dp3 $worldNormal.z, $vNormal, $cModel2

	; Need this for height fog if it's enabled and for height clipping
	dp4 $worldPos.z, $vPos, $cModel2
}
else
{
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
}

mov oPos, $projPos

;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------
&CalcFog( $worldPos, $projPos );

; base tex coords
dp4 oT1.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_6
dp4 oT1.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_7

; normal map coords
;dp4 oT3.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_8
;dp4 oT3.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_9

; spotlight texcoords
dp4 oT0.x, $worldPos, $SHADER_SPECIFIC_CONST_1
dp4 oT0.y, $worldPos, $SHADER_SPECIFIC_CONST_2
dp4 oT0.z, $worldPos, $SHADER_SPECIFIC_CONST_3
dp4 oT0.w, $worldPos, $SHADER_SPECIFIC_CONST_4

local( $worldPosToLightVector, $distFactors );

alloc $worldPosToLightVector

sub $worldPosToLightVector, $SHADER_SPECIFIC_CONST_0, $worldPos
mov oT2, $worldPosToLightVector

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

if( $TEETH )
{
	alloc $mouthAtten
	dp3 $mouthAtten, $worldNormal.xyz, $SHADER_SPECIFIC_CONST_10.xyz
	max $mouthAtten, $cZero, $mouthAtten
	mul $mouthAtten, $mouthAtten, $SHADER_SPECIFIC_CONST_10.w
	mul $vertAtten, $vertAtten, $mouthAtten
	free $mouthAtten
}

mov oD0, $vertAtten

mov oT3.xyz, $worldNormal.xyz


free $dist
free $endFalloffFactor
free $worldPos
free $worldNormal
free $projPos
free $worldPosToLightVector
free $distatten
free $vertAtten
