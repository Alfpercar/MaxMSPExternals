// PDImultiDoc.h : interface of the CPDImultiDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_PDIMULTIDOC_H__1D0D2CDB_266E_4F37_8CA1_0FD9A787CB3C__INCLUDED_)
#define AFX_PDIMULTIDOC_H__1D0D2CDB_266E_4F37_8CA1_0FD9A787CB3C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PDI.h"

class CPDImultiDoc : public CDocument
{
protected: // create from serialization only
	CPDImultiDoc();
	DECLARE_DYNCREATE(CPDImultiDoc)

// Attributes
public:
	CPDIdev		m_pdiDev;
    CPDImdat    m_pdiMDat;
	PBYTE		m_pLastBuf;

// Operations
public:

	BOOL	IsReady	( VOID ) { return m_bCnxReady; }
	BOOL	IsCont  ( VOID ) { return m_bCont; }
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPDImultiDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual void OnCloseDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	VOID PeekCont( VOID );
	VOID Disconnect( VOID );
	VOID ToggleConnect( VOID );
	VOID ParseFrame( PBYTE pBuf, DWORD dwSize );
	VOID AddResultMsg( CString & csCmd );
	VOID ToggleCont( VOID );
	VOID SetupDevice( VOID );
	VOID AddMsg( CString & csMsg );
	VOID Connect( VOID );
	virtual ~CPDImultiDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

	BOOL	m_bCnxReady;
	BOOL	m_bCont;

// Generated message map functions
protected:
	//{{AFX_MSG(CPDImultiDoc)
	afx_msg void OnDevCont();
	afx_msg void OnDevSingle();
	afx_msg void OnDevConnect();
	afx_msg void OnUpdateDevCont(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDevSingle(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDevConnect(CCmdUI* pCmdUI);
	afx_msg void OnDevResetfc();
	afx_msg void OnUpdateDevResetfc(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PDIMULTIDOC_H__1D0D2CDB_266E_4F37_8CA1_0FD9A787CB3C__INCLUDED_)
