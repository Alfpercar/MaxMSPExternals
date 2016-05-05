#include <iostream>
#include <conio.h>
#include <windows.h>

#include "LibertyTracker.hxx"


class StdOutLogger : public LibertyTrackerLogger
{
public:
	void appendMsg(const char *msg)
	{
		std::cout << msg << std::endl;
	}
};

// ---------------------------------------------------------------------------------------

int lastFrameCount = -1;
#include <cassert>
#include <fstream>

int main(int, char*[])
{
	StdOutLogger logger;
	LibertyTracker libertyTracker(&logger);

	libertyTracker.connect();

	libertyTracker.configure();

	libertyTracker.startReceivingEvents();

	while (1)
	{
		LibertyTracker::ItemDataIterator iter;
		int numItems = libertyTracker.queryFrames(iter);

		for (int i = 0; i < (numItems/2); ++i)
		{
			// test continuity of frame count:
			int frameCount = iter.item().frameCount;
			if (lastFrameCount != -1)
			{
				if (frameCount - lastFrameCount != 1)
				{
					std::cout << "Skipping: " << frameCount - lastFrameCount <<  " (full frames) ... (fc = " << frameCount << ", last fc = " << lastFrameCount << ")" << std::endl;
				}
			}
			lastFrameCount = frameCount;

			// sensor 1:
			std::cout << (int)iter.item().header.station << ", " << iter.item().position[0] << ", " << iter.item().position[1] << ", " << iter.item().position[2] << ", " << iter.item().orientation[0] << ", " << iter.item().orientation[1] << ", " << iter.item().orientation[2] << std::endl;
			iter.next();

			// sensor 2:
			std::cout << (int)iter.item().header.station << ", " << iter.item().position[0] << ", " << iter.item().position[1] << ", " << iter.item().position[2] << ", " << iter.item().orientation[0] << ", " << iter.item().orientation[1] << ", " << iter.item().orientation[2] << std::endl;
			iter.next();
		}

		if (kbhit())
		{
			break;
		}

		::Sleep(500);
	}

	libertyTracker.stopReceivingEvents();

	libertyTracker.disconnect();

	return 0;
}