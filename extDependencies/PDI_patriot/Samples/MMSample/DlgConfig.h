#if !defined(AFX_DLGCONFIG_H__C8BC430D_CDD9_4149_88B9_42FB06E9D7CA__INCLUDED_)
#define AFX_DLGCONFIG_H__C8BC430D_CDD9_4149_88B9_42FB06E9D7CA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgConfig.h : header file
//

class CMMSampleDlg;

/////////////////////////////////////////////////////////////////////////////
// CDlgConfig dialog

class CDlgConfig : public CDialog
{
// Construction
public:
	VOID ConfigureTracker( VOID );
	VOID QueryTracker( VOID );
	CDlgConfig(CMMSampleDlg* pParent);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgConfig)
	enum { IDD = IDD_MMSAMPLE_CONFIG1 };
	BOOL	m_bESP;
	BOOL	m_bSta1Bore;
	BOOL	m_bSta1En;
	BOOL	m_bSta2Bore;
	BOOL	m_bSta2En;
	int		m_nFilter;
	int		m_nSta1OD;
	int		m_nSta2OD;
	//}}AFX_DATA

	CPDImm *	m_pMM;
	BOOL		m_bBore[MM_MAX_SENSORS];
	BOOL		m_bStaEn[MM_MAX_SENSORS];
	INT			m_nStaOD[MM_MAX_SENSORS];


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgConfig)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgConfig)
	afx_msg void OnBtnRestore();
	virtual void OnCancel();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGCONFIG_H__C8BC430D_CDD9_4149_88B9_42FB06E9D7CA__INCLUDED_)
