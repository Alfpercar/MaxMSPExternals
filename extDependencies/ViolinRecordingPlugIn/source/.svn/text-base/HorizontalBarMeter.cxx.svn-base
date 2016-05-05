#include "HorizontalBarMeter.hxx"

#include "Exceptions.hxx"

#include <cfloat> // XXX: msvc specific

// ---------------------------------------------------------------------------------------

HorizontalBarMeter::HorizontalBarMeter(const String& componentName) : Component(componentName)
{
	curValue_ = 0.0f;
	barColour_ = Colours::red;

	startTimer(250); // every 250 ms
}

HorizontalBarMeter::~HorizontalBarMeter()
{
	stopTimer();
}

// ---------------------------------------------------------------------------------------

void HorizontalBarMeter::setBarColour(const Colour &colour)
{
	barColour_ = colour;
}

// ---------------------------------------------------------------------------------------

void HorizontalBarMeter::postValue(float value)
{
	if (_isnan(value)) // XXX: msvc specific
	{
		// just in case calibration is very wrong (corrupt file, etc.)
		return;
	}

	if (value > 1.0f)
		value = 1.0f;

	if (value < 0.0f)
		value = 0.0f;

	curValue_ = value; // overwrites value even if it hasn't yet been displayed
}

// ---------------------------------------------------------------------------------------

void HorizontalBarMeter::paint(Graphics &g)
{
	const int w = getWidth();
	const int h = getHeight();

	// Draw background:
	SolidColourBrush backgroundBrush(Colours::white);
	g.setBrush(&backgroundBrush);
	g.fillRect(0, 0, w, h);

	// Draw black border:
	SolidColourBrush borderBrush(Colours::black);
	g.setBrush(&borderBrush);
	g.drawRect(0, 0, w, h, 1);

	SolidColourBrush barBrush(barColour_);
	g.setBrush(&barBrush);
//	g.fillRect(1, 1, int((w-2)*curValue_ + 0.5f), h-2); // XXX: round to int
	g.fillRect(1.f, 1.f, (w-2)*curValue_, (float)(h-2));
}

// ---------------------------------------------------------------------------------------

void HorizontalBarMeter::timerCallback()
{
BEGIN_IGNORE_EXCEPTIONS
	// XXX: here should actually look which values are send and only update the rect/rects needed
	repaint(); // asynchronous repaint
END_IGNORE_EXCEPTIONS("HorizontalBarMeter::timerCallback()")
}
