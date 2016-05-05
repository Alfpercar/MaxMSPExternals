#include "LibertyTracker.hxx"

#include <cassert>
#include <cstddef>

#include "concat/Utilities/Logging.hxx"

#include "juce.h" // for Thread::sleep()

#define USE_3RD_STYLUS 1

// ---------------------------------------------------------------------------------------

int countBits(unsigned int bitSet)
{
	int count = 0;
	for (; bitSet != 0; bitSet >>= 1)
	{
		count += (bitSet & 1);
	}

	return count;
}

LibertyTracker::LibertyTracker()
{
	clientCircularBuffer_ = NULL;
}

LibertyTracker::~LibertyTracker()
{
	delete[] clientCircularBuffer_;
	clientCircularBuffer_ = NULL;
}

void LibertyTracker::connect()
{
	if (libertyTracker_.CnxReady())
	{
		LOG_INFO_N("liberty_tracker", "Tracker already connected.");
		return;
	}

	const int maxRetriesConnect = 4; // sometimes connection fails, try up to 4 times before giving up

	bool successful = false;

	for (int i = 0; i < maxRetriesConnect; ++i)
	{
		int result;
		result = libertyTracker_.DiscoverCnx(); // first tries usb, then all serial ports, then gives up
		if (result < 0)
		{
			Thread::sleep(1000);
		}
		else
		{
			ePiCommType connectionType = (ePiCommType)result;
			switch (connectionType)
			{
				case PI_CNX_USB:
				{
					LOG_INFO_N("liberty_tracker", "Tracker connected via USB port.");
				}
				break;
				case PI_CNX_SERIAL:
				{
					LOG_INFO_N("liberty_tracker", "Tracker connected via serial port.");
				}
				break;
				default: // shouldn't be reached normally
				{
					LOG_INFO_N("liberty_tracker", "No tracker connected via USB or serial port.");
				}
			}

			successful = true;
			break; // done
		}
	}

	if (!successful)
	{
		LOG_INFO_N("liberty_tracker", "No tracker connected via USB or serial port.");
	}
}

#include <string>

void LibertyTracker::configure()
{
	// Configure tracker:
	libertyTracker_.SetBinary(TRUE);				// (required for PDI)
	libertyTracker_.SetFrameRate(PI_FRATE_NORMAL);	// 240 Hz for liberty
	libertyTracker_.SetMetric(TRUE);				// units in centimeters, not inches
	libertyTracker_.SetBufEnabled(TRUE);			// buffer in Liberty device before sending data over USB, 
													// this is independent of SetPnoBuffer() which controls the 
													// buffering in the driver (on the host computer)
													// needed for USB connections, because USB is polling, thus 
													// host needs to buffer to ensure no data is lost between polls
	// (SetExternalSync() is not set here)
	CPDIfilter filter(0.2f, 0.2f, 0.8f, 0.95f);		// adaptive filter parameters: sensitivity (adaption), 
													// low (adaptive 'cutoff'), high (adaptive 'cutoff'), 
													// max transition rate (pole location change rate limit)
	libertyTracker_.SetPosFilter(filter);			// position (x, y, z) filter
	libertyTracker_.SetAttFilter(filter);			// attitude/orientation (az, el, rl) filter
	libertyTracker_.SetEcho(FALSE);					// echo mode off
	libertyTracker_.SetCompState(0);				// disable static metal compensation map algorithm (one of 3 fixed maps)
	PDIori sourceFrame;
	sourceFrame[0] = 0.0f;
	sourceFrame[1] = 0.0f;
	sourceFrame[2] = 0.0f;
	libertyTracker_.SetSourceFrame(sourceFrame);	// non-physical (software) rotation of the source

	// Configure station(s):
	const int ALL_STATIONS = -1;

	CPDImdat stationOutputFormatList;
	ItemData::getOutputFormatList(stationOutputFormatList);
	libertyTracker_.SetSDataList(ALL_STATIONS, stationOutputFormatList); // set output format

	libertyTracker_.SetHostPrediction(ALL_STATIONS, 0);	// host prediction, predicts sensor orientation up to 50 ms ahead 
														// for very latency-critical application (works best with 
														// quaternions instead of euler angles), 0 turns it off

	libertyTracker_.ResetSAlignment(ALL_STATIONS);		// default {(0,0,0), (1,0,0), (0,1,0)}
	libertyTracker_.ClearSBoresight(ALL_STATIONS);		// un-boresight
	PDI3vec startingHemisphereOfOperation;				// forward hemisphere (+X), default
	startingHemisphereOfOperation[0] = 1.0f;
	startingHemisphereOfOperation[1] = 0.0f;
	startingHemisphereOfOperation[2] = 0.0f;
	libertyTracker_.SetSHemisphere(ALL_STATIONS, startingHemisphereOfOperation);
														// while hemisphere tracking (below) overwrites the hemisphere 
														// of operation to (0, 0, 0), SetSHemisphere() is used to set the 
														// starting hemisphere used for continuous tracking/correction of 
														// each sensor's hemisphere of operation
	PDI3vec actualStartingHemisphereOfOperation;
	libertyTracker_.GetSHemisphere(ALL_STATIONS, actualStartingHemisphereOfOperation);
	LOG_INFO_N("liberty_tracker", concat::formatStr("Initial hemisphere of operation: (%.3f, %.3f, %.3f).", actualStartingHemisphereOfOperation[0], actualStartingHemisphereOfOperation[1], actualStartingHemisphereOfOperation[2]));
	libertyTracker_.SetSHemiTrack(ALL_STATIONS);		// enable hemisphere tracking for all stations (overwrites 
														// hemisphere of operation previously set by SetSHemisphere() with [0,0,0], not used in this case)

#if (USE_3RD_STYLUS != 0)
	libertyTracker_.SetSStylusMode(3, PI_STYMODE_MOUSE);
#endif

	libertyTracker_.EnableStation(ALL_STATIONS, true);	// enable all sensors

	// Check for errors:
	CPDIbiterr bitErrors;
	libertyTracker_.GetBITErrs(bitErrors);

	const int errorStringCapacity = 512;
	char errorString[errorStringCapacity];
	bitErrors.Parse(errorString, errorStringCapacity); 

	if (!bitErrors.IsClear())
	{
		LOG_INFO_N("liberty_tracker", "Source BIT errors:");

		// parse lines CR (0x0d) + LF (0x0a) new lines
		std::string tmp = errorString;

		char newline[3];
		newline[0] = 0x0d;
		newline[1] = 0x0a;
		newline[2] = 0x00;

		std::string::size_type b = 0;
		while (1)
		{
			std::string::size_type e = tmp.find(newline, b);
			
			std::string line = tmp.substr(b, e-b);
			if (line.find("\t") == 0)
				line = line.substr(1, line.size() - 1);

			LOG_INFO_N("liberty_tracker", line.c_str());

			if (e == std::string::npos)
			{
				break;
			}
			else
			{
				if (e == tmp.size() - strlen(newline))
					break;
				else
					b = e + strlen(newline);
			}
		}

		// do not call ClearBITErrs() else BIT errors can't be recalled anymore
	}
	else
	{
		LOG_INFO_N("liberty_tracker", concat::formatStr("Source BIT errors: %s", errorString));
	}

	// Find number of active sensors:
	// Note that this value is cached, because once continuous mode has started 
	// the function returns invalid value..
	DWORD stationMap;
	libertyTracker_.GetStationMap(stationMap);
	numEnabledSensors_ = countBits(stationMap);

	// Set up buffering:
	const int bufferSizeFrames = 512; // say 8192 is maximum buffer size at 44100 Hz sample rate is 0.186 seconds with 240 frames per second would require around 45 frames per sensor
	const int bufferSizeItems = getNumEnabledSensors()*bufferSizeFrames;
	const int bufferSizeBytes = bufferSizeItems*sizeof(ItemData);

	delete[] clientCircularBuffer_;
	clientCircularBuffer_ = new BYTE[bufferSizeBytes];

	// Initialize buffer (for debugging purposes):
	ItemData *tmp = (ItemData *)(clientCircularBuffer_);
	for (int i = 0; i < bufferSizeItems; ++i)
	{
		tmp[i].initToZero();
	}

	const int bufferNoLastFrameSizeItems = getNumEnabledSensors()*(bufferSizeFrames - 1);

	libertyTracker_.SetPnoBuffer(clientCircularBuffer_, bufferSizeBytes); // client-supplied circular buffer
	// note while set to bufferFrameSizeBytes, it will actually wrap at bufferNoLastFrameSizeItems items
	// (i.e. the last frame of the allocated buffer is never written to)

	bufferNoLastFrameSizeItems_ = bufferNoLastFrameSizeItems;

	// Dump configuration to log:
	logConfiguration();

	// Store configuration on tracker (so other software can easily re-use the same configuration):
//	BOOL x1 = libertyTracker_.SetCfgLabel("PLUG-IN_001"); // max. 16 char
//	BOOL x2 = libertyTracker_.SaveCfgToSlot(PICFG_SLOT_1); // overwrites current configuration in slot 1
//	LPCTSTR xxx = libertyTracker_.GetLastResultStr();
//	BOOL x3 = libertyTracker_.SetStartupSlot(PICFG_SLOT_1); // use this configuration on next startup/reset
}

void LibertyTracker::logConfiguration()
{
	BOOL binaryMode;
	libertyTracker_.GetBinary(binaryMode);
	if (binaryMode)
		LOG_INFO_N("liberty_tracker", "Mode: Binary.");
	else
		LOG_INFO_N("liberty_tracker", "Mode: ASCII.");

	ePiFrameRate frameRate;
	libertyTracker_.GetFrameRate(frameRate);
	if (frameRate == PI_FRATE_60)
		LOG_INFO_N("liberty_tracker", "Frame rate: 60 Hz.");
	else if (frameRate == PI_FRATE_120)
		LOG_INFO_N("liberty_tracker", "Frame rate: 120 Hz.");
	else if (frameRate == PI_FRATE_240)
		LOG_INFO_N("liberty_tracker", "Frame rate: 240 Hz.");
	else
		LOG_INFO_N("liberty_tracker", "Frame rate: Unknown.");

	BOOL usingMetricUnits;
	libertyTracker_.GetMetric(usingMetricUnits);
	if (usingMetricUnits)
		LOG_INFO_N("liberty_tracker", "Position unit: Centimeters (metric).");
	else
		LOG_INFO_N("liberty_tracker", "Position unit: Inches.");

	BOOL usingUsbBuffering;
	libertyTracker_.GetBufEnabled(usingUsbBuffering);
	if (usingUsbBuffering)
		LOG_INFO_N("liberty_tracker", "USB buffering: Enabled.");
	else
		LOG_INFO_N("liberty_tracker", "USB buffering: Disabled.");

	//BOOL isSyncEnabled;
	//libertyTracker_.GetSyncEnabled(isSyncEnabled);
	//if (isSyncEnabled)
	//	LOG_INFO_N("liberty_tracker", "External sync (in/out): Enabled.");
	//else
	//	LOG_INFO_N("liberty_tracker", "External sync (in/out): Disabled.");

	CPDIfilter posFilter;
	libertyTracker_.GetPosFilter(posFilter);
	LOG_INFO_N("liberty_tracker", concat::formatStr("Pos. filter: (%.3f, %.3f, %.3f, %.3f).", posFilter.m_fSensitivity, posFilter.m_fLowValue, posFilter.m_fHighValue, posFilter.m_fMaxTransRate));

	CPDIfilter attFilter;
	libertyTracker_.GetAttFilter(attFilter);
	LOG_INFO_N("liberty_tracker", concat::formatStr("Orientation filter: (%.3f, %.3f, %.3f, %.3f).", attFilter.m_fSensitivity, attFilter.m_fLowValue, attFilter.m_fHighValue, attFilter.m_fMaxTransRate));

	BOOL usingEcho;
	libertyTracker_.GetEcho(usingEcho);
	if (usingEcho)
		LOG_INFO_N("liberty_tracker", "Echo: Enabled.");
	else
		LOG_INFO_N("liberty_tracker", "Echo: Disabled.");

	INT compMapIdx;
	libertyTracker_.GetCompState(compMapIdx);
	if (compMapIdx == 0)
		LOG_INFO_N("liberty_tracker", "Static metal compensation map: Disabled.");
	else
		LOG_INFO_N("liberty_tracker", concat::formatStr("Static metal compensation map: %d.", compMapIdx));

	PDIori sourceFrame;
	libertyTracker_.GetSourceFrame(sourceFrame);
	LOG_INFO_N("liberty_tracker", concat::formatStr("Software source rotation: (%.3f, %.3f, %.3f).", sourceFrame[0], sourceFrame[1], sourceFrame[2]));

	int numEnabledSensors = getNumEnabledSensors();
	LOG_INFO_N("liberty_tracker", concat::formatStr("Num. enabled sensors: %d.", numEnabledSensors));

	for (int i = 0; i < numEnabledSensors; ++i)
	{
		logSensorConfiguration(1+i);
	}
}

void LibertyTracker::logSensorConfiguration(int index)
{
	// (.. can't get host prediction ..)

	PDI3vec sensorRefOrigin;
	PDI3vec sensorRefPositiveXAxis;
	PDI3vec sensorRefPositiveYAxis;
	libertyTracker_.GetSAlignment(index, sensorRefOrigin, sensorRefPositiveXAxis, sensorRefPositiveYAxis);
	LOG_INFO_N("liberty_tracker", concat::formatStr("Sensor %d ref. origin: (%.3f, %.3f, %.3f).", index, sensorRefOrigin[0], sensorRefOrigin[1], sensorRefOrigin[2]));
	LOG_INFO_N("liberty_tracker", concat::formatStr("Sensor %d ref. x-axis (positive): (%.3f, %.3f, %.3f.)", index, sensorRefPositiveXAxis[0], sensorRefPositiveXAxis[1], sensorRefPositiveXAxis[2]));
	LOG_INFO_N("liberty_tracker", concat::formatStr("Sensor %d ref. y-axis (positive): (%.3f, %.3f, %.3f).", index, sensorRefPositiveYAxis[0], sensorRefPositiveYAxis[1], sensorRefPositiveYAxis[2]));

	PDIori sensorBoresight;
	libertyTracker_.GetSBoresight(index, sensorBoresight);
	LOG_INFO_N("liberty_tracker", concat::formatStr("Sensor %d ref. rotation (boresight): (%.3f, %.3f, %.3f).", index, sensorBoresight[0], sensorBoresight[1], sensorBoresight[2]));

	BOOL isHemisphereTrackingEnabled;
	libertyTracker_.GetSHemiTrack(index, isHemisphereTrackingEnabled);
	if (isHemisphereTrackingEnabled)
	{
		LOG_INFO_N("liberty_tracker", concat::formatStr("Sensor %d hemisphere tracking: Enabled.", index));
	}
	else
	{
		LOG_INFO_N("liberty_tracker", concat::formatStr("Sensor %d hemisphere tracking: Disabled.", index));
		PDI3vec hemisphere;
		libertyTracker_.GetSHemisphere(index, hemisphere);
		LOG_INFO_N("liberty_tracker", concat::formatStr("Sensor %d hemisphere of operation: (%.3f, %.3f, %.3f).", index, hemisphere[0], hemisphere[1], hemisphere[2]));
	}
}

int LibertyTracker::getNumEnabledSensors() const
{
	return numEnabledSensors_;
}

double LibertyTracker::getSampleRate() const
{
	// XXX: ViolinRecordingPlugInEffect relies on this working even if not connected
	return 240.0;
}

void LibertyTracker::startReceivingEvents()
{
	// Log process and thread priorities (for debugging purposes):
	LOG_DEBUG_N("liberty_tracker", concat::formatStr("Thread id: %d.", ::GetCurrentThreadId()));
	LOG_DEBUG_N("liberty_tracker", concat::formatStr("Process base priority: %d.", ::GetPriorityClass(::GetCurrentProcess())));
	LOG_DEBUG_N("liberty_tracker", concat::formatStr("Thread priority: %d.", ::GetThreadPriority(::GetCurrentThread())));

	// Reset frame count to zero and enable external sync i/o on next clock after call:
	libertyTracker_.ResetFrameCount();		// because StartContPno() doesn't reset frame count
	BOOL isSyncEnabled = libertyTracker_.SetSyncEnabled(TRUE);
											// use external sync in/out (in just sets flag in frame, 
											// tracker clock is always free-running)
											// start sending synch out signal from this call
	// -----------------------------------------------------------------------------------
	// POLHEMUS START SENDING EXTERNAL SYNCH SIGNAL ON EACH CLOCK FROM NOW, BUT TRACKER 
	// FRAME COUNT STAYS AT ZERO (UNTIL StartContPno() IS CALLED)
	// -----------------------------------------------------------------------------------

	// Enable continuous data acquisition mode:
	prevQueryEnd_ = ItemDataIterator((ItemData *)clientCircularBuffer_, 0, bufferNoLastFrameSizeItems_);
	libertyTracker_.StartContPno(0); // start continuous mode, buffered (no HWND supplied)
	// -----------------------------------------------------------------------------------
	// TRACKER FRAME COUNT STARTS COUNTING FROM HERE
	// -----------------------------------------------------------------------------------

	// Log if enabling external synch was successful or not:
	if (isSyncEnabled)
		LOG_INFO_N("liberty_tracker", "External sync (in/out): Enabled.");
	else
		LOG_INFO_N("liberty_tracker", "External sync (in/out): Disabled.");
	// Note: GetSyncEnabled() fails if called after StartContPno() and we want the least 
	// amount of cycles wasted between ResetFrameCount()+SetSyncEnabled(TRUE) and 
	// StartContPno(), thats why we use the return of SetSyncEnabled() rather than 
	// GetSyncEnabled() to see if the call was successful.

	// Note: At this point resetting the starting or current hemisphere-of-operation for the 
	// automatic hemisphere tracking is not possible, must disconnect and re-connect.

	//// XXX test:
	//PDI3vec startingHemisphereOfOperation;
	//startingHemisphereOfOperation[0] = 1.0f;
	//startingHemisphereOfOperation[1] = 0.0f;
	//startingHemisphereOfOperation[2] = 0.0f;
	//bool ok = libertyTracker_.SetSHemisphere(-1, startingHemisphereOfOperation);
	//if (ok)
	//	LOG_INFO_N("liberty_tracker", "Post-connect hemisphere setting ok!");
	//else
	//	LOG_INFO_N("liberty_tracker", "[r]Post-connect hemisphere setting failed!");
}

int LibertyTracker::queryFrames(ItemDataIterator &circularIterator)
{
	const int itemsPerFrame = getNumEnabledSensors();

	PBYTE lastPtr; // pointer to first item of last frame (one before end frame)
	DWORD lastFrameSize;

	BOOL ok = libertyTracker_.LastPnoPtr(lastPtr, lastFrameSize);

	if (lastPtr == NULL)
	{
		return 0;
	}

	if (ok != TRUE || lastFrameSize != itemsPerFrame*sizeof(ItemData))
	{
		LOG_ERROR_N("liberty_tracker", "Tracker frames corrupt.");
//		LOG_ERROR_N("liberty_tracker", libertyTracker_.GetLastResultStr()); // XXX: possible unicode issues
		return 0;
	}

	const int distBeginBufferEndQueryItems = (int)((ItemData *)lastPtr - (ItemData *)clientCircularBuffer_) + itemsPerFrame; // last frame is also included
	
	ItemDataIterator queryBegin = prevQueryEnd_;
	ItemDataIterator queryEnd((ItemData *)clientCircularBuffer_, distBeginBufferEndQueryItems, bufferNoLastFrameSizeItems_); // non-wrapped end
	queryEnd.wrap();

	const int numItems = queryEnd - queryBegin; // computes distance with wrapping
	assert((numItems % itemsPerFrame) == 0); // full frames only

	prevQueryEnd_ = queryEnd; // wrapped

	circularIterator = queryBegin;

	return numItems;
}

//void LibertyTracker::clearBuffer()
//{
//	const int itemsPerFrame = getNumEnabledSensors();
//
//	PBYTE lastPtr; // pointer to first item of last frame (one before end frame)
//	DWORD lastFrameSize;
//
//	BOOL ok = libertyTracker_.LastPnoPtr(lastPtr, lastFrameSize);
//
//	if (lastPtr == NULL)
//	{
//		return;
//	}
//
//	if (ok != TRUE || lastFrameSize != itemsPerFrame*sizeof(ItemData))
//	{
//		LOG_ERROR_N("liberty_tracker", "Tracker frames corrupt.");
//		return;
//	}
//
//	const int distBeginBufferEndQueryItems = (int)((ItemData *)lastPtr - (ItemData *)clientCircularBuffer_) + itemsPerFrame; // last frame is also included
//
//	ItemDataIterator queryBegin = prevQueryEnd_;
//	ItemDataIterator queryEnd((ItemData *)clientCircularBuffer_, distBeginBufferEndQueryItems, bufferNoLastFrameSizeItems_); // non-wrapped end
//
//	const int numItems = queryEnd - queryBegin; // computes distance with wrapping (note that queryEnd is not wrapped yet)
//	assert((numItems % itemsPerFrame) == 0); // full frames only
//
//	prevQueryEnd_ = queryEnd;
//	prevQueryEnd_.wrap();
//}

// Sets frame count to zero, note that next received frame may have 
// frame count > 0.
void LibertyTracker::resetFrameCount()
{
	libertyTracker_.ResetFrameCount();
	// NOTE: ResetHostFrameCount() resets p&o buffer index (?)
	// NOTE: ResetTimeStamp() isn't called because time stamps aren't used
}

void LibertyTracker::stopReceivingEvents()
{
	libertyTracker_.StopContPno(); // stop continuous mode
	libertyTracker_.SetSyncEnabled(FALSE); // stop sending external synch signal
}

void LibertyTracker::disconnect()
{
	libertyTracker_.Disconnect();
}

bool LibertyTracker::isOk()
{
	return (libertyTracker_.GetLastResult() == PI_NOERROR);
}
