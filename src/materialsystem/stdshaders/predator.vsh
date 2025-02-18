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

;------------------------------------------------------------------------------
; Vertex blending 
;------------------------------------------------------------------------------
&AllocateRegister( \$worldPos );
&AllocateRegister( \$worldNormal );
&SkinPositionAndNormal( $worldPos, $worldNormal );

;------------------------------------------------------------------------------
; Transform the position from world to proj space
;------------------------------------------------------------------------------

&AllocateRegister( \$projPos );

dp4 $projPos.x, $worldPos, $cViewProj0
dp4 $projPos.y, $worldPos, $cViewProj1
dp4 $projPos.z, $worldPos, $cViewProj2
dp4 $projPos.w, $worldPos, $cViewProj3
mov oPos, $projPos


;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------

&CalcFog( $worldPos, $projPos );

&FreeRegister( \$worldPos );

;------------------------------------------------------------------------------
; Transform the normal from world to proj space
;------------------------------------------------------------------------------

&AllocateRegister( \$projNormal );

; only do X and Y since that's all we care about
dp3 $projNormal.x, $worldNormal, $cViewProj0
dp3 $projNormal.y, $worldNormal, $cViewProj1

&FreeRegister( \$worldNormal );

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

; NOTE: projPos isn't projPos after this point. :)

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

&FreeRegister( \$projPos );
&FreeRegister( \$projNormal );
