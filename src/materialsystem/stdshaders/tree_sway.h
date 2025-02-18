//============ Copyright (c) Valve Corporation, All rights reserved. ============

#ifndef _TREE_SWAY_H
#define _TREE_SWAY_H

#if defined( _PS3 )
#define hlsl_float4x3_element( MATRIX,ROW4,COL3 ) ((MATRIX)[COL3][ROW4])
#define hlsl_float4x3 float3x4
#else
#define hlsl_float4x3_element( MATRIX,ROW4,COL3 ) ((MATRIX)[ROW4][COL3])
#define hlsl_float4x3 float4x3
#endif

// Tree sway vertex animation function. Requires a number of global variables to be defined. See vertexlit_and_unlit_generic_vs20.fxc or depthwrite_vs20.fxc for details.

// Tree sway mode 2:
// Hacks to use tree sway code on rectangular sheets of plastic/tarp attached at the four corners. 
// Inverts the sway scale radius to be 1 at (0,0,0) in model space and fall off radially towards the edges of the model.
// The model is expected to be build lying in the xy plane in model space, with its center at the origin.
// Treeswaystrength should be 0 in the vmt.

#if ( TREESWAY )
	float3 ComputeTreeSway( float3 vPositionOS, float flTime )
	{
		static const float g_flWindOffsetScale = 19;

		float flWindIntensity = length( g_vWindDir );

		// Model root position is the translation component of the model to world matrix
		float3 vModelRoot = float3( hlsl_float4x3_element( cModel[0],3,0 ), hlsl_float4x3_element( cModel[0],3,1 ), hlsl_float4x3_element( cModel[0],3,2 ) );

		// Transform the wind direction into model space
#ifdef _PS3
		float3 vWindDirAndIntensityOS = mul( float3( g_vWindDir, 0 ), ( float3x3 )cModel[0] );
#else
		float3 vWindDirAndIntensityOS = mul( ( float3x3 )cModel[0], float3( g_vWindDir, 0 ) );
#endif // !_PS3

		float flSwayScaleHeight = saturate( ( vPositionOS.z - g_flHeight * g_flStartHeight ) /
											( ( 1.0 - g_flStartHeight ) * g_flHeight ) );

		float flSwayScaleRadius = saturate( length( ( vPositionOS.xy ) - g_flRadius * g_flStartRadius ) /
											( ( 1.0 - g_flStartRadius ) * g_flRadius ) );

		// Used to turn off branch sway and scrumble below the minimum sway height
		float flHeightThreshold = step( 0, vPositionOS.z - g_flHeight * g_flStartHeight );

		#if ( TREESWAY == 2 )
		{
			// Works better for hanging vines
			flHeightThreshold = step( vPositionOS.z - g_flHeight * g_flStartHeight, 0 );
		}
		#endif

		#ifdef _X360
			// Scale branch motion based on how orthogonal they are
			float flOrthoBranchScale = 1.0 - abs( dot( normalize( vWindDirAndIntensityOS.xyz ), float3( normalize( vPositionOS.xy ), 0 ) ) );
		#else
			// Scale branch motion based on how orthogonal they are
			// This is what I want to compute:
			// float flOrthoBranchScale = 1.0 - abs( dot( normalize( vWindDirAndIntensityOS.xyz ), float3( normalize( vPositionOS.xy ), 0 ) ) );
			// Some NV hardware (7800) will do bad things when normalizing a 0 length vector. Instead, I'm doing the dot product unnormalized
			// and divide by the length of the vectors, making sure to avoid divide by 0.
			float flOrthoBranchScale = abs( dot( vWindDirAndIntensityOS.xyz, float3( vPositionOS.xy, 0 ) ) );
			flOrthoBranchScale = 1.0 - saturate( flOrthoBranchScale / ( max( length( vWindDirAndIntensityOS.xyz ), 0.0001 ) * max( length( vPositionOS.xy ), 0.0001 ) ) );
		#endif

		float flSwayScaleTrunk = g_flSwayIntensity * pow( flSwayScaleHeight, g_flSwayFalloffCurve );
		float flSwayScaleBranches = g_flSwayIntensity * flOrthoBranchScale * flSwayScaleRadius * flHeightThreshold;
		#if ( TREESWAY == 2 )
		{
			// Looks stupid on vines
			flSwayScaleBranches = 0.0;
		}
		#endif
		float flWindTimeOffset = dot( vModelRoot.xyz, float3( 1, 1, 1 ) ) * g_flWindOffsetScale;
		float flSlowSwayTime = ( flTime + flWindTimeOffset ) * g_flSwaySpeed;

		float3 vSwayPosOS = normalize( vPositionOS.xyz );
		float3 vScrumblePosOS = vSwayPosOS * g_flScrumbleWaveCount;
		float flScrumbleScale = pow( flSwayScaleRadius, g_flScrumbleFalloffCurve ) * g_flScrumbleIntensity * flHeightThreshold;

		float3 vPositionOffset = float3( 0, 0, 0 );

		// lerp between slow and fast sines based on wind speed
		float flSpeedLerp = smoothstep( g_flWindSpeedLerpStart, g_flWindSpeedLerpEnd, flWindIntensity );
		float4 vABunchOfSines = sin( float4( 1.0, 2.31, g_flFastSwaySpeedScale, 2.14 * g_flFastSwaySpeedScale ) * flSlowSwayTime.xxxx );
		vABunchOfSines.xy = lerp( vABunchOfSines.xy, vABunchOfSines.zw, flSpeedLerp );

		vPositionOffset.xyz = vWindDirAndIntensityOS *  flSwayScaleTrunk * ( vABunchOfSines.x + 0.1 );
		vPositionOffset.xyz += vWindDirAndIntensityOS *  flSwayScaleBranches * ( vABunchOfSines.y + 0.4 );

		float3 vScrumbleScale = flScrumbleScale.xxx;
		#if ( TREESWAY == 2 )
		{
			vScrumbleScale *= float3( 0.5, 0.5, 1.0 );
		}
		#endif

		vPositionOffset.xyz += flWindIntensity * ( vScrumbleScale.xyz * sin( g_flScrumbleSpeed * flTime.xxx + vScrumblePosOS.yzx + flWindTimeOffset.xxx ) );

		return vPositionOS.xyz + vPositionOffset.xyz;
	}
#endif

#endif // _TREE_SWAY_H
