vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"

;------------------------------------------------------------------------------
; Constants specified by the app
;    c0      = (0, 1, 2, 0.5)
;	 c1		 = (1/2.2, 0, 0, 0)
;    c2      = camera position *in world space*
;    c4-c7   = modelViewProj matrix	(transpose)
;    c8-c11  = ViewProj matrix (transpose)
;    c12-c15 = model->view matrix (transpose)
;	 c16	 = [fogStart, fogEnd, fogRange, undefined]
;
; $SHADER_SPECIFIC_CONST_0..$SHADER_SPECIFIC_CONST_3 - special proj matrix
;
; Vertex components (as specified in the vertex DECL)
;    $vPos    = Position
;	 $vTexCoord0.xy = TexCoord0
;------------------------------------------------------------------------------

#include "macros.vsh"

; Vertex components
;    $vPos		= Position
;	 $vNormal		= normal
;	 $vTexCoord0.xy	= TexCoord0
;	 $vTangentS		= S axis of Texture space
;	 $vTangentT	= T axis of Texture space


;------------------------------------------------------------------------------
; Transform the position from world to view space
;------------------------------------------------------------------------------


alloc $projPos

; Transform position from object to projection space
dp4 $projPos.x, $vPos, $cModelViewProj0
dp4 $projPos.y, $vPos, $cModelViewProj1
dp4 $projPos.z, $vPos, $cModelViewProj2
dp4 $projPos.w, $vPos, $cModelViewProj3
mov oPos, $projPos

alloc $worldPos

; Transform position from object to world space
dp4 $worldPos.x, $vPos, $cModel0
dp4 $worldPos.y, $vPos, $cModel1
dp4 $worldPos.z, $vPos, $cModel2

&CalcFog( $worldPos, $projPos );

alloc $worldEyeVect

; Get the eye vector in world space
add $worldEyeVect.xyz, -$worldPos, $cEyePos

alloc $tangentEyeVect

; transform the eye vector to tangent space
dp3 $tangentEyeVect.x, $worldEyeVect, $vTangentS
dp3 $tangentEyeVect.y, $worldEyeVect, $vTangentT
dp3 $tangentEyeVect.z, $worldEyeVect, $vNormal
mov $tangentEyeVect.w, $cZero

mov oT5, $tangentEyeVect

; base coordinates
dp4 oT0.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_1
dp4 oT0.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_2

; reflection
alloc $projPosReflect

mov $projPosReflect, $projPos
add $projPosReflect.xy, $projPosReflect, $projPosReflect.w
mul $projPosReflect.xy, $projPosReflect, $cHalf
mov oT1, $projPosReflect

; refraction
mov $projPos.y, -$projPos.y
add $projPos.xy, $projPos, $projPos.w
mul $projPos.xy, $projPos, $cHalf
mov oT2, $projPos

; reflectionscale, refractionscale
mov oT4, $SHADER_SPECIFIC_CONST_4

free $worldEyeVect
free $tangentEyeVect
free $projPosReflect
free $worldPos
free $projPos