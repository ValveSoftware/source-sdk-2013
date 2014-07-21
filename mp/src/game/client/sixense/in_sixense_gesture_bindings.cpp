//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SIXENSE

#include "in_sixense_gesture_bindings.h"
#include "filesystem.h"
#ifdef TF_CLIENT_DLL
#include "tf_gamerules.h"
#endif

#include <isixense.h>
#include <sixense_math.hpp>
#include <sixense_utils/interfaces.hpp>


SixenseGestureBindings::SixenseGestureBindings() 
{

}

// Help deallocate a binding
void SixenseGestureBindings::FreeStrings( GestureBinding binding )
{
	if( binding.m_pActivateCommand )
	{
		free( binding.m_pActivateCommand );
	}

	if( binding.m_pDeactivateCommand )
	{
		free( binding.m_pDeactivateCommand );
	}
}

// add a binding to the list. Calls should be in the form:
//   AddBinding( "left", "button_press", "start", "+dota_camera_follow", "" );
//
// args:
//   hand_str = "left" or "right"
//   action_str = 'button_press' 'trigger_press' 'tilt_gesture' 'point_gesture' 'velocity_gesture' or 'joystick_move'
//   argument_str depends on action_str, see below for details
//   press_command_str is the concommand executed on start of the action (ie button press) and release_command_str on stop (ie button release)
//   release_command_str can be the empty string if no stop action is desired
//   if the press_command_str begins with "+", an equivalent "-" is set to the release_command automatically
void SixenseGestureBindings::AddBinding( CUtlString hand_str, CUtlString action_str, CUtlString argument_str, CUtlString press_command_str, CUtlString release_command_str ) 
{
	GestureBinding binding;

	// Convert from strings to enums
	sixenseUtils::IButtonStates::ActionType action;
	if( !ActionFromString( action_str, &action ) ) 
	{
		return;
	}

	binding.m_Action = action;

	int hand;
	if( !HandFromString( hand_str, &hand ) ) {
		return;
	}

	binding.m_iHand = hand;


	// handle argument_str per-action type
	if( action == sixenseUtils::IButtonStates::ACTION_BUTTON_PRESS ) 
	{

		// button_press takes a button argument
		unsigned short button_token;
		if( !ButtonMaskFromString( argument_str, &button_token ) )
		{
			return;
		}

		binding.m_iArgument = button_token;
	} 
	else if( action == sixenseUtils::IButtonStates::ACTION_TRIGGER_PRESS ) 
	{
		// trigger press has no argument
		binding.m_iArgument = 0;
	}
	else
	{
		// all other actions take a direction
		sixenseUtils::IButtonStates::Direction dir;
		if( !DirectionFromString( argument_str, &dir ) )
		{
			return;
		}

		binding.m_iArgument = dir;

	}


	// copy the activate command
	binding.m_pActivateCommand = strdup( press_command_str.String() );

	binding.m_bAutoMirrored = false;

	// if there is an explicit release_command, use it
	if ( !release_command_str.IsEmpty() )
	{
		binding.m_pDeactivateCommand = strdup( release_command_str.String() );
	}
	else
	{
		// otherwise try to generate a release command

		// see if it starts with a +, if so, add an off command
		if( press_command_str[0] == '+' ) 
		{
			binding.m_pDeactivateCommand = strdup( press_command_str.String() );
			binding.m_pDeactivateCommand[0] = '-';
			binding.m_bAutoMirrored = true;
		}
		else
		{
			// Just leave release command null
			binding.m_pDeactivateCommand = NULL;
		}
	}

	// Try to keep a single binding per 'action' 'hand' 'arg' pair, ie one per button.
	// We may want to allow multiple if people think it would be useful...
	FOR_EACH_LL( m_GestureBindingList, it )
	{
		GestureBinding existing_binding = m_GestureBindingList[it];

		if( binding.m_Action == existing_binding.m_Action &&
			binding.m_iArgument == existing_binding.m_iArgument &&
			binding.m_iHand == existing_binding.m_iHand ) 
		{
			// Already the same binding active, delete it
			FreeStrings( existing_binding );
			m_GestureBindingList.Remove( it );
			break;
		}
	}

	// add to the list of bindings
	m_GestureBindingList.AddToTail( binding );

}

// just return the binding count
int SixenseGestureBindings::GetNumBindings() 
{
	return m_GestureBindingList.Count();
}

// pretty print all the bindings to the console
void SixenseGestureBindings::ListBindings()
{

	const int strbuflen=256;
	char strbuf[strbuflen];

	// Just go through all the bindings, use helpers to convert from tokens to strings, and print.
	int i=0;
	FOR_EACH_LL( m_GestureBindingList, it )
	{
		GestureBinding binding = m_GestureBindingList[it];

		if( HandTokenToStr( binding.m_iHand, strbuf, strbuflen ) )
		{
			Msg("%d)\t%s", i, strbuf);
		}

		if( ActionTokenToStr( (sixenseUtils::IButtonStates::ActionType)binding.m_Action, strbuf, strbuflen ) )
		{
			Msg("\t%s", strbuf);
		}

		if( binding.m_Action == sixenseUtils::IButtonStates::ACTION_BUTTON_PRESS ) 
		{

			if( ButtonTokenToStr( binding.m_iArgument, strbuf, strbuflen ) )
			{
				Msg("\t%s", strbuf);
			}

		}

		// these all have the same arguments (left, right, up...)
		else if( binding.m_Action == sixenseUtils::IButtonStates::ACTION_JOYSTICK_MOVE ||
			binding.m_Action == sixenseUtils::IButtonStates::ACTION_POINT_GESTURE ||
			binding.m_Action == sixenseUtils::IButtonStates::ACTION_VELOCITY_GESTURE ||
			binding.m_Action == sixenseUtils::IButtonStates::ACTION_TILT_GESTURE )
		{

			if( DirectionTokenToStr( binding.m_iArgument, strbuf, strbuflen ) )
			{
				Msg("\t%s", strbuf);
			}

		} else if( binding.m_Action == sixenseUtils::IButtonStates::ACTION_TRIGGER_PRESS ) 
		{
			Msg("\t"); // no argument
		}

		Msg("\t\"%s\"", binding.m_pActivateCommand);
		if( binding.m_pDeactivateCommand && !binding.m_bAutoMirrored ) // only print deactivated command if we didn't generate it.
		{
			Msg("\t\"%s\"", binding.m_pDeactivateCommand);
		} 

		Msg("\n");
		i++;
	}
}

// Write the current set of bindings to a file, formatted so that the file can be 'exec'ed.
void SixenseGestureBindings::WriteBindings( CUtlString filename_str )
{

	FileHandle_t hFile;

	const int filenamelen = 1024;
	char filename[filenamelen];

	// If no filename was provided, use "cfg/sixense_bindings.cfg"
	if( !filename_str.IsEmpty() )
	{
		Q_snprintf( filename, filenamelen, "%s\\cfg\\%s", engine->GetGameDirectory(), filename_str.String() );
	}
	else
	{
		Q_snprintf( filename, filenamelen, "%s\\cfg\\sixense_bindings.cfg", engine->GetGameDirectory() );
	}

	Msg("writing bindings to %s\n", filename );
	hFile = filesystem->Open( filename, "wt" );	

	if( !hFile )
	{
		return;
	}
	
	const int strbuflen=256;
	char strbuf[strbuflen];
	char writebuf[strbuflen];

	// Go through all the bindings, convert from tokens to strings, build up the command string and write it to the file
	FOR_EACH_LL( m_GestureBindingList, it )
	{
		GestureBinding binding = m_GestureBindingList[it];

		if( HandTokenToStr( binding.m_iHand, strbuf, strbuflen ) )
		{
			Q_snprintf( writebuf, strbuflen, "sixense_bind \"%s\"", strbuf);
			filesystem->Write( writebuf, strlen(writebuf), hFile );
		}

		if( ActionTokenToStr( (sixenseUtils::IButtonStates::ActionType)binding.m_Action, strbuf, strbuflen ) )
		{
			Q_snprintf( writebuf, strbuflen, " \"%s\"", strbuf);
			filesystem->Write( writebuf, strlen(writebuf), hFile );
		}

		if( binding.m_Action == sixenseUtils::IButtonStates::ACTION_BUTTON_PRESS ) {

			if( ButtonTokenToStr( binding.m_iArgument, strbuf, strbuflen ) )
			{
				Q_snprintf( writebuf, strbuflen, " \"%s\"", strbuf);
				filesystem->Write( writebuf, strlen(writebuf), hFile );
			}

		}

		// these all have the same arguments (left, right, up...)
		else if( binding.m_Action == sixenseUtils::IButtonStates::ACTION_JOYSTICK_MOVE ||
			binding.m_Action == sixenseUtils::IButtonStates::ACTION_POINT_GESTURE ||
			binding.m_Action == sixenseUtils::IButtonStates::ACTION_VELOCITY_GESTURE ||
			binding.m_Action == sixenseUtils::IButtonStates::ACTION_TILT_GESTURE )
		{

			if( DirectionTokenToStr( binding.m_iArgument, strbuf, strbuflen ) )
			{
				Q_snprintf( writebuf, strbuflen, " \"%s\"", strbuf);
				filesystem->Write( writebuf, strlen(writebuf), hFile );
			}

		} else if( binding.m_Action == sixenseUtils::IButtonStates::ACTION_TRIGGER_PRESS ) 
		{
			Q_snprintf( writebuf, strbuflen, " \"\""); // no argument
			filesystem->Write( writebuf, strlen(writebuf), hFile );
		}

		Q_snprintf( writebuf, strbuflen, " \"%s\"", binding.m_pActivateCommand);
		filesystem->Write( writebuf, strlen(writebuf), hFile );
		if( binding.m_pDeactivateCommand && !binding.m_bAutoMirrored ) // only print deactivated command if we didn't generate it.
		{
			Q_snprintf( writebuf, strbuflen, " \"%s\"", binding.m_pDeactivateCommand);
			filesystem->Write( writebuf, strlen(writebuf), hFile );
		} 

		Q_snprintf( writebuf, strbuflen, "\n");
		filesystem->Write( writebuf, strlen(writebuf), hFile );
	}

	filesystem->Close( hFile );
}

// Erase all the bindings. Right now this will cause code in in_sixense.cpp to detect the lack of bindings and immediately
// create defaults. I think that's good.
void SixenseGestureBindings::ClearBindings()
{
	FOR_EACH_LL( m_GestureBindingList, it )
	{
		FreeStrings( m_GestureBindingList[it] );
	}
	m_GestureBindingList.RemoveAll();
}

// Delete a specific binding by index. 'sixense_list_bindings' prints the indicies that can be passed to delete.
void SixenseGestureBindings::DeleteBinding( int num )
{
	if( num < 0 || num > m_GestureBindingList.Count()-1 ) return;

	int count=0;
	FOR_EACH_LL( m_GestureBindingList, it )
	{
		if( count == num ) 
		{
			FreeStrings( m_GestureBindingList[it] );
			m_GestureBindingList.Remove(it);
			Msg("Removed %d\n", count );
			break;
		}
		count++;
	}

}

void SixenseGestureBindings::CreateDefaultBindings()
{
	ClearBindings();

#if defined ( CSTRIKE_DLL ) && !defined( TERROR )

	AddBinding( "left", "tilt_gesture", "ccw", "+reload", "" );
	AddBinding( "left", "tilt_gesture", "down", "+duck", "" );
	AddBinding( "left", "tilt_gesture", "up", "+jump", "" );
	AddBinding( "left", "trigger_press", "", "+attack2", "" );
	AddBinding( "left", "button_press", "start", "cancelselect", "" );
	AddBinding( "left", "button_press", "bumper", "+duck", "" );
	AddBinding( "left", "button_press", "joystick", "drop", "" );
	AddBinding( "left", "button_press", "1", "autobuy", "" );
	AddBinding( "left", "button_press", "2", "rebuy", "" );
	AddBinding( "left", "button_press", "3", "impulse 201", "" );
	AddBinding( "left", "button_press", "4", "chooseteam", "" );
	AddBinding( "left", "point_gesture", "up", "slot3", "" );
	AddBinding( "left", "point_gesture", "down", "slot4", "" );
	AddBinding( "right", "joystick_move", "up", "buymenu", "buymenu 0" );
	AddBinding( "right", "joystick_move", "left", "invprev", "" );
	AddBinding( "right", "joystick_move", "right", "invnext", "" );
	AddBinding( "right", "joystick_move", "down", "nightvision", "" );
	AddBinding( "right", "button_press", "1", "+sixense_ratchet", "" );
	AddBinding( "right", "button_press", "2", "+use", "" );
	AddBinding( "right", "button_press", "3", "+speed", "" );
	AddBinding( "right", "button_press", "4", "+voicerecord", "" );
	AddBinding( "right", "button_press", "joystick", "impulse 100", "" );
	AddBinding( "right", "button_press", "bumper", "+jump", "" );
	AddBinding( "right", "trigger_press", "", "+attack", "" );
	AddBinding( "right", "button_press", "start", "+showscores", "" );

#elif defined( TERROR )

	AddBinding( "left", "tilt_gesture", "ccw", "+reload", "" );
	AddBinding( "left", "tilt_gesture", "down", "+duck", "" );
	AddBinding( "left", "tilt_gesture", "up", "+jump", "" );
	AddBinding( "left", "trigger_press", "", "+attack2", "" );
	AddBinding( "left", "button_press", "start", "cancelselect", "" );
	AddBinding( "left", "button_press", "bumper", "+sixense_left_point_gesture", "" );
	AddBinding( "left", "button_press", "joystick", "+use", "" );
	AddBinding( "left", "point_gesture", "left", "invprev", "" );
	AddBinding( "left", "point_gesture", "right", "invnext", "" );
	AddBinding( "left", "point_gesture", "up", "slot3", "" );
	AddBinding( "left", "point_gesture", "down", "slot4", "" );
	AddBinding( "left", "button_press", "2", "phys_swap", "" );
	AddBinding( "left", "button_press", "1", "+sixense_ratchet", "" );
	AddBinding( "left", "button_press", "3", "+speed", "" );
	AddBinding( "left", "button_press", "4", "+voicerecord", "" );
	AddBinding( "right", "joystick_move", "up", "+zoom", "" );
	AddBinding( "right", "joystick_move", "left", "invprev", "" );
	AddBinding( "right", "joystick_move", "right", "invnext", "" );
	AddBinding( "right", "joystick_move", "down", "lastinv", "" );
	AddBinding( "right", "button_press", "2", "Vote no", "" );
	AddBinding( "right", "button_press", "1", "Vote yes", "" );
	AddBinding( "right", "button_press", "3", "askconnect_accept", "" );
	AddBinding( "right", "button_press", "4", "jpeg", "" );
	AddBinding( "right", "button_press", "joystick", "impulse 100", "" );
	AddBinding( "right", "button_press", "bumper", "+duck", "" );
	AddBinding( "right", "trigger_press", "", "+attack", "" );
	AddBinding( "right", "button_press", "1", "+sixense_ratchet", "" );

#elif defined( DOTA_DLL )

	AddBinding( "left", "button_press", "start", "+dota_camera_follow", "" );
	AddBinding( "left", "button_press", "3", "dota_ability_execute 0", "" );
	AddBinding( "left", "button_press", "1", "dota_ability_execute 1", "" );
	AddBinding( "left", "button_press", "2", "dota_ability_execute 2", "" );
	AddBinding( "left", "button_press", "4", "dota_ability_execute 5", "" );
	AddBinding( "left", "joystick_move", "left", "dota_ability_execute 3", "" );
	AddBinding( "left", "joystick_move", "right", "dota_ability_execute 4", "" );
	AddBinding( "left", "joystick_move", "up", "mc_attack", "" );
	AddBinding( "left", "joystick_move", "down", "+sixense_left_alt", "" ); // ping
	AddBinding( "left", "trigger_press", "", "+sixense_camera_pan", "" );
	AddBinding( "left", "button_press", "bumper", "+sixense_camera_drag", "" );

	AddBinding( "right", "button_press", "bumper", "+sixense_left_shift", "" ); // queue
	AddBinding( "right", "button_press", "joystick", "sixense_mouse_set_origin", "" );
	AddBinding( "right", "button_press", "start", "toggleshoppanel", "" );
	AddBinding( "right", "joystick_move", "down", "+sixense_left_ctrl", "" ); // ??
	AddBinding( "right", "joystick_move", "up", "+showscores", "" );
	AddBinding( "right", "button_press", "3", "+attack", "" );
	AddBinding( "right", "button_press", "1", "+attack2", "" );
	AddBinding( "right", "button_press", "4", "+voicerecord", "" );
	AddBinding( "right", "button_press", "3", "+sixense_left_click", "" );
	AddBinding( "right", "button_press", "1", "+sixense_right_click", "" );
	AddBinding( "right", "trigger_press", "", "+sixense_grid 0", "" );
	AddBinding( "right", "button_press", "2", "+sixense_grid 1", "" );

#elif defined( TF_CLIENT_DLL )

	AddBinding( "left", "tilt_gesture", "ccw", "+reload", "" );
	AddBinding( "left", "tilt_gesture", "down", "+duck", "" );
	AddBinding( "left", "tilt_gesture", "up", "+jump", "" );
	AddBinding( "left", "tilt_gesture", "right", "impulse 201", "" );
	AddBinding( "left", "trigger_press", "", "+attack2", "" );
	AddBinding( "left", "button_press", "start", "cancelselect", "" );
	AddBinding( "left", "button_press", "bumper", "+duck", "" );
	AddBinding( "left", "point_gesture", "up", "slot3", "" );
	AddBinding( "left", "point_gesture", "down", "slot4", "" );
	AddBinding( "left", "button_press", "3", "open_charinfo_direct", "" );
	AddBinding( "left", "button_press", "1", "changeclass", "" );
	AddBinding( "left", "button_press", "2", "changeteam", "" );
	AddBinding( "left", "button_press", "4", "lastdisguise", "" );
	AddBinding( "left", "button_press", "joystick", "voicemenu 0 0", "" );
	AddBinding( "right", "joystick_move", "up", "+context_action", "" );
	AddBinding( "right", "joystick_move", "left", "invprev", "" );
	AddBinding( "right", "joystick_move", "right", "invnext", "" );
	AddBinding( "right", "joystick_move", "down", "lastinv", "" );
	AddBinding( "right", "button_press", "1", "+sixense_ratchet", "" );
	AddBinding( "right", "button_press", "2", "cl_decline_first_notification", "" );
	AddBinding( "right", "button_press", "3", "+voicerecord", "" );
	AddBinding( "right", "button_press", "4", "cl_trigger_first_notification", "" );
	AddBinding( "right", "button_press", "joystick", "dropitem", "" );
	AddBinding( "right", "button_press", "bumper", "taunt", "" );
	AddBinding( "right", "trigger_press", "", "+attack", "" );
	AddBinding( "right", "button_press", "start", "+showscores", "" );

#else

	AddBinding( "left", "tilt_gesture", "ccw", "+reload", "" );
	AddBinding( "left", "tilt_gesture", "down", "+duck", "" );
	AddBinding( "left", "tilt_gesture", "up", "+jump", "" );
	AddBinding( "left", "trigger_press", "", "+attack2", "" );
	AddBinding( "left", "button_press", "start", "cancelselect", "" );
	AddBinding( "left", "button_press", "bumper", "+sixense_left_point_gesture", "" );
	AddBinding( "left", "button_press", "joystick", "+use", "" );
	AddBinding( "left", "point_gesture", "left", "invprev", "" );
	AddBinding( "left", "point_gesture", "right", "invnext", "" );
	AddBinding( "left", "point_gesture", "up", "slot3", "" );
	AddBinding( "left", "point_gesture", "down", "slot4", "" );
	AddBinding( "right", "joystick_move", "up", "+zoom", "" );
	AddBinding( "right", "joystick_move", "left", "invprev", "" );
	AddBinding( "right", "joystick_move", "right", "invnext", "" );
	AddBinding( "right", "joystick_move", "down", "lastinv", "" );
	AddBinding( "right", "button_press", "2", "phys_swap", "" );
	AddBinding( "right", "button_press", "1", "+sixense_ratchet", "" );
	AddBinding( "right", "button_press", "3", "+speed", "" );
	AddBinding( "right", "button_press", "4", "+voicerecord", "" );
	AddBinding( "right", "button_press", "joystick", "impulse 100", "" );
	AddBinding( "right", "button_press", "bumper", "+duck", "" );
	AddBinding( "right", "trigger_press", "", "+attack", "" );

#endif

}

// Called each frame with the latest controller info. sixenseUtils::IButtonStates are sixenseUtils classes that look for various transitions of
// controller state, whether it's 'buttonJustPressed', or analog joysticks 'joystickJustPushed(up)'. It now has support for motion triggers like
// 'controllerJustTilted(left)'
void SixenseGestureBindings::UpdateBindings( sixenseUtils::IButtonStates *pLeftButtonStates, sixenseUtils::IButtonStates *pRightButtonStates, bool bIsMenuVisible )
{
	// go through all the bindings and see if any of the desired actions just transitioned on or off
	FOR_EACH_LL( m_GestureBindingList, it )
	{
		GestureBinding binding = m_GestureBindingList[it];

		if( binding.m_iHand == 0 )
		{

			// left hand

			// just started?
			if( pLeftButtonStates->justStarted( (sixenseUtils::IButtonStates::ActionType)binding.m_Action, binding.m_iArgument ) )
			{
				if( binding.m_pActivateCommand )
				{

					// Allow per-game authorization of commmands when the menu is up
					if( bIsMenuVisible && !AllowMenuCommand( binding.m_pActivateCommand )  ) 
					{
						continue;
					}

					// Allow per-game authorization of commmands
					if( !AllowCommand( binding.m_pActivateCommand ) )
					{
						continue;
					}

					engine->ClientCmd_Unrestricted( binding.m_pActivateCommand );
					//Msg("activate: %s\n", binding.m_pActivateCommand );
				}
			}

			// just stopped?
			if( pLeftButtonStates->justStopped( (sixenseUtils::IButtonStates::ActionType)binding.m_Action, binding.m_iArgument ) )
			{
				if( binding.m_pDeactivateCommand )
				{
					engine->ClientCmd_Unrestricted( binding.m_pDeactivateCommand );
					//Msg("deactivate: %s\n", binding.m_pDeactivateCommand );
				}
			}
		}
		else
		{
			// right hand

			// just started?
			if( pRightButtonStates->justStarted( (sixenseUtils::IButtonStates::ActionType)binding.m_Action, binding.m_iArgument ) )
			{
				if( binding.m_pActivateCommand )
				{

					// Allow per-game authorization of commmands when the menu is up
					if( bIsMenuVisible && !AllowMenuCommand( binding.m_pActivateCommand )  ) 
					{
						continue;
					}

					// Allow per-game authorization of commmands
					if( !AllowCommand( binding.m_pActivateCommand ) )
					{
						continue;
					}

					engine->ClientCmd_Unrestricted( binding.m_pActivateCommand );
					//Msg("activate: %s\n", binding.m_pActivateCommand );
				}
			}

			// just stopped?
			if( pRightButtonStates->justStopped( (sixenseUtils::IButtonStates::ActionType)binding.m_Action, binding.m_iArgument ) )
			{
				if( binding.m_pDeactivateCommand )
				{
					engine->ClientCmd_Unrestricted( binding.m_pDeactivateCommand );
					//Msg("deactivate: %s\n", binding.m_pDeactivateCommand );
				}
			}
		}
	}
}


// Give games an opportunity to block commands
bool SixenseGestureBindings::AllowCommand( char *pActivateCommand )
{
#ifdef TF_CLIENT_DLL
	if ( TFGameRules() && TFGameRules()->IsInTraining() && TFGameRules()->IsWaitingForTrainingContinue() )
	{
		if( Q_strcmp( pActivateCommand, "+jump" ) == 0 )
		{
			return false;
		}
	}
#endif

	return true;
}

// If the menu is up, most bindings are blocked. Allow some of them to be activated...
bool SixenseGestureBindings::AllowMenuCommand( char *pActivateCommand )
{

#ifdef TF_CLIENT_DLL
	if( Q_strcmp( pActivateCommand, "+showscores" ) == 0 )
	{
		// Allow for showscores when in-menu
		return true;
	}
#endif

	return false;

}

// from here down are just helper funcs to convert between enums and strings and back

bool SixenseGestureBindings::DirectionFromString( CUtlString dir_str, sixenseUtils::IButtonStates::Direction *dir ) 
{

	if( dir_str == "up" ) 
	{
		*dir = sixenseUtils::IButtonStates::DIR_UP;
	}
	else if( dir_str == "down" ) 
	{
		*dir = sixenseUtils::IButtonStates::DIR_DOWN;
	}
	else if( dir_str == "left" ) 
	{
		*dir = sixenseUtils::IButtonStates::DIR_LEFT;
	}
	else if( dir_str == "right" ) 
	{
		*dir = sixenseUtils::IButtonStates::DIR_RIGHT;
	}
	else if( dir_str == "cw" ) 
	{
		*dir = sixenseUtils::IButtonStates::DIR_CW;
	}
	else if( dir_str == "ccw" ) 
	{
		*dir = sixenseUtils::IButtonStates::DIR_CCW;
	}
	else
	{
		Msg( "Unknown direction %s, shoud be 'up' 'down' 'left' 'right' 'cw' or 'ccw'\n", dir_str.String() );
		return false;
	}

	return true;
}


bool SixenseGestureBindings::ButtonMaskFromString( CUtlString button, unsigned short *button_token ) 
{

	if ( button == "1" )
	{
		*button_token = SIXENSE_BUTTON_1;
	}
	else if ( button == "2" )
	{
		*button_token = SIXENSE_BUTTON_2;
	}
	else if ( button == "3" )
	{
		*button_token = SIXENSE_BUTTON_3;
	}
	else if ( button == "4" )
	{
		*button_token = SIXENSE_BUTTON_4;
	}
	else if ( button == "start" )
	{
		*button_token = SIXENSE_BUTTON_START;
	}
	else if ( button == "bumper" )
	{
		*button_token = SIXENSE_BUTTON_BUMPER;
	}
	else if ( button == "joystick" )
	{
		*button_token = SIXENSE_BUTTON_JOYSTICK;
	}
	else
	{
		Msg( "Unknown button %s, shoud be '1' '2' '3' '4' 'start' 'trigger' or 'joystick'\n", button.String() );
		return false;
	}

	return true;
}

bool SixenseGestureBindings::ActionFromString( CUtlString action_str, sixenseUtils::IButtonStates::ActionType *action ) 
{
	if( action_str == "button_press" ) 
	{
		*action = sixenseUtils::IButtonStates::ACTION_BUTTON_PRESS;
		return true;

	} 
	else if( action_str == "trigger_press" ) 
	{
		*action = sixenseUtils::IButtonStates::ACTION_TRIGGER_PRESS;
		return true;

	} 
	else if( action_str == "tilt_gesture" ) 
	{
		*action = sixenseUtils::IButtonStates::ACTION_TILT_GESTURE;
		return true;

	} 
	else if( action_str == "point_gesture" ) 
	{
		*action = sixenseUtils::IButtonStates::ACTION_POINT_GESTURE;
		return true;

	} 
	else if( action_str == "velocity_gesture" ) 
	{
		*action = sixenseUtils::IButtonStates::ACTION_VELOCITY_GESTURE;
		return true;

	} 
	else if( action_str == "joystick_move" ) 
	{
		*action = sixenseUtils::IButtonStates::ACTION_JOYSTICK_MOVE;
		return true;

	} 
	else 
	{
		Msg( "Unknown action %s, shoud be 'button_press' 'trigger_press' 'tilt_gesture' 'point_gesture' 'velocity_gesture' or 'joystick_move'\n", action_str.String() );
		*action = sixenseUtils::IButtonStates::ACTION_BUTTON_PRESS;
		return false;
	}
}

bool SixenseGestureBindings::HandFromString( CUtlString hand_str, int *hand ) 
{

	if( hand_str == "left" ) 
	{
		*hand = 0;

	} 
	else if( hand_str == "right" ) 
	{
		*hand = 1;

	} 
	else 
	{
		Msg( "Unknown controller %s, should be 'left' or 'right'\n", hand_str.String() );
		return false;
	}

	return true;
}


bool SixenseGestureBindings::HandTokenToStr( int hand, char *buf, int buflen ) 
{
	if( buflen < 10 ) return false;

	if( hand == 0 ) {
		Q_snprintf( buf, buflen, "left" );
	}
	else if( hand == 1 )
	{
		Q_snprintf( buf, buflen, "right" );
	}
	else
	{
		return false;
	}

	return true;
}

bool SixenseGestureBindings::ButtonTokenToStr( int arg, char *buf, int buflen ) 
{
	if( buflen < 10 ) return false;

	if( arg == SIXENSE_BUTTON_1 )
	{
		Q_snprintf( buf, buflen, "1" );
	} 
	else if( arg == SIXENSE_BUTTON_2 )
	{
		Q_snprintf( buf, buflen, "2" );
	} 
	else if( arg == SIXENSE_BUTTON_3 )
	{
		Q_snprintf( buf, buflen, "3" );
	} 
	else if( arg == SIXENSE_BUTTON_4 )
	{
		Q_snprintf( buf, buflen, "4" );
	} 
	else if( arg == SIXENSE_BUTTON_START )
	{
		Q_snprintf( buf, buflen, "start" );
	} 
	else if( arg == SIXENSE_BUTTON_BUMPER )
	{
		Q_snprintf( buf, buflen, "bumper" );
	} 
	else if( arg == SIXENSE_BUTTON_JOYSTICK )
	{
		Q_snprintf( buf, buflen, "joystick" );
	}
	else
	{
		return false;
	}

	return true;
}

bool SixenseGestureBindings::DirectionTokenToStr( int arg, char *buf, int buflen ) 
{
	if( buflen < 10 ) return false;

	if( arg == sixenseUtils::IButtonStates::DIR_LEFT )
	{
		Q_snprintf( buf, buflen, "left" );
	} 
	else if( arg == sixenseUtils::IButtonStates::DIR_RIGHT )
	{
		Q_snprintf( buf, buflen, "right" );
	} 
	else if( arg == sixenseUtils::IButtonStates::DIR_UP )
	{
		Q_snprintf( buf, buflen, "up" );
	} 
	else if( arg == sixenseUtils::IButtonStates::DIR_DOWN )
	{
		Q_snprintf( buf, buflen, "down" );
	} 
	else if( arg == sixenseUtils::IButtonStates::DIR_CW )
	{
		Q_snprintf( buf, buflen, "cw" );
	} 
	else if( arg == sixenseUtils::IButtonStates::DIR_CCW )
	{
		Q_snprintf( buf, buflen, "ccw" );
	} 
	else if( arg == sixenseUtils::IButtonStates::DIR_FORWARD )
	{
		Q_snprintf( buf, buflen, "forward" );
	}
	else if( arg == sixenseUtils::IButtonStates::DIR_BACKWARD )
	{
		Q_snprintf( buf, buflen, "backward" );
	}
	else
	{
		return false;
	}

	return true;
}

bool SixenseGestureBindings::ActionTokenToStr( sixenseUtils::IButtonStates::ActionType action, char *buf, int buflen ) 
{
	if( buflen < 20 ) return false;

	if( action == sixenseUtils::IButtonStates::ACTION_BUTTON_PRESS ) 
	{
		Q_snprintf( buf, buflen, "button_press" );
	}
	else if( action == sixenseUtils::IButtonStates::ACTION_JOYSTICK_MOVE ) 
	{
		Q_snprintf( buf, buflen, "joystick_move" );
	}
	else if( action == sixenseUtils::IButtonStates::ACTION_POINT_GESTURE ) 
	{
		Q_snprintf( buf, buflen, "point_gesture" );
	}
	else if( action == sixenseUtils::IButtonStates::ACTION_VELOCITY_GESTURE ) 
	{
		Q_snprintf( buf, buflen, "velocity_gesture" );
	}
	else if( action == sixenseUtils::IButtonStates::ACTION_TILT_GESTURE ) 
	{
		Q_snprintf( buf, buflen, "tilt_gesture" );
	}
	else if( action == sixenseUtils::IButtonStates::ACTION_TRIGGER_PRESS ) 
	{
		Q_snprintf( buf, buflen, "trigger_press" );
	} 
	else
	{
		return false;
	}

	return true;
}


#endif
