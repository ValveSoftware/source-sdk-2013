//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// This file defines a number of constants and structured which are used to build up a command
// buffer to pass to ShaderAPI for state setting and other operations.  Since the prupose of these
// command buffers is to minimize and optimize calls into shaderapi, their structure is not
// abstract - they are built out by the calling process.
//
//===========================================================================//

#ifndef COMMANDBUFFER_H
#define COMMANDBUFFER_H


#ifdef _WIN32
#pragma once
#endif

// all commands defined with their struct
enum CommandBufferCommand_t
{
	// flow control commands.
	CBCMD_END = 0,											// end of stream
	CBCMD_JUMP = 1,											// int cmd, void *adr. jump to another
															// stream. Can be used to implement
															// non-sequentially allocated storage
	CBCMD_JSR = 2,											// int cmd, void *adr. subroutine call to another stream.

	// constant setting commands
	CBCMD_SET_PIXEL_SHADER_FLOAT_CONST = 256, // int cmd,int first_reg, int nregs, float values[nregs*4]


	CBCMD_SET_VERTEX_SHADER_FLOAT_CONST = 257, // int cmd,int first_reg, int nregs, float values[nregs*4]
	CBCMD_SET_VERTEX_SHADER_FLOAT_CONST_REF = 258, // int cmd,int first_reg, int nregs, &float values[nregs*4]
	CBCMD_SETPIXELSHADERFOGPARAMS = 259,					// int cmd, int regdest
	CBCMD_STORE_EYE_POS_IN_PSCONST = 260,					// int cmd, int regdest
	CBCMD_COMMITPIXELSHADERLIGHTING = 261,					// int cmd, int regdest
	CBCMD_SETPIXELSHADERSTATEAMBIENTLIGHTCUBE = 262,		// int cmd, int regdest
	CBCMD_SETAMBIENTCUBEDYNAMICSTATEVERTEXSHADER = 263,		// int cmd
	CBCMD_SET_DEPTH_FEATHERING_CONST = 264,					// int cmd, int constant register, float blend scale

	// texture binding
	CBCMD_BIND_STANDARD_TEXTURE = 512,						// cmd, sampler, texture id
	CBCMD_BIND_SHADERAPI_TEXTURE_HANDLE = 513,				// cmd, sampler, texture handle

	// shaders
	CBCMD_SET_PSHINDEX = 1024,								// cmd, idx
	CBCMD_SET_VSHINDEX = 1025,								// cmd, idx

	// commands from mainline.   In mainline commands no longer have
	// command id's specified like this. So I make up numbers...
	CBCMD_SET_VERTEX_SHADER_FLASHLIGHT_STATE = 2000,		// cmd, int first_reg (for worldToTexture matrix)
	CBCMD_SET_PIXEL_SHADER_FLASHLIGHT_STATE = 2001,		    // cmd, int color reg, int atten reg, int origin reg, sampler (for flashlight texture)

	CBCMD_SET_PIXEL_SHADER_UBERLIGHT_STATE = 2002,			// cmd

	CBCMD_SET_VERTEX_SHADER_NEARZFARZ_STATE = 2003,		    // cmd

};

//-----------------------------------------------------------------------------
// Commands used by the per-instance command buffer
// NOTE: If you add commands, you probably want to change the size of 
// CInstanceStorageBuffer and/or the choice of making it a fixed-size allocation
// see shaderlib/baseshader.*
//
// FIXME!! NOTE that this whole scheme here generates a dependency of the
// shaders on internal guts of shaderapidx8, since it's responsible for
// setting various pixel shader + vertex shader constants based on the
// commands below. We need to remove this dependency as it's way too restrictive
// and puts the smarts in the wrong place (see CBICMD_SETPIXELSHADERGLINTDAMPING
// as an example). Not going to solve this for l4d though, as I don't anticipate
// a large amount of new shader writing for that product.
//-----------------------------------------------------------------------------
enum CommandBufferInstanceCommand_t
{
	CBICMD_END = 0,										// end of stream
	CBICMD_JUMP,										// int cmd, void *adr. jump to another
	// stream. Can be used to implement
	// non-sequentially allocated storage
	CBICMD_JSR,											// int cmd, void *adr. subroutine call to another stream.

	CBICMD_SETSKINNINGMATRICES,							// int cmd

	CBICMD_SETVERTEXSHADERLOCALLIGHTING,				// int cmd
	CBICMD_SETPIXELSHADERLOCALLIGHTING,					// int cmd, int regdest
	CBICMD_SETVERTEXSHADERAMBIENTLIGHTCUBE,				// int cmd
	CBICMD_SETPIXELSHADERAMBIENTLIGHTCUBE,				// int cmd, int regdest
	CBICMD_SETPIXELSHADERAMBIENTLIGHTCUBELUMINANCE,		// int cmd, int regdest
	CBICMD_SETPIXELSHADERGLINTDAMPING,					// int cmd, int regdest

	CBICMD_BIND_ENV_CUBEMAP_TEXTURE,					// cmd, sampler

	CBICMD_SETMODULATIONPIXELSHADERDYNAMICSTATE,
	CBICMD_SETMODULATIONPIXELSHADERDYNAMICSTATE_LINEARCOLORSPACE_LINEARSCALE, // int cmd, int constant register, Vector color2
	CBICMD_SETMODULATIONPIXELSHADERDYNAMICSTATE_LINEARCOLORSPACE,			// int cmd, int constant register, Vector color2
	CBICMD_SETMODULATIONPIXELSHADERDYNAMICSTATE_LINEARSCALE,				// int cmd, int constant register, Vector color2, float scale
	CBICMD_SETMODULATIONVERTEXSHADERDYNAMICSTATE,							// int cmd, int constant register, Vector color2
	CBICMD_SETMODULATIONPIXELSHADERDYNAMICSTATE_IDENTITY,					// int cmd, int constant register
	CBICMD_SETMODULATIONVERTEXSHADERDYNAMICSTATE_LINEARSCALE,				// int cmd, int constant register, Vector color2, float scale
	// This must be last
	CBICMD_COUNT,
};

#endif // commandbuffer_h
