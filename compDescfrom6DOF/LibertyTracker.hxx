#ifndef INCLUDED_LIBERTYTRACKER_HXX
#define INCLUDED_LIBERTYTRACKER_HXX

#include <windows.h> // PBYTE, DWORD, required for PDI.h
#include <TChar.h>
#include "PDI.h"

// Wrapper class around the PDI (Polhemus Developer Interface).
//
// DATA COLLECTION MECHANISM:
// --------------------------
// The data can be queried in 3 ways:
// 1) As soon as an event is received a windows message (to a user supplied HWND) is send. 
// According to the SDK this method is 'not preferred'.
// 2) The application can query last received frame periodically (on a timer) or in a 
// tight-loop (for single threaded console applications). When no data is available the 
// function will return a frame (byte) size of 0. This method is 'preferred', but some 
// data may be lost if the polling isn't done often enough (because the driver's internal 
// buffer is only 64 frames).
// 3) The third option is to supply a user buffer (of a bigger size than the driver's 
// internal buffer to allow more headroom) and read all events in this buffer periodically.
//
// We will go with option 3, as we do not want to loose data and supplying a HWND isn't 
// practical in a JUCE-VST environment.
//
// DATA-LOSS ON HIGH WORK-LOAD:
// ----------------------------
// The tracker can connected in two ways: using RS-232 or USB.
// As RS-232 has a limited bandwidth (115200 bits per second, or 20 bytes/frame at 
// 240 Hz with 3 sensors enabled), USB is used (bandwidth of 1.5 Mbits per second).
// USB is polled by the host PC (while RS-232 pushes data to the host PC), so to be able 
// to receive more than 1 frame per host poll, hardware buffering must be enabled.
// On high work-load, it may happen that the host PC polls are farther apart (in time) than 
// the hardware buffer size (thus overwriting previous data) and some frames may be lost.
// This is dependent on the available resources on the host PC and the operating system, 
// the hardware buffer has a fixed size.
//
// LATENCY:
// --------
// The system has no input latency due to buffering. That is, there may be some "real-time" 
// latency between a sample being taken by the hardware device and when it can be read/used 
// by the user application, but this not due to some buffer who's read pointer is 
// initialized behind the write pointer (or some multiple buffering scheme, etc.).
// When USB hardware buffering is used "real-time" latency may be greater (or not depending 
// on host PC load, etc.), but data-loss is reduced.
//
// USER-SUPPLIED BUFFER WRAPPING:
// ------------------------------
// Due to a bug in the driver when writing to the user supplied buffer it actually wraps at 
// one whole frame less than the buffer size (in bytes) passed to SetPnoBuffer().
// To work around this, when reading, wrapping should also be done at one frame less than 
// the actual size. Alternatively a bigger than the actually allocated size can be passed to 
// SetPnoBuffer() (this is safe).
class LibertyTracker
{
public:
	// Data for a single item, a frame consists of one or more items, one for each 
	// active sensor.
#pragma pack(push)
#pragma pack(1)
	struct ItemData
	{
		BINHDR header;

		DWORD frameCount;

		float position[3];
		float orientation[3];

		DWORD isStylusButtonPressed;

		DWORD externalSyncFlag;

		static void getOutputFormatList(CPDImdat &stationOutputFormatList)
		{
			stationOutputFormatList.Empty(); // clear

			// Same order as structure, so pointer casting is possible:
			stationOutputFormatList.Append(PDI_MODATA_FRAMECOUNT);

			stationOutputFormatList.Append(PDI_MODATA_POS);
			stationOutputFormatList.Append(PDI_MODATA_ORI);
			// Note: For binary mode ABC and ABC_EP (extended precision) are 
			// equivalent (32-bit floats).
			
			stationOutputFormatList.Append(PDI_MODATA_STYLUS);

			stationOutputFormatList.Append(PDI_MODATA_EXTSYNC);
		}

		void initToZero()
		{
			header.cmd = 0;
			header.err = 0;
			header.length = 0;
			header.preamble = 0;
			header.reserved = 0;
			header.station = 0;

			frameCount = 0;

			position[0] = 0.0f;
			position[1] = 0.0f;
			position[2] = 0.0f;

			orientation[0] = 0.0f;
			orientation[1] = 0.0f;
			orientation[2] = 0.0f;

			isStylusButtonPressed = 0;

			externalSyncFlag = 0;
		}
	};
#pragma pack(pop)

	// Iterator for circular buffer of items.
	class ItemDataIterator
	{
	public:
		ItemDataIterator()
		{
			circBufferBase_ = NULL;
			idx_ = 0;
			size_ = 0;
		}

		ItemDataIterator(ItemData *circBufferBase, int idx, int size)
		{
			circBufferBase_ = circBufferBase;
			idx_ = idx; // possibly not wrapped
			size_ = size;
		}

		bool operator==(const ItemDataIterator &other) const
		{
			return (circBufferBase_ == other.circBufferBase_ && idx_ == other.idx_ && size_ == other.size_);
		}

		bool operator!=(const ItemDataIterator &other) const
		{
			return !(*this == other);
		}

		int operator-(const ItemDataIterator &rhs) const
		{
			int result;

			if (idx_ >= rhs.idx_)
				result = idx_ - rhs.idx_;
			else
				result = (size_ - rhs.idx_) + idx_;

			return result;
		}

		void next()
		{
			advance(1);
		}

		void nextNoWrap()
		{
			advanceNoWrap(1);
		}

		void advance(int count)
		{
			advanceNoWrap(count);
			wrap();
		}

		void advanceNoWrap(int count)
		{
			idx_ += count;
		}

		void wrap()
		{
			if (size_ <= 0)
				return;

			while (idx_ >= size_)
			{
				idx_ = idx_ - size_;
			}
		}

		const ItemData &item() const
		{
			return circBufferBase_[idx_];
		}

	private:
		ItemData *circBufferBase_;
		int idx_;
		int size_;
	};

	LibertyTracker();
	~LibertyTracker();

	void connect();
	void configure();

	int getNumEnabledSensors() const;
	double getSampleRate() const;

	void startReceivingEvents();
	int queryFrames(ItemDataIterator &circularIterator);
//	void clearBuffer();
	void resetFrameCount();
	void stopReceivingEvents();

	void disconnect();

	bool isOk();

private:
	CPDIdev libertyTracker_;

	int numEnabledSensors_; // cached value

	PBYTE clientCircularBuffer_;
	int bufferNoLastFrameSizeItems_;

	ItemDataIterator prevQueryEnd_;

	void logConfiguration();
	void logSensorConfiguration(int index);
};

#endif

