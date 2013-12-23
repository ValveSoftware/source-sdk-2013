vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"
# DYNAMIC: "SKINNING"				"0..1"

;------------------------------------------------------------------------------
; Constants specified by the app
;	 $SHADER_SPECIFIC_CONST_0-$SHADER_SPECIFIC_CONST_2 = Shadow texture matrix
;	 $SHADER_SPECIFIC_CONST_3	 = Tex origin
;	 $SHADER_SPECIFIC_CONST_4	 = Tex Scale
;	 $SHADER_SPECIFIC_CONST_5	 = [Shadow falloff offset, 1/Shadow distance, Shadow scale, 0 ]
;------------------------------------------------------------------------------

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending (whacks r1-r7, positions in r7, normals in r8)
;------------------------------------------------------------------------------
&AllocateRegister( \$worldPos );
&AllocateRegister( \$worldNormal );
&SkinPositionAndNormal( $worldPos, $worldNormal );

; Transform the position from world to view space
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
&FreeRegister( \$projPos );

;------------------------------------------------------------------------------
; Transform position into texture space (from 0 to 1)
;------------------------------------------------------------------------------
&AllocateRegister( \$texturePos );
dp4 $texturePos.x, $worldPos, $SHADER_SPECIFIC_CONST_0
dp4 $texturePos.y, $worldPos, $SHADER_SPECIFIC_CONST_1
dp4 $texturePos.z, $worldPos, $SHADER_SPECIFIC_CONST_2
&FreeRegister( \$worldPos );

;------------------------------------------------------------------------------
; Figure out the shadow fade amount
;------------------------------------------------------------------------------
&AllocateRegister( \$shadowFade );
sub $shadowFade, $texturePos.z, $SHADER_SPECIFIC_CONST_5.x
mul $shadowFade, $shadowFade, $SHADER_SPECIFIC_CONST_5.y
  
;------------------------------------------------------------------------------
; Offset it into the texture
;------------------------------------------------------------------------------
&AllocateRegister( \$actualTextureCoord );
mul $actualTextureCoord.xyz, $SHADER_SPECIFIC_CONST_4, $texturePos
add oT0.xyz, $actualTextureCoord, $SHADER_SPECIFIC_CONST_3
;mov oT0.xyz, $texturePos
&FreeRegister( \$actualTextureCoord );

;------------------------------------------------------------------------------
; We're doing clipping by using texkill
;------------------------------------------------------------------------------
mov oT1.xyz, $texturePos		; also clips when shadow z < 0 !
sub oT2.xyz, $cOne, $texturePos
sub oT2.z, $cOne, $shadowFade.z	; clips when shadow z > shadow distance	
&FreeRegister( \$texturePos );

;------------------------------------------------------------------------------
; We're doing backface culling by using texkill also (wow yucky)
;------------------------------------------------------------------------------
; Transform z component of normal in texture space
; If it's negative, then don't draw the pixel
dp3 oT3, $worldNormal, -$SHADER_SPECIFIC_CONST_2
&FreeRegister( \$worldNormal );

;------------------------------------------------------------------------------
; Shadow color, falloff
;------------------------------------------------------------------------------
mov oD0, $cModulationColor
mul oD0.w, $shadowFade.x, $SHADER_SPECIFIC_CONST_5.z
&FreeRegister( \$shadowFade );

