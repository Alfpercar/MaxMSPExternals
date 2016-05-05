#ifndef INCLUDED_CONCAT_MUSICXMLPARSER_HXX
#define INCLUDED_CONCAT_MUSICXMLPARSER_HXX

#include "tinyxml/tinyxml.h"

#include "concat/InputScore/KeySignature.hxx"
#include "concat/InputScore/TimeSignature.hxx"
#include "concat/InputScore/ScoreNote.hxx"
#include "concat/InputScore/ViolinEnumStringMaps.hxx"

namespace concat
{
	class InputScore;
}
class TiXmlDocument;
class TiXmlElement;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

namespace concat
{
	// tempo helper
	struct ScoreTempo
	{
		int ticksPerBeat;
		double BPM;
		double posSeconds;
		double posBeats;

		ScoreTempo()
		{
			ticksPerBeat = -1; // invalid
			BPM = -1.0; // invalid
			posSeconds = -1.0; // invalid
			posBeats = -1.0; // invalid
		}
	};
}

namespace concat
{
	class MusicXmlParser
	{
	public:
		// MusicXmlParser can split up a MusicXml file by calling load() multiple times 
		// (parsing continues)
		enum ParseStopEvent
		{
			NEVER,
			ANY_DOUBLE_BAR,				// light-light or light-heavy
			LIGHT_LIGHT_DOUBLE_BAR,
			LIGHT_HEAVY_DOUBLE_BAR,
			NEW_SYSTEM,
			NEXT_MEASURE_PHRASE_LABEL	// next label starting with "Phrase "
		};

		enum ParseBeginPoint
		{
			BOF,		// beginning of file (ie. first measure)
			CONTINUE,	// continue from previous load() calls (if first call same as BOF)
			FIND_PHRASE	// find phrase with a specific text label
		};

		MusicXmlParser();

		void init();

		void load(const char *filename, concat::InputScore &target, ParseBeginPoint parseBeginPoint = BOF, ParseStopEvent parseStopEvent = NEVER, std::string parseBeginFindLabel = "", bool allowMissingString = true, bool allowMissingBowDir = true, bool joinTiedNotes = true, DynamicsValue defaultDyn = DynamicsValue::MF, bool defaultDynIsOverride = false);
		bool isOk() const;

		bool hasMorePhrases() const; // for iterative parsing

	private:
		void initializeParsingVariables(concat::InputScore &target);

		bool processCurMeasure(concat::InputScore &target, bool addNotesToTarget, bool allowMissingString, DynamicsValue defaultDyn, bool defaultDynIsOverride);

		bool failed_;

		TiXmlDocument doc_;
		TiXmlElement *part_;
		TiXmlElement *curMeasure_;

		ScoreTempo actTempo_;
		ScoreTempo lastTempo_;

		TimeSignature actTimeSig_;
		TimeSignature lastTimeSig_;

		KeySignature actKeySig_;
		KeySignature lastKeySig_;

		int curMeasureIdx_; // for debugging/logging purposes
		int curNoteIdx_;
		ScoreNote actNote_;
		ScoreNote lastNote_;

		std::string filename_;

		enum ArticulationTypeModifier
		{
			ARCO,
			PIZZ
		};

		ArticulationTypeModifier artTypeMod_;

		double curRestPizzSeqAccumDurBeats_; // if cur note is rest, its duration plus all rests before it (until next non-rest), else zero
		double prevRestPizzSeqAccumDurBeats_; // as curRestSeqAccumDurBeats_, but delayed by one note
		int lastNonRestNonPizzBowDirIdx_; // bow dir (as index) of last non-rest/non-pizz. note

		DynamicsType curDynType_;
		DynamicsValue curDynValue_;
		bool assignCurDynValToNextNote_;
		bool crescDecrescStop_;

		int curPlayedString_;
		int curHandPos_;

		// helper functions
		void updateTempo(TiXmlElement *measure, ScoreTempo &actTempo, const ScoreTempo &lastTempo);
		void updateTimeSig(TiXmlElement *measure, concat::TimeSignature &actTimeSig, const concat::TimeSignature &lastTimeSig);
		void updateKeySig(TiXmlElement *measure, concat::KeySignature &actKeySig, const concat::KeySignature &lastKeySig);

		void getNoteArticulationType(TiXmlElement *note, ScoreNote &actNote, concat::MusicXmlParser::ArticulationTypeModifier artTypeMod);
		void getNotePitchOrRest(TiXmlElement *note, ScoreNote &actNote);
		void getNoteAccent(TiXmlElement *notations, ScoreNote &actNote);
		void getNoteFinger(TiXmlElement *notations, ScoreNote &actNote);
		void getNoteBow(TiXmlElement *curMeasureElement, TiXmlElement *noteElement, ScoreNote &actNote, ScoreNote &lastNote);	// may also affect lastNote for fixing mal-formed MusicXMLs
		void getNoteDuration(TiXmlElement *note, const ScoreTempo &tempo, ScoreNote &actNote);
		
		void getCurDynamic(TiXmlElement *direction);
		void getCurPlayedString(TiXmlElement *direction);
		void getCurHandPosition(TiXmlElement *direction);
		void getCurArtTypeMod(TiXmlElement *direction);

//		bool setStringFingerHandposValid(ScoreNote &actNote, const ScoreNote &lastNote);

	private:
		TiXmlElement *getNextNoteElement(TiXmlElement *noteElement);

		void applyBowDirectionChange(const TiXmlElement *down, const TiXmlElement *up, ScoreNote &actNote);
		void invertLastBowDirection(ScoreNote &actNote, const ScoreNote &lastNote);
		void continueLastBowDirection(ScoreNote &actNote, const ScoreNote &lastNote);

		bool isNextNoteTieAsSlurEnd(TiXmlElement *curMeasureElement, TiXmlElement *noteElement, ScoreNote &actNote, int tieAsSlurLevel);

	private:
		void logTiesSlurs(concat::InputScore &target);
		void convertTiesAsSlurToTies(concat::InputScore &target);
		void joinTies(concat::InputScore &target);
		void computeBowsFromSlurs(concat::InputScore &target);
		void convertSingleNoteLegatosToDetache(concat::InputScore &target);

		void assignDynamicsToAllNotes(concat::InputScore &target);
	};
}

#endif