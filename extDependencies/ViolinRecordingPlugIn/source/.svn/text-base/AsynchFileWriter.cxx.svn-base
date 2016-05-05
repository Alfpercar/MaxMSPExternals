#include "AsynchFileWriter.hxx"

#include "concat/Utilities/FloatToInt.hxx"
#include "concat/Utilities/Logging.hxx"

#include <algorithm>

#include "HorizontalBarMeter.hxx"
#include "Exceptions.hxx"

// ---------------------------------------------------------------------------------------

AsynchFileWriter::AsynchFileWriter()
{
	timerIntervalMilliseconds_ = 0; // invalid

	writeBuffer_ = NULL;
	fileWriter_ = NULL;

	fullnessMeter_ = NULL;

	writeIdxUnwrappedFrames_ = 0;
}

AsynchFileWriter::~AsynchFileWriter()
{
	stopConsumerThread();

	delete[] writeBuffer_;
	writeBuffer_ = NULL;

	delete fileWriter_;
	fileWriter_ = NULL;
}

// ---------------------------------------------------------------------------------------

// avgProductionConsumptionRate in items per second (average production rate and consumption rate are assumed to be equal)
// peakProductionMaximum is maximum peak production per consumption interval, use 0 if not known or if there aren't any production peaks
void AsynchFileWriter::allocate(int consumptionIntervalMilliseconds, double avgProductionConsumptionRate, double toleranceFactor, int peakProductionMaximum)
{
	assert(fileWriter_ != NULL);

	delete[] writeBuffer_;
	writeBuffer_ = NULL;

	writeBufferSize_ = concat::ceil_int(toleranceFactor*avgProductionConsumptionRate*consumptionIntervalMilliseconds/1000.0);
	dataBuffer_.reserve(std::max(writeBufferSize_, peakProductionMaximum));
	writeBuffer_ = new float[writeBufferSize_]; // <= dataBuffer_.capacity()

	eventBuffer_.reserve(2);
	// Note:
	// A maximum of two events are supported and only in the order 
	// pending start-pending stop, not pending stop-pending start.
	// See postStartDiskWriteEvent().

	timerIntervalMilliseconds_ = consumptionIntervalMilliseconds;
}

// ---------------------------------------------------------------------------------------

void AsynchFileWriter::startConsumerThread()
{
	if (isTimerRunning())
		return;

	if (timerIntervalMilliseconds_ > 0)
		startTimer(timerIntervalMilliseconds_);

	isProducerDiskWriting_.set(false);
	isConsumerDiskWriting_.set(false);
}

void AsynchFileWriter::stopConsumerThread()
{
	if (isTimerRunning())
		stopTimer(); // (blocking)

	// In case thread is stopped while recording (i.e. before END_CURRENT_RECORDING 
	// is send), flush the remaining data and close current file:
	writeRemainingDataInBufferToFileAndClose();
}

// ---------------------------------------------------------------------------------------

int AsynchFileWriter::writeData(const float *data, int sizeItems)
{
	if ((sizeItems % fileWriter_->getFrameSize()) != 0)
		LOG_ERROR_N("asynch_file_writer", "[r]ERROR: Asynchronous file writes should be done full frames at a time.");

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Write data to buffer:
	const int result = dataBuffer_.put(data, sizeItems);

	if (result != sizeItems)
		LOG_ERROR_N("asynch_file_writer", concat::formatStr("[r]ERROR: Data buffer underrun (%d samples).", sizeItems - result));

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Do dummy read to empty buffer in case not writing to disk but only keeping history:
	if (!isProducerDiskWriting_.isSet())
	{
		dataBuffer_.clearBySettingReadIdxToWriteIdx();
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Increment unwrapped write idx:
	writeIdxUnwrappedFrames_ += result/fileWriter_->getFrameSize();

	return result;
}

void AsynchFileWriter::postStartDiskWriteEvent(const char *filename, int numFramesToGoBackInHistory)
{
	if (numFramesToGoBackInHistory < 0)
	{
		LOG_ERROR_N("asynch_file_writer", "[r]ERROR: Negative rewind (i.e. forward jump into future) not possible (streams not properly synchronized perhaps?). Recording FAILED.");
		return; // failed
	}
	else if (numFramesToGoBackInHistory > dataBuffer_.getCapacity()/fileWriter_->getFrameSize())
	{
		LOG_ERROR_N("asynch_file_writer", "[r]ERROR: Rewind exceeds buffer size. Recording FAILED.");
		return; // failed
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Do not allow posting start event when there is still a stop event pending, because
	// rewind operation requires read and write pointers to be equal (because rewind amount 
	// of read idx is relative to write idx), which can not be guaranteed when a stop event 
	// is pending.
	// Also fail if data is being written (only possible in case two start events are posted 
	// after each other).
	if (eventBuffer_.getReadAvail() > 0 || isConsumerDiskWriting_.isSet())
	{
		LOG_ERROR_N("asynch_file_writer", "[r]ERROR: Can't start recording while events are pending. Recording FAILED.");
		return; // failed
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Set isDiskWriting flag for producer thread:
	// Note: This flag is only used from this thread (in writeData()), so it doesn't matter if it is 
	// set at the beginning or end of this function.
	isProducerDiskWriting_.set(true);

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Optionally "go back into history" to start writing from correct onset:
	if (dataBuffer_.getReadIdx() != dataBuffer_.getWriteIdx())
	{
		// Shouldn't happen normally (pending stop case handled above).
		LOG_ERROR_N("asynch_file_writer", "[r]ERROR: Read index rewind relative to write index (so should be equal). Recording FAILED.");
		// Synchronization will be incorrect.
	}

	dataBuffer_.decreaseReadIdx(numFramesToGoBackInHistory*fileWriter_->getFrameSize());

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Create event object:
	FileEvent e;
	e.type = FileEvent::START_NEW_RECORDING;

	if (filename != NULL)
	{
		strncpy(e.filename, filename, 1023);
		e.filename[1023] = '\0';
	}
	else
	{
		e.filename[0] = '\0';
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Post event:
	int result = eventBuffer_.put(e);

	if (result != 1)
		LOG_ERROR_N("asynch_file_writer", "[r]ERROR: Event buffer underrun. Recording FAILED.");
}

void AsynchFileWriter::postStopDiskWriteEvent()
{
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Create event object:
	FileEvent e;
	e.type = FileEvent::END_CURRENT_RECORDING;

	e.filename[0] = '\0';

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Post event:
	int result = eventBuffer_.put(e);

	if (result != 1)
		LOG_ERROR_N("asynch_file_writer", "[r]ERROR: Event buffer underrun.");
}

// ---------------------------------------------------------------------------------------

void AsynchFileWriter::clearDataAndResetUnwrappedWriteIdxCount()
{
	// NOTE:
	// When not recording, the data buffer normally is always empty (read pointer is advanced 
	// on write). There is the possibility that a recording has stopped and there's still some 
	// data in the data buffer pending to be written to disk (done from the timer thread).
	// Assuming clearData() is only called when not recording, it should normally have to do 
	// nothing, or, in the case there's still pending data to be written, clear the buffer.
	// In this last case isConsumerDiskWriting_.isSet() will be true so LockFreeFifo::get() and 
	// LockFreeFifo::clear() may be called simultaneously. This is safe as both these functions 
	// are implemented using interlocked operations.

	if (eventBuffer_.getReadAvail() > 0)
	{
		LOG_ERROR_N("asynch_file_writer", "[r]ERROR: Can't clear data when there are pending events.");
		return;
	}

	dataBuffer_.clearBySettingReadIdxToWriteIdx(); // thread-safe

	writeIdxUnwrappedFrames_ = 0; // (only used by this thread)
}

// ---------------------------------------------------------------------------------------

void AsynchFileWriter::timerCallback()
{
BEGIN_IGNORE_EXCEPTIONS
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Notify GUI component of fullness state (optional):
	if (fullnessMeter_ != NULL)
	{
		float fullness = (float)dataBuffer_.getReadAvail()/(float)dataBuffer_.getCapacity(); // [0;1]
		fullnessMeter_->postValue(fullness);
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Handle new event (only a single event is handled per timer callback):
	handleFirstOfPendingEvents();

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Do disk writing if recording:
	if (isConsumerDiskWriting_.isSet())
	{
		const int toWrite = dataBuffer_.get(writeBuffer_, writeBufferSize_);

		if (toWrite > 0)
		{
			if (!fileWriter_->isOpen())
				LOG_ERROR_N("asynch_file_writer", "[r]ERROR: No file open to write to.");
			else
				fileWriter_->writeItems(writeBuffer_, toWrite);
		}
	}
END_IGNORE_EXCEPTIONS("AsynchFileWriter::timerCallback()")
}

// ---------------------------------------------------------------------------------------

void AsynchFileWriter::writeRemainingDataInBufferToFileAndClose()
{
	if (fileWriter_ != NULL && fileWriter_->isOpen())
	{
		// Write all remaining data in buffer:
		while (1)
		{
			int toWrite = dataBuffer_.get(writeBuffer_, writeBufferSize_);
			if (toWrite <= 0)
				break;

			fileWriter_->writeItems(writeBuffer_, toWrite);
		}

		// Close file:
		fileWriter_->closeFile();
	}
}

void AsynchFileWriter::handleFirstOfPendingEvents()
{
	bool hasNewEvent = (eventBuffer_.get(curEvent_) == 1);

	if (hasNewEvent)
	{
		if (curEvent_.type == FileEvent::START_NEW_RECORDING)
		{
			// If there's still a file open, close it (normally shouldn't happen):
			if (isConsumerDiskWriting_.isSet() || fileWriter_->isOpen())
			{
				// Note: Shouldn't happen normally, but may when for instance two 
				// subsequent start events are posted.
				fileWriter_->closeFile();

				LOG_ERROR_N("asynch_file_writer", "[r]ERROR: Starting new recording without ending previous.");
			}

			// Open new file:
			fileWriter_->openFile(curEvent_.filename);

			// Set isDiskWriting flag for consumer thread:
			isConsumerDiskWriting_.set(true);
		}
		else if (curEvent_.type == FileEvent::END_CURRENT_RECORDING)
		{
			if (!isConsumerDiskWriting_.isSet() || !fileWriter_->isOpen())
			{
				// Note: Shouldn't happen normally, but may when for instance two 
				// subsequent end events are posted.
				LOG_ERROR_N("asynch_file_writer", "[r]ERROR: No file open to close.");
			}
			else
			{
				// Write all remaining data in buffer to file and close file:
				writeRemainingDataInBufferToFileAndClose();

				// Unset isDiskWriting flag for producer thread:
				isProducerDiskWriting_.set(false);

				// Unset isDiskWriting flag for consumer thread:
				isConsumerDiskWriting_.set(false);
			}
		}
	}
}

