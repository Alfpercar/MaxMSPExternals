// PDImfc.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "PDImfc.h"
#include "PDImfcDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPDImfcApp

BEGIN_MESSAGE_MAP(CPDImfcApp, CWinApp)
	//{{AFX_MSG_MAP(CPDImfcApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPDImfcApp construction

CPDImfcApp::CPDImfcApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPDImfcApp object

CPDImfcApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CPDImfcApp initialization

BOOL CPDImfcApp::InitInstance()
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

	CPDImfcDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

void CPDImfcApp::LaunchApp( CString csCmdLine )
{
	BOOL				bRes;
	STARTUPINFO			cStartupInfo;
	PROCESS_INFORMATION cProcInfo;

	memset(&cStartupInfo, 0, sizeof(cStartupInfo));
	cStartupInfo.cb = sizeof(cStartupInfo);

	bRes = ::CreateProcess( NULL,				// app name
							(LPSTR)(LPCSTR)csCmdLine,		// cmd line
							NULL,					// proc attrs
							NULL,					// thread attrs
							FALSE,					// bInheritHandles
							NORMAL_PRIORITY_CLASS,	// dwCreationFlags
							NULL,					// lpEnvironment
							NULL,					// lpCurrentDir
							&cStartupInfo,
							&cProcInfo
							);

	if (!bRes)
	{
		DWORD dwError = ::GetLastError();

		PVOID pstrError;
	    DWORD dwResult;

	    dwResult = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			dwError,
			LANG_NEUTRAL,
			(LPTSTR) &pstrError, 0, NULL);

		if (dwResult == 0)
		{
			pstrError = "FormatMessage() failed!";
		}
		
		CString str;
		str.Format("Failed!  %s\n(Code is %d)", pstrError, dwError);

		if (dwResult != 0)
		{
			::LocalFree(pstrError);
		}
		AfxMessageBox(str);
	}
}
