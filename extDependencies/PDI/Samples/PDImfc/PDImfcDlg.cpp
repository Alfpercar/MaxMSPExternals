// PDImfcDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PDImfc.h"
#include "PDImfcDlg.h"

#define _USING_TIMER

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CAboutDlg;

#define BUFFER_SIZE 0x1FA400   // 30 seconds of xyzaer+fc 8 sensors at 240 hz 
//BYTE	g_pMotionBuf[0x0800];  // 2K worth of data.  == 73 frames of XYZAER
BYTE	g_pMotionBuf[BUFFER_SIZE]; 

/////////////////////////////////////////////////////////////////////////////
// CPDImfcDlg dialog

CPDImfcDlg::CPDImfcDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPDImfcDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPDImfcDlg)
	m_bCnxReady = FALSE;
	m_bCont = FALSE;
	m_csFCount = _T("");
	m_bStyPno = FALSE;
	m_bCapture = FALSE;
	m_nStylus = 1;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_dwStationMap = 0;

    m_pdiMDat.Empty();
	m_pdiMDat.Append( PDI_MODATA_FRAMECOUNT );
    m_pdiMDat.Append( PDI_MODATA_POS );
    m_pdiMDat.Append( PDI_MODATA_ORI );
	m_dwFrameSize = 8+4+12+12;

    m_nTextSize = 0;
	m_nTextLimit = 0x2000;
	m_dwContFrameCount = 0;
	m_dwFramesDisplayed = 0;
	m_dwOverflowCount = 0;

	CPDIfilter f;
	f.Disable();

	m_pdiDev.SetPnoBuffer( g_pMotionBuf, BUFFER_SIZE );

	m_pdiSer.SetPort( 2 );

}

void CPDImfcDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPDImfcDlg)
	DDX_Control(pDX, IDC_BTN_PNO, m_btnSinglePno);
	DDX_Control(pDX, IDC_CHK_CAPTURE, m_chkCapture);
	DDX_Control(pDX, IDC_EDT_STYLUSSTA, m_edtStylus);
	DDX_Control(pDX, IDC_CHK_STYPNO, m_chkStyPno);
	DDX_Control(pDX, IDC_CHK_CONT, m_chkCont);
	DDX_Control(pDX, IDC_CHK_CONNECT, m_chkConnect);
	DDX_Control(pDX, IDC_EDT_OUTPUT, m_edtOutput);
	DDX_Check(pDX, IDC_CHK_CONNECT, m_bCnxReady);
	DDX_Check(pDX, IDC_CHK_CONT, m_bCont);
	DDX_Text(pDX, IDC_EDT_FCOUNT, m_csFCount);
	DDX_Check(pDX, IDC_CHK_STYPNO, m_bStyPno);
	DDX_Check(pDX, IDC_CHK_CAPTURE, m_bCapture);
	DDX_Text(pDX, IDC_EDT_STYLUSSTA, m_nStylus);
	DDV_MinMaxInt(pDX, m_nStylus, 1, 8);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPDImfcDlg, CDialog)
	//{{AFX_MSG_MAP(CPDImfcDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_PNO, OnBtnPno)
	ON_BN_CLICKED(IDC_CHK_CONT, OnChkCont)
	ON_BN_CLICKED(IDC_CHK_CONNECT, OnChkConnect)
	ON_MESSAGE(WM_PI_RAWDATA_READY, OnRawDataReady )
	ON_MESSAGE(WM_PI_DATA_STARTED, OnDataStarted )
	ON_MESSAGE(WM_PI_DATA_STOPPED, OnDataStopped )
	ON_MESSAGE(WM_PI_RAWDATA_ERROR, OnDataError )
	ON_MESSAGE(WM_PI_RAWDATA_WARNING, OnDataWarning )
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_WM_CANCELMODE()
	ON_BN_CLICKED(IDC_CHK_STYPNO, OnChkStypno)
	ON_BN_CLICKED(IDC_CHK_CAPTURE, OnChkCapture)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPDImfcDlg message handlers

BOOL CPDImfcDlg::OnInitDialog()
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
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CPDImfcDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CPDImfcDlg::OnPaint() 
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
HCURSOR CPDImfcDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CPDImfcDlg::OnChkConnect() 
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

void CPDImfcDlg::OnBtnPno() 
{
    CString msg;
    m_pdiDev.ReadSinglePno( m_hWnd );
    AddResultMsg( _T("ReadSinglePno") );
}

void CPDImfcDlg::OnChkCont() 
{
	ToggleCont();

    UpdateData( FALSE );	
}

LRESULT	
CPDImfcDlg::OnRawDataReady( WPARAM wP, LPARAM lP )
{
	m_dwContFrameCount++;
	m_dwContSize += (DWORD)wP;

	m_csFCount.Format(_T("%d"), m_dwContFrameCount);
	UpdateData(FALSE);

	if ((PBYTE)lP == g_pMotionBuf)
	{
		CString msg;
		msg.Format(_T("Wrapping at frame %d\r\n"), m_dwContFrameCount );
		AddMsg(msg);
	}

	if (m_bCapture)
	{
	}

	else if ( ( (m_bCont) && ((m_dwContFrameCount % 3) == 0) ) ||
			  (m_bStyPno) ||
			  (!m_bCont)  )
	{
		// display only every 3rd frame if continuous.. 
		ParseFrame( (PBYTE)lP, (DWORD)wP );

		//TRACE("CPDImfcDlg::OnRawDataReady parsing frame %d, dwSize = %d, pBuf = %x\r\n", m_dwContFrameCount, wP, lP);
		//TRACE("CPDImfcDlg::OnRawDataReady parsing %dth frame %d\r\n", m_dwContFrameCount, (m_dwContFrameCount % 3));
	}


    return 0;
}

VOID
CPDImfcDlg::OnTimer( UINT nIDEvent )
{
	TRACE1("OnTimer: nIDEvent = %x\n", nIDEvent );

	/*PBYTE	pBuf;
	DWORD	dwSize;*/

	StopCapture();
}


VOID
CPDImfcDlg::ParseFrame( PBYTE pBuf, DWORD dwSize, BOOL bToFile/*=FALSE*/ )
{
	CString	msg;
    INT		nItemCount = m_pdiMDat.NumItems();
    DWORD	i=0;
	BOOL	bFile = bToFile;
	CFileDialog fdlg( TRUE, CString("txt") );
	CFile	file;

	if (bToFile)
	{
		if (IDOK == fdlg.DoModal())
		{
			bFile = file.Open(fdlg.GetPathName(), CFile::modeCreate|CFile::modeReadWrite);
		}
		else
		{
			bFile = FALSE;
		}

		if (!bFile)
			return;
	}

    while ( i<dwSize)
    {
	    BYTE ucSensor = pBuf[i+2];

        // skip rest of header
        i += 8;

        FLOAT x, y, z;
        FLOAT az, el, ro;
		DWORD fc;

        for (INT j=0; j<nItemCount; j++)
        {
            switch(m_pdiMDat.ItemAt(j))
            {
            case PDI_MODATA_POS:
                x = *((PFLOAT)&pBuf[i]);
                i += sizeof(FLOAT);
                y = *((PFLOAT)&pBuf[i]);
                i += sizeof(FLOAT);
                z = *((PFLOAT)&pBuf[i]);
                i += sizeof(FLOAT);
                break;
            case PDI_MODATA_ORI:
                az = *((PFLOAT)&pBuf[i]);
                i += sizeof(FLOAT);
                el = *((PFLOAT)&pBuf[i]);
                i += sizeof(FLOAT);
                ro = *((PFLOAT)&pBuf[i]);
                i += sizeof(FLOAT);
                break;
			case PDI_MODATA_FRAMECOUNT:
				fc = *((DWORD*)&pBuf[i]);
				i += sizeof(DWORD);
				break;
            default:
                break;
            }
        }
        msg.Format( _T("Sensor %d: %d %f %f %f %f %f %f\r\n"), ucSensor, fc, x, y, z, az, el, ro );
		if (bFile)
		{
			file.Write( (PVOID)(LPCTSTR)msg, msg.GetLength() );
		}
		else
		{
			AddMsg(msg);
		}
    }
	m_dwFramesDisplayed++;

	if (bFile)
	{
		file.Close();

		if (::AfxMessageBox(_T("View File?"), MB_YESNO) == IDYES)
		{
			CString str;
			str.Format(_T("notepad.exe %s"), (LPCTSTR)fdlg.GetPathName() );
			((CPDImfcApp*)::AfxGetApp)->LaunchApp( str );
		}
	}
}

LRESULT	
CPDImfcDlg::OnDataStarted( WPARAM wP, LPARAM lP )
{
	TRACE0("CPDImfcDlg::OnDataStarted\n");
    CString     msg;
	msg.Format(_T("WM_PI_DATA_STARTED\r\n"));
	AddMsg(msg);

	//m_bCont = TRUE;
	m_dwContFrameCount = 0;
	m_dwFramesDisplayed = 0;
	m_dwOverflowCount = 0;
	m_dwContSize = 0;

	if (m_bCapture)
	{
		StartCaptureTimer();
	}

	m_timeStart = CTime::GetCurrentTime();

	EnableButtons();

	UpdateData(FALSE);

    return 0;
}

VOID
CPDImfcDlg::StartCaptureTimer( VOID )
{
	m_uIDTimer = ::SetTimer( m_hWnd, 1, 10000, 0 );
	TRACE1("CPDImfcDlg::StartCaptureTimer: %x\r\n", m_uIDTimer );
	if (!m_uIDTimer)
	{
		CString msg;
		msg.Format(_T("Timer error!\r\n"));
		AddMsg( msg );
		StopCont();
	}
}

VOID
CPDImfcDlg::StopCaptureTimer( VOID )
{
	if (m_uIDTimer)
	{
		if (!(::KillTimer( m_hWnd, m_uIDTimer )))
		{
			CString msg;
			msg.Format(_T("KillTimer error!\r\n"));
			AddMsg( msg );
		}
	}
}

LRESULT	
CPDImfcDlg::OnDataStopped( WPARAM wP, LPARAM lP )
{
	TRACE0("CPDImfcDlg::OnDataStopped\n");
    CString     msg;

	CTimeSpan  ts = m_timeStop - m_timeStart;
	CString    s = ts.Format("%M:%S");
	msg.Format(_T("WM_PI_DATA_STOPPED: time elapsed %s,  %d frames collected, %d bytes, %d frames dropped, %d frames displayed\r\n"), 
				(LPCTSTR)s, m_dwContFrameCount, m_dwContSize, m_dwOverflowCount, m_dwFramesDisplayed);
	AddMsg(msg);

	if (m_bCapture)
	{
		ParseFrame( g_pMotionBuf, m_dwContSize, TRUE );
	}

	if (m_bStyPno)
	{
		ClearStyMode();
	}

	m_bCont = FALSE;
	m_bStyPno = FALSE;
	m_bCapture = FALSE;

	EnableButtons();
	
	UpdateData(FALSE);

    return 0;
}

LRESULT	
CPDImfcDlg::OnDataError( WPARAM wP, LPARAM lP )
{
    CString     msg;
	msg.Format(_T("WM_PI_RAWDATA_ERROR received: %s\r\n"), 
		m_pdiDev.ResultStr( (ePiErrCode)wP, (ePiDevError)lP ));
	AddMsg(msg);

	if ( ((IS_PI_SYS_ERROR(wP)) || IS_PI_DEVICE_ERROR(wP)) && m_bCont)
	{
		ToggleCont();
	}

	if ((ePiErrCode)wP == PI_HOST_OVERFLOW_ERROR)
	{
		m_dwOverflowCount++;
	}

    return 0;
}

LRESULT	
CPDImfcDlg::OnDataWarning( WPARAM wP, LPARAM lP )
{
    CString     msg;
	msg.Format(_T("WM_PI_RAWDATA_WARNING received: %s\r\n"), 
		m_pdiDev.ResultStr( (ePiErrCode)wP, (ePiDevError)lP ));
	AddMsg(msg);

    return 0;
}

VOID CPDImfcDlg::Connect()
{
    CString msg;
    if (!(m_pdiDev.CnxReady()))
    {
		m_pdiDev.SetSerialIF( &m_pdiSer );

        ePiCommType eType = m_pdiDev.DiscoverCnx();
		switch (eType)
		{
		case PI_CNX_USB:
			msg.Format(_T("USB Connection: %s\r\n"), m_pdiDev.GetLastResultStr() );
			break;
		case PI_CNX_SERIAL:
			msg.Format(_T("Serial Connection: %s\r\n"), m_pdiDev.GetLastResultStr() );
			break;
		default:
			msg.Format(_T("DiscoverCnx result: %s\r\n"), m_pdiDev.GetLastResultStr() );
			break;
		}
        m_bCnxReady = m_pdiDev.CnxReady();
		AddMsg( msg );

        if (m_bCnxReady)
        {
            SetupDevice();
        }
    }
    else
    {
        msg.Format(_T("Already connected\r\n"));
        m_bCnxReady = TRUE;
	    AddMsg( msg );
    }
}

VOID CPDImfcDlg::Disconnect()
{
    CString msg;
    if (!(m_pdiDev.CnxReady()))
    {
        msg.Format(_T("Already disconnected\r\n"));
        m_bCnxReady = FALSE;
    }
    else
    {
        m_pdiDev.Disconnect();
        m_bCnxReady = m_pdiDev.CnxReady();
        msg.Format(_T("Disconnect result: %s\r\n"), m_pdiDev.GetLastResultStr() );
    }
    AddMsg( msg );
}

VOID CPDImfcDlg::SetupDevice( VOID )
{
	CString msg;

	m_pdiDev.SetSDataList( -1, m_pdiMDat );
	AddResultMsg(_T("SetSDataList"));

	CPDIbiterr cBE;
	m_pdiDev.GetBITErrs( cBE );
	AddResultMsg(_T("GetBITErrs"));

	TCHAR sz[200];
	cBE.Parse( sz, 200 );
	msg.Format(_T("BIT Errors: %s\r\n"), sz );
	AddMsg(msg);

	if (!(cBE.IsClear()))
	{
		m_pdiDev.ClearBITErrs();
		AddResultMsg(_T("ClearBITErrs"));
	}

	UpdateStationMap();
}

VOID
CPDImfcDlg::UpdateStationMap()
{
	m_pdiDev.GetStationMap( m_dwStationMap );
	AddResultMsg(_T("GetStationMap"));

	CString msg;
	msg.Format(_T("ActiveStationMap: %#x\r\n"), m_dwStationMap );
	AddMsg(msg);
}

VOID CPDImfcDlg::ToggleCont( VOID )
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

VOID
CPDImfcDlg::EnableButtons(VOID)
{
	m_chkCapture.EnableWindow( !m_bCont && !m_bStyPno );
	m_chkCont.EnableWindow( !m_bCapture );
	m_chkStyPno.EnableWindow( !m_bCapture );
	m_btnSinglePno.EnableWindow( !m_bCont && !m_bStyPno && !m_bCapture );
}

VOID CPDImfcDlg::StartCont( VOID )
{
    CString msg;

	if(m_pdiDev.StartContPno( m_hWnd ))
	{
		m_bCont = TRUE;
	}
	else
	{
		m_bCont = FALSE;
	}
	AddResultMsg(_T("StartContPno"));

	UpdateData(FALSE);

}

VOID CPDImfcDlg::StopCont( VOID )
{
	CString msg;

	m_timeStop = CTime::GetCurrentTime();


	if (m_pdiDev.StopContPno())
	{
		m_bCont = FALSE;
	}
	AddResultMsg(_T("StopContPno"));
	UpdateData(FALSE);
}

VOID CPDImfcDlg::ToggleStyPno( VOID )
{

	if (!m_bStyPno)
	{
		StartStyPno();
	}
	else
	{
		StopStyPno();
	}
}

VOID CPDImfcDlg::StartStyPno( VOID )
{
    CString msg;

	if (!m_bCont)
	{
		UpdateData(TRUE);
		UpdateStationMap();

		if ((( 1 << (m_nStylus-1) ) & (m_dwStationMap) ) == 0)
		{
			msg.Format(_T("Stylus Station %d is not an active Station."), m_nStylus);
			::AfxMessageBox( msg, MB_OK|MB_ICONSTOP );
			m_bStyPno = FALSE;
			return;
		}

		ClearStyMode();

		m_pdiDev.SetSStylusMode( m_nStylus, PI_STYMODE_POINT );
		msg.Format(_T("SetStylusMode %d PI_STYMODE_POINT"), m_nStylus );
		AddResultMsg(msg);
	}

	if (m_pdiDev.StartStylusPno( m_hWnd ))
	{
		m_bStyPno = TRUE;
	}
	else
	{
		m_bStyPno = FALSE;
	}
	AddResultMsg(_T("StartStylusPno"));

	UpdateData(FALSE);
}

VOID CPDImfcDlg::StopStyPno( VOID )
{
	CString msg;

	m_timeStop = CTime::GetCurrentTime();

	if (m_pdiDev.StopStylusPno())
	{
		m_bStyPno = FALSE;
	}
	AddResultMsg(_T("StopStylusPno"));

	UpdateData(FALSE);
}

VOID
CPDImfcDlg::ClearStyMode( VOID )
{
	m_pdiDev.SetSStylusMode( -1, PI_STYMODE_MOUSE );
	AddResultMsg(_T("SetSStylusMode -1 PI_STYMODE_MOUSE"));
}

VOID CPDImfcDlg::ToggleCapture( VOID )
{

	if (!m_bCapture)
	{
		StartCapture();
	}
	else
	{
		StopCapture();
	}
}

VOID CPDImfcDlg::StartCapture( VOID )
{
    CString msg;

	m_pdiDev.SetPnoBuffer( g_pMotionBuf, BUFFER_SIZE );

	if (m_pdiDev.StartContPno( m_hWnd ))
	{
		m_bCapture = TRUE;
	}
    AddResultMsg( _T("StartCont") );
}

VOID CPDImfcDlg::StopCapture( VOID )
{
	CString msg;

	m_timeStop = CTime::GetCurrentTime();

	StopCaptureTimer();

	m_pdiDev.StopContPno();

	AddResultMsg( _T("StopCont") );
}


VOID CPDImfcDlg::AddMsg( CString & csMsg )
{
	m_nTextSize += csMsg.GetLength();

	INT		nCount = m_edtOutput.GetLineCount();
	INT		nLastLineIndex = m_edtOutput.LineIndex(nCount-1);

	if (m_nTextSize >= m_nTextLimit)
	{
		//TRACE("Text Limit exceeded, textsize=%x, linecount=%d, llindex=%d\n", m_nTextSize,nCount,nLastLineIndex);
		// chop it in half by removing the first 50%
		m_edtOutput.SetSel(0, m_edtOutput.LineIndex(nCount/2), TRUE);
		m_edtOutput.ReplaceSel(_T("\0"));

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
CPDImfcDlg::AddResultMsg( LPCTSTR szCmd )
{
	CString msg;
	msg.Format(_T("%s result: %s\r\n"), szCmd, m_pdiDev.GetLastResultStr() );
	AddMsg( msg );
}


void CPDImfcDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	PositionDlgItems();	
}

void CPDImfcDlg::OnCancelMode() 
{
	CDialog::OnCancelMode();
	
	PositionDlgItems();
	
}

VOID CPDImfcDlg::PositionDlgItems(VOID)
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
    ocr.bottom = cr.bottom - 105;
    m_edtOutput.SetWindowPos( NULL, 0, 0, ocr.right, ocr.bottom, SWP_NOMOVE );

}

void CPDImfcDlg::OnChkStypno() 
{
	ToggleStyPno();

    UpdateData( FALSE );	
}

void CPDImfcDlg::OnChkCapture() 
{
	ToggleCapture();

	UpdateData( FALSE );
}
