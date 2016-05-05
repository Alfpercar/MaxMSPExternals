#ifndef INCLUDED_CONCAT_OVERLAPPINGCOPYHELPER_HXX
#define INCLUDED_CONCAT_OVERLAPPINGCOPYHELPER_HXX

// Small helper to copy a range of data where the 
// range may outside the data's valid range:
//
// 1:
// xxxx-----
//     -------------------
//
// 2:
//                   -----xxxx
//     -------------------
//
// 3:
//          ---------
//     -------------------
//
// 4:
//     xxxxx---------xxxxx
//          ---------
//
// 5:
// xxxxxxxxx
//             -------------------
//
// 6:
//                           xxxxxxxxx
//     -------------------
//
// - = normal copy
// x = out-of-bounds data

#undef max
#undef min
#include <algorithm>

#include <cassert>

namespace concat
{
	class OverlappingCopyHelper
	{
	public:
		int numFramesPreOutOfBounds;
		int numFramesInBounds;
		int numFramesPostOutOfBounds;

		void compute(int beginDataFrames, int endDataFrames, int beginCopyFrames, int endCopyFrames)
		{
			assert(endDataFrames >= beginDataFrames);
			assert(endCopyFrames >= beginCopyFrames);

			int copySizeFrames = endCopyFrames - beginCopyFrames;

			numFramesPreOutOfBounds = std::min(std::max(beginDataFrames - beginCopyFrames, 0), copySizeFrames);
			numFramesInBounds = std::max(std::min(endCopyFrames, endDataFrames) - std::max(beginCopyFrames, beginDataFrames), 0);
			numFramesPostOutOfBounds = copySizeFrames - (numFramesPreOutOfBounds + numFramesInBounds);
		}
	};
}

#endif
