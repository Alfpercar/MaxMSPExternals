// LATUS sampleDlg.h : header file
//

#if !defined(AFX_LATUSSAMPLEDLG_H__ECB57A05_88E4_43EB_BA2B_56B48F86269B__INCLUDED_)
#define AFX_LATUSSAMPLEDLG_H__ECB57A05_88E4_43EB_BA2B_56B48F86269B__INCLUDED_

#include "PDI.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CLATUSsampleDlg dialog

class CLATUSsampleDlg : public CDialog
{
// Construction
public:
	CLATUSsampleDlg(CWnd* pParent = NULL);	// standard constructor

	VOID PeekCont( VOID );

// Dialog Data
	//{{AFX_DATA(CLATUSsampleDlg)
	enum { IDD = IDD_LATUSSAMPLE_DIALOG };
	CStatic	m_stcULMarker;
	CStatic	m_stcPSMarker;
	CStatic	m_stcReceptor;
	CEdit	m_edtULMarker;
	CEdit	m_edtReceptor;
	CEdit	m_edtPSMarker;
	CEdit	m_edtOutput;
	CButton	m_chkCont;
	CButton	m_chkConnect;
	CButton	m_chkAutoLaunch;
	CButton	m_btnUnlaunch;
	CButton	m_btnSinglePno;
	CButton	m_btnPhaseStep;
	CButton	m_btnLaunch;
	BOOL	m_bCont;
	BOOL	m_bCnxReady;
	int		m_nPSMarker;
	int		m_nReceptor;
	int		m_nULMarker;
	BOOL	m_bAutoLaunch;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLATUSsampleDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	CPDIlatus	m_pdiLatus;
	CPDImdat	m_pdiMDat;
	DWORD		m_dwFrameSize;
	DWORD		m_dwLastHostFrameCount; 

	INT m_nTextSize;
	INT m_nTextLimit;


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CLATUSsampleDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBtnPno();
	afx_msg void OnChkCont();
	afx_msg void OnChkConnect();
	afx_msg void OnChkAutolaunch();
	virtual void OnOK();
	afx_msg void OnBtnPhasestep();
	afx_msg void OnClose();
	afx_msg void OnBtnLaunch();
	afx_msg void OnBtnUnlaunch();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	VOID	AddMsg		( CString & );
	VOID	AddResultMsg( LPCSTR );

	VOID	Connect			( VOID );
	VOID	Disconnect		( VOID );
	VOID	SetupDevice		( VOID );
	VOID	DoWhoAmI		( VOID );
	VOID	DoMarkerMap		( VOID );
	VOID	DoMarkerIDs		( VOID );
	VOID	DoReceptorMap	( VOID );

	VOID	ParseFrame		( PBYTE pBuf, DWORD dwSize );

	VOID	ToggleCont		( VOID );
	VOID	StartCont		( VOID );
	VOID	StopCont		( VOID );

	VOID	PositionDlgItems( VOID );
	VOID	EnableDlgItems	( VOID );

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LATUSSAMPLEDLG_H__ECB57A05_88E4_43EB_BA2B_56B48F86269B__INCLUDED_)
