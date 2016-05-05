// LATUS sampleDlg.cpp : implementation file
//

#include "stdafx.h"
#include "LATUS sample.h"
#include "LATUS sampleDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CAboutDlg;

#define BUFFER_SIZE 0xC6480   // 30 seconds of xyzaer+fc 8 markers at 94 hz 
BYTE	g_pMotionBuf[BUFFER_SIZE]; 

/////////////////////////////////////////////////////////////////////////////
// CLATUSsampleDlg dialog

CLATUSsampleDlg::CLATUSsampleDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLATUSsampleDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLATUSsampleDlg)
	m_bCont = FALSE;
	m_bCnxReady = FALSE;
	m_nPSMarker = 1;
	m_nReceptor = 1;
	m_nULMarker = 1;
	m_bAutoLaunch = FALSE;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    m_pdiMDat.Empty();
	m_pdiMDat.Append( PDI_MODATA_FRAMECOUNT );
    m_pdiMDat.Append( PDI_MODATA_POS );
    m_pdiMDat.Append( PDI_MODATA_ORI );
	m_dwFrameSize = 8+4+12+12;

    m_nTextSize = 0;
	m_nTextLimit = 0x2000;

	m_pdiLatus.SetPnoBuffer( g_pMotionBuf, BUFFER_SIZE );

}

void CLATUSsampleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLATUSsampleDlg)
	DDX_Control(pDX, IDC_STC_ULMARKER, m_stcULMarker);
	DDX_Control(pDX, IDC_STC_PSMARKER, m_stcPSMarker);
	DDX_Control(pDX, IDC_STC_RECEPTOR, m_stcReceptor);
	DDX_Control(pDX, IDC_EDT_ULMARKER, m_edtULMarker);
	DDX_Control(pDX, IDC_EDT_RECEPTOR, m_edtReceptor);
	DDX_Control(pDX, IDC_EDT_PSMARKER, m_edtPSMarker);
	DDX_Control(pDX, IDC_EDT_OUTPUT, m_edtOutput);
	DDX_Control(pDX, IDC_CHK_CONT, m_chkCont);
	DDX_Control(pDX, IDC_CHK_CONNECT, m_chkConnect);
	DDX_Control(pDX, IDC_CHK_AUTOLAUNCH, m_chkAutoLaunch);
	DDX_Control(pDX, IDC_BTN_UNLAUNCH, m_btnUnlaunch);
	DDX_Control(pDX, IDC_BTN_PNO, m_btnSinglePno);
	DDX_Control(pDX, IDC_BTN_PHASESTEP, m_btnPhaseStep);
	DDX_Control(pDX, IDC_BTN_LAUNCH, m_btnLaunch);
	DDX_Check(pDX, IDC_CHK_CONT, m_bCont);
	DDX_Check(pDX, IDC_CHK_CONNECT, m_bCnxReady);
	DDX_Text(pDX, IDC_EDT_PSMARKER, m_nPSMarker);
	DDV_MinMaxInt(pDX, m_nPSMarker, 1, 12);
	DDX_Text(pDX, IDC_EDT_RECEPTOR, m_nReceptor);
	DDV_MinMaxInt(pDX, m_nReceptor, 1, 16);
	DDX_Text(pDX, IDC_EDT_ULMARKER, m_nULMarker);
	DDV_MinMaxInt(pDX, m_nULMarker, 1, 12);
	DDX_Check(pDX, IDC_CHK_AUTOLAUNCH, m_bAutoLaunch);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CLATUSsampleDlg, CDialog)
	//{{AFX_MSG_MAP(CLATUSsampleDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BTN_PNO, OnBtnPno)
	ON_BN_CLICKED(IDC_CHK_CONT, OnChkCont)
	ON_BN_CLICKED(IDC_CHK_CONNECT, OnChkConnect)
	ON_BN_CLICKED(IDC_CHK_AUTOLAUNCH, OnChkAutolaunch)
	ON_BN_CLICKED(IDC_BTN_PHASESTEP, OnBtnPhasestep)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BTN_LAUNCH, OnBtnLaunch)
	ON_BN_CLICKED(IDC_BTN_UNLAUNCH, OnBtnUnlaunch)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLATUSsampleDlg message handlers

BOOL CLATUSsampleDlg::OnInitDialog()
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
	
	// TODO: Add extra initialization here
	/*Connect();
	m_chkConnect.SetCheck( m_bCnxReady );
	m_chkAutoLaunch.SetCheck( m_bAutoLaunch );*/
	EnableDlgItems();
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CLATUSsampleDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CLATUSsampleDlg::OnPaint() 
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
HCURSOR CLATUSsampleDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CLATUSsampleDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	PositionDlgItems();	
}

void CLATUSsampleDlg::OnChkConnect() 
{
	if (m_chkConnect.GetCheck() == 1)
	{
		Connect();
	}
	else
	{
		Disconnect();
	}

	UpdateData( FALSE );
}

void CLATUSsampleDlg::OnBtnPno() 
{
	if (m_bCont)
	{
		ToggleCont();
	}
	else
	{
		PBYTE pBuf = 0;
		DWORD dwSize = 0;
		if (!(m_pdiLatus.ReadSinglePnoBuf( pBuf, dwSize )))
		{
			AddResultMsg(CString("ReadSinglePnoBuf"));
		}
		else 
		{
			ParseFrame( pBuf, dwSize );
		}
	}

}

void CLATUSsampleDlg::OnChkCont() 
{
	ToggleCont();

	UpdateData( FALSE );
}

void CLATUSsampleDlg::OnChkAutolaunch() 
{
	UpdateData(TRUE);

	if (!(m_pdiLatus.SetAutoLaunchMode( m_bAutoLaunch )))
	{
			//ePiErrCode	GetLastResult		( VOID );					// Returns numeric result of last operation
		m_bAutoLaunch = !m_bAutoLaunch;
		UpdateData(FALSE);
	}

	AddResultMsg("SetAutoLaunchMode");
	EnableDlgItems();

}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

VOID CLATUSsampleDlg::Connect()
{
	CString msg;
	if (!(m_pdiLatus.CnxReady()))
	{
		ePiCommType eType = m_pdiLatus.DiscoverCnx();
		switch (eType)
		{
		case PI_CNX_USB:
			msg.Format("USB Connection: %s\r\n", m_pdiLatus.GetLastResultStr() );
			break;
		case PI_CNX_SERIAL:
			msg.Format("Serial Connection: %s\r\n", m_pdiLatus.GetLastResultStr() );
			break;
		default:
			msg.Format("DiscoverCnx result: %s\r\n", m_pdiLatus.GetLastResultStr() );
			break;
		}
		m_bCnxReady = m_pdiLatus.CnxReady();
		AddMsg( msg );

		if (m_bCnxReady)
		{
			SetupDevice();
		}
	}
	else
	{
		msg.Format("Already connected\r\n");
		m_bCnxReady = TRUE;
		AddMsg( msg );
	}
}

VOID CLATUSsampleDlg::Disconnect()
{
    CString msg;
    if (!(m_pdiLatus.CnxReady()))
    {
        msg.Format("Already disconnected\r\n");
        m_bCnxReady = FALSE;
    }
    else
    {
        m_pdiLatus.Disconnect();
        m_bCnxReady = m_pdiLatus.CnxReady();
        msg.Format("Disconnect result: %s\r\n", m_pdiLatus.GetLastResultStr() );
    }
    AddMsg( msg );
}

VOID CLATUSsampleDlg::SetupDevice( VOID )
{
	CString msg;

	// Set output data list
	m_pdiLatus.SetSDataList( -1, m_pdiMDat );
	AddResultMsg("SetSDataList");

	// Read AutoLaunch
	m_pdiLatus.GetAutoLaunchMode( m_bAutoLaunch );
	AddResultMsg("GetAutoLaunchMode");

	// Read WhoAmI
	DoWhoAmI();

	// Read Receptor Map
	DoReceptorMap();

	// Read Active Marker Map
	DoMarkerMap();

	// Read Active Marker IDs
	DoMarkerIDs();
}

VOID CLATUSsampleDlg::DoWhoAmI( VOID )
{
	LPCSTR szWho;

	if (!(m_pdiLatus.WhoAmI( szWho )))
	{
		AddResultMsg("WhoAmI");
	}
	else
	{
		CString msg;
		msg.Format("WhoAmI: %d receptors, %d markers supported.\r\n%s \r\n\r\n", 
					m_pdiLatus.MaxReceptors(), m_pdiLatus.MaxMarkers(), szWho );
		AddMsg(msg);
	}
}

VOID CLATUSsampleDlg::DoMarkerMap( VOID )
{
	DWORD dwMarkerMap;

	m_pdiLatus.GetActiveMarkerMap( dwMarkerMap );
	AddResultMsg("GetActiveMarkerMap");

	CString msg;
	msg.Format("ActiveMarkerMap: %#x\r\n", dwMarkerMap );
	AddMsg(msg);
}

VOID CLATUSsampleDlg::DoMarkerIDs( VOID )
{
	DWORD dwMarkerMap;

	m_pdiLatus.GetActiveMarkerMap( dwMarkerMap );

	if (dwMarkerMap == 0)
	{
		return;
	}

	CString msg;
	msg.Format("Active Markers: ");

	for (INT i=0; i<LATUS_MAX_MARKERS; i++)
	{
		if (dwMarkerMap & (1 << i))
		{
			LPCSTR szID;
			CString msg2;

			if (!(m_pdiLatus.WhoAmIMarker( i+1, szID )))
			{
				msg2.Format("  %d - Error,", i+1);
			}
			else
			{
				msg2.Format("  %d - %s,", i+1, szID );
			}
			msg += msg2;

		}
	}

	msg.TrimRight(',');
	msg += CString("\r\n");
	AddMsg(msg);
}

VOID CLATUSsampleDlg::DoReceptorMap( VOID )
{
	DWORD dwRcpMap;
	DWORD dwRcpAlignMap;

	m_pdiLatus.GetRcpMap( dwRcpMap );
	AddResultMsg("GetRcpMap");

	m_pdiLatus.GetRcpAlignMap( dwRcpAlignMap );
	AddResultMsg("GetRcpAlignMap");

	CString msg;
	msg.Format("DetectedReceptorMap: %#x\r\n", dwRcpMap );
	AddMsg(msg);
	msg.Format("AlignedReceptorMap: %#x\r\n", dwRcpAlignMap );
	AddMsg(msg);

}

VOID CLATUSsampleDlg::ToggleCont( VOID )
{
	if (!m_bCont)
	{
		StartCont();
	}
	else
	{
		StopCont();
	}
}

VOID CLATUSsampleDlg::StartCont( VOID )
{
    CString msg;

	if(m_pdiLatus.StartContPno( 0 ))
	{
		m_bCont = TRUE;
		m_dwLastHostFrameCount = 0;
	}
	else
	{
		m_bCont = FALSE;
	}
	AddResultMsg("StartContPno");

	UpdateData(FALSE);

}

VOID CLATUSsampleDlg::StopCont( VOID )
{
	CString msg;

	if (m_pdiLatus.StopContPno())
	{
		m_bCont = FALSE;
	}
	AddResultMsg("StopContPno");
	UpdateData(FALSE);
}

VOID CLATUSsampleDlg::PeekCont(VOID)
{
	if (!m_bCont)
	{
		return;
	}

	PBYTE pBuf;
	DWORD dwSize;
	DWORD dwFC;

	if (!(m_pdiLatus.LastHostFrameCount( dwFC )))
	{
		AddResultMsg(CString("LastHostFrameCount"));
	}
	else if (dwFC == m_dwLastHostFrameCount)
	{
		// no new frames since last peek
	}
	else if (!(m_pdiLatus.LastPnoPtr( pBuf, dwSize )))
	{
		AddResultMsg(CString("LastPnoPtr"));
	}
	else 
	{
		m_dwLastHostFrameCount = dwFC;
		ParseFrame( pBuf, dwSize );
	}
}

VOID CLATUSsampleDlg::ParseFrame(PBYTE pBuf, DWORD dwSize)
{
	CString	msg;
    INT		nItemCount = m_pdiMDat.NumItems();
    DWORD	i=0;

    while ( i<dwSize)
    {
		BINHDR * pHdr = (BINHDR *)&(pBuf[i]);

        // skip rest of header
        i += sizeof(BINHDR);

		if (pHdr->length)
		{
			FLOAT x, y, z;
			FLOAT az, el, ro;
			DWORD fc, sync, ts;
			CString msg2;

			msg.Format("Marker %d: ", pHdr->station );

			for (INT j=0; j<nItemCount; j++)
			{
				msg2.Empty();

				switch(m_pdiMDat.ItemAt(j))
				{
				case PDI_MODATA_POS:
					x = *((PFLOAT)&pBuf[i]);
					i += sizeof(FLOAT);
					y = *((PFLOAT)&pBuf[i]);
					i += sizeof(FLOAT);
					z = *((PFLOAT)&pBuf[i]);
					i += sizeof(FLOAT);
					msg2.Format( "%f %f %f ", x, y, z );
					break;
				case PDI_MODATA_ORI:
					az = *((PFLOAT)&pBuf[i]);
					i += sizeof(FLOAT);
					el = *((PFLOAT)&pBuf[i]);
					i += sizeof(FLOAT);
					ro = *((PFLOAT)&pBuf[i]);
					i += sizeof(FLOAT);
					msg2.Format( "%f %f %f ", az, el, ro );
					break;
				case PDI_MODATA_TIMESTAMP:
					ts = *((DWORD*)&pBuf[i]);
					i += sizeof(DWORD);
					msg2.Format( "%d ", ts );
					break;
				case PDI_MODATA_FRAMECOUNT:
					fc = *((DWORD*)&pBuf[i]);
					i += sizeof(DWORD);
					msg2.Format( "%d ", fc);
					break;
				case PDI_MODATA_EXTSYNC:
					sync = *((DWORD*)&pBuf[i]);
					i += sizeof(DWORD);
					msg2.Format( "%d ", sync);
					break;
				case PDI_MODATA_CRLF:
					i += 2;
					break;
				case PDI_ODATA_SPACE:
					i++;
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
		AddMsg(msg);
    }
	//m_dwFramesDisplayed++;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

VOID CLATUSsampleDlg::AddMsg( CString & csMsg )
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

VOID
CLATUSsampleDlg::AddResultMsg( LPCSTR szCmd )
{
	CString msg;
	msg.Format("%s result: %s\r\n", szCmd, m_pdiLatus.GetLastResultStr() );
	AddMsg( msg );
}

VOID CLATUSsampleDlg::PositionDlgItems(VOID)
{
    if (!(::IsWindow(m_edtOutput.m_hWnd))) 
    {
        return;
    }

	RECT  cr, ocr;

	GetClientRect( &cr );
	m_edtOutput.GetClientRect( &ocr );

    ocr.right = cr.right - (20);
    ocr.bottom = cr.bottom - 140;
    m_edtOutput.SetWindowPos( NULL, 0, 0, ocr.right, ocr.bottom, SWP_NOMOVE );

}

VOID CLATUSsampleDlg::EnableDlgItems(VOID)
{
    if (!(::IsWindow(m_chkAutoLaunch.m_hWnd))) 
    {
        return;
    }

	m_btnLaunch.EnableWindow( !m_bAutoLaunch );
	m_btnUnlaunch.EnableWindow( !m_bAutoLaunch );
	m_edtReceptor.EnableWindow( !m_bAutoLaunch );
	m_edtULMarker.EnableWindow( !m_bAutoLaunch );
	m_stcReceptor.EnableWindow( !m_bAutoLaunch );
	m_stcULMarker.EnableWindow( !m_bAutoLaunch );
	
	m_btnPhaseStep.EnableWindow( m_bAutoLaunch );
	m_edtPSMarker.EnableWindow( m_bAutoLaunch );
	m_stcPSMarker.EnableWindow( m_bAutoLaunch );
}


void CLATUSsampleDlg::OnOK() 
{
	OnClose();
}

void CLATUSsampleDlg::PostNcDestroy() 
{
	delete this;	
}

void CLATUSsampleDlg::OnBtnPhasestep() 
{
	UpdateData(TRUE);
	
	m_pdiLatus.MarkerPhaseStep( m_nPSMarker );

	CString msg;
	msg.Format("MarkerPhaseStep %d", m_nPSMarker );
	AddResultMsg( msg );
}

void CLATUSsampleDlg::OnClose() 
{
	DestroyWindow();
}

void CLATUSsampleDlg::OnBtnLaunch() 
{
	UpdateData(TRUE);
	m_pdiLatus.MarkerLaunch( m_nReceptor );

	CString msg;
	msg.Format("MarkerLaunch %d", m_nReceptor );
	AddResultMsg( msg );

	DoMarkerMap();
	DoMarkerIDs();
}

void CLATUSsampleDlg::OnBtnUnlaunch() 
{
	UpdateData(TRUE);
	m_pdiLatus.MarkerUnlaunch( m_nULMarker );

	CString msg;
	msg.Format("MarkerUnlaunch %d", m_nULMarker );
	AddResultMsg( msg );

	DoMarkerMap();
	DoMarkerIDs();
	
}
