#ifndef INCLUDED_VIOLINMODEL3D_HXX
#define INCLUDED_VIOLINMODEL3D_HXX

#include "windows.h"
#include "juce.h"

#include "SimpleMatrix.hxx"

class ViolinModel3d
{
public:
	enum Paths
	{
		BODY_OUTLINE_UPPER, // n points (any order)
		BODY_DEPTH, // upper edge, lower edge
		BRIDGE, // lower right, upper right, upper center, upper left, lower left (+ string points)
		FINGER_BOARD, // 4 corner points (any order)
		TAIL_PIECE, // 3 corner points (any order)
		STRINGS_ON_TAIL, // 4 points, string 1 to 4 (right to left)
		NUM_PATHS
	};

public:
	ViolinModel3d();

	// Resetting:
	void reset();

	// Step index:
	int getPathStep() const;
	int getNumPathSteps() const;
	void incrPathStep();
	void decrPathStep();

	int getVertexStep() const;
	int getNumVertexSteps() const; // may be fixed or vary
	void incrVertexStep(); // for all except BODY_OUTLINE_UPPER
//	void decrVertexStep(); // for all except BODY_OUTLINE_UPPER
	void pushBackVertexStep(); // only for BODY_OUTLINE_UPPER
	void popBackVertexStep(); // only for BODY_OUTLINE_UPPER

	// Step data:
	const Matrix3x1 &getCurVertex() const;
	Matrix3x1 &getCurVertex();

	int getNumVertexSteps(int pathStep) const;
	Matrix3x1 &getVertex(int pathStep, int vertexStep);

	void printCurStepMessage();

	// Completeness flags:
	bool areAllStepsSet() const;
	void setCurStepSetFlag();

	// File I/O:
	bool loadFromFile(String filename);
	void saveToFile(String filename);

private:
	int pathStep_;
	int vertexStep_;

	struct StepData
	{
		bool isSetFlag;
		Matrix3x1 vertex;
	};

	Array<int> pathSizes_;
	Array<StepData> stepData_; // linear array for all 

	int getIndexLinear(int pathStep, int vertexStep) const;
};

#endif