#include "Logging.hxx"

// Static instance of logger map:
namespace concat
{
	std::map<std::string, Logger> Logger::loggers_ = std::map<std::string, Logger>();
}

// ---------------------------------------------------------------------------------------

namespace concat
{
	SimpleLayout::SimpleLayout()
	{
		lastLineWasEnded_ = true;
	}

	std::string SimpleLayout::format(const std::string &levelAsString, const std::string &msg, const char *file, int line, bool endLine)
	{
		std::string msg2;

		if (lastLineWasEnded_)
			msg2 = levelAsString + " - " + msg;
		else
			msg2 = msg;

		lastLineWasEnded_ = endLine;

		return msg2;
	}
}

// ---------------------------------------------------------------------------------------

namespace concat
{
	void WriterAppender::appendMsg(const std::string &levelAsString, const std::string &msg, const char *file, int line, bool endLine)
	{
		if (stream_ == NULL)
			return;

		if (endLine)
		{
			if (layout_ == NULL)
				(*stream_) << msg << std::endl;
			else
				(*stream_) << layout_->format(levelAsString, msg, file, line, endLine) << std::endl;
			// XXX: should std::endl be added in the layout or here?
		}
		else
		{
			if (layout_ == NULL)
				(*stream_) << msg;
			else
				(*stream_) << layout_->format(levelAsString, msg, file, line, endLine);
		}
	}
}

// ---------------------------------------------------------------------------------------

namespace concat
{
	FileAppender::FileAppender()
	{
	}

	FileAppender::FileAppender(const std::string &filename, bool appendingMode, int maxNumBackups)
	{
		open(filename, appendingMode, maxNumBackups);
	}

	FileAppender::~FileAppender()
	{
		close();
	}

	void FileAppender::open(const std::string &filename, bool appendingMode, int maxNumBackups)
	{
		if (file_.is_open())
			return;

		if (!appendingMode)
		{
			Filename f(filename.c_str());
			f.makeBackup(maxNumBackups);
		}

		std::ios_base::open_mode mode = std::ofstream::out;
		if (appendingMode)
			mode |= std::ofstream::app;

		file_.open(filename.c_str(), mode);

		if (!file_.fail())
			setStream(file_);
	}

	void FileAppender::close()
	{
		if (!file_.is_open())
			return;

		file_.close();
	}

	bool FileAppender::isOpen() const
	{
		return file_.is_open();
	}
}

// ---------------------------------------------------------------------------------------

namespace concat
{
	Logger &Logger::getLogger(const std::string &name)
	{
		std::map<std::string, Logger>::iterator i = loggers_.find(name);
		if (i == loggers_.end())
		{
			loggers_.insert(std::map<std::string, Logger>::value_type(name, Logger()));
			return getLogger(name);
		}
		else
		{
			return (*i).second;
		}
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	Logger::Logger()
	{
		level_ = LEVEL_ALL;
	}

	Logger::~Logger()
	{
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void Logger::setLevel(Level level)
	{
		level_ = level;
	}

	Logger::Level Logger::getLevel() const
	{
		return level_;
	}

	std::string Logger::getLevelAsString(Level level) const // XXX: return const char * (?)
	{
		switch (level)
		{
		case LEVEL_OFF:		return "OFF  "; // XXX: white space at end sound be done in Layout
		case LEVEL_DEBUG:	return "DEBUG";
		case LEVEL_INFO:	return "INFO ";
		case LEVEL_WARN:	return "WARN ";
		case LEVEL_ERROR:	return "ERROR";
		case LEVEL_FATAL:	return "FATAL";
		case LEVEL_ALL:		return "ALL  ";
		}

		return "";
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// appender pointer copied (i.e. instance must exist throughout Logger's lifetime)
	void Logger::addAppender(Appender &appender)
	{
		std::vector<Appender *>::iterator it = std::find(appenders_.begin(), appenders_.end(), &appender);
		if (it != appenders_.end())
			return; // avoid adding the same appender twice

		appenders_.push_back(&appender);
	}

	void Logger::removeAppender(Appender &appender)
	{
		std::vector<Appender *>::iterator it = std::find(appenders_.begin(), appenders_.end(), &appender);
		if (it == appenders_.end())
			return; // appender not added to this logger

		appenders_.erase(it);
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void Logger::log(Level level, const std::string &msg, const char *file, int line, bool endLine)
	{
		if (level_ < level)
			return;

		// notify all appenders
		for (std::vector<Appender *>::iterator i = appenders_.begin(); i != appenders_.end(); ++i)
		{
			(*i)->appendMsg(getLevelAsString(level), msg, file, line, endLine);
		}
	}

	void Logger::assertLog(bool expr, const std::string &msg)
	{
		if (!expr)
			log(LEVEL_INFO, msg);
	}
}

// ---------------------------------------------------------------------------------------


#include <windows.h>

namespace concat
{
	std::string getSystemName()
	{
		TCHAR name[512];
		DWORD size = 511; // ?
		BOOL ok = ::GetComputerName(name, &size);
		std::string result;
		if (ok)
			result = name;
		return result;
	}

	std::string getSystemTime()
	{
		SYSTEMTIME systemTime;
		::GetLocalTime(&systemTime);
//		std::string result = formatStr("%02d:%02d:%02d (hh:mm:ss)", systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
		std::string result = formatStr("%02d:%02d:%02d", systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
		return result;
	}

	std::string getSystemDate()
	{
		SYSTEMTIME systemTime;
		::GetLocalTime(&systemTime);
//		std::string result = formatStr("%02d-%02d-%04d (dd-mm-yyyy)", systemTime.wDay, systemTime.wMonth, systemTime.wYear);
		std::string result = formatStr("%02d-%02d-%04d", systemTime.wDay, systemTime.wMonth, systemTime.wYear);
		return result;
	}
}





