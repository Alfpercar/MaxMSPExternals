// PDImultiDoc.cpp : implementation of the CPDImultiDoc class
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
// CPDImultiDoc

IMPLEMENT_DYNCREATE(CPDImultiDoc, CDocument)

BEGIN_MESSAGE_MAP(CPDImultiDoc, CDocument)
	//{{AFX_MSG_MAP(CPDImultiDoc)
	ON_COMMAND(ID_DEV_CONT, OnDevCont)
	ON_COMMAND(ID_DEV_SINGLE, OnDevSingle)
	ON_COMMAND(ID_DEV_CONNECT, OnDevConnect)
	ON_UPDATE_COMMAND_UI(ID_DEV_CONT, OnUpdateDevCont)
	ON_UPDATE_COMMAND_UI(ID_DEV_SINGLE, OnUpdateDevSingle)
	ON_UPDATE_COMMAND_UI(ID_DEV_CONNECT, OnUpdateDevConnect)
	ON_COMMAND(ID_DEV_RESETFC, OnDevResetfc)
	ON_UPDATE_COMMAND_UI(ID_DEV_RESETFC, OnUpdateDevResetfc)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPDImultiDoc construction/destruction

CPDImultiDoc::CPDImultiDoc()
{
	// TODO: add one-time construction code here
	m_bCnxReady = FALSE;
	m_bCont = FALSE;
	m_pLastBuf = 0;
}

CPDImultiDoc::~CPDImultiDoc()
{
}

BOOL CPDImultiDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

    //m_pdiMDat.Empty();
	//m_pdiMDat.Append( PDI_MODATA_FRAMECOUNT );
    //m_pdiMDat.Append( PDI_MODATA_POS );
    //m_pdiMDat.Append( PDI_MODATA_ORI );
	//m_dwFrameSize = 8+4+12+12;

	Connect();
	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CPDImultiDoc serialization

void CPDImultiDoc::Serialize(CArchive& ar)
{
	((CEditView*)m_viewList.GetHead())->SerializeRaw(ar);
/*	CString csText;

	POSITION pos = GetFirstViewPosition();
	if (!pos)
	{
	}
	else
	{
		CPDImultiView* pView = (CPDImultiView*)GetNextView( pos );

		if (ar.IsStoring())
		{
			pView->GetText( csText );

			ar << csText;
		}
		else
		{
			ar >> csText;

			pView->AddMsg( csText );
		}
	}*/
}

/////////////////////////////////////////////////////////////////////////////
// CPDImultiDoc diagnostics

#ifdef _DEBUG
void CPDImultiDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CPDImultiDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPDImultiDoc commands

VOID CPDImultiDoc::Connect(VOID)
{
    CString msg;
    if (!(m_pdiDev.CnxReady()))
    {
        ePiCommType eType = m_pdiDev.DiscoverCnx();
		switch (eType)
		{
		case PI_CNX_USB:
			msg.Format("USB Connection: %s\r\n", m_pdiDev.GetLastResultStr() );
			break;
		case PI_CNX_SERIAL:
			msg.Format("Serial Connection: %s\r\n", m_pdiDev.GetLastResultStr() );
			break;
		default:
			msg.Format("DiscoverCnx result: %s\r\n", m_pdiDev.GetLastResultStr() );
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
        msg.Format("Already connected\r\n");
        m_bCnxReady = TRUE;
	    AddMsg( msg );
    }
}

VOID CPDImultiDoc::AddMsg(CString &csMsg)
{
	((CPDImultiView*)m_viewList.GetHead())->AddMsg(csMsg);
}

VOID CPDImultiDoc::SetupDevice(VOID)
{
	CString msg;
	CPDIbiterr cBE;
	CHAR sz[100];

	switch(m_pdiDev.TrackerType())
	{
	default:
	case PI_TRK_LIBERTY:
	case PI_TRK_PATRIOT:
		m_pdiDev.GetBITErrs( cBE );
		AddResultMsg(CString("GetBITErrs"));

		cBE.Parse( sz, 100 );
		msg.Format("BIT Errors: %s\r\n", sz );
		AddMsg(msg);

		if (!(cBE.IsClear()))
		{
			m_pdiDev.ClearBITErrs();
			AddResultMsg(CString("ClearBITErrs"));
		}
		break;

	case PI_TRK_LATUS:
	case PI_TRK_MINUTEMAN:
		break;
	}

	m_pdiMDat.Empty();

	if ( m_pdiDev.TrackerType() == PI_TRK_MINUTEMAN )
	{
		m_pdiMDat.Append( PDI_MODATA_ORI );
	}
	else
	{
		m_pdiMDat.Append( PDI_MODATA_FRAMECOUNT );
		m_pdiMDat.Append( PDI_MODATA_POS );
		m_pdiMDat.Append( PDI_MODATA_ORI );
	}

	m_pdiDev.SetSDataList( -1, m_pdiMDat );
	AddResultMsg(CString("SetSDataList"));

	LPCSTR szWho;
	if (!(m_pdiDev.WhoAmI( szWho )))
	{
		AddResultMsg( CString("WhoAmI") );
	}
	else
	{
		AddMsg(CString(szWho));
	}
	

	//UpdateStationMap();
}

void CPDImultiDoc::OnDevCont() 
{
	ToggleCont();	
}

VOID CPDImultiDoc::ToggleCont(VOID)
{
	if (!m_bCont)
	{
		if(m_pdiDev.StartContPno( 0 ))
		{
			m_bCont = TRUE;
			//(CPDImulti *)(::AfxGetApp())->RegisterContPeek(this);
		}
		else
		{
			m_bCont = FALSE;
		}
		AddResultMsg(CString("StartContPno"));
	}
	else
	{
		if (m_pdiDev.StopContPno())
		{
			m_bCont = FALSE;
		}
		AddResultMsg(CString("StopContPno"));	
	}
}

VOID CPDImultiDoc::AddResultMsg(CString &csCmd)
{
	CString msg;
	msg.Format("%s result: %s\r\n", (LPCSTR)csCmd, m_pdiDev.GetLastResultStr() );
	AddMsg( msg );

}

void CPDImultiDoc::OnDevSingle() 
{
	if (m_bCont)
	{
		ToggleCont();
	}
	else
	{
		PBYTE pBuf = 0;
		DWORD dwSize = 0;
		if (!(m_pdiDev.ReadSinglePnoBuf( pBuf, dwSize )))
		{
			AddResultMsg(CString("ReadSinglePnoBuf"));
		}
		else 
		{
			ParseFrame( pBuf, dwSize );
		}
	}
}

VOID CPDImultiDoc::ParseFrame(PBYTE pBuf, DWORD dwSize)
{
	CString	msg;
    INT		nItemCount = m_pdiMDat.NumItems();
    DWORD	i=0;

    while ( i<dwSize)
    {
	    BYTE ucSensor = pBuf[i+2];

        // skip rest of header
        i += 8;

        FLOAT x, y, z;
        FLOAT az, el, ro;
		DWORD fc, sync, ts, sty;
		CString msg2;

		msg.Format("Sensor %d: ", ucSensor );

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
			case PDI_MODATA_STYLUS:
				sty = *((DWORD*)&pBuf[i]);
				i += sizeof(DWORD);
				msg2.Format( "%d ", sty);
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

		AddMsg(msg);
    }
	//m_dwFramesDisplayed++;
}

void CPDImultiDoc::OnDevConnect() 
{
	ToggleConnect();
}

void CPDImultiDoc::OnUpdateDevConnect(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( IsReady() );
}

VOID CPDImultiDoc::ToggleConnect(VOID)
{
	if (IsReady())
	{
		Disconnect();
	}
	else
	{
		Connect();
	}
}

VOID CPDImultiDoc::Disconnect(VOID)
{
    CString msg;
    if (!(m_pdiDev.CnxReady()))
    {
        msg.Format("Already disconnected\r\n");
        m_bCnxReady = FALSE;
    }
    else
    {
        m_pdiDev.Disconnect();
        m_bCnxReady = m_pdiDev.CnxReady();
        msg.Format("Disconnect result: %s\r\n", m_pdiDev.GetLastResultStr() );
    }
    AddMsg( msg );
}

VOID CPDImultiDoc::PeekCont(VOID)
{
	if (!m_bCont)
	{
		return;
	}

	PBYTE pBuf;
	DWORD dwSize;

	if (!(m_pdiDev.LastPnoPtr( pBuf, dwSize )))
	{
		AddResultMsg(CString("ReadSinglePnoBuf"));
	}
	else if (pBuf != m_pLastBuf)
	{
		ParseFrame( pBuf, dwSize );
		m_pLastBuf = pBuf;
	}
}

void CPDImultiDoc::OnUpdateDevCont(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( IsReady() );
	pCmdUI->SetCheck( IsCont() );
}

void CPDImultiDoc::OnUpdateDevSingle(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( IsReady() );
}


void CPDImultiDoc::OnDevResetfc() 
{
	if (!(m_pdiDev.ResetFrameCount()))
	{
		AddResultMsg( CString("ResetFrameCount"));
	}	
}

void CPDImultiDoc::OnUpdateDevResetfc(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( IsReady() );
}

void CPDImultiDoc::OnCloseDocument() 
{
	Disconnect();
	
	CDocument::OnCloseDocument();
}
