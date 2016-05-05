// MMSampleDlg.h : header file
//

#if !defined(AFX_MMSAMPLEDLG_H__747A333E_1E10_48A0_952E_A64E6C4AEEBC__INCLUDED_)
#define AFX_MMSAMPLEDLG_H__747A333E_1E10_48A0_952E_A64E6C4AEEBC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PDI.h"
/////////////////////////////////////////////////////////////////////////////
// CMMSampleDlg dialog

class CMMSampleDlg : public CDialog
{
// Construction
public:
	VOID ParseFrame( PBYTE, DWORD );
	VOID PeekCont( VOID );
	VOID EnableDlgItems( VOID );
	VOID AddMsg( CString & );
	VOID AddResultMsg( CString & );
	CMMSampleDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMMSampleDlg)
	enum { IDD = IDD_MMSAMPLE_DIALOG1 };
	CEdit	m_edtOutput;
	CButton	m_chkCont;
	CButton	m_chkConnect;
	CButton	m_btnSingle;
	CButton	m_btnConfig;
	BOOL	m_bCnx;
	BOOL	m_bCont;
	//}}AFX_DATA

	CPDImm		m_MM;
	CPDImdat	m_MDat[MM_MAX_SENSORS];

    INT		m_nTextSize;
	INT		m_nTextLimit;
	DWORD	m_dwLastHostFrameCount;

	CFont	m_font;

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMMSampleDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL


// Implementation
protected:
	VOID PositionDlgItems( VOID );
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CMMSampleDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBtnConfig();
	afx_msg void OnBtnSingle();
	afx_msg void OnChkConnect();
	afx_msg void OnChkCont();
	virtual void OnCancel();
	afx_msg LRESULT	OnDataStarted		( WPARAM wP, LPARAM lP );
	afx_msg LRESULT	OnDataStopped		( WPARAM wP, LPARAM lP );
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MMSAMPLEDLG_H__747A333E_1E10_48A0_952E_A64E6C4AEEBC__INCLUDED_)
