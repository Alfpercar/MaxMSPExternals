#ifndef INCLUDED_HORIZONTALBARMETER_HXX
#define INCLUDED_HORIZONTALBARMETER_HXX

#include "juce.h"

#include "ViolinRecordingPlugInConfig.hxx"
#if (DISABLE_TIMERS != 0)
#include "DummyTimer.hxx"
#define Timer DummyTimer
#endif

class HorizontalBarMeter : public Component, private Timer, public SettableTooltipClient
{
public:
	HorizontalBarMeter(const String& componentName);
	virtual ~HorizontalBarMeter();

	void setBarColour(const Colour &colour);
	// XXX: should use Component::setColour(), findColour(), colourChanged(), etc.

	// Client methods:
	void postValue(float value);

public:
	juce_UseDebuggingNewOperator

protected:
	// Implementation of Component::paint().
	virtual void paint(Graphics &g);

private:
	// Implementation of Timer::timerCallback().
	void timerCallback();

private:
	float curValue_;
	Colour barColour_;
};

#endif
