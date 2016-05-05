// PDImfcDlg.h : header file
//
#if !defined(AFX_PDIMFCDLG_H__CD3628C2_A2A7_4F1D_8B1E_A7CF5343DF29__INCLUDED_)
#define AFX_PDIMFCDLG_H__CD3628C2_A2A7_4F1D_8B1E_A7CF5343DF29__INCLUDED_

#include "PDI.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CPDImfcDlg dialog

class CPDImfcDlg : public CDialog
{
// Construction
public:
	CPDImfcDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CPDImfcDlg)
	enum { IDD = IDD_PDIMFC_DIALOG };
	CButton	m_btnSinglePno;
	CButton	m_chkCapture;
	CEdit	m_edtStylus;
	CButton	m_chkStyPno;
	CButton	m_chkCont;
	CButton	m_chkConnect;
	CEdit	m_edtOutput;
	BOOL	m_bCnxReady;
	BOOL	m_bCont;
	CString	m_csFCount;
	BOOL	m_bStyPno;
	BOOL	m_bCapture;
	int		m_nStylus;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPDImfcDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	CPDIdev     m_pdiDev;
    CPDImdat    m_pdiMDat;
	CPDIser		m_pdiSer;
	DWORD		m_dwFrameSize;

	UINT_PTR m_uIDTimer;

    INT m_nTextSize;
	INT m_nTextLimit;
	DWORD m_dwContFrameCount;
	DWORD m_dwOverflowCount;
	DWORD m_dwFramesDisplayed;
	DWORD m_dwContSize;
	DWORD m_dwStationMap;

	CTime m_timeStart;
	CTime m_timeStop;

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CPDImfcDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBtnPno();
	afx_msg void OnChkCont();
	afx_msg void OnChkConnect();
	afx_msg LRESULT	OnRawDataReady		( WPARAM wP, LPARAM lP );
	afx_msg LRESULT	OnDataStarted		( WPARAM wP, LPARAM lP );
	afx_msg LRESULT	OnDataStopped		( WPARAM wP, LPARAM lP );
	afx_msg LRESULT	OnDataError 		( WPARAM wP, LPARAM lP );
	afx_msg LRESULT	OnDataWarning 		( WPARAM wP, LPARAM lP );
	afx_msg VOID    OnTimer				( UINT nIDEvent );
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnCancelMode();
	afx_msg void OnChkStypno();
	afx_msg void OnChkCapture();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    VOID    AddMsg		( CString & );
	VOID	AddResultMsg( LPCSTR );

    VOID    Connect			( VOID );
    VOID    Disconnect		( VOID );
    VOID    SetupDevice		( VOID );
	VOID	UpdateStationMap( VOID );
	VOID	ClearStyMode	( VOID );

	VOID	ParseFrame		( PBYTE pBuf, DWORD dwSize, BOOL bToFile=FALSE );

    VOID    ToggleCont		( VOID );
    VOID    StartCont		( VOID );
    VOID    StopCont		( VOID );

    VOID    ToggleStyPno	( VOID );
	VOID	StartStyPno		( VOID );
	VOID	StopStyPno		( VOID );

	VOID	ToggleCapture	( VOID );
	VOID	StartCapture	( VOID );
	VOID	StopCapture		( VOID );

	VOID	StartCaptureTimer	( VOID );
	VOID	StopCaptureTimer	( VOID );

	VOID	PositionDlgItems( VOID );
	VOID	EnableButtons	( VOID );

};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PDIMFCDLG_H__CD3628C2_A2A7_4F1D_8B1E_A7CF5343DF29__INCLUDED_)
