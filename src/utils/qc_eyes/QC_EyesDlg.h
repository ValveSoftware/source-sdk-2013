//========= Copyright Valve Corporation, All rights reserved. ============//
// QC_EyesDlg.h : header file
//

#if !defined(AFX_QC_EYESDLG_H__9130E22D_05ED_4851_960C_38D90DA94967__INCLUDED_)
#define AFX_QC_EYESDLG_H__9130E22D_05ED_4851_960C_38D90DA94967__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CDialogParams
{
public:
	float m_flLeftEye[3];
	float m_flRightEye[3];

	float m_flIrisSize;
	float m_flEyeballSize;

	float m_flLeftUpperLidRaised;
	float m_flLeftUpperLidNeutral;
	float m_flLeftUpperLidLowered;

	float m_flLeftLowerLidRaised;
	float m_flLeftLowerLidNeutral;
	float m_flLeftLowerLidLowered;
	
	float m_flRightUpperLidRaised;
	float m_flRightUpperLidNeutral;
	float m_flRightUpperLidLowered;

	float m_flRightLowerLidRaised;
	float m_flRightLowerLidNeutral;
	float m_flRightLowerLidLowered;
	
	char m_ReferenceFilename[1024];
	char m_ExpressionsFilename[1024];
	char m_ModelFilename[1024];

	char m_EyeballPrefix[1024];	// eyeball_ or dark_eyeball_
	char m_PupilPrefix[1024];	// pupil_ or grn_pupil_ or bl_pupil_
};

/////////////////////////////////////////////////////////////////////////////
// CQC_EyesDlg dialog

class CQC_EyesDlg : public CDialog
{
// Construction
public:
	CQC_EyesDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CQC_EyesDlg)
	enum { IDD = IDD_QC_EYES_DIALOG };
	CButton	m_IndependentLeftLidControl;
	CStatic	m_PictureControl;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQC_EyesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	void GenerateQCText();
	void AddText( const char *pFormat, ... );
	bool IsOptionChecked( UINT option );
	float GetDlgItemFloat( UINT id );
	void GetDialogParams( CDialogParams &p );
	void SetupBitmapLabel( UINT iBitmapResourceID, const char *pString, ... );

	HWND m_hOutputText;

	
	// Cached list of bitmaps.
	class CBitmapRef
	{
	public:
		UINT m_iResource;
		HBITMAP m_hBitmap;
		CBitmapRef *m_pNext;
	};
	CBitmapRef *m_pBitmapHead;
	HBITMAP GetCachedBitmap( UINT id );


	size_t m_BufSize;
	char *m_Buf;
	bool IsIndependentLeftLidControlEnabled();

	bool CheckNumericInputs();


	// Generated message map functions
	//{{AFX_MSG(CQC_EyesDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnCreateQcText();
	afx_msg void OnIrisColorBrown();
	afx_msg void OnIrisColorGreen();
	afx_msg void OnIrisColorBlue();
	afx_msg void OnEyeColorDark();
	afx_msg void OnEyeColorLight();
	afx_msg void OnSetfocusRightEyeX();
	afx_msg void OnSetfocusRightEyeY();
	afx_msg void OnSetfocusRightEyeZ();
	afx_msg void OnSetfocusLeftEyeX();
	afx_msg void OnSetfocusLeftEyeY();
	afx_msg void OnSetfocusLeftEyeZ();
	afx_msg void OnSetfocusUpperLidLowered();
	afx_msg void OnSetfocusUpperLidNeutral();
	afx_msg void OnSetfocusUpperLidRaised();
	afx_msg void OnSetfocusLowerLidLowered();
	afx_msg void OnSetfocusLowerLidNeutral();
	afx_msg void OnSetfocusLowerLidRaised();
	afx_msg void OnCopyTextToClipboard();
	afx_msg void OnDefaultControls();
	afx_msg void OnAdvancedControls();
	afx_msg void OnLeftLidControl();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QC_EYESDLG_H__9130E22D_05ED_4851_960C_38D90DA94967__INCLUDED_)
