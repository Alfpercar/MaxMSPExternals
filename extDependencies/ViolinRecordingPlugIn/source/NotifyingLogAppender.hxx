#ifndef INCLUDED_NOTIFYINGLOGAPPENDER_HXX
#define INCLUDED_NOTIFYINGLOGAPPENDER_HXX

#include "windows.h"
#include "juce.h"

class ViolinRecordingPlugInEffect;

#include "concat/Utilities/Logging.hxx"

// Buffering Appender which has a maximum size (appending more messages will overwrite old messages) 
// and notifies a Listener on each append.
class NotifyingLogAppender : public concat::Appender
{
public:
	NotifyingLogAppender();

	void setListener(ViolinRecordingPlugInEffect *listener);
	
	void appendMsg(const std::string &levelAsString, const std::string &msg, const char *file, int line, bool endLine);

	int getSize() const;
	String getItem(int idx) const;
	Colour getItemColour(int idx) const;

	void clear();

private:
	ViolinRecordingPlugInEffect *listener_;

	StringArray list_;
	int listCapacity_;
	int listSize_;
	int writeIndex_;

	String curLine_; // for multiple appends per line

	void appendMsg2(const std::string &levelAsString, const std::string &msg, const char *file, int line, bool endLine);
};

#endif


