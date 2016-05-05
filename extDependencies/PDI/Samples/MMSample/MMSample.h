// MMSample.h : main header file for the MMSAMPLE application
//

#if !defined(AFX_MMSAMPLE_H__2A7B7387_1A97_4B05_AEFF_C7E4CACCB8F2__INCLUDED_)
#define AFX_MMSAMPLE_H__2A7B7387_1A97_4B05_AEFF_C7E4CACCB8F2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMMSampleApp:
// See MMSample.cpp for the implementation of this class
//

class CMMSampleApp : public CWinApp
{
public:
	CMMSampleApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMMSampleApp)
	public:
	virtual BOOL InitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMMSampleApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MMSAMPLE_H__2A7B7387_1A97_4B05_AEFF_C7E4CACCB8F2__INCLUDED_)
