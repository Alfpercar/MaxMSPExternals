#include "AsynchFileLogger.hxx"

#include "Exceptions.hxx"

using namespace concat;

AsynchFileLogger::AsynchFileLogger()
{
	init();
}

AsynchFileLogger::~AsynchFileLogger()
{
	close();
}

AsynchFileLogger::AsynchFileLogger(const std::string &filename, bool appendingMode, int maxNumBackups)
{
	init();
	open(filename, appendingMode, maxNumBackups);
}

void AsynchFileLogger::init()
{
	buffer_.reserve(256*1024); // buffering for logs in between timer writes, but also to buffer before a file has been opened
}

void AsynchFileLogger::open(const std::string &filename, bool appendingMode, int maxNumBackups)
{
	if (isOpen())
		return;

	filename_ = Filename(filename.c_str());

	if (!appendingMode)
	{
		filename_.makeBackup(maxNumBackups);
	}

	std::ios_base::open_mode mode = std::ofstream::out | std::ofstream::binary;
	if (appendingMode)
		mode |= std::ofstream::app;

	// LOCK
	{
		const ScopedLock scopedFileLock(fileLock_);
		file_.open(filename.c_str(), mode);
	}
	// UNLOCK

	startTimer(2000);
}

void AsynchFileLogger::close()
{
	if (!file_.is_open())
		return;

	if (isTimerRunning())
		stopTimer(); // blocking

	// flush buffer:
	while (1)
	{
		int ableToRead = buffer_.get(tempBuffer_, 1024);
		if (ableToRead == 0)
			break;

		file_.write(tempBuffer_, ableToRead);
		file_.flush();
	}

	file_.close();
}

bool AsynchFileLogger::isOpen() const
{
	// LOCK
	const ScopedLock scopedFileLock(fileLock_);
	return file_.is_open();
	// UNLOCK
}

void AsynchFileLogger::relocateFile(const std::string &filename, int maxNumBackups)
{
	// If not open, can't relocate, just open normally:
	if (!file_.is_open())
	{
		open(filename, false, maxNumBackups);
		return;
	}

	// Close current file (also flushes buffer to file):
	close();

	// Move existing files out of the way:
	Filename f(filename.c_str());
	f.makeBackup(maxNumBackups);

	// Move file to new location:
	filename_.moveTo(f);

	// Open re-located file in appending mode:
	open(filename, true, 0); // also sets filename_
}

void AsynchFileLogger::appendMsg(const std::string &levelAsString, const std::string &msg, const char *file, int line, bool endLine)
{
	std::string msg2;
	if (msg.find("[r]") == 0)
		msg2 = msg.substr(3, msg.size()-3);
	else
		msg2 = msg;

	buffer_.put(msg2.c_str(), msg2.size());

	if (endLine)
	{
#ifdef WIN32
		char lineEnd[2];
		lineEnd[0] = 0x0d; // cr
		lineEnd[1] = 0x0a; // lf
#else
#error Implement function for platform!
#endif

		buffer_.put(lineEnd, 2);
	}
}

void AsynchFileLogger::timerCallback()
{
BEGIN_IGNORE_EXCEPTIONS
	if (!isOpen())
		return;

	int ableToRead = buffer_.get(tempBuffer_, 1024);
	if (ableToRead > 0)
	{
		file_.write(tempBuffer_, ableToRead);
		file_.flush();
	}
END_IGNORE_EXCEPTIONS("AsynchFileLogger::timerCallback")
}