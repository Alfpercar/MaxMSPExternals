#ifndef INCLUDED_ARDUINOFRAME_HXX
#define INCLUDED_ARDUINOFRAME_HXX

// XXX: perhaps rename to Arduino.hxx and place 'all' arduino code here

#include "ViolinRecordingPlugInConfig.hxx"

struct ArduinoFrame
{
	unsigned int frameCount;
	int valueGages;
	int valueLoadCell;
#if (ENABLE_OPTICAL_SENSOR != 0)
	int valueOptical;
#endif
	int extSyncFlag;
};

#endif