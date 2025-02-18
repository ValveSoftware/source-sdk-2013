//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#pragma warning( disable : 4244 ) // conversion from 'double' to 'float', possible loss of data

#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include <KeyValues.h>
#include <vgui_controls/AnimationController.h>
#include "filesystem.h"
#include "filesystem_helpers.h"

#include <stdio.h>
#include <math.h>
#include "mempool.h"
#include "utldict.h"
#include "mathlib/mathlib.h"
#include "characterset.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/dbg.h>
// for SRC
#include <vstdlib/random.h>
#include <tier0/memdbgon.h>

using namespace vgui;

static CUtlSymbolTable g_ScriptSymbols(0, 128, true);

// singleton accessor for animation controller for use by the vgui controls
namespace vgui
{
AnimationController *GetAnimationController()
{
	static AnimationController *s_pAnimationController = new AnimationController(NULL);
	return s_pAnimationController;
}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
AnimationController::AnimationController(Panel *parent) : BaseClass(parent, NULL)
{
	m_hSizePanel = 0;
	m_nScreenBounds[ 0 ] = m_nScreenBounds[ 1 ] = -1;
	m_nScreenBounds[ 2 ] = m_nScreenBounds[ 3 ] = -1;

	m_bAutoReloadScript = false;

	// always invisible
	SetVisible(false);

	SetProportional(true);

	// get the names of common types
	m_sPosition = g_ScriptSymbols.AddString("position");
	m_sSize = g_ScriptSymbols.AddString("size"); 
	m_sFgColor = g_ScriptSymbols.AddString("fgcolor"); 
	m_sBgColor = g_ScriptSymbols.AddString("bgcolor");

	m_sXPos = g_ScriptSymbols.AddString("xpos");
	m_sYPos = g_ScriptSymbols.AddString("ypos");
	m_sWide = g_ScriptSymbols.AddString("wide");
	m_sTall = g_ScriptSymbols.AddString("tall");

	m_sModelPos = g_ScriptSymbols.AddString( "model_pos" );

	m_flCurrentTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
AnimationController::~AnimationController()
{
}

//-----------------------------------------------------------------------------
// Purpose: Sets which script file to use
//-----------------------------------------------------------------------------
bool AnimationController::SetScriptFile( VPANEL sizingPanel, const char *fileName, bool wipeAll /*=false*/ )
{
	m_hSizePanel = sizingPanel;

	if ( wipeAll )
	{
		// clear the current script
		m_Sequences.RemoveAll();
		m_ScriptFileNames.RemoveAll();

		CancelAllAnimations();
	}

	// Store off this filename for reloading later on (if we don't have it already)
	UtlSymId_t sFilename = g_ScriptSymbols.AddString( fileName );
	if ( m_ScriptFileNames.Find( sFilename ) == m_ScriptFileNames.InvalidIndex() )
	{
		m_ScriptFileNames.AddToTail( sFilename );
	}

	UpdateScreenSize();

	// load the new script file
	return LoadScriptFile( fileName );
}

//-----------------------------------------------------------------------------
// Purpose: reloads the currently set script file
//-----------------------------------------------------------------------------
void AnimationController::ReloadScriptFile()
{
	// Clear all current sequences
	m_Sequences.RemoveAll();
	
	UpdateScreenSize();

	// Reload each file we've loaded
	for ( int i = 0; i < m_ScriptFileNames.Count(); i++ )
	{
		const char *lpszFilename = g_ScriptSymbols.String( m_ScriptFileNames[i] );
		if ( strlen( lpszFilename ) > 0)
		{
			if ( LoadScriptFile( lpszFilename ) == false )
			{
				Assert( 0 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: loads a script file from disk
//-----------------------------------------------------------------------------
bool AnimationController::LoadScriptFile(const char *fileName)
{
	FileHandle_t f = g_pFullFileSystem->Open(fileName, "rt");
	if (!f)
	{
		Warning("Couldn't find script file %s\n", fileName);
		return false;
	}

	// read the whole thing into memory
	int size = g_pFullFileSystem->Size(f);
	// read into temporary memory block
	int nBufSize = size+1;
	if ( IsXbox() )
	{
		nBufSize = AlignValue( nBufSize, 512 );
	}
	char *pMem = (char *)malloc(nBufSize);
	int bytesRead = g_pFullFileSystem->ReadEx(pMem, nBufSize, size, f);
	Assert(bytesRead <= size);
	pMem[bytesRead] = 0;
	g_pFullFileSystem->Close(f);
	// parse
	bool success = ParseScriptFile(pMem, bytesRead);
	free(pMem);
	return success;
}

AnimationController::RelativeAlignmentLookup AnimationController::g_AlignmentLookup[] =
{
	{ AnimationController::a_northwest	, "northwest" },
	{ AnimationController::a_north		, "north" },
	{ AnimationController::a_northeast	, "northeast" },
	{ AnimationController::a_west		, "west" },
	{ AnimationController::a_center		, "center" },
	{ AnimationController::a_east		, "east" },
	{ AnimationController::a_southwest	, "southwest" },
	{ AnimationController::a_south		, "south" },
	{ AnimationController::a_southeast	, "southeast" },

	{ AnimationController::a_northwest	, "nw" },
	{ AnimationController::a_north		, "n" },
	{ AnimationController::a_northeast	, "ne" },
	{ AnimationController::a_west		, "w" },
	{ AnimationController::a_center		, "c" },
	{ AnimationController::a_east		, "e" },
	{ AnimationController::a_southwest	, "sw" },
	{ AnimationController::a_south		, "s" },
	{ AnimationController::a_southeast	, "se" },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
AnimationController::RelativeAlignment AnimationController::LookupAlignment( char const *token )
{
	int c = ARRAYSIZE( g_AlignmentLookup );

	for ( int i = 0; i < c; i++ )
	{
		if ( !Q_stricmp( token, g_AlignmentLookup[ i ].name ) )
		{
			return g_AlignmentLookup[ i ].align;
		}
	}

	return AnimationController::a_northwest;
}

//-----------------------------------------------------------------------------
// Purpose: Parse position including right edge and center adjustment out of a 
//  token.  This is relative to the screen
//-----------------------------------------------------------------------------
void AnimationController::SetupPosition( AnimCmdAnimate_t& cmd, float *output, char const *psz, int screendimension )
{
	bool r = false, c = false;
	int pos;
	if ( psz[0] == '(' )
	{
		psz++;

		if ( Q_strstr( psz, ")" ) )
		{
			char sz[ 256 ];
			Q_strncpy( sz, psz, sizeof( sz ) );

			char *colon = Q_strstr( sz, ":" );
			if ( colon )
			{
				*colon = 0;

				RelativeAlignment ra = LookupAlignment( sz );

				colon++;

				char *panelName = colon;
				char *panelEnd = Q_strstr( panelName, ")" );
				if ( panelEnd )
				{
					*panelEnd = 0;

					if ( Q_strlen( panelName ) > 0 )
					{
						// 
						cmd.align.relativePosition	= true;
						cmd.align.alignPanel			= g_ScriptSymbols.AddString(panelName);
						cmd.align.alignment			= ra;
					}
				}
			}

			psz = Q_strstr( psz, ")" ) + 1;
		}
	}
	else if (psz[0] == 'r' || psz[0] == 'R')
	{
		r = true;
		psz++;
	}
	else if (psz[0] == 'c' || psz[0] == 'C')
	{
		c = true;
		psz++;
	}

	// get the number
	pos = atoi(psz);

	// scale the values
	if (IsProportional())
	{
		pos = vgui::scheme()->GetProportionalScaledValueEx( GetScheme(), pos );
	}

	// adjust the positions
	if (r)
	{
		pos = screendimension - pos;
	}
	if (c)
	{
		pos = (screendimension / 2) + pos;
	}

	// set the value
	*output = static_cast<float>( pos );
}


//-----------------------------------------------------------------------------
// Purpose: parses a script into sequences
//-----------------------------------------------------------------------------
bool AnimationController::ParseScriptFile(char *pMem, int length)
{
	// get the scheme (for looking up color names)
	IScheme *scheme = vgui::scheme()->GetIScheme(GetScheme());

	// get our screen size (for left/right/center alignment)
	int screenWide = m_nScreenBounds[ 2 ];
	int screenTall = m_nScreenBounds[ 3 ];

	// start by getting the first token
	char token[512];
	pMem = ParseFile(pMem, token, NULL);
	while (token[0])
	{
		bool bAccepted = true;

		// should be 'event'
		if (stricmp(token, "event"))
		{
			Warning("Couldn't parse script file: expected 'event', found '%s'\n", token);
			return false;
		}

		// get the event name
		pMem = ParseFile(pMem, token, NULL);
		if (strlen(token) < 1)
		{
			Warning("Couldn't parse script file: expected <event name>, found nothing\n");
			return false;
		}
		
		int seqIndex;
		UtlSymId_t nameIndex = g_ScriptSymbols.AddString(token);
				
		// Create a new sequence
		seqIndex = m_Sequences.AddToTail();
		AnimSequence_t &seq = m_Sequences[seqIndex];
		seq.name = nameIndex;
		seq.duration = 0.0f;

		// get the open brace or a conditional
		pMem = ParseFile(pMem, token, NULL);
		if ( Q_stristr( token, "[$" ) || Q_stristr( token, "[!$" ) )
		{
			bAccepted = EvaluateConditional( token );

			// now get the open brace
			pMem = ParseFile(pMem, token, NULL);
		}

		if (stricmp(token, "{"))
		{
			Warning("Couldn't parse script sequence '%s': expected '{', found '%s'\n", g_ScriptSymbols.String(seq.name), token);
			return false;
		}

		// walk the commands
		while (token[0])
		{
			// get the command type
			pMem = ParseFile(pMem, token, NULL);

			// skip out when we hit the end of the sequence
			if (token[0] == '}')
				break;

			// create a new command
			int cmdIndex = seq.cmdList.AddToTail();
			AnimCommand_t &animCmd = seq.cmdList[cmdIndex];
			memset(&animCmd, 0, sizeof(animCmd));
			if (!stricmp(token, "animate"))
			{
				animCmd.commandType = CMD_ANIMATE;
				// parse out the animation commands
				AnimCmdAnimate_t &cmdAnimate = animCmd.cmdData.animate;
				// panel to manipulate
				pMem = ParseFile(pMem, token, NULL);
				cmdAnimate.panel = g_ScriptSymbols.AddString(token);
				// variable to change
				pMem = ParseFile(pMem, token, NULL);
				cmdAnimate.variable = g_ScriptSymbols.AddString(token);
				// target value
				pMem = ParseFile(pMem, token, NULL);
				if (cmdAnimate.variable == m_sPosition)
				{
					// Get first token
					SetupPosition( cmdAnimate, &cmdAnimate.target.a, token, screenWide );

					// Get second token from "token"
					char token2[32];
					char *psz = ParseFile(token, token2, NULL);
					psz = ParseFile(psz, token2, NULL);
					psz = token2;

					// Position Y goes into ".b"
					SetupPosition( cmdAnimate, &cmdAnimate.target.b, psz, screenTall );
				}
				else if ( cmdAnimate.variable == m_sXPos )
				{
					// XPos and YPos both use target ".a"
					SetupPosition( cmdAnimate, &cmdAnimate.target.a, token, screenWide );
				}
				else if ( cmdAnimate.variable == m_sYPos )
				{
					// XPos and YPos both use target ".a"
					SetupPosition( cmdAnimate, &cmdAnimate.target.a, token, screenTall );
				}
				else 
				{
					// parse the floating point values right out
					if (0 == sscanf(token, "%f %f %f %f", &cmdAnimate.target.a, &cmdAnimate.target.b, &cmdAnimate.target.c, &cmdAnimate.target.d))
					{
						//=============================================================================
						// HPE_BEGIN:
						// [pfreese] Improved handling colors not defined in scheme 
						//=============================================================================
						
						// could be referencing a value in the scheme file, lookup
						Color default_invisible_black(0, 0, 0, 0);
						Color col = scheme->GetColor(token, default_invisible_black);

						// we don't have a way of seeing if the color is not declared in the scheme, so we use this
						// silly method of trying again with a different default to see if we get the fallback again
						if (col == default_invisible_black)
						{
							Color error_pink(255, 0, 255, 255);	// make it extremely obvious if a scheme lookup fails
							col = scheme->GetColor(token, error_pink);

							// commented out for Soldier/Demo release...(getting spammed in console)
							// we'll try to figure this out after the update is out
// 							if (col == error_pink)
// 							{
// 								Warning("Missing color in scheme: %s\n", token);
// 							}
						}
						
						//=============================================================================
						// HPE_END
						//=============================================================================

						cmdAnimate.target.a = col[0];
						cmdAnimate.target.b = col[1];
						cmdAnimate.target.c = col[2];
						cmdAnimate.target.d = col[3];
					}
				}

				// fix up scale
				if (cmdAnimate.variable == m_sSize)
				{
					if (IsProportional())
					{
						cmdAnimate.target.a = static_cast<float>( vgui::scheme()->GetProportionalScaledValueEx(GetScheme(), cmdAnimate.target.a) );
						cmdAnimate.target.b = static_cast<float>( vgui::scheme()->GetProportionalScaledValueEx(GetScheme(), cmdAnimate.target.b) );
					}
				}
				else if (cmdAnimate.variable == m_sWide ||
					     cmdAnimate.variable == m_sTall )
				{
					if (IsProportional())
					{
						// Wide and tall both use.a
						cmdAnimate.target.a = static_cast<float>( vgui::scheme()->GetProportionalScaledValueEx(GetScheme(), cmdAnimate.target.a) );
					}
				}
				
				// interpolation function
				pMem = ParseFile(pMem, token, NULL);
				if (!stricmp(token, "Accel"))
				{
					cmdAnimate.interpolationFunction = INTERPOLATOR_ACCEL;
				}
				else if (!stricmp(token, "Deaccel"))
				{
					cmdAnimate.interpolationFunction = INTERPOLATOR_DEACCEL;
				}
				else if ( !stricmp(token, "Spline"))
				{
					cmdAnimate.interpolationFunction = INTERPOLATOR_SIMPLESPLINE;
				}
				else if (!stricmp(token,"Pulse"))
				{
					cmdAnimate.interpolationFunction = INTERPOLATOR_PULSE;
					// frequencey
					pMem = ParseFile(pMem, token, NULL);
					cmdAnimate.interpolationParameter = (float)atof(token);
				}
				else if (!stricmp(token,"Bias"))
				{
					cmdAnimate.interpolationFunction = INTERPOLATOR_BIAS;
					// bias
					pMem = ParseFile(pMem, token, NULL);
					cmdAnimate.interpolationParameter = (float)atof(token);
				}
				else if (!stricmp(token,"Gain"))
				{
					cmdAnimate.interpolationFunction = INTERPOLATOR_GAIN;
					// bias
					pMem = ParseFile(pMem, token, NULL);
					cmdAnimate.interpolationParameter = (float)atof(token);
				}
				else if ( !stricmp( token, "Flicker"))
				{
					cmdAnimate.interpolationFunction = INTERPOLATOR_FLICKER;
					// noiseamount
					pMem = ParseFile(pMem, token, NULL);
					cmdAnimate.interpolationParameter = (float)atof(token);
				}
				else if (!stricmp(token, "Bounce"))
				{
					cmdAnimate.interpolationFunction = INTERPOLATOR_BOUNCE;
				}
				else
				{
					cmdAnimate.interpolationFunction = INTERPOLATOR_LINEAR;
				}
				// start time
				pMem = ParseFile(pMem, token, NULL);
				cmdAnimate.startTime = (float)atof(token);
				// duration
				pMem = ParseFile(pMem, token, NULL);
				cmdAnimate.duration = (float)atof(token);
				// check max duration
				if (cmdAnimate.startTime + cmdAnimate.duration > seq.duration)
				{
					seq.duration = cmdAnimate.startTime + cmdAnimate.duration;
				}
			}
			else if (!stricmp(token, "runevent"))
			{
				animCmd.commandType = CMD_RUNEVENT;
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.event = g_ScriptSymbols.AddString(token);
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.timeDelay = (float)atof(token);
			}
			else if (!stricmp(token, "runeventchild"))
			{
				animCmd.commandType = CMD_RUNEVENTCHILD;
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.variable = g_ScriptSymbols.AddString(token);
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.event = g_ScriptSymbols.AddString(token);
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.timeDelay = (float)atof(token);
			}
			else if (!stricmp(token, "firecommand"))
			{
				animCmd.commandType = CMD_FIRECOMMAND;
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.timeDelay = (float)atof(token);
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.variable = g_ScriptSymbols.AddString(token);
			}
			else if ( !stricmp(token, "playsound") )
			{
				animCmd.commandType = CMD_PLAYSOUND;
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.timeDelay = (float)atof(token);
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.variable = g_ScriptSymbols.AddString(token);
			}
			else if (!stricmp(token, "setvisible"))
			{
				animCmd.commandType = CMD_SETVISIBLE;
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.variable = g_ScriptSymbols.AddString(token);
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.variable2 = atoi(token);
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.timeDelay = (float)atof(token);
			}
			else if (!stricmp(token, "setinputenabled"))
			{
				animCmd.commandType = CMD_SETINPUTENABLED;
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.variable = g_ScriptSymbols.AddString(token);
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.variable2 = atoi(token);
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.timeDelay = (float)atof(token);
			}
			else if (!stricmp(token, "stopevent"))
			{
				animCmd.commandType = CMD_STOPEVENT;
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.event = g_ScriptSymbols.AddString(token);
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.timeDelay = (float)atof(token);
			}
			else if (!stricmp(token, "StopPanelAnimations"))
			{
				animCmd.commandType = CMD_STOPPANELANIMATIONS;
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.event = g_ScriptSymbols.AddString(token);
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.timeDelay = (float)atof(token);
			}
			else if (!stricmp(token, "stopanimation"))
			{
				animCmd.commandType = CMD_STOPANIMATION;
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.event = g_ScriptSymbols.AddString(token);
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.variable = g_ScriptSymbols.AddString(token);
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.timeDelay = (float)atof(token);
			}
			else if ( !stricmp( token, "SetFont" ))
			{
				animCmd.commandType = CMD_SETFONT;
				// Panel name
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.event = g_ScriptSymbols.AddString(token);
				// Font parameter
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.variable = g_ScriptSymbols.AddString(token);
				// Font name from scheme
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.variable2 = g_ScriptSymbols.AddString(token);

				// Set time
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.timeDelay = (float)atof(token);
			}
			else if ( !stricmp( token, "SetTexture" ))
			{
				animCmd.commandType = CMD_SETTEXTURE;
				// Panel name
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.event = g_ScriptSymbols.AddString(token);
				// Texture Id
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.variable = g_ScriptSymbols.AddString(token);
				// material name
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.variable2 = g_ScriptSymbols.AddString(token);

				// Set time
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.timeDelay = (float)atof(token);
			}
			else if ( !stricmp( token, "SetString" ))
			{
				animCmd.commandType = CMD_SETSTRING;
				// Panel name
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.event = g_ScriptSymbols.AddString(token);
				// String variable name
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.variable = g_ScriptSymbols.AddString(token);
				// String value to set
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.variable2 = g_ScriptSymbols.AddString(token);

				// Set time
				pMem = ParseFile(pMem, token, NULL);
				animCmd.cmdData.runEvent.timeDelay = (float)atof(token);
			}
			else
			{
				Warning("Couldn't parse script sequence '%s': expected <anim command>, found '%s'\n", g_ScriptSymbols.String(seq.name), token);
				return false;
			}
			
			// Look ahead one token for a conditional
			char *peek = ParseFile(pMem, token, NULL);
			if ( Q_stristr( token, "[$" ) || Q_stristr( token, "[!$" ) )
			{
				if ( !EvaluateConditional( token ) )
				{
					seq.cmdList.Remove( cmdIndex );
				}
				pMem = peek;
			}
		}

		if ( bAccepted )
		{
			// Attempt to find a collision in the sequences, replacing the old one if found
			int seqIterator;
			for ( seqIterator = 0; seqIterator < m_Sequences.Count()-1; seqIterator++ )	
			{
				if ( m_Sequences[seqIterator].name == nameIndex )
				{
					// Get rid of it, we're overriding it
					m_Sequences.Remove( seqIndex );
					break;
				}
			}
		}
		else
		{
			// Dump the entire sequence
			m_Sequences.Remove( seqIndex );
		}

		// get the next token, if any
		pMem = ParseFile(pMem, token, NULL);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: checks all posted animation events, firing if time
//-----------------------------------------------------------------------------
void AnimationController::UpdatePostedMessages(bool bRunToCompletion)
{
	CUtlVector<RanEvent_t> eventsRanThisFrame;

	// check all posted messages
	for (int i = 0; i < m_PostedMessages.Count(); i++)
	{
		PostedMessage_t &msgRef = m_PostedMessages[i];

		if ( !msgRef.canBeCancelled && bRunToCompletion )
			continue;

		if (m_flCurrentTime < msgRef.startTime && !bRunToCompletion)
			continue;

		// take a copy of th message
		PostedMessage_t msg = msgRef;

		// remove the event
		// do this before handling the message because the message queue may be messed with
		m_PostedMessages.Remove(i);
		// reset the count, start the whole queue again
		i = -1;

		if ( msg.parent.Get() == NULL )
			continue;

		// handle the event
		switch (msg.commandType)
		{
		case CMD_RUNEVENT:
			{
				RanEvent_t curEvent;
				curEvent.pParent = NULL;
				curEvent.event = msg.event;

				curEvent.pParent = msg.parent.Get();
				
				// run the event, but only if we haven't already run it this frame, for this parent
				if (!eventsRanThisFrame.HasElement(curEvent))
				{
					eventsRanThisFrame.AddToTail(curEvent);
					RunCmd_RunEvent(msg);
				}
			}	
			break;
		case CMD_RUNEVENTCHILD:
			{
				RanEvent_t curEvent;
				curEvent.pParent = NULL;
				curEvent.event =  msg.event;

				curEvent.pParent = msg.parent.Get()->FindChildByName( g_ScriptSymbols.String(msg.variable), true );
				msg.parent = curEvent.pParent;
		
				// run the event, but only if we haven't already run it this frame, for this parent
				if (!eventsRanThisFrame.HasElement(curEvent))
				{
					eventsRanThisFrame.AddToTail(curEvent);
					RunCmd_RunEvent(msg);
				}
			}
			break;
		case CMD_FIRECOMMAND:
			{
				msg.parent->OnCommand( g_ScriptSymbols.String(msg.variable) );
			}
			break;
		case CMD_PLAYSOUND:
			{
				vgui::surface()->PlaySound( g_ScriptSymbols.String(msg.variable) );
			}
			break;
		case CMD_SETVISIBLE:
			{
				Panel* pPanel = msg.parent.Get()->FindChildByName( g_ScriptSymbols.String(msg.variable), true );
				if ( pPanel )
				{
					pPanel->SetVisible( msg.variable2 == 1 );
				}
			}
			break;
		case CMD_SETINPUTENABLED:
			{
				Panel* pPanel = msg.parent.Get()->FindChildByName( g_ScriptSymbols.String(msg.variable), true );
				if ( pPanel )
				{
					pPanel->SetMouseInputEnabled( msg.variable2 == 1 );
					pPanel->SetKeyBoardInputEnabled( msg.variable2 == 1 );
				}
			}
			break;
		case CMD_STOPEVENT:
			RunCmd_StopEvent(msg);
			break;
		case CMD_STOPPANELANIMATIONS:
			RunCmd_StopPanelAnimations(msg);
			break;
		case CMD_STOPANIMATION:
			RunCmd_StopAnimation(msg);
			break;
		case CMD_SETFONT:
			RunCmd_SetFont(msg);
			break;
		case CMD_SETTEXTURE:
			RunCmd_SetTexture(msg);
			break;
		case CMD_SETSTRING:
			RunCmd_SetString( msg );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: runs the current animations
//-----------------------------------------------------------------------------
void AnimationController::UpdateActiveAnimations(bool bRunToCompletion)
{
	// iterate all the currently active animations
	for (int i = 0; i < m_ActiveAnimations.Count(); i++)
	{
		ActiveAnimation_t &anim = m_ActiveAnimations[i];

		if ( !anim.canBeCancelled && bRunToCompletion )
			continue;

		// see if the anim is ready to start
		if (m_flCurrentTime < anim.startTime && !bRunToCompletion)
			continue;

		if (!anim.panel.Get())
		{
			// panel is gone, remove the animation
			m_ActiveAnimations.Remove(i);
			--i;
			continue;
		}

		if (!anim.started && !bRunToCompletion)
		{
			// start the animation from the current value
			anim.startValue = GetValue(anim, anim.panel, anim.variable);
			anim.started = true;

			// Msg( "Starting animation of %s => %.2f (seq: %s) (%s)\n", g_ScriptSymbols.String(anim.variable), anim.endValue.a, g_ScriptSymbols.String(anim.seqName), anim.panel->GetName());
		}

		// get the interpolated value
		Value_t val;
		if (m_flCurrentTime >= anim.endTime || bRunToCompletion)
		{
			// animation is done, use the last value
			val = anim.endValue;
		}
		else
		{
			// get the interpolated value
			val = GetInterpolatedValue(anim.interpolator, anim.interpolatorParam, m_flCurrentTime, anim.startTime, anim.endTime, anim.startValue, anim.endValue);
		}

		// apply the new value to the panel
		SetValue(anim, anim.panel, anim.variable, val);

		// Msg( "Animate value: %s => %.2f for panel '%s'\n", g_ScriptSymbols.String(anim.variable), val.a, anim.panel->GetName());

		// see if we can remove the animation
		if (m_flCurrentTime >= anim.endTime || bRunToCompletion)
		{
			m_ActiveAnimations.Remove(i);
			--i;
		}
	}
}

bool AnimationController::UpdateScreenSize()
{
	// get our screen size (for left/right/center alignment)
	int screenWide, screenTall;
	int sx = 0, sy = 0;
	if ( m_hSizePanel != 0 )
	{
		ipanel()->GetSize( m_hSizePanel, screenWide, screenTall );
		ipanel()->GetPos( m_hSizePanel, sx, sy );
	}
	else
	{
		surface()->GetScreenSize(screenWide, screenTall);
	}

	bool changed =	m_nScreenBounds[ 0 ] != sx || 
					m_nScreenBounds[ 1 ] != sy ||
					m_nScreenBounds[ 2 ] != screenWide || 
					m_nScreenBounds[ 3 ] != screenTall;

	m_nScreenBounds[ 0 ] = sx;
	m_nScreenBounds[ 1 ] = sy;
	m_nScreenBounds[ 2 ] = screenWide;
	m_nScreenBounds[ 3 ] = screenTall;

	return changed;
}
//-----------------------------------------------------------------------------
// Purpose: runs a frame of animation
//-----------------------------------------------------------------------------
void AnimationController::UpdateAnimations( float currentTime )
{
	m_flCurrentTime = currentTime;

	if ( UpdateScreenSize()	&& m_ScriptFileNames.Count() )
	{
		RunAllAnimationsToCompletion();
		ReloadScriptFile();
	}

	UpdatePostedMessages(false);
	UpdateActiveAnimations(false);
}

//-----------------------------------------------------------------------------
// Purpose: plays all animations to completion instantly
//-----------------------------------------------------------------------------
void AnimationController::RunAllAnimationsToCompletion()
{
	// Msg( "AnimationController::RunAllAnimationsToCompletion()\n" );
	UpdatePostedMessages(true);
	UpdateActiveAnimations(true);
}

//-----------------------------------------------------------------------------
// Purpose: Stops all current animations
//-----------------------------------------------------------------------------
void AnimationController::CancelAllAnimations()
{
	// Msg( "AnimationController::CancelAllAnimations()\n" );

	FOR_EACH_VEC_BACK( m_ActiveAnimations, i )
	{
		if ( m_ActiveAnimations[i].canBeCancelled )
			m_ActiveAnimations.Remove( i );
	}

	FOR_EACH_VEC_BACK(m_PostedMessages, i)
	{
		if (m_PostedMessages[i].canBeCancelled)
			m_PostedMessages.Remove(i);
	}
}

//-----------------------------------------------------------------------------
// Purpose: produces an interpolated value
//-----------------------------------------------------------------------------
AnimationController::Value_t AnimationController::GetInterpolatedValue(int interpolator, float interpolatorParam, float currentTime, float startTime, float endTime, Value_t &startValue, Value_t &endValue)
{
	// calculate how far we are into the animation
	float pos = (currentTime - startTime) / (endTime - startTime);

	// adjust the percentage through by the interpolation function
	switch (interpolator)
	{
	case INTERPOLATOR_ACCEL:
		pos *= pos;
		break;
	case INTERPOLATOR_DEACCEL:
		pos = sqrtf(pos);
		break;
	case INTERPOLATOR_SIMPLESPLINE:
		pos = SimpleSpline( pos );
		break;
	case INTERPOLATOR_PULSE:
		// Make sure we end at 1.0, so use cosine
		pos = 0.5f + 0.5f * ( cos( pos * 2.0f * M_PI * interpolatorParam ) );
		break;
	case INTERPOLATOR_BIAS:
		pos = Bias( pos, interpolatorParam );
		break;
	case INTERPOLATOR_GAIN:
		pos = Gain( pos, interpolatorParam );
		break;
	case INTERPOLATOR_FLICKER:
		if ( RandomFloat( 0.0f, 1.0f ) < interpolatorParam )
		{
			pos = 1.0f;
		}
		else
		{
			pos = 0.0f;
		}
		break;
	case INTERPOLATOR_BOUNCE:
	{
		// fall from startValue to endValue, bouncing a few times and settling out at endValue
		const float hit1 = 0.33f;
		const float hit2 = 0.67f;
		const float hit3 = 1.0f;

		if ( pos < hit1 )
		{
			pos = 1.0f - sin( M_PI * pos / hit1 );
		}
		else if ( pos < hit2 )
		{
			pos = 0.5f + 0.5f * ( 1.0f - sin( M_PI * ( pos - hit1 ) / ( hit2 - hit1 ) ) );
		}
		else
		{
			pos = 0.8f + 0.2f * ( 1.0f - sin( M_PI * ( pos - hit2 ) / ( hit3 - hit2 ) ) );
		}
		break;
	}
	case INTERPOLATOR_LINEAR:
	default:
		break;
	}

	// calculate the value
	Value_t val;
	val.a = ((endValue.a - startValue.a) * pos) + startValue.a;
	val.b = ((endValue.b - startValue.b) * pos) + startValue.b;
	val.c = ((endValue.c - startValue.c) * pos) + startValue.c;
	val.d = ((endValue.d - startValue.d) * pos) + startValue.d;
	return val;
}

//-----------------------------------------------------------------------------
// Purpose: sets that the script file should be reloaded each time a script is ran
//			used for development
//-----------------------------------------------------------------------------
void AnimationController::SetAutoReloadScript(bool state)
{
	m_bAutoReloadScript = state;
}

//-----------------------------------------------------------------------------
// Purpose: starts an animation sequence script
//-----------------------------------------------------------------------------
bool AnimationController::StartAnimationSequence(const char *sequenceName, bool bCanBeCancelled )
{
	// We support calling an animation on elements that are not the calling 
	// panel's children. Use the base parent to start the search.

	return StartAnimationSequence( GetParent(), sequenceName, bCanBeCancelled );
}

//-----------------------------------------------------------------------------
// Purpose: starts an animation sequence script
//-----------------------------------------------------------------------------
bool AnimationController::StartAnimationSequence(Panel *pWithinParent, const char *sequenceName, bool bCanBeCancelled )
{
	Assert( pWithinParent );

	if (m_bAutoReloadScript)
	{
		// Reload the script files
		ReloadScriptFile();
	}

	// lookup the symbol for the name
	UtlSymId_t seqName = g_ScriptSymbols.Find(sequenceName);
	if (seqName == UTL_INVAL_SYMBOL)
		return false;

	// Msg("Starting animation sequence %s\n", sequenceName);

	// remove the existing command from the queue
	RemoveQueuedAnimationCommands(seqName, pWithinParent);

	// look through for the sequence
	int i;
	for (i = 0; i < m_Sequences.Count(); i++)
	{
		if (m_Sequences[i].name == seqName)
			break;
	}
	if (i >= m_Sequences.Count())
		return false;

	// execute the sequence
	for (int cmdIndex = 0; cmdIndex < m_Sequences[i].cmdList.Count(); cmdIndex++)
	{
		ExecAnimationCommand(seqName, m_Sequences[i].cmdList[cmdIndex], pWithinParent, bCanBeCancelled);
	}

	return true;	
}

//-----------------------------------------------------------------------------
// Purpose: stops an animation sequence script
//-----------------------------------------------------------------------------
bool AnimationController::StopAnimationSequence( Panel *pWithinParent, const char *sequenceName )
{
	Assert( pWithinParent );

	// lookup the symbol for the name
	UtlSymId_t seqName = g_ScriptSymbols.Find( sequenceName );
	if (seqName == UTL_INVAL_SYMBOL)
		return false;

	// remove the existing command from the queue
	RemoveQueuedAnimationCommands( seqName, pWithinParent );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Runs a custom command from code, not from a script file
//-----------------------------------------------------------------------------
void AnimationController::CancelAnimationsForPanel( Panel *pWithinParent )
{
	// Msg("Removing queued anims for sequence %s\n", g_ScriptSymbols.String(seqName));

	// remove messages posted by this sequence
	// if pWithinParent is specified, remove only messages under that parent
	{
		for (int i = 0; i < m_PostedMessages.Count(); i++)
		{
			if ( m_PostedMessages[i].parent == pWithinParent )
			{
				m_PostedMessages.Remove(i);
				--i;
			}
		}
	}

	// remove all animations
	// if pWithinParent is specified, remove only animations under that parent
	for (int i = 0; i < m_ActiveAnimations.Count(); i++)
	{
		Panel *animPanel = m_ActiveAnimations[i].panel;

		if ( !animPanel )
			continue;

		Panel *foundPanel = pWithinParent->FindChildByName(animPanel->GetName(),true);

		if ( foundPanel != animPanel )
			continue;

		m_ActiveAnimations.Remove(i);
		--i;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Runs a custom command from code, not from a script file
//-----------------------------------------------------------------------------
void AnimationController::RunAnimationCommand(vgui::Panel *panel,
											   const char *variable,
											   float targetValue,
											   float startDelaySeconds,
											   float duration,
											   Interpolators_e interpolator,
											   float animParameter /* = 0 */,
											   bool bClearValueQueue /* = true */,
											   bool bCanBeCancelled /* = true */ )
{
	UtlSymId_t var = g_ScriptSymbols.AddString(variable);
	if ( bClearValueQueue )
	{
		// clear any previous animations of this variable
		RemoveQueuedAnimationByType(panel, var, UTL_INVAL_SYMBOL);
	}

	// build a new animation
	AnimCmdAnimate_t animateCmd;
	memset(&animateCmd, 0, sizeof(animateCmd));
	animateCmd.panel = 0;
	animateCmd.variable = var;
	animateCmd.target.a = targetValue;
	animateCmd.interpolationFunction = interpolator;
	animateCmd.interpolationParameter = animParameter;
	animateCmd.startTime = startDelaySeconds;
	animateCmd.duration = duration;	

	// start immediately
	StartCmd_Animate(panel, 0, animateCmd, bCanBeCancelled);
}

//-----------------------------------------------------------------------------
// Purpose: Runs a custom command from code, not from a script file
//-----------------------------------------------------------------------------
void AnimationController::RunAnimationCommand(vgui::Panel *panel,
											   const char *variable,
											   Color targetValue,
											   float startDelaySeconds,
											   float duration,
											   Interpolators_e interpolator,
											   float animParameter /* = 0 */,
											   bool bClearValueQueue /* = true */,
											   bool bCanBeCancelled /* = true */ )
{
	UtlSymId_t var = g_ScriptSymbols.AddString(variable);

	if ( bClearValueQueue )
	{
		// clear any previous animations of this variable
		RemoveQueuedAnimationByType(panel, var, UTL_INVAL_SYMBOL);
	}

	// build a new animation
	AnimCmdAnimate_t animateCmd;
	memset(&animateCmd, 0, sizeof(animateCmd));
	animateCmd.panel = 0;
	animateCmd.variable = var;
	animateCmd.target.a = targetValue[0];
	animateCmd.target.b = targetValue[1];
	animateCmd.target.c = targetValue[2];
	animateCmd.target.d = targetValue[3];
	animateCmd.interpolationFunction = interpolator;
	animateCmd.interpolationParameter = animParameter;
	animateCmd.startTime = startDelaySeconds;
	animateCmd.duration = duration;

	// start immediately
	StartCmd_Animate(panel, 0, animateCmd, bCanBeCancelled);
}

//-----------------------------------------------------------------------------
// Purpose: gets the length of an animation sequence, in seconds
//-----------------------------------------------------------------------------
float AnimationController::GetAnimationSequenceLength(const char *sequenceName)
{
	// lookup the symbol for the name
	UtlSymId_t seqName = g_ScriptSymbols.Find(sequenceName);
	if (seqName == UTL_INVAL_SYMBOL)
		return 0.0f;

	// look through for the sequence
	int i;
	for (i = 0; i < m_Sequences.Count(); i++)
	{
		if (m_Sequences[i].name == seqName)
			break;
	}
	if (i >= m_Sequences.Count())
		return 0.0f;

	// sequence found
	return m_Sequences[i].duration;
}

//-----------------------------------------------------------------------------
// Purpose: removes an existing set of commands from the queue
//-----------------------------------------------------------------------------
void AnimationController::RemoveQueuedAnimationCommands(UtlSymId_t seqName, Panel *pWithinParent)
{
	// Msg("Removing queued anims for sequence %s\n", g_ScriptSymbols.String(seqName));

	// remove messages posted by this sequence
	// if pWithinParent is specified, remove only messages under that parent
	{for (int i = 0; i < m_PostedMessages.Count(); i++)
	{
		if ( ( m_PostedMessages[i].seqName == seqName ) &&
			 ( !pWithinParent || ( m_PostedMessages[i].parent == pWithinParent ) ) )
		{
			m_PostedMessages.Remove(i);
			--i;
		}
	}}

	// remove all animations
	// if pWithinParent is specified, remove only animations under that parent
	for (int i = 0; i < m_ActiveAnimations.Count(); i++)
	{
		if ( m_ActiveAnimations[i].seqName != seqName )
			continue;

		// panel this anim is on, m_ActiveAnimations[i].panel
		if ( pWithinParent )
		{
			Panel *animPanel = m_ActiveAnimations[i].panel;

			if ( !animPanel )
				continue;

			Panel *foundPanel = pWithinParent->FindChildByName(animPanel->GetName(),true);

			if ( foundPanel != animPanel )
				continue;
		}

		m_ActiveAnimations.Remove(i);
		--i;
	}
}

//-----------------------------------------------------------------------------
// Purpose: removes the specified queued animation
//-----------------------------------------------------------------------------
void AnimationController::RemoveQueuedAnimationByType(vgui::Panel *panel, UtlSymId_t variable, UtlSymId_t sequenceToIgnore)
{
	for (int i = 0; i < m_ActiveAnimations.Count(); i++)
	{
		if (m_ActiveAnimations[i].panel == panel && m_ActiveAnimations[i].variable == variable && m_ActiveAnimations[i].seqName != sequenceToIgnore)
		{
			// Msg("Removing queued anim %s::%s::%s\n", g_ScriptSymbols.String(m_ActiveAnimations[i].seqName), panel->GetName(), g_ScriptSymbols.String(variable));
			m_ActiveAnimations.Remove(i);
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: runs a single line of the script
//-----------------------------------------------------------------------------
void AnimationController::ExecAnimationCommand(UtlSymId_t seqName, AnimCommand_t &animCommand, Panel *pWithinParent, bool bCanBeCancelled)
{
	if (animCommand.commandType == CMD_ANIMATE)
	{
		StartCmd_Animate(seqName, animCommand.cmdData.animate, pWithinParent, bCanBeCancelled);
	}
	else
	{
		// post the command to happen at the specified time
		PostedMessage_t &msg = m_PostedMessages[m_PostedMessages.AddToTail()];
		msg.seqName = seqName;
		msg.commandType = animCommand.commandType;
		msg.event = animCommand.cmdData.runEvent.event;
		msg.variable = animCommand.cmdData.runEvent.variable;
		msg.variable2 = animCommand.cmdData.runEvent.variable2;
		msg.startTime = m_flCurrentTime + animCommand.cmdData.runEvent.timeDelay;
		msg.parent = pWithinParent;
		msg.canBeCancelled = bCanBeCancelled;
	}
}

//-----------------------------------------------------------------------------
// Purpose: starts a variable animation
//-----------------------------------------------------------------------------
void AnimationController::StartCmd_Animate(UtlSymId_t seqName, AnimCmdAnimate_t &cmd, Panel *pWithinParent, bool bCanBeCancelled)
{
	Assert( pWithinParent );
	if ( !pWithinParent )
		return;

	// make sure the child exists
	Panel *panel = pWithinParent->FindChildByName(g_ScriptSymbols.String(cmd.panel),true);
	if ( !panel )
	{
		// Check the parent
		Panel *parent = GetParent();
		if ( !Q_stricmp( parent->GetName(), g_ScriptSymbols.String(cmd.panel) ) )
		{
			panel = parent;
		}
	}
	if (!panel)
		return;

	StartCmd_Animate(panel, seqName, cmd, bCanBeCancelled);
}

//-----------------------------------------------------------------------------
// Purpose: Starts an animation command for the specified panel
//-----------------------------------------------------------------------------
void AnimationController::StartCmd_Animate(Panel *panel, UtlSymId_t seqName, AnimCmdAnimate_t &cmd, bool bCanBeCancelled)
{
	// build a command to add to the animation queue
	ActiveAnimation_t &anim = m_ActiveAnimations[m_ActiveAnimations.AddToTail()];
	anim.panel = panel;
	anim.seqName = seqName;
	anim.variable = cmd.variable;
	anim.interpolator = cmd.interpolationFunction;
	anim.interpolatorParam = cmd.interpolationParameter;
	// timings
	anim.startTime = m_flCurrentTime + cmd.startTime;
	anim.endTime = anim.startTime + cmd.duration;
	// values
	anim.started = false;
	anim.endValue = cmd.target;

	anim.canBeCancelled = bCanBeCancelled;

	anim.align = cmd.align;
}

//-----------------------------------------------------------------------------
// Purpose: a posted message to run another event
//-----------------------------------------------------------------------------
void AnimationController::RunCmd_RunEvent(PostedMessage_t &msg)
{
	StartAnimationSequence(msg.parent.Get(), g_ScriptSymbols.String(msg.event), msg.canBeCancelled);
}

//-----------------------------------------------------------------------------
// Purpose: a posted message to stop another event
//-----------------------------------------------------------------------------
void AnimationController::RunCmd_StopEvent(PostedMessage_t &msg)
{
	RemoveQueuedAnimationCommands(msg.event, msg.parent);
}

//-----------------------------------------------------------------------------
// Purpose: a posted message to stop all animations relevant to a specified panel
//-----------------------------------------------------------------------------
void AnimationController::RunCmd_StopPanelAnimations(PostedMessage_t &msg)
{
	Panel *panel = FindSiblingByName(g_ScriptSymbols.String(msg.event));
	Assert(panel != NULL);
	if (!panel)
		return;

	// loop through all the active animations cancelling any that 
	// are operating on said panel,	except for the event specified
	for (int i = 0; i < m_ActiveAnimations.Count(); i++)
	{
		if (m_ActiveAnimations[i].panel == panel && m_ActiveAnimations[i].seqName != msg.seqName)
		{
			m_ActiveAnimations.Remove(i);
			--i;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: a posted message to stop animations of a specific type
//-----------------------------------------------------------------------------
void AnimationController::RunCmd_StopAnimation(PostedMessage_t &msg)
{
	Panel *panel = FindSiblingByName(g_ScriptSymbols.String(msg.event));
	Assert(panel != NULL);
	if (!panel)
		return;

	RemoveQueuedAnimationByType(panel, msg.variable, msg.seqName);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void AnimationController::RunCmd_SetFont( PostedMessage_t &msg )
{
	Panel *parent = msg.parent.Get();

	if ( !parent )
	{
		parent = GetParent();
	}

	Panel *panel = parent->FindChildByName(g_ScriptSymbols.String(msg.event), true);
	Assert(panel != NULL);
	if (!panel)
		return;

	KeyValues *inputData = new KeyValues(g_ScriptSymbols.String(msg.variable));
	inputData->SetString(g_ScriptSymbols.String(msg.variable), g_ScriptSymbols.String(msg.variable2));
	if (!panel->SetInfo(inputData))
	{
	//	Assert(!("Unhandlable var in AnimationController::SetValue())"));
	}
	inputData->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void AnimationController::RunCmd_SetTexture( PostedMessage_t &msg )
{
	Panel *panel = FindSiblingByName(g_ScriptSymbols.String(msg.event));
	Assert(panel != NULL);
	if (!panel)
		return;

	KeyValues *inputData = new KeyValues(g_ScriptSymbols.String(msg.variable));
	inputData->SetString(g_ScriptSymbols.String(msg.variable), g_ScriptSymbols.String(msg.variable2));
	if (!panel->SetInfo(inputData))
	{
	//	Assert(!("Unhandlable var in AnimationController::SetValue())"));
	}
	inputData->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void AnimationController::RunCmd_SetString( PostedMessage_t &msg )
{
	Panel *panel = FindSiblingByName(g_ScriptSymbols.String(msg.event));
	Assert(panel != NULL);
	if (!panel)
		return;

	KeyValues *inputData = new KeyValues(g_ScriptSymbols.String(msg.variable));
	inputData->SetString(g_ScriptSymbols.String(msg.variable), g_ScriptSymbols.String(msg.variable2));
	if (!panel->SetInfo(inputData))
	{
	//	Assert(!("Unhandlable var in AnimationController::SetValue())"));
	}
	inputData->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int AnimationController::GetRelativeOffset( AnimAlign_t& align, bool xcoord )
{
	if ( !align.relativePosition )
		return 0;

	Panel *panel = GetParent()->FindChildByName(g_ScriptSymbols.String(align.alignPanel), true);
	if ( !panel )
		return 0;

	int x, y, w, h;
	panel->GetBounds( x, y, w, h );

	int offset =0;
	switch ( align.alignment )
	{
	default:
	case a_northwest:
		offset = xcoord ? x : y;
		break;
	case a_north:
		offset = xcoord ? ( x + w ) / 2 : y;
		break;
	case a_northeast:
		offset = xcoord ? ( x + w ) : y;
		break;
	case a_west:
		offset = xcoord ? x : ( y + h ) / 2;
		break;
	case a_center:
		offset = xcoord ? ( x + w ) / 2 : ( y + h ) / 2;
		break;
	case a_east:
		offset = xcoord ? ( x + w ) : ( y + h ) / 2;
		break;
	case a_southwest:
		offset = xcoord ? x : ( y + h );
		break;
	case a_south:
		offset = xcoord ? ( x + w ) / 2 : ( y + h );
		break;
	case a_southeast:
		offset = xcoord ? ( x + w ) : ( y +  h );
		break;
	}

	return offset;
}

//-----------------------------------------------------------------------------
// Purpose: Gets the specified value from a panel
//-----------------------------------------------------------------------------
AnimationController::Value_t AnimationController::GetValue(ActiveAnimation_t& anim, Panel *panel, UtlSymId_t var)
{
	Value_t val = { 0, 0, 0, 0 };
	if (var == m_sPosition)
	{
		int x, y;
		panel->GetPos(x, y);
		val.a = (float)(x - GetRelativeOffset( anim.align, true ) );
		val.b = (float)(y - GetRelativeOffset( anim.align, false ) );
	}
	else if (var == m_sSize)
	{
		int w, t;
		panel->GetSize(w, t);
		val.a = (float)w;
		val.b = (float)t;
	}
	else if (var == m_sFgColor)
	{
		Color col = panel->GetFgColor();
		val.a = col[0];
		val.b = col[1];
		val.c = col[2];
		val.d = col[3];
	}
	else if (var == m_sBgColor)
	{
		Color col = panel->GetBgColor();
		val.a = col[0];
		val.b = col[1];
		val.c = col[2];
		val.d = col[3];
	}
	else if ( var == m_sXPos )
	{
		int x, y;
		panel->GetPos(x, y);
		val.a = (float)( x - GetRelativeOffset( anim.align, true ) );
	}
	else if ( var == m_sYPos )
	{
		int x, y;
		panel->GetPos(x, y);
		val.a = (float)( y - GetRelativeOffset( anim.align, false ) );
	}
	else if ( var == m_sWide )
	{
		int w, h;
		panel->GetSize(w, h);
		val.a = (float)w;
	}
	else if ( var == m_sTall )
	{
		int w, h;
		panel->GetSize(w, h);
		val.a = (float)h;
	}
	else
	{
		KeyValues *outputData = new KeyValues(g_ScriptSymbols.String(var));
		if (panel->RequestInfo(outputData))
		{
			// find the var and lookup it's type
			KeyValues *kv = outputData->FindKey(g_ScriptSymbols.String(var));
			if (kv && kv->GetDataType() == KeyValues::TYPE_FLOAT)
			{
				val.a = kv->GetFloat();
				val.b = 0.0f;
				val.c = 0.0f;
				val.d = 0.0f;
			}
			else if (kv && kv->GetDataType() == KeyValues::TYPE_COLOR)
			{
				Color col = kv->GetColor();
				val.a = col[0];
				val.b = col[1];
				val.c = col[2];
				val.d = col[3];
			}
		}
		else
		{
		//	Assert(!("Unhandlable var in AnimationController::GetValue())"));
		}
		outputData->deleteThis();
	}
	return val;
}

//-----------------------------------------------------------------------------
// Purpose: Sets a value in a panel
//-----------------------------------------------------------------------------
void AnimationController::SetValue(ActiveAnimation_t& anim, Panel *panel, UtlSymId_t var, Value_t &value)
{
	if (var == m_sPosition)
	{
		int x = (int)value.a + GetRelativeOffset( anim.align, true );
		int y = (int)value.b + GetRelativeOffset( anim.align, false );
		panel->SetPos(x, y);
	}
	else if (var == m_sSize)
	{
		panel->SetSize((int)value.a, (int)value.b);
	}
	else if (var == m_sFgColor)
	{
		Color col = panel->GetFgColor();
		col[0] = (unsigned char)value.a;
		col[1] = (unsigned char)value.b;
		col[2] = (unsigned char)value.c;
		col[3] = (unsigned char)value.d;
		panel->SetFgColor(col);
	}
	else if (var == m_sBgColor)
	{
		Color col = panel->GetBgColor();
		col[0] = (unsigned char)value.a;
		col[1] = (unsigned char)value.b;
		col[2] = (unsigned char)value.c;
		col[3] = (unsigned char)value.d;
		panel->SetBgColor(col);
	}
	else if (var == m_sXPos)
	{
		int newx = (int)value.a + GetRelativeOffset( anim.align, true );
		int x, y;
		panel->GetPos( x, y );
		x = newx;
		panel->SetPos(x, y);
	}
	else if (var == m_sYPos)
	{
		int newy = (int)value.a + GetRelativeOffset( anim.align, false );
		int x, y;
		panel->GetPos( x, y );
		y = newy;
		panel->SetPos(x, y);
	}
	else if (var == m_sWide)
	{
		int neww = (int)value.a;
		int w, h;
		panel->GetSize( w, h );
		w = neww;
		panel->SetSize(w, h);
	}
	else if (var == m_sTall)
	{
		int newh = (int)value.a;
		int w, h;
		panel->GetSize( w, h );
		h = newh;
		panel->SetSize(w, h);
	}
	else
	{
		KeyValues *inputData = new KeyValues(g_ScriptSymbols.String(var));
		// set the custom value
		if (value.b == 0.0f && value.c == 0.0f && value.d == 0.0f)
		{
			// only the first value is non-zero, so probably just a float value
			inputData->SetFloat(g_ScriptSymbols.String(var), value.a);
		}
		else
		{
			// multivalue, set the color
			Color col((unsigned char)value.a, (unsigned char)value.b, (unsigned char)value.c, (unsigned char)value.d);
			inputData->SetColor(g_ScriptSymbols.String(var), col);
		}
		if (!panel->SetInfo(inputData))
		{
		//	Assert(!("Unhandlable var in AnimationController::SetValue())"));
		}
		inputData->deleteThis();
	}
}
// Hooks between panels and  animation controller system

class CPanelAnimationDictionary
{
public:
	CPanelAnimationDictionary() : m_PanelAnimationMapPool( 32 )
	{
	}

	~CPanelAnimationDictionary()
	{
		m_PanelAnimationMapPool.Clear();
	}

	PanelAnimationMap		*FindOrAddPanelAnimationMap( char const *className );
	PanelAnimationMap		*FindPanelAnimationMap( char const *className );
	void					PanelAnimationDumpVars( char const *className );
private:

	struct PanelAnimationMapDictionaryEntry
	{
		PanelAnimationMap *map;
	};

	char const *StripNamespace( char const *className );
	void PanelAnimationDumpMap( PanelAnimationMap *map, bool recursive );

	CClassMemoryPool< PanelAnimationMap > m_PanelAnimationMapPool;
	CUtlDict< PanelAnimationMapDictionaryEntry, int > m_AnimationMaps;
};


char const *CPanelAnimationDictionary::StripNamespace( char const *className )
{
	if ( !Q_strnicmp( className, "vgui::", 6 ) )
	{
		return className + 6;
	}
	return className;
}

//-----------------------------------------------------------------------------
// Purpose: Find but don't add mapping
//-----------------------------------------------------------------------------
PanelAnimationMap *CPanelAnimationDictionary::FindPanelAnimationMap( char const *className )
{
	int lookup = m_AnimationMaps.Find( StripNamespace( className ) );
	if ( lookup != m_AnimationMaps.InvalidIndex() )
	{
		return m_AnimationMaps[ lookup ].map;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
PanelAnimationMap *CPanelAnimationDictionary::FindOrAddPanelAnimationMap( char const *className )
{
	PanelAnimationMap *map = FindPanelAnimationMap( className );
	if ( map )
		return map;

	Panel::InitPropertyConverters();

	PanelAnimationMapDictionaryEntry entry;
	entry.map = (PanelAnimationMap *)m_PanelAnimationMapPool.Alloc();
	m_AnimationMaps.Insert( StripNamespace( className ), entry );
	return entry.map;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPanelAnimationDictionary::PanelAnimationDumpMap( PanelAnimationMap *map, bool recursive )
{
	if ( map->pfnClassName )
	{
		Msg( "%s\n", (*map->pfnClassName)() );
	}
	int c = map->entries.Count();
	for ( int i = 0; i < c; i++ )
	{
		PanelAnimationMapEntry *e = &map->entries[ i ];
		Msg( "  %s %s\n", e->type(), e->name() );
	}

	if ( recursive && map->baseMap )
	{
		PanelAnimationDumpMap( map->baseMap, recursive );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPanelAnimationDictionary::PanelAnimationDumpVars( char const *className )
{
	if ( className == NULL )
	{
		for ( int i = 0; i < (int)m_AnimationMaps.Count(); i++ )
		{
			PanelAnimationDumpMap( m_AnimationMaps[ i ].map, false );
		}
	}
	else
	{
		PanelAnimationMap *map = FindPanelAnimationMap( className );
		if ( map )
		{
			PanelAnimationDumpMap( map, true );
		}
		else
		{
			Msg( "No such Panel Animation class %s\n", className );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: singleton accessor
//-----------------------------------------------------------------------------
CPanelAnimationDictionary& GetPanelAnimationDictionary()
{
	static CPanelAnimationDictionary dictionary;
	return dictionary;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
PanelAnimationMap *FindOrAddPanelAnimationMap( char const *className )
{
	return GetPanelAnimationDictionary().FindOrAddPanelAnimationMap( className );
}

//-----------------------------------------------------------------------------
// Purpose: Find but don't add mapping
//-----------------------------------------------------------------------------
PanelAnimationMap *FindPanelAnimationMap( char const *className )
{
	return GetPanelAnimationDictionary().FindPanelAnimationMap( className );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PanelAnimationDumpVars( char const *className )
{
	GetPanelAnimationDictionary().PanelAnimationDumpVars( className );
}