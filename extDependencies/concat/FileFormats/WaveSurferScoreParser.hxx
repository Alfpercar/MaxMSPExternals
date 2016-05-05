#ifndef INCLUDED_CONCAT_WAVESURFERSCOREPARSER_HXX
#define INCLUDED_CONCAT_WAVESURFERSCOREPARSER_HXX

#include "WaveSurferFile.hxx"

namespace concat
{
	class InputScore;
	class ScoreNote;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include "concat/Utilities/Filename.hxx"

namespace concat
{
	class WaveSurferScoreParser
	{
	public:
		struct Filenames
		{
			Filename notesFilename;
			Filename dynFilename;
			Filename bowDirFilename;
			Filename artTypeFilename;
			Filename stringFilename;
			Filename accentFilename;
			Filename bowIdxFilename;
			Filename dynIdxFilename;
		};

	public:
		static Filenames getFilenames(const char *baseFilename, const char *addFilenameSuffix);

		void load(const char *baseFilename, const char *addFilenameSuffix, concat::InputScore &target, bool allowPerformerAndSampleOverride);
		bool isOk() const;

	private:
		bool failed_;

		void getAdditionalNoteParameters(const concat::WaveSurferEntry &wsNote, int noteIdx, concat::ScoreNote &targetNote);
	};
}

#endif