#ifndef INCLUDED_FILEWRITERS_HXX
#define INCLUDED_FILEWRITERS_HXX

#include "libsndfile/sndfile.h"

// FileWriter for asynchronous file writing that writes files 
// in WAVE (.wav) format.
class WaveFileWriter : public AsynchFileWriter::FileWriterInterface
{
public:
	WaveFileWriter(int numChannels, double sampleRate)
	{
		numChannels_ = numChannels;
		sampleRate_ = sampleRate;
		file_ = NULL;
	}

	WaveFileWriter(const WaveFileWriter &other)
	{
		numChannels_ = other.numChannels_;
		sampleRate_ = other.sampleRate_;
		file_ = NULL;
	}

	int getFrameSize() const
	{
		return numChannels_;
	}

	void openFile(const char *filename)
	{
		SF_INFO info;
		info.channels = numChannels_;
		info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
		info.samplerate = (int)(sampleRate_);
		file_ = sf_open(filename, SFM_WRITE, &info);
	}

	void closeFile()
	{
		if (file_ != NULL)
		{
			sf_close(file_);
			file_ = NULL;
		}
	}

	bool isOpen() const
	{
		return (file_ != NULL);
	}

	void writeItems(const float *buffer, int numItems)
	{
		assert(numItems > 0);
		assert((numItems % numChannels_) == 0);
		// Note: Assume writeData() is called for complete frames only. As writes are 
		// atomic, reads should always give an integer multiple of the frame size number of 
		// items.

		sf_writef_float(file_, buffer, numItems/numChannels_);
	}

private:
	int numChannels_;
	double sampleRate_;
	SNDFILE *file_;
};

// ---------------------------------------------------------------------------------------

#include "concat/FileFormats/MatrixDataFile.hxx"

// FileWriter for asynchronous file writing that writes files 
// in MDF (.dat) format.
class DatFileWriter : public AsynchFileWriter::FileWriterInterface
{
public:
	DatFileWriter(int frameSize, double sampleRate, int hopSize)
	{
		frameSize_ = frameSize;
		sampleRate_ = sampleRate;
		hopSize_ = hopSize;
	}

	DatFileWriter(const DatFileWriter &other)
	{
		frameSize_ = other.frameSize_;
		sampleRate_ = other.sampleRate_;
		hopSize_ = other.hopSize_;
	}

	int getFrameSize() const
	{
		return frameSize_;
	}

	void openFile(const char *filename)
	{
		file_.open(filename, sampleRate_, hopSize_, frameSize_);
	}

	void closeFile()
	{
		file_.close();
	}

	bool isOpen() const
	{
		return file_.isOpen(); 
	}

	void writeItems(const float *buffer, int numItems)
	{
		assert(numItems > 0);
		assert((numItems % frameSize_) == 0);
		// Note: Assume writeData() is called for complete frames only. As writes are 
		// atomic, reads should always give an integer multiple of the frame size number of 
		// items.

		file_.write(buffer, numItems/frameSize_);
	}

private:
	int frameSize_;
	double sampleRate_;
	int hopSize_;
	concat::MatrixDataFileWrite file_;
};

#endif