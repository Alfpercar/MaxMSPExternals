// PDImfc.h : main header file for the PDIMFC application
//

#if !defined(AFX_PDIMFC_H__FE3EC2CC_CD58_4C98_BBF4_AB54332F6F70__INCLUDED_)
#define AFX_PDIMFC_H__FE3EC2CC_CD58_4C98_BBF4_AB54332F6F70__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CPDImfcApp:
// See PDImfc.cpp for the implementation of this class
//

class CPDImfcApp : public CWinApp
{
public:
	CPDImfcApp();

	void LaunchApp( CString csCmdLine );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPDImfcApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CPDImfcApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PDIMFC_H__FE3EC2CC_CD58_4C98_BBF4_AB54332F6F70__INCLUDED_)
