/////////////////////////////////////////////////////////////////////
// Polhemus Inc.,  www.polhemus.com
// © 2005 Alken, Inc. dba Polhemus, All Rights Reserved
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//
//  Filename:           $Workfile: PDImm.h $
//
//  Project Name:       Polhemus Developer Interface 
//
//  Description:        MinuteMan Device Object
//
//  VSS $Header: /PIDevTools/Inc/PDImm.h 6     4/18/06 3:42p Suzanne $  
//
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
#ifndef _PDIMM_H_
#define _PDIMM_H_

/////////////////////////////////////////////////////////////////////
#include "PDIdev.h"

/////////////////////////////////////////////////////////////////////
class CPDImdat;
class CPiCmdIF;
class CPiError;

/////////////////////////////////////////////////////////////////////
typedef enum
{
	E_MMFILT_NONE
	, E_MMFILT_LIGHT
	, E_MMFILT_MEDIUM
	, E_MMFILT_HEAVY

} eMMFilterLevel;

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
// CLASS CPDImm
/////////////////////////////////////////////////////////////////////
class PDI_API CPDImm : public CPDIdev
{
public:
	CPDImm( VOID);

	/////////////////////////////////////////////////////////////////
	// Supported Connection Methods
	/////////////////////////////////////////////////////////////////
	BOOL		ConnectMM	( VOID );	// Attempts to connect via virtual USB-Com device

	BOOL		Disconnect	( VOID )	// Ends current connection
							{ return CPDIdev::Disconnect(); }

	BOOL		CnxReady	( VOID )	// Returns TRUE if Connection established
							{ return CPDIdev::CnxReady(); }

	/////////////////////////////////////////////////////////////////
	// Supported MinuteMan configuration commands
	/////////////////////////////////////////////////////////////////
	BOOL	GetStationMap	( DWORD & dwMap )				// Gets Active Station Map
							{ return CPDIdev::GetStationMap( dwMap ); }

	BOOL	SetStationMap	( const DWORD & dwMap )			// Sets Active Station Map 
							{ return CPDIdev::SetStationMap( dwMap ); }

	BOOL	GetFilterLevel	( eMMFilterLevel & );

	BOOL	SetFilterLevel	( const eMMFilterLevel & );

	BOOL	EnableESP		( const BOOL & bEnable );	// Enable MinuteMan Electronic Stabilization Process
	
	BOOL	GetESP			( BOOL & bEnable );			// Get MinuteMan ESP state

	BOOL	EnableStation	( INT nStation, BOOL bEnable )		// Enables or disables station 
							{ return CPDIdev::EnableStation( nStation, bEnable ); }

	BOOL	EnableSBoresight( INT nStation, BOOL bEnable );		// Enables or disables boresight on station

	BOOL	GetSBoresightState( INT nStation, BOOL & bEnabled );	// Gets current boresight state on station

	BOOL	GetSDataList	( INT nStation, CPDImdat & mdat  )	// Gets motion data output list per station
							{ return CPDIdev::GetSDataList( nStation, mdat ); }

	BOOL	SetSDataList	( INT nStation, const CPDImdat & mdat  )	// Sets motion data output list per station 
							{ return CPDIdev::SetSDataList( nStation, mdat ); }


	////////////////////////////////////
	// Runtime Methods
	////////////////////////////////////
	// Host P&O Buffer methods
	BOOL	ClearPnoBuffer	( VOID )							// Clears output P&O output buffer
							{ return CPDIdev::ClearPnoBuffer(); }
	BOOL	LastPnoPtr		( PBYTE & pBuf, DWORD & dwSize )	// Returns pointer to last P&O frame collected
							{ return CPDIdev::LastPnoPtr( pBuf, dwSize ); }
	BOOL	ResetPnoPtr		( VOID )							// Resets P&O output pointer to beginning of buffer
							{ return CPDIdev::ResetPnoPtr(); }
	BOOL	SetPnoBuffer	( PBYTE pBuf, DWORD dwSize )		// Sets/Clears output P&O output buffer
							{ return CPDIdev::SetPnoBuffer( pBuf, dwSize ); }
	BOOL	ResetHostFrameCount ( VOID )						// Resets Host P&O frame counter
								{ return CPDIdev::ResetHostFrameCount(); }
	BOOL	LastHostFrameCount  ( DWORD & dwFrameCount )		// Returns index of last P&O frame collected by the host
								{ return CPDIdev::LastHostFrameCount( dwFrameCount ); }

	// P&O Collection methods
	BOOL	ReadSinglePno	( HWND hwnd )							// Reads single P&O frame. : posts data msg to HWND
							{ return CPDIdev::ReadSinglePno( hwnd ); }

	BOOL	ReadSinglePnoBuf( PBYTE & pBuf, DWORD & dwSize )		// Reads single P&O frame. : returns pointer to frame
							{ return CPDIdev::ReadSinglePnoBuf( pBuf, dwSize ); }

	BOOL	StartContPno	( HWND hwnd )							// Starts continuous P&O collection: posts data msg to HWND
							{ return CPDIdev::StartContPno( hwnd ); }

	BOOL	StopContPno		( VOID )								// Stops continuous P&O collection
							{ return CPDIdev::StopContPno(); }

	// Host Prediction 
	BOOL	SetHostPrediction	( const DWORD & dwMap, const INT & nMS )	// Enables host prediction, sets level
								{ return CPDIdev::SetHostPrediction( dwMap, nMS ); }

	// Other supported tracker commands
	BOOL	ResetTracker	( VOID )								// Initializes Tracker, disconnects 
							{ return CPDIdev::ResetTracker(); }

	BOOL	WhoAmI			( LPCSTR & str )						// Issues tracker WhoAmI command; puts result in argument 
							{ return CPDIdev::WhoAmI( str ); }

	BOOL	WhoAmISensor	( INT nSensor,  LPCSTR & str )			// Issues sensor WhoAmI command; puts result in argument 
							{ return CPDIdev::WhoAmISensor( nSensor, str ); }

	////////////////////////////////////
	// Other Methods
	////////////////////////////////////
	ePiErrCode	GetLastResult		( VOID )					// Returns numeric result of last operation
									{ return CPDIdev::GetLastResult();; }
	ePiErrCode	GetLastResult		( ePiDevError & d )			// Returns numeric result of last operation, plus any device error code
									{ return CPDIdev::GetLastResult( d ); }
	LPCTSTR		GetLastResultStr	( VOID )					// Returns const string result of last operation
									{ return CPDIdev::GetLastResultStr(); }
	LPCTSTR		ResultStr			( ePiErrCode eCode, ePiDevError eDevErr )// Returns const string definition of enum result code
									{ return CPDIdev::ResultStr( eCode, eDevErr ); }

	BOOL		GetDetectedStationMap	( DWORD & dwMap )		// Gets Detected Station Map
										{ return CPDIdev::GetDetectedStationMap( dwMap ); }

	////////////////////////////////////
	// Troubleshooting Methods
	////////////////////////////////////
	BOOL	Simulate	( BOOL bSim )		// Puts object in/out of simulation mode, returns status
						{ return CPDIdev::Simulate( bSim ); }
	PVOID	Trace		( BOOL bT, INT n=0 )	// Enables trace output to debug window (DEBUG build only)
						{ return CPDIdev::Trace( bT, n ); }
	VOID	Log			( BOOL bLog )			// Enables tracker I/O logging to file PICMDIF.LOG
						{ CPDIdev::Log( bLog ); }

	////////////////////////////////////
	// Pipe Export Methods
	////////////////////////////////////
	BOOL	StartPipeExport	( LPCTSTR szPipeName=PDI_EXPORT_PIPE_NAME )//Starts raw P&O export on named pipe
							{ return CPDIdev::StartPipeExport( szPipeName ); }
	BOOL	CancelPipeExport( VOID )									// Cancels raw P&O export on named pipe
							{ return CPDIdev::CancelPipeExport(); }

private:
    // private copy constructor can't be used.
	CPDImm( const CPDImm & );
};


/////////////////////////////////////////////////////////////////////
// END $Workfile: PDImm.h $
/////////////////////////////////////////////////////////////////////
#endif // _PDIMM_H_
