// PDIcons.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <iostream>
#include "PDI.h"

using namespace std ;


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
CPDIdev		g_pdiDev;
CPDImdat    g_pdiMDat;
CPDIser		g_pdiSer;
DWORD		g_dwFrameSize;
BOOL		g_bCnxReady;
DWORD		g_dwStationMap;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
typedef enum
{
	CHOICE_CONT
	, CHOICE_SINGLE
	, CHOICE_QUIT

	, CHOICE_NONE = -1
} eMenuChoice;

#define ESC	0x1b

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
BOOL		Initialize			( VOID );
BOOL		Connect				( VOID );
VOID		Disconnect			( VOID );
BOOL		SetupDevice			( VOID );
VOID		UpdateStationMap	( VOID );
VOID		AddMsg				( string & );
VOID		AddResultMsg		( LPCSTR );
VOID		ShowMenu			( VOID );	
eMenuChoice Prompt				( VOID );
BOOL		StartCont			( VOID );
BOOL		StopCont			( VOID );	
VOID		DisplayCont			( VOID );
VOID		DisplaySingle		( VOID );
VOID		DisplayFrame		( PBYTE, DWORD );

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	eMenuChoice eChoice = CHOICE_NONE;

	if (!(Initialize()))
	{
	}

	//Connect To Tracker
	else if (!( Connect()))
	{
	}

	//Configure Tracker
	else if (!( SetupDevice()))
	{
	}

	else
	{
		//Wait for go-ahead for cont mode
		ShowMenu();
		do
		{
			eChoice = Prompt();

			switch (eChoice)
			{
			case CHOICE_CONT:
				//Start Cont Mode
				if (!(StartCont()))
				{
				}
				else
				{
					//Collect/Display Tracker Data until ESC
					DisplayCont();
					StopCont();
				}

				break;

			case CHOICE_SINGLE:
				DisplaySingle();
				break;

			default:
				break;
			}

		} while (eChoice != CHOICE_QUIT);


	}

	if (eChoice == CHOICE_NONE)
	{
		cout << "\n\nPress any key to exit. " << endl;
		getch();
	}

	//Close Tracker Connection
	Disconnect();

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
BOOL Initialize( VOID )
{
	BOOL	bRet = TRUE;

	::SetConsoleTitle( "PDIconsole" );

    g_pdiMDat.Empty();
    g_pdiMDat.Append( PDI_MODATA_POS );
    g_pdiMDat.Append( PDI_MODATA_ORI );
	g_pdiMDat.Append( PDI_MODATA_FRAMECOUNT );
	g_dwFrameSize = 8+12+12+4;

	g_bCnxReady = FALSE;
	g_dwStationMap = 0;

	return bRet;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
BOOL Connect( VOID )
{
    string msg;
    if (!(g_pdiDev.CnxReady()))
    {
		g_pdiDev.SetSerialIF( &g_pdiSer );

        ePiCommType eType = g_pdiDev.DiscoverCnx();
		switch (eType)
		{
		case PI_CNX_USB:
			msg = "USB Connection: " + string( g_pdiDev.GetLastResultStr() ) + "\r\n";
			break;
		case PI_CNX_SERIAL:
			msg = "Serial Connection: " + string( g_pdiDev.GetLastResultStr() ) + "\r\n";
			break;
		default:
			msg = "DiscoverCnx result: " + string( g_pdiDev.GetLastResultStr() ) + "\r\n";
			break;
		}
        g_bCnxReady = g_pdiDev.CnxReady();
		AddMsg( msg );

    }
    else
    {
        msg = "Already connected\r\n";
        g_bCnxReady = TRUE;
	    AddMsg( msg );
    }

	return g_bCnxReady;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
VOID Disconnect()
{
    string msg;
    if (!(g_pdiDev.CnxReady()))
    {
        msg = "Already disconnected\r\n";
    }
    else
    {
        g_pdiDev.Disconnect();
        msg = "Disconnect result: " + string(g_pdiDev.GetLastResultStr()) + "\r\n";
    }
    AddMsg( msg );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
BOOL SetupDevice( VOID )
{
	string msg;

	g_pdiDev.SetSDataList( -1, g_pdiMDat );
	AddResultMsg("SetSDataList");

	CPDIbiterr cBE;
	g_pdiDev.GetBITErrs( cBE );
	AddResultMsg("GetBITErrs");

	CHAR sz[100];
	cBE.Parse( sz, 100 );
	msg = "BIT Errors: " + string(sz) + "\r\n";
	AddMsg(msg);

	if (!(cBE.IsClear()))
	{
		g_pdiDev.ClearBITErrs();
		AddResultMsg("ClearBITErrs");
	}

	UpdateStationMap();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
VOID UpdateStationMap( VOID )
{
	g_pdiDev.GetStationMap( g_dwStationMap );
	AddResultMsg("GetStationMap");

	CHAR szMsg[100];
	sprintf(szMsg, "ActiveStationMap: %#x\r\n", g_dwStationMap );

	AddMsg( string(szMsg) );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
VOID AddMsg( string & csMsg )
{
	cout << csMsg ;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
VOID AddResultMsg( LPCSTR szCmd )
{
	string msg;

	//msg.Format("%s result: %s\r\n", szCmd, m_pdiDev.GetLastResultStr() );
	msg = string(szCmd) + " result: " + string( g_pdiDev.GetLastResultStr() ) + "\r\n";
	AddMsg( msg );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
VOID ShowMenu( VOID )
{

	cout << "\n\nPlease enter select a data command option: \n\n";
	cout << "C or c:  Continuous Motion Data Display\n";
	cout << "P or p:  Single Motion Data Frame Display\n";
	cout << endl;
	cout << "ESC:     Quit\n";
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
eMenuChoice Prompt( VOID )
{
	eMenuChoice eRet = CHOICE_NONE;
	INT			ch;

	do
	{
		cout << "\nCc/Pp/ESC>> ";
		ch = getche();
		ch = toupper( ch );

		switch (ch)
		{
		case 'C':
			eRet = CHOICE_CONT;
			break;
		case 'P':
			eRet = CHOICE_SINGLE;
			break;
		case ESC: // ESC
			eRet = CHOICE_QUIT;
			break;
		default:
			break;
		}
	} while (eRet == CHOICE_NONE);

	return eRet;
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
BOOL StartCont( VOID )
{
	BOOL bRet = FALSE;

	if (!(g_pdiDev.StartContPno(0)))
	{
	}
	else
	{
		bRet = TRUE;
		Sleep(1000);
	}
	AddResultMsg("\nStartContPno");

	return bRet;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
BOOL StopCont( VOID )
{
	BOOL bRet = FALSE;

	if (!(g_pdiDev.StopContPno()))
	{
	}
	else
	{
		bRet = TRUE;
		Sleep(1000);
	}
	AddResultMsg("StopContPno");

	return bRet;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
VOID DisplayCont( VOID )
{
	BOOL bExit = FALSE;

	PBYTE pBuf;
	DWORD dwSize;

	cout << "\nPress any key to start continuous display\r\n";
	cout << "\nPress ESC to stop.\r\n";
	cout << "\nReady? ";
	getche();
	cout << endl;

	do
	{
		if (!(g_pdiDev.LastPnoPtr(pBuf, dwSize)))
		{
			AddResultMsg("LastPnoPtr");
			bExit = TRUE;
		}
		else if ((pBuf == 0) || (dwSize == 0))
		{
		}
		else 
		{
			DisplayFrame( pBuf, dwSize );
		}

		if ( kbhit() && ( getch() == ESC ) ) 
		{
			bExit = TRUE;
		}

	} while (!bExit);

}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
VOID DisplaySingle( VOID )
{
	BOOL bExit = FALSE;

	PBYTE pBuf;
	DWORD dwSize;

	cout << endl;

	if (!(g_pdiDev.ReadSinglePnoBuf(pBuf, dwSize)))
	{
		AddResultMsg("ReadSinglePno");
		bExit = TRUE;
	}
	else if ((pBuf == 0) || (dwSize == 0))
	{
	}
	else 
	{
		DisplayFrame( pBuf, dwSize );
	}
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
VOID DisplayFrame( PBYTE pBuf, DWORD dwSize )
{
	CHAR	szFrame[200];
	INT		i = 0;

    while ( i<dwSize)
    {
	    BYTE ucSensor = pBuf[i+2];
		SHORT shSize = pBuf[i+6];

        // skip rest of header
        i += 8;

		PFLOAT pPno = (PFLOAT)(&pBuf[i]);

		sprintf( szFrame, "%2d   %+011.6f %+011.6f %+011.6f   %+011.6f %+011.6f %+011.6f\r\n", 
			     ucSensor, pPno[0], pPno[1], pPno[2], pPno[3], pPno[4], pPno[5] );

		AddMsg( string( szFrame ) );

		i += shSize;
	}
}

