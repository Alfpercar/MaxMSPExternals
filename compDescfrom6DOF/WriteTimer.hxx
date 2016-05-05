#ifndef INCLUDED_WriteTimer_HXX
#define INCLUDED_WriteTimer_HXX

//#include "ext.h" // Required for all Max external objects


class WriteTimer
{
public:
	virtual ~WriteTimer()
	{
		stopTimer();
	}

	virtual void timerCallback() = 0;

	void startTimer(const int intervalInMilliseconds) throw ()
	{
		//if (interval_ <= 0)
		//	return;

		interval_ = intervalInMilliseconds;
	}

	void stopTimer() throw ()
	{
		interval_ = -1;
	}

	bool isTimerRunning() const throw ()
	{
		return (interval_ > 0);
	}

	int getTimerInterval() const throw ()
	{
		return interval_;
	}

protected:
	//float *m_clock;
	WriteTimer()
	{
		interval_ = -1;	
	}

	WriteTimer(const WriteTimer &other) throw ()
	{
		interval_ = other.interval_;
	}

private:
	int interval_;
};

#endif