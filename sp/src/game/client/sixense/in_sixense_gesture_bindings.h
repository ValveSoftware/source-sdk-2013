//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef IN_SIXENSE_GESTURES_H
#define IN_SIXENSE_GESTURES_H
#ifdef _WIN32
#pragma once
#endif

#include <sixense_utils/interfaces.hpp>

class SixenseGestureBindings {
public:
	SixenseGestureBindings();

	// Add a new binding. release_command can be empty. If press_command starts with '+', a release_command is generated with '-'.
	void AddBinding( CUtlString hand, CUtlString action, CUtlString arg, CUtlString press_command, CUtlString release_command );

	// Print bindings to console
	void ListBindings();

	// Write the bindings to a cfg file that can be loaded from the console or at startup (defaults to sixense_bindings.cfg)
	void WriteBindings( CUtlString filename );

	// Clear all bindings
	void ClearBindings();

	// Delete the nth binding
	void DeleteBinding( int n );

	// Create a set of default bindings appropriate for this game
	void CreateDefaultBindings();

	// Check to see if any bindings need to be triggered. disable_activations allows the caller to prevent new bindings from being triggered, while
	// still allowing enabled gestures to disable.
	void UpdateBindings( sixenseUtils::IButtonStates *pLeftButtonStates, sixenseUtils::IButtonStates *pRightButtonStates, bool bIsMenuVisible );

	// How many bindings are there?
	int GetNumBindings();

	// Allow per-game authorization of commmands when the menu is up
	bool AllowMenuCommand( char * );

	// Allow per-game authorization of commmands in general
	bool AllowCommand( char * );

protected:
	typedef struct {
		int m_Action;
		int m_iHand; // 0=left, 1=right
		int m_iArgument;
		char *m_pActivateCommand;
		char *m_pDeactivateCommand;
		bool m_bAutoMirrored;
	} GestureBinding;

	// some helpers for converting input strings
	bool HandFromString( CUtlString hand_str, int *hand );
	bool ActionFromString( CUtlString action_str, sixenseUtils::IButtonStates::ActionType *action );
	bool ButtonMaskFromString( CUtlString button, unsigned short *button_token );
	bool DirectionFromString( CUtlString dir_str, sixenseUtils::IButtonStates::Direction *dir );
	bool ActionTokenToStr( sixenseUtils::IButtonStates::ActionType action, char *buf, int buflen );
	bool DirectionTokenToStr( int arg, char *buf, int buflen );
	bool ButtonTokenToStr( int arg, char *buf, int buflen );
	bool HandTokenToStr( int hand, char *buf, int buflen );

	// Help deallocate a binding
	void FreeStrings( GestureBinding binding );

private:
	CUtlLinkedList<GestureBinding> m_GestureBindingList;

};

#endif
