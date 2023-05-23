/******************************************************************************
*   Coffee.h 
*       This module contains the base definitions for the coffee tutorial
*       application.
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/
#pragma once

#include "resource.h"

// Global Variables:
HINSTANCE                       g_hInst;		// current instance
TCHAR g_szCounterDisplay[MAX_LOADSTRING];       // Display String for counter
CComPtr<ISpRecoGrammar>         g_cpCmdGrammar; // Pointer to our grammar object
CComPtr<ISpRecoContext>         g_cpRecoCtxt;   // Pointer to our recognition context
CComPtr<ISpRecognizer>		g_cpEngine;		// Pointer to our recognition engine instance
PMSGHANDLER                     g_fpCurrentPane;// Pointer to current message handler

// Foward declarations of functions included in this code module:
int                 APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                                     LPSTR lpCmdLine, int nCmdShow);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
HRESULT             InitSAPI( HWND hWnd );
void                CleanupSAPI( void );
void                ProcessRecoEvent( HWND hWnd );
void                ExecuteCommand(ISpPhrase *pPhrase, HWND hWnd);
LRESULT             EntryPaneProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
LRESULT             CounterPaneProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

// Declaration of UI specific routines located in display.cpp
ATOM				MyRegisterClass(HINSTANCE hInstance, WNDPROC WndProc);
BOOL				InitInstance(HINSTANCE, int);
void                EraseBackground( HDC hDC );
void                CleanupGDIObjects( void );
void                EntryPanePaint( HWND hWnd );
void                CounterPanePaint( HWND hWnd, LPCTSTR szCounterDisplay );


