#ifndef INCLUDED_COMPORT_HXX
#define INCLUDED_COMPORT_HXX

#include "windows.h"
#include "juce.h"

#define NOMINMAX // avoid min/max macros from windows.h
#define NOGDI // avoid GDI stuff (messes up juce)
#include <windows.h>

#include "ViolinRecordingPlugInConfig.hxx"
#if (ENABLE_EXTENDED_SERIAL_COMM_ERROR_LOGGING != 0)
#include "concat/Utilities/Logging.hxx"
#endif

// Win32 serial port communication using Juce for strings/arrays
class ComPort
{
public:
	static int getNumComPorts();
	static String getPortName(int idx);

	enum BaudRates
	{
		RATE_9600 = 9600,
		RATE_14400 = 14400,
		RATE_19200 = 19200,
		RATE_38400 = 38400,
		RATE_57600 = 57600,
		RATE_115200 = 115200
	};

	enum ParityMode
	{
        NO_PARITY,
		ODD_PARITY,
		EVEN_PARITY
	};

	ComPort();
	~ComPort();
	bool open(int idx, BaudRates baudRate = RATE_115200, ParityMode parityMode = NO_PARITY);
	void close();
	bool isOpen() const;

	int readBlocking(unsigned char *data, int size);
	int readNonBlocking(unsigned char *data, int sizeMax);

	bool isOk() const { return ((*pendingErrors_) == 0); }
	bool hasBreakErr() const { return (((*pendingErrors_) & CE_BREAK) != 0); }
	bool hasFrameErr() const { return (((*pendingErrors_) & CE_FRAME) != 0); }
	bool hasOverrunErr() const { return (((*pendingErrors_) & CE_OVERRUN) != 0); }
	bool hasRxOverErr() const { return (((*pendingErrors_) & CE_RXOVER) != 0); }
	bool hasRxParityErr() const { return (((*pendingErrors_) & CE_RXPARITY) != 0); }

private:
	static StringArray *portNames_;
	static void enumPorts();
	static int refCount_;

	HANDLE handle_;

	volatile LONG *pendingErrors_;

	ComPort(const ComPort &); // non-copyable
	ComPort &operator=(const ComPort &); // non-copyable
};

// ---------------------------------------------------------------------------------------

#include <cassert>
#include "concat/Utilities/StdInt.hxx"

// utility class to read serial port into
// can be access using circular iterator
class ComPortReadBuffer
{
public:
	class Iterator
	{
	public:
		const concat::byte operator*() const
		{
			return base_[offset_];
		}

		const concat::byte operator[](int index) const
		{
			assert(index >= 0);
			assert(index < size_);
			int i = offset_ + index;
			if (i >= size_)
				i -= size_;
			return base_[i];
		}

		const concat::byte *getPtrAt(int index) const
		{
			assert(index >= 0);
			assert(index < size_);
			int i = offset_ + index;
			if (i >= size_)
				i -= size_;
			return &base_[i];
		}

		concat::byte getByteAt(int indexBytes) const
		{
			return (*this)[indexBytes];
		}

		concat::uint32_t getUint32At(int indexBytes) const
		{
			concat::uint32_t result = 0;
			concat::byte *tmp = (concat::byte *)(&result);
			tmp[0] = (*this)[indexBytes + 0];
			tmp[1] = (*this)[indexBytes + 1];
			tmp[2] = (*this)[indexBytes + 2];
			tmp[3] = (*this)[indexBytes + 3];

			return result;
		}

		concat::int16_t getInt16At(int indexBytes) const
		{
			concat::int16_t result = 0;
			concat::byte *tmp = (concat::byte *)(&result);
			tmp[0] = (*this)[indexBytes + 0];
			tmp[1] = (*this)[indexBytes + 1];

			return result;
		}

		Iterator &operator++()
		{
			++offset_;
			if (offset_ >= size_)
				offset_ -= size_;
			return *this;
		}

	private:
		concat::byte *base_;
		int offset_;
		int size_;

	private:
		Iterator()
		{
			base_ = NULL;
			offset_ = 0;
			size_ = 0;
		}

		void assign(concat::byte *base, int offset, int size)
		{
			assert(base != NULL);
			assert(offset >= 0);
			assert(offset < size);

			base_ = base;
			offset_ = offset;
			size_ = size;
		}

		void advance(int n)
		{
			assert(n >= 0);
			assert(size_ > 0);

			offset_ += n;
			
			while (offset_ >= size_)
				offset_ -= size_;
		}

		int getOffset() const
		{
			return offset_;
		}

		concat::byte *getPtr()
		{
			return &base_[offset_];
		}

		friend class ComPortReadBuffer;
	};

public:
	ComPortReadBuffer()
	{
		buffer_ = NULL;
		bufferSize_ = 0;
	}

	void attach(ComPort &comPort, int bufferSize)
	{
		delete[] buffer_;
		buffer_ = NULL;
		bufferSize_ = 0;

		comPort_ = &comPort;

		if (bufferSize > 0)
		{
			buffer_ = new concat::byte[bufferSize];
			bufferSize_ = bufferSize;

			readIter_.assign(buffer_, 0, bufferSize_);
			writeIter_.assign(buffer_, 0, bufferSize_);
		}
	}

	~ComPortReadBuffer()
	{
		delete[] buffer_;
		buffer_ = NULL;
		bufferSize_ = 0;
	}

	bool readAllAvailable()
	{
		if (buffer_ == NULL || comPort_ == NULL)
		{
#if (ENABLE_EXTENDED_SERIAL_COMM_ERROR_LOGGING != 0)
			LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: buffer/comport null!");
#endif
			return false; // error
		}

		const int maxWrite = bufferSize_ - getReadAvailable() - 1;
		// Don't allow writing last element, to differentiate between full and empty.

		const int comAvail = comPort_->readBlocking(NULL, 0);
		if (!comPort_->isOk())
		{
#if (ENABLE_EXTENDED_SERIAL_COMM_ERROR_LOGGING != 0)
			LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: error when getting num bytes avail");
			if (comPort_->hasBreakErr())
				LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: break error");
			if (comPort_->hasFrameErr())
				LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: frame error");
			if (comPort_->hasOverrunErr())
				LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: overrun error");
			if (comPort_->hasRxOverErr())
				LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: rx overrun error");
			if (comPort_->hasRxParityErr())
				LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: rx parity error");
#endif
			return false; // error
		}

		int toRead = comAvail;
		if (toRead > maxWrite)
			toRead = maxWrite;

		const int endRead = writeIter_.getOffset() + toRead;
		if (endRead > bufferSize_)
		{
			// Wrapped read/write:
			const int n1 = bufferSize_ - writeIter_.getOffset();
			const int n2 = endRead - bufferSize_;
			assert(n1 > 0);
			assert(n2 > 0);
			comPort_->readBlocking(writeIter_.getPtr(), n1);
			if (!comPort_->isOk())
			{
#if (ENABLE_EXTENDED_SERIAL_COMM_ERROR_LOGGING != 0)
				LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: error when doing two pass copy, pass 1");
				if (comPort_->hasBreakErr())
					LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: break error");
				if (comPort_->hasFrameErr())
					LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: frame error");
				if (comPort_->hasOverrunErr())
					LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: overrun error");
				if (comPort_->hasRxOverErr())
					LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: rx overrun error");
				if (comPort_->hasRxParityErr())
					LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: rx parity error");
#endif
				return false; // error
			}
			writeIter_.advance(n1);
			comPort_->readBlocking(writeIter_.getPtr(), n2);
			if (!comPort_->isOk())
			{
#if (ENABLE_EXTENDED_SERIAL_COMM_ERROR_LOGGING != 0)
				LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: error when doing two pass copy, pass 2");
				if (comPort_->hasBreakErr())
					LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: break error");
				if (comPort_->hasFrameErr())
					LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: frame error");
				if (comPort_->hasOverrunErr())
					LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: overrun error");
				if (comPort_->hasRxOverErr())
					LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: rx overrun error");
				if (comPort_->hasRxParityErr())
					LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: rx parity error");
#endif
				return false; // error
			}
			writeIter_.advance(n2);
		}
		else
		{
			// Single read/write:
			const int n = endRead - writeIter_.getOffset();
			assert(n >= 0);
			comPort_->readBlocking(writeIter_.getPtr(), n);
			if (!comPort_->isOk())
			{
#if (ENABLE_EXTENDED_SERIAL_COMM_ERROR_LOGGING != 0)
				LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: error when doing single pass copy");
				if (comPort_->hasBreakErr())
					LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: break error");
				if (comPort_->hasFrameErr())
					LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: frame error");
				if (comPort_->hasOverrunErr())
					LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: overrun error");
				if (comPort_->hasRxOverErr())
					LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: rx overrun error");
				if (comPort_->hasRxParityErr())
					LOG_WARN_N("violin_recording_plugin", "[r]DEBUG: rx parity error");
#endif
				return false; // error
			}
			writeIter_.advance(n);
		}

		return true; // ok
	}

	Iterator getReadIter() const
	{
		return readIter_;
	}

	int getReadAvailable() const
	{
		const int w = writeIter_.getOffset();
		const int r = readIter_.getOffset();

		if (w >= r)
		{
			return w - r;
		}
		else
		{
			return bufferSize_ - r + w;
		}
	}

	void advanceReadIter(int n)
	{
		assert(n >= 0);
		assert(n <= getReadAvailable());
		readIter_.advance(n);
	}

private:
	ComPort *comPort_;

	concat::byte *buffer_;
	int bufferSize_;
	Iterator readIter_;
	Iterator writeIter_;
};

#endif