#ifndef INCLUDED_FORCECALIBRATION_HXX
#define INCLUDED_FORCECALIBRATION_HXX

#include <vector>
#include "SimpleMatrix.hxx"

class ForceCalibration
{
public:
	struct MeasurementData
	{
		bool stepSetFlag;
		Matrix3x1 point;
	};

	enum MeasurementSteps
	{
		FLOOR_ORIGIN,
		FLOOR_LEFT,
		FLOOR_FWD,
		STRING_LEFT,
		STRING_RIGHT,
		NUM_STEPS
	};

public:
	ForceCalibration();

	// Resetting:
	void reset();

	// Step index:
	int getStepIdx() const;
	int getNumSteps() const;
	void decrStepIdx();
	void incrStepIdx();

	// Step data:
	const Matrix3x1 &getCurStepPoint() const;
	void setCurStepPoint(const Matrix3x1 &point);

	const Matrix3x1 &getPoint(int stepIdx) const;
	Matrix3x1 &getPointForSetting(int stepIdx);

	void printCurStepMessage();

	// Completeness flags:
	bool areAllStepsSet() const;

	// File I/O:
	bool loadFromFile(const char *filename);
	bool loadFromData(const double *data, int numFloats);
	void saveToFile(const char *filename);

private:
	std::vector<MeasurementData> measurements_;
	int curStep_;
};

#endif