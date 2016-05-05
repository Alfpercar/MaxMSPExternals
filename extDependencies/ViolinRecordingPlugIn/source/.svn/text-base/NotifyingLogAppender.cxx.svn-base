#include "NotifyingLogAppender.hxx"

#include "ViolinRecordingPlugInEffect.hxx"

const int MAX_NUM_LINES = 64;
const int MAX_LINE_WIDTH = 62;

NotifyingLogAppender::NotifyingLogAppender()
{
	listener_ = NULL;

	listCapacity_ = MAX_NUM_LINES; // maximum number of lines
	for (int i = 0; i < listCapacity_; ++i)
		list_.add(String::empty);
	listSize_ = 0;
	writeIndex_ = 0;

	curLine_ = String::empty;
}

void NotifyingLogAppender::setListener(ViolinRecordingPlugInEffect *listener)
{
	listener_ = listener;
}

void NotifyingLogAppender::appendMsg(const std::string &levelAsString, const std::string &msg, const char *file, int line, bool endLine)
{
	std::string msgRemain = msg;
	std::string colourMarkupPrefix = "";
	if (msg.find("[r]") == 0)
		colourMarkupPrefix = "[r]";

	while (msgRemain.size() - colourMarkupPrefix.size() > MAX_LINE_WIDTH)
	{
		appendMsg2(levelAsString, msgRemain.substr(0, MAX_LINE_WIDTH), file, line, endLine);
		msgRemain = colourMarkupPrefix + msgRemain.substr(MAX_LINE_WIDTH, msgRemain.size()-MAX_LINE_WIDTH);
	}

	appendMsg2(levelAsString, msgRemain, file, line, endLine);
}

void NotifyingLogAppender::appendMsg2(const std::string &levelAsString, const std::string &msg, const char *file, int line, bool endLine)
{
	curLine_ += msg.c_str();

	if (endLine)
	{
		list_.set(writeIndex_, curLine_);
		curLine_ = String::empty;

		++writeIndex_;
		if (writeIndex_ >= listCapacity_)
			writeIndex_ -= listCapacity_; // wrap

		if (listSize_ < listCapacity_)
			++listSize_;

		if (listener_ != NULL)
		{
			listener_->updateLoggerListBoxAsynchronousSignal.sendChangeMessage(NULL);
			// XXX: also sendChangeMessage() instead of sendSynchronousChangeMessage()?
		}
	}
}

int NotifyingLogAppender::getSize() const
{
	return listSize_;
}

String NotifyingLogAppender::getItem(int idx) const
{
	if (idx < 0 || idx >= listSize_)
		return String::empty;

	// XXX: not thread safe!!
	if (listSize_ == listCapacity_)
	{
		idx = writeIndex_ + idx;
		if (idx >= listCapacity_)
			idx -= listCapacity_; // wrap
	}

	if (list_[idx].startsWith(T("[r]")))
		return list_[idx].substring(3);
	else
		return list_[idx];
}

Colour NotifyingLogAppender::getItemColour(int idx) const
{
	if (idx < 0 || idx >= listSize_)
		return Colours::black;

	// XXX: not thread safe!!
	if (listSize_ == listCapacity_)
	{
		idx = writeIndex_ + idx;
		if (idx >= listCapacity_)
			idx -= listCapacity_; // wrap
	}

	if (list_[idx].startsWith(T("[r]")))
		return Colours::red;
	else
		return Colours::black;
}

void NotifyingLogAppender::clear()
{
	listSize_ = 0;
	writeIndex_ = 0;
	curLine_ = String::empty;

	list_.clear();
}


