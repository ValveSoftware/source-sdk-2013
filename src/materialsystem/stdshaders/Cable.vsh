vs.1.1

#include "macros.vsh"

# DYNAMIC: "DOWATERFOG"				"0..1"

;------------------------------------------------------------------------------
; The cable equation is:
; [L dot N] * C * T
;
; where:
; C = directional light color
; T = baseTexture
; N = particle normal (stored in the normal map)
; L = directional light direction
;
;	$SHADER_SPECIFIC_CONST_0		= Directional light direction
;------------------------------------------------------------------------------


;------------------------------------------------------------------------------
; Transform position from object to projection space
;------------------------------------------------------------------------------

&AllocateRegister( \$projPos );

dp4		$projPos.x, $vPos, $cModelViewProj0
dp4		$projPos.y, $vPos, $cModelViewProj1
dp4		$projPos.z, $vPos, $cModelViewProj2
dp4		$projPos.w, $vPos, $cModelViewProj3

mov		oPos, $projPos


;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------

alloc $worldPos
if( $DOWATERFOG == 1 )
{
	; Get the worldpos z component only since that's all we need for height fog
	dp4 $worldPos.z, $vPos, $cModel2
}
&CalcFog( $worldPos, $projPos );
free $worldPos

&FreeRegister( \$projPos );

;------------------------------------------------------------------------------
; Setup the tangent space
;------------------------------------------------------------------------------

&AllocateRegister( \$tmp1 );
&AllocateRegister( \$tmp2 );
&AllocateRegister( \$tmp3 );
&AllocateRegister( \$r );

; Get S crossed with T (call it R)
mov		$tmp1, $vTangentS
mov		$tmp2, $vTangentT

mul		$tmp3, $vTangentS.yzxw,  $tmp2.zxyw
mad		$r, -$vTangentS.zxyw, $tmp2.yzxw, $tmp3

&FreeRegister( \$tmp2 );
&FreeRegister( \$tmp3 );

&AllocateRegister( \$s );

; Normalize S (into $s)
dp3		$s.w, $vTangentS, $vTangentS
rsq		$s.w, $s.w
mul		$s.xyz, $vTangentS, $s.w

; Normalize R (into $r)
dp3		$r.w, $r, $r
rsq		$r.w, $r.w
mul		$r.xyz, $r, $r.w

&AllocateRegister( \$t );

; Regenerate T (into $t)
mul		$t, $r.yzxw,  $tmp1.zxyw
mad		$t, -$r.zxyw, $tmp1.yzxw, $t

&FreeRegister( \$tmp1 );

;------------------------------------------------------------------------------
; Transform the light direction (into oD1)
;------------------------------------------------------------------------------

&AllocateRegister( \$lightDirection );

dp3		$lightDirection.x, $s, $SHADER_SPECIFIC_CONST_0
dp3		$lightDirection.y, $t, $SHADER_SPECIFIC_CONST_0
dp3		$lightDirection.z, $r, $SHADER_SPECIFIC_CONST_0

&FreeRegister( \$r );
&FreeRegister( \$s );
&FreeRegister( \$t );

; Scale into 0-1 range (we're assuming light direction was normalized prior to here)
add		oT2, $lightDirection, $cHalf	; + 0.5 
&FreeRegister( \$lightDirection );

;------------------------------------------------------------------------------
; Copy texcoords for the normal map and base texture
;------------------------------------------------------------------------------

mov		oT0, $vTexCoord0
mov		oT1, $vTexCoord1

; Pass the dirlight color through
mov		oD0.xyzw, $vColor


