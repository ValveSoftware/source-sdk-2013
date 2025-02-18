
#include "macros.vsh"

sub UnlitTwoTexture
{
	local( $vertexcolor ) = shift;

	local( $worldPos, $projPos );

	;------------------------------------------------------------------------------
	; Vertex blending
	;------------------------------------------------------------------------------
	&AllocateRegister( \$worldPos );
	&SkinPosition( $worldPos );

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
	&FreeRegister( \$projPos );
	&FreeRegister( \$worldPos );

	;------------------------------------------------------------------------------
	; Texture coordinates (use world-space normal for envmap, tex transform for mask)
	;------------------------------------------------------------------------------

	dp4 oT0.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_0
	dp4 oT0.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_1
	if ( $g_x360 )
	{
		; must write xyzw to match read in pixelshader
		mov oT0.zw, $cZero
	}

	dp4 oT1.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_2
	dp4 oT1.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_3
	if ( $g_x360 )
	{
		; must write xyzw to match read in pixelshader
		mov oT1.zw, $cZero
	}

	if( $vertexcolor )
	{
		; Modulation color
		mul oD0, $vColor, $cModulationColor
	}
	else
	{
		; Modulation color
		mov oD0, $cModulationColor
	}
}