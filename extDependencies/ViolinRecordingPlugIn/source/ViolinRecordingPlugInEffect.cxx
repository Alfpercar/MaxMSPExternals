#include "ViolinRecordingPlugInEffect.hxx"

#include "ViolinRecordingPlugInEditor.hxx"

#include <sstream>
#include <fstream>

#include "concat/Utilities/FloatToInt.hxx"
#include "concat/Utilities/Logging.hxx"
#include "TrackerThread.hxx"
#include "NotifyingLogAppender.hxx"
#include "AsynchFileLogger.hxx"
#include "AsynchFileWriter.hxx"
#include "FileWriters.hxx"
#include "Exceptions.hxx"
//l2 #include "Crc32.hxx"

#include "concat/FileFormats/HeaderAndMetronomeFile.hxx"

using namespace concat;

// ---------------------------------------------------------------------------------------

#if (ENABLE_OPTICAL_SENSOR != 0)
const int numItemsPerFrameArduino = 3; // gages + load cell + optical
#else
const int numItemsPerFrameArduino = 2; // gages + load cell
#endif
const int numItemsPerFramePolhemus = 12; // 2 sensors with for each (x, y, z) + (alpha, beta, gamma)

// ---------------------------------------------------------------------------------------

// global function that creates instances of concrete effect class, 
// function declared in wrapper/juce_AudioFilterBase.h
AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new ViolinRecordingPlugInEffect();
}

// ---------------------------------------------------------------------------------------

ViolinRecordingPlugInEffect::ViolinRecordingPlugInEffect()
{
	// Config (default):
	useGagesEnabled_ = false;
	if (ComPort::getNumComPorts() == 0)
		comPortIdx_ = 0;
	else
		comPortIdx_ = ComPort::getNumComPorts() - 1; // highest com port is most likely last installed one, which is most likely the Arduino USB virtual serial port driver
	useStereoEnabled_ = false;
	isAutoStringEnabled_ = false;
	useScene3dEnabled_ = false;
	isManualSyncEnabled_ = true;
	isPlotSyncSigEnabled_ = false;
	isContRetriggerEnabled_ = false;

	if (ComPort::getNumComPorts() == 0)
		useGagesEnabled_ = false; // (here in case default changes to true, now superfluous)

	// State (initial):
	trackerState_ = TRACKER_DISCONNECTED;
	trackerCalibrationState_ = TRACKER_NOT_CALIBRATED;
	recordingState_ = NOT_RECORDING;
	operationMode_ = NORMAL;

	scoreListFilename_ = String::empty;
	outputPath_ = String::empty;
	readyToRecord_.set(false);

	curPhraseIdx_ = 0;
	scoreList_.clearItems();

	trackerToAudioSyncOffset_ = 0; // default
	arduinoToTrackerSyncOffset_ = 0; // default
	syncRequiresRetrigger_ = true;

	useTuningRefTone_.set(false);

	// Sub components:
	eventLogAppender_.setListener(this);
	fileLogger_.open("violin_recording_plugin-rec_log.txt", false, 8); // will be relocated whenever output path is set

	concat::Logger &log1 = concat::Logger::getLogger("violin_recording_plugin");
	concat::Logger &log2 = concat::Logger::getLogger("asynch_file_writer");
	concat::Logger &log3 = concat::Logger::getLogger("liberty_tracker");
	concat::Logger &log4 = concat::Logger::getLogger("violin_recording_plugin.filelog");
	concat::Logger &log5 = concat::Logger::getLogger("log.ignored_exceptions");
	
	log1.addAppender(eventLogAppender_);
	log2.addAppender(eventLogAppender_);
	log3.addAppender(eventLogAppender_);
	log5.addAppender(eventLogAppender_);
	log1.addAppender(fileLogger_);
	log2.addAppender(fileLogger_);
	log3.addAppender(fileLogger_);
	log4.addAppender(fileLogger_);
	log5.addAppender(fileLogger_);

	trackerThread_ = new TrackerThread(this);

	initAsynchDiskWriters();

	// Internal:
	wasRecordingLastFrame_ = false; // also reset on resume()
	wasTrackerConnectedLastFrame_ = false; // also reset on resum()

	prevFrameStylusButtonPressed_ = false;

	hasTrackerCalibrationBeenModified_ = false;

	editorEventQueue_.reserve(32);

	blockSize_ = -1;
	sampleRate_ = -1.0;

	displayedNumChannels_ = false;

#if (ENABLE_CHECK_PROCESSING_DEAD != 0)
	isProcessingDead_.set(true);
#endif

	comPortReadBuffer_.attach(comPort_, VALID_ARDUINO_FRAMES_BUFFER_SIZE);
//l2	computeCrc32Table(crcTable256_, 0x04c11db7);

	// (only using lastTrackerFrameInterp_ after filling it)

	hasValidLastCamera_ = false;

	LOG_INFO_N("violin_recording_plugin", formatStr("COMPUTER: %s.", getSystemName().c_str()));
	LOG_INFO_N("violin_recording_plugin", formatStr("DATE: %s.", getSystemDate().c_str()));
	LOG_INFO_N("violin_recording_plugin", formatStr("TIME: %s.", getSystemTime().c_str()));
#ifdef _DEBUG
	LOG_INFO_N("violin_recording_plugin", "Debug build.");
#else
	LOG_INFO_N("violin_recording_plugin", "Release build.");
#endif
	LOG_INFO_N("violin_recording_plugin", "Plug-in instantiated.");

	isProcessingSuspended_ = true;
}

ViolinRecordingPlugInEffect::~ViolinRecordingPlugInEffect()
{
	LOG_INFO_N("violin_recording_plugin", "Performing clean-up...");

	stopRecording(); // just in case still recording
	
	if (trackerThread_ != NULL)
	{
		disconnectTrackerSynch();
		delete trackerThread_;
		trackerThread_ = NULL;
	}

	destroyAsynchDiskWriters();

	LOG_INFO_N("violin_recording_plugin", "Done!");
	LOG_INFO_N("violin_recording_plugin", formatStr("TIME: %s.", getSystemTime().c_str()));

	fileLogger_.close();
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	LOG_INFO_N("violin_recording_plugin", "Preparing audio processing resources...");

	// Update state, initialize:
	wasRecordingLastFrame_ = false;
	wasTrackerConnectedLastFrame_ = false;

	recordingState_ = NOT_RECORDING;

	displayedNoHostTimingInfoError_ = false;
	displayedBarTooLongError_ = false;
	displayedSerialCommError_ = false;

	// Allocate asynchronous file writers:
	createAsynchDiskWriters(sampleRate);

	// Log messages:
	if (sampleRate_ != sampleRate)
	{
		LOG_INFO_N("violin_recording_plugin", formatStr("Sample rate: %.2f.", sampleRate));
		if (sampleRate != 44100.0)
			LOG_WARN_N("violin_recording_plugin", "[r]Warning: Audio sample rate is not 44.1 kHx.");
		sampleRate_ = sampleRate;
	}

	if (blockSize_ != samplesPerBlock)
	{
		LOG_INFO_N("violin_recording_plugin", formatStr("Block size: %d.", samplesPerBlock));
		blockSize_ = samplesPerBlock;
	}

	// Input waveform computation:
	inWaveformStepSize_ = sampleRate/tracker_.getSampleRate();
	inWaveformNextStepEnd_ = inWaveformStepSize_;
	inWaveformNextStepEndRounded_ = round_int(inWaveformNextStepEnd_);
	inWaveformCurPos_ = 0;
	inWaveformCurValue_ = 0.0f;

	// Tuning reference tone generator:
	tuningRefToneGenerator_.generateTable(442.0, sampleRate, FixedFreqTone::TRIANGLE);

	// Sync out signal generator:
	syncOutGen_.init(4.0, sampleRate_, tracker_.getSampleRate(), 25, blockSize_, blockSize_);

	isProcessingSuspended_ = false;
}

void ViolinRecordingPlugInEffect::releaseResources()
{
	LOG_INFO_N("violin_recording_plugin", "Releasing audio processing resources...");

	isProcessingSuspended_ = true;

	stopRecording(); // just in case still recording
	disconnectTrackerSynch(); // blocking

	updateEntireEditorSignal.sendChangeMessage(NULL);

	destroyAsynchDiskWriters();
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEffect::processBlock(AudioSampleBuffer &audioBuffer, MidiBuffer &midiMessages)
{
BEGIN_IGNORE_EXCEPTIONS
#if (ENABLE_CHECK_PROCESSING_DEAD != 0)
	isProcessingDead_.set(false);
#endif
	processingDebugLocator = 0;

	if (!displayedNumChannels_)
	{
		LOG_INFO_N("violin_recording_plugin", formatStr("Num. inputs: %d.", getNumInputChannels()));
		LOG_INFO_N("violin_recording_plugin", formatStr("Num. outputs: %d.", getNumOutputChannels()));
		displayedNumChannels_ = true;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Get timing information from host:
	processingDebugLocator = 1;
	AudioPlayHead::CurrentPositionInfo pos;

	if (!getPlayHead()->getCurrentPosition(pos))
	{
		// Shouldn't happen normally:
		if (!displayedNoHostTimingInfoError_)
		{
			LOG_ERROR_N("violin_recording_plugin", "[r]ERROR: No host timing info!");
			displayedNoHostTimingInfoError_ = true;
		}

		return; // return from function (memory state for next call will be messed up)
	}

//	pos.bpm = 120.0; // XXX: override tempo for testing in hosts that don't support VstTimeInfo

	if (pos.bpm <= 30.0)
	{
		if (displayedBarTooLongError_ == false)
		{
			LOG_ERROR_N("violin_recording_plugin", "[r]ERROR: Tempo must be greater than 30 BPM to allow buffering history.");
			displayedBarTooLongError_ = true;
		}

		return;
	}
	else
	{
		displayedBarTooLongError_ = false;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	const bool isTrackerConnectedCurFrame = (trackerState_ == TRACKER_CONNECTED);

	if (isTrackerConnectedCurFrame) // TRACKER_CONNECTED
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		// If first frame when connected, discard any already received frames and reset frame count:
		processingDebugLocator = 2;
		bool reset = startTrackerAndArduinoStreamsOnFirstFrameAfterConnect(); // also resets all frame count variables and such

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		// Get tracker data from buffer:
		processingDebugLocator = 3;
		if (reset)
			LOG_INFO_N("violin_recording_plugin", "Getting data from Polhemus...");
		LibertyTracker::ItemDataIterator beginBuffer;
		const int numTrackerItems = tracker_.queryFrames(beginBuffer);
		const int numTrackerSensors = tracker_.getNumEnabledSensors(); // num items per frame
		const int numTrackerFrames = numTrackerItems/numTrackerSensors;

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		// Check if no tracker frames were dropped:
		processingDebugLocator = 4;
		if (reset)
			LOG_INFO_N("violin_recording_plugin", "Checking frame continuity...");
		checkTrackerFrameCountContinuity(beginBuffer, numTrackerFrames, numTrackerSensors);

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		// Get Arduino data from OS comm. port buffer:
		numValidArduinoFrames_ = 0;
		if (useGagesEnabled_)
		{
			processingDebugLocator = 6;
			if (reset)
				LOG_INFO_N("violin_recording_plugin", "Getting com port data...");
			
			numValidArduinoFrames_ = getArduinoDataFromSerialPort(comPortReadBuffer_, validArduinoFrames_, numArduinoBytesSkipped_);
			checkArduinoFrameCountContinuity(validArduinoFrames_, numValidArduinoFrames_);
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		// Generate sync out signal and estimate stream offsets from received sync signals:
		float *syncOutBuffer = NULL;
		if (getNumOutputChannels() >= 3)
			syncOutBuffer = audioBuffer.getSampleData(2, 0);

		syncOutGen_.processAudio(syncOutBuffer, audioBuffer.getNumSamples(), useGagesEnabled_);
		syncOutGen_.processTracker(beginBuffer, numTrackerFrames, numTrackerSensors);
		syncOutGen_.processArduino(validArduinoFrames_, numValidArduinoFrames_);

		if (!isManualSyncEnabled_)
		{
			// Automatically adjust sync:
			int trToAudOffset = syncOutGen_.getEstimatedTrackerToAudioSyncOffset();
			if (trToAudOffset != INVALID_STREAM_OFFSET)
			{
				if (trackerToAudioSyncOffset_ != trToAudOffset)
					LOG_INFO_N("violin_recording_plugin", concat::formatStr("Changing tr./aud. offset %d -> %d (%s)", trackerToAudioSyncOffset_, trToAudOffset, getSystemTime().c_str()));
				trackerToAudioSyncOffset_ = trToAudOffset;
			}

			int ardToTrOffset = syncOutGen_.getEstimatedArduinoToTrackerSyncOffset();
			if (ardToTrOffset != INVALID_STREAM_OFFSET)
			{
				if (arduinoToTrackerSyncOffset_ != ardToTrOffset)
					LOG_INFO_N("violin_recording_plugin", concat::formatStr("Changing ard./tr. offset %d -> %d (%s)", arduinoToTrackerSyncOffset_, ardToTrOffset, getSystemTime().c_str()));
				arduinoToTrackerSyncOffset_ = ardToTrOffset;
			}

			if (trToAudOffset != INVALID_STREAM_OFFSET || ardToTrOffset != INVALID_STREAM_OFFSET)
				updateEntireEditorSignal.sendChangeMessage(NULL);
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		// Send audio/tracker/Arduino data to history buffers (for file writing) and editor (plots); 
		// These functions also take care of interpolating on frame drops and stream synchronization:
#if (DISABLE_SENDING_DATA == 0)
		processingDebugLocator = 10;
		if (reset)
			LOG_INFO_N("violin_recording_plugin", "Sending data to history buffers and editor...");
		sendAudioDataToHistoryBufferAndEditor(audioBuffer);
		sendTrackerDataToHistoryBufferAndEditor(beginBuffer, numTrackerFrames, numTrackerSensors);
		sendArduinoDataToHistoryBufferAndEditor(validArduinoFrames_, numValidArduinoFrames_);
		sendDataToScene3d(beginBuffer, numTrackerFrames, numTrackerSensors);
#endif

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		if (operationMode_ == CALIBRATING_TRACKER)
		{
			processingDebugLocator = 11;
			// Wait for 'take sample' event:
            processTrackerDataForTrackerCalibratingMode(beginBuffer, numTrackerFrames, numTrackerSensors);
		}
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		else if (operationMode_ == CALIBRATING_FORCE_POINTS)
		{
			processingDebugLocator = 12;
			// Wait for 'take sample' event:
			processTrackerDataForForceCalibratingMode(beginBuffer, numTrackerFrames, numTrackerSensors);
		}
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		else if (operationMode_ == SAMPLING_3D_MODEL)
		{
			processingDebugLocator = 13;
			processTrackerDataForSampling3dModelMode(beginBuffer, numTrackerFrames, numTrackerSensors);
		}
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		else if (operationMode_ == NORMAL || operationMode_ == CALIBRATING_FORCE)
		{
			processingDebugLocator = 14;
			if (reset)
				LOG_INFO_N("violin_recording_plugin", "Processing normal...");

			bool isCalibrated = (trackerCalibrationState_ == TRACKER_CALIBRATED);
			bool isReadyForRecording = (isCalibrated && readyToRecord_.isSet());//(isCalibrated && scoreListFilename_ != String::empty && outputPath_ != String::empty && pos.bpm >= minAllowedTempo_);

			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			// If ready to record, check if transport changed:
			processingDebugLocator = 15;
			if (isReadyForRecording)
			{
				// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
				// Transport changed from not recording (stop/pause/playback) to recording:
				if (pos.isRecording == true && wasRecordingLastFrame_ == false)
				{
					startRecording(pos, audioBuffer.getNumSamples(), numTrackerFrames);
				}
				// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
				// Transport changed from recording to not recording (stop/pause/playback):
				else if (pos.isRecording == false && wasRecordingLastFrame_ == true)
				{
					stopRecording();
				}

				wasRecordingLastFrame_ = pos.isRecording; // remember for next frame (only when isReadyForRecording == true)
			}
		}
	}

	// Remember for next frame:
	wasTrackerConnectedLastFrame_ = isTrackerConnectedCurFrame;
	processingDebugLocator = 16;

	if (trackerState_ == TRACKER_PENDING_DISCONNECT)
	{
		pendingDisconnectCanComplete_.signal();
	}

	// Generate tuning tone:
	// Note: This should be done AFTER writing to disk, because processing is 
	// done in-place so filling output effectively overwrites input. Thus in case 
	// the tone is generated before writing to disk, the tone is written to disk, not 
	// the input audio!
	if (useTuningRefTone_.isSet() && !pos.isRecording)
	{
		if (getNumOutputChannels() >= 2)
			tuningRefToneGenerator_.synthesize2(audioBuffer.getSampleData(0, 0), audioBuffer.getSampleData(1, 0), audioBuffer.getNumSamples());
		else
			tuningRefToneGenerator_.synthesize1(audioBuffer.getSampleData(0, 0), audioBuffer.getNumSamples());
	}

END_IGNORE_EXCEPTIONS(formatStr("ViolinRecordingPlugInEffect::processBlock() (processing debug locator = %d)", processingDebugLocator).c_str())
}

bool ViolinRecordingPlugInEffect::startTrackerAndArduinoStreamsOnFirstFrameAfterConnect()
{
	if (!wasTrackerConnectedLastFrame_)
	{
		LOG_INFO_N("violin_recording_plugin", "Starting tracker and Arduino data streams...");

		// Start receiving events (call from high priority thread in the hope to get 
		// less latency between tracker commands over USB):
		tracker_.startReceivingEvents();
		if (!tracker_.isOk())
		{
			LOG_ERROR_N("violin_recording_plugin", "[r]ERROR: Failed starting tracker stream (disconnecting)!");
//TEMP			disconnectTrackerAsynch();
		}

		// Reset all related variables:
		lastTrackerFrameCountCheckContinuity_ = -1; // (wrapped)
		lastTrackerFrameCountInterp_ = -1; // (wrapped)

		if (useGagesEnabled_)
		{
			lastArduinoFrameCountCheckContinuity_ = -1; // (wrapped)
			lastArduinoFrameCountInterp_ = -1; // (wrapped)
			numArduinoBytesSkipped_ = 0;
		}

		curUsedTrackerToAudioSyncOffset_ = 0;
		curUsedArduinoToAudioSyncOffset_ = 0;

		if (!isManualSyncEnabled_)
		{
			trackerToAudioSyncOffset_ = 0;
			arduinoToTrackerSyncOffset_ = 0;
			updateEntireEditorSignal.sendChangeMessage(NULL);
		}

		// Reset asynchronous disk writers to reset unwrapped write idx count (buffer 
		// should normally already be empty because disconnect flushes buffer):
		LOG_INFO_N("violin_recording_plugin", "Resetting asynchronous disk writers...");
		processingDebugLocator = 201;
		resetAsynchDiskWriters();

		// Sync out signal generator:
		syncOutGen_.resetFrameCounts();
		syncOutGen_.resetEstimatedOffsets();

		return true;
	}

	return false;
}

void ViolinRecordingPlugInEffect::checkTrackerFrameCountContinuity(LibertyTracker::ItemDataIterator iter, int numTrackerFrames, int numItemsPerTrackerFrame)
{
	for (int i = 0; i < numTrackerFrames; ++i)
	{
		// Check frame count continuity of subsequent frames:
		const uint32_t frameCount = iter.item().frameCount;
		iter.advance(1);

		// If not first frame, check if difference between last frame count and current 
		// frame count is one:
		if (lastTrackerFrameCountCheckContinuity_ != -1)
		{
			const int32_t frameSkip = frameCount - lastTrackerFrameCountCheckContinuity_ - 1;
			if (frameSkip > 0)
			{
				LOG_INFO_N("violin_recording_plugin", formatStr("[r]Tracker data loss: %d frames (frame count gap: %u -> %u).", frameSkip, lastTrackerFrameCountCheckContinuity_, frameCount));
			}
			else if (frameSkip < 0)
			{
				// normal negative frame skip can happen when resetting frame count, but displayed anyways 
				// for debugging purposes
				LOG_INFO_N("violin_recording_plugin", formatStr("Tracker negative frame count gap (may be normal): %u -> %u.", lastTrackerFrameCountCheckContinuity_, frameCount));
			}
		}
		else
		{
//			const int32_t frameSkip = frameCount;
//			if (frameSkip > 0)
//			{
//				LOG_INFO_N("violin_recording_plugin", formatStr("[r]Tracker data loss (initial): %d frames (should start at 0).", frameSkip));
				LOG_INFO_N("violin_recording_plugin", formatStr("Tracker stream initial frame count: %d.", frameCount));
//			}
		}
		lastTrackerFrameCountCheckContinuity_ = frameCount;

		// Check if all items of the frame have the same frame count (just sanity check, should never happen normally):
		for (int j = 1; j < numItemsPerTrackerFrame; ++j)
		{
			const uint32_t frameCount2 = iter.item().frameCount;
			iter.advance(1);

			if (frameCount2 != frameCount)
			{
				LOG_INFO_N("violin_recording_plugin", "[r]ERROR: Corrupt tracker frame count!");
			}
		}
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int ViolinRecordingPlugInEffect::getArduinoDataFromSerialPort(ComPortReadBuffer &readBuffer, ArduinoFrame *frames, int &numArduinoBytesSkipped)
{
	bool readOk = readBuffer.readAllAvailable();
	if (!readOk)
	{
		if (!displayedSerialCommError_)
		{
			LOG_WARN_N("violin_recording_plugin", "[r]Warning: Arduino serial communication error.");
			displayedSerialCommError_ = true;
		}
		return 0;
	}

	ComPortReadBuffer::Iterator beginBufferArduino = readBuffer.getReadIter();
	const int numBytesReadArduino = readBuffer.getReadAvailable();

	// Validate/correct serial data:
	// Note: Each frame of data is send as start byte (0xff), payload (uint32 frame 
	// count + int16 + int16, both 10 bit values), checksum (XOR/CRC32) of payload (4 bytes).
	// The idea is that if a communication error occurs, looking for a 0xff byte followed by 
	// 8 bytes whose checksum matches the 4 bytes after, is very likely a frame onset.
	// Partial or corrupt frames are simply skipped. Later on the data is interpolated 
	// based on gaps in the frame counts.
	const int arduinoPayloadSizeBytes = 4 + 2*numItemsPerFrameArduino + 1; // uint32_t timestamp (4) + int16_t value gages (2) + int16_t value load cell (2) + int16_t value optical + byte ext sync flag (1)
	const int arduinoFrameSizeBytes = 1 + arduinoPayloadSizeBytes + 4; // start byte + payload + crc32
	int numProcessedArduinoBytes = 0;
	
	int numFrames = 0;

	while (numProcessedArduinoBytes < numBytesReadArduino)
	{
		int i = numProcessedArduinoBytes;
		const int remainingBytes = numBytesReadArduino - i;

		// Stop if less than a whole frame is available (part of current frame will be kept in 
		// the ComPortReadBuffer):
		if (remainingBytes < arduinoFrameSizeBytes)
			break;

		// Look for start byte:
		if (beginBufferArduino[i] == 0xff)
		{
			// Compute checksum of payload:
			uint32_t computePayloadChecksum = 0;
			for (int k = 0; k < arduinoPayloadSizeBytes; ++k)
			{
				computePayloadChecksum ^= beginBufferArduino[i+1+k];
			}
//l2		const uint32_t computePayloadChecksum = computeCrc32(crcTable256_, beginBufferArduino.getPtrAt(i+1), arduinoPayloadSizeBytes);

			const uint32_t referencePayloadChecksum = beginBufferArduino.getUint32At(i+1+arduinoPayloadSizeBytes);
//l2		const uint32_t referencePayloadChecksum = beginBufferArduino.getUint32At(i+1+arduinoPayloadSizeBytes);

			// See if checksum of payload is correct:
			if (computePayloadChecksum == referencePayloadChecksum)
			{
				if (numFrames < VALID_ARDUINO_FRAMES_BUFFER_SIZE)
				{
					frames[numFrames].frameCount = beginBufferArduino.getUint32At(i+1);
					frames[numFrames].valueGages =  beginBufferArduino.getInt16At(i+1+4);
					frames[numFrames].valueLoadCell = beginBufferArduino.getInt16At(i+1+4+2);
#if (ENABLE_OPTICAL_SENSOR != 0)
					frames[numFrames].valueOptical = beginBufferArduino.getInt16At(i+1+4+2+2);
					frames[numFrames].extSyncFlag = beginBufferArduino.getByteAt(i+1+4+2+2+2);
#else
					frames[numFrames].extSyncFlag = beginBufferArduino.getByteAt(i+1+4+2+2);
#endif
					++numFrames;
				}
				else
				{
					LOG_WARN_N("violin_recording_plugin", "[r]Warning: Arduino valid frame buffer overrun.");
				}

				// Valid frame, continue to next frame:
				numProcessedArduinoBytes += arduinoFrameSizeBytes;

				if (numArduinoBytesSkipped != 0)
				{
					LOG_WARN_N("violin_recording_plugin", formatStr("[r]Arduino communication failure, skipped %d bytes to onset valid frame.", numArduinoBytesSkipped));
				}
				numArduinoBytesSkipped = 0;
			}
			else
			{
				// Invalid starting point in data stream, try next byte:
				++numProcessedArduinoBytes;
				++numArduinoBytesSkipped;
			}
		}
		else
		{
			// Invalid starting point in data stream, try next byte:
			++numProcessedArduinoBytes;
			++numArduinoBytesSkipped;
		}
	}

	readBuffer.advanceReadIter(numProcessedArduinoBytes);

	return numFrames;
}

void ViolinRecordingPlugInEffect::checkArduinoFrameCountContinuity(const ArduinoFrame *arduinoFrames, int numArduinoFrames)
{
	for (int i = 0; i < numArduinoFrames; ++i)
	{
		// Check frame count continuity of subsequent frames:
		const uint32_t frameCount = arduinoFrames[i].frameCount;

		// If not first frame, check if difference between last frame count and current 
		// frame count is one:
		if (lastArduinoFrameCountCheckContinuity_ != -1)
		{
			const int32_t frameSkip = frameCount - lastArduinoFrameCountCheckContinuity_ - 1;
			if (frameSkip > 0)
			{
				LOG_INFO_N("violin_recording_plugin", formatStr("[r]Arduino data loss: %d frames (frame count gap: %u -> %u).", frameSkip, lastArduinoFrameCountCheckContinuity_, frameCount));
			}
			else if (frameSkip < 0)
			{
				// normal negative frame skip can happen when resetting frame count, but displayed anyways 
				// for debugging purposes
				LOG_INFO_N("violin_recording_plugin", formatStr("Arduino negative frame count gap (may be normal): %u -> %u.", lastArduinoFrameCountCheckContinuity_, frameCount));
			}
		}
		else
		{
//			const int32_t frameSkip = frameCount;
//			if (frameSkip > 0)
//			{
//				LOG_INFO_N("violin_recording_plugin", formatStr("[r]Arduino data loss (initial): %d frames (should start at 0).", frameSkip));
				LOG_INFO_N("violin_recording_plugin", formatStr("Arduino stream initial frame count: %d.", frameCount));
//			}
		}

		lastArduinoFrameCountCheckContinuity_ = frameCount;
	}
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEffect::sendAudioDataToHistoryBufferAndEditor(AudioSampleBuffer &audioBuffer)
{
	assert(trackerState_ == TRACKER_CONNECTED || trackerState_ == TRACKER_PENDING_DISCONNECT);

	ViolinRecordingPlugInEditor *editor = (ViolinRecordingPlugInEditor *)getActiveEditor();
	bool canSendDataToEditor = (editor != NULL && editor->arePlotGraphsValid_.isSet());
	// XXX: assume editor and widgets always exist, widgets or the buffers they use should really be moved to effect in future

	// Send to history buffer:
	if (getNumInputChannels() >= 1)
		audioCh1Writer_->writeData(audioBuffer.getSampleData(0), audioBuffer.getNumSamples()); // (internally checks and logs if not all of the requested items could be written)
	if (getNumInputChannels() >= 2)
		audioCh2Writer_->writeData(audioBuffer.getSampleData(1), audioBuffer.getNumSamples()); // (internally checks and logs if not all of the requested items could be written)

	// Send to editor:
	int ch = 0; // channel 1 (audio in left or tune reference out)
	float inputGainBoost = 4.0f; // 4x boost to see sound of stick hitting bridge better

	if (isPlotSyncSigEnabled_ && audioBuffer.getNumChannels() >= 3)
	{
		ch = 2; // channel 3 (sync out)
		inputGainBoost = 1.0f; // don't boost sync signals
	}

	const float *in1 = audioBuffer.getSampleData(ch, 0);
	const int n = audioBuffer.getNumSamples();
	for (int i = 0; i < n; ++i)
	{
		inWaveformCurValue_ = jmax(inWaveformCurValue_, fabs(in1[i]));
		++inWaveformCurPos_;

		if (inWaveformCurPos_ >= inWaveformNextStepEndRounded_)
		{
			editor->noiseAndSyncPlot_->postValue(1, inputGainBoost*10.0f*inWaveformCurValue_); // scaling factor to allow easier visual synchronization (but clipping can't be trusted)

			inWaveformCurValue_ = 0.0f;
			inWaveformNextStepEnd_ += inWaveformStepSize_;
			inWaveformNextStepEndRounded_ = round_int(inWaveformNextStepEnd_);
		}
	}
}

void ViolinRecordingPlugInEffect::sendTrackerDataToHistoryBufferAndEditor(LibertyTracker::ItemDataIterator iter, int numTrackerFrames, int numTrackerSensors)
{
	assert(trackerState_ == TRACKER_CONNECTED || trackerState_ == TRACKER_PENDING_DISCONNECT);

	ViolinRecordingPlugInEditor *editor = (ViolinRecordingPlugInEditor *)getActiveEditor();
	bool canSendDataToEditor = (editor != NULL && editor->arePlotGraphsValid_.isSet());
	// XXX: assume editor and widgets always exist, widgets or the buffers they use should really be moved to effect in future

	// Apply changes in synchronization offset between tracker and audio streams:
	const int trackerToAudioSyncDelta = trackerToAudioSyncOffset_ - curUsedTrackerToAudioSyncOffset_;
	if (trackerToAudioSyncDelta > 0)
	{
		if (!isContRetriggerEnabled_)
			LOG_INFO_N("violin_recording_plugin", formatStr("Applying delay to tracker stream (%d frames)...", trackerToAudioSyncDelta));
		curUsedTrackerToAudioSyncOffset_ = trackerToAudioSyncOffset_;
	}
	else if (trackerToAudioSyncDelta < 0)
	{
		if (!isContRetriggerEnabled_)
			LOG_INFO_N("violin_recording_plugin", formatStr("Applying advance to tracker stream (%d frames)...", -trackerToAudioSyncDelta));
		curUsedTrackerToAudioSyncOffset_ = trackerToAudioSyncOffset_;
	}

	// Process tracker frames:
	for (int i = 0; i < numTrackerFrames; ++i)
	{
		// Compute 'raw' descriptors for current frame:
		RawSensorData rawSensorData = computeDescriptors_.trackerDataToRawSensorData(iter);

		// Compute frame count of frame:
		uint32_t frameCount;
		if (lastTrackerFrameCountInterp_ != -1)
		{
			frameCount = iter.item().frameCount + curUsedTrackerToAudioSyncOffset_;
		}
		else
		{
			frameCount = iter.item().frameCount; // don't offset first frame to avoid not having a lhs frame for interpolation
		}

		bool dropCurFrame = false;

		if (lastTrackerFrameCountInterp_ == -1)
		{
			// Special case for first frame in stream:
			// Allowed to have any frame count.
			// Don't apply interpolation of frame drop just do current frame (dropCurFrame = false).
		}
		else
		{
			// Compute jump in consecutive frame frame counts:
			int32_t frameJump = frameCount - lastTrackerFrameCountInterp_ - 1; // 0 means no skip

			// Positive jump, tracker dropped frames (hardware buffer overrun) or tracker stream 
			// was delayed for synchronization:
			if (frameJump > 0)
			{
				// Insert interpolated frames to make up for gap:
				RawSensorData rawSensorDataInterp;

				if (frameJump > 1000)
				{
					// Avoid insertion loop blocking in case of some unforeseen error (normally shouldn't happen):
					frameJump = 0;
					LOG_INFO_N("violin_recording_plugin", "[r]ERROR: Tracker frame jump too big! (limiting to 1000)");
				}

				for (int j = 0; j < frameJump; ++j)
				{
					const float d = float(1+j)/float(1+frameJump); // ]0;1[

					trackerFrameInterpolator_.interpolate(lastRawSensorDataInterp_, rawSensorData, d, rawSensorDataInterp);
					sendTrackerDataToHistoryBufferSingleFrame(rawSensorDataInterp);
					sendTrackerDataToEditorSingleFrame(editor, rawSensorDataInterp);
				}
			}
			// Negative jump, tracker stream was advanced for synchronization (or strange error):
			else if (frameJump < 0)
			{
				dropCurFrame = true;
			}
		}

		// Use current frame if it's not dropped:
		if (!dropCurFrame)
		{
			// Remember current frame for next iteration:
			lastTrackerFrameCountInterp_ = frameCount;
			lastRawSensorDataInterp_ = rawSensorData;

			// Send current frame to history buffer and editor:
			sendTrackerDataToHistoryBufferSingleFrame(rawSensorData);
			sendTrackerDataToEditorSingleFrame(editor, rawSensorData);
		}

		// Next frame:
		iter.advance(numTrackerSensors);
	}
}

void ViolinRecordingPlugInEffect::sendArduinoDataToHistoryBufferAndEditor(const ArduinoFrame *arduinoFrames, int numArduinoFrames)
{
	assert(trackerState_ == TRACKER_CONNECTED || trackerState_ == TRACKER_PENDING_DISCONNECT);

	ViolinRecordingPlugInEditor *editor = (ViolinRecordingPlugInEditor *)getActiveEditor();
	bool canSendDataToEditor = (editor != NULL && editor->arePlotGraphsValid_.isSet());
	// XXX: assume editor and widgets always exist, widgets or the buffers they use should really be moved to effect in future

	if (!useGagesEnabled_)
		return;

	// Apply changes in synchronization offset between Arduino and tracker streams:
	const int arduinoToAudioSyncOffset = arduinoToTrackerSyncOffset_ + trackerToAudioSyncOffset_;
	const int arduinoToAudioSyncDelta = arduinoToAudioSyncOffset - curUsedArduinoToAudioSyncOffset_;
	if (arduinoToAudioSyncDelta > 0)
	{
		if (!isContRetriggerEnabled_)
			LOG_INFO_N("violin_recording_plugin", formatStr("Applying delay to Arduino stream (%d frames)...", arduinoToAudioSyncDelta));
		curUsedArduinoToAudioSyncOffset_ = arduinoToAudioSyncOffset;
	}
	else if (arduinoToAudioSyncDelta < 0)
	{
		if (!isContRetriggerEnabled_)
			LOG_INFO_N("violin_recording_plugin", formatStr("Applying advance to Arduino stream (%d frames)...", -arduinoToAudioSyncDelta));
		curUsedArduinoToAudioSyncOffset_ = arduinoToAudioSyncOffset;
	}

	// Process Arduino frames:
	for (int i = 0; i < numArduinoFrames; ++i)
	{
		// Compute normalized sensor values:
		float normGagesValue = arduinoFrames[i].valueGages/1024.0f; // 10 bit
		normGagesValue = jmax(jmin(normGagesValue, 1023.0f/1024.0f), 0.0f); // normally won't be out of bounds but just in case (bug, heavy serial communication error, etc.)

		float normLoadCellValue = arduinoFrames[i].valueLoadCell/1024.0f; // 10 bit
		normLoadCellValue = jmax(jmin(normLoadCellValue, 1023.0f/1024.0f), 0.0f); // normally won't be out of bounds but just in case (bug, heavy serial communication error, etc.)

#if (ENABLE_OPTICAL_SENSOR != 0)
		float normOpticalValue = arduinoFrames[i].valueOptical/1024.0f; // 10 bit
		normOpticalValue = jmax(jmin(normOpticalValue, 1023.0f/1024.0f), 0.0f); // normally won't be out of bounds but just in case (bug, heavy serial communication error, etc.)
#endif

		// Compute frame count of frame:
		uint32_t frameCount;
		if (lastArduinoFrameCountInterp_ != -1)
		{
			frameCount = arduinoFrames[i].frameCount + curUsedArduinoToAudioSyncOffset_;
		}
		else
		{
			frameCount = arduinoFrames[i].frameCount; // don't offset first frame to avoid not having a lhs frame for interpolation
		}

		bool dropCurFrame = false;

		if (lastArduinoFrameCountInterp_ == -1)
		{
			// Special case for first frame in stream:
			// Allowed to have any frame count.
			// Don't apply interpolation of frame drop just do current frame (dropCurFrame = false).
		}
		else
		{
			// Compute jump in consecutive frame frame counts:
			int32_t frameJump = frameCount - lastArduinoFrameCountInterp_ - 1; // 0 means no skip

			// Positive jump, tracker dropped frames (hardware buffer overrun) or Arduino stream 
			// was delayed for synchronization:
			if (frameJump > 0)
			{
				// Insert interpolated frames to make up for gap:
				float normGagesValueInterp;
				float normLoadCellValueInterp;
#if (ENABLE_OPTICAL_SENSOR != 0)
				float normOpticalValueInterp;
#endif

				if (frameJump > 1000)
				{
					// Avoid insertion loop blocking in case of some unforeseen error (normally shouldn't happen):
					frameJump = 0;
					LOG_INFO_N("violin_recording_plugin", "[r]ERROR: Arduino frame jump too big! (limiting to 1000)");
				}

				for (int j = 0; j < frameJump; ++j)
				{
					const float d = float(1+j)/float(1+frameJump); // ]0;1[

					normGagesValueInterp = interp1Linear(lastArduinoGagesValueInterp_, normGagesValue, d);
					normLoadCellValueInterp = interp1Linear(lastArduinoLoadCellValueInterp_, normLoadCellValue, d);
#if (ENABLE_OPTICAL_SENSOR != 0)
					normOpticalValueInterp = interp1Linear(lastArduinoOpticalValueInterp_, normOpticalValue, d);
#endif

					float frame[numItemsPerFrameArduino];
					frame[0] = normGagesValueInterp;
					frame[1] = normLoadCellValueInterp;
#if (ENABLE_OPTICAL_SENSOR != 0)
					frame[2] = normOpticalValueInterp;
#endif
					arduinoWriter_->writeData(frame, numItemsPerFrameArduino); // (internally checks and logs if not all of the requested items could be written)

					editor->bowForcePlot_->postValue(1, 100.0f*normGagesValueInterp);
					editor->bowForcePlot_->postValue(2, 100.0f*normLoadCellValueInterp);
#if (ENABLE_OPTICAL_SENSOR != 0)
					editor->bowForcePlot_->postValue(0, 100.0f*normOpticalValueInterp);
#endif

                    editor->noiseAndSyncPlot_->postValue(2, 5.0f*0.0f); // digital signal, don't interpolate
					// (always send to plot 2 regardless of isPlotSyncSigEnabled_ to allow changing while connected)
				}
			}
			// Negative jump, Arduino stream was advanced for synchronization (or strange error):
			else if (frameJump < 0)
			{
				dropCurFrame = true;
			}
		}

		// Use current frame if it's not dropped:
		if (!dropCurFrame)
		{
			// Remember current frame for next iteration:
			lastArduinoFrameCountInterp_ = frameCount;
			lastArduinoGagesValueInterp_ = normGagesValue;
			lastArduinoLoadCellValueInterp_ = normLoadCellValue;
#if (ENABLE_OPTICAL_SENSOR != 0)
			lastArduinoOpticalValueInterp_ = normOpticalValue;
#endif

			// Send current frame to history buffer and editor:
			float frame[numItemsPerFrameArduino];
			frame[0] = normGagesValue;
			frame[1] = normLoadCellValue;
#if (ENABLE_OPTICAL_SENSOR != 0)
			frame[2] = normOpticalValue;
#endif
			arduinoWriter_->writeData(frame, numItemsPerFrameArduino); // (internally checks and logs if not all of the requested items could be written)

			editor->bowForcePlot_->postValue(1, 100.0f*normGagesValue);
			editor->bowForcePlot_->postValue(2, 100.0f*normLoadCellValue);
#if (ENABLE_OPTICAL_SENSOR != 0)
			editor->bowForcePlot_->postValue(0, 100.0f*normOpticalValue);
#endif

			if (isPlotSyncSigEnabled_)
			{
				if (arduinoFrames[i].extSyncFlag != 0)
					editor->noiseAndSyncPlot_->postValue(2, 5.0f*1.0f);
				else
					editor->noiseAndSyncPlot_->postValue(2, 5.0f*0.0f);
			}
			else
			{
				editor->noiseAndSyncPlot_->postValue(2, 5.0f*0.0f);
				// (always send to plot 2 regardless of isPlotSyncSigEnabled_ to allow changing while connected)
			}

		}
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ViolinRecordingPlugInEffect::sendTrackerDataToHistoryBufferSingleFrame(const RawSensorData &frame)
{
	static float trackerFrame[numItemsPerFramePolhemus];

	// sensor 1 (violin body):
	trackerFrame[0] = (float)frame.violinBodySensPos(0, 0);
	trackerFrame[1] = (float)frame.violinBodySensPos(1, 0);
	trackerFrame[2] = (float)frame.violinBodySensPos(2, 0);
	trackerFrame[3] = (float)frame.violinBodySensOrientation(0, 0);
	trackerFrame[4] = (float)frame.violinBodySensOrientation(1, 0);
	trackerFrame[5] = (float)frame.violinBodySensOrientation(2, 0);

	// sensor 2 (bow):
	trackerFrame[6] = (float)frame.bowSensPos(0, 0);
	trackerFrame[7] = (float)frame.bowSensPos(1, 0);
	trackerFrame[8] = (float)frame.bowSensPos(2, 0);
	trackerFrame[9] = (float)frame.bowSensOrientation(0, 0);
	trackerFrame[10] = (float)frame.bowSensOrientation(1, 0);
	trackerFrame[11] = (float)frame.bowSensOrientation(2, 0);

	trackerWriter_->writeData(trackerFrame, numItemsPerFramePolhemus); // (internally checks and logs if not all of the requested items could be written)
	// Note: Write entire frame as a single atomic operation.
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ViolinRecordingPlugInEffect::sendTrackerDataToEditorSingleFrame(ViolinRecordingPlugInEditor *editor, const RawSensorData &rawSensorData)
{
	assert(editor != NULL);
	assert(trackerState_ == TRACKER_CONNECTED || trackerState_ == TRACKER_PENDING_DISCONNECT);

	if (!editor->arePlotGraphsValid_.isSet())
		return;

	// Compute sensor accelerations:
	computeDescriptors_.computeSensorAccelerations(rawSensorData, tracker_.getSampleRate());

	// If calibrated (and not in the process of re-calibrating):
	if (trackerCalibrationState_ == TRACKER_CALIBRATED && operationMode_ != CALIBRATING_TRACKER)
	{
		// Compute derived 3d data and actual descriptors:
		Derived3dData derived3dData = computeDescriptors_.computeDerived3dData(rawSensorData, trackerCalibration_, isAutoStringEnabled_, anglesCalibration_, (operationMode_ == CALIBRATING_FORCE), &forceCalibration_);
		ViolinPerformanceDescriptors descriptors = computeDescriptors_.computeViolinPerformanceDescriptors(rawSensorData, derived3dData);

		// Plot all graphs (with sensor acceleration only from sens. 1 and 2):
		editor->bowDisplacementPlot_->postValue(0, (float)descriptors.bowDisplacement);
#if (ENABLE_OPTICAL_SENSOR == 0)
		editor->bowForcePlot_->postValue(0, (1.0f + (float)descriptors.bowForce)/3.0f*100.0f);
#endif
		editor->bowBridgeDistancePlot_->postValue(0, (float)descriptors.bowBridgeDistance);
		if (!isPlotSyncSigEnabled_)
		{
#if (PLOT_STICK_BRIDGE_DISTANCE == 0)
			// sensor accel.
			editor->noiseAndSyncPlot_->postValue(0, (float)(computeDescriptors_.getSens1Accel() + computeDescriptors_.getSens2Accel())/8000.0f*10.0f);
#else
			// stick-bridge dist.
			editor->noiseAndSyncPlot_->postValue(0, (float)(descriptors.stickBridgeDistance));
#endif
		}
		else
		{
			if (rawSensorData.extSyncFlag)
				editor->noiseAndSyncPlot_->postValue(0, 10.0f);
			else
				editor->noiseAndSyncPlot_->postValue(0, 0.0f);
		}
	}
	else
	{
		editor->bowDisplacementPlot_->postValue(0, 0.0f);
#if (ENABLE_OPTICAL_SENSOR == 0)
		editor->bowForcePlot_->postValue(0, 0.0f);
#endif
		editor->bowBridgeDistancePlot_->postValue(0, 0.0f);

		// If not calibrated, but calibrating tracker points:
		if (operationMode_ == CALIBRATING_TRACKER)
		{
			// Plot only sensor acceleration with all three sensors:
			if (!isPlotSyncSigEnabled_)
			{
#if (PLOT_STICK_BRIDGE_DISTANCE == 0)
				// sensor accel.
				editor->noiseAndSyncPlot_->postValue(0, (float)(computeDescriptors_.getSens1Accel() + computeDescriptors_.getSens2Accel() + computeDescriptors_.getSens3Accel())/8000.0f*10.0f);
#else
				// stick-bridge dist.
				editor->noiseAndSyncPlot_->postValue(0, 0.0f);
#endif
			}
			else
			{
				if (rawSensorData.extSyncFlag)
					editor->noiseAndSyncPlot_->postValue(0, 10.0f);
				else
					editor->noiseAndSyncPlot_->postValue(0, 0.0f);
			}
		}
		else
		{
			if (!isPlotSyncSigEnabled_)
			{
				editor->noiseAndSyncPlot_->postValue(0, 0.0f);
			}
			else
			{
				if (rawSensorData.extSyncFlag)
					editor->noiseAndSyncPlot_->postValue(0, 10.0f);
				else
					editor->noiseAndSyncPlot_->postValue(0, 0.0f);
			}
		}
	}
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEffect::sendDataToScene3d(LibertyTracker::ItemDataIterator iter, int numTrackerFrames, int numTrackerSensors)
{
	assert(trackerState_ == TRACKER_CONNECTED || trackerState_ == TRACKER_PENDING_DISCONNECT);

	ViolinRecordingPlugInEditor *editor = (ViolinRecordingPlugInEditor *)getActiveEditor();

	if (editor == NULL)
		return;

	if (!editor->isScene3dValid_.isSet())
		return;

	if (trackerCalibrationState_ == TRACKER_CALIBRATED && operationMode_ != CALIBRATING_TRACKER)
	{
		// Plot 3d scene with current calibration:
		for (int i = 0; i < numTrackerFrames; ++i)
		{
			RawSensorData rawSensorData = computeDescriptors_.trackerDataToRawSensorData(iter);
			Derived3dData derived3dData = computeDescriptors_.computeDerived3dData(rawSensorData, trackerCalibration_, isAutoStringEnabled_, anglesCalibration_, (operationMode_ == CALIBRATING_FORCE), &forceCalibration_);
			// XXX: above descriptors are compute twice
			ViolinPerformanceDescriptors descriptors = computeDescriptors_.computeViolinPerformanceDescriptors(rawSensorData, derived3dData);

			editor->scene3d_->postData(rawSensorData, derived3dData, descriptors, operationMode_ == CALIBRATING_FORCE);

			iter.advance(numTrackerSensors);
		}
	}
	else
	{
		bool hasValidPrevCalibration = trackerCalibrationPrev_.areAllStepFlagsSet();

		// If not calibrated, but calibrating tracker points:
		if (operationMode_ == CALIBRATING_TRACKER && hasValidPrevCalibration)
		{
			// Plot 3d scene with tracker points of previous calibration:
			for (int i = 0; i < numTrackerFrames; ++i)
			{
				RawSensorData rawSensorData = computeDescriptors_.trackerDataToRawSensorData(iter);
				Derived3dData derived3dDataPrevCalib = computeDescriptorsPrevCalib_.computeDerived3dData(rawSensorData, trackerCalibrationPrev_, isAutoStringEnabled_, anglesCalibration_, (operationMode_ == CALIBRATING_FORCE), &forceCalibration_);
				// XXX: above descriptors are compute twice
				ViolinPerformanceDescriptors descriptorsPrevCalib = computeDescriptorsPrevCalib_.computeViolinPerformanceDescriptors(rawSensorData, derived3dDataPrevCalib);

				editor->scene3d_->postData(rawSensorData, derived3dDataPrevCalib, descriptorsPrevCalib, operationMode_ == CALIBRATING_FORCE);

				iter.advance(numTrackerSensors);
			}
		}
		// Otherwise (always):
		else
		{
			// Plot 3d scene without calibration data (just sensor positions/orientations):
			for (int i = 0; i < numTrackerFrames; ++i)
			{
				RawSensorData rawSensorData = computeDescriptors_.trackerDataToRawSensorData(iter);
				editor->scene3d_->postData(rawSensorData);
				iter.advance(numTrackerSensors);
			}
		}
	}
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEffect::processTrackerDataForTrackerCalibratingMode(LibertyTracker::ItemDataIterator iter, int numTrackerFrames, int numItemsPerTrackerFrame)
{
	for (int i = 0; i < numTrackerFrames; ++i)
	{
		RawSensorData rawSensorData = computeDescriptors_.trackerDataToRawSensorData(iter);

		// Wait for 'take sample' event and send to editor to handle if received:
		EditorEvent unused;
		if ((rawSensorData.stylusButtonPressed && !prevFrameStylusButtonPressed_) || 
			editorEventQueue_.get(unused) == 1)
		{
			int64 curTimeMilliseconds = Time::getMillisecondCounter();
			int64 intervalMilliseconds = 0;
			if (lastCalibrationPointSampleTime_ != 0)
				intervalMilliseconds = curTimeMilliseconds - lastCalibrationPointSampleTime_;
			lastCalibrationPointSampleTime_ = curTimeMilliseconds;			
			LOG_INFO_N("violin_recording_plugin", formatStr("Point collected! (%.4f s)", intervalMilliseconds/1000.0));

			// Compute beta from sensor data (calibration step determines which sensor is 
			// used as a reference):
			int calibrationStep = trackerCalibration_.getCalibrationStep();
			if (calibrationStep >= TrackerCalibration::STR1_BRIDGE && calibrationStep < TrackerCalibration::BOW_FROG_LHS)
			{
				trackerCalibration_.getBetaForSetting(calibrationStep) = trackerCalibration_.computeBetaWithLogMsg(rawSensorData.stylusSensPos, rawSensorData.stylusSensOrientation, rawSensorData.violinBodySensPos, rawSensorData.violinBodySensOrientation);
			}
			else
			{
				trackerCalibration_.getBetaForSetting(calibrationStep) = trackerCalibration_.computeBetaWithLogMsg(rawSensorData.stylusSensPos, rawSensorData.stylusSensOrientation, rawSensorData.bowSensPos, rawSensorData.bowSensOrientation);
			}

			// Set step set flag:
			trackerCalibration_.setStepSetFlag(calibrationStep);

			// Increment step:
			trackerCalibration_.incrCalibrationStep();
			calibrationStep = trackerCalibration_.getCalibrationStep();

			// Check if calibration sequence is complete:
			if (calibrationStep == TrackerCalibration::NUM_STEPS)
			{
				endTrackerCalibrationSequence(true);

				endTrackerCalibrationSequenceReachedSignal.sendChangeMessage(NULL);
				// NOTE: signal editor thread because may display blocking warning dialog
			}
			// Else, print next step message (instructions):
			else
			{
				trackerCalibration_.printCalibrationStepMessage();
			}

			updateEntireEditorSignal.sendChangeMessage(NULL);
		}

		// Remember for next frame:
		prevFrameStylusButtonPressed_ = rawSensorData.stylusButtonPressed;

		// Advance to next frame:
		iter.advance(numItemsPerTrackerFrame);
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ViolinRecordingPlugInEffect::processTrackerDataForForceCalibratingMode(LibertyTracker::ItemDataIterator iter, int numTrackerFrames, int numItemsPerTrackerFrame)
{
	for (int i = 0; i < numTrackerFrames; ++i)
	{
		RawSensorData rawSensorData = computeDescriptors_.trackerDataToRawSensorData(iter);

		// Wait for 'take sample' event and send to editor to handle if received:
		EditorEvent unused;
		if ((rawSensorData.stylusButtonPressed && !prevFrameStylusButtonPressed_) || 
			editorEventQueue_.get(unused) == 1)
		{
			LOG_INFO_N("violin_recording_plugin", "Point collected!");

			// Set point:
			forceCalibration_.setCurStepPoint(rawSensorData.stylusSensPos);

			// Increment step:
			forceCalibration_.incrStepIdx();
			const int calibrationStep = forceCalibration_.getStepIdx();

			// Check if calibration sequence is complete:
			if (calibrationStep == ForceCalibration::NUM_STEPS)
			{
				operationMode_ = NORMAL;

				endForceCalibrationSequenceReachedSignal.sendChangeMessage(NULL);
				// NOTE: signal editor thread because may display blocking warning dialog
			}
			// Else, print next step message (instructions):
			else
			{
				forceCalibration_.printCurStepMessage();
			}

			updateEntireEditorSignal.sendChangeMessage(NULL);
		}

		// Remember for next frame:
		prevFrameStylusButtonPressed_ = rawSensorData.stylusButtonPressed;

		// Advance to next frame:
		iter.advance(numItemsPerTrackerFrame);
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ViolinRecordingPlugInEffect::processTrackerDataForSampling3dModelMode(LibertyTracker::ItemDataIterator iter, int numTrackerFrames, int numItemsPerTrackerFrame)
{
	ViolinRecordingPlugInEditor *editor = (ViolinRecordingPlugInEditor *)getActiveEditor();

	for (int i = 0; i < numTrackerFrames; ++i)
	{
		RawSensorData rawSensorData = computeDescriptors_.trackerDataToRawSensorData(iter);

		// Wait for 'take sample' event and send to editor to handle if received:
		EditorEvent unused;
		if ((rawSensorData.stylusButtonPressed && !prevFrameStylusButtonPressed_) || 
			editorEventQueue_.get(unused) == 1)
		{
			LOG_INFO_N("violin_recording_plugin", "Point collected!");

			// Compute beta:
			Matrix3x1 beta = computeBeta(rawSensorData.stylusSensPos, rawSensorData.violinBodySensPos, rawSensorData.violinBodySensOrientation);

			// Set step data:
			violinModel3d_.getCurVertex() = beta;

			// Set step set flag:
			violinModel3d_.setCurStepSetFlag();

			// Increment step:
			if (violinModel3d_.getPathStep() == ViolinModel3d::BODY_OUTLINE_UPPER)
			{
				violinModel3d_.pushBackVertexStep();
			}
			else
			{
				if (violinModel3d_.getVertexStep() < violinModel3d_.getNumVertexSteps() - 1)
				{
					violinModel3d_.incrVertexStep();
				}
				else
				{
					violinModel3d_.incrPathStep();
				}
			}

			// Check if calibration sequence is complete:
			if (violinModel3d_.getPathStep() == ViolinModel3d::NUM_PATHS)
			{
				endSampling3dModelSequence();

				endSampling3dModelSequenceReachedSignal.sendChangeMessage(NULL);
				// NOTE: signal editor thread because may display blocking warning dialog
			}
			// Else, print next step message (instructions):
			else
			{
				violinModel3d_.printCurStepMessage();
			}

			updateEntireEditorSignal.sendChangeMessage(NULL);
		}

		// Remember for next frame:
		prevFrameStylusButtonPressed_ = rawSensorData.stylusButtonPressed;

		// Advance to next frame:
		iter.advance(numItemsPerTrackerFrame);
	}
}

// ---------------------------------------------------------------------------------------

AudioProcessorEditor *ViolinRecordingPlugInEffect::createEditor()
{
	// NOTE: do not store pointer created here internally use getActiveEditor() or createEditorIfNeeded()
	ViolinRecordingPlugInEditor *p = new ViolinRecordingPlugInEditor(this);
	if (useGagesEnabled_)
	{
		p->bowForcePlot_->addPlot(Colours::green); // 1
		p->bowForcePlot_->addPlot(Colours::blue); // 2

		p->noiseAndSyncPlot_->addPlot(Colours::blue); // 2
		// (always send to plot 2 regardless of isPlotSyncSigEnabled_ to allow changing while connected)
	}
	return static_cast<AudioProcessorEditor *>(p);
}

// ---------------------------------------------------------------------------------------

int ViolinRecordingPlugInEffect::getNumParameters()
{
    return 0; // not supported
}

float ViolinRecordingPlugInEffect::getParameter(int index)
{
	return 0.0f; // not supported
}

void ViolinRecordingPlugInEffect::setParameter(int index, float newValue)
{
	 // not supported
}

const String ViolinRecordingPlugInEffect::getParameterName(int index)
{
	return String::empty; // not supported
}

const String ViolinRecordingPlugInEffect::getParameterText(int index)
{
	return String::empty; // not supported
}

// ---------------------------------------------------------------------------------------

const String ViolinRecordingPlugInEffect::getInputChannelName(const int channelIndex) const
{
	return String(channelIndex + 1);
}

const String ViolinRecordingPlugInEffect::getOutputChannelName(const int channelIndex) const
{
	return String(channelIndex + 1);
}

bool ViolinRecordingPlugInEffect::isInputChannelStereoPair(int index) const
{
	return false;
}

bool ViolinRecordingPlugInEffect::isOutputChannelStereoPair(int index) const
{
	return false;
}

// ---------------------------------------------------------------------------------------

// helper function
void writeMatrix3x1ToXml(XmlElement &xmlState, String attributeBaseName, const Matrix3x1 &m)
{
	xmlState.setAttribute(attributeBaseName + T("00"), m(0, 0));
	xmlState.setAttribute(attributeBaseName + T("10"), m(1, 0));
	xmlState.setAttribute(attributeBaseName + T("20"), m(2, 0));
}

void ViolinRecordingPlugInEffect::getStateInformation(JUCE_NAMESPACE::MemoryBlock& destData)
{
	LOG_INFO_N("violin_recording_plugin", "Saving effect state...");

	XmlElement xmlState(T("ViolinRecordingPlugInEffectSettings"));

	xmlState.setAttribute(T("pluginVersion"), 2);

	// configuration:
	xmlState.setAttribute(T("isUseGagesEnabled"), useGagesEnabled_);
	xmlState.setAttribute(T("comPortIdx"), comPortIdx_);
	xmlState.setAttribute(T("isUseStereoEnabled"), useStereoEnabled_);
	xmlState.setAttribute(T("isAutoStringEnabled"), isAutoStringEnabled_);
	xmlState.setAttribute(T("useScene3dEnabled"), useScene3dEnabled_);
	xmlState.setAttribute(T("isManualSyncEnabled"), isManualSyncEnabled_);
	xmlState.setAttribute(T("isPlotSyncSigEnabled"), isPlotSyncSigEnabled_);
	xmlState.setAttribute(T("isContRetriggerEnabled"), isContRetriggerEnabled_);

	// angles calibration:
	xmlState.setAttribute(T("anglesCalibrationValid"), anglesCalibration_.isValid());
	xmlState.setAttribute(T("anglesHysteresisDegrees"), anglesCalibration_.getHysteresisDegrees());
	xmlState.setAttribute(T("ang43"), anglesCalibration_.getAng43());
	xmlState.setAttribute(T("ang32"), anglesCalibration_.getAng32());
	xmlState.setAttribute(T("ang21"), anglesCalibration_.getAng21());

	// sync offsets:
	xmlState.setAttribute(T("trackerToAudioSyncOffset"), trackerToAudioSyncOffset_);
	xmlState.setAttribute(T("arduinoToTrackerSyncOffset"), arduinoToTrackerSyncOffset_);

	// tracker calibration:
	xmlState.setAttribute(T("isCalibrated"), (trackerCalibrationState_ == TRACKER_CALIBRATED));

	writeMatrix3x1ToXml(xmlState, T("betaStr1Bridge"), trackerCalibration_.getBeta(TrackerCalibration::STR1_BRIDGE));
	writeMatrix3x1ToXml(xmlState, T("betaStr2Bridge"), trackerCalibration_.getBeta(TrackerCalibration::STR2_BRIDGE));
	writeMatrix3x1ToXml(xmlState, T("betaStr3Bridge"), trackerCalibration_.getBeta(TrackerCalibration::STR3_BRIDGE));
	writeMatrix3x1ToXml(xmlState, T("betaStr4Bridge"), trackerCalibration_.getBeta(TrackerCalibration::STR4_BRIDGE));

	writeMatrix3x1ToXml(xmlState, T("betaStr1Wood"), trackerCalibration_.getBeta(TrackerCalibration::STR1_WOOD));
	writeMatrix3x1ToXml(xmlState, T("betaStr2Wood"), trackerCalibration_.getBeta(TrackerCalibration::STR2_WOOD));
	writeMatrix3x1ToXml(xmlState, T("betaStr3Wood"), trackerCalibration_.getBeta(TrackerCalibration::STR3_WOOD));
	writeMatrix3x1ToXml(xmlState, T("betaStr4Wood"), trackerCalibration_.getBeta(TrackerCalibration::STR4_WOOD));

	writeMatrix3x1ToXml(xmlState, T("betaStr1Fb"), trackerCalibration_.getBeta(TrackerCalibration::STR1_FB));
	writeMatrix3x1ToXml(xmlState, T("betaStr2Fb"), trackerCalibration_.getBeta(TrackerCalibration::STR2_FB));
	writeMatrix3x1ToXml(xmlState, T("betaStr3Fb"), trackerCalibration_.getBeta(TrackerCalibration::STR3_FB));
	writeMatrix3x1ToXml(xmlState, T("betaStr4Fb"), trackerCalibration_.getBeta(TrackerCalibration::STR4_FB));

	writeMatrix3x1ToXml(xmlState, T("betaFrogLhs"), trackerCalibration_.getBeta(TrackerCalibration::BOW_FROG_LHS));
	writeMatrix3x1ToXml(xmlState, T("betaFrogRhs"), trackerCalibration_.getBeta(TrackerCalibration::BOW_FROG_RHS));

	writeMatrix3x1ToXml(xmlState, T("betaTipLhs"), trackerCalibration_.getBeta(TrackerCalibration::BOW_TIP_LHS));
	writeMatrix3x1ToXml(xmlState, T("betaTipRhs"), trackerCalibration_.getBeta(TrackerCalibration::BOW_TIP_RHS));

	// force calibration:
	xmlState.setAttribute(T("forceCalibPointsValid"), areAllForceCalibrationStepsSet());
	writeMatrix3x1ToXml(xmlState, T("floorOrigin"), forceCalibration_.getPoint(ForceCalibration::FLOOR_ORIGIN));
	writeMatrix3x1ToXml(xmlState, T("floorLeft"), forceCalibration_.getPoint(ForceCalibration::FLOOR_LEFT));
	writeMatrix3x1ToXml(xmlState, T("floorFwd"), forceCalibration_.getPoint(ForceCalibration::FLOOR_FWD));
	writeMatrix3x1ToXml(xmlState, T("virtStringLhs"), forceCalibration_.getPoint(ForceCalibration::STRING_LEFT));
	writeMatrix3x1ToXml(xmlState, T("virtStringRhs"), forceCalibration_.getPoint(ForceCalibration::STRING_RIGHT));

	// score list/output path:
	xmlState.setAttribute(T("scoreListFilename"), scoreListFilename_);
	xmlState.setAttribute(T("outputPath"), outputPath_);

	// cur phrase idx:
	xmlState.setAttribute(T("curPhraseIdx"), curPhraseIdx_);

	// camera position/orientation:
	ViolinRecordingPlugInEditor *editor = (ViolinRecordingPlugInEditor *)getActiveEditor();
	Camera camera;
	bool cameraValid = getLastKnownCamera(camera); // if no last camera, camera remains unchanged
	xmlState.setAttribute(T("cameraValid"), cameraValid);
	writeMatrix3x1ToXml(xmlState, T("cameraEye"), camera.getEye());
	writeMatrix3x1ToXml(xmlState, T("cameraCen"), camera.getCenter());
	writeMatrix3x1ToXml(xmlState, T("cameraUp"), camera.getUp());
	xmlState.setAttribute(T("cameraGravityEnabled"), camera.isGravityEnabled());

	// convert xml to binary chunk:
	copyXmlToBinary(xmlState, destData);
}

// helper function
void readMatrix3x1FromXml(XmlElement *xmlState, String attributeBaseName, Matrix3x1 &result)
{
	result(0, 0) = xmlState->getDoubleAttribute(attributeBaseName + T("00"), 0.0);
	result(1, 0) = xmlState->getDoubleAttribute(attributeBaseName + T("10"), 0.0);
	result(2, 0) = xmlState->getDoubleAttribute(attributeBaseName + T("20"), 0.0);
}

void ViolinRecordingPlugInEffect::setStateInformation(const void* data, int sizeInBytes)
{
	LOG_INFO_N("violin_recording_plugin", "Loading effect state...");

	XmlElement *xmlState = getXmlFromBinary(data, sizeInBytes);

    if (xmlState != NULL)
    {
        if (xmlState->hasTagName(T("ViolinRecordingPlugInEffectSettings")) && xmlState->getIntAttribute(T("pluginVersion"), 0) == 2)
        {
			// stop recording (if needed):
			stopRecording();

			// stop and disconnect tracker (if connected and/or started):
			disconnectTrackerSynch();

			// clear score list:
			scoreList_.clearItems();

			// update state from xml:
			bool useGagesEnabled = xmlState->getBoolAttribute(T("isUseGagesEnabled"), useGagesEnabled_);
			comPortIdx_= xmlState->getIntAttribute(T("comPortIdx"), 0);
			if (comPortIdx_ < 0 || comPortIdx_ >= ComPort::getNumComPorts())
			{
				comPortIdx_ = 0;
				useGagesEnabled = false;
			}
			bool useStereoEnabled = xmlState->getBoolAttribute(T("isUseStereoEnabled"), useStereoEnabled_);
			bool useAutoStringEnabled = xmlState->getBoolAttribute(T("isAutoStringEnabled"), isAutoStringEnabled_);
			bool useScene3dEnabled = xmlState->getBoolAttribute(T("useScene3dEnabled"), useScene3dEnabled_);
			bool isManualSyncEnabled = xmlState->getBoolAttribute(T("isManualSyncEnabled"), isManualSyncEnabled_);
			bool isPlotSyncSigEnabled = xmlState->getBoolAttribute(T("isPlotSyncSigEnabled"), isPlotSyncSigEnabled_);
			bool isContRetriggerEnabled = xmlState->getBoolAttribute(T("isContRetriggerEnabled"), isContRetriggerEnabled_);

			setUseGagesEnabled(useGagesEnabled);
			setUseStereoEnabled(useStereoEnabled);
			setAutoStringEnabled(useAutoStringEnabled);
			setUseScene3dEnabled(useScene3dEnabled);
			setManualSyncEnabled(isManualSyncEnabled);
			setPlotSyncSigEnabled(isPlotSyncSigEnabled);
			setContRetriggerEnabled(isContRetriggerEnabled);

			// angles calibration:
			bool anglesCalibrationValid = xmlState->getBoolAttribute(T("anglesCalibrationValid"), false);
			double hysteresisDegrees = xmlState->getDoubleAttribute(T("anglesHysteresisDegrees"), 0.0);
			double ang43 = xmlState->getDoubleAttribute(T("ang43"), 0.0);
			double ang32 = xmlState->getDoubleAttribute(T("ang32"), 0.0);
			double ang21 = xmlState->getDoubleAttribute(T("ang21"), 0.0);

			if (anglesCalibrationValid)
				anglesCalibration_.loadFromData(hysteresisDegrees, ang43, ang32, ang21);

			// sync offsets:
			trackerToAudioSyncOffset_ = xmlState->getIntAttribute(T("trackerToAudioSyncOffset"), trackerToAudioSyncOffset_);
			arduinoToTrackerSyncOffset_ = xmlState->getIntAttribute(T("arduinoToTrackerSyncOffset"), arduinoToTrackerSyncOffset_);

			// tracker calibration:
			bool isCalibrated = xmlState->getBoolAttribute(T("isCalibrated"), false);
			if (isCalibrated)
			{
				LOG_INFO_N("violin_recording_plugin", "Restoring calibration data...");
				readMatrix3x1FromXml(xmlState, T("betaStr1Bridge"), trackerCalibration_.getBetaForSetting(TrackerCalibration::STR1_BRIDGE));
				readMatrix3x1FromXml(xmlState, T("betaStr2Bridge"), trackerCalibration_.getBetaForSetting(TrackerCalibration::STR2_BRIDGE));
				readMatrix3x1FromXml(xmlState, T("betaStr3Bridge"), trackerCalibration_.getBetaForSetting(TrackerCalibration::STR3_BRIDGE));
				readMatrix3x1FromXml(xmlState, T("betaStr4Bridge"), trackerCalibration_.getBetaForSetting(TrackerCalibration::STR4_BRIDGE));

				readMatrix3x1FromXml(xmlState, T("betaStr1Wood"), trackerCalibration_.getBetaForSetting(TrackerCalibration::STR1_WOOD));
				readMatrix3x1FromXml(xmlState, T("betaStr2Wood"), trackerCalibration_.getBetaForSetting(TrackerCalibration::STR2_WOOD));
				readMatrix3x1FromXml(xmlState, T("betaStr3Wood"), trackerCalibration_.getBetaForSetting(TrackerCalibration::STR3_WOOD));
				readMatrix3x1FromXml(xmlState, T("betaStr4Wood"), trackerCalibration_.getBetaForSetting(TrackerCalibration::STR4_WOOD));

				readMatrix3x1FromXml(xmlState, T("betaStr1Fb"), trackerCalibration_.getBetaForSetting(TrackerCalibration::STR1_FB));
				readMatrix3x1FromXml(xmlState, T("betaStr2Fb"), trackerCalibration_.getBetaForSetting(TrackerCalibration::STR2_FB));
				readMatrix3x1FromXml(xmlState, T("betaStr3Fb"), trackerCalibration_.getBetaForSetting(TrackerCalibration::STR3_FB));
				readMatrix3x1FromXml(xmlState, T("betaStr4Fb"), trackerCalibration_.getBetaForSetting(TrackerCalibration::STR4_FB));

				readMatrix3x1FromXml(xmlState, T("betaFrogLhs"), trackerCalibration_.getBetaForSetting(TrackerCalibration::BOW_FROG_LHS));
				readMatrix3x1FromXml(xmlState, T("betaFrogRhs"), trackerCalibration_.getBetaForSetting(TrackerCalibration::BOW_FROG_RHS));

				readMatrix3x1FromXml(xmlState, T("betaTipLhs"), trackerCalibration_.getBetaForSetting(TrackerCalibration::BOW_TIP_LHS));
				readMatrix3x1FromXml(xmlState, T("betaTipRhs"), trackerCalibration_.getBetaForSetting(TrackerCalibration::BOW_TIP_RHS));

				trackerCalibrationState_ = TRACKER_CALIBRATED;

				LOG_INFO_N("violin_recording_plugin.filelog", (const char *)trackerCalibration_.getAsCsvString());

				hasTrackerCalibrationBeenModified_ = true;
			}
			else
			{
				LOG_INFO_N("violin_recording_plugin", "Can't restore calibration data because not yet calibrated.");
				trackerCalibrationState_ = TRACKER_NOT_CALIBRATED;
			}

			// force calibration:
			bool isForceCalibrated = xmlState->getBoolAttribute(T("forceCalibPointsValid"), false);
			if (isForceCalibrated)
			{
				readMatrix3x1FromXml(xmlState, T("floorOrigin"), forceCalibration_.getPointForSetting(ForceCalibration::FLOOR_ORIGIN));
				readMatrix3x1FromXml(xmlState, T("floorLeft"), forceCalibration_.getPointForSetting(ForceCalibration::FLOOR_LEFT));
				readMatrix3x1FromXml(xmlState, T("floorFwd"), forceCalibration_.getPointForSetting(ForceCalibration::FLOOR_FWD));
				readMatrix3x1FromXml(xmlState, T("virtStringLhs"), forceCalibration_.getPointForSetting(ForceCalibration::STRING_LEFT));
				readMatrix3x1FromXml(xmlState, T("virtStringRhs"), forceCalibration_.getPointForSetting(ForceCalibration::STRING_RIGHT));
			}

			// score list / output path / cur phrase idx:
			scoreListFilename_ = xmlState->getStringAttribute(T("scoreListFilename"), String::empty);
			outputPath_ = xmlState->getStringAttribute(T("outputPath"), String::empty);

			curPhraseIdx_ = xmlState->getIntAttribute(T("curPhraseIdx"), 0);

			// load score list (if set):
			if (scoreListFilename_ != String::empty)
			{
				if (!scoreList_.loadFromFile(scoreListFilename_))
				{
					scoreListFilename_ = String::empty; // invalid filename, reset
					LOG_INFO_N("violin_recording_plugin", formatStr("[r]WARNING: Score list %s failed to restore.", (const char *)scoreListFilename_));
				}
				else
				{
					LOG_INFO_N("violin_recording_plugin", formatStr("Score list %s restored successfully.", (const char *)scoreListFilename_));
				}
			}

			// check if output path is valid:
			if (outputPath_ != String::empty)
			{
				File f(outputPath_);
				if (!f.exists() || !f.isDirectory() || f.getBytesFreeOnVolume() == 0)
				{
					outputPath_ = String::empty; // invalid path, reset
					LOG_INFO_N("violin_recording_plugin", formatStr("[r]WARNING: Restored output path %s invalid.", (const char *)outputPath_));
				}
				else
				{
					LOG_INFO_N("violin_recording_plugin", formatStr("Restored output path %s valid.", (const char *)outputPath_));
				}
			}

			// check if cur. phrase index is valid:
			if (curPhraseIdx_ < 0 || curPhraseIdx_ >= scoreList_.getNumItems())
				curPhraseIdx_ = 0; // invalid index, reset

			// update take index cache:
			if (scoreListFilename_ != String::empty && outputPath_ != String::empty)
				scoreList_.updateAllTakeIndexesFromFiles(outputPath_);

			// if score list filename and output path are set and valid, set state to 
			// 'ready to record':
			if (scoreListFilename_ != String::empty && outputPath_ != String::empty)
				readyToRecord_.set(true);

			// camera:
			bool cameraValid = xmlState->getBoolAttribute(T("cameraValid"), false);
			if (cameraValid)
			{
				Matrix3x1 cameraEye;
				Matrix3x1 cameraCen;
				Matrix3x1 cameraUp;
				bool cameraGravityEnabled;

				readMatrix3x1FromXml(xmlState, T("cameraEye"), cameraEye);
				readMatrix3x1FromXml(xmlState, T("cameraCen"), cameraCen);
				readMatrix3x1FromXml(xmlState, T("cameraUp"), cameraUp);
				cameraGravityEnabled = xmlState->getBoolAttribute(T("cameraGravityEnabled"), true);

				Camera camera;
				camera.setupCamera(
					cameraEye(0, 0), cameraEye(1, 0), cameraEye(2, 0),
					cameraCen(0, 0), cameraCen(1, 0), cameraCen(2, 0),
					cameraUp(0, 0), cameraUp(1, 0), cameraUp(2, 0)
					);
				camera.setGravityEnabled(cameraGravityEnabled);

				setLastKnownCamera(camera);
			}

			// update editor:
            updateEntireEditorSignal.sendChangeMessage(NULL); // update entire editor
        }
		else
		{
			LOG_INFO_N("violin_recording_plugin", "[r]WARNING: Invalid data or chunk format revision from host, can't restore settings.");
		}

        delete xmlState;
    }
}

// ---------------------------------------------------------------------------------------

bool ViolinRecordingPlugInEffect::setScoreListFilenameAndLoadIfValid(String filename)
{
	bool ok = scoreList_.loadFromFile(filename);
	if (ok)
	{
		bool wasLogFileOpen = fileLogger_.isOpen();

		if (wasLogFileOpen)
		{
			// Construct new log filename, reflecting score list name:
			String pathTerminated;

			if (outputPath_ == String::empty)
				pathTerminated = T("");
			else
				pathTerminated = outputPath_ + T("\\");

			File f(filename);
			String logFilename = pathTerminated + f.getFileNameWithoutExtension() + T("-rec_log.txt");

			// If setting initial score list (no previous active score list), rename the 
			// current log file to reflect the score list filename:
			if (scoreListFilename_ == String::empty)
			{
				LOG_INFO_N("violin_recording_plugin", "Renaming log file...");
				fileLogger_.relocateFile((const char *)logFilename, 8);
			}
			// Else close current log file and open a new one with name reflecting 
			// score list filename:
			else
			{
				LOG_INFO_N("violin_recording_plugin", "Ending log due to score list change...");
				LOG_INFO_N("violin_recording_plugin.filelog", formatStr("DATE: %s.", getSystemDate().c_str()));
				LOG_INFO_N("violin_recording_plugin.filelog", formatStr("TIME: %s.", getSystemTime().c_str()));

				fileLogger_.close(); // flushes all messages

				fileLogger_.open((const char *)logFilename, false, 8);
				String prevLogFilename = pathTerminated + File(scoreListFilename_).getFileNameWithoutExtension() + T("-rec_log.txt");
				LOG_INFO_N("violin_recording_plugin", "Continuing session due to score list change...");
				LOG_INFO_N("violin_recording_plugin.filelog", formatStr("Previous log file: \"%s\".", getSystemName().c_str()));
				LOG_INFO_N("violin_recording_plugin.filelog", formatStr("COMPUTER: %s.", (const char *)prevLogFilename));
				LOG_INFO_N("violin_recording_plugin.filelog", formatStr("DATE: %s.", getSystemDate().c_str()));
				LOG_INFO_N("violin_recording_plugin.filelog", formatStr("TIME: %s.", getSystemTime().c_str()));
			}
		}

		// Set active score list filename:
		scoreListFilename_ = filename;

		// Update take indexes if output path was already set:
		if (outputPath_ != String::empty)
		{
			scoreList_.updateAllTakeIndexesFromFiles(outputPath_);
			curPhraseIdx_ = 0; // reset
		}

		// Change state to 'ready to record' if output path was already set:
		if (outputPath_ != String::empty)
		{
			readyToRecord_.set(true);
		}

		LOG_INFO_N("violin_recording_plugin", formatStr("Score list \"%s\" loaded successfully.", (const char *)filename));
	}
	else
	{
		LOG_INFO_N("violin_recording_plugin", formatStr("[r]WARNING: Score list \"%s\" failed to load.", (const char *)filename));
	}

	return ok;
}

void ViolinRecordingPlugInEffect::setOutputPath(String path)
{
	LOG_INFO_N("violin_recording_plugin", formatStr("Output path set to \"%s\".", (const char *)path));

	// (Re)open log file in the output path:
	bool wasLogFileOpen = fileLogger_.isOpen();

	String logFilename;
	if (scoreListFilename_ == String::empty)
	{
		logFilename = path + T("\\unknown_score-rec_log.txt");
	}
	else
	{
		File f(scoreListFilename_);
		logFilename = path + T("\\") + f.getFileNameWithoutExtension() + T("-rec_log.txt");
	}

	if (wasLogFileOpen)
	{
		LOG_INFO_N("violin_recording_plugin", "Re-locating log file...");
		fileLogger_.relocateFile((const char *)logFilename, 8);
	}
	else
	{
		fileLogger_.open((const char *)logFilename, false, 8);
	}

	// Store path and update take indexes:
	outputPath_ = path;
	scoreList_.updateAllTakeIndexesFromFiles(outputPath_);

	// Change state to 'ready to record' if score list filename already set:
	if (scoreListFilename_ != String::empty)
	{
		readyToRecord_.set(true);
	}
}

// ---------------------------------------------------------------------------------------

NotifyingLogAppender *ViolinRecordingPlugInEffect::getEventLog()
{
	return &eventLogAppender_;
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEffect::connectTrackerSynch()
{
	if (trackerState_ == TRACKER_CONNECTED)
		return;

	if (useGagesEnabled_)
	{
		LOG_INFO_N("violin_recording_plugin", formatStr("Opening COM port (%s)...", (const char *)ComPort::getPortName(comPortIdx_)));
		bool comPortOk = comPort_.open(comPortIdx_, ComPort::RATE_115200, ComPort::NO_PARITY);
		if (!comPortOk)
		{
			trackerState_ = TRACKER_FAILED;
			LOG_ERROR_N("violin_recording_plugin", "[r]ERROR: Failed to open COM port.");
			return;
		}
	}

	LOG_INFO_N("violin_recording_plugin", "Connecting tracker...");
	tracker_.connect();
	if (!tracker_.isOk())
	{
		if (useGagesEnabled_)
			comPort_.close();
		trackerState_ = TRACKER_FAILED;
		LOG_ERROR_N("violin_recording_plugin", "[r]ERROR: Failed to connect.");
		return;
	}

	LOG_INFO_N("violin_recording_plugin", "Configuring...");
	tracker_.configure();
	if (!tracker_.isOk())
	{
		if (useGagesEnabled_)
			comPort_.close();
		tracker_.disconnect();
		trackerState_ = TRACKER_FAILED;
		LOG_ERROR_N("violin_recording_plugin", "[r]ERROR: Failed to configure.");
		return;
	}

	if (tracker_.getNumEnabledSensors() != 3)
	{
		if (useGagesEnabled_)
			comPort_.close();
		tracker_.disconnect();
		trackerState_ = TRACKER_FAILED;
		LOG_ERROR_N("violin_recording_plugin", "[r]ERROR: Invalid number of active sensors.");
		return;
	}

	LOG_INFO_N("violin_recording_plugin", "Starting GUI timers...");
	ViolinRecordingPlugInEditor *editor = (ViolinRecordingPlugInEditor *)getActiveEditor();
	if (editor != NULL)
	{
		// Clear graph plotters:
		editor->bowDisplacementPlot_->clear();
		editor->bowForcePlot_->clear();
		editor->bowBridgeDistancePlot_->clear();
		editor->noiseAndSyncPlot_->clear();

		// Start graph plotters:
		editor->bowDisplacementPlot_->start(100, 240.0, 8.0);
		editor->bowForcePlot_->start(100, 240.0, 8.0);
		editor->bowBridgeDistancePlot_->start(100, 240.0, 8.0);
		editor->noiseAndSyncPlot_->start(100, 240.0, 8.0);

		// Start 3d scene timer:
		if (editor->isScene3dValid_.isSet())
			editor->scene3d_->start(60, 240.0f, 4.0f);
		else
			if (useScene3dEnabled_)
				LOG_INFO_N("violin_recording_plugin", "[r]Warning: No 3d scene component!");
	}
	else
	{
		LOG_INFO_N("violin_recording_plugin", "[r]Warning: No editor!");
	}

	LOG_INFO_N("violin_recording_plugin", "Ok!");
	trackerState_ = TRACKER_CONNECTED;
}

void ViolinRecordingPlugInEffect::disconnectTrackerSynch()
{
	if (trackerState_ == TRACKER_DISCONNECTED || trackerState_ == TRACKER_FAILED)
		return;

	// Wait to ensure processBlock() isn't using tracker_:
	if (!isProcessingSuspended_)
	{
		LOG_INFO_N("violin_recording_plugin", "Waiting for audio thread to finnish using tracker...");
		if (!pendingDisconnectCanComplete_.wait(1500))
			LOG_INFO_N("violin_recording_plugin", "[r]Warning: Waiting timed out!");
	}

	// Set tracker state:
	trackerState_ = TRACKER_DISCONNECTED;

	// Set operation mode (in case in the middle of calibrating, etc.):
	operationMode_ = NORMAL;

	// Reset event in case processBlock() signaled the event in between wait() and updating tracker state:
	pendingDisconnectCanComplete_.reset();

	ViolinRecordingPlugInEditor *editor = (ViolinRecordingPlugInEditor *)getActiveEditor();
	if (editor != NULL)
	{
		LOG_INFO_N("violin_recording_plugin", "Stopping GUI timers...");

		// Stop graph plotters:
		editor->bowDisplacementPlot_->stop();
		editor->bowForcePlot_->stop();
		editor->bowBridgeDistancePlot_->stop();
		editor->noiseAndSyncPlot_->stop();

		// Clear graph plotters:
		editor->bowDisplacementPlot_->clear();
		editor->bowForcePlot_->clear();
		editor->bowBridgeDistancePlot_->clear();
		editor->noiseAndSyncPlot_->clear();

		// Stop 3d scene timer:
		if (editor->isScene3dValid_.isSet())
			editor->scene3d_->stop();
	}

	// Disconnect tracker:
	LOG_INFO_N("violin_recording_plugin", "Stopping continuous data collection mode...");
	tracker_.stopReceivingEvents();

	LOG_INFO_N("violin_recording_plugin", "Disconnecting tracker...");
	tracker_.disconnect();

	// Close com port:
	if (useGagesEnabled_)
	{
		LOG_INFO_N("violin_recording_plugin", "Closing COM port...");
		comPort_.close();
	}
	
	LOG_INFO_N("violin_recording_plugin", "Ok!");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ViolinRecordingPlugInEffect::setUseGagesEnabled(bool enabled)
{
	if (enabled != useGagesEnabled_)
	{
		LOG_INFO_N("violin_recording_plugin.filelog", formatStr("Changing use gages mode to: %d...", (int)enabled));

		ViolinRecordingPlugInEditor *editor = (ViolinRecordingPlugInEditor *)getActiveEditor();
		if (editor != NULL)
		{
			if (!enabled)
			{
				editor->bowForcePlot_->removePlot(2);
				editor->bowForcePlot_->removePlot(1);

//				if (isPlotSyncSigEnabled_)
				editor->noiseAndSyncPlot_->removePlot(2);
			}
			else
			{
				editor->bowForcePlot_->addPlot(Colours::green); // 1
				editor->bowForcePlot_->addPlot(Colours::blue); // 2

//				if (isPlotSyncSigEnabled_)
				editor->noiseAndSyncPlot_->addPlot(Colours::blue); // 2
			}
		}
	}
	useGagesEnabled_ = enabled;
}

void ViolinRecordingPlugInEffect::setComPortIdx(int idx)
{
	if (idx != comPortIdx_)
	{
		LOG_INFO_N("violin_recording_plugin.filelog", formatStr("Changing gages serial comm. port to: %d...", idx));
	}
	comPortIdx_ = idx;
}

void ViolinRecordingPlugInEffect::setUseStereoEnabled(bool enabled)
{
	if (enabled != useStereoEnabled_)
	{
		LOG_INFO_N("violin_recording_plugin.filelog", formatStr("Changing use stereo mode to: %d...", (int)enabled));
	}
	useStereoEnabled_ = enabled;
}

void ViolinRecordingPlugInEffect::setAutoStringEnabled(bool enabled)
{
	if (enabled != isAutoStringEnabled_)
	{
		LOG_INFO_N("violin_recording_plugin.filelog", formatStr("Changing automatic string estimation mode to: %d...", (int)enabled));
	}
	isAutoStringEnabled_ = enabled;
}

void ViolinRecordingPlugInEffect::setUseScene3dEnabled(bool enabled)
{
	if (enabled != useScene3dEnabled_)
	{
		LOG_INFO_N("violin_recording_plugin.filelog", formatStr("Changing use 3d scene mode to: %d...", (int)enabled));
	}
	useScene3dEnabled_ = enabled;
}

void ViolinRecordingPlugInEffect::setManualSyncEnabled(bool enabled)
{
	if (enabled != isManualSyncEnabled_)
	{
		LOG_INFO_N("violin_recording_plugin.filelog", formatStr("Changing manual sync mode to: %d...", (int)enabled));
		syncOutGen_.resetEstimatedOffsets(); // so disabling manual sync doesn't overwrite manual sync values until 'trigger sync' button is pressed

		if (enabled == true)
		{
			setPlotSyncSigEnabled(false); // disable plot sync sig
			setContRetriggerEnabled(false); // disable continuous retrigger
		}
		else
		{
			syncRequiresRetrigger_ = true;
		}
	}
	isManualSyncEnabled_ = enabled;
}

void ViolinRecordingPlugInEffect::setPlotSyncSigEnabled(bool enabled)
{
	if (enabled != isPlotSyncSigEnabled_)
	{
		LOG_INFO_N("violin_recording_plugin.filelog", formatStr("Changing plot sync sig. mode to: %d...", (int)enabled));

		//if (useGagesEnabled_)
		//{
		//	ViolinRecordingPlugInEditor *editor = (ViolinRecordingPlugInEditor *)getActiveEditor();
		//	if (editor != NULL)
		//	{
		//		if (!enabled)
		//		{
		//			editor->noiseAndSyncPlot_->removePlot(2);
		//		}
		//		else
		//		{
		//			editor->noiseAndSyncPlot_->addPlot(Colours::blue); // 2
		//		}
		//	}
		//}
	}
	isPlotSyncSigEnabled_ = enabled;
}

void ViolinRecordingPlugInEffect::setContRetriggerEnabled(bool enabled)
{
	if (enabled != isContRetriggerEnabled_)
	{
		LOG_INFO_N("violin_recording_plugin.filelog", formatStr("Changing continuous sync retrigger mode to: %d...", (int)enabled));
		syncOutGen_.setContRetrigger(enabled);
	}
	isContRetriggerEnabled_ = enabled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ViolinRecordingPlugInEffect::connectTrackerAsynch()
{
	assert(trackerState_ != TRACKER_PENDING_CONNECT && trackerState_ != TRACKER_PENDING_DISCONNECT);

	if (trackerState_ == TRACKER_CONNECTED)
		return;

	if (trackerThread_ == NULL)
		return;

	LOG_INFO_N("violin_recording_plugin", "Connecting...");

#if (USE_ASYNCH_TRACKER_CONNECT_DISCONNECT != 0)
	trackerState_ = TRACKER_PENDING_CONNECT;
	updateEntireEditorSignal.sendSynchronousChangeMessage(NULL); // assume called from gui thread
	trackerThread_->setTask(TrackerThread::CONNECT_TRACKER);
	trackerThread_->startThread(4);
#else
	connectTrackerSynch();
	updateEntireEditorSignal.sendChangeMessage(NULL);
#endif
}

void ViolinRecordingPlugInEffect::disconnectTrackerAsynch()
{
	assert(trackerState_ != TRACKER_PENDING_CONNECT && trackerState_ != TRACKER_PENDING_DISCONNECT);

	if (trackerState_ == TRACKER_DISCONNECTED || trackerState_ == TRACKER_FAILED)
		return;

	if (trackerThread_ == NULL)
		return;

	LOG_INFO_N("violin_recording_plugin", "Disconnecting...");

#if (USE_ASYNCH_TRACKER_CONNECT_DISCONNECT != 0)
	trackerState_ = TRACKER_PENDING_DISCONNECT;
	updateEntireEditorSignal.sendSynchronousChangeMessage(NULL); // assume called from gui thread
	trackerThread_->setTask(TrackerThread::DISCONNECT_TRACKER);
	trackerThread_->startThread(4);
#else
	disconnectTrackerSynch();
	updateEntireEditorSignal.sendChangeMessage(NULL);
#endif
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEffect::startTrackerCalibrationSequence()
{
	// Change operation mode:
	operationMode_ = CALIBRATING_TRACKER;
	if (trackerCalibrationState_ == TRACKER_CALIBRATED)
		Thread::sleep(100); // wait for processBlock() to enter CALIBRATING mode (stop using trackerCalibration_, etc.)

	// Remember current calibration (if any) to show while doing new calibration:
	if (trackerCalibrationState_ == TRACKER_CALIBRATED)
	{
		trackerCalibrationPrev_ = trackerCalibration_;
		for (int i = 0; i < TrackerCalibration::NUM_STEPS; ++i)
			trackerCalibrationPrev_.setStepSetFlag(i);
	}

	// Reset current calibration:
	trackerCalibration_.resetSetFlags();
	trackerCalibration_.resetCalibrationStep();
	lastCalibrationPointSampleTime_ = 0;

	// Show instructions:
	LOG_INFO_N("violin_recording_plugin", "Starting calibration sequence...");
	trackerCalibration_.printCalibrationStepMessage(); // first step
}

void ViolinRecordingPlugInEffect::endTrackerCalibrationSequence(bool successful)
{
	operationMode_ = NORMAL;
	if (successful)
	{
		hasTrackerCalibrationBeenModified_ = true;
		trackerCalibrationState_ = TRACKER_CALIBRATED;
	}
	else
	{
		if (trackerCalibrationState_ == TRACKER_NOT_CALIBRATED)
			hasTrackerCalibrationBeenModified_ = false;
		else
			hasTrackerCalibrationBeenModified_ = true;
		trackerCalibrationState_ = TRACKER_NOT_CALIBRATED;
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool ViolinRecordingPlugInEffect::loadTrackerCalibrationFile(const String &filename)
{
	bool ok = trackerCalibration_.loadFromFile(filename);
	
	if (ok)
	{
		LOG_INFO_N("violin_recording_plugin", formatStr("Calibration file %s loaded correctly.", (const char *)filename));
		trackerCalibrationState_ = TRACKER_CALIBRATED;
		hasTrackerCalibrationBeenModified_ = false;
	}
	else
	{
		LOG_INFO_N("violin_recording_plugin", formatStr("[r]WARNING: Calibration file %s failed to load.", (const char *)filename));
		trackerCalibrationState_ = TRACKER_NOT_CALIBRATED;
		// (hasCalibrationBeenModified_ remains true if was true)
	}

	return ok;
}

bool ViolinRecordingPlugInEffect::saveTrackerCalibrationFile(String filename)
{
	bool result = trackerCalibration_.saveToFile(filename, false);
	if (result)
	{
		hasTrackerCalibrationBeenModified_ = false;
		updateEntireEditorSignal.sendChangeMessage(NULL);
	}
	return result;
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEffect::startRecording(const AudioPlayHead::CurrentPositionInfo &pos, int curAudioBufferSize, int curTrackerBufferSize)
{
	// Compute onset previous bar:
	const double tempoBpqn = pos.bpm;
	const double posCursorAtOnsetCurBufferQn = pos.ppqPosition;
	const double onsetPrevBarQn = pos.ppqPositionOfLastBarStart;
	const double onsetNextBarQn = pos.ppqPositionOfLastBarStart + 4.0*(double)pos.timeSigNumerator/(double)pos.timeSigDenominator;

	const double offsetCursorPrevBarSeconds = (posCursorAtOnsetCurBufferQn - onsetPrevBarQn)/tempoBpqn*60.0; // positive distance
	// Note: Assuming tempo stayed constant between onset previous bar and cursor position.

	const int offsetCurCursorPrevBarAudioSamples = round_int(offsetCursorPrevBarSeconds*getSampleRate());
	const int offsetCurWriteIdxPrevBarAudioSamples = offsetCurCursorPrevBarAudioSamples + curAudioBufferSize; // have already written current buffer (positive distance)

	assert((int)audioCh1Writer_->getUnwrappedWriteIdxFrames() >= offsetCurWriteIdxPrevBarAudioSamples);
	const int rewindIdxAudioSamples = audioCh1Writer_->getUnwrappedWriteIdxFrames() - offsetCurWriteIdxPrevBarAudioSamples;

	const int rewindIdxTrackerSamples = round_int(rewindIdxAudioSamples/getSampleRate()*tracker_.getSampleRate());// - curUsedTrackerToAudioSyncOffset_;
	const int offsetCurWriteIdxPrevBarTrackerSamples = trackerWriter_->getUnwrappedWriteIdxFrames() - rewindIdxTrackerSamples;

	const int rewindIdxArduinoSamples = round_int(rewindIdxAudioSamples/getSampleRate()*tracker_.getSampleRate());// - curUsedArduinoToAudioSyncOffset_;
	const int offsetCurWriteIdxPrevBarArduinoSamples = arduinoWriter_->getUnwrappedWriteIdxFrames() - rewindIdxArduinoSamples;

	// Log msg:
	LOG_INFO_N("violin_recording_plugin", formatStr("Starting recording (%s)...", getSystemTime().c_str()));
	LOG_INFO_N("violin_recording_plugin", formatStr("Rewinding audio to previous bar: %.2f s...", offsetCurWriteIdxPrevBarAudioSamples/getSampleRate()));
	LOG_INFO_N("violin_recording_plugin", formatStr("Rewinding tracker to previous bar: %.2f s...", offsetCurWriteIdxPrevBarTrackerSamples/tracker_.getSampleRate()));
	if (useGagesEnabled_)
		LOG_INFO_N("violin_recording_plugin", formatStr("Rewinding arduino to previous bar: %.2f s...", offsetCurWriteIdxPrevBarArduinoSamples/tracker_.getSampleRate()));
	String baseFilename = scoreList_.getFilename(curPhraseIdx_, getCurPhraseTakeIdx(), outputPath_, "*", ".*").c_str();
	LOG_INFO_N("violin_recording_plugin.filelog", formatStr("Files: \"%s\".", (const char *)baseFilename));

	// Additional debug logging if one of the rewinds is negative (shouldn't happen normally):
	// Note: offsetCurWriteIdxPrevBarAudioSamples should NEVER be negative
	if (offsetCurWriteIdxPrevBarTrackerSamples < 0)
	{
		LOG_INFO_N("violin_recording_plugin", "[r]ERROR: Tracker rewind size is negative (streams not properly synchronized perhaps?):");
		// XXX: log audio unwrapped write idx (audio frames)
		// XXX: lgo tracker to audio offset (in tracker frames)
		LOG_INFO_N("violin_recording_plugin", formatStr("[r]   Audio rewind idx: %d.", rewindIdxAudioSamples));
		LOG_INFO_N("violin_recording_plugin", formatStr("[r]   Tracker unwrapped write idx: %u.", trackerWriter_->getUnwrappedWriteIdxFrames()));
		LOG_INFO_N("violin_recording_plugin", formatStr("[r]   Tracker rewind offset (negative means future): %d.", curUsedTrackerToAudioSyncOffset_));
	}

	if (useGagesEnabled_ && offsetCurWriteIdxPrevBarArduinoSamples < 0)
	{
		LOG_INFO_N("violin_recording_plugin", "[r]ERROR: Arduino rewind size is negative (streams not properly synchronized perhaps?):");
		// XXX: log audio unwrapped write idx (audio frames)
		// XXX: lgo tracker to audio offset (in tracker frames)
		LOG_INFO_N("violin_recording_plugin", formatStr("[r]   Audio rewind idx: %d.", rewindIdxAudioSamples));
		LOG_INFO_N("violin_recording_plugin", formatStr("[r]   Arduino unwrapped write idx: %u.", arduinoWriter_->getUnwrappedWriteIdxFrames()));
		LOG_INFO_N("violin_recording_plugin", formatStr("[r]   Arduino rewind offset (negative means future): %d.", curUsedArduinoToAudioSyncOffset_));
	}

	// Start audio recording:
	String audioCh1Filename = scoreList_.getFilename(curPhraseIdx_, getCurPhraseTakeIdx(), outputPath_, "ch1", ".wav").c_str();
	audioCh1Writer_->postStartDiskWriteEvent((const char *)audioCh1Filename, offsetCurWriteIdxPrevBarAudioSamples);

	if (useStereoEnabled_)
	{
		String audioCh2Filename = scoreList_.getFilename(curPhraseIdx_, getCurPhraseTakeIdx(), outputPath_, "ch2", ".wav").c_str();
		audioCh2Writer_->postStartDiskWriteEvent((const char *)audioCh2Filename, offsetCurWriteIdxPrevBarAudioSamples);
	}

	// Start tracker recording:
	String trackerFilename = scoreList_.getFilename(curPhraseIdx_, getCurPhraseTakeIdx(), outputPath_, "tracker", ".dat").c_str();
	trackerWriter_->postStartDiskWriteEvent((const char *)trackerFilename, offsetCurWriteIdxPrevBarTrackerSamples);

	// Start arduino recording:
	if (useGagesEnabled_)
	{
		String arduinoFilename = scoreList_.getFilename(curPhraseIdx_, getCurPhraseTakeIdx(), outputPath_, "arduino", ".dat").c_str();
		arduinoWriter_->postStartDiskWriteEvent((const char *)arduinoFilename, offsetCurWriteIdxPrevBarArduinoSamples);
	}

	// Write header file (raw binary):
	// NOTE: This file is written synchronously (to reduce code size), but 
	// it is only few data.
	String headerFilename = scoreList_.getFilename(curPhraseIdx_, getCurPhraseTakeIdx(), outputPath_, "header", ".dat").c_str();
	if (!writeHeaderFile(headerFilename,
		trackerCalibration_.getBeta(TrackerCalibration::STR1_BRIDGE).getPtr(),
		trackerCalibration_.getBeta(TrackerCalibration::STR2_BRIDGE).getPtr(), 
		trackerCalibration_.getBeta(TrackerCalibration::STR3_BRIDGE).getPtr(), 
		trackerCalibration_.getBeta(TrackerCalibration::STR4_BRIDGE).getPtr(), 
		trackerCalibration_.getBeta(TrackerCalibration::STR1_WOOD).getPtr(), 
		trackerCalibration_.getBeta(TrackerCalibration::STR2_WOOD).getPtr(), 
		trackerCalibration_.getBeta(TrackerCalibration::STR3_WOOD).getPtr(), 
		trackerCalibration_.getBeta(TrackerCalibration::STR4_WOOD).getPtr(), 
		trackerCalibration_.getBeta(TrackerCalibration::STR1_FB).getPtr(), 
		trackerCalibration_.getBeta(TrackerCalibration::STR2_FB).getPtr(), 
		trackerCalibration_.getBeta(TrackerCalibration::STR3_FB).getPtr(), 
		trackerCalibration_.getBeta(TrackerCalibration::STR4_FB).getPtr(), 
		trackerCalibration_.getBeta(TrackerCalibration::BOW_FROG_LHS).getPtr(), 
		trackerCalibration_.getBeta(TrackerCalibration::BOW_FROG_RHS).getPtr(), 
		trackerCalibration_.getBeta(TrackerCalibration::BOW_TIP_LHS).getPtr(), 
		trackerCalibration_.getBeta(TrackerCalibration::BOW_TIP_RHS).getPtr()))
	{
		LOG_ERROR_N("violin_recording_plugin", "[r]ERROR: Failed writing recording header file!");
	}
	String metronomeFilename = scoreList_.getFilename(curPhraseIdx_, getCurPhraseTakeIdx(), outputPath_, "metronome", ".txt").c_str();
	if (!writeMetronomeFile(metronomeFilename, pos.bpm, pos.timeSigNumerator, pos.timeSigDenominator))
	{
		LOG_ERROR_N("violin_recording_plugin", "[r]ERROR: Failed writing recording metronome file!");
	}

	// Change state and update editor:
	recordingState_ = RECORDING;
	updateEntireEditorSignal.sendChangeMessage(NULL); // asynchronous
}

void ViolinRecordingPlugInEffect::stopRecording()
{
	if (recordingState_ == NOT_RECORDING)
		return;

	// Log msg:
	LOG_INFO_N("violin_recording_plugin", formatStr("Stopping recording (%s)...", getSystemTime().c_str()));

	// Stop audio/tracker/arduino recording:
	audioCh1Writer_->postStopDiskWriteEvent();
	if (useStereoEnabled_)
		audioCh2Writer_->postStopDiskWriteEvent();
	trackerWriter_->postStopDiskWriteEvent();
	if (useGagesEnabled_)
		arduinoWriter_->postStopDiskWriteEvent();

	// Update recording state
	recordingState_ = NOT_RECORDING;

	// Next recording will require manual sync re-trigger:
	syncRequiresRetrigger_ = true;

	// Send signal to editor to display a dialog which asks user whether to 
	// redo current take or advance to next take:
	// Note: Incrementing take index of current (just recorded) phrase is also 
	// done inside the editor function.
	endRecordingPhraseSignal.sendChangeMessage(NULL); // also updates editor
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEffect::setAudioCh1BufferFullnessMeter(class HorizontalBarMeter *meter)
{
	if (audioCh1Writer_ != NULL)
		audioCh1Writer_->setFullnessMeter(meter);
}

void ViolinRecordingPlugInEffect::setAudioCh2BufferFullnessMeter(class HorizontalBarMeter *meter)
{
	if (audioCh2Writer_ != NULL)
		audioCh2Writer_->setFullnessMeter(meter);
}

void ViolinRecordingPlugInEffect::setTrackerBufferFullnessMeter(class HorizontalBarMeter *meter)
{
	if (trackerWriter_ != NULL)
		trackerWriter_->setFullnessMeter(meter);
}

void ViolinRecordingPlugInEffect::setArduinoBufferFullnessMeter(class HorizontalBarMeter *meter)
{
	if (arduinoWriter_ != NULL)
		arduinoWriter_->setFullnessMeter(meter);
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEffect::initAsynchDiskWriters()
{
	audioCh1Writer_ = NULL;
	audioCh2Writer_ = NULL;
	trackerWriter_ = NULL;
	arduinoWriter_ = NULL;
}

void ViolinRecordingPlugInEffect::createAsynchDiskWriters(double sampleRate)
{
	// Destroy existing disk writers (normally not needed):
	destroyAsynchDiskWriters();

	// Allocate new disk writers:
	minAllowedTempo_ = 30.0; // qn per minute
	const double maxSecondsPerQn = 1.0/minAllowedTempo_ * 60.0f;
	const double maxSecondsPerBar = 4.0*maxSecondsPerQn;

	audioCh1Writer_ = new AsynchFileWriter();
	audioCh1Writer_->setFileWriter(WaveFileWriter(1, sampleRate));
	audioCh1Writer_->allocate(1000, sampleRate, 2.0, ceil_int(maxSecondsPerBar*sampleRate));
	audioCh1Writer_->startConsumerThread();

	audioCh2Writer_ = new AsynchFileWriter();
	audioCh2Writer_->setFileWriter(WaveFileWriter(1, sampleRate));
	audioCh2Writer_->allocate(1000, sampleRate, 2.0, ceil_int(maxSecondsPerBar*sampleRate));
	audioCh2Writer_->startConsumerThread();

	const double trackerSampleRate = tracker_.getSampleRate();

	trackerWriter_ = new AsynchFileWriter();
	trackerWriter_->setFileWriter(DatFileWriter(numItemsPerFramePolhemus, trackerSampleRate, 1));
	trackerWriter_->allocate(1000, numItemsPerFramePolhemus*trackerSampleRate, 2.0, ceil_int(numItemsPerFramePolhemus*maxSecondsPerBar*trackerSampleRate));
	trackerWriter_->startConsumerThread();

	arduinoWriter_ = new AsynchFileWriter();
	arduinoWriter_->setFileWriter(DatFileWriter(numItemsPerFrameArduino, trackerSampleRate, 1));
	arduinoWriter_->allocate(1000, numItemsPerFrameArduino*trackerSampleRate, 2.0, ceil_int(numItemsPerFrameArduino*maxSecondsPerBar*trackerSampleRate));
	arduinoWriter_->startConsumerThread();
}

void ViolinRecordingPlugInEffect::destroyAsynchDiskWriters()
{
	if (audioCh1Writer_ != NULL)
	{
		audioCh1Writer_->stopConsumerThread(); // (blocking)
		delete audioCh1Writer_;
		audioCh1Writer_ = NULL;
	}

	if (audioCh2Writer_ != NULL)
	{
		audioCh2Writer_->stopConsumerThread(); // (blocking)
		delete audioCh2Writer_;
		audioCh2Writer_ = NULL;
	}

	if (trackerWriter_ != NULL)
	{
		trackerWriter_->stopConsumerThread(); // (blocking)
		delete trackerWriter_;
		trackerWriter_ = NULL;
	}

	if (arduinoWriter_ != NULL)
	{
		arduinoWriter_->stopConsumerThread(); // (blocking)
		delete arduinoWriter_;
		arduinoWriter_ = NULL;
	}
}

void ViolinRecordingPlugInEffect::resetAsynchDiskWriters()
{
	audioCh1Writer_->clearDataAndResetUnwrappedWriteIdxCount();
	audioCh2Writer_->clearDataAndResetUnwrappedWriteIdxCount();
	trackerWriter_->clearDataAndResetUnwrappedWriteIdxCount();
	arduinoWriter_->clearDataAndResetUnwrappedWriteIdxCount();
}

// ---------------------------------------------------------------------------------------

bool ViolinRecordingPlugInEffect::getLastKnownCamera(Camera &targetCamera)
{
	ViolinRecordingPlugInEditor *editor = (ViolinRecordingPlugInEditor *)getActiveEditor();

	if (editor != NULL && editor->scene3d_ != NULL)
	{
		targetCamera = editor->scene3d_->getCamera();
		return true;
	}
	else if (hasValidLastCamera_)
	{
		targetCamera = lastCamera_;
		return true;
	}
	else
	{
		return false;
	}
}

void ViolinRecordingPlugInEffect::setLastKnownCamera(const Camera &camera)
{
	lastCamera_ = camera;
	hasValidLastCamera_ = true;

	ViolinRecordingPlugInEditor *editor = (ViolinRecordingPlugInEditor *)getActiveEditor();
	if (editor != NULL && editor->scene3d_ != NULL)
	{
		editor->scene3d_->setCamera(camera); // XXX: might not be thread-safe (but probably not very harmful
	}
}




