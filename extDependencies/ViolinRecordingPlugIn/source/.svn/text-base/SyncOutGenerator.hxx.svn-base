#ifndef INCLUDED_SYNCOUTGENERATOR_HXX
#define INCLUDED_SYNCOUTGENERATOR_HXX

#include "concat/Utilities/FloatToInt.hxx"
#include "concat/Utilities/Optional.hxx"
#include "LibertyTracker.hxx"
#include "ArduinoFrame.hxx"

#include "ViolinRecordingPlugInConfig.hxx"

#include <cmath>

#define MAX_PENDING_PULSES 32
#define MAX_ALLOWED_PULSE_DISTANCE_SEC 2.0 // +/- 480 frames
#define MAX_NUM_FRAMES_DIFF_WITH_PREV 10

#define CONT_RETRIGGER_INTERVAL_MILLISECONDS 200.0 // hardcoded here because affects hystersis and initial pause
#define NUM_PULSES_INITIALPAUSE 10 // after connect: send first pulse, wait some time continue sending pulses (cont. retrigger mode)
#define HYSTERESIS_TIME_RETRIGGER_PULSES 15 // 15*200ms = 3 sec

#define ADDITIONAL_DEBUG_LOGGING 0

#define INVALID_STREAM_OFFSET -10000

#define DISABLE_HYSTERESIS 1
#define DISABLE_CHANGE_LIMITING 1

class PulseSynchronizer
{
public:
	PulseSynchronizer()
	{
		reset();
	}

	void reset()
	{
		for (int i = 0; i < MAX_PENDING_PULSES; ++i)
			pendingPulseOnsets_[i] = -1; // invalid

		numPendingPulses_ = 0;

		prevResult_ = INVALID_STREAM_OFFSET;
	}

	bool addPendingPulse(int pulseOnsetAudioFrameCount)
	{
		if (numPendingPulses_ < MAX_PENDING_PULSES)
		{
			pendingPulseOnsets_[numPendingPulses_] = pulseOnsetAudioFrameCount;
			++numPendingPulses_;

			return true;
		}
		else
		{
			numPendingPulses_ = 0; // reset (shouldn't happen normally, but just in case)
			return false;
		}
	}

	double getBestMatchingPulseOffset(int receiveFrameCount, double sendSampleRate, double receiveSampleRate, int sendInputLatency, int sendOutputLatency)
	{
#if (ADDITIONAL_DEBUG_LOGGING != 0)
		LOG_INFO_N("violin_recording_plugin", concat::formatStr("Received pulse onset at frame count %d (num pending pulses %d).", receiveFrameCount, numPendingPulses_));
#endif

		double result = INVALID_STREAM_OFFSET;

		if (prevResult_ == INVALID_STREAM_OFFSET)
		{
			// Try all pending pulses (starting with the oldest one), stop after first 
			// valid (within valid range) distance:
			int n = 0;
			while (numPendingPulses_ > 0)
			{
				// compute distance between pulses (in seconds):
				double timeSendPulseSec = (pendingPulseOnsets_[0] + sendInputLatency + sendOutputLatency)/sendSampleRate;
				double timeReceivePulseSec = receiveFrameCount/receiveSampleRate;
				double pulseDistanceSeconds = timeSendPulseSec - timeReceivePulseSec;

#if (ADDITIONAL_DEBUG_LOGGING != 0)
				LOG_INFO_N("violin_recording_plugin", concat::formatStr("Pending pulse %d at audio frame count %d (%d millisec diff)", 1+n, pendingPulseOnsets_[0], concat::round_int(1000.0*pulseDistanceSeconds)));
#endif

				// pop front (oldest) pending pulse:
				for (int k = 0; k < numPendingPulses_-1; ++k)
				{
					pendingPulseOnsets_[k] = pendingPulseOnsets_[k+1];
				}
				pendingPulseOnsets_[numPendingPulses_-1] = -1;
				--numPendingPulses_;

				// check if distance is within allowed range:
				if (fabs(pulseDistanceSeconds) <= MAX_ALLOWED_PULSE_DISTANCE_SEC)
				{
					result = pulseDistanceSeconds;
					break;
				}
				++n;
			}
		}
		else
		{
			// Try all, use one that closest matches previous offset (assuming first is okay and sync cables don't loose connection, etc.):
			int bestPendingPulse = -1;
			double bestOffsetDiff = 1000000.0; // invalidly big (seconds)
			for (int i = 0; i < numPendingPulses_; ++i)
			{
				double timeSendPulseSec = (pendingPulseOnsets_[i] + sendInputLatency + sendOutputLatency)/sendSampleRate;
				double timeReceivePulseSec = receiveFrameCount/receiveSampleRate;
				double pulseDistanceSeconds = timeSendPulseSec - timeReceivePulseSec;
				
#if (ADDITIONAL_DEBUG_LOGGING != 0)
				LOG_INFO_N("violin_recording_plugin", concat::formatStr("Pending pulse %d at audio frame count %d (%d millisec diff)", 1+i, pendingPulseOnsets_[i], concat::round_int(1000.0*pulseDistanceSeconds)));
#endif

				double offsetDiff = fabs(pulseDistanceSeconds - prevResult_);
				if (offsetDiff < bestOffsetDiff &&
#if (DISABLE_CHANGE_LIMITING == 0)
					offsetDiff <= MAX_NUM_FRAMES_DIFF_WITH_PREV*1.0/receiveSampleRate && 
#endif
					fabs(pulseDistanceSeconds) <= MAX_ALLOWED_PULSE_DISTANCE_SEC)
				{
					bestOffsetDiff = offsetDiff;
					bestPendingPulse = i;
				}

#if (ADDITIONAL_DEBUG_LOGGING != 0 && DISABLE_CHANGE_LIMITING == 0)
				if (offsetDiff < bestOffsetDiff &&
					offsetDiff > MAX_NUM_FRAMES_DIFF_WITH_PREV*1.0/receiveSampleRate && 
					fabs(pulseDistanceSeconds) <= MAX_ALLOWED_PULSE_DISTANCE_SEC)
				{
					LOG_INFO_N("violin_recording_plugin", concat::formatStr("[r]Warning: Discarding sync pulse candidate (%d/%d) because difference with current sync too big (%+d frames).", 1+i, numPendingPulses_, concat::round_int((pulseDistanceSeconds - prevResult_)*receiveSampleRate)));
				}
#endif
			}

#if (ADDITIONAL_DEBUG_LOGGING != 0)
			LOG_INFO_N("violin_recording_plugin", concat::formatStr("Best pending pulse = %d", 1+bestPendingPulse));
#endif

			if (bestPendingPulse != -1)
			{
				double timeSendPulseSec = (pendingPulseOnsets_[bestPendingPulse] + sendInputLatency + sendOutputLatency)/sendSampleRate;
				double timeReceivePulseSec = receiveFrameCount/receiveSampleRate;
				double pulseDistanceSeconds = timeSendPulseSec - timeReceivePulseSec;

				result = pulseDistanceSeconds;

				// pop all front pending pulses until (and including) best:
				for (int k = 0; k < numPendingPulses_-bestPendingPulse-1; ++k)
				{
					pendingPulseOnsets_[k] = pendingPulseOnsets_[k+bestPendingPulse+1];
				}

				for (int k = 0; k < bestPendingPulse+1; ++k)
				{
					pendingPulseOnsets_[numPendingPulses_-1-k] = -1;
				}

				numPendingPulses_ -= bestPendingPulse+1;
			}
		}

		if (result != INVALID_STREAM_OFFSET)
			prevResult_ = result;

		return result;
	}

private:
	int numPendingPulses_;
	unsigned int pendingPulseOnsets_[MAX_PENDING_PULSES];

	double prevResult_;
};

// Crude kind of hysteresis
// If a value is changed it must remain unchanged then for a number of frames (assume periodic 
// calls)
class ValueWithHysteresis
{
public:
	void setValueImmediately(int value)
	{
		curValue_ = value;
		newValue_ = curValue_;
		newValueCount_ = 0;
	}

	void setValueHysteresis(int value)
	{
#if (DISABLE_HYSTERESIS != 0)
		setValueImmediately(value);
		return;
#endif

		if (curValue_ == INVALID_STREAM_OFFSET)
		{
			setValueImmediately(value);
			return;
		}

		if (value != newValue_)
		{
			newValue_ = value;
			newValueCount_ = 0; // reset
		}
		else
		{
			++newValueCount_;
		}

        if (newValueCount_ >= HYSTERESIS_TIME_RETRIGGER_PULSES)
			curValue_ = newValue_;
	}

	int getValue() const
	{
		return curValue_;
	}

private:
	int curValue_;
	int newValue_;
	int newValueCount_;
};

class SyncOutGenerator
{
public:
	SyncOutGenerator()
	{
		// initialize configuration variables to invalid values:
		pulseWidthMilliseconds_ = 0.0;
		senderSampleRate_ = 0.0;
		receiverSampleRate_ = 0.0;
		clockRateDivider_ = 0;
		audioInputLatencySamples_ = 0;
		audioOutputLatencySamples_ = 0;

		// initialize mode of operation variables to 'none':
		isPerodicPulseEnabled_ = false;
		wantTriggerPulseOnNextClock_ = false;
		numClocksPerRetriggerPeriod_ = 0;
		intervalMilliseconds_ = 0.0;
		// (don't trigger anything until trigger() or setPeriodic() is called)

		// initialize clock variables to invalid:
		clockCount_ = 0;
		clockBegin_ = -1; // wraps to very high number
		clockEnd_ = -1; // wraps to very high number

		pulseCount_ = 0;

		// reset other variables:
		resetFrameCounts();
		resetEstimatedOffsets();
	}

	// clockRateDivider is used to make sure sync clocks (and thus pulses) happen at an integer multiple of tracker frame interval
	// (so a divider of 8 means pulses are quantized to a 8*1/240 second grid)
	void init(double pulseWidthMilliseconds, double audioSampleRate, double trackerSampleRate, int clockRateDivider, int audioInputLatencySamples, int audioOutputLatencySamples)
	{
		pulseWidthMilliseconds_ = pulseWidthMilliseconds;

		senderSampleRate_ = audioSampleRate;
		receiverSampleRate_ = trackerSampleRate;
		clockRateDivider_ = clockRateDivider;
		audioInputLatencySamples_ = audioInputLatencySamples;
		audioOutputLatencySamples_ = audioOutputLatencySamples;

		updateClockBeginEnd();
		updateNumClocksPerRetriggerPeriod();
	}

	// when reconnecting tracker, reset all frame counters, sync objects, etc.
	void resetFrameCounts()
	{
		audioFrameCount_ = 0;
		trackerFrameCount_ = 0;
		arduinoFrameCount_ = 0;

		lastTrackerFrameCount_ = -1; // (wraps)
		lastArduinoFrameCount_ = -1; // (wraps)

		clockCount_ = 0;

		prevTrackerExtSyncFlag_ = false;
		prevArduinoExtSyncFlag_ = false;

		pulseCount_ = 0;
		
		syncTracker_.reset();
		syncArduino_.reset();

		updateClockBeginEnd();
	}

	// when using manual sync offsets, reset estimated ones (also when reconnecting tracker)
	void resetEstimatedOffsets()
	{
		estimatedTrackerToAudioSyncOffset_.setValueImmediately(INVALID_STREAM_OFFSET);
		estimatedArduinoToAudioSyncOffset_.setValueImmediately(INVALID_STREAM_OFFSET);
	}

	void trigger()
	{
		wantTriggerPulseOnNextClock_ = true;
		isPerodicPulseEnabled_ = false; // disable continuous retrigger (should normally already be disabled)
	}

	void setContRetrigger(bool enable)
	{
		intervalMilliseconds_ = CONT_RETRIGGER_INTERVAL_MILLISECONDS;
		isPerodicPulseEnabled_ = enable;
		wantTriggerPulseOnNextClock_ = false;
		updateNumClocksPerRetriggerPeriod();
	}

	void processAudio(float *audioOut, int numAudioSamples, bool isArduinoEnabled)
	{
		// Generate sync out pulse signal:
		for (int i = 0; i < numAudioSamples; ++i)
		{
			bool shouldClockTriggerPulse = ((isPerodicPulseEnabled_ && (clockCount_ % numClocksPerRetriggerPeriod_) == 0) || wantTriggerPulseOnNextClock_);

			bool shouldSkipPulseBecauseOfInitialDelay = (isPerodicPulseEnabled_ && pulseCount_ >= 1 && pulseCount_ <= NUM_PULSES_INITIALPAUSE);
			// Note: If the first pulse after connecting is send using continuous retrigger mode, the closest received peak will be 
			// matched, which will be incorrect. Solution is to send one pulse, pause, then send continuously retriggered pulses.

			if (audioOut != NULL)
			{
				if (audioFrameCount_ >= clockBegin_ && audioFrameCount_ < clockEnd_ && shouldClockTriggerPulse && !shouldSkipPulseBecauseOfInitialDelay)
				{
					audioOut[i] = 0.966051f; // -0.3 dB to avoid "clipping" warning in Nuendo 3
				}
				else
				{
					audioOut[i] = 0.0f;
				}

//				audioOut[i] = (float)(0.5 + 0.5*sin(2.0*3.1415*0.5*audioFrameCount_*1.0/audioSampleRate_));
//				audioOut[i] = (float)(0.5 + 0.5*sin(2.0*3.1415*0.125*audioFrameCount_/1024.0));

				// add pulse onset to stream synchronizers
				if (audioFrameCount_ >= clockBegin_ && audioFrameCount_ - 1 < clockBegin_ && shouldClockTriggerPulse && !shouldSkipPulseBecauseOfInitialDelay)
				{
#if (ADDITIONAL_DEBUG_LOGGING != 0)
					LOG_INFO_N("violin_recording_plugin", concat::formatStr("Send pulse at audio frame count %d.", audioFrameCount_));
#endif
					//static int prevPulseOnset = -1;
					//if (prevPulseOnset != -1)
					//{
					//	LOG_INFO_N("violin_recording_plugin", concat::formatStr("%d", audioFrameCount_ - prevPulseOnset));
					//}
					//prevPulseOnset = audioFrameCount_;
					
					if (!syncTracker_.addPendingPulse(audioFrameCount_))
						LOG_INFO_N("violin_recording_plugin", "[r]Warning: Tracker pending sync pulses buffer overrrun!");

					if (isArduinoEnabled)
						if (!syncArduino_.addPendingPulse(audioFrameCount_))
							LOG_INFO_N("violin_recording_plugin", "[r]Warning: Arduino pending sync pulses buffer overrrun!");
				}
			}

			// end of current pulse, advance
			if (audioFrameCount_ >= clockEnd_ && audioFrameCount_ - 1 < clockEnd_)
			{
				++clockCount_;
				updateClockBeginEnd();

				wantTriggerPulseOnNextClock_ = false;

				if (shouldClockTriggerPulse)
					++pulseCount_;
			}

			++audioFrameCount_;
		}
	}

	void processTracker(LibertyTracker::ItemDataIterator trackerBufferIter, int numTrackerFrames, int numTrackerSensors)
	{
		// Analyze input signal from tracker to compute stream offsets:
		for (int i = 0; i < numTrackerFrames; ++i)
		{
			bool trackerExtSyncFlag = (trackerBufferIter.item().externalSyncFlag != 0);

			if (trackerExtSyncFlag && !prevTrackerExtSyncFlag_)
			{
#if (ADDITIONAL_DEBUG_LOGGING != 0)
				LOG_INFO_N("violin_recording_plugin", "getBestMatchingPulseOffset() for tracker");
#endif
				double offsetSeconds = syncTracker_.getBestMatchingPulseOffset(trackerFrameCount_, senderSampleRate_, receiverSampleRate_, audioInputLatencySamples_, audioOutputLatencySamples_);

				if (offsetSeconds != INVALID_STREAM_OFFSET)
				{
#if (ADDITIONAL_DEBUG_LOGGING != 0)
					LOG_INFO_N("violin_recording_plugin", "result valid");
#endif
					int estimatedTrackerToAudioSyncOffset = concat::round_int(offsetSeconds*receiverSampleRate_);
					if (isPerodicPulseEnabled_)
						estimatedTrackerToAudioSyncOffset_.setValueHysteresis(estimatedTrackerToAudioSyncOffset);
					else
						estimatedTrackerToAudioSyncOffset_.setValueImmediately(estimatedTrackerToAudioSyncOffset);

//					LOG_INFO_N("violin_recording_plugin", concat::formatStr("Tracker sync flag found! Offset %d frames.", estimatedTrackerToAudioSyncOffset_));
				}
				else
				{
#if (ADDITIONAL_DEBUG_LOGGING != 0)
					LOG_INFO_N("violin_recording_plugin", "result invalid");
#endif
				}
			}

			prevTrackerExtSyncFlag_ = trackerExtSyncFlag;

			if (lastTrackerFrameCount_ == -1)
				trackerFrameCount_ += 1;
			else
				trackerFrameCount_ += (trackerBufferIter.item().frameCount - lastTrackerFrameCount_);
			// XXX: assume trackerBufferIter.item().frameCount - lastTrackerFrameCount_ >= 0

			lastTrackerFrameCount_ = trackerBufferIter.item().frameCount;

			trackerBufferIter.advance(numTrackerSensors);
		}
	}

	void processArduino(const ArduinoFrame *arduinoBuffer, int numArduinoFrames)
	{
		// Analyze input signal from Arduino to compute stream offsets:
		for (int i = 0; i < numArduinoFrames; ++i)
		{
//			LOG_INFO_N("violin_recording_plugin", concat::formatStr("ard. ext. sync. flag %d = %d", i, arduinoBuffer[i].extSyncFlag));

			bool arduinoExtSyncFlag = (arduinoBuffer[i].extSyncFlag != 0);

			if (arduinoExtSyncFlag && !prevArduinoExtSyncFlag_)
			{
#if (ADDITIONAL_DEBUG_LOGGING != 0)
				LOG_INFO_N("violin_recording_plugin", "getBestMatchingPulseOffset() for arduino");
#endif
				double offsetSeconds = syncArduino_.getBestMatchingPulseOffset(arduinoFrameCount_, senderSampleRate_, receiverSampleRate_, audioInputLatencySamples_, audioOutputLatencySamples_);

				if (offsetSeconds != INVALID_STREAM_OFFSET)
				{
#if (ADDITIONAL_DEBUG_LOGGING != 0)
					LOG_INFO_N("violin_recording_plugin", "result valid");
#endif
					int estimatedArduinoToAudioSyncOffset = concat::round_int(offsetSeconds*receiverSampleRate_);
					if (isPerodicPulseEnabled_)
						estimatedArduinoToAudioSyncOffset_.setValueHysteresis(estimatedArduinoToAudioSyncOffset);
					else
						estimatedArduinoToAudioSyncOffset_.setValueImmediately(estimatedArduinoToAudioSyncOffset);
//					LOG_INFO_N("violin_recording_plugin", concat::formatStr("Arduino sync flag found! Offset %d frames.", estimatedArduinoToAudioSyncOffset_));
				}
				else
				{
#if (ADDITIONAL_DEBUG_LOGGING != 0)
					LOG_INFO_N("violin_recording_plugin", "result invalid");
#endif
				}
			}

			prevArduinoExtSyncFlag_ = arduinoExtSyncFlag;

			if (lastArduinoFrameCount_ == -1)
				arduinoFrameCount_ += 1;
			else
				arduinoFrameCount_ += (arduinoBuffer[i].frameCount - lastArduinoFrameCount_);
			// XXX: assume arduinoBuffer[i].frameCount - lastArduinoFrameCount_ >= 0

			lastArduinoFrameCount_ = arduinoBuffer[i].frameCount;
		}
	}

	int getEstimatedTrackerToAudioSyncOffset() const
	{
		return estimatedTrackerToAudioSyncOffset_.getValue();
	}

	int getEstimatedArduinoToTrackerSyncOffset() const
	{
		if (estimatedTrackerToAudioSyncOffset_.getValue() == INVALID_STREAM_OFFSET || estimatedArduinoToAudioSyncOffset_.getValue() == INVALID_STREAM_OFFSET)
			return INVALID_STREAM_OFFSET;
		else
			return -estimatedTrackerToAudioSyncOffset_.getValue() + estimatedArduinoToAudioSyncOffset_.getValue();
	}

private:
	unsigned int audioFrameCount_; // since onset stream
	unsigned int trackerFrameCount_; // since onset stream
	unsigned int arduinoFrameCount_; // since onset stream
	
	unsigned int lastTrackerFrameCount_; // actual frame count returned from device (might not be zero at onset stream for instance)
	unsigned int lastArduinoFrameCount_; // idem

	unsigned int clockCount_;
	unsigned int clockBegin_; // in audio samples
	unsigned int clockEnd_; // in audio samples

	double pulseWidthMilliseconds_;

	double senderSampleRate_; // audio sample rate
	double receiverSampleRate_; // tracker sample rate (= Arduino sample rate)
	int clockRateDivider_;
	int audioInputLatencySamples_;
	int audioOutputLatencySamples_;

	bool isPerodicPulseEnabled_;
	bool wantTriggerPulseOnNextClock_;
	unsigned int numClocksPerRetriggerPeriod_;
	double intervalMilliseconds_;
	unsigned int pulseCount_;

	PulseSynchronizer syncTracker_;
	PulseSynchronizer syncArduino_;

	bool prevTrackerExtSyncFlag_;
	bool prevArduinoExtSyncFlag_;

	ValueWithHysteresis estimatedTrackerToAudioSyncOffset_;
	ValueWithHysteresis estimatedArduinoToAudioSyncOffset_;

	void updateClockBeginEnd()
	{
		if (clockRateDivider_ > 0 && receiverSampleRate_ > 0.0 && senderSampleRate_ > 0.0 && pulseWidthMilliseconds_ > 0.0)
		{
			clockBegin_ = concat::round_int(clockCount_*clockRateDivider_*1.0/receiverSampleRate_*senderSampleRate_);
			clockEnd_ = concat::round_int((clockCount_*clockRateDivider_*1.0/receiverSampleRate_ + pulseWidthMilliseconds_/1000.0)*senderSampleRate_);
		}
	}

	void updateNumClocksPerRetriggerPeriod()
	{
		if (intervalMilliseconds_ > 0.0 && clockRateDivider_ > 0 && receiverSampleRate_ > 0.0)
		{
			numClocksPerRetriggerPeriod_ = concat::round_int(intervalMilliseconds_/1000.0 / (clockRateDivider_*1.0/receiverSampleRate_));
			if (numClocksPerRetriggerPeriod_ <= 0)
				numClocksPerRetriggerPeriod_ = 1;
		}
	}
};

#endif