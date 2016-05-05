#include "ForceCalibration.hxx"

#include <fstream>

#include "concat/Utilities/Logging.hxx"

// ---------------------------------------------------------------------------------------

ForceCalibration::ForceCalibration()
{
	reset();
}

// ---------------------------------------------------------------------------------------

void ForceCalibration::reset()
{
	measurements_.clear();
	MeasurementData null;
	null.point = Matrix3x1(0.0, 0.0, 0.0);
	null.stepSetFlag = false;

	for (int i = 0; i < NUM_STEPS; ++i)
	{
		measurements_.push_back(null);
	}

	curStep_ = FLOOR_ORIGIN;
}

// ---------------------------------------------------------------------------------------

int ForceCalibration::getStepIdx() const
{
	return curStep_;
}

int ForceCalibration::getNumSteps() const
{
    return NUM_STEPS;
}

void ForceCalibration::decrStepIdx()
{
	--curStep_;
}

void ForceCalibration::incrStepIdx()
{
	++curStep_;
}

// ---------------------------------------------------------------------------------------

const Matrix3x1 &ForceCalibration::getCurStepPoint() const
{
	return measurements_[curStep_].point;
}

void ForceCalibration::setCurStepPoint(const Matrix3x1 &point)
{
	measurements_[curStep_].point = point;
	measurements_[curStep_].stepSetFlag = true;
}

// ---------------------------------------------------------------------------------------

const Matrix3x1 &ForceCalibration::getPoint(int stepIdx) const
{
	if (stepIdx < 0 || stepIdx >= NUM_STEPS)
	{
		static Matrix3x1 null(0.0, 0.0, 0.0);
		return null;
	}
	return measurements_[stepIdx].point;
}

Matrix3x1 &ForceCalibration::getPointForSetting(int stepIdx)
{
	if (stepIdx < 0 || stepIdx >= NUM_STEPS)
	{
		static Matrix3x1 unused(0.0, 0.0, 0.0);
		return unused;
	}
	measurements_[stepIdx].stepSetFlag = true;
	return measurements_[stepIdx].point;
}

// ---------------------------------------------------------------------------------------

void ForceCalibration::printCurStepMessage()
{
	std::string msg;
	if (curStep_ == FLOOR_ORIGIN)
	{
		msg = "floor reversed L origin";
	}
	else if (curStep_ == FLOOR_LEFT)
	{
		msg = "floor reversed L left";
	}
	else if (curStep_ == FLOOR_FWD)
	{
		msg = "floor reversed L forward";
	}
	else if (curStep_ == STRING_LEFT)
	{
		msg = "virtual string on wheel left";
	}
	else if (curStep_ == STRING_RIGHT)
	{
		msg = "virtual string on wheel right";
	}

	LOG_INFO_N("violin_recording_plugin", concat::formatStr("[r]Take sample at %s...", msg.c_str()));
}

// ---------------------------------------------------------------------------------------

bool ForceCalibration::areAllStepsSet() const
{
	for (int i = 0; i < NUM_STEPS; ++i)
	{
		if (measurements_[i].stepSetFlag == false)
			return false;
	}

	return true;
}

// ---------------------------------------------------------------------------------------

bool ForceCalibration::loadFromFile(const char *filename)
{
	std::fstream file;
	file.open(filename, std::ifstream::in | std::ios_base::binary);

	if (!file)
		return false;

	file.seekg(0, std::ios_base::end);
	int size = (int)file.tellg();
	file.seekg(0, std::ios_base::beg);

	if (size != NUM_STEPS*3*sizeof(double))
	{
		file.close();
		return false;
	}

	for (int i = 0; i < NUM_STEPS; ++i)
	{
		file.read((char *)measurements_[i].point.getPtr(), 3*sizeof(double));
		measurements_[i].stepSetFlag = true;
	}

	file.close();

	bool badData = false;
	for (int i = 0; i < NUM_STEPS; ++i)
	{
		double x = measurements_[i].point(0, 0);
		double y = measurements_[i].point(1, 0);
		double z = measurements_[i].point(2, 0);

		// make sure coordinates don't exceed 10 meters:
		if (x < -1000.0 || x > +1000.0)
		{
			measurements_[i].stepSetFlag = false;
			badData = true;
		}

		if (y < -1000.0 || y > +1000.0)
		{
			measurements_[i].stepSetFlag = false;
			badData = true;
		}

		if (z < -1000.0 || z > +1000.0)
		{
			measurements_[i].stepSetFlag = false;
			badData = true;
		}
	}

	if (badData)
		return false;

	return true;
}

// ---------------------------------------------------------------------------------------

bool ForceCalibration::loadFromData(const double *data, int numFloats)
{
	if (numFloats != NUM_STEPS*3)
		return false;

    for (int i = 0; i < NUM_STEPS; ++i)
		measurements_[i].point = Matrix3x1(data[i*3+0], data[i*3+1], data[i*3+2]);

	return true;
}

// ---------------------------------------------------------------------------------------

void ForceCalibration::saveToFile(const char *filename)
{
	std::ofstream file;
	file.open((const char *)filename, std::ios_base::trunc | std::ios_base::binary);

	if (!file)
		return;

	for (int i = 0; i < NUM_STEPS; ++i)
		file.write((const char *)measurements_[i].point.getPtr(), 3*sizeof(double));

	file.close();
}



