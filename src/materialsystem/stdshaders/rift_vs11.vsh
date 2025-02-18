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

; Transform position from object to world space
alloc $worldPos
&SkinPosition( $worldPos );

; Project to view space
alloc $projPos
dp4 $projPos.x, $worldPos, $cViewProj0
dp4 $projPos.y, $worldPos, $cViewProj1
dp4 $projPos.z, $worldPos, $cViewProj2
dp4 $projPos.w, $worldPos, $cViewProj3
mov oPos, $projPos

&CalcFog( $worldPos, $projPos );

alloc $newProjPos

mov oPos, $projPos

; special case perspective correct texture projection so that the texture fits exactly on the screen
mul $projPos.y, $projPos.y, $SHADER_SPECIFIC_CONST_4.w
add $projPos.xy, $projPos.xy, $projPos.w
mul $projPos.xy, $projPos.xy, $cHalf

mov oT0.xy, $projPos.xy
mov oT0.z, $cZero
mov oT0.w, $projPos.w

free $projPos
free $worldPos
free $newProjPos

; alpha map
alloc $bumpTexCoord
dp4 $bumpTexCoord.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_1
dp4 $bumpTexCoord.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_2
mov oT1.xy, $bumpTexCoord
free $bumpTexCoord
