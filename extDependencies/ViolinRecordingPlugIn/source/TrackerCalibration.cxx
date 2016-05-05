#include "TrackerCalibration.hxx"

#include <string>
#include <ostream>
#include <sstream>
#include <fstream>

#include "concat/Utilities/Logging.hxx"
#include "concat/Utilities/Filename.hxx"
#include "ComputeDescriptors.hxx"

#define EXTENDED_DEBUG_LOG 0

using namespace concat;

// ---------------------------------------------------------------------------------------

namespace
{
	// utility for reading csv files using fstreams
	std::istream &comma(std::istream &in)
	{
		if ((in >> std::ws).get() != in.widen(','))
			in.setstate(std::ios_base::failbit);
		else
			in.ignore();

		return in;
	}
}

// ---------------------------------------------------------------------------------------

TrackerCalibration::TrackerCalibration()
{
	// Set some values to avoid getting NaNs when using an incomplete calibration:
	for (int i = 0; i < NUM_STEPS; ++i)
	{
		betas_[i] = Matrix3x1(0.0, 0.0, 0.0);
	}
	
	resetSetFlags();
}

// ---------------------------------------------------------------------------------------

bool TrackerCalibration::loadFromFile(const char *filename)
{
	// Open file:
	std::fstream calibFile;
	calibFile.open(filename, std::ifstream::in);

	if (!calibFile)
		return false;

	std::string line;

	resetSetFlags();

	for (int i = 0; i < NUM_STEPS; ++i)
	{
		if (!std::getline(calibFile, line).fail())
		{
			std::stringstream lineStream(line);
			lineStream >> betas_[i](0,0) >> comma >> betas_[i](1,0) >> comma >> betas_[i](2,0);
			if (lineStream.fail())
			{
				calibFile.close();
				return false;
			}
		}
		else
		{
			calibFile.close();
			return false;
		}

		setStepSetFlag(i);
	}

	// Close file:
	calibFile.close();

	return true;
}

bool TrackerCalibration::loadFromData(const double *data, int numFloats)
{
	if (data == NULL || numFloats != NUM_STEPS*3)
		return false;

	for (int i = 0; i < NUM_STEPS; ++i)
	{
		betas_[i] = Matrix3x1(data[i*3+0], data[i*3+1], data[i*3+2]);
	}

	return true;
}

// ---------------------------------------------------------------------------------------

bool TrackerCalibration::saveToFile(const char *filename, bool failIfExists)
{
	concat::Filename f(filename);
	if (failIfExists && f.exists())
		return false;

	std::fstream calibFile;
	calibFile.open(filename, std::ifstream::out);

	if (!calibFile)
		return false;

	calibFile.setf(std::ios::scientific, std::ios::floatfield);
	calibFile.precision(9);

	// Store fields in same order as calibration process:
	// NOTE: NOT compatible with version 1 calibration files
	// which were frog lhs, tip lhs, (frog rhs, tip rhs), 
	// str_n_bridge, str_n_fb, str_n+1_bridge, str_n+1_fb

	for (int i = 0; i < NUM_STEPS; ++i)
	{
		calibFile << betas_[i](0, 0) << ", " << betas_[i](1, 0) << ", " << betas_[i](2, 0) << std::endl;
	}

	calibFile.close();

	return true;
}

// ---------------------------------------------------------------------------------------

// get all data as a single comma separated string, for displaying (logging) purposes
const char *TrackerCalibration::getAsCsvString() const
{
	std::stringstream ss;
	ss.setf(std::ios::scientific, std::ios::floatfield);
	ss.precision(9);

	for (int i = 0; i < NUM_STEPS; ++i)
	{
		ss << betas_[i](0, 0) << ", " << betas_[i](1, 0) << ", " << betas_[i](2, 0);
		if (i < NUM_STEPS - 1)
			ss << ", ";
	}

	static std::string s;
	s = ss.str();

	return s.c_str();
}

// ---------------------------------------------------------------------------------------

void TrackerCalibration::resetSetFlags()
{
	for (int i = 0; i < NUM_STEPS; ++i)
	{
		stepSetFlags_[i] = false;
	}
}

bool TrackerCalibration::areAllStepFlagsSet() const
{
	for (int i = 0; i < NUM_STEPS; ++i)
	{
		if (stepSetFlags_[i] == false)
			return false;
	}

	return true;
}

// ---------------------------------------------------------------------------------------

const char *TrackerCalibration::getDescription(int index) const
{
	switch (index)
	{
	case STR1_BRIDGE:
		return "string 1 (right), bridge";
	case STR2_BRIDGE:
		return "string 2, bridge";
	case STR3_BRIDGE:
		return "string 3, bridge";
	case STR4_BRIDGE:
		return "string 4, bridge";
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case STR1_WOOD:
		return "wood under string 1 (right), beginning finger board";
	case STR2_WOOD:
		return "wood under string 2, beginning finger board";
	case STR3_WOOD:
		return "wood under string 3, beginning finger board";
	case STR4_WOOD:
		return "wood under string 4, beginning finger board";
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case STR1_FB:
		return "string 1 (right), end finger board";
	case STR2_FB:
		return "string 2, end finger board";
	case STR3_FB:
		return "string 3, end finger board";
	case STR4_FB:
		return "string 4, end finger board";
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case BOW_FROG_LHS:
		return "bow frog, left-hand side";
	case BOW_FROG_RHS:
		return "bow frog, right-hand side";
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case BOW_TIP_LHS:
		return "bow tip, left-hand side";
	case BOW_TIP_RHS:
		return "bow tip, right-hand side";
	}

	return "";
}

Matrix3x1 &TrackerCalibration::getBetaForSetting(int index)
{
#if (EXTENDED_DEBUG_LOG != 0)
	LOG_INFO_N("violin_recording_plugin", concat::formatStr("get beta for setting (i=%d, ts=%d)", index, NUM_STEPS));
#endif
	return betas_[index];
}

const Matrix3x1 &TrackerCalibration::getBeta(int index) const
{
	// (no logging because function is called many times)
	return betas_[index];
}

// ---------------------------------------------------------------------------------------

void TrackerCalibration::setStepSetFlag(int index)
{
	stepSetFlags_[index] = true;
}

// ---------------------------------------------------------------------------------------
/*
bool TrackerCalibration::verifyCalibration() const
{
	// Assuming the sensors are more or less aligned to the objects they are fixed on, the compute 
	// "betas" should also align with the coordinate system. So some simple tests can be done 
	// to estimate if the calibration was done correctly or not.

	// Order strings 1-4 (right to left):
	bool stringOrderAtBridgeOk = false;
	if (getBeta(STR1_BRIDGE)(0, 0) > getBeta(STR2_BRIDGE)(0, 0) && 
		getBeta(STR2_BRIDGE)(0, 0) > getBeta(STR3_BRIDGE)(0, 0) &&
		getBeta(STR3_BRIDGE)(0, 0) > getBeta(STR4_BRIDGE)(0, 0))
		stringOrderAtBridgeOk = true;

	bool stringOrderAtWoodOk = false;
	if (getBeta(STR1_WOOD)(0, 0) > getBeta(STR2_WOOD)(0, 0) && 
		getBeta(STR2_WOOD)(0, 0) > getBeta(STR3_WOOD)(0, 0) &&
		getBeta(STR3_WOOD)(0, 0) > getBeta(STR4_WOOD)(0, 0))
		stringOrderAtBridgeOk = true;

	bool stringOrderAtFbOk = false;
	if (getBeta(STR1_FB)(0, 0) > getBeta(STR2_FB)(0, 0) && 
		getBeta(STR2_FB)(0, 0) > getBeta(STR3_FB)(0, 0) &&
		getBeta(STR3_FB)(0, 0) > getBeta(STR4_FB)(0, 0))
		stringOrderAtBridgeOk = true;

	return true;
}
*/

// ---------------------------------------------------------------------------------------

void TrackerCalibration::printCalibrationStepMessage()
{
	LOG_INFO_N("violin_recording_plugin", formatStr("[r]Take sample at %s...", (const char *)getDescription(calibrationStep_)));
}

Matrix3x1 TrackerCalibration::computeBetaWithLogMsg(const Matrix3x1 &point, const Matrix3x1 &attUnused, const Matrix3x1 &refSensPos, const Matrix3x1 &refSensOrientation)
{
	// Log information (file only):
	LOG_INFO_N("violin_recording_plugin.filelog", formatStr("Stylus pos. = (%.8f, %.8f, %.8f)", point(0, 0), point(1, 0), point(2, 0)));
	LOG_INFO_N("violin_recording_plugin.filelog", formatStr("Stylus att. (unused) = (%.8f, %.8f, %.8f)", attUnused(0, 0), attUnused(1, 0), attUnused(2, 0)));
	LOG_INFO_N("violin_recording_plugin.filelog", formatStr("Ref. pos. = (%.8f, %.8f, %.8f)", refSensPos(0, 0), refSensPos(1, 0), refSensPos(2, 0)));
	LOG_INFO_N("violin_recording_plugin.filelog", formatStr("Ref. att. = (%.8f, %.8f, %.8f)", refSensOrientation(0, 0), refSensOrientation(1, 0), refSensOrientation(2, 0)));

	// Ensure point and reference sensor position are in the positive-x hemisphere:
	// (not strictly needed, but ensures there are no problems with the hemisphere tracking 
	// during calibration, as long as the sensors start out in the positive-x hemisphere)
	if (point(0, 0) <= 0.0 || refSensPos(0, 0) <= 0.0)
	{
		LOG_INFO_N("violin_recording_plugin", "[r]Warning: Stylus sensor and reference sensor not in positive-x hemisphere!");
		LOG_INFO_N("violin_recording_plugin", "[r]Automatic hemisphere estimation algorithm may be in erroneous state.");
	}

	// Compute beta:
	Matrix3x1 beta = computeBeta(point, refSensPos, refSensOrientation);

	// Log information (file only):
	LOG_INFO_N("violin_recording_plugin.filelog", formatStr("Beta = (%.8f, %.8f, %.8f)", beta(0, 0), beta(1, 0), beta(2, 0)));

	return beta;
}

// ---------------------------------------------------------------------------------------

void TrackerCalibration::resetCalibrationStep()
{
	calibrationStep_ = 0;
#if (EXTENDED_DEBUG_LOG != 0)
	LOG_INFO_N("violin_recording_plugin", concat::formatStr("reset step (step=%d)", calibrationStep_));
#endif
}

int TrackerCalibration::getCalibrationStep() const
{
	return calibrationStep_;
}

void TrackerCalibration::decrCalibrationStep()
{
	--calibrationStep_;
#if (EXTENDED_DEBUG_LOG != 0)
	LOG_INFO_N("violin_recording_plugin", concat::formatStr("decr step (step=%d)", calibrationStep_));
#endif
}

void TrackerCalibration::incrCalibrationStep()
{
	++calibrationStep_;
#if (EXTENDED_DEBUG_LOG != 0)
	LOG_INFO_N("violin_recording_plugin", concat::formatStr("incr step (step=%d)", calibrationStep_));
#endif
}



