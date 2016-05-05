#ifndef INCLUDED_TRACKERTHREAD_HXX
#define INCLUDED_TRACKERTHREAD_HXX

#include "ViolinRecordingPlugInEffect.hxx"
#include "juce.h"
#include "Exceptions.hxx"

class TrackerThread : public Thread
{
public:
	enum Task
	{
		NONE,
		CONNECT_TRACKER,
		DISCONNECT_TRACKER
	};

	TrackerThread(ViolinRecordingPlugInEffect *effect) : Thread(T("TrackerConnectDisconnectThread")), effect_(effect)
	{
		task_ = NONE;
	}

	~TrackerThread()
	{
		stopThread(2000);
	}

	void setTask(Task task)
	{
		if (isThreadRunning())
			return;

		task_ = task;
	}

	void run()
	{
BEGIN_IGNORE_EXCEPTIONS
		// (enter thread)

		if (task_ == CONNECT_TRACKER)
		{
			effect_->connectTrackerSynch();
		}
		else if (task_ == DISCONNECT_TRACKER)
		{
			effect_->disconnectTrackerSynch();
		}

		effect_->updateEntireEditorSignal.sendChangeMessage(NULL); // updating more widgets than strictly needed, but so be it

		task_ = NONE;

		// (exit thread)
END_IGNORE_EXCEPTIONS("TrackerThread::run()")
	}

private:
	ViolinRecordingPlugInEffect *effect_;
	Task task_;
};

#endif


