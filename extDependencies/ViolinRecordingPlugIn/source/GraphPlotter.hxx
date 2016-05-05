#ifndef INCLUDED_GRAPHPLOTTER_HXX
#define INCLUDED_GRAPHPLOTTER_HXX

#include "windows.h"
#include "juce.h"

#include "LockFreeFifo.hxx"
#include "AtomicFlag.hxx"

#include "ViolinRecordingPlugInConfig.hxx"
#if (DISABLE_TIMERS != 0)
#include "DummyTimer.hxx"
#define Timer DummyTimer
#endif

class GraphPlotter : public Component, private Timer
{
public:
	GraphPlotter(const String &componentName);
	virtual ~GraphPlotter();

	// Set up:
	void setNumValuesPerPixel(int numValuesPerPixel);
	void setLineWidth(float lineWidth);
	void setYLim(float y0, float y1);
	void setBounds(int x, int y, int width, int height); // overload of Component::setBounds()
	void setHorizontalOverlay(float realValue, bool enable = true);
	void setScrolling(bool enabled);
	void setNumDisplayEnabled(bool enabled);

	int addPlot(const Colour &colour);
	void removePlot(int n);

	// Start/stop:
	void start(int updateIntervalMilliseconds, double productionRate, double toleranceFactor);
	void stop();

	// Methods while running:
	void clear();
	void postValue(int plotIdx, float value);

public:
	juce_UseDebuggingNewOperator

protected:
	virtual void paint(Graphics &g); // implementation of Component::paint()

	virtual void paintNumDisplay(Graphics &g, float value);
	virtual void paintHorizontalDashedLine(Graphics &g, float dashGapPx);
	virtual void paintBackgroundToCache();
	virtual void paintBackgroundPartialToCache(int xStart, int xEnd);
	virtual void paintPlotToCacheAndComputeAvgValue(int plotIdx, int dataSize);

	bool numDisplayEnabled_;

private:
	void timerCallback(); // implementation of Timer::timerCallback()

private:
	struct Plotter
	{
		Plotter()
		{
			isActive_ = false;
			yPos_ = 0.0f;
			avgValue_ = 0.0f;
		}

		bool isActive_;
		SolidColourBrush lineBrush_;

		LockFreeFifo<float> queue_; // value queue
		float yPos_;

		float avgValue_;
	};

	enum
	{
		MAX_NUM_PLOTTERS = 4
	};

	Plotter plotters_[MAX_NUM_PLOTTERS];

	int xPos_;

	float scale_;
	float offset_;

	bool isHorizontalOverlayEnabled_;
	float horizontalOverlayValueReal_;

	bool scrollingEnabled_;

	Image *cachedImage_;
	Image *cachedImageTmp_; // temp. image for scrolling

	int numValuesPerPixel_;
	float plotLineWidth_;

	AtomicFlag clearPlotFlag_;

	float normalizeValue(float value);
	float unnormalizeValue(float value);

	void clearPlotInternal();

	GraphPlotter(const GraphPlotter &); // non-copyable
	GraphPlotter &operator=(const GraphPlotter &); // non-copyable
};

// ---------------------------------------------------------------------------------------

class MultiGraphPlotter
{
public:
private:
};

#endif
