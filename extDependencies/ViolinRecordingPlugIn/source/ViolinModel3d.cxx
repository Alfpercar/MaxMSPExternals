#include "ViolinModel3d.hxx"

#include "concat/Utilities/Logging.hxx"
#include "concat/Utilities/StdInt.hxx"

#include <cassert>
#include <fstream>

#define EXTENDED_DEBUG_LOG 0

ViolinModel3d::ViolinModel3d()
{
	reset();
};

// ---------------------------------------------------------------------------------------

void ViolinModel3d::reset()
{
	// Reset step indexes:
	pathStep_ = 0;
	vertexStep_ = 0;

	// Clear path sizes data:
	pathSizes_.clear();
	for (int i = 0; i < NUM_PATHS; ++i)
	{
		pathSizes_.add(0);
	}
	pathSizes_.set(BODY_OUTLINE_UPPER, 0); // still unknown
	pathSizes_.set(BODY_DEPTH, 2);
	pathSizes_.set(BRIDGE, 5);
	pathSizes_.set(FINGER_BOARD, 4);
	pathSizes_.set(TAIL_PIECE, 3);
	pathSizes_.set(STRINGS_ON_TAIL, 4);

	// Clear step data:
	stepData_.clear();
	int totalDataSize = 0;
	for (int i = 0; i < NUM_PATHS; ++i)
	{
		totalDataSize += pathSizes_[i];
	}
	for (int i = 0; i < totalDataSize; ++i)
	{
		StepData nullStepData;
		nullStepData.isSetFlag = false;
		stepData_.add(nullStepData);
	}

#if (EXTENDED_DEBUG_LOG != 0)
	LOG_INFO_N("violin_recording_plugin", concat::formatStr("reset ts=%d (0+2+5+4=11)", stepData_.size()));
#endif
}

// ---------------------------------------------------------------------------------------

int ViolinModel3d::getPathStep() const
{
	return pathStep_;
}

int ViolinModel3d::getNumPathSteps() const
{
	return NUM_PATHS;
}

void ViolinModel3d::incrPathStep()
{
	assert(pathStep_ <= NUM_PATHS - 1); // also allow to increase to NUM_PATHS
	++pathStep_;
	vertexStep_ = 0;
#if (EXTENDED_DEBUG_LOG != 0)
	LOG_INFO_N("violin_recording_plugin", concat::formatStr("incr path step (p=%d, v=%d)", pathStep_, vertexStep_));
#endif
}

void ViolinModel3d::decrPathStep()
{
	assert(pathStep_ > 0);
	--pathStep_;
	vertexStep_ = 0;
#if (EXTENDED_DEBUG_LOG != 0)
	LOG_INFO_N("violin_recording_plugin", concat::formatStr("decr path step (p=%d, v=%d)", pathStep_, vertexStep_));
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int ViolinModel3d::getVertexStep() const
{
	return vertexStep_;
}

int ViolinModel3d::getNumVertexSteps() const
{
	return pathSizes_[pathStep_];
}

void ViolinModel3d::incrVertexStep()
{
	assert(pathStep_ != BODY_OUTLINE_UPPER);
	assert(vertexStep_ < getNumVertexSteps() - 1);
	++vertexStep_;
#if (EXTENDED_DEBUG_LOG != 0)
	LOG_INFO_N("violin_recording_plugin", concat::formatStr("incr vertex step (p=%d, v=%d)", pathStep_, vertexStep_));
#endif
}

/*void ViolinModel3d::decrVertexStep()
{
	assert(pathStep_ != BODY_OUTLINE_UPPER);
	assert(vertexStep_ > 0);
	--vertexStep_;
}*/

void ViolinModel3d::pushBackVertexStep()
{
	assert(pathStep_ == BODY_OUTLINE_UPPER);

	pathSizes_.getReference(pathStep_) += 1;
	StepData nullStepData;
	nullStepData.isSetFlag = false;
	stepData_.insert(getIndexLinear(pathStep_, vertexStep_) + 1, nullStepData); // before next
	++vertexStep_; // go to last (for BODY_OUTLINE_UPPER, vertexStep_ is always last)

#if (EXTENDED_DEBUG_LOG != 0)
	LOG_INFO_N("violin_recording_plugin", concat::formatStr("push back (s=%d, p=%d, v=%d, ts=%d)", getNumVertexSteps(), pathStep_, vertexStep_, stepData_.size()));
#endif
}

void ViolinModel3d::popBackVertexStep()
{
	assert(pathStep_ == BODY_OUTLINE_UPPER);

	pathSizes_.getReference(pathStep_) -= 1;
	stepData_.remove(getIndexLinear(pathStep_, vertexStep_));
	--vertexStep_; // go to last (for BODY_OUTLINE_UPPER, vertexStep_ is always last)

#if (EXTENDED_DEBUG_LOG != 0)
	LOG_INFO_N("violin_recording_plugin", concat::formatStr("pop back (s=%d, p=%d, v=%d, ts=%d)", getNumVertexSteps(), pathStep_, vertexStep_, stepData_.size()));
#endif
}

// ---------------------------------------------------------------------------------------

const Matrix3x1 &ViolinModel3d::getCurVertex() const
{
#if (EXTENDED_DEBUG_LOG != 0)
	LOG_INFO_N("violin_recording_plugin", concat::formatStr("get cur vertex (p=%d, v=%d, l=%d)", pathStep_, vertexStep_, getIndexLinear(pathStep_, vertexStep_)));
#endif
	return stepData_.getReference(getIndexLinear(pathStep_, vertexStep_)).vertex;
}

Matrix3x1 &ViolinModel3d::getCurVertex()
{
#if (EXTENDED_DEBUG_LOG != 0)
	LOG_INFO_N("violin_recording_plugin", concat::formatStr("get cur vertex (p=%d, v=%d, l=%d)", pathStep_, vertexStep_, getIndexLinear(pathStep_, vertexStep_)));
#endif
	return stepData_.getReference(getIndexLinear(pathStep_, vertexStep_)).vertex;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int ViolinModel3d::getNumVertexSteps(int pathStep) const
{
	return pathSizes_[pathStep];
}
	
Matrix3x1 &ViolinModel3d::getVertex(int pathStep, int vertexStep)
{
	return stepData_.getReference(getIndexLinear(pathStep, vertexStep)).vertex;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ViolinModel3d::printCurStepMessage()
{
	String pathDescr;
	String vertexDescr;
	if (pathStep_ == BODY_OUTLINE_UPPER)
	{
		pathDescr = String(T("violin body outline (upper)"));
		vertexDescr = String(T("any (ordered)"));
	}
	else if (pathStep_ == BODY_DEPTH)
	{
		pathDescr = String(T("violin body depth"));
		if (vertexStep_ == 0)
		{
			vertexDescr = String(T("upper"));			
		}
		else if (vertexStep_ == 1)
		{
			vertexDescr = String(T("lower"));
		}
	}
	else if (pathStep_ == BRIDGE)
	{
		pathDescr = String(T("additional bridge points"));
		if (vertexStep_ == 0)
		{
			vertexDescr = String(T("lower-right"));
		}
		else if (vertexStep_ == 1)
		{
			vertexDescr = String(T("upper-right"));
		}
		else if (vertexStep_ == 2)
		{
			vertexDescr = String(T("upper-center"));
		}
		else if (vertexStep_ == 3)
		{
			vertexDescr = String(T("upper-left"));
		}
		else if (vertexStep_ == 4)
		{
			vertexDescr = String(T("lower-left"));
		}
	}
	else if (pathStep_ == FINGER_BOARD)
	{
		pathDescr = String(T("finger board"));
		vertexDescr = String(T("any (ordered)"));
	}
	else if (pathStep_ == TAIL_PIECE)
	{
		pathDescr = String(T("tail piece triangle"));
		vertexDescr = String(T("any (ordered)"));
	}
	else if (pathStep_ == STRINGS_ON_TAIL)
	{
		pathDescr = String(T("strings on tail piece"));
		vertexDescr = String::formatted(T("string %d"), vertexStep_ + 1);
	}

	LOG_INFO_N("violin_recording_plugin", concat::formatStr("[r]Sample %s path, at %s vertex...", (const char *)pathDescr, (const char *)vertexDescr));
}

// ---------------------------------------------------------------------------------------

bool ViolinModel3d::areAllStepsSet() const
{
	for (int i = 0; i < stepData_.size(); ++i)
	{
		if (!stepData_[i].isSetFlag)
		{
			// XXX: temp!
////			LOG_INFO_N("violin_recording_plugin", concat::formatStr("[r]Step %d not set!", i));
			// XXX
			return false;
		}
	}

	return true;
}

void ViolinModel3d::setCurStepSetFlag()
{
	stepData_.getReference(getIndexLinear(pathStep_, vertexStep_)).isSetFlag = true;
}

// ---------------------------------------------------------------------------------------

bool ViolinModel3d::loadFromFile(String filename)
{
	using namespace concat;

	std::ifstream file;
	file.open((const char *)filename, std::ios_base::binary);

	if (!file.is_open())
		return false;

	// Four character ID:
	char id[4];
	file.read(&id[0], sizeof(char));
	file.read(&id[1], sizeof(char));
	file.read(&id[2], sizeof(char));
	file.read(&id[3], sizeof(char));

	if (id[0] != 'V' ||
		id[1] != 'M' ||
		id[2] != 'd' ||
		id[3] != 'l')
		return false;

	// Num paths:
	int32_t numPaths;
	file.read((char *)&numPaths, sizeof(int32_t));

	if (numPaths != NUM_PATHS)
		return false;

	// Clear:
	pathSizes_.clear();
	stepData_.clear();
	pathStep_ = 0;
	vertexStep_ = 0;

	// Paths:
	for (int i = 0; i < numPaths; ++i)
	{
		int32_t numVertexes;
		file.read((char *)&numVertexes, sizeof(int32_t));

		pathSizes_.add(numVertexes);

		for (int j = 0; j < numVertexes; ++j)
		{
			double m[3];
			file.read((char *)&m[0], sizeof(double));
			file.read((char *)&m[1], sizeof(double));
			file.read((char *)&m[2], sizeof(double));

			StepData stepData;
			stepData.isSetFlag = true;
			stepData.vertex = Matrix3x1(m[0], m[1], m[2]);
			stepData_.add(stepData);
		}
	}

	file.close();

	return true;
}

void ViolinModel3d::saveToFile(String filename)
{
	using namespace concat;

	std::ofstream file;
	file.open((const char *)filename, std::ios_base::trunc | std::ios_base::binary);

	if (file.is_open())
	{
		// Four character ID:
		char id[4];
		id[0] = 'V'; // V(iolin)
		id[1] = 'M'; // M(o)
		id[2] = 'd'; // d(e)
		id[3] = 'l'; // l
		file.write(&id[0], sizeof(char));
		file.write(&id[1], sizeof(char));
		file.write(&id[2], sizeof(char));
		file.write(&id[3], sizeof(char));

		// Num paths:
		int32_t numPaths = NUM_PATHS;
		file.write((char *)&numPaths, sizeof(int32_t));

		// Paths:
		for (int i = 0; i < NUM_PATHS; ++i)
		{
			int32_t numVertexes = getNumVertexSteps(i);
			file.write((char *)&numVertexes, sizeof(int32_t));

			for (int j = 0; j < numVertexes; ++j)
			{
				file.write((char *)getVertex(i, j).getPtr(), 3*sizeof(double));
			}
		}
	}

	file.close();
}

// ---------------------------------------------------------------------------------------

int ViolinModel3d::getIndexLinear(int pathStep, int vertexStep) const
{
	int k = 0;
	for (int i = 0; i < pathStep; ++i)
	{
		k += pathSizes_[i];
	}
	k += vertexStep;
	return k;
}


