// LATUS sample.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "LATUS sample.h"
#include "LATUS sampleDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLATUSsampleApp

BEGIN_MESSAGE_MAP(CLATUSsampleApp, CWinApp)
	//{{AFX_MSG_MAP(CLATUSsampleApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLATUSsampleApp construction

CLATUSsampleApp::CLATUSsampleApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CLATUSsampleApp object

CLATUSsampleApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CLATUSsampleApp initialization

BOOL CLATUSsampleApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CLATUSsampleDlg * pDlg = new CLATUSsampleDlg();
	m_pMainWnd = pDlg;

	if (pDlg)
	{
		pDlg->Create(IDD_LATUSSAMPLE_DIALOG);
		pDlg->ShowWindow(SW_SHOW);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CLATUSsampleApp::OnIdle(LONG lCount) 
{
	((CLATUSsampleDlg *)m_pMainWnd)->PeekCont();

	CWinApp::OnIdle( lCount );

	return TRUE;
}
