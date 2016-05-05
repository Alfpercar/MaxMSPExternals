// PDImultiView.h : interface of the CPDImultiView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_PDIMULTIVIEW_H__B01B842E_2B02_4F4E_A39C_37E335A9B5F9__INCLUDED_)
#define AFX_PDIMULTIVIEW_H__B01B842E_2B02_4F4E_A39C_37E335A9B5F9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CPDImultiView : public CEditView
{
protected: // create from serialization only
	CPDImultiView();
	DECLARE_DYNCREATE(CPDImultiView)

// Attributes
public:
	CPDImultiDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPDImultiView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	VOID GetText( CString & csTxt );
	VOID AddMsg( CString & csMsg );
	virtual ~CPDImultiView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	INT	m_nTextLimit;
	INT	m_nTextSize;

// Generated message map functions
protected:
	//{{AFX_MSG(CPDImultiView)
	afx_msg void OnChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in PDImultiView.cpp
inline CPDImultiDoc* CPDImultiView::GetDocument()
   { return (CPDImultiDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PDIMULTIVIEW_H__B01B842E_2B02_4F4E_A39C_37E335A9B5F9__INCLUDED_)
