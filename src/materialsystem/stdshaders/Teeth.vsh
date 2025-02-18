vs.1.1

# STATIC: "INTRO"					"0..1"
# STATIC: "HALF_LAMBERT"			"0..1"
# DYNAMIC: "DOWATERFOG"				"0..1"
# DYNAMIC: "LIGHT_COMBO"			"0..21"
# DYNAMIC: "SKINNING"				"0..1"

;------------------------------------------------------------------------------
;	 $SHADER_SPECIFIC_CONST_0	 = xyz = mouth forward direction vector, w = illum factor
;------------------------------------------------------------------------------

#include "macros.vsh"

$WARPPARAM = $SHADER_SPECIFIC_CONST_2;
$ENTITY_ORIGIN = $SHADER_SPECIFIC_CONST_3;

;------------------------------------------------------------------------------
; Vertex blending 
;------------------------------------------------------------------------------
alloc $worldPos
alloc $worldNormal
&SkinPositionAndNormal( $worldPos, $worldNormal );

;------------------------------------------------------------------------------
; Optional intro warping
;------------------------------------------------------------------------------
if ( $INTRO == 1 )
{
	alloc $tmp
	sub $tmp.xyz, $worldPos, $ENTITY_ORIGIN
	mul $tmp.xy, $tmp, $WARPPARAM
	add $worldPos.xyz, $tmp, $ENTITY_ORIGIN
	free $tmp
}

;------------------------------------------------------------------------------
; Transform the position from world to view space
;------------------------------------------------------------------------------

alloc $projPos

dp4 $projPos.x, $worldPos, $cViewProj0
dp4 $projPos.y, $worldPos, $cViewProj1
dp4 $projPos.z, $worldPos, $cViewProj2
dp4 $projPos.w, $worldPos, $cViewProj3
mov oPos, $projPos

;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------

&CalcFog( $worldPos, $projPos );

free $projPos

;------------------------------------------------------------------------------
; Lighting
;------------------------------------------------------------------------------
alloc $linearColor
&DoDynamicLightingToLinear( $worldPos, $worldNormal, $linearColor );

;------------------------------------------------------------------------------
; Factor in teeth darkening factors
;------------------------------------------------------------------------------

alloc $tmp

mul $linearColor.xyz, $SHADER_SPECIFIC_CONST_0.w, $linearColor	; FIXME Color darkened by illumination factor
dp3 $tmp, $worldNormal, $SHADER_SPECIFIC_CONST_0					; Figure out mouth forward dot normal
max	$tmp, $cZero, $tmp						; clamp from 0 to 1
mul $linearColor.xyz, $tmp, $linearColor	; Darken by forward dot normal too

;------------------------------------------------------------------------------
; Output color (gamma correction)
;------------------------------------------------------------------------------

alloc $gammaColor
&LinearToGamma( $linearColor, $gammaColor );
free $linearColor
mul oD0.xyz, $gammaColor.xyz, $cOverbrightFactor
mov oD0.w, $cOne				; make sure all components are defined


free $gammaColor
free $worldPos
free $worldNormal
free $tmp

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

mov oT0, $vTexCoord0



