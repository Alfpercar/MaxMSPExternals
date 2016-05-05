// MMSampleDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MMSample.h"
#include "MMSampleDlg.h"
#include "DlgConfig.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMMSampleDlg dialog

CMMSampleDlg::CMMSampleDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMMSampleDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMMSampleDlg)
	m_bCnx = FALSE;
	m_bCont = FALSE;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    m_nTextSize = 0;
	m_nTextLimit = 0x2000;
}

void CMMSampleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMMSampleDlg)
	DDX_Control(pDX, IDC_EDT_OUTPUT, m_edtOutput);
	DDX_Control(pDX, IDC_CHK_CONT, m_chkCont);
	DDX_Control(pDX, IDC_CHK_CONNECT, m_chkConnect);
	DDX_Control(pDX, IDC_BTN_SINGLE, m_btnSingle);
	DDX_Control(pDX, IDC_BTN_CONFIG, m_btnConfig);
	DDX_Check(pDX, IDC_CHK_CONNECT, m_bCnx);
	DDX_Check(pDX, IDC_CHK_CONT, m_bCont);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMMSampleDlg, CDialog)
	//{{AFX_MSG_MAP(CMMSampleDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CONFIG, OnBtnConfig)
	ON_BN_CLICKED(IDC_BTN_SINGLE, OnBtnSingle)
	ON_BN_CLICKED(IDC_CHK_CONNECT, OnChkConnect)
	ON_BN_CLICKED(IDC_CHK_CONT, OnChkCont)
	ON_MESSAGE(WM_PI_DATA_STARTED, OnDataStarted )
	ON_MESSAGE(WM_PI_DATA_STOPPED, OnDataStopped )
	ON_WM_CLOSE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMMSampleDlg message handlers

BOOL CMMSampleDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	m_font.CreateStockObject(ANSI_FIXED_FONT); //default

	m_edtOutput.SetFont(&m_font);

	EnableDlgItems();
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMMSampleDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMMSampleDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMMSampleDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CMMSampleDlg::OnBtnConfig() 
{
	CDlgConfig dlg(this);

	dlg.QueryTracker();
	
	dlg.DoModal();
}

void CMMSampleDlg::OnBtnSingle() 
{
    PBYTE pBuf;
	DWORD dwSize;

    if (m_MM.ReadSinglePnoBuf( pBuf, dwSize ))
	{
		ParseFrame( pBuf, dwSize );
	}
	else
	{
		AddResultMsg( CString("ReadSinglePno") );
	}	
}

void CMMSampleDlg::OnChkConnect() 
{
    if (m_chkConnect.GetCheck() == 1)
    {
		m_MM.ConnectMM();
		AddResultMsg( CString("ConnectMM") );
    }
    else
    {
		m_MM.Disconnect();
		AddResultMsg( CString("Disconnect") );
    }

	m_bCnx = m_MM.CnxReady();

	if (m_bCnx)
	{
		CString msg;
		for (INT i=0; i<MM_MAX_SENSORS; i++)
		{
			m_MDat[i].Empty();
			m_MDat[i].Append( PDI_MODATA_ORI );
			m_MM.SetSDataList( i+1, m_MDat[i] );
			msg.Format("Station %d SetSDataList: ", i+1 );
			AddResultMsg( msg );
		}
	}

    UpdateData( FALSE );
	
	EnableDlgItems();
}

void CMMSampleDlg::OnChkCont() 
{
    if (m_chkCont.GetCheck() == 1)
    {
		m_bCont = m_MM.StartContPno( m_hWnd );
		AddResultMsg( CString("StartCont") );
		m_dwLastHostFrameCount = 0;
    }
    else
    {
		m_bCont = !(m_MM.StopContPno());
		AddResultMsg( CString("StopCont") );
    }

    UpdateData( FALSE );	
	EnableDlgItems();

}

void CMMSampleDlg::OnCancel() 
{
	OnClose();
}

VOID CMMSampleDlg::AddResultMsg(CString & csPre )
{
	CString msg;
	msg.Format("%s result: %s\r\n", csPre, m_MM.GetLastResultStr() );
	AddMsg( msg );

}

VOID CMMSampleDlg::AddMsg(CString & csMsg)
{
	m_nTextSize += csMsg.GetLength();

	INT		nCount = m_edtOutput.GetLineCount();
	INT		nLastLineIndex = m_edtOutput.LineIndex(nCount-1);

	if (m_nTextSize >= m_nTextLimit)
	{
		//TRACE("Text Limit exceeded, textsize=%x, linecount=%d, llindex=%d\n", m_nTextSize,nCount,nLastLineIndex);
		// chop it in half by removing the first 50%
		m_edtOutput.SetSel(0, m_edtOutput.LineIndex(nCount/2), TRUE);
		m_edtOutput.ReplaceSel("\0");

		nCount = m_edtOutput.GetLineCount();
		nLastLineIndex = m_edtOutput.LineIndex(nCount-1);
		m_nTextSize = nLastLineIndex;
		//TRACE("Chopped: textsize=%x, linecount=%d, llindex=%d\n", m_nTextSize,nCount,nLastLineIndex);
	}

	// position cursor to end. 
	m_edtOutput.SetSel(nLastLineIndex, nLastLineIndex, TRUE);

	m_edtOutput.ReplaceSel( csMsg ); 
	nCount = m_edtOutput.GetLineCount();
	nLastLineIndex = m_edtOutput.LineIndex(nCount-1);

	// position cursor at end again, and scroll
	m_edtOutput.SetSel(nLastLineIndex, nLastLineIndex);

	m_edtOutput.SetModify(FALSE);

}

LRESULT	
CMMSampleDlg::OnDataStarted( WPARAM wP, LPARAM lP )
{
	TRACE("CMMSampleDlg::OnDataStarted\n");
    CString     msg;

	msg.Format("WM_PI_DATA_STARTED\r\n");
	AddMsg(msg);

    return 1;
}

LRESULT	
CMMSampleDlg::OnDataStopped( WPARAM wP, LPARAM lP )
{
	TRACE("CMMSampleDlg::OnDataStopped\n");
    CString     msg;

	msg.Format("WM_PI_DATA_STOPPED\r\n");
	AddMsg(msg);

	m_bCont = FALSE;

	EnableDlgItems();
	
	UpdateData(FALSE);

    return 1;
}

VOID CMMSampleDlg::EnableDlgItems(VOID)
{
	m_btnSingle.EnableWindow( m_bCnx & !m_bCont );
	m_btnConfig.EnableWindow( m_bCnx & !m_bCont );
	m_chkCont.EnableWindow( m_bCnx );
}

VOID CMMSampleDlg::PeekCont(VOID)
{
	if (!m_bCont)
	{
		return;
	}

	PBYTE pBuf;
	DWORD dwSize;
	DWORD dwFC;

	if (!(m_MM.LastHostFrameCount( dwFC )))
	{
		AddResultMsg(CString("LastHostFrameCount"));
	}
	else if (dwFC == m_dwLastHostFrameCount)
	{
		// no new frames since last peek
	}
	else if (!(m_MM.LastPnoPtr( pBuf, dwSize )))
	{
		AddResultMsg(CString("LastPnoPtr"));
	}
	else 
	{
		m_dwLastHostFrameCount = dwFC;
		ParseFrame( pBuf, dwSize );
	}
}

VOID CMMSampleDlg::ParseFrame(PBYTE pBuf, DWORD dwSize)
{
	CString	msg;
    DWORD	i=0;

    while ( i<dwSize)
    {
		BINHDR * pHdr = (BINHDR *)&(pBuf[i]);
		INT		 nSta = pHdr->station;

        // skip rest of header
        i += sizeof(BINHDR);


		if (pHdr->length)
		{
			PDIori		ori;
			PDIqtrn		qtrn;
			PDIdircos	dircos;
			INT			nItemCount;
			CString		msg2;
			INT			nIndex = i;

			msg.Format("Station %d:    ", nSta );
			nItemCount = m_MDat[nSta-1].NumItems();

			for (INT j=0; j<nItemCount; j++)
			{
				msg2.Empty();

				switch(m_MDat[nSta-1].ItemAt(j))
				{
				case PDI_MODATA_ORI:
					memcpy(ori, ((PFLOAT)&pBuf[nIndex]), sizeof(ori));
					nIndex += sizeof(ori);
					msg2.Format( "% 10.3f  % 10.3f  % 10.3f ", ori[0], ori[1], ori[2] );
					break;
				case PDI_MODATA_DIRCOS:
					memcpy(dircos, ((PFLOAT)&pBuf[nIndex]), sizeof(dircos));
					nIndex += sizeof(dircos);
					msg2.Format( "% 10.3f  % 10.3f  % 10.3f \r\n              % 10.3f  % 10.3f  % 10.3f\r\n              % 10.3f  % 10.3f  % 10.3f", 
								dircos[0][0], dircos[0][1], dircos[0][2],
								dircos[1][0], dircos[1][1], dircos[1][2],
								dircos[2][0], dircos[2][1], dircos[2][2] );
					break;
				case PDI_MODATA_QTRN:
					memcpy(qtrn, ((PFLOAT)&pBuf[nIndex]), sizeof(qtrn));
					nIndex += sizeof(qtrn);
					msg2.Format( "% 10.3f  % 10.3f  % 10.3f  % 10.3f", qtrn[0], qtrn[1], qtrn[2], qtrn[3] );
					break;
				default:
					break;
				}

				msg += msg2;
			}
			msg += CString("\r\n");

		}
		else
		{
			msg = CString("Empty Frame\r\n");
		}

		i += pHdr->length;

		AddMsg(msg);
    }
}

void CMMSampleDlg::OnClose() 
{
	DestroyWindow();
}

void CMMSampleDlg::PostNcDestroy() 
{
	delete this;
}

void CMMSampleDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	PositionDlgItems();	
	
}

VOID CMMSampleDlg::PositionDlgItems(VOID)
{
    if (!(::IsWindow(m_edtOutput.m_hWnd))) 
    {
        return;
    }

	RECT  cr, ocr;

	GetClientRect( &cr );
	m_edtOutput.GetClientRect( &ocr );

    // Cmd Edit
    ocr.right = cr.right - (20);
    ocr.bottom = cr.bottom - 67;
    m_edtOutput.SetWindowPos( NULL, 0, 0, ocr.right, ocr.bottom, SWP_NOMOVE );

}
