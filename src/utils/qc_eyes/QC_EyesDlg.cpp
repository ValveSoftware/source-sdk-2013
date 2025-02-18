//========= Copyright Valve Corporation, All rights reserved. ============//
// QC_EyesDlg.cpp : implementation file
//

#include "stdafx.h"
#include <math.h>
#include "QC_Eyes.h"
#include "QC_EyesDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CQC_EyesDlg dialog

CQC_EyesDlg::CQC_EyesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CQC_EyesDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CQC_EyesDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pBitmapHead = NULL;
}

void CQC_EyesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CQC_EyesDlg)
	DDX_Control(pDX, IDC_LEFT_LID_CONTROL, m_IndependentLeftLidControl);
	DDX_Control(pDX, IDC_PICTURES, m_PictureControl);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CQC_EyesDlg, CDialog)
	//{{AFX_MSG_MAP(CQC_EyesDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_CREATE_QC_TEXT, OnCreateQcText)
	ON_BN_CLICKED(IDC_IRIS_COLOR_BROWN, OnIrisColorBrown)
	ON_BN_CLICKED(IDC_IRIS_COLOR_GREEN, OnIrisColorGreen)
	ON_BN_CLICKED(IDC_IRIS_COLOR_BLUE, OnIrisColorBlue)
	ON_BN_CLICKED(IDC_EYE_COLOR_DARK, OnEyeColorDark)
	ON_BN_CLICKED(IDC_EYE_COLOR_LIGHT, OnEyeColorLight)
	ON_EN_SETFOCUS(IDC_RIGHT_EYE_X, OnSetfocusRightEyeX)
	ON_EN_SETFOCUS(IDC_RIGHT_EYE_Y, OnSetfocusRightEyeY)
	ON_EN_SETFOCUS(IDC_RIGHT_EYE_Z, OnSetfocusRightEyeZ)
	ON_EN_SETFOCUS(IDC_LEFT_EYE_X, OnSetfocusLeftEyeX)
	ON_EN_SETFOCUS(IDC_LEFT_EYE_Y, OnSetfocusLeftEyeY)
	ON_EN_SETFOCUS(IDC_LEFT_EYE_Z, OnSetfocusLeftEyeZ)
	ON_EN_SETFOCUS(IDC_UPPER_LID_LOWERED, OnSetfocusUpperLidLowered)
	ON_EN_SETFOCUS(IDC_UPPER_LID_NEUTRAL, OnSetfocusUpperLidNeutral)
	ON_EN_SETFOCUS(IDC_UPPER_LID_RAISED, OnSetfocusUpperLidRaised)
	ON_EN_SETFOCUS(IDC_LOWER_LID_LOWERED, OnSetfocusLowerLidLowered)
	ON_EN_SETFOCUS(IDC_LOWER_LID_NEUTRAL, OnSetfocusLowerLidNeutral)
	ON_EN_SETFOCUS(IDC_LOWER_LID_RAISED, OnSetfocusLowerLidRaised)
	ON_BN_CLICKED(IDC_COPY_TEXT_TO_CLIPBOARD, OnCopyTextToClipboard)
	ON_BN_CLICKED(IDC_DEFAULT_CONTROLS, OnDefaultControls)
	ON_BN_CLICKED(IDC_ADVANCED_CONTROLS, OnAdvancedControls)
	ON_BN_CLICKED(IDC_LEFT_LID_CONTROL, OnLeftLidControl)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CQC_EyesDlg::AddText( const char *pFormat, ... )
{
	char tempMsg[4096];
	va_list marker;
	va_start( marker, pFormat );
	_vsnprintf( tempMsg, sizeof( tempMsg ), pFormat, marker );
	tempMsg[ ARRAYSIZE(tempMsg) - 1 ] = 0;
	va_end( marker );

	size_t nCharsInBuf = strlen( m_Buf );
	size_t nCharsInMsg = strlen( tempMsg );

	if ( nCharsInBuf + nCharsInMsg + 1 > m_BufSize )
	{
		m_BufSize = nCharsInBuf + nCharsInMsg + 4096;
		char *newbuf = new char[m_BufSize];
		memcpy( newbuf, m_Buf, nCharsInBuf + 1 );
		delete [] m_Buf;
		m_Buf = newbuf;
	}
	
	strcat( m_Buf, tempMsg );
}


void SendToEditControl( HWND hEditControl, const char *pText )
{
	LRESULT nLen = SendMessage( hEditControl, EM_GETLIMITTEXT, 0, 0 );
	SendMessage( hEditControl, EM_SETSEL, nLen, nLen );
	SendMessage( hEditControl, EM_REPLACESEL, FALSE, (LPARAM)pText );
}


void FormatAndSendToEditControl( void *hWnd, const char *pText )
{
	HWND hEditControl = (HWND)hWnd;

	// Translate \n to \r\n.
	char outMsg[1024];
	const char *pIn = pText;
	char *pOut = outMsg;
	while ( *pIn )
	{
		if ( *pIn == '\n' )
		{
			*pOut = '\r';
			pOut++;
		}
		*pOut = *pIn;

		++pIn;
		++pOut;

		if ( pOut - outMsg >= 1020 )
		{
			*pOut = 0;
			SendToEditControl( hEditControl, outMsg );
			pOut = outMsg;
		}
	}
	*pOut = 0;
	SendToEditControl( hEditControl, outMsg );
}


HBITMAP CQC_EyesDlg::GetCachedBitmap( UINT id )
{
	for ( CBitmapRef *pCur=m_pBitmapHead; pCur; pCur=pCur->m_pNext )
	{
		if ( pCur->m_iResource == id )
			return pCur->m_hBitmap;
	}

	CBitmapRef *pNew = new CBitmapRef;
	pNew->m_iResource = id;
	pNew->m_hBitmap = ::LoadBitmap( AfxGetInstanceHandle(), MAKEINTRESOURCE(id) );
	pNew->m_pNext = m_pBitmapHead;
	m_pBitmapHead = pNew;

	return pNew->m_hBitmap;
}


/////////////////////////////////////////////////////////////////////////////
// CQC_EyesDlg message handlers

BOOL CQC_EyesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	GetDlgItem( IDC_REFERENCE_FILENAME )->SetWindowText( "filename_reference" );
	GetDlgItem( IDC_EXPRESSIONS_FILENAME )->SetWindowText( "filename_expressions" );
	GetDlgItem( IDC_MODEL_FILENAME )->SetWindowText( "filename_model" );
	GetDlgItem( IDC_IRIS_SIZE )->SetWindowText( "0.63" );
	GetDlgItem( IDC_EYEBALL_SIZE )->SetWindowText( "1.0" );

	::SendMessage( ::GetDlgItem( m_hWnd, IDC_Y_AXIS_UP ), BM_SETCHECK, BST_CHECKED, 0 );
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_DEFAULT_CONTROLS ), BM_SETCHECK, BST_CHECKED, 0 );
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_IRIS_COLOR_BROWN ), BM_SETCHECK, BST_CHECKED, 0 );
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_EYE_COLOR_LIGHT ), BM_SETCHECK, BST_CHECKED, 0 );

	m_hOutputText = ::GetDlgItem( m_hWnd, IDC_OUTPUT_TEXT );

	m_PictureControl.SetBitmap( GetCachedBitmap( IDB_EYE_DEFAULT ) );
	OnDefaultControls(); // Hide the advanced controls.
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CQC_EyesDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CQC_EyesDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}


void HandleYAxisUp( float &yVal, float &zVal )
{
	float flTemp = yVal;
	yVal = -zVal;
	zVal = flTemp;
}


float CQC_EyesDlg::GetDlgItemFloat( UINT id )
{
	char text[4096];
	GetDlgItemText( id, text, sizeof( text ) );
	return (float)atof( text );
}


bool CQC_EyesDlg::IsOptionChecked( UINT option )
{
	return (::SendMessage( ::GetDlgItem( m_hWnd, option ), BM_GETCHECK, 0, 0 ) == BST_CHECKED);
}


void CQC_EyesDlg::GetDialogParams( CDialogParams &p )
{
	p.m_flLeftEye[0] = GetDlgItemFloat( IDC_LEFT_EYE_X );
	p.m_flLeftEye[1] = GetDlgItemFloat( IDC_LEFT_EYE_Y );
	p.m_flLeftEye[2] = GetDlgItemFloat( IDC_LEFT_EYE_Z );

	p.m_flRightEye[0] = GetDlgItemFloat( IDC_RIGHT_EYE_X );
	p.m_flRightEye[1] = GetDlgItemFloat( IDC_RIGHT_EYE_Y );
	p.m_flRightEye[2] = GetDlgItemFloat( IDC_RIGHT_EYE_Z );

	bool bYAxisUp = IsOptionChecked( IDC_Y_AXIS_UP );
	if ( bYAxisUp )
	{
		HandleYAxisUp( p.m_flLeftEye[1], p.m_flLeftEye[2] );
		HandleYAxisUp( p.m_flRightEye[1], p.m_flRightEye[2] );
	}

	GetDlgItemText( IDC_REFERENCE_FILENAME, p.m_ReferenceFilename, sizeof( p.m_ReferenceFilename ) );
	GetDlgItemText( IDC_EXPRESSIONS_FILENAME, p.m_ExpressionsFilename, sizeof( p.m_ExpressionsFilename ) );
	GetDlgItemText( IDC_MODEL_FILENAME, p.m_ModelFilename, sizeof( p.m_ModelFilename ) );

	p.m_flIrisSize = GetDlgItemFloat( IDC_IRIS_SIZE );
	p.m_flEyeballSize = GetDlgItemFloat( IDC_EYEBALL_SIZE );

	p.m_flRightUpperLidRaised = GetDlgItemFloat( IDC_UPPER_LID_RAISED );
	p.m_flRightUpperLidNeutral = GetDlgItemFloat( IDC_UPPER_LID_NEUTRAL );
	p.m_flRightUpperLidLowered = GetDlgItemFloat( IDC_UPPER_LID_LOWERED );

	p.m_flRightLowerLidRaised = GetDlgItemFloat( IDC_LOWER_LID_RAISED );
	p.m_flRightLowerLidNeutral = GetDlgItemFloat( IDC_LOWER_LID_NEUTRAL );
	p.m_flRightLowerLidLowered = GetDlgItemFloat( IDC_LOWER_LID_LOWERED );

	if ( IsIndependentLeftLidControlEnabled() )
	{
		p.m_flLeftUpperLidRaised = GetDlgItemFloat( IDC_UPPER_LEFT_LID_RAISED );
		p.m_flLeftUpperLidNeutral = GetDlgItemFloat( IDC_UPPER_LEFT_LID_NEUTRAL );
		p.m_flLeftUpperLidLowered = GetDlgItemFloat( IDC_UPPER_LEFT_LID_LOWERED );

		p.m_flLeftLowerLidRaised = GetDlgItemFloat( IDC_LOWER_LEFT_LID_RAISED );
		p.m_flLeftLowerLidNeutral = GetDlgItemFloat( IDC_LOWER_LEFT_LID_NEUTRAL );
		p.m_flLeftLowerLidLowered = GetDlgItemFloat( IDC_LOWER_LEFT_LID_LOWERED );
	}
	else
	{
		// Left lids follow the right lids.
		p.m_flLeftUpperLidRaised = p.m_flRightUpperLidRaised;
		p.m_flLeftUpperLidNeutral = p.m_flRightUpperLidNeutral;
		p.m_flLeftUpperLidLowered = p.m_flRightUpperLidLowered;

		p.m_flLeftLowerLidRaised = p.m_flRightLowerLidRaised;
		p.m_flLeftLowerLidNeutral = p.m_flRightLowerLidNeutral;
		p.m_flLeftLowerLidLowered = p.m_flRightLowerLidLowered;
	}

	// Figure out the eyeball prefix.
	if ( IsOptionChecked( IDC_EYE_COLOR_LIGHT ) )
		strcpy( p.m_EyeballPrefix, "eyeball" );
	else
		strcpy( p.m_EyeballPrefix, "dark_eyeball" );

	// Figure out the pupil prefix.
	if ( IsOptionChecked( IDC_IRIS_COLOR_BROWN ) )
		strcpy( p.m_PupilPrefix, "pupil" );
	else if ( IsOptionChecked( IDC_IRIS_COLOR_GREEN ) )
		strcpy( p.m_PupilPrefix, "grn_pupil" );
	else
		strcpy( p.m_PupilPrefix, "bl_pupil" );
}


void CQC_EyesDlg::GenerateQCText()
{
	CDialogParams p;
	GetDialogParams( p );
	
	
	m_BufSize = 16 * 1024;
	m_Buf = new char[m_BufSize];
	m_Buf[0] = 0;

	AddText( "//start eye/face data\n" );
	AddText( "$eyeposition 0 0 70\n\n" );

	AddText( "//head controllers\n" );
	AddText( "$attachment \"eyes\" \"ValveBiped.Bip01_Head1\" %.2f %.2f %.2f absolute\n", 
		p.m_flLeftEye[0] - ((fabs( p.m_flRightEye[0] ) + p.m_flLeftEye[0]) * 0.5), 
		(p.m_flLeftEye[1] + p.m_flRightEye[1]) * 0.5, 
		(p.m_flLeftEye[2] + p.m_flRightEye[2]) * 0.5 );
	
	AddText( "$attachment \"mouth\" \"ValveBiped.Bip01_Head1\" 0.80 -5.80 -0.15 rotate 0 -80 -90\n\n" );

	AddText( "$model %s \"%s.smd\" {\n", 
		p.m_ModelFilename, p.m_ReferenceFilename );
	
	AddText( "\teyeball righteye \"ValveBiped.Bip01_Head1\" %.2f %.2f %.2f \"%s_r\" %.2f 4 \"%s_r\" %.2f\n",
		p.m_flRightEye[0], 
		p.m_flRightEye[1], 
		p.m_flRightEye[2], 
		p.m_EyeballPrefix, 
		p.m_flEyeballSize,
		p.m_PupilPrefix, 
		p.m_flIrisSize );
	
	AddText( "\teyeball lefteye \"ValveBiped.Bip01_Head1\" %.2f %.2f %.2f \"%s_l\" %.2f -4 \"%s_l\" %.2f\n\n",
		p.m_flLeftEye[0], 
		p.m_flLeftEye[1], 
		p.m_flLeftEye[2], 
		p.m_EyeballPrefix, 
		p.m_flEyeballSize,
		p.m_PupilPrefix, 
		p.m_flIrisSize );

	AddText( "\teyelid  upper_right \"%s\" lowerer 1 %.2f neutral 0 %.2f raiser 2 %.2f split 0.1 eyeball righteye\n",
		p.m_ExpressionsFilename, 
		p.m_flRightUpperLidLowered - p.m_flRightEye[2], 
		p.m_flRightUpperLidNeutral - p.m_flRightEye[2], 
		p.m_flRightUpperLidRaised - p.m_flRightEye[2] );

	AddText( "\teyelid  lower_right \"%s\" lowerer 3 %.2f neutral 0 %.2f raiser 4 %.2f split 0.1 eyeball righteye\n",
		p.m_ExpressionsFilename, 
		p.m_flRightLowerLidLowered - p.m_flRightEye[2], 
		p.m_flRightLowerLidNeutral - p.m_flRightEye[2], 
		p.m_flRightLowerLidRaised - p.m_flRightEye[2] );

	AddText( "\teyelid  upper_left \"%s\" lowerer 1 %.2f neutral 0 %.2f raiser 2 %.2f split -0.1 eyeball lefteye\n",
		p.m_ExpressionsFilename, 
		p.m_flLeftUpperLidLowered - p.m_flLeftEye[2], 
		p.m_flLeftUpperLidNeutral - p.m_flLeftEye[2], 
		p.m_flLeftUpperLidRaised - p.m_flLeftEye[2] );

	AddText( "\teyelid  lower_left \"%s\" lowerer 3 %.2f neutral 0 %.2f raiser 4 %.2f split -0.1 eyeball lefteye\n\n",
		p.m_ExpressionsFilename, 
		p.m_flLeftLowerLidLowered - p.m_flLeftEye[2], 
		p.m_flLeftLowerLidNeutral - p.m_flLeftEye[2], 
		p.m_flLeftLowerLidRaised - p.m_flLeftEye[2] );

	AddText( "\tmouth 0 \"mouth\" \"ValveBiped.Bip01_Head1\" 0 1 0     // mouth illumination\n" );
	AddText( "\tflexfile \"%s\" {\n", p.m_ExpressionsFilename );
	AddText( "\t\t$include \"../standardflex_xsi.qci\"\n" );
	AddText( "\t}\n" );
	AddText( "\t$include \"../facerules_xsi.qci\"\n" );
	AddText( "\t$include \"../bodyrules_xsi.qci\"\n" );
	AddText( "}\n" );
	AddText( "//end eye/face data\n" );
}


bool CQC_EyesDlg::CheckNumericInputs()
{
	struct
	{
		const char *pControlName;
		UINT controlID;
	} 
	controls[] =
	{
		{"Right Eye X", IDC_RIGHT_EYE_X},
		{"Right Eye Y", IDC_RIGHT_EYE_Y},
		{"Right Eye Z", IDC_RIGHT_EYE_Z},
		
		{"Left Eye X", IDC_LEFT_EYE_X},
		{"Left Eye Y", IDC_LEFT_EYE_Y},
		{"Left Eye Z", IDC_LEFT_EYE_Z},

		{"Upper Lid Raised", IDC_UPPER_LID_RAISED},
		{"Upper Lid Neutral", IDC_UPPER_LID_NEUTRAL},
		{"Upper Lid Lowered", IDC_UPPER_LID_LOWERED},

		{"Lower Lid Raised", IDC_LOWER_LID_RAISED},
		{"Lower Lid Neutral", IDC_LOWER_LID_NEUTRAL},
		{"Lower Lid Lowered", IDC_LOWER_LID_LOWERED},

		{"Upper Left Lid Raised", IDC_UPPER_LEFT_LID_RAISED},
		{"Upper Left Lid Neutral", IDC_UPPER_LEFT_LID_NEUTRAL},
		{"Upper Left Lid Lowered", IDC_UPPER_LEFT_LID_LOWERED},

		{"Lower Left Lid Raised", IDC_LOWER_LEFT_LID_RAISED},
		{"Lower Left Lid Neutral", IDC_LOWER_LEFT_LID_NEUTRAL},
		{"Lower Left Lid Lowered", IDC_LOWER_LEFT_LID_LOWERED},

		{"Iris Size", IDC_IRIS_SIZE},
		{"Eyeball Size", IDC_EYEBALL_SIZE}
	};

	for ( int i=0; i < sizeof( controls ) / sizeof( controls[0] ); i++ )
	{
		char text[512];
		GetDlgItem( controls[i].controlID )->GetWindowText( text, sizeof( text ) );

		for ( int z=0; z < (int)strlen( text ); z++ )
		{
			if ( text[z] < '0' || text[z] > '9' )
			{
				if ( text[z] != '.' && text[z] != '-' )
				{
					char errMsg[512];
					_snprintf( errMsg, sizeof( errMsg ), "The '%s' control must have a numeric value.", controls[i].pControlName );
					AfxMessageBox( errMsg, MB_OK );
					return false;
				}
			}
		}
	}

	return true;
}


void CQC_EyesDlg::OnCreateQcText() 
{
	if ( !CheckNumericInputs() )
		return;
	
	GenerateQCText();

	// Clear the edit control.
	LRESULT nLen = ::SendMessage( m_hOutputText, EM_GETLIMITTEXT, 0, 0 );
	::SendMessage( m_hOutputText, EM_SETSEL, 0, nLen );
	::SendMessage( m_hOutputText, EM_REPLACESEL, FALSE, (LPARAM)"" );

	FormatAndSendToEditControl( m_hOutputText, m_Buf );

	delete [] m_Buf;
}

void CQC_EyesDlg::OnIrisColorBrown() 
{
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_IRIS_COLOR_BROWN ), BM_SETCHECK, BST_CHECKED, 0 );
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_IRIS_COLOR_GREEN ), BM_SETCHECK, BST_UNCHECKED, 0 );
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_IRIS_COLOR_BLUE ), BM_SETCHECK, BST_UNCHECKED, 0 );
	SetupBitmapLabel( IDB_EYE_DEFAULT, "" );
}

void CQC_EyesDlg::OnIrisColorGreen() 
{
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_IRIS_COLOR_BROWN ), BM_SETCHECK, BST_UNCHECKED, 0 );
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_IRIS_COLOR_GREEN ), BM_SETCHECK, BST_CHECKED, 0 );
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_IRIS_COLOR_BLUE ), BM_SETCHECK, BST_UNCHECKED, 0 );
	SetupBitmapLabel( IDB_EYE_DEFAULT, "" );
}

void CQC_EyesDlg::OnIrisColorBlue() 
{
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_IRIS_COLOR_BROWN ), BM_SETCHECK, BST_UNCHECKED, 0 );
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_IRIS_COLOR_GREEN ), BM_SETCHECK, BST_UNCHECKED, 0 );
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_IRIS_COLOR_BLUE ), BM_SETCHECK, BST_CHECKED, 0 );
	SetupBitmapLabel( IDB_EYE_DEFAULT, "" );
}

void CQC_EyesDlg::OnEyeColorDark() 
{
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_EYE_COLOR_LIGHT ), BM_SETCHECK, BST_UNCHECKED, 0 );
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_EYE_COLOR_DARK ), BM_SETCHECK, BST_CHECKED, 0 );
	SetupBitmapLabel( IDB_EYE_DEFAULT, "" );
}

void CQC_EyesDlg::OnEyeColorLight() 
{
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_EYE_COLOR_LIGHT ), BM_SETCHECK, BST_CHECKED, 0 );
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_EYE_COLOR_DARK ), BM_SETCHECK, BST_UNCHECKED, 0 );
	SetupBitmapLabel( IDB_EYE_DEFAULT, "" );
}

void CQC_EyesDlg::SetupBitmapLabel( UINT iBitmapResourceID, const char *pString, ... )
{
	char msg[4096];
	va_list marker;
	va_start( marker, pString );
	_vsnprintf( msg, sizeof( msg ), pString, marker );
	msg[ ARRAYSIZE(msg) - 1 ] = 0;
	va_end( marker );

	m_PictureControl.SetBitmap( GetCachedBitmap( iBitmapResourceID ) );
	GetDlgItem( IDC_PICTURE_LABEL )->SetWindowText( msg );
}

void CQC_EyesDlg::OnSetfocusRightEyeX() 
{
	SetupBitmapLabel( IDB_EYE_XY_R, "Enter the X position of the center vertex of the right eye" );
}

void CQC_EyesDlg::OnSetfocusRightEyeY() 
{
	SetupBitmapLabel( IsOptionChecked( IDC_Y_AXIS_UP ) ? IDB_EYE_XY_R : IDB_EYE_Z_R, "Enter the Y position of the center vertex of the right eye" );
}

void CQC_EyesDlg::OnSetfocusRightEyeZ() 
{
	SetupBitmapLabel( IsOptionChecked( IDC_Y_AXIS_UP ) ? IDB_EYE_Z_R : IDB_EYE_XY_R, "Enter the Z position of the center vertex of the right eye" );
}

void CQC_EyesDlg::OnSetfocusLeftEyeX() 
{
	SetupBitmapLabel( IDB_EYE_XY_L, "Enter the X position of the center vertex of the right eye" );
}

void CQC_EyesDlg::OnSetfocusLeftEyeY() 
{
	SetupBitmapLabel( IsOptionChecked( IDC_Y_AXIS_UP ) ? IDB_EYE_XY_L : IDB_EYE_Z_L, "Enter the Y position of the center vertex of the right eye" );
}

void CQC_EyesDlg::OnSetfocusLeftEyeZ() 
{
	SetupBitmapLabel( IsOptionChecked( IDC_Y_AXIS_UP ) ? IDB_EYE_Z_L : IDB_EYE_XY_L, "Enter the Z position of the center vertex of the right eye" );
}

void CQC_EyesDlg::OnSetfocusUpperLidLowered() 
{
	const char *pCoord = IsOptionChecked( IDC_Y_AXIS_UP ) ? "Y" : "Z";
	SetupBitmapLabel( IDB_EYE_UPPER_LO, "At Frame 1, enter the %s position of the center vertex of the right upper eye lid", pCoord );
}

void CQC_EyesDlg::OnSetfocusUpperLidNeutral() 
{
	const char *pCoord = IsOptionChecked( IDC_Y_AXIS_UP ) ? "Y" : "Z";
	SetupBitmapLabel( IDB_EYE_UPPER_MID, "At Frame 0, enter the %s position of the center vertex of the right upper eye lid", pCoord );
}

void CQC_EyesDlg::OnSetfocusUpperLidRaised() 
{
	const char *pCoord = IsOptionChecked( IDC_Y_AXIS_UP ) ? "Y" : "Z";
	SetupBitmapLabel( IDB_EYE_UPPER_HI, "At Frame 2, enter the %s position of the center vertex of the right upper eye lid", pCoord );
}


void CQC_EyesDlg::OnSetfocusLowerLidLowered() 
{
	const char *pCoord = IsOptionChecked( IDC_Y_AXIS_UP ) ? "Y" : "Z";
	SetupBitmapLabel( IDB_EYE_LOWER_LO, "At Frame 3, enter the %s position of the center vertex of the right lower eye lid", pCoord );
}

void CQC_EyesDlg::OnSetfocusLowerLidNeutral() 
{
	const char *pCoord = IsOptionChecked( IDC_Y_AXIS_UP ) ? "Y" : "Z";
	SetupBitmapLabel( IDB_EYE_LOWER_MID, "At Frame 0, enter the %s position of the center vertex of the right lower eye lid", pCoord );
}

void CQC_EyesDlg::OnSetfocusLowerLidRaised() 
{
	const char *pCoord = IsOptionChecked( IDC_Y_AXIS_UP ) ? "Y" : "Z";
	SetupBitmapLabel( IDB_EYE_LOWER_HI, "At Frame 4, enter the %s position of the center vertex of the right lower eye lid", pCoord );
}

void CQC_EyesDlg::OnCopyTextToClipboard() 
{
	if ( !CheckNumericInputs() )
		return;

	GenerateQCText();

	if ( !OpenClipboard() )
		return;
	
	size_t textLen = strlen( m_Buf );
	HANDLE hmem = GlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE, textLen + 1 );
	if ( hmem )
	{
		void *ptr = GlobalLock( hmem );
		if ( ptr )
		{
			memcpy( ptr, m_Buf, textLen+1 );
			GlobalUnlock( hmem );

			SetClipboardData( CF_TEXT, hmem );
		}
	}
	
	CloseClipboard();

	delete [] m_Buf;
}


int g_AdvancedControls[] =
{
	IDC_EYE_DETAIL_CONTROL_FRAME,
	IDC_IRIS_SIZE,
	IDC_IRIS_SIZE_LABEL,
	IDC_EYEBALL_SIZE,
	IDC_EYEBALL_SIZE_LABEL,
	IDC_LEFT_LID_CONTROL
};
#define NUM_ADVANCED_CONTROLS	( sizeof( g_AdvancedControls ) / sizeof( g_AdvancedControls[0] ) )

int g_LeftLidPositionControls[] =
{
	IDC_UPPER_LEFT_LID_PANEL,
	IDC_UPPER_LEFT_LID_RAISED,
	IDC_UPPER_LEFT_LID_RAISED_LABEL,
	IDC_UPPER_LEFT_LID_NEUTRAL,
	IDC_UPPER_LEFT_LID_NEUTRAL_LABEL,
	IDC_UPPER_LEFT_LID_LOWERED,
	IDC_UPPER_LEFT_LID_LOWERED_LABEL,
	IDC_LOWER_LEFT_LID_PANEL,
	IDC_LOWER_LEFT_LID_RAISED,
	IDC_LOWER_LEFT_LID_RAISED_LABEL,
	IDC_LOWER_LEFT_LID_NEUTRAL,
	IDC_LOWER_LEFT_LID_NEUTRAL_LABEL,
	IDC_LOWER_LEFT_LID_LOWERED,
	IDC_LOWER_LEFT_LID_LOWERED_LABEL
};
#define NUM_LEFT_LID_POSITION_CONTROLS	( sizeof( g_LeftLidPositionControls ) / sizeof( g_LeftLidPositionControls[0] ) )


void CQC_EyesDlg::OnDefaultControls() 
{
	GetDlgItem( IDC_PICTURES )->ShowWindow( SW_SHOW );

	// Hide all the advanced controls.
	for ( int i=0; i < NUM_ADVANCED_CONTROLS; i++ )
	{
		GetDlgItem( g_AdvancedControls[i] )->ShowWindow( SW_HIDE );
	}

	for ( int i=0; i < NUM_LEFT_LID_POSITION_CONTROLS; i++ )
	{
		GetDlgItem( g_LeftLidPositionControls[i] )->ShowWindow( SW_HIDE );
	}
	
}

void CQC_EyesDlg::OnAdvancedControls() 
{
	GetDlgItem( IDC_PICTURES )->ShowWindow( SW_HIDE );

	// Show the advanced controls.
	for ( int i=0; i < NUM_ADVANCED_CONTROLS; i++ )
	{
		GetDlgItem( g_AdvancedControls[i] )->ShowWindow( SW_SHOW );
		GetDlgItem( g_AdvancedControls[i] )->InvalidateRect( NULL );
	}

	if ( IsIndependentLeftLidControlEnabled() )
	{
		OnLeftLidControl();
	}
}


bool CQC_EyesDlg::IsIndependentLeftLidControlEnabled()
{
	return m_IndependentLeftLidControl.GetCheck() == 1;
}

void CQC_EyesDlg::OnLeftLidControl() 
{
	if ( IsIndependentLeftLidControlEnabled() )
	{
		for ( int i=0; i < NUM_LEFT_LID_POSITION_CONTROLS; i++ )
		{
			GetDlgItem( g_LeftLidPositionControls[i] )->ShowWindow( SW_SHOW );
			GetDlgItem( g_LeftLidPositionControls[i] )->InvalidateRect( NULL );
		}
	}
	else
	{
		for ( int i=0; i < NUM_LEFT_LID_POSITION_CONTROLS; i++ )
		{
			GetDlgItem( g_LeftLidPositionControls[i] )->ShowWindow( SW_HIDE );
			GetDlgItem( g_LeftLidPositionControls[i] )->InvalidateRect( NULL );
		}
	}	
}
