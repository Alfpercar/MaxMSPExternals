#ifndef INCLUDED_TRACKERCALIBRATION_HXX
#define INCLUDED_TRACKERCALIBRATION_HXX

#include "SimpleMatrix.hxx"

class TrackerCalibration
{
public:
	TrackerCalibration();

	bool loadFromFile(const char *filename);
	bool loadFromData(const double *data, int numFloats);
	bool saveToFile(const char *filename, bool failIfExists = true);

	const char *getAsCsvString() const;

	enum StepIndex
	{
		STR1_BRIDGE = 0,
		STR2_BRIDGE,
		STR3_BRIDGE,
		STR4_BRIDGE,

		STR1_WOOD,
		STR2_WOOD,
		STR3_WOOD,
		STR4_WOOD,

		STR1_FB,
		STR2_FB,
		STR3_FB,
		STR4_FB,

		BOW_FROG_LHS,
		BOW_FROG_RHS,

		BOW_TIP_LHS,
		BOW_TIP_RHS,

		NUM_STEPS
	};

	const char *getDescription(int index) const;

	Matrix3x1 &getBetaForSetting(int index);
	const Matrix3x1 &getBeta(int index) const;

	void resetSetFlags();
	bool areAllStepFlagsSet() const;
	void setStepSetFlag(int index);

	void resetCalibrationStep();
	int getCalibrationStep() const;
	void decrCalibrationStep();
	void incrCalibrationStep();

	void printCalibrationStepMessage();

	Matrix3x1 computeBetaWithLogMsg(const Matrix3x1 &point, const Matrix3x1 &attUnused, const Matrix3x1 &refSensPos, const Matrix3x1 &refSensOrientation);

private:
	Matrix3x1 betas_[NUM_STEPS];
	bool stepSetFlags_[NUM_STEPS];

	int calibrationStep_;
};

#endif