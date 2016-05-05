#ifndef INCLUDED_CONCAT_LOGGING_HXX
#define INCLUDED_CONCAT_LOGGING_HXX

#include <cstddef>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <algorithm>
#include <vector>

#include "concat/Utilities/Filename.hxx"
#include "concat/Utilities/StringHelpers.hxx"

// Simple logging facilities, loosely based on log4cxx/log4j

// TODO:
// - add multiple instance support for use with dll
// (maybe using a map<HINSTANCE, map<string, Logger> > or 
// vector<pair<HINSTANCE, map<string, Logger>> as there will be few instances probably)
//   XXX: is HINSTANCE shared between different dlls?
//
// - more fancy output and formatting (html, etc.)

namespace concat
{
	// Global functions:
	std::string getSystemName();
	std::string getSystemTime();
	std::string getSystemDate();

	// Classes:
	class Layout;
	class SimpleLayout;

	class Appender;
	class WriterAppender;
	class ConsoleAppender;
	class FileAppender;

	class Logger;

	class IndentHelper;

	// Macros:
	// *_N macros use name instead of object
	// LOG_LOG* macros need level specified explicitly
#ifndef CONCAT_DISABLE_LOGGING
	#define ASSERT_LOG(logger, expr, msg) (logger).assertLog((expr), (msg), __FILE__, __LINE__)
	#define LOG_LOG(logger, level, msg) (logger).log((level), (msg), __FILE__, __LINE__)
	#define LOG_DEBUG(logger, msg) (logger).log(concat::Logger::LEVEL_DEBUG, (msg), __FILE__, __LINE__)
	#define LOG_INFO(logger, msg) (logger).log(concat::Logger::LEVEL_INFO, (msg), __FILE__, __LINE__)
	#define LOG_WARN(logger, msg) (logger).log(concat::Logger::LEVEL_WARN, (msg), __FILE__, __LINE__)
	#define LOG_ERROR(logger, msg) (logger).log(concat::Logger::LEVEL_ERROR, (msg), __FILE__, __LINE__)
	#define LOG_FATAL(logger, msg) (logger).log(concat::Logger::LEVEL_FATAL, (msg), __FILE__, __LINE__)

	#define ASSERT_LOG_N(logger_name, expr, msg) concat::Logger::getLogger((logger_name)).assertLog((expr), (msg), __FILE__, __LINE__)
	#define LOG_LOG_N(logger_name, level, msg) concat::Logger::getLogger((logger_name)).log((level), (msg), __FILE__, __LINE__)
	#define LOG_DEBUG_N(logger_name, msg) concat::Logger::getLogger((logger_name)).log(concat::Logger::LEVEL_DEBUG, (msg), __FILE__, __LINE__)
	#define LOG_INFO_N(logger_name, msg) concat::Logger::getLogger((logger_name)).log(concat::Logger::LEVEL_INFO, (msg), __FILE__, __LINE__)
	#define LOG_WARN_N(logger_name, msg) concat::Logger::getLogger((logger_name)).log(concat::Logger::LEVEL_WARN, (msg), __FILE__, __LINE__)
	#define LOG_ERROR_N(logger_name, msg) concat::Logger::getLogger((logger_name)).log(concat::Logger::LEVEL_ERROR, (msg), __FILE__, __LINE__)
	#define LOG_FATAL_N(logger_name, msg) concat::Logger::getLogger((logger_name)).log(concat::Logger::LEVEL_FATAL, (msg), __FILE__, __LINE__)

	#define LOG_LOG_NOEND(logger, level, msg) (logger).log((level), (msg), __FILE__, __LINE__, false)
	#define LOG_LOG_N_NOEND(logger_name, level, msg) concat::Logger::getLogger((logger_name)).log((level), (msg), __FILE__, __LINE__, false)
#else
	#define ASSERT_LOG(logger, expr, msg)
	#define LOG_LOG(logger, level, msg)
	#define LOG_DEBUG(logger, msg)
	#define LOG_INFO(logger, msg)
	#define LOG_WARN(logger, msg)
	#define LOG_ERROR(logger, msg)
	#define LOG_FATAL(logger, msg)

	#define ASSERT_LOG_N(logger, expr, msg)
	#define LOG_LOG_N(logger, level, msg)
	#define LOG_DEBUG_N(logger, msg)
	#define LOG_INFO_N(logger, msg)
	#define LOG_WARN_N(logger, msg)
	#define LOG_ERROR_N(logger, msg)
	#define LOG_FATAL_N(logger, msg)

	#define LOG_LOG_N_NOEND(logger_name, level, msg)
#endif
}

// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------

namespace concat
{
	// Layout interface:
	class Layout
	{
	public:
		virtual ~Layout() {}
		virtual std::string format(const std::string &levelAsString, const std::string &msg, const char *file, int line, bool endLine) = 0;
	};

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	class SimpleLayout : public Layout
	{
	public:
		SimpleLayout();
		std::string format(const std::string &levelAsString, const std::string &msg, const char *file, int line, bool endLine);

	private:
		bool lastLineWasEnded_;
	};
}

// ---------------------------------------------------------------------------------------

namespace concat
{
	// Appender interface.
	// Appenders 'process' messages some how (e.g. write them to a file, store them in a 
	// chunk of memory, send them over the network, etc.).
	class Appender
	{
	public:
		virtual ~Appender() {}
		virtual void appendMsg(const std::string &levelAsString, const std::string &msg, const char *file, int line, bool endLine) = 0;
	};

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// std::ostream Appender:
	class WriterAppender : public Appender
	{
	public:
		virtual ~WriterAppender() {}

		void appendMsg(const std::string &levelAsString, const std::string &msg, const char *file, int line, bool endLine);

	protected:
		WriterAppender() : stream_(NULL), layout_(NULL) {}
		WriterAppender(std::ostream &stream) : stream_(&stream), layout_(NULL) {}

		void setStream(std::ostream &stream) { stream_ = &stream; }

	public:
		// layout pointer copied (i.e. instance must exist throughout Appender's lifetime)
		void setLayout(Layout *layout)
		{
			layout_ = layout;
		}

	private:
		std::ostream *stream_;
		Layout *layout_;
	};

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// std::cout Appender:
	class ConsoleAppender : public WriterAppender
	{
	public:
		ConsoleAppender() : WriterAppender(std::cout) {}
	};

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// std::ofstream Appender:
	//
	// maxNumBackups = 0  : no backups
	// maxNumBackups = -1 : 999 backups
	class FileAppender : public WriterAppender
	{
	public:
		FileAppender();

		FileAppender(const std::string &filename, bool appendingMode = false, int maxNumBackups = 0);
		~FileAppender();

		void open(const std::string &filename, bool appendingMode = false, int maxNumBackups = 0);
		void close();
		bool isOpen() const;

	private:
		std::ofstream file_;

		FileAppender(const FileAppender &); // non-copyable
		FileAppender &operator=(const FileAppender &); // non-copyable
	};
}

// ---------------------------------------------------------------------------------------

namespace concat
{
	// Logger supporting multiple, named loggers, but not hierarchic loggers.
	class Logger
	{
	// Static:
	public:
		static Logger &getLogger(const std::string &name);

	private:
		static std::map<std::string, Logger> loggers_;

	// Non-static:
	public:
		enum Level
		{
			LEVEL_OFF,
			LEVEL_DEBUG,
			LEVEL_INFO,
			LEVEL_WARN,
			LEVEL_ERROR,
			LEVEL_FATAL,
			LEVEL_ALL
		};
		// note: DEBUG and ERROR are already defined in windows headers and give problems

	public:
		Logger();
		~Logger();
		// NOTE: needs to be public for map<>, normally don't create Logger instances manually, but use getLogger()

		void setLevel(Level level);
		Level getLevel() const;
		std::string getLevelAsString(Level level) const;

		void addAppender(Appender &appender);
		void removeAppender(Appender &appender);

		void log(Level level, const std::string &msg, const char *file = "", int line = -1, bool endLine = true);
		void assertLog(bool expr, const std::string &msg);

	private:
		Level level_;
		std::vector<Appender *> appenders_;
	};
}

// ---------------------------------------------------------------------------------------

namespace concat
{
	// Helper for indentation.
	//
	// Use:
	// IndentHelper indent; // global or member for instance
	// ..
	// indent.incr();
	// someFunction();
	// indent.decr();
	// ..
	// someFunction():
	// LOG_INFO_N("my_log", formatStr("%sDoing some function...", indent.getStr()));
	class IndentHelper
	{
	public:
		IndentHelper(int indentSize = 3)
		{
			if (indentSize < 1)
				indentSize = 1;
			if (indentSize > 4)
				indentSize = 4;

			indentSize_ = indentSize;

			indent_ = 0;
		}

		void incr()
		{
			++indent_;
			if (indent_ > 6)
				indent_ = 6;
			updateString();
		}

		void decr()
		{
			--indent_;
			if (indent_ < 0)
				indent_ = 0;
			updateString();
		}

		const char *getStr() const
		{
			return indentStr_;
		}

	private:
		int indentSize_;
		int indent_;
		char indentStr_[6*4+1]; // max 6 indents, max 4 spaces per indent, plus null-terminator

		void updateString()
		{
			for (int i = 0; i < indent_*indentSize_; ++i)
				indentStr_[i] = ' ';
			indentStr_[indent_*indentSize_] = '\0';
		}
	};
}

#endif