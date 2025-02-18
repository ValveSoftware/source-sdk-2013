vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending
;------------------------------------------------------------------------------

alloc $projPos

; Transform position from object to projection space
; put in r1 now since we need it for fog.
dp4 $projPos.x, $vPos, $cModelViewProj0
dp4 $projPos.y, $vPos, $cModelViewProj1
dp4 $projPos.z, $vPos, $cModelViewProj2
dp4 $projPos.w, $vPos, $cModelViewProj3

mov oPos, $projPos

alloc $worldPos
if( $DOWATERFOG == 1 )
{
	; Get the worldpos z component only since that's all we need for height fog
	dp4 $worldPos.z, $vPos, $cModel2
}
&CalcFog( $worldPos, $projPos );
free $worldPos


;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

; Compute the texture coordinates given the offset between
; each bumped lightmap

&AllocateRegister( \$bumpOffset );

mov $bumpOffset.xy, $vTexCoord2
mov oT0, $vTexCoord0				; bumpmap texcoords 
add oT1.xy, $bumpOffset, $vTexCoord1			; first lightmap texcoord
mad oT2.xy, $bumpOffset, $cTwo, $vTexCoord1		; second lightmap texcoord
; make a 3
alloc $three
add $three, $cOne, $cTwo
mad oT3.xy, $bumpOffset, $three, $vTexCoord1	; third lightmap texcoord
free $three

&FreeRegister( \$bumpOffset );

;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------

free $projPos
