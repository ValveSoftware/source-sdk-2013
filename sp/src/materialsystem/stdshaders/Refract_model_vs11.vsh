vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"
# DYNAMIC: "SKINNING"				"0..1"

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

alloc $worldPos
alloc $worldNormal
alloc $worldTangentS
alloc $worldTangentT

&SkinPositionNormalAndTangentSpace( $worldPos, $worldNormal, 
					$worldTangentS, $worldTangentT );

alloc $projPos

; Transform position from world to projection space
dp4 $projPos.x, $worldPos, $cViewProj0
dp4 $projPos.y, $worldPos, $cViewProj1
dp4 $projPos.z, $worldPos, $cViewProj2
dp4 $projPos.w, $worldPos, $cViewProj3

&CalcFog( $worldPos, $projPos );

alloc $worldEyeVect

; Get the eye vector in world space
add $worldEyeVect.xyz, -$worldPos, $cEyePos

alloc $tangentEyeVect
; transform the eye vector to tangent space
dp3 oT3.x, $worldEyeVect, $worldTangentS
dp3 oT3.y, $worldEyeVect, $worldTangentT
dp3 oT3.z, $worldEyeVect, $worldNormal

alloc $bumpTexCoord

dp4 $bumpTexCoord.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_1
dp4 $bumpTexCoord.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_2
 
; dudv map
mov oT0.xy, $bumpTexCoord

; refract tint + alpha channel
mov oT2.xy, $bumpTexCoord
mov oT3.xy, $bumpTexCoord

free $bumpTexCoord

mov oPos, $projPos

; special case perspective correct texture projection so that the texture fits exactly on the screen

; flip Y by multiplying by -1
mul $projPos.y, $projPos.y, $SHADER_SPECIFIC_CONST_4.w

; transform from [-w,w] to [0,2*w]
; The reason this is w is because we are in perspective space/homogenous clip space.
add $projPos.xy, $projPos.xy, $projPos.w

; transform from [0,2*w] to [0,w]
; We'll end up dividing by w in the pixel shader to get to [0,1]
mul $projPos.xy, $projPos.xy, $cHalf

mov oT1.xy, $projPos.xy

; emit w to both z and w in case the driver screws up and divides by z
mov oT1.z, $projPos.w
mov oT1.w, $projPos.w

free $projPos
free $worldPos
free $worldEyeVect
free $tangentEyeVect
free $w
free $worldNormal
free $worldTangentS
free $worldTangentT
