#ifndef INCLUDED_CALIBRATIONANGLES_HXX
#define INCLUDED_CALIBRATIONANGLES_HXX

#include <string>
#include <ostream>
#include <sstream>
#include <fstream>

class CalibrationAngles
{
public:
	CalibrationAngles();

	bool loadFromFile(const char *filename);
	void loadFromData(double hysteresisDegrees, double ang43, double ang32, double ang21);

	bool isValid() const { return isValid_; }

	double getHysteresisDegrees() const { return hysteresisDegrees_; }
	double getAng43() const { return ang43_; }
	double getAng32() const { return ang32_; }
	double getAng21() const { return ang21_; }

private:
	bool isValid_;

	double hysteresisDegrees_;
	double ang43_;
	double ang32_;
	double ang21_;
};

// ---------------------------------------------------------------------------------------

inline CalibrationAngles::CalibrationAngles()
{
	isValid_ = false;

	hysteresisDegrees_ = 0.0;
	ang43_ = 0.0;
	ang32_ = 0.0;
	ang21_ = 0.0;
}

namespace impl // XXX: HACK
{
	// utility for reading csv files using fstreams
	inline std::istream &comma(std::istream &in)
	{
		if ((in >> std::ws).get() != in.widen(','))
			in.setstate(std::ios_base::failbit);
		else
			in.ignore();

		return in;
	}
} // XXX: HACK

inline bool CalibrationAngles::loadFromFile(const char *filename)
{
	using namespace impl; // XXX: HACK

	// Open file:
	std::fstream calibFile;
	calibFile.open(filename, std::ifstream::in);

	if (!calibFile)
		return false;

	std::string line;

	if (std::getline(calibFile, line).fail())
	{
		calibFile.close();
		return false;
	}

	std::stringstream lineStream(line);
	lineStream >> hysteresisDegrees_ >> comma >> ang43_ >> comma >> ang32_ >> comma >> ang21_;
	if (lineStream.fail())
	{
		calibFile.close();
		return false;
	}

	// Close file:
	calibFile.close();

	return true;
}

inline void CalibrationAngles::loadFromData(double hysteresisDegrees, double ang43, double ang32, double ang21)
{
	isValid_ = true;

	hysteresisDegrees_ = hysteresisDegrees;
	ang43_ = ang43;
	ang32_ = ang32;
	ang21_ = ang21;
}

#endif

