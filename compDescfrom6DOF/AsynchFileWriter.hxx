#ifndef INCLUDED_ASYNCHFILEWRITER_HXX
#define INCLUDED_ASYNCHFILEWRITER_HXX

//#include "ext_obex.h"	// required for new style Max object
//#include "z_dsp.h"
//#include "ext.h" // Required for all Max external objects

#include "LockFreeFifo.hxx"

#include "AtomicFlag.hxx"
#include "AtomicPtr.hxx"

//#include "ViolinRecordingPlugInConfig.hxx"
#define DISABLE_TIMERS 1
#if (DISABLE_TIMERS != 0)
#include "WriteTimer.hxx"
#define Timer WriteTimer
#endif
 

// Asynchronous file writer. 
//
// Writing files from a periodically-called audio callback should be avoided as these kind 
// of operations generally can't be guaranteed to complete in a bound time.
// These kind of operations also may involve locking the high priority audio thread, 
// possibly causing priority inversion problems, etc.
//
// Asynchronous file writer allows the audio thread to post data to a lock-free circular 
// buffer, which is then written to disk from a low priority thread.
// Assuming the long-term producing and consuming rates of the two threads is at least 
// equal, the buffer only has to be big enough to handle short-term variations in the 
// rates (e.g. some other process may be doing some disk access which causes a temporary 
// drop in consumption rate, while the production rate stays constant).
class AsynchFileWriter : private Timer
{
private:
	struct FileEvent
	{
		enum EventType
		{
			START_NEW_RECORDING,
			END_CURRENT_RECORDING
		};

		// Event type;
		EventType type;

		// Event payload (for START_NEW_RECORDING event):
		char filename[1024];

		FileEvent()
		{
		}

		FileEvent(const FileEvent &rhs)
		{
			type = rhs.type;
			memcpy(filename, rhs.filename, 1024);
		}

		FileEvent &operator=(const FileEvent &rhs)
		{
			type = rhs.type;
			memcpy(filename, rhs.filename, 1024);
			return *this;
		}
	};

public:
	// Interface that must be implemented by class which does the actual file writing in 
	// a specific format.
	class FileWriterInterface
	{
	public:
		virtual ~FileWriterInterface() {}

		virtual int getFrameSize() const = 0;

		virtual void openFile(const char *filename) = 0;
		virtual void closeFile() = 0;
		virtual bool isOpen() const = 0;

		virtual void writeItems(const float *buffer, int numItems) = 0;
	};

public:
	AsynchFileWriter(); //t_object *maxObject);
	~AsynchFileWriter();

	// Configuration functions:
	void allocate(int consumptionIntervalMilliseconds, double avgProductionConsumptionRate, double toleranceFactor, int peakProductionMaximum);

	template<typename ConcreteFileWriterT>
	void setFileWriter(ConcreteFileWriterT &prototype)
	{
		if (fileWriter_ != NULL)
		{
			delete fileWriter_;
			fileWriter_ = NULL;
		}

		ConcreteFileWriterT *writer = new ConcreteFileWriterT(prototype);
		fileWriter_ = dynamic_cast<FileWriterInterface *>(writer);
		if (fileWriter_ == NULL)
			delete writer;
	}

	//void setFullnessMeter(class HorizontalBarMeter *fullnessMeter) { fullnessMeter_ = fullnessMeter; }

	// Starting/stopping (timer):
	void startConsumerThread();
	void stopConsumerThread();

	// Writing data (may be done continuously even when not disk writing):
	int writeData(const float *data, int sizeItems);

	// Posting events (to start/end disk write):
	void postStartDiskWriteEvent(const char *filename, int numFramesToGoBackInHistory);
	void postStopDiskWriteEvent();

	// Clearing (thread-safe):
	void clearDataAndResetUnwrappedWriteIdxCount();

	// Computing rewind:
	unsigned int getUnwrappedWriteIdxFrames() const { return writeIdxUnwrappedFrames_; }
	void timerCallback();
private:
	//void timerCallback2(t_object *polhemusSoundRec);


	void writeRemainingDataInBufferToFileAndClose();
	void handleFirstOfPendingEvents();

private:
	//t_object * maxObject_;
	LockFreeFifo<float> dataBuffer_;
	LockFreeFifo<FileEvent> eventBuffer_;

	int timerIntervalMilliseconds_;

	float *writeBuffer_;
	int writeBufferSize_;
	FileEvent curEvent_;
	unsigned int writeIdxUnwrappedFrames_;

	AtomicFlag isProducerDiskWriting_;
	AtomicFlag isConsumerDiskWriting_; // (doesn't have to be atomic, but so both flags use same syntax)
	// Note: Two separate flags for thread-safety issues.

	FileWriterInterface *fileWriter_;

	//AtomicPtr<HorizontalBarMeter> fullnessMeter_;
};


#endif