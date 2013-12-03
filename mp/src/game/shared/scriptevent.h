//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef SCRIPTEVENT_H
#define SCRIPTEVENT_H

#define SCRIPT_EVENT_DEAD			1000		// character is now dead
#define SCRIPT_EVENT_NOINTERRUPT	1001		// does not allow interrupt
#define SCRIPT_EVENT_CANINTERRUPT	1002		// will allow interrupt
#define SCRIPT_EVENT_FIREEVENT		1003		// Fires OnScriptEventXX output in the script entity, where XX is the event number from the options data.
#define SCRIPT_EVENT_SOUND			1004		// Play named wave file (on CHAN_BODY)
#define SCRIPT_EVENT_SENTENCE		1005		// Play named sentence
#define SCRIPT_EVENT_INAIR			1006		// Leave the character in air at the end of the sequence (don't find the floor)
#define SCRIPT_EVENT_ENDANIMATION	1007		// Set the animation by name after the sequence completes
#define SCRIPT_EVENT_SOUND_VOICE	1008		// Play named wave file (on CHAN_VOICE)
#define	SCRIPT_EVENT_SENTENCE_RND1	1009		// Play sentence group 25% of the time
#define SCRIPT_EVENT_NOT_DEAD		1010		// Bring back to life (for life/death sequences)
#define SCRIPT_EVENT_EMPHASIS		1011		// Emphasis point for gestures

#define SCRIPT_EVENT_BODYGROUPON	1020		// Turn a bodygroup on
#define SCRIPT_EVENT_BODYGROUPOFF	1021		// Turn a bodygroup off
#define SCRIPT_EVENT_BODYGROUPTEMP	1022		// Turn a bodygroup on until this sequence ends

#define SCRIPT_EVENT_FIRE_INPUT		1100		// Fires named input on the event handler

#endif   //SCRIPTEVENT_H
