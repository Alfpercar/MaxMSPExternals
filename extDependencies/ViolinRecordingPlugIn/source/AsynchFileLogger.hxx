#ifndef INCLUDED_ASYNCHFILELOGGER_HXX
#define INCLUDED_ASYNCHFILELOGGER_HXX

#include "windows.h"
#include "juce.h"

#include "concat/Utilities/Logging.hxx"

#include "concat/Utilities/Filename.hxx"
#include "LockFreeFifo.hxx"
#include <fstream>

#include "ViolinRecordingPlugInConfig.hxx"
#if (DISABLE_TIMERS != 0)
#include "DummyTimer.hxx"
#define Timer DummyTimer
#endif

class AsynchFileLogger : public concat::Appender, private Timer
{
public:
	AsynchFileLogger();
	AsynchFileLogger(const std::string &filename, bool appendingMode = false, int maxNumBackups = 0);
	~AsynchFileLogger();

	void open(const std::string &filename, bool appendingMode = false, int maxNumBackups = 0);
	void close();
	bool isOpen() const;

	void relocateFile(const std::string &filename, int maxNumBackups = 0);

	void appendMsg(const std::string &levelAsString, const std::string &msg, const char *file, int line, bool endLine);

private:
	concat::Filename filename_;

	LockFreeFifo<char> buffer_;
	std::ofstream file_;
	char tempBuffer_[1024];
	CriticalSection fileLock_;

	void init();

	void timerCallback();

	AsynchFileLogger(const AsynchFileLogger &); // non-copyable
	const AsynchFileLogger &operator=(const AsynchFileLogger &); // non-copyable
};

#endif

