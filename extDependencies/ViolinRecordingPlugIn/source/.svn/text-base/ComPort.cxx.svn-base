#include "ComPort.hxx"

#include <malloc.h> // _aligned_malloc()/_aligned_free()

// ---------------------------------------------------------------------------------------

StringArray *ComPort::portNames_ = NULL;
int ComPort::refCount_ = 0;

int ComPort::getNumComPorts()
{
	if (portNames_ == NULL)
		enumPorts();

	return portNames_->size();
}

String ComPort::getPortName(int idx)
{
	if (portNames_ == NULL)
		enumPorts();

	if (idx < 0 || idx >= portNames_->size())
		return T("");

	String name = (*portNames_)[idx];
	return name;
}

void ComPort::enumPorts()
{
	delete portNames_;
	portNames_ = NULL;

	portNames_ = new StringArray;

	for (int i = 0; i < 256; ++i)
	{
		String tryName = String::formatted(T("\\\\.\\COM%d"), i);
		HANDLE hPort = ::CreateFile(tryName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
		if (hPort != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(hPort);
			portNames_->add(String::formatted(T("COM%d"), i));
		}
	}
}

// ---------------------------------------------------------------------------------------

ComPort::ComPort()
{
	pendingErrors_ = NULL;
	handle_ = INVALID_HANDLE_VALUE;

	pendingErrors_ = (LONG *)(_aligned_malloc(sizeof(LONG), 4));
	(*pendingErrors_) = 0;

	++refCount_;
}

ComPort::~ComPort()
{
	_aligned_free((void *)(pendingErrors_));
	pendingErrors_ = NULL;
	close();

	// delete singleton if class isn't used anymore 
	// (not really needed, but most memory leak detectors report this 
	// as a leak)
	--refCount_;
	if (refCount_ <= 0)
	{
		delete portNames_;
		portNames_ = NULL;
		refCount_ = 0;
	}
}

bool ComPort::open(int idx, BaudRates baudRate, ParityMode parityMode)
{
	if (isOpen())
		return false;

	if (portNames_ == NULL)
		enumPorts();

	if (idx < 0 || idx >= portNames_->size())
		return false;

	// Open port:
	String portName = T("\\\\.\\") + getPortName(idx);

	handle_ = ::CreateFile((const char *)portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, NULL, NULL);

	if (handle_ == INVALID_HANDLE_VALUE)
		return false;

	// Set comm. mask to zero:
	if (::SetCommMask(handle_, 0) == 0)
	{
		close();
		return false;
	}

	// Set comm. time-outs:
	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	if (::SetCommTimeouts(handle_, &timeouts) == 0)
	{
		close();
		return false;
	}

	// Set comm. config:
	DCB config;
	
	memset(&config, 0, sizeof(DCB));
	config.DCBlength = sizeof(DCB);
	
	config.BaudRate = (DWORD)baudRate;

	if (parityMode == ODD_PARITY)
	{
		config.Parity = ODDPARITY;
		config.fParity = 1;
	}
	else if (parityMode == EVEN_PARITY)
	{
		config.Parity = EVENPARITY;
		config.fParity = 1;
	}
	else
	{
		config.Parity = NOPARITY;
		config.fParity = 0;
	}

	config.StopBits = ONESTOPBIT;
	config.ByteSize = 8;

	config.fOutxCtsFlow = 0;
	config.fOutxDsrFlow = 0;
	config.fDtrControl = DTR_CONTROL_DISABLE;
	config.fDsrSensitivity = 0;
	config.fRtsControl = RTS_CONTROL_DISABLE;
	config.fOutX = 0;
	config.fInX = 0;

	config.fErrorChar = 0;
	config.fBinary = 1;
	config.fNull = 0;
	config.fAbortOnError = 0;
	config.wReserved = 0;
	config.XonLim = 2;
	config.XoffLim = 4;
	config.XonChar = 0x13;
	config.XoffChar = 0x19;
	config.EvtChar = 0;

	if (::SetCommState(handle_, &config) == 0)
	{
		close();
		return false;
	}

	// Completed with no errors:
	return true; // successful
}

void ComPort::close()
{
	if (!isOpen())
		return;

	::FlushFileBuffers(handle_); // (use ::PurgeComm() to discard instead of flush)
	::CloseHandle(handle_);
	handle_ = INVALID_HANDLE_VALUE;
}

bool ComPort::isOpen() const
{
	return (handle_ != INVALID_HANDLE_VALUE);
}

// ---------------------------------------------------------------------------------------

// Doesn't return until size bytes are read.
// Use readBlocking(NULL, 0) to see how many bytes are available and read less than that 
// for non-blocking behavior.
int ComPort::readBlocking(unsigned char *data, int size)
{
	if (!isOpen())
		return 0;

	if (size <= 0 && data != NULL) // if data is NULL, size is ignored
		return 0;

	// Clear any errors (to allow additional read/write operations):
	DWORD pendingErrors;
	COMSTAT status;
	if (::ClearCommError(handle_, &pendingErrors, &status) == 0)
		return 0;

	::InterlockedExchange(pendingErrors_, pendingErrors);

	if (data == NULL)
		return (int)status.cbInQue;

	// Read data:
	DWORD numBytesRead;
	if (::ReadFile(handle_, (void *)data, size, &numBytesRead, NULL) == 0)
		return 0;
	// NOTE: Normally numBytesRead will be equal to size as ReadFile() blocks until size 
	// bytes have been read when port isn't open with OVERLAPPED flag set.

	return (int)numBytesRead;
}

int ComPort::readNonBlocking(unsigned char *data, int sizeMax)
{
	const int avail = readBlocking(NULL, 0);
	if (sizeMax > avail)
		sizeMax = avail;
	return readBlocking(data, sizeMax);
}



