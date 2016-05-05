#ifndef INCLUDED_COMPUTEDESCRIPTORS_HXX
#define INCLUDED_COMPUTEDESCRIPTORS_HXX

#include "LibertyTracker.hxx"
#include "TrackerCalibration.hxx"
#include "ViolinRecordingPlugInConfig.hxx"
#include "CalibrationAngles.hxx"
#include "ForceCalibration.hxx"

#include "SimpleMatrix.hxx"
#include <cmath>

#undef min
#undef max
#include <algorithm>

#include "FilterFir.hxx"
#include "WindowGen.hxx"
#include "Derivative2.hxx"

//#define KSTICK 10
//#define TENSIONHAIRS 1500
#define SENSOR_BOW_DISPLACEMENT 15.5


// ---------------------------------------------------------------------------------------

// Forward declarations (overview):
// --------------------------------

// basic 3d operations:
Matrix3x1 computeBeta(const Matrix3x1 &point, const Matrix3x1 &refSensPos, const Matrix3x1 &refSensOrientation);
Matrix3x1 rotateAndTranslate(const Matrix3x1 &refPos, const Matrix3x3 &refRotMatrix, const Matrix3x1 &transformeeBeta);

// bow model force computation:
double integralOneHairForce(double l, double x, double y);
double oneHairForce(double l /* bowLength */, 
					double x /*Bow Displacement*/, 
					double y /* pseudoforce distance */);


// data structures:
struct RawSensorData;
struct Derived3dData;
struct ViolinPerformanceDescriptors;

// compute descriptors class:
class ComputeViolinPeformanceDescriptors;

// helpers:
struct Line3;
class ComputeSensorVelocityAndAcceleration;
class ComputePlayedStringFromAngleWithHysteresis;

// ---------------------------------------------------------------------------------------

inline Matrix3x1 computeBeta(const Matrix3x1 &point, const Matrix3x1 &refSensPos, const Matrix3x1 &refSensOrientation)
{
	Matrix3x1 result;
	Matrix3x3 rot = Matrix3x3::rotation_matrix_zyx(refSensOrientation(0, 0), refSensOrientation(1, 0), refSensOrientation(2, 0));
	Matrix3x3 invRot = inverse(rot);
	result = invRot*(point - refSensPos);
	return result;
}

inline Matrix3x1 rotateAndTranslate(const Matrix3x1 &refPos, const Matrix3x3 &refRotMatrix, const Matrix3x1 &transformeeBeta)
{
	return refRotMatrix*transformeeBeta + refPos;
}

inline double integralOneHairForce(double l /* bowLength */, 
								   double x /*Bow Displacement*/, 
								   double y /* pseudoforce distance */)
{
	return ((6*pow(y,2) - (2*pow(pow(l - x,2) + pow(y,2),1.5))/
		(l - 2*x) + (2*pow(pow(x,2) + pow(y,2),1.5))/(l - 2*x) + 
		(2*pow((pow(l - x,2) + pow(y,2))*(pow(x,2) + pow(y,2)),
		1.5))/((l - 2*x)*pow(pow(l - x,2) + pow(y,2),1.5)) + 
		(2*pow((pow(l - x,2) + pow(y,2))*(pow(x,2) + pow(y,2)),
		1.5))/((-l + 2*x)*pow(pow(x,2) + pow(y,2),1.5))))/6.;
}

inline double oneHairForce(double l /* bowLength */, 
						   double x /*Bow Displacement*/, 
						   double y /* pseudoforce distance */)
{
	return (y*(2*pow(x,2) + 2*pow(y,2) - l*sqrt(pow(x,2) + pow(y,2)) + 
		2*sqrt((pow(l - x,2) + pow(y,2))*(pow(x,2) + pow(y,2))) - 
		(l*sqrt((pow(l - x,2) + pow(y,2))*(pow(x,2) + pow(y,2))))/
		sqrt(pow(l - x,2) + pow(y,2))))/
		(pow(x,2) + pow(y,2) + 
		sqrt((pow(l - x,2) + pow(y,2))*(pow(x,2) + pow(y,2))));
}






// ---------------------------------------------------------------------------------------

struct Line3
{
	Line3() {}
	Line3(const Matrix3x1 &p1, const Matrix3x1 &p2) : p1(p1), p2(p2) {}

	Matrix3x1 p1;
	Matrix3x1 p2;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// 'key' 3d points derived from the calibration and the raw sensor data, 
// these are displayed in the 3d visualization and used to compute the actual violin 
// performance descriptors
struct Derived3dData
{
	Matrix3x3 violinBodyRotMat;
	Matrix3x3 bowRotMat;

	Matrix3x1 posBowFrogLhs;
	Matrix3x1 posBowTipLhs;
	Matrix3x1 posBowFrogRhs;
	Matrix3x1 posBowTipRhs;

	Matrix3x1 posStr1Bridge;
	Matrix3x1 posStr1Wood;
	Matrix3x1 posStr1Fb;
	Matrix3x1 posStr2Bridge;
	Matrix3x1 posStr2Wood;
	Matrix3x1 posStr2Fb;
	Matrix3x1 posStr3Bridge;
	Matrix3x1 posStr3Wood;
	Matrix3x1 posStr3Fb;
	Matrix3x1 posStr4Bridge;
	Matrix3x1 posStr4Wood;
	Matrix3x1 posStr4Fb; 

	Matrix3x1 posStrRefBridge;
	Matrix3x1 posStrRefFb;

	Line3 interRefLhs;
	Line3 interRefRhs;
	bool isInterRefLhsInsideStringAndBow;
	bool isInterRefRhsInsideStringAndBow;

	Line3 inter1Lhs;
	Line3 inter1Rhs;

	Line3 inter2Lhs;
	Line3 inter2Rhs;
	Line3 inter3Lhs;
	Line3 inter3Rhs;
	Line3 inter4Lhs;
	Line3 inter4Rhs;

	float bowInclinationDegrees;
	float bowInclinationSmoothDegrees; // is delayed by 2 frames (due to 5 point linear phase filter)

	int playedString;	// 1-base, 0 means no string played
						// is delayed by 2 frames (+ hysteresis) because derived from bowInclinationSmoothDegrees

	float bowInclinationZDegrees;
	float bowInclinationZSmoothDegrees; // is delayed by 2 frames (due to 5 point linear phase filter)

	float bowTiltAngleDegrees;
	float bowTiltAngleZDegrees;

//	float bowUpAngleZDegrees;

	float bowBridgeAngleDegrees;

	Matrix3x1 posBridgeMiddle;
	Matrix3x1 vecBridgeUp;
	Matrix3x1 vecBridgeLeft;
	Matrix3x1 vecBridgeFwd;

	Matrix3x1 vecFrogUp;
	Matrix3x1 vecFrogLeft;
	Matrix3x1 vecFrogFwd;

	Matrix3x1 bowStickFrog; // rough approximation (for visualization and stream synchronization purposes)
	Matrix3x1 bowStickTip;
	//Line3 lineBetweenStickAndBridge; // XXX: TEMP

	//------------------------------------------
	//-----------violin2- Esto es una cerdada, cambiar por arrays.. estaba dando problemas de memoria
	/*Matrix3x3 violinBodyRotMat_v2;
	Matrix3x3 bowRotMat_v2;

	Matrix3x1 posBowFrogLhs_v2;
	Matrix3x1 posBowTipLhs_v2;
	Matrix3x1 posBowFrogRhs_v2;
	Matrix3x1 posBowTipRhs_v2;

	Matrix3x1 posStr1Bridge_v2;
	Matrix3x1 posStr1Wood_v2;
	Matrix3x1 posStr1Fb_v2;
	Matrix3x1 posStr2Bridge_v2;
	Matrix3x1 posStr2Wood_v2;
	Matrix3x1 posStr2Fb_v2;
	Matrix3x1 posStr3Bridge_v2;
	Matrix3x1 posStr3Wood_v2;
	Matrix3x1 posStr3Fb_v2;
	Matrix3x1 posStr4Bridge_v2;
	Matrix3x1 posStr4Wood_v2;
	Matrix3x1 posStr4Fb_v2; 

	Matrix3x1 posStrRefBridge_v2;
	Matrix3x1 posStrRefFb_v2;

	Line3 interRefLhs_v2;
	Line3 interRefRhs_v2;
	bool isInterRefLhsInsideStringAndBow_v2;
	bool isInterRefRhsInsideStringAndBow_v2;

	Line3 inter1Lhs_v2;
	Line3 inter1Rhs_v2;

	float bowInclinationDegrees_v2;
	float bowInclinationSmoothDegrees_v2; // is delayed by 2 frames (due to 5 point linear phase filter)

	int playedString_v2;	// 1-base, 0 means no string played
	// is delayed by 2 frames (+ hysteresis) because derived from bowInclinationSmoothDegrees

	float bowInclinationZDegrees_v2;
	float bowInclinationZSmoothDegrees_v2; // is delayed by 2 frames (due to 5 point linear phase filter)

	float bowTiltAngleDegrees_v2;
	float bowTiltAngleZDegrees_v2;

	//	float bowUpAngleZDegrees;

	float bowBridgeAngleDegrees_v2;

	Matrix3x1 posBridgeMiddle_v2;
	Matrix3x1 vecBridgeUp_v2;
	Matrix3x1 vecBridgeLeft_v2;
	Matrix3x1 vecBridgeFwd_v2;

	Matrix3x1 vecFrogUp_v2;
	Matrix3x1 vecFrogLeft_v2;
	Matrix3x1 vecFrogFwd_v2;

	Matrix3x1 bowStickFrog_v2; // rough approximation (for visualization and stream synchronization purposes)
	Matrix3x1 bowStickTip_v2;*/
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Helper class that estimates played string from bow angle applying some hysteresis
class ComputePlayedStringFromAngleWithHysteresis
{
public:
	ComputePlayedStringFromAngleWithHysteresis()
	{
		playedStringNmin1_ = 0;
		isCello_ = false;
	}

	void setCelloEnabled(bool isCello)
	{
		isCello_ = isCello;
	}


	int compute(Derived3dData &ref_to_result, double deg_hyst, Matrix3x1 &v_up)
	{
		int playedString=0;
		const double pi = 3.1415926535897932384626433832795;

		Matrix3x1 v_43 = ref_to_result.posStr4Bridge - ref_to_result.posStr3Bridge;
		Matrix3x1 v_32 = ref_to_result.posStr3Bridge - ref_to_result.posStr2Bridge;
		Matrix3x1 v_21 = ref_to_result.posStr2Bridge - ref_to_result.posStr1Bridge;

		double bowAngleDegrees = (double)ref_to_result.bowInclinationSmoothDegrees;
		
		//double deg_hyst = anglesCalibration.getHysteresisDegrees();
		double ang_43 = (double)(acos( dot(v_up, v_43)/(euclidean_length(v_up)*euclidean_length(v_43)) )/pi*180.0 - 90.0);
		double ang_32 = (double)(acos( dot(v_up, v_32)/(euclidean_length(v_up)*euclidean_length(v_32)) )/pi*180.0 - 90.0);
		double ang_21 = (double)(acos( dot(v_up, v_21)/(euclidean_length(v_up)*euclidean_length(v_21)) )/pi*180.0 - 90.0);

		//ROUGH CHANGE BY PANOS TO ACCOUNT FOR THE DIFFERENT BOW ORIENTATION IN CELLO
		//This needs to be properly changed by adding an option to choose 'type of instrument'
		//We determine if the instrument is a cell by calculating the length of the first string
		bool cello = isCello_;//(abs(euclidean_length((ref_to_result.posStr1Bridge-ref_to_result.posStr1Fb)))>=50);
		
		// First frame:
		if (playedStringNmin1_ == 0)
		{
			if (bowAngleDegrees > ang_43)
			{
				playedString = 4;
			}
			else if (bowAngleDegrees <= ang_43 && bowAngleDegrees > ang_32)
			{
				playedString = 3;
			}
			else if (bowAngleDegrees <= ang_32 && bowAngleDegrees > ang_21)
			{
				playedString = 2;
			}
			else
			{
				playedString = 1;
			}
		}
		// Other frames:
		else
		{ 
			// Copied from Esteban's Matlab script -->
			if ( bowAngleDegrees >= (ang_43 + deg_hyst))												// [angle_43 + deg_hyst,inf)
				playedString = 4;

			// hysteresis
			if ( (bowAngleDegrees >= (ang_43 - deg_hyst)) && (bowAngleDegrees < (ang_43 + deg_hyst)) )	// [angle_43 - deg_hyst, angle_43 + deg_hyst)
				playedString = playedStringNmin1_;

			if ( (bowAngleDegrees >= (ang_32 + deg_hyst)) && (bowAngleDegrees < (ang_43 - deg_hyst)) )	// [angle_32 + deg_hyst, angle_43 - deg_hyst)
				playedString = 3;

			// hysteresis
			if ( (bowAngleDegrees >= (ang_32 - deg_hyst)) && (bowAngleDegrees < (ang_32 + deg_hyst)) )	// [angle_32 - deg_hyst, angle_32 + deg_hyst)
				playedString = playedStringNmin1_;

			if ( (bowAngleDegrees >= (ang_21 + deg_hyst)) && (bowAngleDegrees < (ang_32 - deg_hyst)) )	// [angle_21 + deg_hyst, angle_32 - deg_hyst)
				playedString = 2;

			// hysteresis
			if ( (bowAngleDegrees >= (ang_21 - deg_hyst)) && (bowAngleDegrees < (ang_21 + deg_hyst)) )	// [angle_21 - deg_hyst, angle_21 + deg_hyst)
				playedString = playedStringNmin1_;

			if ( bowAngleDegrees < (ang_21 + deg_hyst))												// (-inf, angle_21 - deg_hyst];
				playedString = 1;
			// <--------------------------------------
		}

		if (cello) playedString = (4-playedString)+1;
		playedStringNmin1_ = playedString;

		return playedString;
	}


private:
	int playedStringNmin1_; // 1-base, 0 means no string/first frame/etc.
	bool isCello_;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Helper class that computes velocity and acceleration of a 3d sensor's position.
class ComputeSensorVelocityAndAcceleration
{
public:
	ComputeSensorVelocityAndAcceleration();

	void reset();

	void compute(const Matrix3x1 &sensPos, double sampleRate);

	double getSensVelocity() const;
	double getSensAcceleration() const;

private:
	bool reset_;

	Matrix3x1 prevSensPos_;
	double prevSensVelocity_;

	double sensVelocity_;
	double sensAcceleration_;
};

// ---------------------------------------------------------------------------------------

// 'raw' sensor data expressed in Matrix3x1 types (with attitudes as euler angles in 
// radians); other descriptors are compute from these basic descriptors
struct RawSensorData
{
	bool extSyncFlag;

	// Sensor 1:
	Matrix3x1 violinBodySensPos[MAX_NUM_VIOLINS];
	Matrix3x1 violinBodySensOrientation[MAX_NUM_VIOLINS]; // euler angles in radians

	// Sensor 1, violin2:
	//Matrix3x1 violin2BodySensPos;
	//Matrix3x1 violin2BodySensOrientation; // euler angles in radians

	// Sensor 2:
	Matrix3x1 bowSensPos[MAX_NUM_VIOLINS];
	Matrix3x1 bowSensOrientation[MAX_NUM_VIOLINS]; // euler angles in radians

	// Sensor 2, bow 2:
	//Matrix3x1 bow2SensPos;
	//Matrix3x1 bow2SensOrientation; // euler angles in radians

	// Sensor 3:
	Matrix3x1 stylusSensPos;
	Matrix3x1 stylusSensOrientation; // euler angles in radians
	bool stylusButtonPressed;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// violin performance descriptors, to be displayed in plots
struct ViolinPerformanceDescriptors
{
	double bowForceLhs;
	double bowForceRhs;
	double bowForce; // max(forceLhs, forceRhs)

	double bowDisplacement; // lhs (should be approx. same as rhs)
	double bowBridgeDistance; // rhs (closest to bow for conventional playing)

	double stickBridgeDistance; // approximation (for stream synchronization)

	double bowVel; // delayed by 1 sample for derivative + 2 samples for smoothing = 3 in total
	double bowAccel; // delayed by 2*1 sample for derivatives + 2*2+4 samples for smoothing = 10 in total
};

struct ExtendedViolinPerformanceDescriptors {
	ViolinPerformanceDescriptors violinPerformanceDescriptors;
	Derived3dData derived3ddata;
};
// ---------------------------------------------------------------------------------------

class ComputeViolinPeformanceDescriptors
{
public:
	ComputeViolinPeformanceDescriptors();

	RawSensorData trackerDataToRawSensorData(LibertyTracker::ItemDataIterator iter, int numViolins, bool isUsingStylus);

	void computeSensorAccelerations(const RawSensorData &rawSensorData, double sampleRate, int numViolins);
	void computeStylusAcceleration(const RawSensorData &rawSensorData, double sampleRate);


	void setCorrectionAngle(double angleValue);
	void setKost(double kost);
	void setKstick(double kstick);
	void setB(double b);

	double HairStickForce(double l /* bowLength */, 
					  double x /*Bow Displacement*/, 
			    	  double ylhs /* observable pseudoforce distance left hand side*/,
					  double yrhs /* observable pseudoforce distance right hand side*/);
	double ComputeViolinPeformanceDescriptors::bowForceHairRibbon(double x /*Bow Displacement*/, 
								 double ylhs /* pseudoforce distance */,
								 double yrhs /* pseudoforce distance */,
								 double l /* bowLength */ );


	Derived3dData computeDerived3dData(const RawSensorData &rawSensorData, const TrackerCalibration &calibration, bool useAutoString, const CalibrationAngles &anglesCalibration, bool isCalibratingForce, const ForceCalibration *forceCalibration, int numViolins, int currViolin);
	Derived3dData computeDerived3dData(const RawSensorData &rawSensorData, const TrackerCalibration &calibration, bool useAutoString, const CalibrationAngles &anglesCalibration, bool isCalibratingForce, const ForceCalibration *forceCalibration, int currViolin);


	ViolinPerformanceDescriptors computeViolinPerformanceDescriptors(const RawSensorData &rawSensorData, const Derived3dData &derived3dData, bool zeroDescriptorsWhenNotPlaying = true, int numViolins=1, int iViolin=0, bool computeForce=false);




private:
	ComputeSensorVelocityAndAcceleration sens1VelAndAccel_;
	ComputeSensorVelocityAndAcceleration sens2VelAndAccel_;
	ComputeSensorVelocityAndAcceleration sens3VelAndAccel_;
	ComputeSensorVelocityAndAcceleration sens4VelAndAccel_;
	ComputeSensorVelocityAndAcceleration sens5VelAndAccel_;

	Derivative2 compBowVel_;
	FilterFir bowVelSmoother_;

	Derivative2 compBowAccel_;
	FilterFir bowAccelSmoother1_;
	FilterFir bowAccelSmoother2_;
	FilterFir bowAccelSmoother3_;

	FilterFir inclinationSmoother_;
	ComputePlayedStringFromAngleWithHysteresis stringEstHysteresis_;

	FilterFir inclinationZSmoother_;

	double correctionAngle_, kost_, kstick_, b_;

	Line3 smallestLineBetweenTwoLines(const Line3 &l1, const Line3 &l2);
	bool isPointWithinLineSegment(const Matrix3x1 &p0, const Matrix3x1 &p1, const Matrix3x1 &p2);

	double computeBowDisplacement(const Matrix3x1 &bowFrog, const Matrix3x1 &pointOnSmallestLineBetweenPlayedStringAndBowHairRibbonAtBowHairRibbon);

	void initSmoothingFilter(FilterFir &filter, int size);
};

// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------

inline ComputeViolinPeformanceDescriptors::ComputeViolinPeformanceDescriptors()
{
	initSmoothingFilter(bowVelSmoother_, 5);
	initSmoothingFilter(bowAccelSmoother1_, 5);
	initSmoothingFilter(bowAccelSmoother2_, 5);
	initSmoothingFilter(bowAccelSmoother3_, 9); // XXX: was 10 in Esteban code, but needs to be odd for symmetric filter
	initSmoothingFilter(inclinationSmoother_, 5);
	initSmoothingFilter(inclinationZSmoother_, 5);

	correctionAngle_ = 0.0;
	kost_ = 1.0;
	kstick_ = 100.0;
	b_ = 0.0;

	//initSmoothingFilter(bowVelSmoother_v2_, 5);
	//initSmoothingFilter(bowAccelSmoother1_v2_, 5);
	//initSmoothingFilter(bowAccelSmoother2_v2_, 5);
	//initSmoothingFilter(bowAccelSmoother3_v2_, 9); // XXX: was 10 in Esteban code, but needs to be odd for symmetric filter
	//initSmoothingFilter(inclinationSmoother_v2_, 5);
	//initSmoothingFilter(inclinationZSmoother_v2_, 5);
}

inline void ComputeViolinPeformanceDescriptors::setCorrectionAngle(double angleValue)
{
	correctionAngle_ = angleValue;
}

inline void ComputeViolinPeformanceDescriptors::setKost(double kost)
{
	kost_ = kost;
}

inline void ComputeViolinPeformanceDescriptors::setKstick(double kstick)
{
	kstick_ = kstick;
}
inline void ComputeViolinPeformanceDescriptors::setB(double b)
{
	b_ = b;
}

inline void ComputeViolinPeformanceDescriptors::initSmoothingFilter(FilterFir &filter, int size)
{
	if (size <= 0)
		return;

	float *coeffs = new float[size];

	// compute gaussian window for n points:
	computeGaussWindow(coeffs, size);

	// scale so coefficients sum to unity:
	float sum = 0.0f;
	for (int i = 0; i < size; ++i)
		sum += coeffs[i];
	for (int i = 0; i < size; ++i)
		coeffs[i] /= sum;

	// init fir filter:
	filter.init(coeffs, size); // copies coeffs to internal memory

	delete[] coeffs;
}


// ---------------------------------------------------------------------------------------

inline RawSensorData ComputeViolinPeformanceDescriptors::trackerDataToRawSensorData(LibertyTracker::ItemDataIterator iter, int numViolins, bool isUsingStylus)
{
	RawSensorData result;

	const double pi = 3.1415926535897932384626433832795;
	const double degreesToRadians = pi/180.0;

	result.extSyncFlag = (iter.item().externalSyncFlag != 0);
	
	for (int iviolin=0;iviolin<numViolins;iviolin++)
	{
		// sensor 1 (violin body):
		result.violinBodySensPos[iviolin] = Matrix3x1(iter.item().position[0], iter.item().position[1], iter.item().position[2]);
		result.violinBodySensOrientation[iviolin] = Matrix3x1(iter.item().orientation[0], iter.item().orientation[1], iter.item().orientation[2]);
		result.violinBodySensOrientation[iviolin] *= degreesToRadians;
		iter.next();

		// sensor 2 (bow):
		result.bowSensPos[iviolin] = Matrix3x1(iter.item().position[0], iter.item().position[1], iter.item().position[2]);
		result.bowSensOrientation[iviolin] = Matrix3x1(iter.item().orientation[0], iter.item().orientation[1], iter.item().orientation[2]);
		result.bowSensOrientation[iviolin] *= degreesToRadians;
		iter.next();
	}

	if (isUsingStylus)
	{
		// sensor 3 (stylus):
		result.stylusSensPos = Matrix3x1(iter.item().position[0], iter.item().position[1], iter.item().position[2]);
		result.stylusSensOrientation = Matrix3x1(iter.item().orientation[0], iter.item().orientation[1], iter.item().orientation[2]);
		result.stylusSensOrientation *= degreesToRadians;
		result.stylusButtonPressed = (iter.item().isStylusButtonPressed == 1);
	}
	return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// needs to be called for each frame (memory)
inline void ComputeViolinPeformanceDescriptors::computeSensorAccelerations(const RawSensorData &rawSensorData, double sampleRate, int curViolin)
{

		sens1VelAndAccel_.compute(rawSensorData.violinBodySensPos[curViolin], sampleRate);
		sens2VelAndAccel_.compute(rawSensorData.bowSensPos[curViolin], sampleRate);
		sens3VelAndAccel_.compute(rawSensorData.stylusSensPos, sampleRate); // (only used when calibrating tracker points, but always computed to keep memory state valid)

}

inline void ComputeViolinPeformanceDescriptors::computeStylusAcceleration(const RawSensorData &rawSensorData, double sampleRate)
{
		sens3VelAndAccel_.compute(rawSensorData.stylusSensPos, sampleRate); // (only used when calibrating tracker points, but always computed to keep memory state valid)

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline Derived3dData ComputeViolinPeformanceDescriptors::computeDerived3dData(const RawSensorData &rawSensorData, const TrackerCalibration &calibration, bool useAutoString, const CalibrationAngles &anglesCalibration, bool isCalibratingForce, const ForceCalibration *forceCalibration, int numViolins, int currViolin)
{
	const double pi = 3.1415926535897932384626433832795;

	Derived3dData result;
	//int currViolin=0;
	// Aliases of betas:
	const Matrix3x1 &betaStr1Bridge = calibration.getBeta(currViolin,TrackerCalibration::STR1_BRIDGE);
	const Matrix3x1 &betaStr2Bridge = calibration.getBeta(currViolin, TrackerCalibration::STR2_BRIDGE);
	const Matrix3x1 &betaStr3Bridge = calibration.getBeta(currViolin, TrackerCalibration::STR3_BRIDGE);
	const Matrix3x1 &betaStr4Bridge = calibration.getBeta(currViolin, TrackerCalibration::STR4_BRIDGE);
	const Matrix3x1 &betaStr1Wood = calibration.getBeta(currViolin, TrackerCalibration::STR1_WOOD);
	const Matrix3x1 &betaStr2Wood = calibration.getBeta(currViolin, TrackerCalibration::STR2_WOOD);
	const Matrix3x1 &betaStr3Wood = calibration.getBeta(currViolin, TrackerCalibration::STR3_WOOD);
	const Matrix3x1 &betaStr4Wood = calibration.getBeta(currViolin, TrackerCalibration::STR4_WOOD);
	const Matrix3x1 &betaStr1Fb = calibration.getBeta(currViolin, TrackerCalibration::STR1_FB);
	const Matrix3x1 &betaStr2Fb = calibration.getBeta(currViolin, TrackerCalibration::STR2_FB);
	const Matrix3x1 &betaStr3Fb = calibration.getBeta(currViolin, TrackerCalibration::STR3_FB);
	const Matrix3x1 &betaStr4Fb = calibration.getBeta(currViolin, TrackerCalibration::STR4_FB);
	const Matrix3x1 &betaFrogLhs = calibration.getBeta(currViolin, TrackerCalibration::BOW_FROG_LHS);
	const Matrix3x1 &betaFrogRhs = calibration.getBeta(currViolin, TrackerCalibration::BOW_FROG_RHS);
	const Matrix3x1 &betaTipLhs = calibration.getBeta(currViolin, TrackerCalibration::BOW_TIP_LHS);
	const Matrix3x1 &betaTipRhs = calibration.getBeta(currViolin, TrackerCalibration::BOW_TIP_RHS);

	// Aliases of rotation matrices:
	Matrix3x3 &violinBodyRotMat = result.violinBodyRotMat;
	Matrix3x3 &bowRotMat = result.bowRotMat;

	// Compute rotation matrices:
	violinBodyRotMat = Matrix3x3::rotation_matrix_zyx(rawSensorData.violinBodySensOrientation[currViolin](0, 0), rawSensorData.violinBodySensOrientation[currViolin](1, 0), rawSensorData.violinBodySensOrientation[currViolin](2, 0));
	bowRotMat = Matrix3x3::rotation_matrix_zyx(rawSensorData.bowSensOrientation[currViolin](0, 0), rawSensorData.bowSensOrientation[currViolin](1, 0), rawSensorData.bowSensOrientation[currViolin](2, 0));

	// Compute rotated and translated strings:
	result.posStr1Bridge = violinBodyRotMat*betaStr1Bridge + rawSensorData.violinBodySensPos[currViolin];
	result.posStr2Bridge = violinBodyRotMat*betaStr2Bridge + rawSensorData.violinBodySensPos[currViolin];
	result.posStr3Bridge = violinBodyRotMat*betaStr3Bridge + rawSensorData.violinBodySensPos[currViolin];
	result.posStr4Bridge = violinBodyRotMat*betaStr4Bridge + rawSensorData.violinBodySensPos[currViolin];
	result.posStr1Wood = violinBodyRotMat*betaStr1Wood + rawSensorData.violinBodySensPos[currViolin];
	result.posStr2Wood = violinBodyRotMat*betaStr2Wood + rawSensorData.violinBodySensPos[currViolin];
	result.posStr3Wood = violinBodyRotMat*betaStr3Wood + rawSensorData.violinBodySensPos[currViolin];
	result.posStr4Wood = violinBodyRotMat*betaStr4Wood + rawSensorData.violinBodySensPos[currViolin];
	result.posStr1Fb = violinBodyRotMat*betaStr1Fb + rawSensorData.violinBodySensPos[currViolin];
	result.posStr2Fb = violinBodyRotMat*betaStr2Fb + rawSensorData.violinBodySensPos[currViolin];
	result.posStr3Fb = violinBodyRotMat*betaStr3Fb + rawSensorData.violinBodySensPos[currViolin];
	result.posStr4Fb = violinBodyRotMat*betaStr4Fb + rawSensorData.violinBodySensPos[currViolin];

	// Compute rotated and translated bow:
	result.posBowFrogLhs = bowRotMat*betaFrogLhs + rawSensorData.bowSensPos[currViolin];
	result.posBowFrogRhs = bowRotMat*betaFrogRhs + rawSensorData.bowSensPos[currViolin];
	result.posBowTipLhs = bowRotMat*betaTipLhs + rawSensorData.bowSensPos[currViolin];
	result.posBowTipRhs = bowRotMat*betaTipRhs + rawSensorData.bowSensPos[currViolin];

	// Overwrite string 1 when calibrating force:
	if (isCalibratingForce && forceCalibration != NULL)
	{
		// Str1 is used to compute some descriptors (together with StrRef).
		result.posStr1Bridge = forceCalibration->getPoint(ForceCalibration::STRING_RIGHT);
		result.posStr1Fb = forceCalibration->getPoint(ForceCalibration::STRING_LEFT);
		result.posStr1Wood = (result.posStr1Bridge + result.posStr1Fb)/2.0; // not used
	}

	// Aliases of rotated and translated betas:
	const Matrix3x1 &br1 = result.posStr1Bridge;
	const Matrix3x1 &br2 = result.posStr2Bridge;
	const Matrix3x1 &br3 = result.posStr3Bridge;
	const Matrix3x1 &br4 = result.posStr4Bridge;

	const Matrix3x1 &fb2 = result.posStr2Fb;
	const Matrix3x1 &fb3 = result.posStr3Fb;

	const Matrix3x1 &frog_lhs = result.posBowFrogLhs;
	const Matrix3x1 &tip_lhs = result.posBowTipLhs;
	const Matrix3x1 &frog_rhs = result.posBowFrogRhs;
	const Matrix3x1 &tip_rhs = result.posBowTipRhs;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// Compute bridge vectors:
	Matrix3x1 m_br;							// center of bridge
	Matrix3x1 m_fb;							// center of fingerboard
	Matrix3x1 v_br_left;					// bridge vector pointing left
	Matrix3x1 v_br_fwd;						// bridge vector pointing forward
	Matrix3x1 v_br_up;						// bridge vector pointing up

	m_br = (br2 + br3)/2;					// middle point on bridge, also origin of vectors
	v_br_left = normalize(br4 - br1);		// vector pointing left from (0, 0, 0) (normalized)
	m_fb = (fb2 + fb3)/2;					// middle point on finger board
	v_br_fwd = normalize(m_fb - m_br);		// vector pointing forward from (0, 0, 0) (normalized)
	v_br_up = cross(v_br_fwd, v_br_left);	// vector pointing up from (0, 0, 0) (normalized), according to right-hand rule for the cross-product
											// NOTE: order of cross arguments determines up/down direction, left x fwd = up
											// XXX: for some reason we need fwd x left to get up

	// Store points for visualization:
	result.posBridgeMiddle = m_br;
	result.vecBridgeUp = v_br_up;
	result.vecBridgeLeft = v_br_left;
	result.vecBridgeFwd = v_br_fwd;

	// Compute hair ribbon vector:
	Matrix3x1 v_hr;

	v_hr = tip_lhs - frog_lhs;				// hair ribbon vector pointing towards tip from (0, 0, 0) (not normalized)
											// lhs or rhs should give (nearly) same result
	
	// Compute bow inclination angle:
	// v1.v2 = ||v1|| ||v2|| cos(angle)

	if (isCalibratingForce && forceCalibration != NULL)
	{
		// Calibrating force, use floor up vector:
		Matrix3x1 v_fl_left;
		Matrix3x1 v_fl_fwd;
		Matrix3x1 v_fl_up;

		v_fl_left = normalize(forceCalibration->getPoint(ForceCalibration::FLOOR_LEFT) - forceCalibration->getPoint(ForceCalibration::FLOOR_ORIGIN));
		v_fl_fwd = normalize(forceCalibration->getPoint(ForceCalibration::FLOOR_FWD) - forceCalibration->getPoint(ForceCalibration::FLOOR_ORIGIN));
		v_fl_up = cross(v_fl_fwd, v_fl_left);

		result.bowInclinationDegrees = (float)(acos( dot(v_fl_up, v_hr)/(euclidean_length(v_fl_up)*euclidean_length(v_hr)) )/pi*180.0 - 90.0); // so 90 degree angle between v_up and v_hr corresponds to 0 degree bow-angle
		inclinationSmoother_.process(&result.bowInclinationDegrees, &result.bowInclinationSmoothDegrees, 1);
	}
	else
	{
		// Normal operation, use bridge up vector:
		result.bowInclinationDegrees = (float)(acos( dot(v_br_up, v_hr)/(euclidean_length(v_br_up)*euclidean_length(v_hr)) )/pi*180.0 - 90.0); // so 90 degree angle between v_up and v_hr corresponds to 0 degree bow-angle
		inclinationSmoother_.process(&result.bowInclinationDegrees, &result.bowInclinationSmoothDegrees, 1);
	}

	// Compute bow inclination relative to z-axis of source (~gravity): NOTE THAT source-Z POINTS DOWNWARDS (~gravity)
	Matrix3x1 v_z_up(0.0, 0.0, 1.0); // normalized
	result.bowInclinationZDegrees = (float)(acos( dot(v_z_up, v_hr)/(euclidean_length(v_z_up)*euclidean_length(v_hr)) )/pi*180.0 - 90.0); // so 90 degree angle between v_z_up and v_hr corresponds to 0 degree bow-angle
	inclinationZSmoother_.process(&result.bowInclinationZDegrees, &result.bowInclinationZSmoothDegrees, 1);

	// Estimate played string (apply hysteresis, etc.):

	if (currViolin == 3) stringEstHysteresis_.setCelloEnabled(true);
	else stringEstHysteresis_.setCelloEnabled(false);
	int autoStringEst = stringEstHysteresis_.compute(result, 1, v_br_up);

	if (useAutoString)
	{
		result.playedString = autoStringEst;
	}
	else
	{
		result.playedString = 1;
	}

	// Overwrite played string when calibrating force:
	if (isCalibratingForce && forceCalibration != NULL)
	{
		// StrRef is used to compute some descriptors (together with Str1).
		result.playedString = 1;
	}

	// Reference string used to compute descriptors:
	//assert(result.playedString >= 1 && result.playedString <= 4);
	if(result.playedString < 1 || result.playedString > 4)
		result.playedString = 1;
	if (result.playedString == 1)
	{
		result.posStrRefBridge = result.posStr1Bridge;
		result.posStrRefFb = result.posStr1Fb;
	}
	else if (result.playedString == 2)
	{
		result.posStrRefBridge = result.posStr2Bridge;
		result.posStrRefFb = result.posStr2Fb;
	}
	else if (result.playedString == 3)
	{
		result.posStrRefBridge = result.posStr3Bridge;
		result.posStrRefFb = result.posStr3Fb;
	}
	else if (result.playedString == 4)
	{
		result.posStrRefBridge = result.posStr4Bridge;
		result.posStrRefFb = result.posStr4Fb;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// Compute bow vectors:
	Matrix3x1 v_fr_left;
	Matrix3x1 v_fr_fwd;
	Matrix3x1 v_fr_up;
	const Matrix3x1 &frog_lhs_with_stick_top = frog_rhs;
	const Matrix3x1 &frog_rhs_with_stick_top = frog_lhs;
	v_fr_left = normalize(frog_lhs_with_stick_top - frog_rhs_with_stick_top);		// vector pointing left (with hair ribbon bottom, stick top)
	v_fr_fwd = normalize(tip_lhs - frog_lhs);										// vector pointing forward
	v_fr_up = cross(v_fr_fwd, v_fr_left);											// vector pointing up

	// Store points for visualization:
	result.vecFrogLeft = v_fr_left;
	result.vecFrogFwd = v_fr_fwd;
	result.vecFrogUp = v_fr_up;

	// Compute bow tilt angle:
	Matrix3x1 v_str = result.posStrRefBridge - result.posStrRefFb;
	result.bowTiltAngleDegrees = (float)(acos( dot(v_str, v_fr_up)/(euclidean_length(v_str)*euclidean_length(v_fr_up)) )/pi*180.0 - 90.0);

	// Compute bow tilt angle Z:
	Matrix3x1 v_froglh = frog_rhs - frog_lhs; // frog left to right vector
	result.bowTiltAngleZDegrees = (float)(acos( dot(v_z_up, v_froglh)/(euclidean_length(v_z_up)*euclidean_length(v_froglh)) )/pi*180.0 - 90.0);

	//// Compute bowup angle Z
	//Matrix3x1 v_bowup = cross(v_froglh, v_hr);
	//result.bowUpAngleZDegrees = (float)(acos( dot(v_z_up, v_bowup)/(euclidean_length(v_z_up)*euclidean_length(v_bowup)) )/pi*180.0 - 90.0);

	// Compute bow-bridge angle (angle between hair-ribbon and bridge, thus around 0 degrees 
	// when playing normally):
	result.bowBridgeAngleDegrees = (float)(acos( dot(v_br_left, v_hr)/(euclidean_length(v_br_left)*euclidean_length(v_hr)) )/pi*180.0);

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// Compute smallest line between any point on bow line and string 1 line:
	
	result.inter1Lhs = smallestLineBetweenTwoLines(Line3(result.posStr1Bridge, result.posStr1Fb), Line3(result.posBowFrogLhs, result.posBowTipLhs));
	result.inter1Rhs = smallestLineBetweenTwoLines(Line3(result.posStr1Bridge, result.posStr1Fb), Line3(result.posBowFrogRhs, result.posBowTipRhs));

	result.inter2Lhs = smallestLineBetweenTwoLines(Line3(result.posStr2Bridge, result.posStr2Fb), Line3(result.posBowFrogLhs, result.posBowTipLhs));
	result.inter2Rhs = smallestLineBetweenTwoLines(Line3(result.posStr2Bridge, result.posStr2Fb), Line3(result.posBowFrogRhs, result.posBowTipRhs));

	result.inter3Lhs = smallestLineBetweenTwoLines(Line3(result.posStr3Bridge, result.posStr3Fb), Line3(result.posBowFrogLhs, result.posBowTipLhs));
	result.inter3Rhs = smallestLineBetweenTwoLines(Line3(result.posStr3Bridge, result.posStr3Fb), Line3(result.posBowFrogRhs, result.posBowTipRhs));

	result.inter4Lhs = smallestLineBetweenTwoLines(Line3(result.posStr4Bridge, result.posStr4Fb), Line3(result.posBowFrogLhs, result.posBowTipLhs));
	result.inter4Rhs = smallestLineBetweenTwoLines(Line3(result.posStr4Bridge, result.posStr4Fb), Line3(result.posBowFrogRhs, result.posBowTipRhs));

	// Compute smallest line between any point on bow line and reference string line:
	result.interRefLhs = smallestLineBetweenTwoLines(Line3(result.posStrRefBridge, result.posStrRefFb), Line3(result.posBowFrogLhs, result.posBowTipLhs));
	result.interRefRhs = smallestLineBetweenTwoLines(Line3(result.posStrRefBridge, result.posStrRefFb), Line3(result.posBowFrogRhs, result.posBowTipRhs));
	// NOTE:
	// inter.p1 is intersection point on ref. string vector
	// inter.p2 is intersection point on bow vector

	// Check if smallest line between string/bow actually lie inside string/bow lines (with 
	// tolerance to avoid descriptors going to 0 at extrema when there is some inaccuracy with 
	// the calibration):
	Matrix3x1 strRefVectorNorm = normalize(result.posStrRefFb - result.posStrRefBridge); // bridge pointing towards fb
	Matrix3x1 bowVectorNormLhs = normalize(result.posBowTipLhs - result.posBowFrogLhs); // frog pointing towards tip (left)
	Matrix3x1 bowVectorNormRhs = normalize(result.posBowTipRhs - result.posBowFrogRhs); // frog pointing towards tip (right)
	double stringVectorToleranceCm = 3.0;
	double bowVectorToleranceCm = 2.0;
	result.isInterRefLhsInsideStringAndBow = (isPointWithinLineSegment(result.interRefLhs.p1, result.posStrRefBridge - stringVectorToleranceCm*strRefVectorNorm, result.posStrRefFb + stringVectorToleranceCm*strRefVectorNorm) &&
		isPointWithinLineSegment(result.interRefLhs.p2, result.posBowFrogLhs - bowVectorToleranceCm*bowVectorNormLhs, result.posBowTipLhs + bowVectorToleranceCm*bowVectorNormLhs));
	result.isInterRefRhsInsideStringAndBow = (isPointWithinLineSegment(result.interRefRhs.p1, result.posStrRefBridge - stringVectorToleranceCm*strRefVectorNorm, result.posStrRefFb + stringVectorToleranceCm*strRefVectorNorm) &&
		isPointWithinLineSegment(result.interRefRhs.p2, result.posBowFrogRhs - bowVectorToleranceCm*bowVectorNormRhs, result.posBowTipRhs + bowVectorToleranceCm*bowVectorNormRhs));

	if (!result.isInterRefLhsInsideStringAndBow && !result.isInterRefRhsInsideStringAndBow)
	{
		result.playedString = 0; // no string being played
	}

	// Compute approximation of bow stick:
	{
		Matrix3x1 v_left;
		Matrix3x1 v_fwd;
		Matrix3x1 v_up;

		const Matrix3x1 &frog_lhs_with_stick_top = frog_rhs;
		const Matrix3x1 &frog_rhs_with_stick_top = frog_lhs;
		const Matrix3x1 &tip_lhs_with_stick_top = tip_rhs;
		const Matrix3x1 &tip_rhs_with_stick_top = tip_lhs;
		
		// vector pointing left (with hair ribbon bottom, stick top)
		// note: left - right means right pointing to left
		v_left = normalize(frog_lhs_with_stick_top - frog_rhs_with_stick_top);
		// vector pointing forward
		// note: using lhs or rhs shouldn't really matter
		v_fwd = normalize(tip_rhs_with_stick_top - frog_rhs_with_stick_top);
		// vector pointing downward
		// note: cross product of two normalized vector is a normalized vector
		v_up = cross(v_fwd, v_left);

		Matrix3x1 frog_center = (frog_lhs_with_stick_top + frog_rhs_with_stick_top)*0.5;
		Matrix3x1 tip_center = (tip_lhs_with_stick_top + tip_rhs_with_stick_top)*0.5;

		double stickHairDistanceCm = 1.0; // at most narrow part, conservative approximation (we don't want absolute distance wrapping when hitting bridge)

		result.bowStickFrog = frog_center + stickHairDistanceCm*v_up;
		result.bowStickTip = tip_center + stickHairDistanceCm*v_up;

	}

	return result;
}

inline Derived3dData ComputeViolinPeformanceDescriptors::computeDerived3dData(const RawSensorData &rawSensorData, const TrackerCalibration &calibration, bool useAutoString, const CalibrationAngles &anglesCalibration, bool isCalibratingForce, const ForceCalibration *forceCalibration, int currViolin)
{
	const double pi = 3.1415926535897932384626433832795;

	Derived3dData result;
	//int currViolin=0;
	// Aliases of betas:
	const Matrix3x1 &betaStr1Bridge = calibration.getBeta(0,TrackerCalibration::STR1_BRIDGE);
	const Matrix3x1 &betaStr2Bridge = calibration.getBeta(0, TrackerCalibration::STR2_BRIDGE);
	const Matrix3x1 &betaStr3Bridge = calibration.getBeta(0, TrackerCalibration::STR3_BRIDGE);
	const Matrix3x1 &betaStr4Bridge = calibration.getBeta(0, TrackerCalibration::STR4_BRIDGE);
	const Matrix3x1 &betaStr1Wood = calibration.getBeta(0, TrackerCalibration::STR1_WOOD);
	const Matrix3x1 &betaStr2Wood = calibration.getBeta(0, TrackerCalibration::STR2_WOOD);
	const Matrix3x1 &betaStr3Wood = calibration.getBeta(0, TrackerCalibration::STR3_WOOD);
	const Matrix3x1 &betaStr4Wood = calibration.getBeta(0, TrackerCalibration::STR4_WOOD);
	const Matrix3x1 &betaStr1Fb = calibration.getBeta(0, TrackerCalibration::STR1_FB);
	const Matrix3x1 &betaStr2Fb = calibration.getBeta(0, TrackerCalibration::STR2_FB);
	const Matrix3x1 &betaStr3Fb = calibration.getBeta(0, TrackerCalibration::STR3_FB);
	const Matrix3x1 &betaStr4Fb = calibration.getBeta(0, TrackerCalibration::STR4_FB);
	const Matrix3x1 &betaFrogLhs = calibration.getBeta(0, TrackerCalibration::BOW_FROG_LHS);
	const Matrix3x1 &betaFrogRhs = calibration.getBeta(0, TrackerCalibration::BOW_FROG_RHS);
	const Matrix3x1 &betaTipLhs = calibration.getBeta(0, TrackerCalibration::BOW_TIP_LHS);
	const Matrix3x1 &betaTipRhs = calibration.getBeta(0, TrackerCalibration::BOW_TIP_RHS);

	// Aliases of rotation matrices:
	Matrix3x3 &violinBodyRotMat = result.violinBodyRotMat;
	Matrix3x3 &bowRotMat = result.bowRotMat;

	// Compute rotation matrices:
	violinBodyRotMat = Matrix3x3::rotation_matrix_zyx(rawSensorData.violinBodySensOrientation[currViolin](0, 0), rawSensorData.violinBodySensOrientation[currViolin](1, 0), rawSensorData.violinBodySensOrientation[currViolin](2, 0));
	bowRotMat = Matrix3x3::rotation_matrix_zyx(rawSensorData.bowSensOrientation[currViolin](0, 0), rawSensorData.bowSensOrientation[currViolin](1, 0), rawSensorData.bowSensOrientation[currViolin](2, 0));

	// Compute rotated and translated strings:
	result.posStr1Bridge = violinBodyRotMat*betaStr1Bridge + rawSensorData.violinBodySensPos[currViolin];
	result.posStr2Bridge = violinBodyRotMat*betaStr2Bridge + rawSensorData.violinBodySensPos[currViolin];
	result.posStr3Bridge = violinBodyRotMat*betaStr3Bridge + rawSensorData.violinBodySensPos[currViolin];
	result.posStr4Bridge = violinBodyRotMat*betaStr4Bridge + rawSensorData.violinBodySensPos[currViolin];
	result.posStr1Wood = violinBodyRotMat*betaStr1Wood + rawSensorData.violinBodySensPos[currViolin];
	result.posStr2Wood = violinBodyRotMat*betaStr2Wood + rawSensorData.violinBodySensPos[currViolin];
	result.posStr3Wood = violinBodyRotMat*betaStr3Wood + rawSensorData.violinBodySensPos[currViolin];
	result.posStr4Wood = violinBodyRotMat*betaStr4Wood + rawSensorData.violinBodySensPos[currViolin];
	result.posStr1Fb = violinBodyRotMat*betaStr1Fb + rawSensorData.violinBodySensPos[currViolin];
	result.posStr2Fb = violinBodyRotMat*betaStr2Fb + rawSensorData.violinBodySensPos[currViolin];
	result.posStr3Fb = violinBodyRotMat*betaStr3Fb + rawSensorData.violinBodySensPos[currViolin];
	result.posStr4Fb = violinBodyRotMat*betaStr4Fb + rawSensorData.violinBodySensPos[currViolin];

	// Compute rotated and translated bow:
	result.posBowFrogLhs = bowRotMat*betaFrogLhs + rawSensorData.bowSensPos[currViolin];
	result.posBowFrogRhs = bowRotMat*betaFrogRhs + rawSensorData.bowSensPos[currViolin];
	result.posBowTipLhs = bowRotMat*betaTipLhs + rawSensorData.bowSensPos[currViolin];
	result.posBowTipRhs = bowRotMat*betaTipRhs + rawSensorData.bowSensPos[currViolin];

	// Overwrite string 1 when calibrating force:
	if (isCalibratingForce && forceCalibration != NULL)
	{
		// Str1 is used to compute some descriptors (together with StrRef).
		result.posStr1Bridge = forceCalibration->getPoint(ForceCalibration::STRING_RIGHT);
		result.posStr1Fb = forceCalibration->getPoint(ForceCalibration::STRING_LEFT);
		result.posStr1Wood = (result.posStr1Bridge + result.posStr1Fb)/2.0; // not used
	}

	// Aliases of rotated and translated betas:
	const Matrix3x1 &br1 = result.posStr1Bridge;
	const Matrix3x1 &br2 = result.posStr2Bridge;
	const Matrix3x1 &br3 = result.posStr3Bridge;
	const Matrix3x1 &br4 = result.posStr4Bridge;

	const Matrix3x1 &fb2 = result.posStr2Fb;
	const Matrix3x1 &fb3 = result.posStr3Fb;

	const Matrix3x1 &frog_lhs = result.posBowFrogLhs;
	const Matrix3x1 &tip_lhs = result.posBowTipLhs;
	const Matrix3x1 &frog_rhs = result.posBowFrogRhs;
	const Matrix3x1 &tip_rhs = result.posBowTipRhs;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// Compute bridge vectors:
	Matrix3x1 m_br;							// center of bridge
	Matrix3x1 m_fb;							// center of fingerboard
	Matrix3x1 v_br_left;					// bridge vector pointing left
	Matrix3x1 v_br_fwd;						// bridge vector pointing forward
	Matrix3x1 v_br_up;						// bridge vector pointing up

	m_br = (br2 + br3)/2;					// middle point on bridge, also origin of vectors
	v_br_left = normalize(br4 - br1);		// vector pointing left from (0, 0, 0) (normalized)
	m_fb = (fb2 + fb3)/2;					// middle point on finger board
	v_br_fwd = normalize(m_fb - m_br);		// vector pointing forward from (0, 0, 0) (normalized)
	v_br_up = cross(v_br_fwd, v_br_left);	// vector pointing up from (0, 0, 0) (normalized), according to right-hand rule for the cross-product
											// NOTE: order of cross arguments determines up/down direction, left x fwd = up
											// XXX: for some reason we need fwd x left to get up

	// Store points for visualization:
	result.posBridgeMiddle = m_br;
	result.vecBridgeUp = v_br_up;
	result.vecBridgeLeft = v_br_left;
	result.vecBridgeFwd = v_br_fwd;

	// Compute hair ribbon vector:
	Matrix3x1 v_hr;

	v_hr = tip_lhs - frog_lhs;				// hair ribbon vector pointing towards tip from (0, 0, 0) (not normalized)
											// lhs or rhs should give (nearly) same result
	
	// Compute bow inclination angle:
	// v1.v2 = ||v1|| ||v2|| cos(angle)

	if (isCalibratingForce && forceCalibration != NULL)
	{
		// Calibrating force, use floor up vector:
		Matrix3x1 v_fl_left;
		Matrix3x1 v_fl_fwd;
		Matrix3x1 v_fl_up;

		v_fl_left = normalize(forceCalibration->getPoint(ForceCalibration::FLOOR_LEFT) - forceCalibration->getPoint(ForceCalibration::FLOOR_ORIGIN));
		v_fl_fwd = normalize(forceCalibration->getPoint(ForceCalibration::FLOOR_FWD) - forceCalibration->getPoint(ForceCalibration::FLOOR_ORIGIN));
		v_fl_up = cross(v_fl_fwd, v_fl_left);

		result.bowInclinationDegrees = (float)(acos( dot(v_fl_up, v_hr)/(euclidean_length(v_fl_up)*euclidean_length(v_hr)) )/pi*180.0 - 90.0); // so 90 degree angle between v_up and v_hr corresponds to 0 degree bow-angle
		inclinationSmoother_.process(&result.bowInclinationDegrees, &result.bowInclinationSmoothDegrees, 1);
	}
	else
	{
		// Normal operation, use bridge up vector:
		result.bowInclinationDegrees = (float)(acos( dot(v_br_up, v_hr)/(euclidean_length(v_br_up)*euclidean_length(v_hr)) )/pi*180.0 - 90.0); // so 90 degree angle between v_up and v_hr corresponds to 0 degree bow-angle
		inclinationSmoother_.process(&result.bowInclinationDegrees, &result.bowInclinationSmoothDegrees, 1);
	}

	// Compute bow inclination relative to z-axis of source (~gravity): NOTE THAT source-Z POINTS DOWNWARDS (~gravity)
	Matrix3x1 v_z_up(0.0, 0.0, 1.0); // normalized
	result.bowInclinationZDegrees = (float)(acos( dot(v_z_up, v_hr)/(euclidean_length(v_z_up)*euclidean_length(v_hr)) )/pi*180.0 - 90.0); // so 90 degree angle between v_z_up and v_hr corresponds to 0 degree bow-angle
	inclinationZSmoother_.process(&result.bowInclinationZDegrees, &result.bowInclinationZSmoothDegrees, 1);

	// Estimate played string (apply hysteresis, etc.):
	if (currViolin == 3) stringEstHysteresis_.setCelloEnabled(true);
	else stringEstHysteresis_.setCelloEnabled(false);
	int autoStringEst = stringEstHysteresis_.compute(result, 1, v_br_up);

	if (useAutoString)
	{
		result.playedString = autoStringEst;
	}
	else
	{
		result.playedString = 1;
	}

	// Overwrite played string when calibrating force:
	if (isCalibratingForce && forceCalibration != NULL)
	{
		// StrRef is used to compute some descriptors (together with Str1).
		result.playedString = 1;
	}

	// Reference string used to compute descriptors:
	//assert(result.playedString >= 1 && result.playedString <= 4);
	if(result.playedString < 1 || result.playedString > 4)
		result.playedString = 1;
	if (result.playedString == 1)
	{
		result.posStrRefBridge = result.posStr1Bridge;
		result.posStrRefFb = result.posStr1Fb;
	}
	else if (result.playedString == 2)
	{
		result.posStrRefBridge = result.posStr2Bridge;
		result.posStrRefFb = result.posStr2Fb;
	}
	else if (result.playedString == 3)
	{
		result.posStrRefBridge = result.posStr3Bridge;
		result.posStrRefFb = result.posStr3Fb;
	}
	else if (result.playedString == 4)
	{
		result.posStrRefBridge = result.posStr4Bridge;
		result.posStrRefFb = result.posStr4Fb;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// Compute bow vectors:
	Matrix3x1 v_fr_left;
	Matrix3x1 v_fr_fwd;
	Matrix3x1 v_fr_up;
	const Matrix3x1 &frog_lhs_with_stick_top = frog_rhs;
	const Matrix3x1 &frog_rhs_with_stick_top = frog_lhs;
	v_fr_left = normalize(frog_lhs_with_stick_top - frog_rhs_with_stick_top);		// vector pointing left (with hair ribbon bottom, stick top)
	v_fr_fwd = normalize(tip_lhs - frog_lhs);										// vector pointing forward
	v_fr_up = cross(v_fr_fwd, v_fr_left);											// vector pointing up

	// Store points for visualization:
	result.vecFrogLeft = v_fr_left;
	result.vecFrogFwd = v_fr_fwd;
	result.vecFrogUp = v_fr_up;

	// Compute bow tilt angle:
	Matrix3x1 v_str = result.posStrRefBridge - result.posStrRefFb;
	result.bowTiltAngleDegrees = (float)(acos( dot(v_str, v_fr_up)/(euclidean_length(v_str)*euclidean_length(v_fr_up)) )/pi*180.0 - 90.0);

	// Compute bow tilt angle Z:
	Matrix3x1 v_froglh = frog_rhs - frog_lhs; // frog left to right vector
	result.bowTiltAngleZDegrees = (float)(acos( dot(v_z_up, v_froglh)/(euclidean_length(v_z_up)*euclidean_length(v_froglh)) )/pi*180.0 - 90.0);

	//// Compute bowup angle Z
	//Matrix3x1 v_bowup = cross(v_froglh, v_hr);
	//result.bowUpAngleZDegrees = (float)(acos( dot(v_z_up, v_bowup)/(euclidean_length(v_z_up)*euclidean_length(v_bowup)) )/pi*180.0 - 90.0);

	// Compute bow-bridge angle (angle between hair-ribbon and bridge, thus around 0 degrees 
	// when playing normally):
	result.bowBridgeAngleDegrees = (float)(acos( dot(v_br_left, v_hr)/(euclidean_length(v_br_left)*euclidean_length(v_hr)) )/pi*180.0);

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// Compute smallest line between any point on bow line and string 1 line:
	
	result.inter1Lhs = smallestLineBetweenTwoLines(Line3(result.posStr1Bridge, result.posStr1Fb), Line3(result.posBowFrogLhs, result.posBowTipLhs));
	result.inter1Rhs = smallestLineBetweenTwoLines(Line3(result.posStr1Bridge, result.posStr1Fb), Line3(result.posBowFrogRhs, result.posBowTipRhs));

	result.inter2Lhs = smallestLineBetweenTwoLines(Line3(result.posStr2Bridge, result.posStr2Fb), Line3(result.posBowFrogLhs, result.posBowTipLhs));
	result.inter2Rhs = smallestLineBetweenTwoLines(Line3(result.posStr2Bridge, result.posStr2Fb), Line3(result.posBowFrogRhs, result.posBowTipRhs));

	result.inter3Lhs = smallestLineBetweenTwoLines(Line3(result.posStr3Bridge, result.posStr3Fb), Line3(result.posBowFrogLhs, result.posBowTipLhs));
	result.inter3Rhs = smallestLineBetweenTwoLines(Line3(result.posStr3Bridge, result.posStr3Fb), Line3(result.posBowFrogRhs, result.posBowTipRhs));

	result.inter4Lhs = smallestLineBetweenTwoLines(Line3(result.posStr4Bridge, result.posStr4Fb), Line3(result.posBowFrogLhs, result.posBowTipLhs));
	result.inter4Rhs = smallestLineBetweenTwoLines(Line3(result.posStr4Bridge, result.posStr4Fb), Line3(result.posBowFrogRhs, result.posBowTipRhs));

	// Compute smallest line between any point on bow line and reference string line:
	result.interRefLhs = smallestLineBetweenTwoLines(Line3(result.posStrRefBridge, result.posStrRefFb), Line3(result.posBowFrogLhs, result.posBowTipLhs));
	result.interRefRhs = smallestLineBetweenTwoLines(Line3(result.posStrRefBridge, result.posStrRefFb), Line3(result.posBowFrogRhs, result.posBowTipRhs));
	// NOTE:
	// inter.p1 is intersection point on ref. string vector
	// inter.p2 is intersection point on bow vector

	// Check if smallest line between string/bow actually lie inside string/bow lines (with 
	// tolerance to avoid descriptors going to 0 at extrema when there is some inaccuracy with 
	// the calibration):
	Matrix3x1 strRefVectorNorm = normalize(result.posStrRefFb - result.posStrRefBridge); // bridge pointing towards fb
	Matrix3x1 bowVectorNormLhs = normalize(result.posBowTipLhs - result.posBowFrogLhs); // frog pointing towards tip (left)
	Matrix3x1 bowVectorNormRhs = normalize(result.posBowTipRhs - result.posBowFrogRhs); // frog pointing towards tip (right)
	double stringVectorToleranceCm = 3.0;
	double bowVectorToleranceCm = 2.0;
	result.isInterRefLhsInsideStringAndBow = (isPointWithinLineSegment(result.interRefLhs.p1, result.posStrRefBridge - stringVectorToleranceCm*strRefVectorNorm, result.posStrRefFb + stringVectorToleranceCm*strRefVectorNorm) &&
		isPointWithinLineSegment(result.interRefLhs.p2, result.posBowFrogLhs - bowVectorToleranceCm*bowVectorNormLhs, result.posBowTipLhs + bowVectorToleranceCm*bowVectorNormLhs));
	result.isInterRefRhsInsideStringAndBow = (isPointWithinLineSegment(result.interRefRhs.p1, result.posStrRefBridge - stringVectorToleranceCm*strRefVectorNorm, result.posStrRefFb + stringVectorToleranceCm*strRefVectorNorm) &&
		isPointWithinLineSegment(result.interRefRhs.p2, result.posBowFrogRhs - bowVectorToleranceCm*bowVectorNormRhs, result.posBowTipRhs + bowVectorToleranceCm*bowVectorNormRhs));

	if (!result.isInterRefLhsInsideStringAndBow && !result.isInterRefRhsInsideStringAndBow)
	{
		result.playedString = 0; // no string being played
	}

	// Compute approximation of bow stick:
	{
		Matrix3x1 v_left;
		Matrix3x1 v_fwd;
		Matrix3x1 v_up;

		const Matrix3x1 &frog_lhs_with_stick_top = frog_rhs;
		const Matrix3x1 &frog_rhs_with_stick_top = frog_lhs;
		const Matrix3x1 &tip_lhs_with_stick_top = tip_rhs;
		const Matrix3x1 &tip_rhs_with_stick_top = tip_lhs;
		
		// vector pointing left (with hair ribbon bottom, stick top)
		// note: left - right means right pointing to left
		v_left = normalize(frog_lhs_with_stick_top - frog_rhs_with_stick_top);
		// vector pointing forward
		// note: using lhs or rhs shouldn't really matter
		v_fwd = normalize(tip_rhs_with_stick_top - frog_rhs_with_stick_top);
		// vector pointing downward
		// note: cross product of two normalized vector is a normalized vector
		v_up = cross(v_fwd, v_left);

		Matrix3x1 frog_center = (frog_lhs_with_stick_top + frog_rhs_with_stick_top)*0.5;
		Matrix3x1 tip_center = (tip_lhs_with_stick_top + tip_rhs_with_stick_top)*0.5;

		double stickHairDistanceCm = 1.0; // at most narrow part, conservative approximation (we don't want absolute distance wrapping when hitting bridge)

		result.bowStickFrog = frog_center + stickHairDistanceCm*v_up;
		result.bowStickTip = tip_center + stickHairDistanceCm*v_up;
	}

	return result;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline ViolinPerformanceDescriptors ComputeViolinPeformanceDescriptors::computeViolinPerformanceDescriptors(const RawSensorData &rawSensorData, const Derived3dData &derived3dData, bool zeroDescriptorsWhenNotPlaying, int numViolins, int iViolin, bool computeForce)
{
	ViolinPerformanceDescriptors result;
	const double pesudoForceOffset=0.0; //cm

	// Aliases of variables in structs:
	const Line3 &inter1Lhs = derived3dData.inter1Lhs;
	const Line3 &inter1Rhs = derived3dData.inter1Rhs;
	const Line3 &interRefLhs = derived3dData.interRefLhs;
	const Line3 &interRefRhs = derived3dData.interRefRhs;
	const Matrix3x1 &posBowFrogLhs = derived3dData.posBowFrogLhs;
	const Matrix3x1 &posStr1Bridge = derived3dData.posStr1Bridge;
	const Matrix3x1 &posStrRefBridge = derived3dData.posStrRefBridge;
	const Matrix3x1 &violinBodySensPos = rawSensorData.violinBodySensPos[iViolin];

	// Compute bow displacement:
	//result.bowDisplacement = computeBowDisplacement(posBowFrogLhs, inter1Lhs.p2); // lhs/rhs shouldn't differ much

	// SEPT 2011 CHANGE, the bowDisplacement should be relative to the currently played string for a correct force estimation. Marco
	result.bowDisplacement = computeBowDisplacement(posBowFrogLhs, interRefLhs.p2); // lhs/rhs shouldn't differ much

	// Compute bow force:
	double bowLength = euclidean_length(derived3dData.posBowTipLhs - derived3dData.posBowFrogLhs);

	result.bowForceLhs = euclidean_length(interRefLhs.p2 - interRefLhs.p1)+pesudoForceOffset; // absolute length
	if (euclidean_length(interRefLhs.p1 - violinBodySensPos) < euclidean_length(interRefLhs.p2 - violinBodySensPos)+pesudoForceOffset) // bow is farther away than string, assume not playing
		result.bowForceLhs = -result.bowForceLhs;

	result.bowForceRhs = euclidean_length(interRefRhs.p2 - interRefRhs.p1)+pesudoForceOffset; // absolute length
	if (euclidean_length(interRefRhs.p1 - violinBodySensPos) < euclidean_length(interRefRhs.p2 - violinBodySensPos)+pesudoForceOffset) // bow is farther away than string, assume not playing
		result.bowForceRhs = -result.bowForceRhs;

	//result.bowForce = bowForceHairRibbon(result.bowDisplacement,result.bowForceLhs,result.bowForceRhs,bowLength);
	if (computeForce)
		result.bowForce = HairStickForce(result.bowDisplacement,b_ + cos(correctionAngle_)*result.bowForceLhs + sin(correctionAngle_)*result.bowDisplacement,b_ + cos(correctionAngle_)*result.bowForceRhs + sin(correctionAngle_)*result.bowDisplacement, bowLength);
		//result.bowForce = HairStickForce(result.bowDisplacement,result.bowForceLhs,result.bowForceRhs,bowLength);

	/****OLD FORCE CALCULATION*********************
	if (result.bowForceRhs > result.bowForceLhs)
	result.bowForce = result.bowForceRhs;
	else
	result.bowForce = result.bowForceLhs;
	****OLD FORCE CALCULATION*********************/

	// Compute bow-bridge distance:
	result.bowBridgeDistance = euclidean_length(inter1Rhs.p1 - posStr1Bridge); // rhs because it is closest to bridge

	// Compute bow stick-bridge (top) distance:
	Line3 lineBridgeTop = Line3(derived3dData.posStr2Bridge, derived3dData.posStr3Bridge);
	Line3 lineBowStick = Line3(derived3dData.bowStickTip, derived3dData.bowStickFrog);
	Line3 lineBetweenStickAndBridge = smallestLineBetweenTwoLines(lineBridgeTop, lineBowStick);
	result.stickBridgeDistance = euclidean_length(lineBetweenStickAndBridge.p1 - lineBetweenStickAndBridge.p2);

//	if (!isPointWithinLineSegment(lineBetweenStickAndBridge.p1, lineBridgeTop.p1, lineBridgeTop.p2) ||
//		!isPointWithinLineSegment(lineBetweenStickAndBridge.p2, lineBowStick.p1, lineBowStick.p2))
//		result.stickBridgeDistance = 1000.0; // very big (outside of display's range)

	// Compute bow velocity:
	float bowVel = compBowVel_.compute((float)result.bowDisplacement, 240.0f);
	float bowVelSmooth;
	bowVelSmoother_.process(&bowVel, &bowVelSmooth, 1);
	result.bowVel = bowVelSmooth;

	// Compute bow acceleration:
	float bowAccel = compBowAccel_.compute(bowVel, 240.0f);
	float bowAccelSmooth;
	bowAccelSmoother1_.process(&bowAccel, &bowAccelSmooth, 1);
	bowAccelSmoother2_.process(&bowAccelSmooth, &bowAccelSmooth, 1);
	bowAccelSmoother3_.process(&bowAccelSmooth, &bowAccelSmooth, 1);
	result.bowAccel = bowAccelSmooth;

	// Check if descriptors are valid:
	if (!derived3dData.isInterRefLhsInsideStringAndBow && !derived3dData.isInterRefRhsInsideStringAndBow && zeroDescriptorsWhenNotPlaying)
	{
		// override descriptors to zero
		result.bowForceLhs = -1000.0;
		result.bowForceRhs = -1000.0;
		result.bowForce = -1000.0;
		result.bowDisplacement = 0.0;
		result.bowBridgeDistance = 0.0;
		result.bowVel = 0.0;
		result.bowAccel = 0.0;
	}

	return result;
}

// ---------------------------------------------------------------------------------------

// note that lines defined by l1 and l2 are extended to an infinite length 
// so result may not be inside line segments defined by l1 and l2
inline Line3 ComputeViolinPeformanceDescriptors::smallestLineBetweenTwoLines(const Line3 &l1, const Line3 &l2)
{
	const Matrix3x1 p13 = l1.p1 - l2.p1;

	const Matrix3x1 p43 = l2.p2 - l2.p1; // length of line 2
	// XXX: check if not very small (<- especially this one, because div zero below)
	
	const Matrix3x1 p21 = l1.p2 - l1.p1; // length of line 1
	// XXX: check if not very small

	// XXX: if p21 or p43 is really small, probably change to case of distance between 
	// line and point or between point and point

	const double d1343 = dot(p13, p43);//sum(times(p13, p43));
	const double d4321 = dot(p43, p21);//sum(times(p43, p21));
	const double d1321 = dot(p13, p21);//sum(times(p13, p21));
	const double d4343 = dot(p43, p43);//sum(times(p43, p43));
	const double d2121 = dot(p21, p21);//sum(times(p21, p21));

	const double denom = d2121*d4343 - d4321*d4321;
	// XXX: check if not very small

	const double numer = d1343*d4321 - d1321*d4343;

	const double mu1 = numer/denom;
	const double mu2 = (d1343 + d4321*mu1)/d4343;
	// NOTE: if p43 isn't very small d4343 can't be very small either

	Line3 result;
	result.p1 = l1.p1 + p21*mu1; // point on line 1
	result.p2 = l2.p1 + p43*mu2; // point on line 2

	return result;
}

// assuming point p0 is point on (infinite length) line defined by p1 and p2, determine 
// whether p0 lies within segment defined by p1 and p2 or lies outside of that segment
inline bool ComputeViolinPeformanceDescriptors::isPointWithinLineSegment(const Matrix3x1 &p0, const Matrix3x1 &p1, const Matrix3x1 &p2)
{
	const double xMin = std::min(p1(0, 0), p2(0, 0));
	const double xMax = std::max(p1(0, 0), p2(0, 0));
	const double yMin = std::min(p1(1, 0), p2(1, 0));
	const double yMax = std::max(p1(1, 0), p2(1, 0));
	const double zMin = std::min(p1(2, 0), p2(2, 0));
	const double zMax = std::max(p1(2, 0), p2(2, 0));

	if (p0(0, 0) >= xMin && p0(0, 0) <= xMax &&
		p0(1, 0) >= yMin && p0(1, 0) <= yMax &&
		p0(2, 0) >= zMin && p0(2, 0) <= zMax)
	{
		return true;
	}
	else
	{
		return false;
	}
}

inline double ComputeViolinPeformanceDescriptors::bowForceHairRibbon(double x /*Bow Displacement*/, 
								 double ylhs /* pseudoforce distance */,
								 double yrhs /* pseudoforce distance */,
								 double l /* bowLength */ )
{
	double M,m;

	if (yrhs > ylhs)
	{
		M = yrhs;
		m = ylhs;
	}
	else
	{
		M = ylhs;
		m = yrhs;
	}

	if (M<0)
	{
		return 0;
	}
	else if ((m<M)&&(m<=0))
	{
		return kstick_*(integralOneHairForce(l,x,M)-integralOneHairForce(l,x,0))/(M-m);
	}
	else if ((m<M)&&(m>0))
	{
		return kstick_*(integralOneHairForce(l,x,M)-integralOneHairForce(l,x,m))/(M-m);
	}
	else if (m==M)
	{
		return kstick_*oneHairForce(l,x,M);
	}
	else
	{
		return 0;
	}
}

inline double ComputeViolinPeformanceDescriptors::HairStickForce(double x /*Bow Displacement*/, 
							 double ylhs /* observable pseudoforce distance left hand side*/,
							 double yrhs /* observable pseudoforce distance right hand side*/,
							 double l /* bowLength */)
{
	int n;
	double a,b,c;
	a=-1.3;
	b=4;
	double sintheta = (ylhs-yrhs)/2.0;
	double yobs = (ylhs+yrhs)/2.0;
	double force,fc,fa,forcea,forcec;



	//f= bowForceHairRibbon(x,c+sintheta,c-sintheta,l)*KSTICK*x*(x+SENSOR_BOW_DISPLACEMENT)+l*l*(c-yobs);
	if ((ylhs<=0.0 && yrhs<=0.0) || (ylhs>4.0 && yrhs>4.0) || fabs(ylhs-yrhs)>1.0)
		force = 0.0;
	else
	{
		for(n=0;n<50;n++){

			c=(a+b)/2.0;

			forcec = bowForceHairRibbon(x,c+sintheta,c-sintheta,l);

			fc = forcec*kstick_*x*(x+SENSOR_BOW_DISPLACEMENT)+l*l*(c-yobs);

			if(fabs(fc)<1.0e-6)	break;		//break if solution becomes close to the convergence point(root)

			forcea = bowForceHairRibbon(x,a+sintheta,a-sintheta,l);

			fa= forcea*kstick_*x*(x+SENSOR_BOW_DISPLACEMENT)+l*l*(a-yobs);

			//force = bowForceHairRibbon(x,c+sintheta,c-sintheta,l); // just for debugging

			if(fa*fc<0.0)
			{
				b=c;
				if (forcec-forcea<1.0e-6) break;
			}
			else 
			{
				a=c;
				if ((bowForceHairRibbon(x,b+sintheta,b-sintheta,l)-forcec)<1.0e-6) break;
			}
		}
		force = bowForceHairRibbon(x,c+sintheta,c-sintheta,l);
	}
	return force;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//inline void ComputeViolinPeformanceDescriptors::computeBowForce()
//{
//}

inline double ComputeViolinPeformanceDescriptors::computeBowDisplacement(const Matrix3x1 &bowFrog, const Matrix3x1 &pointOnSmallestLineBetweenPlayedStringAndBowHairRibbonAtBowHairRibbon)
{
	// compute distance (in cm) between point of line on bow to bow frog:
	Matrix3x1 tmp = pointOnSmallestLineBetweenPlayedStringAndBowHairRibbonAtBowHairRibbon - bowFrog;
	return euclidean_length(tmp);
}

// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------

inline ComputeSensorVelocityAndAcceleration::ComputeSensorVelocityAndAcceleration()
{
	reset();
}

// when calling reset(), the velocity and acceleration compute on the next compute() call 
// will be zero and regain their actual values over the next couple of compute() calls 
inline void ComputeSensorVelocityAndAcceleration::reset()
{
	reset_ = true;
}

inline void ComputeSensorVelocityAndAcceleration::compute(const Matrix3x1 &sensPos, double sampleRate)
{
	if (reset_)
	{
		// Compute sensor velocity and acceleration:
		sensVelocity_ = 0.0f;
		sensAcceleration_ = 0.0f;

		// Store values for next call:
		prevSensPos_ = sensPos;
		prevSensVelocity_ = sensVelocity_;

		reset_ = false;
	}
	else
	{
		// Compute sensor velocity and acceleration:
		sensVelocity_ = euclidean_length(sensPos - prevSensPos_)*sampleRate;
		sensAcceleration_ = fabs(sensVelocity_ - prevSensVelocity_)*sampleRate;

		// Store values for next call:
		prevSensPos_ = sensPos;
		prevSensVelocity_ = sensVelocity_;
	}
}

inline double ComputeSensorVelocityAndAcceleration::getSensVelocity() const
{
	return sensVelocity_;
}

inline double ComputeSensorVelocityAndAcceleration::getSensAcceleration() const
{
	return sensAcceleration_;
}

#endif

