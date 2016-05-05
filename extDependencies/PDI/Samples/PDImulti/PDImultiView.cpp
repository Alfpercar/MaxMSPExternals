// PDImultiView.cpp : implementation of the CPDImultiView class
//

#include "stdafx.h"
#include "PDImulti.h"

#include "PDImultiDoc.h"
#include "PDImultiView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPDImultiView

IMPLEMENT_DYNCREATE(CPDImultiView, CEditView)

BEGIN_MESSAGE_MAP(CPDImultiView, CEditView)
	//{{AFX_MSG_MAP(CPDImultiView)
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPDImultiView construction/destruction

CPDImultiView::CPDImultiView()
{
	// TODO: add construction code here
	m_nTextLimit = 0x2000;
    m_nTextSize = 0;

}

CPDImultiView::~CPDImultiView()
{
}

BOOL CPDImultiView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CEditView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CPDImultiView drawing

void CPDImultiView::OnDraw(CDC* pDC)
{
	CPDImultiDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CPDImultiView diagnostics

#ifdef _DEBUG
void CPDImultiView::AssertValid() const
{
	CView::AssertValid();
}

void CPDImultiView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CPDImultiDoc* CPDImultiView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CPDImultiDoc)));
	return (CPDImultiDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPDImultiView message handlers

VOID CPDImultiView::AddMsg(CString &csMsg)
{
	CEdit & csEdit         = GetEditCtrl();

	INT		nCount         = csEdit.GetLineCount();
	INT		nLastLineIndex = csEdit.LineIndex(nCount-1);
	
	m_nTextSize += csMsg.GetLength();

	if (m_nTextSize >= m_nTextLimit)
	{
		//TRACE("Text Limit exceeded, textsize=%x, linecount=%d, llindex=%d\n", m_nTextSize,nCount,nLastLineIndex);
		// chop it in half by removing the first 50%
		csEdit.SetSel(0, csEdit.LineIndex(nCount/2), TRUE);
		csEdit.ReplaceSel("\0");

		nCount = csEdit.GetLineCount();
		nLastLineIndex = csEdit.LineIndex(nCount-1);
		m_nTextSize = nLastLineIndex;
	}

	// position cursor to end. 
	csEdit.SetSel(nLastLineIndex, nLastLineIndex, TRUE);

	csEdit.ReplaceSel( csMsg ); 
	nCount = csEdit.GetLineCount();
	nLastLineIndex = csEdit.LineIndex(nCount-1);

	// position cursor at end again, and scroll
	csEdit.SetSel(nLastLineIndex, nLastLineIndex);

	csEdit.SetModify(FALSE);
}

VOID CPDImultiView::GetText(CString &csTxt)
{
	GetWindowText( csTxt );
}

void CPDImultiView::OnChange() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CEditView::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	
}
