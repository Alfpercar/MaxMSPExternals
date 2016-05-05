// PDImulti.h : main header file for the PDIDUAL application
//

#if !defined(AFX_PDIMULTI_H__3F595478_1D6F_4BCA_A336_BDABC00DE753__INCLUDED_)
#define AFX_PDIMULTI_H__3F595478_1D6F_4BCA_A336_BDABC00DE753__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPDImultiApp:
// See PDImulti.cpp for the implementation of this class
//

class CPDImultiApp : public CWinApp
{
public:
	CPDImultiApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPDImultiApp)
	public:
	virtual BOOL InitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CPDImultiApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PDIMULTI_H__3F595478_1D6F_4BCA_A336_BDABC00DE753__INCLUDED_)
