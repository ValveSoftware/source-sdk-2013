vs.1.1
;------------------------------------------------------------------------------
;	 $SHADER_SPECIFIC_CONST_0	 = eyeball origin			
;	 $SHADER_SPECIFIC_CONST_1	 = eyeball up * 0.5			
;	 $SHADER_SPECIFIC_CONST_2	 = iris projection U		
;	 $SHADER_SPECIFIC_CONST_3	 = iris projection V		
;	 $SHADER_SPECIFIC_CONST_4	 = glint projection U		
;	 $SHADER_SPECIFIC_CONST_5	 = glint projection V		
;------------------------------------------------------------------------------

# STATIC: "HALF_LAMBERT"			"0..1"
# DYNAMIC: "DOWATERFOG"				"0..1"
# DYNAMIC: "LIGHT_COMBO"			"0..21"
# DYNAMIC: "SKINNING"				"0..1"

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending (whacks r1-r7, positions in r7)
;------------------------------------------------------------------------------
&AllocateRegister( \$worldPos );
&SkinPosition( $worldPos );

;------------------------------------------------------------------------------
; Transform the position from world to view space
;------------------------------------------------------------------------------

&AllocateRegister( \$projPos );

dp4 $projPos.x, $worldPos, $cViewProj0
dp4 $projPos.y, $worldPos, $cViewProj1
dp4 $projPos.z, $worldPos, $cViewProj2
dp4 $projPos.w, $worldPos, $cViewProj3
mov oPos, $projPos

;------------------------------------------------------------------------------
; Normal is based on vertex position 
;------------------------------------------------------------------------------
&AllocateRegister( \$worldNormal );
&AllocateRegister( \$normalDotUp );

sub $worldNormal, $worldPos, $SHADER_SPECIFIC_CONST_0		; Normal = (Pos - Eye origin)
dp3 $normalDotUp, $worldNormal, $SHADER_SPECIFIC_CONST_1		; Normal -= 0.5f * (Normal dot Eye Up) * Eye Up
mul $normalDotUp, $normalDotUp, $cHalf
mad $worldNormal, -$normalDotUp, $SHADER_SPECIFIC_CONST_1, $worldNormal

&FreeRegister( \$normalDotUp );

; normalize the normal
&Normalize( $worldNormal );

;------------------------------------------------------------------------------
; Lighting
;------------------------------------------------------------------------------
&DoLighting( $worldPos, $worldNormal );

&FreeRegister( \$worldNormal );
 
;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------

&CalcFog( $worldPos, $projPos );

&FreeRegister( \$projPos );

;------------------------------------------------------------------------------
; Texture coordinates
; Texture 0 is the base texture
; Texture 1 is a planar projection used for the iris
; Texture 2 is a planar projection used for the glint
;------------------------------------------------------------------------------

mov oT0, $vTexCoord0
dp4 oT1.x, $SHADER_SPECIFIC_CONST_2, $worldPos
dp4 oT1.y, $SHADER_SPECIFIC_CONST_3, $worldPos
dp4 oT2.x, $SHADER_SPECIFIC_CONST_4, $worldPos
dp4 oT2.y, $SHADER_SPECIFIC_CONST_5, $worldPos

&FreeRegister( \$worldPos );
