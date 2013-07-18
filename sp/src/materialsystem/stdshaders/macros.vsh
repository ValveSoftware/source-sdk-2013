;------------------------------------
; RULES FOR AUTHORING VERTEX SHADERS:
;------------------------------------
; - never use "def" . . .set constants in code instead. . our constant shadowing will break otherwise.
;	(same goes for pixel shaders)
; - use cN notation instead of c[N] notation. .makes grepping for registers easier.
;   The only exception is c[a0.x+blah] where you have no choice.
$g_NumRegisters = 12;

; NOTE: These must match the same values in vsh_prep.pl!
$vPos				= "v0";
$vBoneWeights		= "v1";
$vBoneIndices		= "v2";
$vNormal			= "v3";
$vColor				= "v5";
$vSpecular			= "v6";
$vTexCoord0			= "v7";
$vTexCoord1			= "v8";
$vTexCoord2			= "v9";
$vTexCoord3			= "v10";
$vTangentS			= "v11";
$vTangentT			= "v12";
$vUserData			= "v14";

if( $g_dx9 )
{
	if( $g_usesPos )
	{
		dcl_position $vPos;
	}
	
	if( $g_usesBoneWeights )
	{
		dcl_blendweight $vBoneWeights;
	}
	if( $g_usesBoneIndices )
	{
		dcl_blendindices $vBoneIndices;
	}
	if( $g_usesNormal )
	{
		dcl_normal $vNormal;
	}
	if( $g_usesColor )
	{
		dcl_color0 $vColor;
	}
	if( $g_usesSpecular )
	{
		dcl_color1 $vSpecular;
	}
	if( $g_usesTexCoord0 )
	{
		dcl_texcoord0 $vTexCoord0;
	}
	if( $g_usesTexCoord1 )
	{
		dcl_texcoord1 $vTexCoord1;
	}
	if( $g_usesTexCoord2 )
	{
		dcl_texcoord2 $vTexCoord2;
	}
	if( $g_usesTexCoord3 )
	{
		dcl_texcoord3 $vTexCoord3;
	}
	if( $g_usesTangentS )
	{
		dcl_tangent $vTangentS;
	}
	if( $g_usesTangentT )
	{
		dcl_binormal0 $vTangentT;
	}
	if( $g_usesUserData )
	{
		dcl_tangent $vUserData;
	}
}

# NOTE: These should match g_LightCombinations in vertexshaderdx8.cpp!
# NOTE: Leave this on single lines or shit might blow up.
@g_staticLightTypeArray = ( "none", "static", "none", "none", "none", "none", "none", "none", "none", "none", "none", "none", "static", "static", "static", "static", "static", "static", "static", "static", "static", "static" );
@g_ambientLightTypeArray = ( "none", "none", "ambient", "ambient", "ambient", "ambient", "ambient", "ambient", 	"ambient", "ambient", "ambient", "ambient", "ambient", "ambient", "ambient", "ambient", "ambient", "ambient", "ambient", "ambient", "ambient", "ambient" );
@g_localLightType1Array = ( "none", "none", "none", "spot", "point", "directional", "spot", "spot", "spot", "point", "point", "directional", "none", "spot", "point", "directional", "spot", "spot", "spot", "point", "point", "directional" );
@g_localLightType2Array = ( "none", "none", "none", "none", "none", "none", "spot", "point", "directional", "point", "directional", "directional", "none", "none", "none", "none", "spot", "point", "directional", "point", "directional", "directional" );

$cConstants0		= "c0";
$cZero				= "c0.x";
$cOne				= "c0.y";
$cTwo				= "c0.z";
$cHalf				= "c0.w";

$cConstants1		    = "c1";
$cOOGamma			    = "c1.x"; # 1/2.2
$cOtherOverbrightFactor = "c1.y"; # overbright
$cOneThird			    = "c1.z"; # 1/3
$cOverbrightFactor      = "c1.w"; # 1/overbright

$cEyePos			= "c2";
$cWaterZ			= "c2.w";
$cEyePosWaterZ		= "c2";

$cLightIndex		= "c3";
$cLight0Offset		= "c3.x"; # 27
$cLight1Offset		= "c3.y"; # 32
$cColorToIntScale	= "c3.z"; # matrix array offset = 3.0f * 255.0f + 0.01 (epsilon ensures floor yields desired result)
$cModel0Index		= "c3.w"; # base for start of skinning matrices

; NOTE: These must match the same values in vsh_prep.pl!
$cModelViewProj0	= "c4";
$cModelViewProj1	= "c5";
$cModelViewProj2	= "c6";
$cModelViewProj3	= "c7";

$cViewProj0			= "c8";
$cViewProj1			= "c9";
$cViewProj2			= "c10";
$cViewProj3			= "c11";

; currently unused
; c12, c13

$SHADER_SPECIFIC_CONST_10 = "c14";
$SHADER_SPECIFIC_CONST_11 = "c15";

$cFogParams			= "c16";
$cFogEndOverFogRange = "c16.x";
$cFogOne			= "c16.y";
$cFogMaxDensity		= "c16.z";
$cOOFogRange		= "c16.w"; # (1/(fogEnd-fogStart))

$cViewModel0		= "c17";
$cViewModel1		= "c18";
$cViewModel2		= "c19";
$cViewModel3		= "c20";

$cAmbientColorPosX	= "c21";
$cAmbientColorNegX	= "c22";
$cAmbientColorPosY	= "c23";
$cAmbientColorNegY	= "c24";
$cAmbientColorPosZ	= "c25";
$cAmbientColorNegZ	= "c26";

$cAmbientColorPosXOffset = "21";
$cAmbientColorPosYOffset = "23";
$cAmbientColorPosZOffset = "25";

$cLight0DiffColor	= "c27";
$cLight0Dir			= "c28";
$cLight0Pos			= "c29";
$cLight0SpotParams  = "c30"; # [ exponent, stopdot, stopdot2, 1 / (stopdot - stopdot2)
$cLight0Atten		= "c31"; # [ constant, linear, quadratic, 0.0f ]

$cLight1DiffColor	= "c32";
$cLight1Dir			= "c33";
$cLight1Pos			= "c34";
$cLight1SpotParams  = "c35"; # [ exponent, stopdot, stopdot2, 1 / (stopdot - stopdot2)
$cLight1Atten		= "c36"; # [ constant, linear, quadratic, 0.0f ]

$cModulationColor	= "c37";

$SHADER_SPECIFIC_CONST_0  = "c38";
$SHADER_SPECIFIC_CONST_1  = "c39";
$SHADER_SPECIFIC_CONST_2  = "c40";
$SHADER_SPECIFIC_CONST_3  = "c41";
$SHADER_SPECIFIC_CONST_4  = "c42";
$SHADER_SPECIFIC_CONST_5  = "c43";
$SHADER_SPECIFIC_CONST_6  = "c44";
$SHADER_SPECIFIC_CONST_7  = "c45";
$SHADER_SPECIFIC_CONST_8  = "c46";
$SHADER_SPECIFIC_CONST_9  = "c47";
; $SHADER_SPECIFIC_CONST_10 is c14
; $SHADER_SPECIFIC_CONST_11 is c15

; There are 16 model matrices for skinning
; NOTE: These must match the same values in vsh_prep.pl!
$cModel0			= "c48";
$cModel1			= "c49";
$cModel2			= "c50";

sub OutputUsedRegisters
{
	local( $i );
	; USED REGISTERS
	for( $i = 0; $i < $g_NumRegisters; $i++ )
	{
		if( $g_allocated[$i] )
		{
			; $g_allocatedname[$i] = r$i
		}
	}
	;
}

sub AllocateRegister
{
	local( *reg ) = shift;
	local( $regname ) = shift;
	local( $i );
	for( $i = 0; $i < $g_NumRegisters; $i++ )
	{
		if( !$g_allocated[$i] )
		{
			$g_allocated[$i] = 1;
			$g_allocatedname[$i] = $regname;
			; AllocateRegister $regname = r$i
			$reg = "r$i";
			&OutputUsedRegisters();
			return;
		}
	}
	; Out of registers allocating $regname!
	$reg = "rERROR_OUT_OF_REGISTERS";
	&OutputUsedRegisters();
}

; pass in a reference to a var that contains a register. . ie \$var where var will constain "r1", etc
sub FreeRegister
{
	local( *reg ) = shift;
	local( $regname ) = shift;
	; FreeRegister $regname = $reg
	if( $reg =~ m/rERROR_DEALLOCATED/ )
	{
		; $regname already deallocated
		; $reg = "rALREADY_DEALLOCATED";
		&OutputUsedRegisters();
		return;
	}
;	if( $regname ne g_allocatedname[$reg] )
;	{
;		; Error freeing $reg
;		mov compileerror, freed unallocated register $regname
;	}

	if( ( $reg =~ m/r(.*)/ ) )
	{
		$g_allocated[$1] = 0;
	}
	$reg = "rERROR_DEALLOCATED";
	&OutputUsedRegisters();
}

sub CheckUnfreedRegisters()
{
	local( $i );
	for( $i = 0; $i < $g_NumRegisters; $i++ )
	{
		if( $g_allocated[$i] )
		{
			print "ERROR: r$i allocated to $g_allocatedname[$i] at end of program\n";
			$g_allocated[$i] = 0;
		}
	}
}

sub Normalize
{
	local( $r ) = shift;
	dp3 $r.w, $r, $r
	rsq $r.w, $r.w
	mul $r, $r, $r.w
}

sub Cross
{
	local( $result ) = shift;
	local( $a ) = shift;
	local( $b ) = shift;

	mul $result.xyz, $a.yzx, $b.zxy
	mad $result.xyz, -$b.yzx, $a.zxy, $result
}

sub RangeFog
{
	local( $projPos ) = shift;
	
	;------------------------------
	; Regular range fog
	;------------------------------

	; oFog.x = 1.0f = no fog
	; oFog.x = 0.0f = full fog
	; compute fog factor f = (fog_end - dist)*(1/(fog_end-fog_start))
	; this is == to: (fog_end/(fog_end-fog_start) - dist/(fog_end-fog_start)
	; which can be expressed with a single mad instruction!

	; Compute |projPos|
	local( $tmp );
	&AllocateRegister( \$tmp );
	dp3 $tmp.x, $projPos.xyw, $projPos.xyw
	rsq $tmp.x, $tmp.x
	rcp $tmp.x, $tmp.x

	if( $g_dx9 )
	{
		mad $tmp, -$tmp.x, $cOOFogRange, $cFogEndOverFogRange
		min $tmp, $tmp, $cOne
		max oFog, $tmp.x, $cFogMaxDensity
	}
	else
	{
		mad $tmp, -$tmp.x, $cOOFogRange, $cFogEndOverFogRange
		min $tmp, $tmp, $cOne
		max oFog.x, $tmp.x, $cFogMaxDensity
	}
	&FreeRegister( \$tmp );
}

sub DepthFog
{
	local( $projPos ) = shift;
	local( $dest ) = shift;

	if ( $dest eq "" )
	{
		$dest = "oFog";
	}

	;------------------------------
	; Regular range fog
	;------------------------------

	; oFog.x = 1.0f = no fog
	; oFog.x = 0.0f = full fog
	; compute fog factor f = (fog_end - dist)*(1/(fog_end-fog_start))
	; this is == to: (fog_end/(fog_end-fog_start) - dist/(fog_end-fog_start)
	; which can be expressed with a single mad instruction!

	; Compute |projPos|
	local( $tmp );
	&AllocateRegister( \$tmp );

	if( $g_dx9 )
	{
		mad $tmp, -$projPos.w, $cOOFogRange, $cFogEndOverFogRange
		min $tmp, $tmp, $cOne
		max $dest, $tmp.x, $cFogMaxDensity
	}
	else
	{
		mad $tmp, -$projPos.w, $cOOFogRange, $cFogEndOverFogRange
		min $tmp, $tmp, $cOne
		max $dest.x, $tmp.x, $cFogMaxDensity
	}

	&FreeRegister( \$tmp );
}

sub WaterRangeFog
{
	; oFog.x = 1.0f = no fog
	; oFog.x = 0.0f = full fog

	; only $worldPos.z is used out of worldPos
	local( $worldPos ) = shift;
	local( $projPos ) = shift;
	
	local( $tmp );
	&AllocateRegister( \$tmp );

	; This is simple similar triangles. Imagine a line passing from the point directly vertically
	; and another line passing from the point to the eye position.
	; Let d = total distance from point to the eye
	; Let h = vertical distance from the point to the eye
	; Let hw = vertical distance from the point to the water surface
	; Let dw = distance from the point to a point on the water surface that lies along the ray from point to eye
	; Therefore d/h = dw/hw by similar triangles, or dw = d * hw / h.
	; d = |projPos|, h = eyepos.z - worldPos.z, hw = waterheight.z - worldPos.z, dw = what we solve for

	; Now, tmp.x = hw, and tmp.y = h
	add $tmp.xy, $cEyePosWaterZ.wz, -$worldPos.z

	; if $tmp.x < 0, then set it to 0
	; This is the equivalent of moving the vert to the water surface if it's above the water surface
	max $tmp.x, $tmp.x, $cZero

	; Compute 1 / |projPos| = 1/d
	dp3 $tmp.z, $projPos.xyw, $projPos.xyw
	rsq $tmp.z, $tmp.z

	; Now we have h/d
	mul $tmp.z, $tmp.z, $tmp.y

	; Now we have d/h
	rcp $tmp.w, $tmp.z
	
	; We finally have d * hw / h
	; $tmp.w is now the distance that we see through water.
	mul $tmp.w, $tmp.x, $tmp.w

	if( $g_dx9 )
	{
		mad $tmp, -$tmp.w, $cOOFogRange, $cFogOne
		min $tmp, $tmp, $cOne
		max oFog, $tmp.x, $cFogMaxDensity
	}
	else
	{
		mad $tmp, -$tmp.w, $cOOFogRange, $cFogOne
		min $tmp, $tmp, $cOne
		max oFog.x, $tmp.x, $cFogMaxDensity
	}

	&FreeRegister( \$tmp );
}

sub WaterDepthFog
{
	; oFog.x = 1.0f = no fog
	; oFog.x = 0.0f = full fog

	; only $worldPos.z is used out of worldPos
	local( $worldPos ) = shift;
	local( $projPos ) = shift;
	local( $dest ) = shift;
	
	if ( $dest eq "" )
	{
		$dest = "oFog";
	}
	
	local( $tmp );
	&AllocateRegister( \$tmp );

	; This is simple similar triangles. Imagine a line passing from the point directly vertically
	; and another line passing from the point to the eye position.
	; Let d = total distance from point to the eye
	; Let h = vertical distance from the point to the eye
	; Let hw = vertical distance from the point to the water surface
	; Let dw = distance from the point to a point on the water surface that lies along the ray from point to eye
	; Therefore d/h = dw/hw by similar triangles, or dw = d * hw / h.
	; d = projPos.w, h = eyepos.z - worldPos.z, hw = waterheight.z - worldPos.z, dw = what we solve for

	; Now, tmp.x = hw, and tmp.y = h
	add $tmp.xy, $cEyePosWaterZ.wz, -$worldPos.z

	; if $tmp.x < 0, then set it to 0
	; This is the equivalent of moving the vert to the water surface if it's above the water surface
	max $tmp.x, $tmp.x, $cZero

	; Now we have 1/h
	rcp $tmp.z, $tmp.y

	; Now we have d/h
	mul $tmp.w, $projPos.w, $tmp.z

	; We finally have d * hw / h
	; $tmp.w is now the distance that we see through water.
	mul $tmp.w, $tmp.x, $tmp.w

	if( $g_dx9 )
	{
		mad $tmp, -$tmp.w, $cOOFogRange, $cFogOne
		min $tmp, $tmp, $cOne
		max $dest, $tmp.x, $cZero
	}
	else
	{
		mad $tmp, -$tmp.w, $cOOFogRange, $cFogOne
		min $tmp, $tmp, $cOne
		max $dest.x, $tmp.x, $cZero
	}

	&FreeRegister( \$tmp );
}


;------------------------------------------------------------------------------
; Main fogging routine
;------------------------------------------------------------------------------
sub CalcFog
{
	if( !defined $DOWATERFOG )
	{
		die "CalcFog called without using \$DOWATERFOG\n";
	}
	my $fogType;
	if( $DOWATERFOG == 0 )
	{
		$fogType = "rangefog";		
	}
	else
	{
		$fogType = "heightfog";
	}

#	print "\$fogType = $fogType\n";

	; CalcFog
	local( $worldPos ) = shift;
	local( $projPos ) = shift;
	local( $dest ) = shift;

	if ( $dest eq "" )
	{
		$dest = "oFog";
	}

	if( $fogType eq "rangefog" )
	{
		&DepthFog( $projPos, $dest );
	}
	elsif( $fogType eq "heightfog" )
	{
		&WaterDepthFog( $worldPos, $projPos, $dest );
	}
	else
	{
		die;
	}	
}

sub CalcRangeFog
{
	; CalcFog
	local( $worldPos ) = shift;
	local( $projPos ) = shift;

	if( $DOWATERFOG == 0 )
	{
		&RangeFog( $projPos );
	}
	elsif( $DOWATERFOG == 1 )
	{
		&WaterRangeFog( $worldPos, $projPos );
	}
	else
	{
		die;
	}	
}

sub GammaToLinear
{
	local( $gamma ) = shift;
	local( $linear ) = shift;

	local( $tmp );
	&AllocateRegister( \$tmp );

	; Is rcp more expensive than just storing 2.2 somewhere and doing a mov?
	rcp $gamma.w, $cOOGamma							; $gamma.w = 2.2
	lit $linear.z, $gamma.zzzw						; r0.z = linear blue
	lit $tmp.z, $gamma.yyyw							; r2.z = linear green
	mov $linear.y, $tmp.z							; r0.y = linear green
	lit $tmp.z, $gamma.xxxw							; r2.z = linear red
	mov $linear.x, $tmp.z							; r0.x = linear red

	&FreeRegister( \$tmp );
}

sub LinearToGamma
{
	local( $linear ) = shift;
	local( $gamma ) = shift;

	local( $tmp );
	&AllocateRegister( \$tmp );

	mov $linear.w, $cOOGamma						; $linear.w = 1.0/2.2
	lit $gamma.z, $linear.zzzw						; r0.z = gamma blue
	lit $tmp.z, $linear.yyyw						; r2.z = gamma green
	mov $gamma.y, $tmp.z							; r0.y = gamma green
	lit $tmp.z, $linear.xxxw						; r2.z = gamma red
	mov $gamma.x, $tmp.z							; r0.x = gamma red

	&FreeRegister( \$tmp );
}

sub ComputeReflectionVector
{
	local( $worldPos ) = shift;
	local( $worldNormal ) = shift;
	local( $reflectionVector ) = shift;

	local( $vertToEye ); &AllocateRegister( \$vertToEye );
	local( $tmp ); &AllocateRegister( \$tmp );

	; compute reflection vector r = 2 * (n dot v) n - v
	sub $vertToEye.xyz, $cEyePos.xyz, $worldPos  ; $tmp1 = v = c - p
	dp3 $tmp, $worldNormal, $vertToEye			; $tmp = n dot v
	mul $tmp.xyz, $tmp.xyz, $worldNormal	; $tmp = (n dot v ) n
	mad $reflectionVector.xyz, $tmp, $cTwo, -$vertToEye

	&FreeRegister( \$vertToEye );
	&FreeRegister( \$tmp );
}

sub ComputeSphereMapTexCoords
{
	local( $reflectionVector ) = shift;
	local( $sphereMapTexCoords ) = shift;

	local( $tmp ); &AllocateRegister( \$tmp );

	; transform reflection vector into view space
	dp3 $tmp.x, $reflectionVector, $cViewModel0
	dp3 $tmp.y, $reflectionVector, $cViewModel1
	dp3 $tmp.z, $reflectionVector, $cViewModel2

	; generate <rx ry rz+1>
	add $tmp.z, $tmp.z, $cOne

	; find 1 / the length of r2
	dp3 $tmp.w, $tmp, $tmp
	rsq $tmp.w, $tmp.w

	; r1 = r2/|r2| + 1
	mad $tmp.xy, $tmp.w, $tmp, $cOne
	mul $sphereMapTexCoords.xy, $tmp.xy, $cHalf
	
	&FreeRegister( \$tmp );
}

sub SkinPosition
{
#	print "\$SKINNING = $SKINNING\n";
	local( $worldPos ) = shift;
	
	if( !defined $SKINNING )
	{
		die "using \$SKINNING without defining.\n";
	}
		
	if( $SKINNING == 0 )
	{
		;
		; 0 bone skinning (4 instructions)
		;
		; Transform position into world space
		; position
		dp4 $worldPos.x, $vPos, $cModel0
		dp4 $worldPos.y, $vPos, $cModel1
		dp4 $worldPos.z, $vPos, $cModel2
		mov $worldPos.w, $cOne
	} 
	else
	{
		;
		; 3 bone skinning  (19 instructions)
		;
		local( $boneIndices );
		local( $blendedMatrix0 );
		local( $blendedMatrix1 );
		local( $blendedMatrix2 );
		local( $localPos );
		&AllocateRegister( \$boneIndices );
		&AllocateRegister( \$blendedMatrix0 );
		&AllocateRegister( \$blendedMatrix1 );
		&AllocateRegister( \$blendedMatrix2 );

		; Transform position into world space using all bones
		; denormalize d3dcolor to matrix index
		mad $boneIndices, $vBoneIndices, $cColorToIntScale, $cModel0Index
		if ( $g_x360 )
		{
			mov $boneIndices, $boneIndices.zyxw
		}
		
		; r11 = boneindices at this point
		; first matrix
		mov a0.x, $boneIndices.z
		mul $blendedMatrix0, $vBoneWeights.x, c[a0.x]
		mul $blendedMatrix1, $vBoneWeights.x, c[a0.x+1]
		mul $blendedMatrix2, $vBoneWeights.x, c[a0.x+2]
		; second matrix
		mov a0.x, $boneIndices.y
		mad $blendedMatrix0, $vBoneWeights.y, c[a0.x], $blendedMatrix0
		mad $blendedMatrix1, $vBoneWeights.y, c[a0.x+1], $blendedMatrix1
		mad $blendedMatrix2, $vBoneWeights.y, c[a0.x+2], $blendedMatrix2

		; Calculate third weight
		; compute 1-(weight1+weight2) to calculate weight2
		; Use $boneIndices.w as a temp since we aren't using it for anything.
		add $boneIndices.w, $vBoneWeights.x, $vBoneWeights.y
		sub $boneIndices.w, $cOne, $boneIndices.w

		; third matrix
		mov a0.x, $boneIndices.x
		mad $blendedMatrix0, $boneIndices.w, c[a0.x], $blendedMatrix0
		mad $blendedMatrix1, $boneIndices.w, c[a0.x+1], $blendedMatrix1
		mad $blendedMatrix2, $boneIndices.w, c[a0.x+2], $blendedMatrix2
		
		dp4 $worldPos.x, $vPos, $blendedMatrix0
		dp4 $worldPos.y, $vPos, $blendedMatrix1
		dp4 $worldPos.z, $vPos, $blendedMatrix2
		mov $worldPos.w, $cOne

		&FreeRegister( \$boneIndices );
		&FreeRegister( \$blendedMatrix0 );
		&FreeRegister( \$blendedMatrix1 );
		&FreeRegister( \$blendedMatrix2 );
	}
}


sub SkinPositionAndNormal
{
#	print "\$SKINNING = $SKINNING\n";
	local( $worldPos ) = shift;
	local( $worldNormal ) = shift;

	if( !defined $SKINNING )
	{
		die "using \$SKINNING without defining.\n";
	}

	if( $SKINNING == 0 )
	{
		;
		; 0 bone skinning (13 instructions)
		;
		; Transform position + normal + tangentS + tangentT into world space
		; position
		dp4 $worldPos.x, $vPos, $cModel0
		dp4 $worldPos.y, $vPos, $cModel1
		dp4 $worldPos.z, $vPos, $cModel2
		mov $worldPos.w, $cOne
		; normal
		dp3 $worldNormal.x, $vNormal, $cModel0
		dp3 $worldNormal.y, $vNormal, $cModel1
		dp3 $worldNormal.z, $vNormal, $cModel2
	}
	else
	{
		local( $boneIndices );
		local( $blendedMatrix0 );
		local( $blendedMatrix1 );
		local( $blendedMatrix2 );
		local( $localPos );
		local( $localNormal );
		local( $normalLength );
		local( $ooNormalLength );
		&AllocateRegister( \$boneIndices );
		&AllocateRegister( \$blendedMatrix0 );
		&AllocateRegister( \$blendedMatrix1 );
		&AllocateRegister( \$blendedMatrix2 );

		; Transform position into world space using all bones
		; denormalize d3dcolor to matrix index
		mad $boneIndices, $vBoneIndices, $cColorToIntScale, $cModel0Index
		if ( $g_x360 )
		{
			mov $boneIndices, $boneIndices.zyxw
		}

		; r11 = boneindices at this point
		; first matrix
		mov a0.x, $boneIndices.z
		mul $blendedMatrix0, $vBoneWeights.x, c[a0.x]
		mul $blendedMatrix1, $vBoneWeights.x, c[a0.x+1]
		mul $blendedMatrix2, $vBoneWeights.x, c[a0.x+2]
		; second matrix
		mov a0.x, $boneIndices.y
		mad $blendedMatrix0, $vBoneWeights.y, c[a0.x], $blendedMatrix0
		mad $blendedMatrix1, $vBoneWeights.y, c[a0.x+1], $blendedMatrix1
		mad $blendedMatrix2, $vBoneWeights.y, c[a0.x+2], $blendedMatrix2

		; Calculate third weight
		; compute 1-(weight1+weight2) to calculate weight2
		; Use $boneIndices.w as a temp since we aren't using it for anything.
		add $boneIndices.w, $vBoneWeights.x, $vBoneWeights.y
		sub $boneIndices.w, $cOne, $boneIndices.w

		; third matrix
		mov a0.x, $boneIndices.x
		mad $blendedMatrix0, $boneIndices.w, c[a0.x], $blendedMatrix0
		mad $blendedMatrix1, $boneIndices.w, c[a0.x+1], $blendedMatrix1
		mad $blendedMatrix2, $boneIndices.w, c[a0.x+2], $blendedMatrix2
		
		dp4 $worldPos.x, $vPos, $blendedMatrix0
		dp4 $worldPos.y, $vPos, $blendedMatrix1
		dp4 $worldPos.z, $vPos, $blendedMatrix2
		mov $worldPos.w, $cOne

		; normal
		dp3 $worldNormal.x, $vNormal, $blendedMatrix0
		dp3 $worldNormal.y, $vNormal, $blendedMatrix1
		dp3 $worldNormal.z, $vNormal, $blendedMatrix2

		&FreeRegister( \$boneIndices );
		&FreeRegister( \$blendedMatrix0 );
		&FreeRegister( \$blendedMatrix1 );
		&FreeRegister( \$blendedMatrix2 );
	}	
}

sub SkinPositionNormalAndTangentSpace
{
#	print "\$SKINNING = $SKINNING\n";
	local( $worldPos ) = shift;
	local( $worldNormal ) = shift;
	local( $worldTangentS ) = shift;
	local( $worldTangentT ) = shift;
	local( $userData );
	local( $localPos );
	local( $localNormal );
	local( $normalLength );
	local( $ooNormalLength );
	
	if( !defined $SKINNING )
	{
		die "using \$SKINNING without defining.\n";
	}

# X360TBD: needed for compressed vertex format
#	if ( $g_x360 )
#	{
#		&AllocateRegister( \$userData );
#		; remap compressed range [0..1] to [-1..1]
#		mad $userData, $vUserData, $cTwo, -$cOne
#	}

	if( $SKINNING == 0 )
	{
		;
		; 0 bone skinning (13 instructions)
		;
		; Transform position + normal + tangentS + tangentT into world space
		dp4 $worldPos.x, $vPos, $cModel0
		dp4 $worldPos.y, $vPos, $cModel1
		dp4 $worldPos.z, $vPos, $cModel2
		mov $worldPos.w, $cOne

		; normal
		dp3 $worldNormal.x, $vNormal, $cModel0
		dp3 $worldNormal.y, $vNormal, $cModel1
		dp3 $worldNormal.z, $vNormal, $cModel2

# X360TBD: needed for compressed vertex format
#		if ( $g_x360 )
#		{
#			; tangents
#			dp3 $worldTangentS.x, $userData, $cModel0
#			dp3 $worldTangentS.y, $userData, $cModel1
#			dp3 $worldTangentS.z, $userData, $cModel2
#
#			; calculate tangent t via cross( N, S ) * S[3]
#			&Cross( $worldTangentT, $worldNormal, $worldTangentS );
#			mul $worldTangentT.xyz, $userData.w, $worldTangentT.xyz
#		}
#		else
		{
			; tangents
			dp3 $worldTangentS.x, $vUserData, $cModel0
			dp3 $worldTangentS.y, $vUserData, $cModel1
			dp3 $worldTangentS.z, $vUserData, $cModel2

			; calculate tangent t via cross( N, S ) * S[3]
			&Cross( $worldTangentT, $worldNormal, $worldTangentS );
			mul $worldTangentT.xyz, $vUserData.w, $worldTangentT.xyz
		}
	}
	else
	{
		local( $boneIndices );
		local( $blendedMatrix0 );
		local( $blendedMatrix1 );
		local( $blendedMatrix2 );
		&AllocateRegister( \$boneIndices );
		&AllocateRegister( \$blendedMatrix0 );
		&AllocateRegister( \$blendedMatrix1 );
		&AllocateRegister( \$blendedMatrix2 );

		; Transform position into world space using all bones
		; denormalize d3dcolor to matrix index
		mad $boneIndices, $vBoneIndices, $cColorToIntScale, $cModel0Index
		if ( $g_x360 )
		{
			mov $boneIndices, $boneIndices.zyxw
		}

		; r11 = boneindices at this point
		; first matrix
		mov a0.x, $boneIndices.z
		mul $blendedMatrix0, $vBoneWeights.x, c[a0.x]
		mul $blendedMatrix1, $vBoneWeights.x, c[a0.x+1]
		mul $blendedMatrix2, $vBoneWeights.x, c[a0.x+2]
		; second matrix
		mov a0.x, $boneIndices.y
		mad $blendedMatrix0, $vBoneWeights.y, c[a0.x], $blendedMatrix0
		mad $blendedMatrix1, $vBoneWeights.y, c[a0.x+1], $blendedMatrix1
		mad $blendedMatrix2, $vBoneWeights.y, c[a0.x+2], $blendedMatrix2

		; Calculate third weight
		; compute 1-(weight1+weight2) to calculate weight2
		; Use $boneIndices.w as a temp since we aren't using it for anything.
		add $boneIndices.w, $vBoneWeights.x, $vBoneWeights.y
		sub $boneIndices.w, $cOne, $boneIndices.w

		; third matrix
		mov a0.x, $boneIndices.x
		mad $blendedMatrix0, $boneIndices.w, c[a0.x], $blendedMatrix0
		mad $blendedMatrix1, $boneIndices.w, c[a0.x+1], $blendedMatrix1
		mad $blendedMatrix2, $boneIndices.w, c[a0.x+2], $blendedMatrix2
		
		; position
		dp4 $worldPos.x, $vPos, $blendedMatrix0
		dp4 $worldPos.y, $vPos, $blendedMatrix1
		dp4 $worldPos.z, $vPos, $blendedMatrix2
		mov $worldPos.w, $cOne

		; normal
		dp3 $worldNormal.x, $vNormal, $blendedMatrix0
		dp3 $worldNormal.y, $vNormal, $blendedMatrix1
		dp3 $worldNormal.z, $vNormal, $blendedMatrix2

# X360TBD: needed for compressed vertex format
#		if ( $g_x360 )
#		{
#			; tangents
#			dp3 $worldTangentS.x, $userData, $blendedMatrix0
#			dp3 $worldTangentS.y, $userData, $blendedMatrix1
#			dp3 $worldTangentS.z, $userData, $blendedMatrix2
#
#			; calculate tangent t via cross( N, S ) * S[3]
#			&Cross( $worldTangentT, $worldNormal, $worldTangentS );
#			mul $worldTangentT.xyz, $userData.w, $worldTangentT.xyz
#		}
#		else
		{
			; tangents
			dp3 $worldTangentS.x, $vUserData, $blendedMatrix0
			dp3 $worldTangentS.y, $vUserData, $blendedMatrix1
			dp3 $worldTangentS.z, $vUserData, $blendedMatrix2

			; calculate tangent t via cross( N, S ) * S[3]
			&Cross( $worldTangentT, $worldNormal, $worldTangentS );
			mul $worldTangentT.xyz, $vUserData.w, $worldTangentT.xyz
		}

		&FreeRegister( \$boneIndices );
		&FreeRegister( \$blendedMatrix0 );
		&FreeRegister( \$blendedMatrix1 );
		&FreeRegister( \$blendedMatrix2 );
	}

# X360TBD: needed for compressed vertex format
#	if ( $g_x360 )
#	{
#		&FreeRegister( \$userData );
#	}
}

sub ColorClamp
{
	; ColorClamp; stomps $color.w
	local( $color ) = shift;
	local( $dst ) = shift;

	; Get the max of RGB and stick it in W
	max $color.w, $color.x, $color.y
	max $color.w, $color.w, $color.z

	; get the greater of one and the max color.
	max $color.w, $color.w, $cOne

	rcp $color.w, $color.w
	mul $dst.xyz, $color.w, $color.xyz
}

sub AmbientLight
{
	local( $worldNormal ) = shift;
	local( $linearColor ) = shift;
	local( $add ) = shift;

	; Ambient lighting
	&AllocateRegister( \$nSquared );
	&AllocateRegister( \$isNegative );

	mul $nSquared.xyz, $worldNormal.xyz, $worldNormal.xyz				; compute n times n
	slt $isNegative.xyz, $worldNormal.xyz, $cZero				; Figure out whether each component is >0
	mov a0.x, $isNegative.x
	if( $add )
	{
		mad $linearColor.xyz, $nSquared.x, c[a0.x + $cAmbientColorPosXOffset], $linearColor			; $linearColor = normal[0]*normal[0] * box color of appropriate x side
	}
	else
	{
		mul $linearColor.xyz, $nSquared.x, c[a0.x + $cAmbientColorPosXOffset]			; $linearColor = normal[0]*normal[0] * box color of appropriate x side
	}
	mov a0.x, $isNegative.y
	mad $linearColor.xyz, $nSquared.y, c[a0.x + $cAmbientColorPosYOffset], $linearColor
	mov a0.x, $isNegative.z
	mad $linearColor.xyz, $nSquared.z, c[a0.x + $cAmbientColorPosZOffset], $linearColor

	&FreeRegister( \$isNegative );
	&FreeRegister( \$nSquared );
}

sub DirectionalLight
{
	local( $worldNormal ) = shift;
	local( $linearColor ) = shift;
	local( $add ) = shift;

	&AllocateRegister( \$nDotL ); # FIXME: This only needs to be a scalar

	; NOTE: Gotta use -l here, since light direction = -l
	; DIRECTIONAL LIGHT
	; compute n dot l
	dp3 $nDotL.x, -c[a0.x + 1], $worldNormal
	
	if ( $HALF_LAMBERT == 0 )
	{
		; lambert
		max $nDotL.x, $nDotL.x, c0.x				; Clamp to zero
	}
	elsif ( $HALF_LAMBERT == 1 )
	{
		; half-lambert
		mad $nDotL.x, $nDotL.x, $cHalf, $cHalf		; dot = (dot * 0.5 + 0.5)^2
		mul $nDotL.x, $nDotL.x, $nDotL.x
	}
	else
	{
		die "\$HALF_LAMBERT is hosed\n";
	}
  
	if( $add )
	{
		mad $linearColor.xyz, c[a0.x], $nDotL.x, $linearColor
	}
	else
	{
		mul $linearColor.xyz, c[a0.x], $nDotL.x
	}

	&FreeRegister( \$nDotL );
}

sub PointLight
{
	local( $worldPos ) = shift;
	local( $worldNormal ) = shift;
	local( $linearColor ) = shift;
	local( $add ) = shift;

	local( $lightDir );
	&AllocateRegister( \$lightDir );
	
	; POINT LIGHT
	; compute light direction
	sub $lightDir, c[a0.x+2], $worldPos
	
	local( $lightDistSquared );
	local( $ooLightDist );
	&AllocateRegister( \$lightDistSquared );
	&AllocateRegister( \$ooLightDist );

	; normalize light direction, maintain temporaries for attenuation
	dp3 $lightDistSquared, $lightDir, $lightDir
	rsq $ooLightDist, $lightDistSquared.x
	mul $lightDir, $lightDir, $ooLightDist.x
	
	local( $attenuationFactors );
	&AllocateRegister( \$attenuationFactors );

	; compute attenuation amount (r2 = 'd*d d*d d*d d*d', r3 = '1/d 1/d 1/d 1/d')
	dst $attenuationFactors, $lightDistSquared, $ooLightDist						; r4 = ( 1, d, d*d, 1/d )
	&FreeRegister( \$lightDistSquared );
	&FreeRegister( \$ooLightDist );
	local( $attenuation );
	&AllocateRegister( \$attenuation );
	dp3 $attenuation, $attenuationFactors, c[a0.x+4]				; r3 = atten0 + d * atten1 + d*d * atten2

	rcp $lightDir.w, $attenuation						; $lightDir.w = 1 / (atten0 + d * atten1 + d*d * atten2)

	&FreeRegister( \$attenuationFactors );
	&FreeRegister( \$attenuation );
	
	local( $tmp );
	&AllocateRegister( \$tmp ); # FIXME : really only needs to be a scalar

	; compute n dot l, fold in distance attenutation
	dp3 $tmp.x, $lightDir, $worldNormal

	if ( $HALF_LAMBERT == 0 )
	{
		; lambert
		max $tmp.x, $tmp.x, c0.x				; Clamp to zero
	}
	elsif ( $HALF_LAMBERT == 1 )
	{
		; half-lambert
		mad $tmp.x, $tmp.x, $cHalf, $cHalf		; dot = (dot * 0.5 + 0.5)^2
		mul $tmp.x, $tmp.x, $tmp.x
	}
	else
	{
		die "\$HALF_LAMBERT is hosed\n";
	}
	
	mul $tmp.x, $tmp.x, $lightDir.w
	if( $add )
	{
		mad $linearColor.xyz, c[a0.x], $tmp.x, $linearColor
	}
	else
	{
		mul $linearColor.xyz, c[a0.x], $tmp.x
	}

	&FreeRegister( \$lightDir );
	&FreeRegister( \$tmp ); # FIXME : really only needs to be a scalar
}

sub SpotLight
{
	local( $worldPos ) = shift;
	local( $worldNormal ) = shift;
	local( $linearColor ) = shift;
	local( $add ) = shift;
	
	local( $lightDir );
	&AllocateRegister( \$lightDir );

	; SPOTLIGHT
	; compute light direction
	sub $lightDir, c[a0.x+2], $worldPos
	
	local( $lightDistSquared );
	local( $ooLightDist );
	&AllocateRegister( \$lightDistSquared );
	&AllocateRegister( \$ooLightDist );

	; normalize light direction, maintain temporaries for attenuation
	dp3 $lightDistSquared, $lightDir, $lightDir
	rsq $ooLightDist, $lightDistSquared.x
	mul $lightDir, $lightDir, $ooLightDist.x
	
	local( $attenuationFactors );
	&AllocateRegister( \$attenuationFactors );

	; compute attenuation amount (r2 = 'd*d d*d d*d d*d', r3 = '1/d 1/d 1/d 1/d')
	dst $attenuationFactors, $lightDistSquared, $ooLightDist						; r4 = ( 1, d, d*d, 1/d )

	&FreeRegister( \$lightDistSquared );
	&FreeRegister( \$ooLightDist );
	local( $attenuation );	&AllocateRegister( \$attenuation );

	dp3 $attenuation, $attenuationFactors, c[a0.x+4]				; r3 = atten0 + d * atten1 + d*d * atten2
	rcp $lightDir.w, $attenuation						; r1.w = 1 / (atten0 + d * atten1 + d*d * atten2)

	&FreeRegister( \$attenuationFactors );
	&FreeRegister( \$attenuation );
	
	local( $litSrc ); &AllocateRegister( \$litSrc );
	local( $tmp ); &AllocateRegister( \$tmp ); # FIXME - only needs to be scalar

	; compute n dot l
	dp3 $litSrc.x, $worldNormal, $lightDir
	
	if ( $HALF_LAMBERT == 0 )
	{
		; lambert
		max $litSrc.x, $litSrc.x, c0.x				; Clamp to zero
	}
	elsif ( $HALF_LAMBERT == 1 )
	{
		; half-lambert
		mad $litSrc.x, $litSrc.x, $cHalf, $cHalf	; dot = (dot * 0.5 + 0.5) ^ 2
		mul $litSrc.x, $litSrc.x, $litSrc.x
	}
	else
	{
		die "\$HALF_LAMBERT is hosed\n";
	}

	; compute angular attenuation
	dp3 $tmp.x, c[a0.x+1], -$lightDir				; dot = -delta * spot direction
	sub $litSrc.y, $tmp.x, c[a0.x+3].z				; r2.y = dot - stopdot2
	&FreeRegister( \$tmp );
	mul $litSrc.y, $litSrc.y, c[a0.x+3].w			; r2.y = (dot - stopdot2) / (stopdot - stopdot2)
	mov $litSrc.w, c[a0.x+3].x						; r2.w = exponent
	local( $litDst ); &AllocateRegister( \$litDst );
	lit $litDst, $litSrc							; r3.y = N dot L or 0, whichever is bigger
	&FreeRegister( \$litSrc );
													; r3.z = pow((dot - stopdot2) / (stopdot - stopdot2), exponent)
	min $litDst.z, $litDst.z, $cOne		 			; clamp pow() to 1
	
	local( $tmp1 ); &AllocateRegister( \$tmp1 );
	local( $tmp2 ); &AllocateRegister( \$tmp2 );  # FIXME - could be scalar

	; fold in distance attenutation with other factors
	mul $tmp1, c[a0.x], $lightDir.w
	mul $tmp2.x, $litDst.y, $litDst.z
	if( $add )
	{
		mad $linearColor.xyz, $tmp1, $tmp2.x, $linearColor
	}
	else
	{
		mul $linearColor.xyz, $tmp1, $tmp2.x
	}

	&FreeRegister( \$lightDir );
	&FreeRegister( \$litDst );
	&FreeRegister( \$tmp1 );
	&FreeRegister( \$tmp2 );
}

sub DoLight
{
	local( $lightType ) = shift;
	local( $worldPos ) = shift;
	local( $worldNormal ) = shift;
	local( $linearColor ) = shift;
	local( $add ) = shift;

	if( $lightType eq "spot" )
	{
		&SpotLight( $worldPos, $worldNormal, $linearColor, $add );
	}
	elsif( $lightType eq "point" )
	{
		&PointLight( $worldPos, $worldNormal, $linearColor, $add );
	}
	elsif( $lightType eq "directional" )
	{
		&DirectionalLight( $worldNormal, $linearColor, $add );
	}
	else
	{
		die "don't know about light type \"$lightType\"\n";
	}
}

sub DoLighting
{
	if( !defined $LIGHT_COMBO )
	{
		die "DoLighting called without using \$LIGHT_COMBO\n";
	}
	if ( !defined $HALF_LAMBERT )
	{
		die "DoLighting called without using \$HALF_LAMBERT\n";
	}

	my $staticLightType = $g_staticLightTypeArray[$LIGHT_COMBO];
	my $ambientLightType = $g_ambientLightTypeArray[$LIGHT_COMBO];
	my $localLightType1 = $g_localLightType1Array[$LIGHT_COMBO];
	my $localLightType2 = $g_localLightType2Array[$LIGHT_COMBO];

#	print "\$staticLightType = $staticLightType\n";
#	print "\$ambientLightType = $ambientLightType\n";
#	print "\$localLightType1 = $localLightType1\n";
#	print "\$localLightType2 = $localLightType2\n";

	local( $worldPos ) = shift;
	local( $worldNormal ) = shift;

	; special case for no lighting
	if( $staticLightType eq "none" && $ambientLightType eq "none" &&
		$localLightType1 eq "none" && $localLightType2 eq "none" )
	{
		; Have to write something here since debug d3d runtime will barf otherwise.
		mov oD0, $cOne
		return;
	}

	; special case for static lighting only
	; Don't need to bother converting to linear space in this case.
	if( $staticLightType eq "static" && $ambientLightType eq "none" &&
		$localLightType1 eq "none" && $localLightType2 eq "none" )
	{
		mov oD0, $vSpecular
		return;
	}

	alloc $linearColor
	alloc $gammaColor

	local( $add ) = 0;
	if( $staticLightType eq "static" )
	{
		; The static lighting comes in in gamma space and has also been premultiplied by $cOverbrightFactor
		; need to get it into
		; linear space so that we can do adds.
		rcp $gammaColor.w, $cOverbrightFactor
		mul $gammaColor.xyz, $vSpecular, $gammaColor.w
		&GammaToLinear( $gammaColor, $linearColor );
		$add = 1;
	}

	if( $ambientLightType eq "ambient" )
	{
		&AmbientLight( $worldNormal, $linearColor, $add );
		$add = 1;
	}

	if( $localLightType1 ne "none" )
	{
		mov a0.x, $cLight0Offset
		&DoLight( $localLightType1, $worldPos, $worldNormal, $linearColor, $add );
		$add = 1;
	}

	if( $localLightType2 ne "none" )
	{
		mov a0.x, $cLight1Offset
		&DoLight( $localLightType2, $worldPos, $worldNormal, $linearColor, $add );
		$add = 1;
	}

	;------------------------------------------------------------------------------
	; Output color (gamma correction)
	;------------------------------------------------------------------------------

	&LinearToGamma( $linearColor, $gammaColor );
	if( 0 )
	{
		mul oD0.xyz, $gammaColor.xyz, $cOverbrightFactor
	}
	else
	{
		mul $gammaColor.xyz, $gammaColor.xyz, $cOverbrightFactor
		&ColorClamp( $gammaColor, "oD0" );
	}

;	mov oD0.xyz, $linearColor
	mov oD0.w, $cOne				; make sure all components are defined

	free $linearColor
	free $gammaColor
}

sub DoDynamicLightingToLinear
{
	local( $worldPos ) = shift;
	local( $worldNormal ) = shift;
	local( $linearColor ) = shift;

	if( !defined $LIGHT_COMBO )
	{
		die "DoLighting called without using \$LIGHT_COMBO\n";
	}
	if ( !defined $HALF_LAMBERT )
	{
		die "DoLighting called without using \$HALF_LAMBERT\n";
	}

	my $staticLightType = $g_staticLightTypeArray[$LIGHT_COMBO];
	my $ambientLightType = $g_ambientLightTypeArray[$LIGHT_COMBO];
	my $localLightType1 = $g_localLightType1Array[$LIGHT_COMBO];
	my $localLightType2 = $g_localLightType2Array[$LIGHT_COMBO];

	# No lights at all. . note that we don't even consider static lighting here.
	if( $ambientLightType eq "none" &&
		$localLightType1 eq "none" && $localLightType2 eq "none" )
	{
		mov $linearColor, $cZero
		return;
	}

	local( $add ) = 0;
	if( $ambientLightType eq "ambient" )
	{
		&AmbientLight( $worldNormal, $linearColor, $add );
		$add = 1;
	}

	if( $localLightType1 ne "none" )
	{
		mov a0.x, $cLight0Offset
		&DoLight( $localLightType1, $worldPos, $worldNormal, $linearColor, $add );
		$add = 1;
	}

	if( $localLightType2 ne "none" )
	{
		mov a0.x, $cLight1Offset
		&DoLight( $localLightType2, $worldPos, $worldNormal, $linearColor, $add );
		$add = 1;
	}
}

sub NotImplementedYet
{
	&AllocateRegister( \$projPos );
	dp4 $projPos.x, $worldPos, $cViewProj0
	dp4 $projPos.y, $worldPos, $cViewProj1
	dp4 $projPos.z, $worldPos, $cViewProj2
	dp4 $projPos.w, $worldPos, $cViewProj3
	mov oPos, $projPos
	&FreeRegister( \$projPos );
	exit;
}
