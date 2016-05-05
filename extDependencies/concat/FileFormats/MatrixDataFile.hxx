#ifndef INCLUDED_CONCAT_MATRIXDATAFILE_HXX
#define INCLUDED_CONCAT_MATRIXDATAFILE_HXX

#include <cassert>
#include <fstream>
#include <vector>

//#include "concat/Utilities/StdInt.hxx"
#include "../Utilities/StdInt.hxx" // relative for tools

// XXX-TODO:
// 1) rename numValuesPerFrame to frameSize
// 2) add option for reader to pre-read header in memory and re-open file using pre-read header
// 3) isOk() for MatrixDataFileWrite
// 4) rename to MatrixDataFileReader/Writer
// 5) less asserts for things that may happen at run time due to corrupt file, etc.

// ---------------------------------------------------------------------------------------

// Matrix data file is a simple binary file format for storing unstructured 1d or 2d 
// numerical data (float).
//
// Data format (all fields stored little endian):
// -----------------------------:-------------------------------:-----------------------------------------------------
// Field:						:	Size:						:	Example:
// -----------------------------:-------------------------------:-----------------------------------------------------
// Identifier					:	4*char						:	'M', 'T', 'R', 'X' (required)
// Sample rate (in Hz)			:	float64						:	44100.0 (samples per second)
// Hop size (in samples)		:	uint32						:	256, or 0 for non-constant hop size (see below)
// Number of values per frame	:	uint32						:	1, or 0 for non-constant num. values per frames
// Number of frames				:	uint32						:	5
// Data size (in floats)		:	uint32						:	5 (const size) or 7 (non-const size)
// -----------------------------:-------------------------------:-----------------------------------------------------
// Data	(const size), -OR-		:	M x N x float32, -OR-		:	0.23f, 0.19f, 0.15f, 0.09f, 0.04f
// Data (non-const size)		:	M x N_i x float32			:	{0.1f, 0.2f}, {0.1f}, {0.1f}, {0.1f, 0.2f}, {0.1f}
// -----------------------------:-------------------------------:-----------------------------------------------------
// Num. of values per frame		:	M x uint32					:	2, 1, 1, 2, 1
// array (opt., non-const size) :                               :
// -----------------------------:-------------------------------:-----------------------------------------------------
//
// Non-constant frame rate (hop size):
// For time-varying hop sizes, used for instance with pitch-synchronous algorithms, 
// the hop size field in the header must be set to 0. The frame time tags (i.e. onset 
// of frame or center of frame window) should be stored in a separate MatrixDataFile 
// with one float value per frame and the same number of frames as the other 
// MatrixDataFiles that correspond to the same data.
//
// Non-constant number of values per frame (frame size):
// Some descriptors require a different number of values per frame. For instance number 
// of spectral peaks (frequency and amplitude) is dependent on pitch and may thus vary 
// from frame-to-frame. To create a file with non-constant number of values per frame, 
// set the "number of values per frame" field in the header to 0 and write values using 
// writeSingleNonConstSizeFrame(). When calling close() in this case, an array of frame 
// sizes is appended to the end of the file (so all the data doesn't have to be moved 
// for every write).
//
// File size limitation:
// Maximum supported file size limited to 2^32 floats (16 GB).

namespace concat
{
	// reading a .dat file
	class MatrixDataFileRead
	{
	public:
		MatrixDataFileRead();
		explicit MatrixDataFileRead(const char *filename);
		~MatrixDataFileRead();

		void open(const char *filename);
		void close();
		bool isOpen() const;

		bool isOk() const;

		double getSampleRate() const;
		uint32_t getHopSize() const;
		bool hasNonConstantFrameRate() const;
		uint32_t getNumValuesPerFrame() const;
		bool hasNonConstantNumValuesPerFrame() const;
		uint32_t getNumFrames() const; // num. frames in file
		uint32_t getDataSizeFloats() const; // size of entire file in num. floats

		int read(float *data, int sizeFrames);
		// sequential read, from current position
		// if passed data argument is NULL, function will return the size of the data to be 
		// read in number of floats (useful for non-const num. values per frame)
		
//		int readNonSeq(float *data, int offsetFrames, int sizeFrames); // non-sequential read, position is not changed; if data is NULL will return the size of the data to be read in number of floats (for non-const num values per frame)

		uint32_t tellFrames() const;
		void seekFromBegin(uint32_t offsetFrames); 

		const uint32_t *getSizeTable() const;
		const uint32_t *getOffsetTable() const;

	public:
		// helper function to be used by external code which is similar to MatrixDataFileRead (in order not to duplicate code)
		static int computeReadTargetSizeFloats(int numFramesToRead, int readPosFrames, int availableFrames, int numItemsPerFrame, const int *sizeTable)
		{
			assert(numItemsPerFrame >= 0);
			assert(availableFrames >= 0);
			assert(numItemsPerFrame >= 1 || sizeTable != NULL);

			// force arguments to be in-range:
			if (numFramesToRead <= 0)
				return 0;

			if (numFramesToRead > availableFrames)
				numFramesToRead = availableFrames;

			// computed required target memory:
			int numFloatsToRead;
			if (numItemsPerFrame == 0)
			{
				numFloatsToRead = 0;
				for (int i = readPosFrames; i < readPosFrames + numFramesToRead; ++i)
				{
					numFloatsToRead += sizeTable[i];
				}
			}
			else
			{
				numFloatsToRead = numFramesToRead*numItemsPerFrame;
			}

			return numFloatsToRead;
		}

	private:
		std::ifstream stream_;
		uint32_t readPosFrames_;

		double sampleRate_;
		uint32_t hopSize_;
		uint32_t numValuesPerFrame_;
		uint32_t numFrames_;
		uint32_t dataSizeFloats_;

		uint32_t *offsetTable_;
		uint32_t *sizeTable_;

		bool isOk_;

		void initState();

		uint32_t remainingBytes();
		uint32_t remainingFrames();

	private:
		MatrixDataFileRead(const MatrixDataFileRead &); // non-copyable
		MatrixDataFileRead &operator=(const MatrixDataFileRead &); // non-copyable
	};

	// -----------------------------------------------------------------------------------

	// writing a .dat file
	class MatrixDataFileWrite
	{
	public:
		MatrixDataFileWrite();
		MatrixDataFileWrite(const char *filename, double sampleRate, int hopSize, int numValuesPerFrame = 1);
		~MatrixDataFileWrite();

		void open(const char *filename, double sampleRate, int hopSize, int numValuesPerFrame = 1);
		void close(); // close also appends size table to end of file (non-const num values per frame)
		bool isOpen() const;

		void write(const float *data, int sizeFrames); // sequential write, from current position
		void writeSingleNonConstSizeFrame(const float *data, int numValuesPerFrame); // sequential write, from current position
		void flush();

	private:
		std::ofstream stream_;

		double sampleRate_;
		uint32_t hopSize_;
		uint32_t numValuesPerFrame_;
		uint32_t numFrames_;
		uint32_t dataSizeFloats_;

		std::vector<uint32_t> sizeTable_;

		void initState();
		void writeHeader();
		void writeSizeTable();

	private:
		MatrixDataFileWrite(const MatrixDataFileWrite &); // non-copyable
		MatrixDataFileWrite &operator=(const MatrixDataFileWrite &); // non-copyable
	};
}

#endif // INCLUDED_CONCAT_MATRIXDATAFILE_HXX



