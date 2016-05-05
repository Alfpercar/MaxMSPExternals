#include "GraphPlotter.hxx"

#include "Exceptions.hxx"
#include <cfloat> // XXX: msvc specific

#include "concat/Utilities/FloatToInt.hxx"

// ---------------------------------------------------------------------------------------

#define NUM_BOX_WIDTH 70
#define NUM_BOX_HEIGHT 15
#define NUM_BOX_MARGIN 5 // 5 px from top right edge

// ---------------------------------------------------------------------------------------

GraphPlotter::GraphPlotter(const String &componentName) : Component(componentName)
{
	cachedImage_ = NULL;
	cachedImageTmp_ = NULL;

	clearPlotFlag_.set(false);

	xPos_ = 0;

	addPlot(Colours::red); // by default activate one plot

	setNumValuesPerPixel(6); // each plotted pixel corresponds to 6 values
	setLineWidth(1.25f); // 1.25 px line width
	setYLim(0.0f, 1.0f);
	setHorizontalOverlay(0.0f, false);
	setScrolling(false);
	setNumDisplayEnabled(true);
}

GraphPlotter::~GraphPlotter()
{
	stop();

	delete cachedImage_;
	cachedImage_ = NULL;

	delete cachedImageTmp_;
	cachedImageTmp_ = NULL;
}

// ---------------------------------------------------------------------------------------

void GraphPlotter::setNumValuesPerPixel(int numValuesPerPixel)
{
	if (isTimerRunning())
		return;

	numValuesPerPixel_ = numValuesPerPixel;
}

void GraphPlotter::setLineWidth(float lineWidth)
{
	if (isTimerRunning())
		return;

	plotLineWidth_ = lineWidth;
}

// Note: Also clears the plot!
void GraphPlotter::setYLim(float y0, float y1)
{
	if (isTimerRunning())
		return;

	// valueNorm = offset + scale*valueReal
	// scale = 1/range = 1/(y1 - y0)
	// offset = -y0/range = (-y0)/(y1 - y0)

	assert(y0 < y1);
	if (y0 >= y1)
	{
		// use default values
		y0 = 0.0f;
		y1 = 1.0f;
	}

	scale_ = 1.0f/(y1 - y0);
	offset_ = (-y0)/(y1 - y0);

	// repaint background (zero line might have changed):
	setBounds(getX(), getY(), getWidth(), getHeight()); // updates background and repaints
}

// also clears plot!
void GraphPlotter::setBounds(int x, int y, int w, int h)
{
	if (isTimerRunning())
		return;

	Component::setBounds(x, y, w, h);

	if (w > 2 && h > 2)
	{
		w = w-2;
		h = h-2;

		// Allocate image cache:
		if (cachedImage_ != NULL)
		{
			delete cachedImage_;
			cachedImage_ = NULL;
		}

		cachedImage_ = new Image(Image::RGB, w, h, false);

		// Allocate secondary image cache (for scrolling):
		if (cachedImageTmp_ != NULL)
		{
			delete cachedImageTmp_;
			cachedImageTmp_ = NULL;
		}

		cachedImageTmp_ = new Image(Image::RGB, w, h, false);

		// Fill cached image with default (no data to plot):
		paintBackgroundToCache();

		// Repaint:
		repaint();
	}
}

void GraphPlotter::setHorizontalOverlay(float realValue, bool enable)
{
	if (isTimerRunning())
		return;

	isHorizontalOverlayEnabled_ = enable;

	const float norm = normalizeValue(realValue);
	if (norm > 1.f)
		realValue = unnormalizeValue(1.0f);
	if (norm < 0.f)
		realValue = unnormalizeValue(0.0f);
	horizontalOverlayValueReal_ = realValue;
}

void GraphPlotter::setScrolling(bool enabled)
{
	if (isTimerRunning())
		return;

	scrollingEnabled_ = enabled;
}

void GraphPlotter::setNumDisplayEnabled(bool enabled)
{
    numDisplayEnabled_ = enabled;
}

// ---------------------------------------------------------------------------------------

int GraphPlotter::addPlot(const Colour &colour)
{
	if (isTimerRunning())
		return -1;

	int idx = -1;
	int i=0;
	for (i = 0; i < MAX_NUM_PLOTTERS; ++i)
	{
		if (!plotters_[i].isActive_)
		{
			idx = i;
			break;
		}
	}

	if (idx != -1)
	{
		plotters_[i].isActive_ = true;
		plotters_[i].lineBrush_.setColour(colour);
	}
	
	return idx;
}

void GraphPlotter::removePlot(int n)
{
	if (isTimerRunning())
		return;

	if (n < 0 || n >= MAX_NUM_PLOTTERS)
		return;

	plotters_[n].isActive_ = false;	
	plotters_[n].queue_.reserve(0);
}

// ---------------------------------------------------------------------------------------

void GraphPlotter::start(int updateIntervalMilliseconds, double productionRate, double toleranceFactor)
{
	if (isTimerRunning())
		stop(); // blocks until timer callback is done

	const int consumptionIntervalMilliseconds = updateIntervalMilliseconds;
	const int peakProductionMax = 1024; // on frame drop and interpolation a lot of frames may be generated at once
	const int bufferSize = jmax(concat::ceil_int(productionRate*consumptionIntervalMilliseconds/1000.0f*toleranceFactor), peakProductionMax);
	for (int i = 0; i < MAX_NUM_PLOTTERS; ++i)
	{
		if (plotters_[i].isActive_)
		{
			plotters_[i].queue_.reserve(bufferSize);
		}
	}

	startTimer(updateIntervalMilliseconds);
}

void GraphPlotter::stop()
{
	if (!isTimerRunning())
		return;

	stopTimer(); // (blocking)
}

// ---------------------------------------------------------------------------------------

void GraphPlotter::clear()
{
	if (isTimerRunning())
	{
		clearPlotFlag_.set(true);
	}
	else
	{
		const MessageManagerLock mmLock; // might be called from non-gui thread

		clearPlotInternal();
		repaint();
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void GraphPlotter::postValue(int plotIdx, float value)
{
	if (plotIdx < 0 || plotIdx >= MAX_NUM_PLOTTERS)
		return;

	if (!plotters_[plotIdx].isActive_)
		return;

	if (_isnan(value)) // XXX: msvc specific
	{
		// just in case calibration is very wrong (corrupt file, etc.)
		return;
	}

	const float norm = normalizeValue(value);

	// correct real value if normalized value is out of bounds:
	if (norm > 1.f)
		value = unnormalizeValue(1.0f);
	if (norm < 0.f)
		value = unnormalizeValue(0.0f);

	// store real value in queue:
	plotters_[plotIdx].queue_.put(value);
}

// ---------------------------------------------------------------------------------------

void GraphPlotter::paint(Graphics &g)
{
	if (cachedImage_ == NULL)
		return;

	const int w = getWidth();
	const int h = getHeight();

	// Note:
	// The g Graphic object internally clips all drawing operations.
	
	// Draw black border:
	SolidColourBrush borderBrush(Colours::black);
	g.setBrush(&borderBrush);
	g.drawRect(0, 0, w, h, 1);

	// Draw plot cache image:
	g.drawImage(cachedImage_, 1, 1, w-2, h-2, 0, 0, w-2, h-2);

	// Draw option horizontal line overlay:
	if (isHorizontalOverlayEnabled_)
		paintHorizontalDashedLine(g, 4.0f);

	// Draw numerical display:
	if (numDisplayEnabled_)
		paintNumDisplay(g, plotters_[0].avgValue_);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void GraphPlotter::paintNumDisplay(Graphics &g, float value)
{
	const int w = getWidth();
	const int h = getHeight();

	SolidColourBrush backgroundBrush(Colours::white);
	g.setBrush(&backgroundBrush);
	g.fillRect(w - NUM_BOX_MARGIN - NUM_BOX_WIDTH, NUM_BOX_MARGIN, NUM_BOX_WIDTH, NUM_BOX_HEIGHT);
	SolidColourBrush borderBrush(Colours::black);
	g.setBrush(&borderBrush);
	g.drawRect(w - NUM_BOX_MARGIN - NUM_BOX_WIDTH, NUM_BOX_MARGIN, NUM_BOX_WIDTH, NUM_BOX_HEIGHT, 1);
	SolidColourBrush lineBrush(Colours::red);
	g.setBrush(&lineBrush);
	String valueStr = String::formatted(T("%4.3f"), value);
	g.drawText(valueStr, w - NUM_BOX_MARGIN - NUM_BOX_WIDTH, NUM_BOX_MARGIN, NUM_BOX_WIDTH, NUM_BOX_HEIGHT, Justification::centred, false);
}

void GraphPlotter::paintHorizontalDashedLine(Graphics &g, float dashGapPx)
{
	SolidColourBrush horizontalOverlayBrush(Colours::darkred);
	g.setBrush(&horizontalOverlayBrush);

	const int w = cachedImage_->getWidth();
	const int h = cachedImage_->getHeight();

	const float valueNorm = 1.0f - normalizeValue(horizontalOverlayValueReal_);
	assert(valueNorm >= 0.0f && valueNorm <= 1.0f);

	float yPos = 1.0f + valueNorm*h;
	float xPos = 1.0f;
	// skip border

	while (1)
	{
		g.drawLine(xPos, yPos, xPos + dashGapPx, yPos, 0.5f);
		xPos += 2.0f*dashGapPx;

		if (xPos >= (float)w)
			break;
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GraphPlotter::paintBackgroundToCache()
{
	Graphics g(*cachedImage_);
	const int w = cachedImage_->getWidth();
	const int h = cachedImage_->getHeight();

	// Draw background:
	g.fillAll(Colours::white);

	// Draw zero line:
	const float zeroLine = normalizeValue(0.0f);
	if (zeroLine != 0.0f)
	{
		SolidColourBrush zeroLineBrush(Colours::grey);
		g.setBrush(&zeroLineBrush);
		g.drawLine(0, zeroLine*h, (float)w, zeroLine*h, 1.0f);
	}
}

void GraphPlotter::paintBackgroundPartialToCache(int xStart, int xEnd)
{
	Graphics g(*cachedImage_);
	const int w = cachedImage_->getWidth();
	const int h = cachedImage_->getHeight();

	SolidColourBrush backgroundBrush(Colours::white);

	g.setBrush(&plotters_[0].lineBrush_); // invert colours of background/first plot line (red typically)

	g.fillRect(xStart, 0, xEnd, h);

	const float zeroLine = normalizeValue(0.0f);
	if (zeroLine != 0.0f)
	{
		SolidColourBrush zeroLineBrush(Colours::grey);
		g.setBrush(&zeroLineBrush);
		g.drawLine((float)xStart, zeroLine*h, (float)xEnd, zeroLine*h, 1.0f);
	}
}

void GraphPlotter::paintPlotToCacheAndComputeAvgValue(int plotIdx, int dataSize)
{
	Graphics g(*cachedImage_);
	const int w = cachedImage_->getWidth();
	const int h = cachedImage_->getHeight();

	Plotter &p = plotters_[plotIdx];

	g.setBrush(&p.lineBrush_);

	int x = xPos_;
	float sum = 0.0f;
	int numPixels = dataSize/numValuesPerPixel_; // floor to int
	for (int i = 0; i < numPixels; ++i)
	{
		float yMin = 1.0e16f;
		float yMax = -1.0e16f;

		for (int j = 0; j < numValuesPerPixel_; ++j)
		{
			float yTmp;
			p.queue_.get(yTmp);
			sum += yTmp;
			yMin = jmin(yTmp, yMin);
			yMax = jmax(yTmp, yMax);
		}

		float y;
		if (-yMin > yMax)
		{
			y = yMin;
		}
		else
		{
			y = yMax;
		}

		y = normalizeValue(y); // normalize [0..1]
		y = (1.f - y)*(h - 1); // scale and invert

		g.drawLine((float)x, p.yPos_, (float)(x + 1), y, plotLineWidth_);

		x += 1;
		if (x >= w && !scrollingEnabled_)
			x -= w; // wrap around

		p.yPos_ = y;
	}
	xPos_ = x;

	// Compute average value (used to draw numerical display on paint()):
	int numValuesProcessed = numPixels*numValuesPerPixel_;
	if (numValuesProcessed > 0)
		p.avgValue_ = sum/(float)(numValuesProcessed);
}

// ---------------------------------------------------------------------------------------

void GraphPlotter::timerCallback()
{
BEGIN_IGNORE_EXCEPTIONS
	Graphics cachedImageGraph(*cachedImage_);
	const int w = cachedImage_->getWidth();
	const int h = cachedImage_->getHeight();

	if (clearPlotFlag_.isSet())
	{
		clearPlotInternal();

		clearPlotFlag_.set(false);
	}
	else
	{
		int plotDataSize = -1;
		for (int i = 0; i < MAX_NUM_PLOTTERS; ++i)
		{
			if (plotters_[i].isActive_)
			{
				int n = plotters_[i].queue_.getReadAvail();

				if (plotDataSize == -1)
					plotDataSize = n;
				else
					plotDataSize = jmin(n, plotDataSize);
			}
		}

		if (plotDataSize != 0)
		{
			if (!scrollingEnabled_)
			{
				// Erase previous plot in part that will be overwritten:
				const int numPixels = plotDataSize/numValuesPerPixel_;
				const int xBegin = xPos_;
				const int xEnd = xBegin + numPixels;

				if (xEnd >= w)
				{
					paintBackgroundPartialToCache(xBegin, w - xBegin);
					paintBackgroundPartialToCache(0, xEnd - w);
				}
				else
				{
					paintBackgroundPartialToCache(xBegin, xEnd - xBegin);
				}
			}
			else
			{
				// Scroll back:
				const int numPixels = plotDataSize/numValuesPerPixel_;
				const int scrollBack = jmax(0, (xPos_ + numPixels) - w);

				if (scrollBack > 0)
				{
					// Copy to temp. Image:
					Graphics tmp(*cachedImageTmp_);
					tmp.drawImage(cachedImage_, 0, 0, xPos_ - scrollBack, h, scrollBack, 0, xPos_ - scrollBack, h);

					// Clear cached image:
					paintBackgroundToCache();

					// Copy back:
					cachedImageGraph.drawImage(cachedImageTmp_, 0, 0, xPos_ - scrollBack, h, 0, 0, xPos_ - scrollBack, h);

					// Adjust current x position:
					xPos_ -= scrollBack;
				}
			}

			// Draw part of plot that has been updated:
			int x = xPos_;
			for (int i = 0; i < MAX_NUM_PLOTTERS; ++i)
			{
				if (plotters_[i].isActive_)
				{
					xPos_ = x; // reset x position for each plot
					paintPlotToCacheAndComputeAvgValue(i, plotDataSize); // advances xPos_ internally
				}
			}
		}
	}

	// XXX: here should actually look which values are send and only update the rect/rects needed
	repaint(); // asynchronous repaint
END_IGNORE_EXCEPTIONS("GraphPlotter::timerCallback()")
}

// ---------------------------------------------------------------------------------------

float GraphPlotter::normalizeValue(float value)
{
	// NOTE: no checking/clamping of ranges! (do outside)
	return offset_ + scale_*value;
}

float GraphPlotter::unnormalizeValue(float value)
{
	// NOTE: no checking/clamping of ranges! (do outside)
	return (value - offset_)/scale_;
}

// ---------------------------------------------------------------------------------------

void GraphPlotter::clearPlotInternal()
{
	Graphics cachedImageGraph(*cachedImage_);
	const int w = cachedImage_->getWidth();
	const int h = cachedImage_->getHeight();

	// Clear cached image:
	paintBackgroundToCache();

	// Reset x-position:
	xPos_ = 0;

	for (int i = 0; i < MAX_NUM_PLOTTERS; ++i)
	{
		if (plotters_[i].isActive_)
		{
			// Discard all data in buffer:
			plotters_[i].queue_.clearBySettingReadIdxToWriteIdx();

			// Reset average value (for numerical display drawn on paint()):
			plotters_[i].avgValue_ = 0.0f;

			// Reset y-position:
			plotters_[i].yPos_ = 0.0f;
		}
	}
}



