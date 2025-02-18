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
; Lighting
;------------------------------------------------------------------------------
mov oD0, $cHalf

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

dp4 oT0.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_0
dp4 oT0.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_1

alloc $tmp2

dp4 $tmp2.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_0
dp4 $tmp2.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_1

add oT1.xy, $tmp2, $SHADER_SPECIFIC_CONST_4

free $tmp2

; YUCK!  This is to make texcoords continuous for mat_softwaretl
mov oT2, $cZero

&FreeRegister( \$worldPos );
&FreeRegister( \$worldNormal );
