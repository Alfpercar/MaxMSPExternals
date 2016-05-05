#ifndef INCLUDED_VIOLINRECORDINGPLUGINEFFECT_HXX
#define INCLUDED_VIOLINRECORDINGPLUGINEFFECT_HXX

#include "ViolinRecordingPlugInConfig.hxx"

#include "LibertyTracker.hxx"
#include <vector>
#include <string>

#include "juce.h"

#include "TrackerCalibration.hxx"
#include "ForceCalibration.hxx"
#include "concat/FileFormats/ScoreList.hxx"
#include "NotifyingLogAppender.hxx"
#include "AsynchFileLogger.hxx"
#include "LockFreeFifo.hxx"
#include "ComputeDescriptors.hxx"
#include "Interpolation.hxx"
#include "AtomicFlag.hxx"
#include "ComPort.hxx"
#include "ViolinModel3d.hxx"
#include "AtomicEnum.hxx"
#include "concat/Utilities/StdInt.hxx"
#include "CalibrationAngles.hxx"
#include "FixedFreqTone.hxx"
#include "SyncOutGenerator.hxx"
#include "ArduinoFrame.hxx"
#include "AtomicInt.hxx"

#include "Camera.hxx"

class ViolinRecordingPlugInEditor;
class GraphPlotter;

// ---------------------------------------------------------------------------------------

class ViolinRecordingPlugInEffect : public AudioProcessor
{
public:
    ViolinRecordingPlugInEffect();
    ~ViolinRecordingPlugInEffect();

	const String getName() const
	{
		return T("ViolinRecordingPlugIn");
	}

	// Processing:
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();

	void processBlock(AudioSampleBuffer &audioBuffer, MidiBuffer &midiMessages);

	// Creation:
    AudioProcessorEditor *createEditor();

	// Parameters:
	int getNumParameters();
	
	float getParameter(int index);
	void setParameter(int index, float newValue);

	const String getParameterName(int index);
	const String getParameterText(int index);

	// IO configuration:
	const String getInputChannelName(const int channelIndex) const;
	const String getOutputChannelName(const int channelIndex) const;
	bool isInputChannelStereoPair(int index) const;
	bool isOutputChannelStereoPair(int index) const;

	bool acceptsMidi() const { return false; }
	bool producesMidi() const { return false; }

	// Programs:
	int getNumPrograms() { return 0; }
    int getCurrentProgram() { return 0; }
    void setCurrentProgram(int index) { }
    const String getProgramName(int index) { return String::empty; }
    void changeProgramName(int index, const String& newName) { }

	// Persistence:
    void getStateInformation(JUCE_NAMESPACE::MemoryBlock& destData);
    void setStateInformation(const void* data, int sizeInBytes);

	juce_UseDebuggingNewOperator

public:
	enum TrackerState
	{
		TRACKER_DISCONNECTED,		// disconnected (initial state)
		TRACKER_PENDING_CONNECT,	// in the process of doing an asynchronous connect (audio thread waits until TRACKER_CONNECTED)
		TRACKER_CONNECTED,			// connected
		TRACKER_FAILED,				// connecting failed (disconnected)
		TRACKER_PENDING_DISCONNECT	// in the process of doing an asynchronous disconnect (audio thread may still use data, but next callback won't)
	};

	enum TrackerCalibrationState
	{
		TRACKER_NOT_CALIBRATED, // tracker not calibrated (initial state)
		TRACKER_CALIBRATED // calibration completed or loaded
	};

	enum RecordingState
	{
		NOT_RECORDING,
		RECORDING
	};

	enum OperationMode
	{
		NORMAL,
		CALIBRATING_TRACKER,
		CALIBRATING_FORCE,
		CALIBRATING_FORCE_POINTS,
		SAMPLING_3D_MODEL
	};

	// Query config:
	bool isUseGagesEnabled() const { return useGagesEnabled_; }
	int getGagesComPort() const { return comPortIdx_; }
	bool isUseStereoEnabled() const { return useStereoEnabled_; }
	bool isAutoStringEnabled() const { return isAutoStringEnabled_; }
	bool isUseScene3dEnabled() const { return useScene3dEnabled_; }
	bool isManualSyncEnabled() const { return isManualSyncEnabled_; }
	bool isPlotSyncSigEnabled() const { return isPlotSyncSigEnabled_; }
	bool isContRetriggerEnabled() const { return isContRetriggerEnabled_; }

	// Query state:
	TrackerState getTrackerState() const { return trackerState_; }
	bool isTrackerBusy() const { return (trackerState_ == TRACKER_PENDING_CONNECT || trackerState_ == TRACKER_PENDING_DISCONNECT); }
	TrackerCalibrationState getTrackerCalibrationState() const { return trackerCalibrationState_; }
	RecordingState getRecordingState() const { return recordingState_; }
	OperationMode getOperationMode() const { return operationMode_; }

	const String &getScoreListFilename() const { return scoreListFilename_; }
	const String &getOutputPath() const { return outputPath_; }

	int getCurPhraseIdx() const { return curPhraseIdx_; }
	String getCurPhraseName() const { return scoreList_.getItemAsName(curPhraseIdx_).c_str(); }
	int getNumPhrases() const { return scoreList_.getNumItems(); }

	int getCurPhraseTakeIdx() const { return scoreList_.getTakeIndex(curPhraseIdx_); }
	bool incrCurPhraseTakeIdx() { return scoreList_.increaseTakeIndex(curPhraseIdx_); }

	int getTrackerToAudioSyncOffset() const { return trackerToAudioSyncOffset_; }
	int getArduinoToTrackerSyncOffset() const { return arduinoToTrackerSyncOffset_; }

	CalibrationAngles &getAnglesCalibration() { return anglesCalibration_; }

	// Functions:
	void setUseGagesEnabled(bool enabled);
	void setComPortIdx(int idx);
	void setUseStereoEnabled(bool enabled);
	void setAutoStringEnabled(bool enabled);
	void setUseScene3dEnabled(bool enabled);
	void setManualSyncEnabled(bool enabled);
	void setPlotSyncSigEnabled(bool enabled);
	void setContRetriggerEnabled(bool enabled);

	void connectTrackerAsynch();
	void disconnectTrackerAsynch();

	void startTrackerCalibrationSequence();
	void endTrackerCalibrationSequence(bool successful);
	void postTakeTrackerCalibrationSampleEvent() { EditorEvent e; e.type = EditorEvent::TAKE_TRACKER_CALIBRATION_SAMPLE; editorEventQueue_.put(e); }
	void prevTrackerCalibrationStep() { trackerCalibration_.decrCalibrationStep(); trackerCalibration_.printCalibrationStepMessage(); }
	void nextTrackerCalibrationStep() { trackerCalibration_.incrCalibrationStep(); trackerCalibration_.printCalibrationStepMessage(); }
	int getTrackerCalibrationStep() const { return trackerCalibration_.getCalibrationStep(); }
	bool areAllTrackerCalibrationStepsSet() const { return trackerCalibration_.areAllStepFlagsSet(); }
	bool loadTrackerCalibrationFile(const String &filename);
	bool saveTrackerCalibrationFile(String filename);
	bool hasTrackerCalibrationBeenModified() const { return hasTrackerCalibrationBeenModified_; }

	void startForceCalibration()
	{
		operationMode_ = CALIBRATING_FORCE;
		LOG_INFO_N("violin_recording_plugin", "Starting force calibration...");
	}
	void endForceCalibration()
	{
		LOG_INFO_N("violin_recording_plugin", "Ending force calibration...");
		operationMode_ = NORMAL;
	}
	void startForceCalibTakePointsSeq()
	{
		operationMode_ = CALIBRATING_FORCE_POINTS;
		forceCalibration_.reset();
		LOG_INFO_N("violin_recording_plugin", "Starting force calibration point sampling sequence...");
		forceCalibration_.printCurStepMessage(); // first step
	}
	void endForceCalibTakePointsSeq()
	{
		LOG_INFO_N("violin_recording_plugin", "Ending force calibration point sampling sequence...");
		operationMode_ = NORMAL;
	}
	void postTakeForceCalibrationSampleEvent() { EditorEvent e; e.type = EditorEvent::TAKE_FORCE_CALIBRATION_SAMPLE; editorEventQueue_.put(e); }
	void prevForceCalibrationStep() { forceCalibration_.decrStepIdx(); forceCalibration_.printCurStepMessage(); }
	void nextForceCalibrationStep() { forceCalibration_.incrStepIdx(); forceCalibration_.printCurStepMessage(); }
	int getForceCalibrationStep() const { return forceCalibration_.getStepIdx(); }
	void saveForceCalibrationFile(String filename) { forceCalibration_.saveToFile(filename); }
	bool areAllForceCalibrationStepsSet() const { return forceCalibration_.areAllStepsSet(); }
	bool loadForceCalibrationPointsFile(String filename) { return forceCalibration_.loadFromFile(filename); }

	ViolinModel3d &getViolinModel3d() { return violinModel3d_; }
	void startSampling3dModelSequence()
	{
		violinModel3d_.reset();
		operationMode_ = SAMPLING_3D_MODEL;
		LOG_INFO_N("violin_recording_plugin", "Starting 3d model sampling sequence...");
		violinModel3d_.printCurStepMessage(); // first step
	}
	void endSampling3dModelSequence()
	{
		operationMode_ = NORMAL;
	}
	void prevSampling3dModelStep()
	{
		if (violinModel3d_.getPathStep() == ViolinModel3d::BODY_OUTLINE_UPPER)
			violinModel3d_.popBackVertexStep();
		violinModel3d_.decrPathStep();
		violinModel3d_.printCurStepMessage();
	}
	void nextSampling3dModelStep()
	{
		if (violinModel3d_.getPathStep() == ViolinModel3d::BODY_OUTLINE_UPPER)
			violinModel3d_.popBackVertexStep();
		violinModel3d_.incrPathStep();
		violinModel3d_.printCurStepMessage();
	}

	bool setScoreListFilenameAndLoadIfValid(String filename);
	void setOutputPath(String path);

	bool setScoreListItem(String name) { return scoreList_.setItem(curPhraseIdx_, name, outputPath_); /* also updates take idx */ }
	void saveUpdatedScoreList() { scoreList_.saveToFile(scoreListFilename_, false); }
	void increaseCurPhraseIdx() { curPhraseIdx_ += 1; }
	void decreaseCurPhraseIdx() { curPhraseIdx_ -= 1; }
	void trySetCurPhraseIdxFromNameElseIdx(String name, int idx)
	{
		int tryIdx = scoreList_.findIdxFromName(name);
		if (tryIdx != -1)
			curPhraseIdx_ = tryIdx;
		else if (idx >= 0 && idx < scoreList_.getNumItems())
			curPhraseIdx_ = idx;
		// else curPhraseIdx_ remains unchanged
	}

	NotifyingLogAppender *getEventLog(); // for gui to display log

	void setAudioCh1BufferFullnessMeter(class HorizontalBarMeter *meter);
	void setAudioCh2BufferFullnessMeter(class HorizontalBarMeter *meter);
	void setTrackerBufferFullnessMeter(class HorizontalBarMeter *meter);
	void setArduinoBufferFullnessMeter(class HorizontalBarMeter *meter);

#if (ENABLE_CHECK_PROCESSING_DEAD != 0)
	bool isProcessingDead() const { return isProcessingDead_.isSet(); }
	void setProcessingDeadFlag() { isProcessingDead_.set(true); }
#endif

	void setTrackerToAudioSyncOffset(int desiredOffset) { trackerToAudioSyncOffset_ = desiredOffset; }
	void setArduinoToTrackerSyncOffset(int desiredOffset) { arduinoToTrackerSyncOffset_ = desiredOffset; }
	bool doesSyncRequireRetrigger() const { return syncRequiresRetrigger_; }

	void enableTuningRefTone(bool enable) { useTuningRefTone_.set(enable); }
	bool getUseTuningRefTone() const { return useTuningRefTone_.isSet(); }

	void triggerSyncSignal()
	{
		syncOutGen_.trigger();
		syncRequiresRetrigger_ = false;
	}

	// Signals (effect-to-editor communication):
	ChangeBroadcaster updateLoggerListBoxAsynchronousSignal;
	ChangeBroadcaster updateEntireEditorSignal;
	ChangeBroadcaster endTrackerCalibrationSequenceReachedSignal;
	ChangeBroadcaster endForceCalibrationSequenceReachedSignal;
	ChangeBroadcaster endSampling3dModelSequenceReachedSignal;
	ChangeBroadcaster endRecordingPhraseSignal;

	AtomicInt processingDebugLocator;

private:
	// State (public):
	// -----------------------------------------------------------------------------------
	bool useGagesEnabled_;
	int comPortIdx_;
	bool useStereoEnabled_;
	bool isAutoStringEnabled_;
	bool useScene3dEnabled_;
	bool isManualSyncEnabled_;
	bool isPlotSyncSigEnabled_;
	bool isContRetriggerEnabled_;

	AtomicEnum<TrackerState> trackerState_;
	AtomicEnum<TrackerCalibrationState> trackerCalibrationState_;
	AtomicEnum<RecordingState> recordingState_;
	AtomicEnum<OperationMode> operationMode_;

	WaitableEvent pendingDisconnectCanComplete_;

	TrackerCalibration trackerCalibration_;
	bool hasTrackerCalibrationBeenModified_;
	CalibrationAngles anglesCalibration_;

	TrackerCalibration trackerCalibrationPrev_; // for showing previous calibration while doing new calibration

	String scoreListFilename_;
	String outputPath_;
	AtomicFlag readyToRecord_;

	int curPhraseIdx_;
	concat::ScoreList scoreList_;

	ForceCalibration forceCalibration_;

	ViolinModel3d violinModel3d_;

	int trackerToAudioSyncOffset_; // the desired offset
	int arduinoToTrackerSyncOffset_;
	int curUsedTrackerToAudioSyncOffset_; // the currently used offset
	int curUsedArduinoToAudioSyncOffset_;
	bool syncRequiresRetrigger_;

	// Sub components:
	// -----------------------------------------------------------------------------------
	LibertyTracker tracker_;
	NotifyingLogAppender eventLogAppender_;
	AsynchFileLogger fileLogger_;

	class TrackerThread *trackerThread_;

	class AsynchFileWriter *audioCh1Writer_;
	class AsynchFileWriter *audioCh2Writer_;
	class AsynchFileWriter *trackerWriter_;
	class AsynchFileWriter *arduinoWriter_;

	ComPort comPort_;
	ComPortReadBuffer comPortReadBuffer_;

	FixedFreqTone tuningRefToneGenerator_;
	AtomicFlag useTuningRefTone_;

	SyncOutGenerator syncOutGen_;

	// Internal:
	// -----------------------------------------------------------------------------------
	// Connecting:
	bool wasTrackerConnectedLastFrame_;

	// Recording start/stop:
	bool wasRecordingLastFrame_;

	// Computing descriptors:
	ComputeViolinPeformanceDescriptors computeDescriptors_;
	ComputeViolinPeformanceDescriptors computeDescriptorsPrevCalib_;

	// Tracker frame count continuity:
	unsigned int lastTrackerFrameCountCheckContinuity_; // for testing if any frames were dropped (for logging)

	// Tracker interpolation:
	unsigned int lastTrackerFrameCountInterp_;
	RawSensorData lastRawSensorDataInterp_;
	TrackerFrameInterpolator<3> trackerFrameInterpolator_;

	// Arduino serial communication:
	enum
	{
		VALID_ARDUINO_FRAMES_BUFFER_SIZE = 512
	};
	ArduinoFrame validArduinoFrames_[VALID_ARDUINO_FRAMES_BUFFER_SIZE];
	int numValidArduinoFrames_;
	int numArduinoBytesSkipped_; // for log only
//l2	concat::uint32_t crcTable256_[256];

	// Arduino frame count continuity:
	unsigned int lastArduinoFrameCountCheckContinuity_; // for testing if any frames were dropped (for logging)

	// Arduino interpolation:
	unsigned int lastArduinoFrameCountInterp_;
	float lastArduinoGagesValueInterp_;
	float lastArduinoLoadCellValueInterp_;
#if (ENABLE_OPTICAL_SENSOR != 0)
	float lastArduinoOpticalValueInterp_;
#endif

	// Displaying process errors (once):
	bool displayedNoHostTimingInfoError_;
	bool displayedNumChannels_;
	double minAllowedTempo_; // have minimum allowed tempo to be able to allocate memory for 1 bar history
	bool displayedBarTooLongError_;
	bool displayedSerialCommError_;

	// Stylus button state:
	bool prevFrameStylusButtonPressed_;

	// Processing alive flag:
#if (ENABLE_CHECK_PROCESSING_DEAD != 0)
	AtomicFlag isProcessingDead_;
#endif

	// Currently active audio stream properties (sample rate, block size):
	bool isProcessingSuspended_;
	int blockSize_;			// used to see if blockSize/sampleRate changed wrt. 
	double sampleRate_;		// ... previously set blockSize/sampleRate for logging purposes

	// Computing input 'waveform':
	double inWaveformStepSize_;
	double inWaveformNextStepEnd_;
	concat::int32_t inWaveformNextStepEndRounded_;
	concat::int32_t inWaveformCurPos_; // wraps at 13 hours (signed because of double-to-int conversion)
	float inWaveformCurValue_;

	// Effect-to-editor asynchronous events:
	// -----------------------------------------------------------------------------------
	struct EditorEvent
	{
		enum EventType
		{
			TAKE_TRACKER_CALIBRATION_SAMPLE,
			TAKE_FORCE_CALIBRATION_SAMPLE
		};

		EventType type;
	};

	LockFreeFifo<EditorEvent> editorEventQueue_;

	int64 lastCalibrationPointSampleTime_; // for timing time between taking two samples (if too close could cause problems)

	// Private functions:
	// -----------------------------------------------------------------------------------
	void connectTrackerSynch();
	void disconnectTrackerSynch();
	friend class TrackerThread; // (call synchronous connect/disconnect methods)

	bool startTrackerAndArduinoStreamsOnFirstFrameAfterConnect();
	void checkTrackerFrameCountContinuity(LibertyTracker::ItemDataIterator iter, int numTrackerFrames, int numItemsPerTrackerFrame);

	int getArduinoDataFromSerialPort(ComPortReadBuffer &readBuffer, ArduinoFrame *frames, int &numArduinoBytesSkipped);
	void checkArduinoFrameCountContinuity(const ArduinoFrame *arduinoFrames, int numArduinoFrames);

	void sendAudioDataToHistoryBufferAndEditor(AudioSampleBuffer &audioBuffer);
	void sendTrackerDataToHistoryBufferAndEditor(LibertyTracker::ItemDataIterator iter, int numTrackerFrames, int numTrackerSensors);
	void sendArduinoDataToHistoryBufferAndEditor(const ArduinoFrame *arduinoFrames, int numArduinoFrames);

	void sendTrackerDataToHistoryBufferSingleFrame(const RawSensorData &frame);
	void sendTrackerDataToEditorSingleFrame(ViolinRecordingPlugInEditor *editor, const RawSensorData &rawSensorData);

	void sendDataToScene3d(LibertyTracker::ItemDataIterator iter, int numTrackerFrames, int numTrackerSensors);

	void processTrackerDataForTrackerCalibratingMode(LibertyTracker::ItemDataIterator iter, int numTrackerFrames, int numItemsPerTrackerFrame);
	void processTrackerDataForForceCalibratingMode(LibertyTracker::ItemDataIterator iter, int numTrackerFrames, int numItemsPerTrackerFrame);
	void processTrackerDataForSampling3dModelMode(LibertyTracker::ItemDataIterator iter, int numTrackerFrames, int numItemsPerTrackerFrame);

	void startRecording(const AudioPlayHead::CurrentPositionInfo &pos, int curAudioBufferSize, int curTrackerBufferSize);
	void stopRecording();

	void initAsynchDiskWriters();
	void createAsynchDiskWriters(double sampleRate);
	void destroyAsynchDiskWriters();
	void resetAsynchDiskWriters();

	// -----------------------------------------------------------------------------------	

	bool hasValidLastCamera_;
	Camera lastCamera_;

	bool getLastKnownCamera(Camera &targetCamera); // from 3d scene, else last known
public:
	void setLastKnownCamera(const Camera &camera); // set last known, and update scene 3d if possible

	bool hasValidLastCamera() const 
	{
		return hasValidLastCamera_;
	}
	const Camera &getLastCamera() const
	{
		return lastCamera_;
	}
};


#endif
