#ifndef INCLUDED_CONCAT_FILEVERIFYHELPERS_HXX
#define INCLUDED_CONCAT_FILEVERIFYHELPERS_HXX

#include "libsndfile/sndfile.h"

namespace concat
{
	class AudioFileVerifier
	{
	public:
		AudioFileVerifier(const char *filename)
		{
			SF_INFO audioFileInfo;
			audioFile_ = sf_open(filename, SFM_READ, &audioFileInfo);
			
			if (audioFile_ != NULL)
			{
				sampleRate_ = (double)audioFileInfo.samplerate;
			}
		}

		~AudioFileVerifier()
		{
			if (audioFile_ != NULL)
				sf_close(audioFile_);
		}

		bool operator!() const
		{
			return (audioFile_ == NULL);
		}

		double getSampleRate() const
		{
			return sampleRate_;
		}

	private:
		SNDFILE *audioFile_;
		double sampleRate_;
	};
}

// ---------------------------------------------------------------------------------------

#include "concat/FileFormats/MatrixDataFile.hxx"

namespace concat
{
	class MatrixDataFileVerifier
	{
	public:
		MatrixDataFileVerifier(const char *filename)
		{
			file_.open(filename);
			sampleRate_ = file_.getSampleRate();
			hopSize_ = file_.getHopSize();
		}

		~MatrixDataFileVerifier()
		{
			file_.close();
		}

		bool operator!() const
		{
			return (!file_.isOk());
		}

		double getSampleRate() const
		{
			return sampleRate_;
		}

		int getHopSize() const
		{
			return hopSize_;
		}

	private:
		MatrixDataFileRead file_;
		double sampleRate_;
		int hopSize_;
	};
}

#endif