# STATIC: "UNLIT"					"0..1"
# STATIC:  "HALF_LAMBERT"			"0..1"
# DYNAMIC: "LIGHT_COMBO"			"0..21"
# DYNAMIC: "SKINNING"				"0..1"

vs.1.1
#include "macros.vsh"

$WARPPARAM = $SHADER_SPECIFIC_CONST_2;
$ENTITY_ORIGIN = $SHADER_SPECIFIC_CONST_3;

;------------------------------------------------------------------------------
; Vertex blending 
;------------------------------------------------------------------------------
&AllocateRegister( \$worldPos );
&AllocateRegister( \$worldNormal );
&AllocateRegister( \$projPos );

&SkinPositionAndNormal( $worldPos, $worldNormal );

alloc $tmp
sub $tmp.xyz, $worldPos, $ENTITY_ORIGIN
mul $tmp.xy, $tmp, $WARPPARAM
add $worldPos.xyz, $tmp, $ENTITY_ORIGIN
free $tmp

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
if( $UNLIT )
{
	mov oD0, $cHalf
}
else
{
	&DoLighting( $worldPos, $worldNormal );
}

if( !$envmap )
{
	&FreeRegister( \$worldNormal );
}

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

