#ifndef INCLUDED_CONCAT_LOGGINGPROGRESS_HXX
#define INCLUDED_CONCAT_LOGGINGPROGRESS_HXX

#include "concat/Utilities/Logging.hxx"
#include "concat/Utilities/FloatToInt.hxx"

namespace concat
{
	class GenericProgressListener
	{
	public:
		virtual void onTick(int numTicks) = 0;
	};

	class GenericProgressToConsole : public GenericProgressListener
	{
	public:
		GenericProgressToConsole(Logger &logger)
		{
			logger_ = &logger;
			tickCount_ = 0;
			curPercentFixed_ = 0;
		}

		void onTick(int numTicks)
		{
			if (tickCount_ < 0 || tickCount_ >= numTicks)
				return;

			++tickCount_;

			float percent = 100.0f*(float)(tickCount_)/(float)(numTicks);
			int percentFixed = round_int(percent);
			int deltaPercent = percentFixed - curPercentFixed_;
			while (deltaPercent > 0)
			{
				LOG_LOG_NOEND(*logger_, concat::Logger::LEVEL_INFO, ".");
				--deltaPercent;
			}
			curPercentFixed_ = percentFixed;

			if (tickCount_ == numTicks)
				LOG_INFO(*logger_, ""); // new line
		}

	private:
		Logger *logger_;
		int tickCount_;
		int curPercentFixed_;
	};
}

#endif

