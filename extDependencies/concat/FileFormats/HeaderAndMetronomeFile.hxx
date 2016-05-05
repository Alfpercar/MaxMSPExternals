#ifndef INCLUDED_CONCAT_HEADERANDMETRONOMEFILE_HXX
#define INCLUDED_CONCAT_HEADERANDMETRONOMEFILE_HXX

#include <fstream>
#include "concat/Utilities/StdInt.hxx"

// ---------------------------------------------------------------------------------------

// 1 = legacy version (some obsolete fields and metronome information in binary format inside header)
// 2 = current version (metronome as separate ascii file)
#define HEADER_WRITE_VERSION 2

// ---------------------------------------------------------------------------------------
 
namespace concat
{
	bool writeHeaderFile(const char *filename,	const double str1Br[3], const double str2Br[3], const double str3Br[3], const double str4Br[3], 
												const double str1Wd[3], const double str2Wd[3], const double str3Wd[3], const double str4Wd[3], 
												const double str1Fb[3], const double str2Fb[3], const double str3Fb[3], const double str4Fb[3], 
												const double frogLhs[3], const double frogRhs[3], const double tipLhs[3], const double tipRhs[3],
												const double str1Br_v2[3], const double str2Br_v2[3], const double str3Br_v2[3], const double str4Br_v2[3], 
												const double str1Wd_v2[3], const double str2Wd_v2[3], const double str3Wd_v2[3], const double str4Wd_v2[3], 
												const double str1Fb_v2[3], const double str2Fb_v2[3], const double str3Fb_v2[3], const double str4Fb_v2[3], 
												const double frogLhs_v2[3], const double frogRhs_v2[3], const double tipLhs_v2[3], const double tipRhs_v2[3]);

	bool writeMetronomeFile(const char *filename, double tempo, int timeSigNum, int timeSigDen);

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	bool readHeaderFile(const char *filename, double *outCalib, int outCalibSize);

	struct MetronomeData
	{
		double tempo;
		int timeSigNum;
		int timeSigDen;
	};

	bool readMetronomeFromMetronomeFile(const char *filename, MetronomeData &result);
	bool readMetronomeFromHeaderFile(const char *filename, MetronomeData &result); // for legacy header format
}

// ---------------------------------------------------------------------------------------

namespace concat
{
	inline bool writeHeaderFile(const char *filename,	const double str1Br[3], const double str2Br[3], const double str3Br[3], const double str4Br[3], 
														const double str1Wd[3], const double str2Wd[3], const double str3Wd[3], const double str4Wd[3], 
														const double str1Fb[3], const double str2Fb[3], const double str3Fb[3], const double str4Fb[3], 
														const double frogLhs[3], const double frogRhs[3], const double tipLhs[3], const double tipRhs[3],
														const double str1Br_v2[3], const double str2Br_v2[3], const double str3Br_v2[3], const double str4Br_v2[3], 
														const double str1Wd_v2[3], const double str2Wd_v2[3], const double str3Wd_v2[3], const double str4Wd_v2[3], 
														const double str1Fb_v2[3], const double str2Fb_v2[3], const double str3Fb_v2[3], const double str4Fb_v2[3], 
														const double frogLhs_v2[3], const double frogRhs_v2[3], const double tipLhs_v2[3], const double tipRhs_v2[3])
	{	
		std::ofstream headerFile;
		headerFile.open(filename, std::ios_base::trunc | std::ios_base::binary);

		if (!headerFile.is_open())
			return false;

#if (HEADER_WRITE_VERSION == 1)
/*		// File format version 1:
		// Four character ID:
		char id[4];
		id[0] = 'V'; // V(iolin recording)
		id[1] = 'H'; // H(ea)
		id[2] = 'd'; // d(e)
		id[3] = 'r'; // r
		headerFile.write(&id[0], sizeof(char));
		headerFile.write(&id[1], sizeof(char));
		headerFile.write(&id[2], sizeof(char));
		headerFile.write(&id[3], sizeof(char));

		// Note:
		// Sample rates are already stored in the audio/tracker files themselves.

		// Input latency audio:
		int32_t inputLatencyAudio = blockSize_;
		headerFile.write((const char *)&inputLatencyAudio, sizeof(int32_t));

		// Input latency tracker:
		int32_t inputLatencyTracker = 0;
		headerFile.write((const char *)&inputLatencyTracker, sizeof(int32_t));

		// Host metronome tempo:
		double hostMetronomeTempo = tempo;
		headerFile.write((const char *)&hostMetronomeTempo, sizeof(double));

		// Host metronome time signature numerator:
		int32_t timeSigNumerator = timeSigNum;
		headerFile.write((const char *)&timeSigNumerator, sizeof(int32_t));

		// Host metronome time signature denominator:
		int32_t timeSigDenominator = timeSigDen;
		headerFile.write((const char *)&timeSigDenominator, sizeof(int32_t));

		// Calibration data:
		headerFile.write((const char *)str1Br, 3*sizeof(double));
		headerFile.write((const char *)str2Br, 3*sizeof(double));
		headerFile.write((const char *)str3Br, 3*sizeof(double));
		headerFile.write((const char *)str4Br, 3*sizeof(double));

		headerFile.write((const char *)str1Wd, 3*sizeof(double));
		headerFile.write((const char *)str2Wd, 3*sizeof(double));
		headerFile.write((const char *)str3Wd, 3*sizeof(double));
		headerFile.write((const char *)str4Wd, 3*sizeof(double));

		headerFile.write((const char *)str1Fb, 3*sizeof(double));
		headerFile.write((const char *)str2Fb, 3*sizeof(double));
		headerFile.write((const char *)str3Fb, 3*sizeof(double));
		headerFile.write((const char *)str4Fb, 3*sizeof(double));

		headerFile.write((const char *)frogLhs, 3*sizeof(double));
		headerFile.write((const char *)frogRhs, 3*sizeof(double));

		headerFile.write((const char *)tipLhs, 3*sizeof(double));
		headerFile.write((const char *)tipRhs, 3*sizeof(double));

		headerFile.close();*/
#elif (HEADER_WRITE_VERSION == 2)
		// File format version 2 and above:
		// Four character ID:
		char id[4];
		id[0] = 'V'; // Violin
		id[1] = 'R'; // Recording
		id[2] = 'H'; // Header
		id[3] = 'X'; // version 2 and above
		headerFile.write(&id[0], sizeof(char));
		headerFile.write(&id[1], sizeof(char));
		headerFile.write(&id[2], sizeof(char));
		headerFile.write(&id[3], sizeof(char));

		int32_t fileFormatVersion = 2;
		headerFile.write((const char *)&fileFormatVersion, sizeof(int32_t));

		// Note:
		// Sample rates are already stored in the audio/tracker files themselves.
		// Input latencies are not important as output from plug-in is supposed to be synchronized.
		// Host metronome information is stored in a easier-to-change/read text format.

		// Calibration data:
		headerFile.write((const char *)str1Br, 3*sizeof(double));
		headerFile.write((const char *)str2Br, 3*sizeof(double));
		headerFile.write((const char *)str3Br, 3*sizeof(double));
		headerFile.write((const char *)str4Br, 3*sizeof(double));

		headerFile.write((const char *)str1Wd, 3*sizeof(double));
		headerFile.write((const char *)str2Wd, 3*sizeof(double));
		headerFile.write((const char *)str3Wd, 3*sizeof(double));
		headerFile.write((const char *)str4Wd, 3*sizeof(double));

		headerFile.write((const char *)str1Fb, 3*sizeof(double));
		headerFile.write((const char *)str2Fb, 3*sizeof(double));
		headerFile.write((const char *)str3Fb, 3*sizeof(double));
		headerFile.write((const char *)str4Fb, 3*sizeof(double));

		headerFile.write((const char *)frogLhs, 3*sizeof(double));
		headerFile.write((const char *)frogRhs, 3*sizeof(double));

		headerFile.write((const char *)tipLhs, 3*sizeof(double));
		headerFile.write((const char *)tipRhs, 3*sizeof(double));

		headerFile.write((const char *)str1Br_v2, 3*sizeof(double));
		headerFile.write((const char *)str2Br_v2, 3*sizeof(double));
		headerFile.write((const char *)str3Br_v2, 3*sizeof(double));
		headerFile.write((const char *)str4Br_v2, 3*sizeof(double));

		headerFile.write((const char *)str1Wd_v2, 3*sizeof(double));
		headerFile.write((const char *)str2Wd_v2, 3*sizeof(double));
		headerFile.write((const char *)str3Wd_v2, 3*sizeof(double));
		headerFile.write((const char *)str4Wd_v2, 3*sizeof(double));

		headerFile.write((const char *)str1Fb_v2, 3*sizeof(double));
		headerFile.write((const char *)str2Fb_v2, 3*sizeof(double));
		headerFile.write((const char *)str3Fb_v2, 3*sizeof(double));
		headerFile.write((const char *)str4Fb_v2, 3*sizeof(double));

		headerFile.write((const char *)frogLhs_v2, 3*sizeof(double));
		headerFile.write((const char *)frogRhs_v2, 3*sizeof(double));

		headerFile.write((const char *)tipLhs_v2, 3*sizeof(double));
		headerFile.write((const char *)tipRhs_v2, 3*sizeof(double));

		headerFile.close();
#else
#error Specified header file version not implemented!
#endif

		return true;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	inline bool writeMetronomeFile(const char *filename, double tempo, int timeSigNum, int timeSigDen)
	{
		std::ofstream metronomeFile;
		metronomeFile.open(filename, std::ios_base::trunc);

		if (!metronomeFile.is_open())
			return false;

		metronomeFile << "tempo = " << tempo << std::endl;
		metronomeFile << "time signature numerator = " << timeSigNum << std::endl;
		metronomeFile << "time signature denominator = " << timeSigDen << std::endl;

		metronomeFile.close();

		return true;
	}

	// -----------------------------------------------------------------------------------

	inline bool readHeaderFile(const char *filename, double *outCalib, int outCalibSize)
	{
		if (outCalibSize != 3*32)
			return false;

		std::ifstream headerFile;
		headerFile.open(filename, std::ios_base::binary);

		if (!headerFile.is_open())
			return false;

		char id[4];
		headerFile.read(&id[0], sizeof(char));
		headerFile.read(&id[1], sizeof(char));
		headerFile.read(&id[2], sizeof(char));
		headerFile.read(&id[3], sizeof(char));

		if (id[0] != 'V' || id[1] != 'R' || id[2] != 'H' || id[3] != 'X' || !headerFile)
		{
			// Note: If version 1 header file, id will be "VHdr".
			headerFile.close();
			return false;
		}

		int32_t fileFormatVersion;
		headerFile.read((char *)&fileFormatVersion, sizeof(int32_t));

		if (fileFormatVersion != 2 || !headerFile)
		{
			headerFile.close();
			return false;
		}

		headerFile.read((char *)outCalib, outCalibSize*sizeof(double));

		if (!headerFile)
		{
			headerFile.close();
			return false;
		}

		headerFile.close();

		return true;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	
	inline bool readMetronomeFromMetronomeFile(const char *filename, MetronomeData &result)
	{
		result.tempo = -1.0; // invalid
		result.timeSigNum = -1; // invalid
		result.timeSigDen = -1; // invalid

		std::fstream file;
		file.open(filename, std::ifstream::in);

		if (!file)
			return false;

		while (!file.fail())
		{
			std::string line;
			std::getline(file, line);

			std::stringstream ssLine(line);
			std::string field;
			
			while (1)
			{
				std::string token;
				ssLine >> token;

				if (ssLine.fail())
					break; // failed getting more tokens

				if (token == "=")
				{
					break; // end of field identifier
				}
				else
				{
					if (field != "")
						field += " ";
					field += token;
				}
			}

			if (field == "tempo")
			{
				ssLine >> result.tempo;
			}
			else if (field == "time signature numerator")
			{
				ssLine >> result.timeSigNum;
			}
			else if (field == "time signature denominator")
			{
				ssLine >> result.timeSigDen;
			}
		}

		file.close();

		if (result.tempo == -1.0 ||
			result.timeSigNum == -1 || 
			result.timeSigDen == -1)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	inline bool readMetronomeFromHeaderFile(const char *filename, MetronomeData &result)
	{
		result.tempo = -1.0; // invalid
		result.timeSigNum = -1; // invalid
		result.timeSigDen = -1; // invalid

		std::fstream file;
		file.open(filename, std::ifstream::in | std::ifstream::binary);

		if (!file)
			return false;

		char id[4];
		file.read(&id[0], 1);
		file.read(&id[1], 1);
		file.read(&id[2], 1);
		file.read(&id[3], 1);

		if (id[0] == 'V' &&
			id[1] == 'H' && 
			id[2] == 'd' && 
			id[3] == 'r')
		{
			// int32 : input latency audio
			// int32 : input latency tracker
			file.seekg(2*4, std::ios::cur); // skip

			if (file.fail())
				return false;

			// double : metronome
			file.read((char *)&result.tempo, sizeof(double));

			if (file.fail())
			{
				result.tempo = -1.0; // invalid
				result.timeSigNum = -1; // invalid
				result.timeSigDen = -1; // invalid
				return false;
			}

			// int32 : time signature numerator
			file.read((char *)&result.timeSigNum, sizeof(int32_t));

			if (file.fail())
			{
				result.tempo = -1.0; // invalid
				result.timeSigNum = -1; // invalid
				result.timeSigDen = -1; // invalid
				return false;
			}

			// int32 : time signature denominator
			file.read((char *)&result.timeSigDen, sizeof(int32_t));

			if (file.fail())
			{
				result.tempo = -1.0; // invalid
				result.timeSigNum = -1; // invalid
				result.timeSigDen = -1; // invalid
				return false;
			}
		}
		else if (id[0] == 'V' &&
			id[1] == 'R' && 
			id[2] == 'H' && 
			id[3] == 'X')
		{
			return false; // version 2 and above recording headers do not contain metronome information (use text metronome file instead)
		}
		else
		{
			return false;
		}

		return true;
	}
}

#endif