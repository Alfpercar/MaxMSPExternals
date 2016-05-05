// DlgConfig.cpp : implementation file
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
// CDlgConfig dialog


CDlgConfig::CDlgConfig(CMMSampleDlg* pParent )
	: CDialog(CDlgConfig::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgConfig)
	m_bESP = FALSE;
	m_bSta1Bore = FALSE;
	m_bSta1En = FALSE;
	m_bSta2Bore = FALSE;
	m_bSta2En = FALSE;
	m_nFilter = -1;
	m_nSta1OD = -1;
	m_nSta2OD = -1;
	//}}AFX_DATA_INIT

	m_pMM = &(pParent->m_MM);
}

void CDlgConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgConfig)
	DDX_Check(pDX, IDC_CHK_ESP, m_bESP);
	DDX_Check(pDX, IDC_CHK_STA1_BORE, m_bSta1Bore);
	DDX_Check(pDX, IDC_CHK_STA1_EN, m_bSta1En);
	DDX_Check(pDX, IDC_CHK_STA2_BORE, m_bSta2Bore);
	DDX_Check(pDX, IDC_CHK_STA2_EN, m_bSta2En);
	DDX_Radio(pDX, IDC_RAD_FILT0, m_nFilter);
	DDX_Radio(pDX, IDC_RAD_STA1_OD0, m_nSta1OD);
	DDX_Radio(pDX, IDC_RAD_STA2_OD0, m_nSta2OD);
	//}}AFX_DATA_MAP
	DDX_Check(pDX, IDC_CHK_STA1_BORE, m_bBore[0]);
	DDX_Check(pDX, IDC_CHK_STA1_EN, m_bStaEn[0]);
	DDX_Check(pDX, IDC_CHK_STA2_BORE, m_bBore[1]);
	DDX_Check(pDX, IDC_CHK_STA2_EN, m_bStaEn[1]);
	DDX_Radio(pDX, IDC_RAD_STA1_OD0, m_nStaOD[0]);
	DDX_Radio(pDX, IDC_RAD_STA2_OD0, m_nStaOD[1]);
}


BEGIN_MESSAGE_MAP(CDlgConfig, CDialog)
	//{{AFX_MSG_MAP(CDlgConfig)
	ON_BN_CLICKED(IDC_BTN_RESTORE, OnBtnRestore)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgConfig message handlers

void CDlgConfig::OnBtnRestore() 
{
	QueryTracker();
	UpdateData(FALSE);
}

void CDlgConfig::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void CDlgConfig::OnOK() 
{
	UpdateData( TRUE );

	ConfigureTracker();
	
	CDialog::OnOK();
}

BOOL CDlgConfig::OnInitDialog() 
{

	CDialog::OnInitDialog();
	
	//QueryTracker();

	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

VOID CDlgConfig::QueryTracker(VOID)
{
	eMMFilterLevel  eF;
	DWORD			dwMap=0;

	m_pMM->GetESP( m_bESP );
	((CMMSampleDlg *)m_pParentWnd)->AddResultMsg( CString("GetESP"));

	m_pMM->GetFilterLevel( eF );
	((CMMSampleDlg *)m_pParentWnd)->AddResultMsg(CString("GetFilterLevel"));
	m_nFilter = eF;

	m_pMM->GetStationMap( dwMap );
	((CMMSampleDlg *)m_pParentWnd)->AddResultMsg(CString("GetStationMap"));

	for (INT i=0; i<MM_MAX_SENSORS; i++)
	{
		CString msg;

		m_bStaEn[i] = ((dwMap & (1<<i)) != 0);

		m_pMM->GetSBoresightState( i+1, m_bBore[i] );
		msg.Format( "Station %d GetSBoresightState", i+1);
		((CMMSampleDlg *)m_pParentWnd)->AddResultMsg(msg);

		m_pMM->GetSDataList( i+1, ((CMMSampleDlg *)m_pParentWnd)->m_MDat[i] );
		msg.Format( "Station %d GetSDataList", i+1 );
		((CMMSampleDlg *)m_pParentWnd)->AddResultMsg(msg);

		if (((CMMSampleDlg *)m_pParentWnd)->m_MDat[i].NumItems())
		{
			switch (((CMMSampleDlg *)m_pParentWnd)->m_MDat[i].ItemAt( 0 ))
			{
			case PDI_MODATA_ORI:
				m_nStaOD[i] = 0;
				break;
			case PDI_MODATA_QTRN:
				m_nStaOD[i] = 1;
				break;
			case PDI_MODATA_DIRCOS:
				m_nStaOD[i] = 2;
				break;
			}
		}
		else
		{
			m_nStaOD[i] = -1;
		}
	}
}

VOID CDlgConfig::ConfigureTracker(VOID)
{
	eMMFilterLevel  eF = (eMMFilterLevel)m_nFilter;
	DWORD			dwMap=0;

	m_pMM->EnableESP( m_bESP );
	((CMMSampleDlg *)m_pParentWnd)->AddResultMsg( CString("EnableESP"));

	m_pMM->SetFilterLevel( eF );
	((CMMSampleDlg *)m_pParentWnd)->AddResultMsg(CString("SetFilterLevel"));


	for (INT i=0; i<MM_MAX_SENSORS; i++)
	{
		if (m_bStaEn[i])
		{
			dwMap |= (1<<i);
		}

		CString msg;

		m_pMM->EnableSBoresight( i+1, m_bBore[i] );
		msg.Format( "Station %d EnableSBoresight result: %s", i+1, m_pMM->GetLastResultStr() );
		((CMMSampleDlg *)m_pParentWnd)->AddResultMsg(msg);


		((CMMSampleDlg *)m_pParentWnd)->m_MDat[i].Empty();

		switch (m_nStaOD[i])
		{
		case 0:
			((CMMSampleDlg *)m_pParentWnd)->m_MDat[i].Append( PDI_MODATA_ORI );
			break;
		case 1:
			((CMMSampleDlg *)m_pParentWnd)->m_MDat[i].Append( PDI_MODATA_QTRN );
			break;
		case 2:
			((CMMSampleDlg *)m_pParentWnd)->m_MDat[i].Append( PDI_MODATA_DIRCOS );
			break;
		default:
			break;
		}

		if (((CMMSampleDlg *)m_pParentWnd)->m_MDat[i].NumItems())
		{
			m_pMM->SetSDataList( i+1, ((CMMSampleDlg *)m_pParentWnd)->m_MDat[i] );
			msg.Format( "Station %d SetSDataList", i+1 );
			((CMMSampleDlg *)m_pParentWnd)->AddResultMsg(msg);
		}
	}

	m_pMM->SetStationMap( dwMap );
	((CMMSampleDlg *)m_pParentWnd)->AddResultMsg(CString("SetStationMap"));
}
