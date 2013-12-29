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
alloc $projPos

dp4 $projPos.x, $vPos, $cModelViewProj0
dp4 $projPos.y, $vPos, $cModelViewProj1
dp4 $projPos.z, $vPos, $cModelViewProj2
dp4 $projPos.w, $vPos, $cModelViewProj3
mov oPos, $projPos

dp3 $worldPos.x, $vPos, $cModel0
dp3 $worldPos.y, $vPos, $cModel1
dp3 $worldPos.z, $vPos, $cModel2

dp3 $worldNormal.x, $vNormal, $cModel0
dp3 $worldNormal.y, $vNormal, $cModel1
dp3 $worldNormal.z, $vNormal, $cModel2

dp3 $worldTangentS.x, $vTangentS, $cModel0
dp3 $worldTangentS.y, $vTangentS, $cModel1
dp3 $worldTangentS.z, $vTangentS, $cModel2

dp3 $worldTangentT.x, $vTangentT, $cModel0
dp3 $worldTangentT.y, $vTangentT, $cModel1
dp3 $worldTangentT.z, $vTangentT, $cModel2

&CalcFog( $worldPos, $projPos );

alloc $worldEyeVect

; Get the eye vector in world space
add $worldEyeVect.xyz, -$worldPos, $cEyePos

alloc $tangentEyeVect
alloc $bumpTexCoord

; transform the eye vector to tangent space
dp3 $tangentEyeVect.x, $worldEyeVect, $worldTangentS
dp3 $tangentEyeVect.y, $worldEyeVect, $worldTangentT
dp3 $tangentEyeVect.z, $worldEyeVect, $worldNormal

&Normalize( $tangentEyeVect );

; stick the tangent space eye vector into oD0
mad oD0.xyz, $tangentEyeVect, $cHalf, $cHalf

dp4 $bumpTexCoord.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_1
dp4 $bumpTexCoord.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_2

; dudv map
mov oT0.xy, $bumpTexCoord

; refract tint
mov oT3.xy, $bumpTexCoord

free $bumpTexCoord

alloc $newProjPos
alloc $w

mov oPos, $projPos

; special case perspective correct texture projection so that the texture fits exactly on the screen
mul $projPos.y, $projPos.y, $SHADER_SPECIFIC_CONST_4.w
add $projPos.xy, $projPos.xy, $projPos.w
mul $projPos.xy, $projPos.xy, $cHalf

; Do the perspective divide here. .yuck . . we aren't going to be perspective correct
rcp $w.w, $projPos.w
mul $projPos, $projPos, $w.w

#max $projPos.x, $projPos.x, -$cOne
#min $projPos.x, $projPos.x, $cOne
#max $projPos.z, $projPos.z, $cZero
#min $projPos.z, $projPos.z, $cOne

;------------------------------------------------------------------------------
; Transform the tangentS from world to view space
;------------------------------------------------------------------------------

alloc $projTangentS

; we only care about x and y
dp3 $projTangentS.x, $worldTangentS, $cViewProj0
dp3 $projTangentS.y, $worldTangentS, $cViewProj1

; project tangentS
mul $projTangentS.xy, $projTangentS.xy, $w.w

;max $projTangentS.xy, $projTangentS.xy, $cOne
;min $projTangentS.xy, $projTangentS.xy, -$cOne

;------------------------------------------------------------------------------
; Transform the tangentT from world to view space
;------------------------------------------------------------------------------

alloc $projTangentT
alloc $texCoord

; we only care about x and y
dp3 $projTangentT.x, $worldTangentT, $cViewProj0
dp3 $projTangentT.y, $worldTangentT, $cViewProj1

; project tangentT
mul $projTangentT.xy, $projTangentT.xy, $w.w

;max $projTangentT.xy, $projTangentT.xy, $cOne
;min $projTangentT.xy, $projTangentT.xy, -$cOne

;max $projPos.xy, $projPos.xy, $cOne
;min $projPos.xy, $projPos.xy, -$cOne

mul oT1.x, $projTangentS.x, $SHADER_SPECIFIC_CONST_3.x
mul oT1.y, $projTangentT.x, $SHADER_SPECIFIC_CONST_3.x
mov oT1.z, $projPos.x ; huh?

mul $texCoord.x, $projTangentS.y, -$SHADER_SPECIFIC_CONST_3.x
mul $texCoord.y, $projTangentT.y, -$SHADER_SPECIFIC_CONST_3.x
mov $texCoord.z, $projPos.y
mov oT2.xyz, $texCoord
mov oT3.xyz, $texCoord

free $texCoord
free $projPos
free $worldPos
free $worldEyeVect
free $tangentEyeVect
free $w
free $projTangentS
free $projTangentT
free $newProjPos
free $worldNormal
free $worldTangentS
free $worldTangentT
