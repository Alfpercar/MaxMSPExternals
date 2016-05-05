#ifndef INCLUDED_DUMMYTIMER_HXX
#define INCLUDED_DUMMYTIMER_HXX

class DummyTimer
{
public:
	virtual ~DummyTimer()
	{
		stopTimer();
	}

	virtual void timerCallback() = 0;

	void startTimer(const int intervalInMilliseconds) throw ()
	{
		if (interval_ <= 0)
			return;

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
	DummyTimer()
	{
		interval_ = -1;
	}

	DummyTimer(const DummyTimer &other) throw ()
	{
		interval_ = other.interval_;
	}

private:
	int interval_;
};

#endif