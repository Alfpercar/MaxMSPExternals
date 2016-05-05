#include "MusicXmlParser.hxx"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "concat/InputScore/ScoreNote.hxx"
#include "concat/InputScore/ScoreBow.hxx"
#include "concat/InputScore/InputScore.hxx"
#include "concat/InputScore/ViolinScoreConversions.hxx"

#include "concat/Utilities/XmlHelpers.hxx"
#include "concat/Utilities/Logging.hxx"
#include "concat/Utilities/StringHelpers.hxx"
#include "concat/Utilities/StringConversions.hxx"
#include "concat/Utilities/FloatEq.hxx"

using namespace std;
using namespace concat;

// ---------------------------------------------------------------------------------------

namespace concat
{
	MusicXmlParser::MusicXmlParser()
	{
		init();
	}

	void MusicXmlParser::init()
	{
		failed_ = true;
		curMeasure_ = NULL;
		filename_ = "";
	}

	// helper to compare two labels in a special way
	bool doLabelsMatch(std::string label1, std::string label2)
	{
		// labelPlus should be label plus pre/post fix:
		std::string label;
		std::string labelPlus;
		if (label1.size() >= label2.size())
		{
			labelPlus = label1;
			label = label2;
		}
		else
		{
			labelPlus = label2;
			label = label1;
		}

		// Convert both to lower case:
		strToLower(label);
		strToLower(labelPlus);

		// Replace spaces, dashes and dots with underscores:
		strReplaceAll(label, " ", "_");
		strReplaceAll(label, "-", "_");
		strReplaceAll(label, ".", "_");
		strReplaceAll(labelPlus, " ", "_");
		strReplaceAll(labelPlus, "-", "_");
		strReplaceAll(labelPlus, ".", "_");

		// Avoid using string labels for label
		// (e.g. when label = "a" and labelPlus = "1string_a", it would otherwise be 
		// considered a valid match):
		if (label == "g" || label == "d" || label == "e" || label == "a")
			return false;

		// Avoid using fingering labels for label
		// (e.g. when label = "1" and labelPlus = "4string_1_mf", it would otherwise be 
		// considered a valid match):
		if (label == "0" || label == "1" || label == "2" || label == "3" || label == "4")
			return false;

		// Consider the labels matching if they are equal or 
		// if they are equal not considering pre and post fixes, 
		// requiring that pre and post fixes are separated by an underscore:
		if (label == labelPlus)
			return true;

		std::string::size_type matchBegin = labelPlus.find(label);
		if (matchBegin != std::string::npos)
		{
			std::string::size_type matchEnd = matchBegin + label.size();

			bool beginOk = false;
			bool endOk = false;

			if (matchBegin == 0)
				beginOk = true;
			else if (labelPlus[matchBegin - 1] == '_')
				beginOk = true;

			if (matchEnd == labelPlus.size())
				endOk = true;
			else if (labelPlus[matchEnd] == '_')
				endOk = true;

			if (beginOk && endOk)
				return true;
		}

		return false;
	}

	void MusicXmlParser::load(const char *filename, concat::InputScore &target, ParseBeginPoint parseBeginPoint, ParseStopEvent parseStopEvent, std::string parseBeginFindLabel, bool allowMissingString, bool allowMissingBowDir, bool joinTiedNotes, DynamicsValue defaultDyn, bool defaultDynIsOverride)
	{
		// First time calling load() or when calling load() with a 
		// different filename, parse XML to in-memory DOM structure:
		if (curMeasure_ == NULL || filename_ != filename)
		{
			doc_ = TiXmlDocument();
			bool ok = loadXml(filename, doc_);

			if (!ok)
			{
				LOG_ERROR_N("concat.musicxmlparser", "Error: Input XML file does not exist or failed to parse.");
				failed_ = true;
				return;
			}

			filename_ = filename;

			// Get root element (<score-partwise>):
			TiXmlElement *root = doc_.FirstChildElement();
			if (root == NULL || std::string(root->Value()) != std::string("score-partwise"))
			{
				LOG_ERROR_N("concat.musicxmlparser", "Error: No or invalid root element in input XML file, should be \"<score-partwise>\".");
				failed_ = true;
				return;
			}

			// Get first part (assume only one part):
			part_ = root->FirstChildElement("part");
			if (part_ == NULL)
			{
				LOG_ERROR_N("concat.musicxmlparser", "Error: No or invalid \"<part>\" elements found in input XML file.");
				failed_ = true;
				return;
			}
		}

		if (parseBeginPoint == BOF || (parseBeginPoint == CONTINUE && curMeasure_ == NULL))
		{
			// Initialize parsing variables:
			initializeParsingVariables(target);

			// Go to first measure:
			curMeasure_ = part_->FirstChildElement("measure");
			// (check if <measure> was found later)
		}
		else if (parseBeginPoint == CONTINUE)
		{
			// Reset parsing target position:
			target.setNumMeasures(0);
			actTempo_.posSeconds = 0.0;
			actTempo_.posBeats = 0.0;
			artTypeMod_ = ARCO;
		}
		else if (parseBeginPoint == FIND_PHRASE)
		{
			// Initialize parsing variables:
			initializeParsingVariables(target);

			// Find specified phrase begin measure:
			bool foundLabel = false;
			std::string foundLabelName;
			for (curMeasure_ = part_->FirstChildElement("measure"); curMeasure_ != NULL; curMeasure_ = curMeasure_->NextSiblingElement("measure"))
			{
				// Check if measure has phrase label:
				TiXmlElement *direction;
				for (direction = curMeasure_->FirstChildElement("direction"); direction != NULL; direction = direction->NextSiblingElement("direction"))
				{
					TiXmlElement *directionType = direction->FirstChildElement("direction-type");
					if (directionType == NULL)
						continue;

					TiXmlElement *words = directionType->FirstChildElement("words");
					if (words == NULL)
						continue;

					std::string label = words->GetText();

					if (doLabelsMatch(parseBeginFindLabel, label))
					{
						foundLabel = true;
						foundLabelName = label;
						break;
					}
				}

				// If matching label found, stop searching:
				if (foundLabel)
					break;

				// Disable logging:
				Logger::Level prevLogLevel = Logger::getLogger("concat.musicxmlparser").getLevel();
				Logger::getLogger("concat.musicxmlparser").setLevel(Logger::LEVEL_OFF);

				// Process measure to keep running variables updated:
				if (!processCurMeasure(target, false, allowMissingString, defaultDyn, defaultDynIsOverride))
				{
					LOG_ERROR_N("concat.musicxmlparser", "Error: Failed processing measure.");
					failed_ = true;
					return;
				}

				// Re-enable logging:
				Logger::getLogger("concat.musicxmlparser").setLevel(prevLogLevel);
			}

			// Reset parsing target position (calling processCurMeasure() in for-loop above modifies position):
			target.setNumMeasures(0);
			actTempo_.posSeconds = 0.0;
			actTempo_.posBeats = 0.0;
			artTypeMod_ = ARCO;

			// Log (not) found phrase:
			if (!foundLabel)
			{
				LOG_ERROR_N("concat.musicxmlparser", formatStr("Error: Could not find specified phrase (\"%s\").", parseBeginFindLabel.c_str()));
				failed_ = true;
				return;
			}

			LOG_INFO_N("concat.musicxmlparser", formatStr("Found matching label: \"%s\".", foundLabelName.c_str()));
		}

		// always reset dynamics type/value for a new phrase:
		// (will cause the first note to have dynamics assigned)
		curDynType_ = DynamicsType::NONE;
		curDynValue_ = DynamicsValue::NONE;

		// Go through all measures in part:
		while (curMeasure_ != NULL)
		{
			int measureNumberBase1 = 0; // invalid
			curMeasure_->QueryIntAttribute("number", &measureNumberBase1);
			if (measureNumberBase1 == 0)
				LOG_INFO_N("concat.musicxmlparser", "Processing measure (number unknown)...");
			else
				LOG_INFO_N("concat.musicxmlparser", formatStr("Processing measure %d...", measureNumberBase1));

			if (!processCurMeasure(target, true, allowMissingString, defaultDyn, defaultDynIsOverride))
				return;

			// Check if measure ends in double bar:
			bool shouldEndPhrase = false;
			if (parseStopEvent == ANY_DOUBLE_BAR || parseStopEvent == LIGHT_LIGHT_DOUBLE_BAR || parseStopEvent == LIGHT_HEAVY_DOUBLE_BAR)
			{
				TiXmlElement *barline = curMeasure_->FirstChildElement("barline");
				if (barline != NULL)
				{
					TiXmlElement *barStyle = barline->FirstChildElement("bar-style");
					if (barStyle != NULL)
					{
						std::string barStyleText = barStyle->GetText();
						if (parseStopEvent == ANY_DOUBLE_BAR && (barStyleText == "light-light" || barStyleText == "light-heavy"))
							shouldEndPhrase = true;
						else if (parseStopEvent == LIGHT_LIGHT_DOUBLE_BAR && barStyleText == "light-light")
							shouldEndPhrase = true;
						else if (parseStopEvent == LIGHT_HEAVY_DOUBLE_BAR && barStyleText == "light-heavy")
							shouldEndPhrase = true;
					}
				}
			}
			else if (parseStopEvent == NEW_SYSTEM)
			{
				TiXmlElement *print = curMeasure_->FirstChildElement("print");
				if (print != NULL)
				{
					const char *newSystem = print->Attribute("new-system");
					if (newSystem != NULL)
						if (std::string(newSystem) == "yes")
							shouldEndPhrase = true;
				}
			}
			else if (parseStopEvent == NEXT_MEASURE_PHRASE_LABEL)
			{
				TiXmlElement *nextMeasure = curMeasure_->NextSiblingElement("measure");
				if (nextMeasure != NULL)
				{
					TiXmlElement *direction;
					for (direction = nextMeasure->FirstChildElement("direction"); direction != NULL; direction = direction->NextSiblingElement("direction"))
					{
						TiXmlElement *directionType = direction->FirstChildElement("direction-type");
						if (directionType == NULL)
							continue;

						TiXmlElement *words = directionType->FirstChildElement("words");
						if (words == NULL)
							continue;

						std::string label = words->GetText();

						// NOTE: Requires label to start with "Phrase " to be able to 
						// differentiate between normal <directions> (say "aggresive") and 
						// phrase label <directions>!
						if (label.find("Phrase ") == 0)
						{
							shouldEndPhrase = true;
							break;
						}
					}
				}
			}

			// Next measure:
			curMeasure_ = curMeasure_->NextSiblingElement("measure");
			target.setNumMeasures(target.getNumMeasures() + 1);

			// Exit loop if needed:
			if (shouldEndPhrase)
				break;
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		// Correct last tied note of mal-formed MusicXML (missing tie stop):
		if (target.getNumNotes() > 0)
		{
			if (target.getNoteAt(target.getNumNotes()-1).isTied())
				target.getNoteAt(target.getNumNotes()-1).setTieStop();
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		LOG_INFO_N("concat.musicxmlparser", "Performing post-parsing steps and sanity checks...");

		// Correct measure indexes and measure's first note index (for logging purposes):
		int firstNoteMeasureIdx = 0;
		int firstNoteMeasureFirstNoteIdx = 0;

		if (target.getNumNotes() > 0)
		{
			const ScoreNote &firstNote = target.getNoteAt(0);

			firstNoteMeasureIdx = firstNote.getMeasureIdx();
			firstNoteMeasureFirstNoteIdx = firstNote.getMeasureFirstNoteIdx();

			for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
			{
				ScoreNote &note = target.getNoteAt(noteIdx);
				note.setMeasureIdx(note.getMeasureIdx() - firstNoteMeasureIdx);
				note.setMeasureFirstNoteIdx(note.getMeasureFirstNoteIdx() - firstNoteMeasureFirstNoteIdx);
			}
		}

		// Sanity check:
		// See if all note pitches are valid for played string fields.
		// Note:
		// Do *before* changing ties/slurs because other wise relative note numbers won't match any more.
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			const ScoreNote &note = target.getNoteAt(noteIdx);

			if (note.isRest())
				continue;

			if (!note.isPlayedStringValid())
			{
				LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Note %d (measure %d) has no valid played string specified in score.", 1+note.getNoteIdxRelativeToBeginMeasure(), 1+note.getMeasureIdx()+firstNoteMeasureIdx));
			}
			else
			{
				if (!note.isValidNotePitchForPlayedString(24))
				{
					LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Note %d (measure %d) is out of pitch range of played string (allowing up to two octaves).", 1+note.getNoteIdxRelativeToBeginMeasure(), 1+note.getMeasureIdx()+firstNoteMeasureIdx));
					LOG_WARN_N("concat.musicxmlparser", formatStr("         %s (%d) does not lie on %s string; between %s (%d) and %s (%d).",
						note.getPitchStr(), note.getMidiNote(),
						note.getPlayedString(),
						convertNotePitchFromMidiNoteToStringNoKey(note.getPlayedStringAsMidiNote()).c_str(), note.getPlayedStringAsMidiNote(),
						convertNotePitchFromMidiNoteToStringNoKey(note.getPlayedStringAsMidiNote()+24).c_str(), note.getPlayedStringAsMidiNote()+24));
				}

			}
		}

		// Sanity check:
		// See if all non-rest/non-pizzicatos notes have a bow direction set.
		// See if all rests/pizzicatos have no bow direction set.
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			const ScoreNote &note = target.getNoteAt(noteIdx);

			if (!note.isRest() && !note.isPizzicato())
			{
				if (!note.getBowDirection() == true)
					LOG_ERROR_N("concat.musicxmlparser", formatStr("Error: Sanity check; note %d is non-rest/non-pizzicato but has no bow direction set!", 1+noteIdx));
			}
			else
			{
				if (!note.getBowDirection() == false)
					LOG_ERROR_N("concat.musicxmlparser", formatStr("Error: Sanity check; note %d is rest/pizzicato but has bow direction set!", 1+noteIdx));
			}
		}

		// Sanity check:
		// See if all notes within ties (as slur) and slurs have same bow direction.
		int curSlurBowDir;
		int curTieBowDir;
		int curTieAsSlurBowDir;
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			const ScoreNote &note = target.getNoteAt(noteIdx);

			if (note.isSlurStart())
				curSlurBowDir = note.getBowDirectionAsInt();
			if (note.isTieStart())
				curTieBowDir = note.getBowDirectionAsInt();
			if (note.isTieAsSlurStart(0) || note.isTieAsSlurStart(1))
				curTieAsSlurBowDir = note.getBowDirectionAsInt();

			if (note.isSlurred())
			{
				if (note.getBowDirectionAsInt() != curSlurBowDir)
					LOG_ERROR_N("concat.musicxmlparser", formatStr("Error: Sanity check; note %d is in slur but does not match bow direction of other notes in slur!", 1+noteIdx));
			}

			if (note.isTied())
			{
				if (note.getBowDirectionAsInt() != curTieBowDir)
					LOG_ERROR_N("concat.musicxmlparser", formatStr("Error: Sanity check; note %d is in tie but does not match bow direction of other notes in tie!", 1+noteIdx));
			}

			if (note.isTieAsSlurStop())
			{
				if (note.getBowDirectionAsInt() != curTieAsSlurBowDir)
					LOG_ERROR_N("concat.musicxmlparser", formatStr("Error: Sanity check; note %d is in tie as slur but does not match bow direction of other notes in tie as slur!", 1+noteIdx));
			}
		}

		// Sanity check:
		// See if there aren't two (or more) adjacent notes in a slur that have the same pitch.
		// With the exception of portato notes (the same note played in a single 
		// bow/slur with multiple tenuto accents). Staccato, etc. notes can also have same 
		// pitch notes in a single bow.
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			const ScoreNote &note = target.getNoteAt(noteIdx);

			if (note.isSlurStart())
			{
				int slurStartIdx = noteIdx;
				int slurStopIdx = -1;

				for (int noteIdxSearch = slurStartIdx+1; noteIdxSearch < target.getNumNotes(); ++noteIdxSearch)
				{
					ScoreNote &next = target.getNoteAt(noteIdxSearch);
					if (next.isSlurStop())
					{
						slurStopIdx = noteIdxSearch;
						break;
					}
				}

				if (slurStopIdx != -1)
				{
					int lastPitch = -1;
					bool lastHasTenuto;
					bool lastTieAsSlurStart;

					for (int noteIdxCheck = slurStartIdx; noteIdxCheck <= slurStopIdx; ++noteIdxCheck)
					{
						ScoreNote &check = target.getNoteAt(noteIdxCheck);
						int curPitch = check.getMidiNote();
						bool curHasTenuto = check.hasAccentTenuto();
						bool curTieAsSlurStop = check.isTieAsSlurStop();
						bool curTieStop = check.isTieStop();
						bool curIsLegatoOrDetache = (check.isLegato() || check.isDetache());

						if (lastPitch != -1 && curPitch != -1)
						{
							if (curIsLegatoOrDetache && 
								lastPitch == curPitch && !(lastHasTenuto || curHasTenuto) &&
								!(lastTieAsSlurStart && curTieAsSlurStop) && !check.isTied())
							{
								LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Note %d (measure %d) and %d (measure %d) have same pitch, but are in slur!", 1+check.getPrevSibling()->getNoteIdxRelativeToBeginMeasure(), 1+check.getPrevSibling()->getMeasureIdx()+firstNoteMeasureIdx, 1+check.getNoteIdxRelativeToBeginMeasure(), 1+check.getMeasureIdx()+firstNoteMeasureIdx));
							}
						}

						lastPitch = curPitch;
						lastHasTenuto = curHasTenuto;
						lastTieAsSlurStart = (check.isTieAsSlurStart(0) || check.isTieAsSlurStart(1));
					}
				}
			}
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		// Join ties ("legato" notes with the same pitch) to single notes (only a notation thing):

		convertTiesAsSlurToTies(target);

		if (joinTiedNotes)
		{
			joinTies(target);
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		// Compute bows (position/length) from slurs:

		computeBowsFromSlurs(target);

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		// Convert single note 'legatos' to detaches
		// (because both detache and legato notes are initially set to the 'legato' 
		// articulation type):
		// Note: Must be done AFTER computeBowsFromSlurs().

		convertSingleNoteLegatosToDetache(target);

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		// Assign dynamics value to each note (needs to be done after cresc./decresc. lengths are known):

		assignDynamicsToAllNotes(target);

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		// Add bows:
		for (int i = 0; i < target.getNumNotes(); ++i)
		{
			ScoreNote &note = target.getNoteAt(i);

			if (note.getBowNoteIndex() == 0)
			{
				target.pushBackBow(i, i + note.getBowLength());
			}
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		// Sanity check:
		// See if all notes are continuous (i.e. note[i].begin == note[i-1].end).
		// See if all notes have correct note index field.
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			const ScoreNote &note = target.getNoteAt(noteIdx);

			if (noteIdx > 0)
			{
				const ScoreNote &notePrev = target.getNoteAt(noteIdx-1);

				if (!floatEq(note.getBeginSeconds(), notePrev.getEndSeconds()) || 
					!floatEq(note.getBeginBeats(), notePrev.getEndBeats()))
				{
					LOG_ERROR_N("concat.musicxmlparser", formatStr("Error: Sanity check; note %d and note %d are not continuous!", 1+noteIdx-1, 1+noteIdx));
				}
			}

			if (note.getNoteIdx() != noteIdx)
			{
				LOG_ERROR_N("concat.musicxmlparser", formatStr("Error: Sanity check; note %d in sequence has incorrect note index field (%d).", 1+noteIdx, 1+note.getNoteIdx()));
			}
		}

		failed_ = false;
	}

	// -----------------------------------------------------------------------------------

	bool MusicXmlParser::isOk() const
	{
		return (failed_ == false);
	}

	// -----------------------------------------------------------------------------------

	bool MusicXmlParser::hasMorePhrases() const
	{
		return (curMeasure_ != NULL);
	}

	// -----------------------------------------------------------------------------------

	bool MusicXmlParser::processCurMeasure(concat::InputScore &target, bool addNotesToTarget, bool allowMissingString, DynamicsValue defaultDyn, bool defaultDynIsOverride)
	{
		if (curMeasure_ == NULL)
			return false;

		updateTempo(curMeasure_, actTempo_, lastTempo_);
		updateTimeSig(curMeasure_, actTimeSig_, lastTimeSig_);
		updateKeySig(curMeasure_, actKeySig_, lastKeySig_);

		if (!(lastTempo_.BPM == -1 || actTempo_.BPM == lastTempo_.BPM))
		{
			LOG_WARN_N("concat.musicxmlparser", "Warning: Tempo change in score, but not supported by parser!");
		}
		// XXX:
		// Should implement multiple tempo/time sig/key sig per score, at very least 1 per measure.

		target.setTempo(actTempo_.BPM, false);
		target.setTimeSig(actTimeSig_);
		target.setKeySig(actKeySig_);

		int curMeasureFirstNoteIdx = -1;
		bool wasCurMeasureFirstNoteIdxSet = false;

		for (TiXmlElement *element = curMeasure_->FirstChildElement(); element != NULL; element = element->NextSiblingElement()) 
		{
			// Directions affecting following note(s):
			if (element->ValueStr() == "direction")
			{ 
				TiXmlElement *direction = element;

				getCurDynamic(direction);
				getCurPlayedString(direction);
				getCurHandPosition(direction);
				getCurArtTypeMod(direction);

				// force all dynamics to default dynamic:
				if (defaultDynIsOverride)
				{
					curDynType_ = DynamicsType::NORMAL;
					curDynValue_ = defaultDyn;
					assignCurDynValToNextNote_ = true;
				}
			}
			// Note element affecting only current note/rest:
			else if (element->ValueStr() == "note")
			{
				TiXmlElement *note = element;

				if (note->FirstChildElement("chord") != NULL)
					continue; // skip chord notes (double strings, harmonics, non-violin scores, etc.)

				if (!wasCurMeasureFirstNoteIdxSet)
				{
					curMeasureFirstNoteIdx = curNoteIdx_;
					wasCurMeasureFirstNoteIdxSet = true;
				}

				actNote_.setNoteIdx(curNoteIdx_);
				actNote_.setMeasureIdx(curMeasureIdx_);
				actNote_.setMeasureFirstNoteIdx(curMeasureFirstNoteIdx);

				getNoteDuration(note, actTempo_, actNote_);
				getNotePitchOrRest(note, actNote_);

				// Non-rest note:
				if (!actNote_.isRest())
				{
					// Get other note attributes:
					TiXmlElement *notations = note->FirstChildElement("notations");
					getNoteArticulationType(note, actNote_, artTypeMod_); // call after getNoteDuration()
					getNoteBow(curMeasure_, note, actNote_, lastNote_); // call after getNoteArticulationType(), getNoteBow() can change lastNote for mal-formed MusicXMLs
					getNoteAccent(notations, actNote_);
					getNoteFinger(notations, actNote_);

					// Apply current dynamics:
					if (!curDynType_ || curDynType_ == DynamicsType::NONE ||
						!curDynValue_ || curDynValue_ == DynamicsValue::NONE)
					{
						LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: First non-rest note must have dynamics specified (note %d). Using default (%s).", 1+actNote_.getNoteIdx(), defaultDyn.getAsString()));
						curDynType_ = DynamicsType::NORMAL;
						curDynValue_ = defaultDyn;
						assignCurDynValToNextNote_ = true;
					}

					assert((!curDynType_) == false);
					assert(curDynType_ != DynamicsType::NONE);
					assert((!curDynValue_) == false);
					assert(curDynValue_ != DynamicsValue::NONE);
					if (curDynType_ == DynamicsType::NORMAL)
					{
						// normal:
						DynamicsValue dynVal = DynamicsValue::NONE;
						if (assignCurDynValToNextNote_) // only set dynamics value for notes that immediately follow dynamics directive (NONE for others, will be set later)
						{
							dynVal = curDynValue_;
							assignCurDynValToNextNote_ = false;
						}

						actNote_.setDynamics(curDynType_, dynVal);
					}
					else
					{
						// cresc/decresc
						DynamicsValue dynVal = DynamicsValue::NONE;
						if (assignCurDynValToNextNote_) // only set dynamics value for notes that immediately follow dynamics directive (NONE for others, will be set later)
						{
							dynVal = curDynValue_;
							assignCurDynValToNextNote_ = false;
						}

						actNote_.setDynamics(curDynType_, dynVal);

						// cresc/decresc stop:
						if (crescDecrescStop_)
						{
							// store stop flag for later when setting dynamics gesture 
							// length and indexes:
							actNote_.setCrescDecrescStop();

							// reset dynamics type for next note:
							curDynType_ = DynamicsType::NORMAL;
							crescDecrescStop_ = false;
						}
					}

					// Apply current played string (even past rests):
					if (curPlayedString_ != -1)
						actNote_.setPlayedStringAsInt(curPlayedString_);

					// Apply current hand position (even past rests):
					if (curHandPos_ != -1)
						actNote_.setHandPos(curHandPos_);

					// XXX: ????????????
					// XXX: disabled because would incorrectly set string!!
					// Set string/fingering/handpos:
					//if (!setStringFingerHandposValid(actNote_, lastNote_) && !allowMissingString)
					//{
					//	LOG_ERROR_N("concat.musicxmlparser", formatStr("Error: Could not determine string/hand position with only fingering at note %d (measure %d)." , 1+actNote_.getNoteIdxRelativeToBeginMeasure(), 1+actNote_.getMeasureIdx()));
					//	failed_ = true;
					//	return false;
					//}

					//if (actNote_.getHandPos() == -1)
					//	actNote_.setHandPos(1);
					// XXX: ????????????

					if (actNote_.isPizzicato())
					{
						curRestPizzSeqAccumDurBeats_ += actNote_.getDurationBeats(); // rest (for first rest in sequence curRestAccumDurBeats_ will always be zero)
					}
					else
					{
						curRestPizzSeqAccumDurBeats_ = 0.0; // not rest
						lastNonRestNonPizzBowDirIdx_ = actNote_.getBowDirectionAsInt();
					}
				}
				// Rest note:
				else
				{
					// getNoteBow() can change lastNote for mal-formed MusicXMLs:
					getNoteBow(curMeasure_, note, actNote_, lastNote_);

					curRestPizzSeqAccumDurBeats_ += actNote_.getDurationBeats(); // rest (for first rest in sequence curRestAccumDurBeats_ will always be zero)
				}

				// Add note to target:
				if (addNotesToTarget)
				{
					// the last note may have also changed:
					if (target.getNumNotes() > 0)
					{
						target.popBackNote();
						target.pushBackNote(lastNote_);
					}

					target.pushBackNote(actNote_);
				}

				// Store running state for next note:
				actTempo_.posSeconds += actNote_.getDurationSeconds();
				actTempo_.posBeats += actNote_.getDurationBeats();
				lastNote_ = actNote_;
				actNote_ = ScoreNote();
				curNoteIdx_++;
				prevRestPizzSeqAccumDurBeats_ = curRestPizzSeqAccumDurBeats_;
			}
		}

		// Remember for next iteration:
		lastTempo_ = actTempo_;
		lastTimeSig_ = actTimeSig_;
		lastKeySig_ = actKeySig_;
		curMeasureIdx_++;

		return true;
	}

	void MusicXmlParser::initializeParsingVariables(concat::InputScore &target)
	{
		actTempo_ = ScoreTempo(); // invalid
		lastTempo_ = ScoreTempo(); // invalid

		actTimeSig_ = TimeSignature(); // invalid
		lastTimeSig_ = TimeSignature(); // invalid

		actKeySig_ = KeySignature(); // invalid
		lastKeySig_ = KeySignature(); // invalid

		actNote_ = ScoreNote(); // invalid
		lastNote_ = ScoreNote(); // invalid

		actTempo_.posSeconds = 0.0;
		actTempo_.posBeats = 0.0;

		target.setNumMeasures(0);
		curMeasureIdx_ = 0; // for debugging/logging purposes
		curNoteIdx_ = 0;

		artTypeMod_ = ARCO;

		curRestPizzSeqAccumDurBeats_ = 0.0;
		prevRestPizzSeqAccumDurBeats_ = 0.0;
		lastNonRestNonPizzBowDirIdx_ = -1; // invalid

		curDynType_ = DynamicsType::NONE; // invalid
		curDynValue_ = DynamicsValue::NONE; // invalid
		assignCurDynValToNextNote_ = false; // don't assign to next note
		crescDecrescStop_ = false;

		curPlayedString_ = -1;
		curHandPos_ = -1;
	}
}

// ---------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------

namespace concat
{
	void MusicXmlParser::updateTempo(TiXmlElement *measure, ScoreTempo &actTempo, const ScoreTempo &lastTempo)
	{
		// Get timing resolution:
		actTempo.ticksPerBeat = -1; // invalid
		TiXmlElement *attributes = measure->FirstChildElement("attributes");

		if (attributes != NULL)
		{ 
			TiXmlElement *divisions = attributes->FirstChildElement("divisions");
			if (divisions != NULL)
			{
				ConvertFromString<int> s2i(divisions->GetText());
				int tmp = s2i.result();
				if (!s2i.fail())
					actTempo.ticksPerBeat = tmp;
			}
		}

		if (actTempo.ticksPerBeat == -1)
		{
			if (lastTempo.ticksPerBeat == -1)
			{
				actTempo.ticksPerBeat = 480; // default
				LOG_WARN_N("concat.musicxmlparser", "Warning: No <divisions> element found, using default (480).");
			}
			else
			{
				actTempo.ticksPerBeat = lastTempo.ticksPerBeat;
			}
		}

		// Get tempo:
		actTempo.BPM = -1.0; // invalid

		TiXmlElement *direction = NULL;
		for (direction = measure->FirstChildElement("direction"); direction != NULL; direction = direction->NextSiblingElement("direction"))
		{
			TiXmlElement *sound = direction->FirstChildElement("sound");

			if (sound != NULL)
			{
				sound->QueryDoubleAttribute("tempo", &actTempo.BPM);
			}
		}

		if (actTempo.BPM == -1.0)
		{
			if (lastTempo.BPM == -1.0)
			{
				actTempo.BPM = 120.0; // default
				LOG_WARN_N("concat.musicxmlparser", "Warning: No <sound> element found, using default tempo (120).");
			}
			else
			{
				actTempo.BPM = lastTempo.BPM;
			}
		}
	}

	// -----------------------------------------------------------------------------------

	void MusicXmlParser::updateTimeSig(TiXmlElement *measure, concat::TimeSignature &actTimeSig, const concat::TimeSignature &lastTimeSig)
	{
		actTimeSig = concat::TimeSignature(); // invalid

		// Try to read from XML:
		TiXmlElement *attributes = measure->FirstChildElement("attributes");
		if (attributes != NULL)
		{ 
			TiXmlElement *time = attributes->FirstChildElement("time");
			if (time != NULL)
			{
				TiXmlElement *beats = time->FirstChildElement("beats");
				TiXmlElement *beatType = time->FirstChildElement("beat-type");

				if (beats != NULL && beatType != NULL)
				{
					ConvertFromString<int> s2i1(beats->GetText());
					int tmp1 = s2i1.result();

					ConvertFromString<int> s2i2(beatType->GetText());
					int tmp2 = s2i2.result();

					if (!s2i1.fail() && !s2i2.fail())
					{
						actTimeSig = concat::TimeSignature(tmp1, tmp2);
					}
				}
			}
		}

		// Error recovery:
		if (!actTimeSig)
		{
			if (!lastTimeSig)
			{
				actTimeSig = concat::TimeSignature(4, 4); // default
				LOG_WARN_N("concat.musicxmlparser", "Warning: No or invalid <time> element, using default (4/4).");
			}
			else
			{
				actTimeSig = lastTimeSig;
			}
		}
	}

	// -----------------------------------------------------------------------------------

	void MusicXmlParser::updateKeySig(TiXmlElement *measure, concat::KeySignature &actKeySig, const concat::KeySignature &lastKeySig)
	{
		actKeySig = concat::KeySignature(); // invalid

		// Try to read from XML:
		TiXmlElement *attributes = measure->FirstChildElement("attributes");
		if (attributes != NULL)
		{ 
			TiXmlElement *key = attributes->FirstChildElement("key");
			if (key != NULL)
			{
				TiXmlElement *keyMode = key->FirstChildElement("mode");
				TiXmlElement *keyFifths = key->FirstChildElement("fifths");

				if (keyMode != NULL && keyFifths != NULL)
				{
					ConvertFromString<int> s2i(keyFifths->GetText());
					int tmp1 = s2i.result();

					int tmp2 = -1;
					if (std::string(keyMode->GetText()) == "minor")
						tmp2 = concat::KeySignature::MINOR;
					else if (std::string(keyMode->GetText()) == "major")
						tmp2 = concat::KeySignature::MAJOR;

					if (!s2i.fail() && tmp2 != -1)
					{
						actKeySig = concat::KeySignature((concat::KeySignature::KeyMode)(tmp2), tmp1);
					}
				}
			}
		}

		// Error recovery:
		if (!actKeySig)
		{
			if (!lastKeySig)
			{
				actKeySig = concat::KeySignature(concat::KeySignature::C_MAJOR); // default
				LOG_WARN_N("concat.musicxmlparser", "Warning: No or invalid <key> element using default (C-major).");
			}
			else
			{
				actKeySig = lastKeySig;
			}
		}
	}

	// -----------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------

	bool hasHarmonicArtType(TiXmlElement *note)
	{
		if (note)
		{
			TiXmlElement *nextNote = note->NextSiblingElement("note");
			if (nextNote)
			{
				if (nextNote->FirstChildElement("chord") != NULL)
				{
					TiXmlElement *notehead = nextNote->FirstChildElement("notehead");
					if (notehead != NULL)
					{
						std::string noteheadText = notehead->GetText();
						if (noteheadText == "diamond")
							return true;
					}
				}
			}
		}
		
		return false;
	}

	bool hasNaturalHarmonicArtType(TiXmlElement *note)
	{
		if (note)
		{
			TiXmlElement *notations = note->FirstChildElement("notations");

			if (notations)
			{
				TiXmlElement *technical = notations->FirstChildElement("technical");
				if (technical)
				{
					TiXmlElement *harmonic = technical->FirstChildElement("harmonic");
					if (harmonic)
						return true;
				}
			}
		}

		return false;
	}

	bool hasStaccatoOrSaltatoArtType(TiXmlElement *note)
	{
		if (note)
		{
			TiXmlElement *notations = note->FirstChildElement("notations");

			if (notations)
			{
				TiXmlElement *articulations = notations->FirstChildElement("articulations");
				if (articulations)
				{
					TiXmlElement *staccato = articulations->FirstChildElement("staccato");
					if (staccato)
						return true;
				}
			}
		}

		return false;
	}

	bool hasSpiccatoArtType(TiXmlElement *note)
	{
		if (note)
		{
			TiXmlElement *notations = note->FirstChildElement("notations");

			if (notations)
			{
				TiXmlElement *articulations = notations->FirstChildElement("articulations");
				if (articulations)
				{
					TiXmlElement *staccatissimo = articulations->FirstChildElement("staccatissimo");
					if (staccatissimo)
						return true;
				}
			}
		}

		return false;
	}

	bool hasMordentArtType(TiXmlElement *note)
	{
		if (note)
		{
			TiXmlElement *notations = note->FirstChildElement("notations");

			if (notations)
			{
				TiXmlElement *ornaments = notations->FirstChildElement("ornaments");
				if (ornaments)
				{
					TiXmlElement *invertedMordent = ornaments->FirstChildElement("inverted-mordent");
					if (invertedMordent)
						return true;
				}
			}
		}

		return false;
	}

	bool hasTrillArtType(TiXmlElement *note)
	{
		if (note)
		{
			TiXmlElement *notations = note->FirstChildElement("notations");

			if (notations)
			{
				TiXmlElement *ornaments = notations->FirstChildElement("ornaments");
				if (ornaments)
				{
					TiXmlElement *trillMark = ornaments->FirstChildElement("trill-mark");
					if (trillMark)
						return true;
				}
			}
		}

		return false;
	}

	void MusicXmlParser::getNoteArticulationType(TiXmlElement *note, ScoreNote& actNote, concat::MusicXmlParser::ArticulationTypeModifier artTypeMod)
	{	
		// Find articulation type:
		// Articulation types are mutually exclusive, in case there are multiple articulation 
		// specifications, the following precedence is used:
		// - Harmonic
		// - Pizzicato
		// - Natural harmonic
		// - Staccato/saltato (notations->articulations->staccato)
		// - Spiccato (notations->articulations->staccatissimo)
		// - Mordent
		// - Trill
		// - Legato/Detache (no notations)

		// Default articulation type (may be overwritten later).
		// LEGATO here means LEGATO or DETACHE depending on bow length (corrected later):
		actNote.setArticulationType(ArticulationType::LEGATO);

		if (hasHarmonicArtType(note))
		{
			actNote.setArticulationType(ArticulationType::HARMONIC);
			return;
		}

		if (artTypeMod == concat::MusicXmlParser::PIZZ)
		{
			actNote.setArticulationType(ArticulationType::PIZZICATO);
			return;
		}

		if (hasNaturalHarmonicArtType(note))
		{
			actNote.setArticulationType(ArticulationType::NATURAL_HARMONIC);
			return;
		}

		if (hasStaccatoOrSaltatoArtType(note))
		{
			bool isFastNote = (actNote.getDurationBeats() <= 1.0/16.0 * 4.0); // fast notes 1/32-1/16 are saltato, slower notes are staccato
			bool isNextNoteRest = false;
			TiXmlElement *nextNote = getNextNoteElement(note);
			if (nextNote != NULL)
				isNextNoteRest = (nextNote->FirstChildElement("rest") != NULL);

			if (isFastNote && !isNextNoteRest)
				actNote.setArticulationType(ArticulationType::SALTATO);
			else
				actNote.setArticulationType(ArticulationType::STACCATO);
			return;
		}

		if (hasSpiccatoArtType(note))
		{
			actNote.setArticulationType(ArticulationType::SPICCATO);
			return;
		}

		if (hasMordentArtType(note))
		{
			actNote.setArticulationType(ArticulationType::MORDENT);
			return;
		}

		if (hasTrillArtType(note))
		{
			actNote.setArticulationType(ArticulationType::TRILL);
			return;
		}
	}

	// -----------------------------------------------------------------------------------

	void MusicXmlParser::getNotePitchOrRest(TiXmlElement *note, ScoreNote& actNote)
	{
		// Get note pitch string:
		TiXmlElement *pitch = note->FirstChildElement("pitch");
		TiXmlElement *rest = note->FirstChildElement("rest");
		if (pitch != NULL)
		{
			TiXmlElement *step = pitch->FirstChildElement("step");
			TiXmlElement *octave = pitch->FirstChildElement("octave");
			TiXmlElement *alter = pitch->FirstChildElement("alter");

			std::string noAlter = "0";
			
			if (step != NULL && octave != NULL)
			{
				if (alter != NULL)
					actNote.setPitchStr(convertLetterOctaveAndAlterToPitchString(step->GetText(), octave->GetText(), alter->GetText()).c_str());
				else
					actNote.setPitchStr(convertLetterOctaveAndAlterToPitchString(step->GetText(), octave->GetText(), noAlter.c_str()).c_str());

				actNote.setIsRest(false);
			}
		}
		else if (rest != NULL)
		{
			actNote.setIsRest(true);
		}
		else
		{
			LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Note %d (measure %d) is neither rest nor pitched note. Considering as rest.", 1+actNote.getNoteIdxRelativeToBeginMeasure(), 1+actNote.getMeasureIdx()));
			actNote.setIsRest(true);
		}
	}

	// -----------------------------------------------------------------------------------

	void MusicXmlParser::getNoteAccent(TiXmlElement *notations, ScoreNote& actNote)
	{
		actNote.setAccent(Accent::NO_ACCENT); // default

		// Get note accents:
		if (notations != NULL)
		{
			TiXmlElement *articulations = notations->FirstChildElement("articulations");
			if (articulations != NULL)
			{  
				TiXmlElement *accent = articulations->FirstChildElement("accent");
				TiXmlElement *tenuto = articulations->FirstChildElement("tenuto");

				if (accent != NULL)
				{
					actNote.setAccent(Accent::ACCENT);
				}
				else if (tenuto != NULL)
				{
					actNote.setAccent(Accent::TENUTO);
				}
			}
		}
	}

	// -----------------------------------------------------------------------------------

	void MusicXmlParser::getNoteFinger(TiXmlElement *notations, ScoreNote& actNote)
	{
		if (notations != NULL)
		{
			TiXmlElement *technical = notations->FirstChildElement("technical");
			//Technical		   
			if (technical != NULL)
			{   
				TiXmlElement *fingering = technical->FirstChildElement("fingering");
				if (fingering != NULL)
				{
					const char *fingeringStr = fingering->GetText();
					actNote.setFingeringAsString(fingeringStr);
				} 
			}
		}
	}

	// -----------------------------------------------------------------------------------

	void MusicXmlParser::getCurHandPosition(TiXmlElement *direction)
	{
		if (direction != NULL)
		{
			TiXmlElement *directionType = direction->FirstChildElement("direction-type");
			if (directionType != NULL)  
			{ 
				TiXmlElement *words = directionType->FirstChildElement("words");
				if (words != NULL)
				{
					int handPos = convertHandPositionStringToBase1Index(words->GetText());

					if (handPos > 0)
						curHandPos_ = handPos;
				}
			}
		}
	}
	// -----------------------------------------------------------------------------------

	//bool MusicXmlParser::setStringFingerHandposValid(ScoreNote& actNote, const ScoreNote& lastNote)
	//{
	//	//fsp=(f)inger,(s)tring,(p)osition: try 8 combinations:
	//	if (actNote.isFingeringValid())
	//	{
	//		if (actNote.isPlayedStringValid())
	//		{
	//			int handPos=actNote.obtainHandPos(actNote.getFingering(), actNote.getPlayedString(), actNote.getPitchNoAlter());
	//			if (actNote.isHandPosValid())
	//			{ //fsp=111
	//				if (handPos==actNote.getHandPos())
	//					return true;
	//				else
	//					return false;
	//			}
	//			//fsp=110
	//			actNote.setHandPos(handPos);
	//			return true;
	//		}
	//		else if (actNote.isHandPosValid())
	//		{ //fsp=101
	//			const char *string=actNote.obtainPlayedString(actNote.getFingering(), actNote.getHandPos(), actNote.getPitchNoAlter());
	//			if (strcmp(string,"G") || strcmp(string,"D") || strcmp(string,"A") || strcmp(string,"E"))
	//				return true;
	//			else
	//				return false;
	//		}
	//		else
	//		{//fsp=100: check if only finger is enough
	//			if (actNote.setHandposAndString(actNote, lastNote, actNote.getPitchNoAlter()))
	//				return true;
	//			else
	//			{
	//				//try with lastNote handPosition
	//				actNote.setHandPos(lastNote.getHandPos());
	//				const char *string=actNote.obtainPlayedString(actNote.getFingering(), actNote.getHandPos(), actNote.getPitchNoAlter());
	//				if (strcmp(string,"G") || strcmp(string,"D") || strcmp(string,"A") || strcmp(string,"E"))
	//					return true;
	//				else
	//					return false;			
	//			}
	//		}
	//	}
	//	else if (actNote.isPlayedStringValid())
	//	{
	//		if (actNote.isHandPosValid())
	//		{//fsp=011
	//			int finger = actNote.obtainFingering(actNote.getPlayedString(), actNote.getHandPos(), actNote.getPitchNoAlter());
	//			if (finger>0)
	//				return true;
	//			else
	//				return false;
	//		}
	//		else
	//		{//fsp=010
	//			//string is not enough, get handPosition from lastNote
	//			//if(lastNote.getHandPos()!=-1)
	//			actNote.setHandPos(lastNote.getHandPos());
	//			int finger = actNote.obtainFingering(actNote.getPlayedString(), actNote.getHandPos(),actNote.getPitchNoAlter());
	//			if (finger>0)
	//				return true;
	//			else
	//				return false;
	//		}
	//	}
	//	else if (actNote.isHandPosValid())
	//	{//fsp=001
	//		if (actNote.setFingerAndString(actNote,actNote.getPitchNoAlter()))
	//			return true;
	//		else
	//			return false;
	//	}
	//	else 
	//	{
	//		//try with lastNote handPos
	//		actNote.setHandPos(lastNote.getHandPos());
	//		if(actNote.setFingerAndString(actNote,actNote.getPitchNoAlter()))
	//		{
	//			return true; //is_setHandPosEnough
	//		}
	//		else if (strcmp(actNote.getPitchStr(),lastNote.getPitchStr())==0)
	//		{
	//			actNote.setFingering(lastNote.getFingering());
	//			char* string=actNote.obtainPlayedString(actNote.getFingering(), actNote.getHandPos(), actNote.getPitchNoAlter());
	//			if (strcmp(string,"G") || strcmp(string,"D") || strcmp(string,"A") || strcmp(string,"E"))
	//				return true;
	//			else
	//				return false;
	//		}
	//		else
	//		{
	//			return false;
	//		}
	//	}
	//}

	// -----------------------------------------------------------------------------------

	void MusicXmlParser::getCurPlayedString(TiXmlElement *direction)
	{
		if (direction != NULL)
		{
			TiXmlElement *directionType = direction->FirstChildElement("direction-type");
			
			if (directionType != NULL)  
			{ 
				TiXmlElement *words = directionType->FirstChildElement("words");

				if (words != NULL)
				{
					std::string playedString = words->GetText();

					if (playedString == "G" ||
						playedString == "D" || 
						playedString == "A" ||
						playedString == "E")
					{
						curPlayedString_ = convertPlayedStringNameToBase1Index(playedString.c_str());
					}
				}
			}
		}

		// NOTE:
		// If this <direction> or other <direction>s do not cause the played string to be 
		// set, the played string of the last note will be used (somewhere else in code).
		// Except for the case where there is no last note, in that case an error will be 
		// produced.
	}

	// -----------------------------------------------------------------------------------

	TiXmlElement *MusicXmlParser::getNextNoteElement(TiXmlElement *noteElement)
	{
		if (noteElement == NULL)
			return NULL;

		if (noteElement->Parent() == NULL)
			return NULL;

		TiXmlElement *curMeasure = noteElement->Parent()->ToElement();
		TiXmlElement *nextNoteElement = noteElement->NextSiblingElement("note");

		if (nextNoteElement == NULL)
		{
			// try next measure(s):
			for (TiXmlElement *measure = curMeasure->NextSiblingElement("measure"); measure != NULL; measure = measure->NextSiblingElement("measure"))
			{
				nextNoteElement = measure->FirstChildElement("note");
				if (nextNoteElement != NULL)
					break;
			}
		}

		return nextNoteElement; // NULL if none found
	}

	void MusicXmlParser::applyBowDirectionChange(const TiXmlElement *down, const TiXmlElement *up, ScoreNote &actNote)
	{
		if (down != NULL)
		{
			actNote.setBowDirection(BowDirection::DOWN);
		}
		else if (up != NULL)
		{
			actNote.setBowDirection(BowDirection::UP);
		}
	}

	void MusicXmlParser::invertLastBowDirection(ScoreNote &actNote, const ScoreNote &lastNote)
	{
		if (actNote.getNoteIdx() == 0)
		{
			LOG_WARN_N("concat.musicxmlparser", "Warning: First note in score does not have bow direction explicitly specified. Defaulting to DOWN, but might not match performer's playing.");

			// No last note, use DOWN bow (default):
			actNote.setBowDirection(BowDirection::DOWN);
		}
		else if (lastNote.isRest() || lastNote.isPizzicato())
		{
			// XXX: for right-hand pizzicato should probably always use DOWN

			// Short rest (<= 2 beats):
			if (prevRestPizzSeqAccumDurBeats_ <= 2.0)
			{
				// Last note was short rest, use inverse of last non-rest note bow direction:
				if (lastNonRestNonPizzBowDirIdx_ == 0)
				{
					actNote.setBowDirection(BowDirection(1));
				}
				else if (lastNonRestNonPizzBowDirIdx_ == 1)
				{
					actNote.setBowDirection(BowDirection(0));
				}
				else
				{
					LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Unable to invert bow direction for note %d (measure %d) after short rest (using DOWN). Might be first non-rest note in score (otherwise bug in code!).", 1+actNote.getNoteIdxRelativeToBeginMeasure(), 1+actNote.getMeasureIdx()));
					actNote.setBowDirection(BowDirection::DOWN);
					// XXX: if had access to all past notes in target, could look back if only contains rest notes
				}
			}
			// Long rest (> 2 beats):
			else
			{
				LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Note %d (measure %d) follows long rest, but doesn't have bow direction specified (using DOWN).", 1+actNote.getNoteIdxRelativeToBeginMeasure(), 1+actNote.getMeasureIdx()));

				// Last note was long rest, use DOWN bow (default, although not very strict rule):
				actNote.setBowDirection(BowDirection::DOWN);
			}
		}
		else if (!lastNote.getBowDirection())
		{
			LOG_ERROR_N("concat.musicxmlparser", formatStr("Error: Note %d (measure %d) does not have valid bow direction field, but is non-rest. Next note's bow direction will default to DOWN. BUG IN CODE!", 1+lastNote.getNoteIdxRelativeToBeginMeasure(), 1+lastNote.getMeasureIdx()));
			actNote.setBowDirection(BowDirection::DOWN);
		}
		else
		{
			// Use inverse of last bow direction:
			if (lastNote.getBowDirection() == BowDirection::DOWN)
				actNote.setBowDirection(BowDirection::UP);
			else
				actNote.setBowDirection(BowDirection::DOWN);
		}
	}

	void MusicXmlParser::continueLastBowDirection(ScoreNote &actNote, const ScoreNote &lastNote)
	{
		actNote.setBowDirection(lastNote.getBowDirection());
	}

	bool MusicXmlParser::isNextNoteTieAsSlurEnd(TiXmlElement *curMeasureElement, TiXmlElement *noteElement, ScoreNote &actNote, int tieAsSlurLevel)
	{
		TiXmlElement *nextNoteElement = getNextNoteElement(noteElement);

		if (nextNoteElement == NULL)
			return false;

		TiXmlElement *notations = nextNoteElement->FirstChildElement("notations");

		if (notations == NULL)
			return false;

		TiXmlElement *slur;

		for (slur = notations->FirstChildElement("slur"); slur != NULL; slur = slur->NextSiblingElement("slur"))
		{
			std::string slurType = slur->Attribute("type"); // if type attribute doesn't exist slurType will be ""

			int slurNumber = -1; // invalid or not specified (slur numbers start at 1)
			slur->QueryIntAttribute("number", &slurNumber);
			if (slurNumber == -1)
				slurNumber = 1;

			if (slurNumber == 3)
				slurNumber = 1;
			// assume this only happens when there's a >= 3 note tie as slur whose first note is also the 
			// first note of a longer slur, first tie might be slur 1, longer slur might be slur 2 and 
			// second tie might be slur 3

			// If next note is slur stop, with same slur number as preceding slur start 
			// and, additionally, has the same pitch as the last note, then it is a tie as 
			// slur stop:
			if (slurType == "stop" && slurNumber-1 == tieAsSlurLevel)
			{
				ScoreNote tmp;
				getNotePitchOrRest(nextNoteElement, tmp);
				if (tmp.getMidiNote() == actNote.getMidiNote())
					return true;
			}
		}

		return false;
	}

	void MusicXmlParser::getNoteBow(TiXmlElement *curMeasureElement, TiXmlElement *noteElement, ScoreNote &actNote, ScoreNote &lastNote)
	{
		if (noteElement == NULL)
			return; // shouldn't happen normally

		// Tie/slur:
		// -------------------------------------------------------------------------------

		// If last note was tied (but not tie stop), continue tie:
		if (lastNote.isTied() && !lastNote.isTieStop())
		{
			// Just make sure all notes in tie have same pitch (requirement for tie), 
			// if this is not the case the MusicXML is probably malformed and the tie stop 
			// is missing. Work-around this issue:
			if (actNote.getMidiNote() != lastNote.getMidiNote())
			{
				LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Missing tie stop or incorrect tie! Notes %d (measure %d) and %d (measure %d) are tied by continuation, but have different pitches. Perhaps slur specified as tie with missing tie stop.", 1+lastNote.getNoteIdxRelativeToBeginMeasure(), 1+lastNote.getMeasureIdx(), 1+actNote.getNoteIdxRelativeToBeginMeasure(), 1+actNote.getMeasureIdx()));
				actNote.setTied(false);
				if (lastNote.isTieStart())
				{
					LOG_WARN_N("concat.musicxmlparser", "         Correcting by removing entire tie.");
					lastNote.setTied(false); // (also removes isTieStart flag)
				}
				else
				{
					LOG_WARN_N("concat.musicxmlparser", "         Correcting by adding missing tie stop.");
					lastNote.setTieStop();
				}
			}
			// Default case for well-formed MusicXMLs:
			else
			{
				actNote.setTied(true);
			}
		}

		// If actNote is rest stop here, only do above to correct lastNote in case of 
		// missing tie stop (mal-formed MusicXML):
		if (actNote.isRest() || actNote.isPizzicato())
			return;

		// If last note was slurred (but not slur stop), continue slur:
		if (lastNote.isSlurred() && !lastNote.isSlurStop())
			actNote.setSlurred(true);

		// Get notations child element:
		TiXmlElement *notations = noteElement->FirstChildElement("notations");

		// Handle tie/slur notations that affect current note:
		if (notations != NULL)
		{
			// Handle slur:                                      
			TiXmlElement *slur;

			for (slur = notations->FirstChildElement("slur"); slur != NULL; slur = slur->NextSiblingElement("slur"))
			{
				std::string slurType = slur->Attribute("type"); // if type attribute doesn't exist slurType will be ""
				
				int slurNumber = -1; // invalid or not specified (slur numbers start at 1)
				slur->QueryIntAttribute("number", &slurNumber);
				if (slurNumber == -1)
					slurNumber = 1;

				if (slurNumber == 3)
					slurNumber = 1;
				// assume this only happens when there's a >= 3 note tie as slur whose first note is also the 
				// first note of a longer slur, first tie might be slur 1, longer slur might be slur 2 and 
				// second tie might be slur 3

				if (slurNumber == 1 || slurNumber == 2) // only first and second slur is supported (and slurs with no specified number)
				{
					if (slurType == "start")
					{
						// Check if tie as slur start:
						if (isNextNoteTieAsSlurEnd(curMeasureElement, noteElement, actNote, slurNumber-1))
						{
							actNote.setTieAsSlurStart(true, slurNumber-1);
						}
						// Normal slur start:
						else
						{
							actNote.setSlurred(true); // (only need to set slurred on first note, other notes are propagated)
							actNote.setSlurStart();
						}
					}
					else if (slurType == "stop")
					{
						// Check if tie as slur stop:
						if (lastNote.isTieAsSlurStart(slurNumber-1))
						{
							 actNote.setTieAsSlurStop(true);
						}
						// Normal slur stop:
						else
						{
							if (actNote.isSlurred()) // in case there are slur stops that do not correspond to any slur start
								actNote.setSlurStop();
						}
					}
				}
				else
				{
					LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Note %d (measure %d) has more than two multiple simultaneous slurs, only first two (slur + tie as slur) are handled.", 1+actNote.getNoteIdxRelativeToBeginMeasure(), 1+actNote.getMeasureIdx()));
				}
			}

			// NOTE:
			// It might happen that a note is both tie as slur stop and tie as slur start 
			// to form > 2 note ties (using slurs). These should be handled when converting 
			// tie as slurs to real ties in a separate step later.

			// Handle tie:
			bool hasTiedStart = false;
			bool hasTiedStop = false;
			TiXmlElement *tied;
			for (tied = notations->FirstChildElement("tied"); tied != NULL; tied = tied->NextSiblingElement("tied"))
			{
				std::string tiedType = tied->Attribute("type"); // if type attribute doesn't exist tiedType will be ""

				if (tiedType == "start")
				{
					if (hasTiedStart)
						LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Note %d (measure %d) has multiple tie start specifications.", 1+actNote.getNoteIdxRelativeToBeginMeasure(), 1+actNote.getMeasureIdx()));

					hasTiedStart = true;
				}
				else if (tiedType == "stop")
				{
					if (hasTiedStop)
						LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Note %d (measure %d) has multiple tie stop specifications.", 1+actNote.getNoteIdxRelativeToBeginMeasure(), 1+actNote.getMeasureIdx()));

					hasTiedStop = true;
				}
			}

			if (hasTiedStop && hasTiedStart)
			{
				// Tie stop and tie start, means multiple note tie, tie continue for  
				// notes in between start and stop:
				actNote.setTied(true);
			}
			else if (hasTiedStart)
			{
				// Work-around for within in a tie (of same pitch) having multiple tie starts:
				if (lastNote.isTied() && actNote.getMidiNote() == lastNote.getMidiNote())
				{
					; // do nothing (tie already continued above)
				}
				// Default case for well-formed MusicXMLs:
				else
				{
					actNote.setTied(true); // (only need to set tied on first note, other notes are propagated)
					actNote.setTieStart();
				}
			}
			else if (hasTiedStop)
			{
				if (actNote.isTied()) // in case there are too many tie stops
					actNote.setTieStop();
			}
		}

		// Bow direction (must come AFTER tie/slur handling):
		// -------------------------------------------------------------------------------

		TiXmlElement *down = NULL;
		TiXmlElement *up = NULL;

		if (notations != NULL)
		{		
			// Handle bow direction:
			TiXmlElement *technical = notations->FirstChildElement("technical");

			if (technical != NULL)
			{   
				down = technical->FirstChildElement("down-bow");
				up = technical->FirstChildElement("up-bow");
			}
		}

		bool hasBowDirectionChange = (down != NULL || up != NULL);

		bool isTiedAsSlurStart = (actNote.isTieAsSlurStart(0) || actNote.isTieAsSlurStart(1));
		bool isTiedAsSlur = (isTiedAsSlurStart || actNote.isTieAsSlurStop());

		// If slurred, continue bow direction:
		if (actNote.isSlurred())
		{
			if (hasBowDirectionChange)
			{
				// Only allow bow direction to be specified for first note in slur:
				if (actNote.isSlurStart())
				{
					applyBowDirectionChange(down, up, actNote);
				}
				else
				{
					LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Bow direction change within slur (note %d, measure %d), ignoring bow direction change.", 1+actNote.getNoteIdxRelativeToBeginMeasure(), 1+actNote.getMeasureIdx()));
					continueLastBowDirection(actNote, lastNote);
				}
			}
			else
			{
				if (actNote.isSlurStart())
				{
					invertLastBowDirection(actNote, lastNote);
				}
				else
				{
					continueLastBowDirection(actNote, lastNote);
				}
			}
		}
		// Else, if tied, continue bow direction:
		else if (actNote.isTied() || isTiedAsSlur)
		{
			if (hasBowDirectionChange)
			{
				// Only allow bow direction to be specified for first note in tie:
				if (actNote.isTieStart() || isTiedAsSlurStart)
				{
					applyBowDirectionChange(down, up, actNote);
				}
				else
				{
					LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Bow direction change within tie (note %d, measure %d), ignoring bow direction change.", 1+actNote.getNoteIdxRelativeToBeginMeasure(), 1+actNote.getMeasureIdx()));
					continueLastBowDirection(actNote, lastNote);
				}
			}
			else
			{
				if (actNote.isTieStart() || isTiedAsSlurStart)
				{
					invertLastBowDirection(actNote, lastNote);
				}
				else
				{
					continueLastBowDirection(actNote, lastNote);
				}
			}
		}
		// Otherwise (not slurred/tied), apply bow direction change or invert bow 
		// direction if there's no change specified:
		else
		{
			if (hasBowDirectionChange)
			{
				applyBowDirectionChange(down, up, actNote);
			}
			else
			{
				invertLastBowDirection(actNote, lastNote);
			}
		}
	}

	// -----------------------------------------------------------------------------------

	void MusicXmlParser::getNoteDuration(TiXmlElement *note, const ScoreTempo &tempo, ScoreNote &actNote)
	{
		actNote.setBeginSeconds(tempo.posSeconds);
		actNote.setBeginBeats(tempo.posBeats);
		TiXmlElement *duration = note->FirstChildElement("duration");
		if (duration == NULL)
		{
			LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Duration-less note (note %d, measure %d), using zero.", 1+actNote.getNoteIdxRelativeToBeginMeasure(), 1+actNote.getMeasureIdx()));
			actNote.setDurationBeats(0.0);
			actNote.setDurationSeconds(0.0);
			return;
		}

		// Compute duration from <duration> element:
		// NOTE: The <duration> element does correspond to <type> and optional <dot> and tuplet (<time-modification>, etc.) elements.
		ConvertFromString<float> s2f(duration->GetText());
		float durationTicks = s2f.result();

		if (s2f.fail())
		{
			LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Failure parsing note duration (note %d, measure %d), using zero.", 1+actNote.getNoteIdxRelativeToBeginMeasure(), 1+actNote.getMeasureIdx()));
			actNote.setDurationBeats(0.0);
			actNote.setDurationSeconds(0.0);
			return;
		}

		// XXX: It would be good to check if <duration> element (in ticks) corresponds to <type> element (i.e. whole, 16th, etc.), 
		// but difficult because should also take into account dots and tuplet time modifications

		double durationBeats = (double)durationTicks/(double)tempo.ticksPerBeat; // durationQuarterNotes = ticks/ticksPerQuarterNote
		double durationSeconds = 60.0*durationBeats/tempo.BPM; // durationSeconds = 60 * durationQuarterNotes / quarterNotesPerMinute

		actNote.setDurationBeats(durationBeats);
		actNote.setDurationSeconds(durationSeconds);	
	}

	// -----------------------------------------------------------------------------------

	void MusicXmlParser::getCurDynamic(TiXmlElement *direction)
	{
		if (direction != NULL)
		{
			TiXmlElement *directiontype = direction->FirstChildElement("direction-type");
			if (directiontype != NULL)  
			{ 
				TiXmlElement *dynamics = directiontype->FirstChildElement("dynamics"); 
				if (dynamics != NULL)
				{			
					TiXmlElement *pp = dynamics->FirstChildElement("pp");
					TiXmlElement *p = dynamics->FirstChildElement("p");
					TiXmlElement *mf = dynamics->FirstChildElement("mf");
					TiXmlElement *f = dynamics->FirstChildElement("f");
					TiXmlElement *ff = dynamics->FirstChildElement("ff");

					if (pp != NULL) 
					{
						curDynType_ = DynamicsType::NORMAL;
						curDynValue_ = DynamicsValue::PP;
						assignCurDynValToNextNote_ = true;
					}
					else if (p != NULL) 
					{
						curDynType_ = DynamicsType::NORMAL;
						curDynValue_ = DynamicsValue::P;
						assignCurDynValToNextNote_ = true;
					}
					else if (mf != NULL) 
					{
						curDynType_ = DynamicsType::NORMAL;
						curDynValue_ = DynamicsValue::MF;
						assignCurDynValToNextNote_ = true;
					}
					else if (f != NULL) 
					{
						curDynType_ = DynamicsType::NORMAL;
						curDynValue_ = DynamicsValue::F;
						assignCurDynValToNextNote_ = true;
					}
					else if (ff != NULL) 
					{
						curDynType_ = DynamicsType::NORMAL;
						curDynValue_ = DynamicsValue::FF;
						assignCurDynValToNextNote_ = true;
					}
					else
					{
						LOG_INFO_N("concat.musicxmlparser", "Warning: Invalid dynamics element found.");
						// (ignore)
					}
				}

				TiXmlElement *words = directiontype->FirstChildElement("words");
				if (words != NULL)
				{
					std::string wordsText = words->GetText();

					if (wordsText == "cresc.")
					{
						LOG_INFO_N("concat.musicxmlparser", "Warning: Direction with text \"cresc.\" found but not handled. Use <wedge> instead.");
					}
					else if (wordsText == "decresc.")
					{
						LOG_INFO_N("concat.musicxmlparser", "Warning: Direction with text \"decresc.\" found but not handled. Use <wedge> instead.");
					}
				}

				TiXmlElement *wedge = directiontype->FirstChildElement("wedge");
				if (wedge != NULL)
				{
					std::string wedgeType = wedge->Attribute("type"); // in case attribute type doesn't exist wedgeType will be ""

					if (wedgeType == "crescendo")
					{
						curDynType_ = DynamicsType::CRESC;
						crescDecrescStop_ = false; // just in case previous wedge stop wasn't handled (too many wedge stops, etc.)
					}
					else if (wedgeType == "diminuendo")
					{
						curDynType_ = DynamicsType::DECRESC;
						crescDecrescStop_ = false; // just in case previous wedge stop wasn't handled (too many wedge stops, etc.)
					}
					else if (wedgeType == "stop")
					{
						crescDecrescStop_ = true;
					}
					else
					{
						LOG_INFO_N("concat.musicxmlparser", "Warning: Invalid wedge type found.");
						// (ignore)
					}
				}
			}
		}

		// NOTE:
		// In case dynamics aren't specified for note, dynamics of last note are used (in a different part of the code).
		// Except of course in the case when there is no last note, then a default dynamics value (MF) will be used.
	}

	// -----------------------------------------------------------------------------------

	void MusicXmlParser::getCurArtTypeMod(TiXmlElement *direction)
	{
		if (direction != NULL)
		{
			TiXmlElement *directionType = direction->FirstChildElement("direction-type");

			if (directionType != NULL)  
			{ 
				TiXmlElement *words = directionType->FirstChildElement("words");

				if (words != NULL)
				{
					std::string label = words->GetText();

					if (label == "pizz." || label == "Pizz." || label == "PIZZ." ||
						label == "pizz" || label == "Pizz" || label == "PIZZ" || 
						label == "pizzicato" || label == "Pizzicato" || label == "PIZZICATO")
					{
						artTypeMod_ = PIZZ;
					}
					else if (label == "arco" || label == "Arco" || label == "ARCO")
					{
						artTypeMod_ = ARCO;
					}
				}
			}
		}
	}

	// -----------------------------------------------------------------------------------

	// Logging function for debugging purposes only.
	void MusicXmlParser::logTiesSlurs(concat::InputScore &target)
	{
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			ScoreNote &cur = target.getNoteAt(noteIdx);
			std::string p = cur.getPitchStr();
			if (p.size() == 2)
				p += " ";
			if (p.size() == 0)
				p += "   ";
			LOG_INFO_N("concat.musicxmlparser", formatStr("%03d : n = %s, nn = %d, b = %.3f, e = %.3f, tie start = %d, tie stop = %d, tied = %d", 1+noteIdx, p.c_str(), cur.getMidiNote(), cur.getBeginSeconds(), cur.getEndSeconds(), (int)cur.isTieStart(), (int)cur.isTieStop(), (int)cur.isTied()));
		}
		LOG_INFO_N("concat.musicxmlparser", "-------");
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			ScoreNote &cur = target.getNoteAt(noteIdx);
			std::string p = cur.getPitchStr();
			if (p.size() == 2)
				p += " ";
			if (p.size() == 0)
				p += "   ";
			LOG_INFO_N("concat.musicxmlparser", formatStr("%03d : n = %s, nn = %d, b = %.3f, e = %.3f, slur start = %d, slur stop = %d, slurred = %d", 1+noteIdx, p.c_str(), cur.getMidiNote(), cur.getBeginSeconds(), cur.getEndSeconds(), (int)cur.isSlurStart(), (int)cur.isSlurStop(), (int)cur.isSlurred()));
		}
		LOG_INFO_N("concat.musicxmlparser", "-------");
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			ScoreNote &cur = target.getNoteAt(noteIdx);
			std::string p = cur.getPitchStr();
			if (p.size() == 2)
				p += " ";
			if (p.size() == 0)
				p += "   ";
//			LOG_INFO_N("concat.musicxmlparser", formatStr("%03d : n = %s, nn = %d, b = %.3f, e = %.3f, tied as slur start 1 = %d, tied as slur start 2 = %d, tied as slur stop = %d, tied as slur = %d", 1+noteIdx, p.c_str(), cur.getMidiNote(), cur.getBeginSeconds(), cur.getEndSeconds(), (int)cur.isTieAsSlurStart(0), (int)cur.isTieAsSlurStart(1), (int)cur.isTieAsSlurStop(), (int)cur.isTiedAsSlur()));
			LOG_INFO_N("concat.musicxmlparser", formatStr("%03d : n = %s, nn = %d, b = %.3f, e = %.3f, tied as slur start 1 = %d, tied as slur start 2 = %d, tied as slur stop = %d", 1+noteIdx, p.c_str(), cur.getMidiNote(), cur.getBeginSeconds(), cur.getEndSeconds(), (int)cur.isTieAsSlurStart(0), (int)cur.isTieAsSlurStart(1), (int)cur.isTieAsSlurStop()));
		}
	}

	// -----------------------------------------------------------------------------------

	void MusicXmlParser::convertTiesAsSlurToTies(concat::InputScore &target)
	{
		// Convert tie as slurs to real ties:
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			ScoreNote &note = target.getNoteAt(noteIdx);

			bool tieAsSlurStart = note.isTieAsSlurStart(0) || note.isTieAsSlurStart(1);
			bool tieAsSlurStop = note.isTieAsSlurStop();

			// Both stop and start, convert to tie continue:
			if (tieAsSlurStop && tieAsSlurStart)
			{
				note.setTieAsSlurStart(false, -1);
				note.setTieAsSlurStop(false);
				note.setTied(true);
				assert(!note.isTieStart());
				assert(!note.isTieStop());
			}
			// Start only, convert to tie start:
			else if (tieAsSlurStart)
			{
				note.setTieAsSlurStart(false, -1);
				note.setTied(true);
				note.setTieStart();
				assert(!note.isTieStop());
			}
			// Stop only, convert to tie stop:
			else if (tieAsSlurStop)
			{
				note.setTieAsSlurStop(false);
				note.setTied(true);
				note.setTieStop();
				assert(!note.isTieStart());
			}
		}

		// Sanity check:
		// See if all tie as slur fields have been removed (converted to real ties).
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			ScoreNote &note = target.getNoteAt(noteIdx);

			if (note.isTieAsSlurStart(0) || note.isTieAsSlurStart(1) || note.isTieAsSlurStop())
			{
				LOG_ERROR_N("concat.musicxmlparser", formatStr("Error: Sanity check; Note %d is tied. There should be no tie as slurs left after converting to real ties!", 1+noteIdx));
			}
		}
	}

	// -----------------------------------------------------------------------------------

	void MusicXmlParser::joinTies(concat::InputScore &target)
	{
		// XXX: test
//		LOG_INFO_N("concat.musicxmlparser", "BEFORE:");
//		logTiesSlurs(target);

		for (int noteIdx = target.getNumNotes()-1; noteIdx >= 0; --noteIdx) // iterate backwards for easier debugging
		{
			ScoreNote &cur = target.getNoteAt(noteIdx);
			if (cur.isTieStart())
			{
				int tieStartIdx = noteIdx;
				int tieStopIdx = -1;

				for (int noteIdxSearch = tieStartIdx+1; noteIdxSearch < target.getNumNotes(); ++noteIdxSearch)
				{
					ScoreNote &next = target.getNoteAt(noteIdxSearch);
					if (next.isTieStop())
					{
						tieStopIdx = noteIdxSearch;
						break;
					}
				}

				// Additional work-around for mal-formed MusicXMLs (missing tie stops), but this is already 
				// handled in getNoteBow():
				if (tieStopIdx == -1)
				{
					if (tieStartIdx+1 < target.getNumNotes() && target.getNoteAt(tieStartIdx+1).getMidiNote() == target.getNoteAt(tieStartIdx).getMidiNote())
					{
						tieStopIdx = tieStartIdx+1;
						target.getNoteAt(tieStopIdx).setTied(true);
						target.getNoteAt(tieStopIdx).setTieStop();
						LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: No matching tie stop for tie start (note %d), but following note has correct pitch. Defaulting to two note tie.", 1+tieStartIdx));
						LOG_ERROR_N("concat.musicxmlparser", "Error: Above warning should already be handled elsewhere. BUG IN CODE!");
					}
				}

				if (tieStopIdx != -1)
				{
					for (int noteIdxNext = tieStopIdx; noteIdxNext >= tieStartIdx+1; --noteIdxNext) // iterate backwards so that still to process indexes stay valid
					{
						ScoreNote &next = target.getNoteAt(noteIdxNext);

						// sanity check (normally already handled in getNoteBow()):
						if (cur.getMidiNote() != next.getMidiNote())
						{
							LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Notes %d and %d are tied, but have different pitches! Forcing to single pitch tie (second note is lost).", 1+noteIdx, 1+noteIdxNext));
							LOG_ERROR_N("concat.musicxmlparser", "Error: Above warning should already be handled elsewhere. BUG IN CODE!");
						}

						// enlarge first note:
						cur.setDurationBeats(cur.getDurationBeats() + next.getDurationBeats());
						cur.setDurationSeconds(cur.getDurationSeconds() + next.getDurationSeconds());

						// copy cresc./decresc. stop flag:
						if (next.isCrescDecrescStop())
							cur.setCrescDecrescStop();

						// (bow length is set below, so no need to correct it here yet)

						// erase second note:
						target.eraseNote(noteIdxNext, false); // do not update onset times (already done above)
					}

					// untie first (now joined) note:
					cur.setTied(false);
				}
				else
				{
					LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: No matching tie stop for tie start (note %d) and following note does not match tie start pitch! Ignoring tie.", 1+tieStartIdx));
					LOG_ERROR_N("concat.musicxmlparser", "Error: Above warning should already be handled elsewhere. BUG IN CODE!");
				}
			}
		}

		// XXX: test
//		LOG_INFO_N("concat.musicxmlparser", "AFTER:");
//		logTiesSlurs(target);

		// Sanity check:
		// After joining ties, there should be no tied notes left.
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			const ScoreNote &note = target.getNoteAt(noteIdx);

			if (note.isTied() || note.isTieStart() || note.isTieStop())
				LOG_ERROR_N("concat.musicxmlparser", formatStr("Error: Sanity check; note %d is in tie but there should have been no ties left after joining tied notes!", 1+noteIdx));
		}
	}

	// -----------------------------------------------------------------------------------

	void MusicXmlParser::convertSingleNoteLegatosToDetache(concat::InputScore &target)
	{
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			ScoreNote &note = target.getNoteAt(noteIdx);

			if (note.isLegato() && note.getBowLength() == 1)
				note.setArticulationType(ArticulationType::DETACHE);
		}
	}  

	// -----------------------------------------------------------------------------------

	void MusicXmlParser::computeBowsFromSlurs(concat::InputScore &target) 
	{
		//ScoreNote &xxx = target.getNoteAt(1);
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			ScoreNote &note = target.getNoteAt(noteIdx);

			if (!note.isSlurred())
			{
				note.setBowNoteIndex(0);
				note.setBowLength(1); 
			}  
			else if (note.isSlurStart())
			{
				// Find slur stop:
				int slurStartIdx = noteIdx;
				int slurStopIdx = -1;

				for (int noteIdxSearch = slurStartIdx+1; noteIdxSearch < target.getNumNotes(); ++noteIdxSearch)
				{
					ScoreNote &noteSearch = target.getNoteAt(noteIdxSearch);

					if (!noteSearch.isSlurred())
					{
						LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Note %d after slur start (note %d) and before slur stop does not have slurred flag set! Ignoring slur.", 1+noteIdxSearch, 1+slurStartIdx));
						break;
					}

					if (noteSearch.isSlurStop())
					{
						slurStopIdx = noteIdxSearch;
						break;
					}
				}

				//  If slur stop was found:
				if (slurStopIdx != -1)
				{
					const int bowLength = slurStopIdx - slurStartIdx + 1;
					for (int slurNoteIdx = slurStartIdx; slurNoteIdx <= slurStopIdx; ++slurNoteIdx)
					{
						ScoreNote &noteSlur = target.getNoteAt(slurNoteIdx);

						noteSlur.setBowNoteIndex(slurNoteIdx-slurStartIdx);
						noteSlur.setBowLength(bowLength);
					}

					// Update iterator:
					noteIdx = slurStopIdx; // will be incremented on continue
				}
				else
				{
					LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: No matching slur stop for slur start (note %d)! Ignoring slur.", 1+slurStartIdx));

					// Try to fix as a last remedy, set current note to non-slurred and set bow note index and bow length:
					note.setSlurred(false);
					note.setBowNoteIndex(0);
					note.setBowLength(1);

					// Set all following slurred notes to non-slurred as well:
					int noteIdxFix;
					for (noteIdxFix = slurStartIdx+1; noteIdxFix < target.getNumNotes(); ++noteIdxFix)
					{
						ScoreNote &noteFix = target.getNoteAt(noteIdxFix);

						if (!noteFix.isSlurred())
							break;

						noteFix.setSlurred(false);
						noteFix.setBowNoteIndex(0);
						noteFix.setBowLength(1);
					}

					// Update iterator:
					noteIdx = noteIdxFix-1; // will be incremented on continue
				}
			}
			else // slur but not after slur start
			{
				assert(note.getBowNoteIndex() == -1 && note.getBowLength() == -1);
				LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Slurred note but not following slur start (note %d)! Ignoring slur.", 1+noteIdx));

				// Try to fix as a last remedy by removing slur:
				note.setBowNoteIndex(0);
				note.setBowLength(1);
			}
		}

		// Sanity check:
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			ScoreNote &note = target.getNoteAt(noteIdx);
			const int bowIdx = note.getBowNoteIndex();

			bool bowIdxOk = true;
			if (!(bowIdx >= 0))
				bowIdxOk = false;

			if (noteIdx > 0)
			{
				ScoreNote &notePrev = target.getNoteAt(noteIdx-1);
				const int bowIdxPrev = notePrev.getBowNoteIndex();
				if (!(bowIdx == bowIdxPrev || bowIdx < bowIdxPrev || bowIdx == bowIdxPrev+1))
					bowIdxOk = false;
			}

			int bowLength = note.getBowLength();
			if (bowLength <= 0)
				bowIdxOk = false;

			if (bowIdx >= bowLength)
				bowIdxOk = false;

			if (!bowIdxOk)
				LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Bow note index or bow length invalid (note %d)!", 1+noteIdx));
		}
	}

	// -----------------------------------------------------------------------------------

	void MusicXmlParser::assignDynamicsToAllNotes(concat::InputScore &target)
	{
		// pass 1 - compute cresc./decresc. pos and length:
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			ScoreNote &note = target.getNoteAt(noteIdx);

			if (note.isRest())
				continue;

			if (note.getDynType() == DynamicsType::NORMAL)
			{
				note.setDynPos(0);
				note.setDynLength(1);
			}
			else
			{
				if (note.getDynType() == DynamicsType::CRESC ||
					note.getDynType() == DynamicsType::DECRESC)
				{
					int dynBegin = noteIdx;
					int dynEnd = -1;

					for (int noteIdxSearch = noteIdx+1; noteIdxSearch < target.getNumNotes(); ++noteIdxSearch)
					{
						ScoreNote &noteSearch = target.getNoteAt(noteIdxSearch);

						// (allow rests in between cresc./decresc.)

						if (noteSearch.isCrescDecrescStop() || noteIdxSearch == target.getNumNotes()-1)
						{
							dynEnd = noteIdxSearch+1;
							break;
						}
					}

					int dynLength = dynEnd - dynBegin;
					int dynPos = 0;

					for (int noteIdxSet = dynBegin; noteIdxSet < dynEnd; ++noteIdxSet)
					{
						ScoreNote &noteSet = target.getNoteAt(noteIdxSet);

						// (allow rests in between cresc./decresc.)

						noteSet.setDynPos(dynPos);
						noteSet.setDynLength(dynLength);
						++dynPos;
					}

					// Continue after cresc/decresc:
					noteIdx = dynEnd-1; // will get incremented above
				}
			}
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		// pass 2 - set cresc./decresc. dynamics values based on pos and length:
		DynamicsValue runningDynVal = DynamicsValue::NONE;
		DynamicsValue crescDecrescFromDynVal;
		DynamicsValue crescDecrescToDynVal;

		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			ScoreNote &note = target.getNoteAt(noteIdx);

			if (note.isRest())
				continue;

			// Update running dynamics value:
			if ((!note.getDynValue()) == false && note.getDynValue() != DynamicsValue::NONE)
			{
				if (note.getDynPos() > 0)
				{
					; // ignore dynamics changes in the middle of cresc./decresc. (at start are handled, and at end are handled below)
				}
				else
				{
					runningDynVal = note.getDynValue();
				}
			}

			std::string xxx = runningDynVal.getAsString();

			// Modify for cresc./decresc.:
			if (note.getDynType() == DynamicsType::CRESC || note.getDynType() == DynamicsType::DECRESC)
			{
				int crescDecrescBegin = noteIdx - note.getDynPos();
				int crescDecrescLast = crescDecrescBegin + note.getDynLength() - 1;
				float crescDecrescDurationBeats = (float)target.getNoteAt(crescDecrescLast).getEndBeats() - (float)target.getNoteAt(crescDecrescBegin).getBeginBeats();

				float noteMiddleBeats = (float)note.getBeginBeats() + (float)note.getDurationBeats()/2.0f - (float)target.getNoteAt(crescDecrescBegin).getBeginBeats();
				float normalizedNotePos = noteMiddleBeats/crescDecrescDurationBeats; // [0.0;1.0[

				// Compute from/to dynamics value for cresc./decresc. on the first note of the 
				// cresc./decresc.:
				if (noteIdx == crescDecrescBegin)
				{
					// Compute start dynamics value of cresc./decresc:
					crescDecrescFromDynVal = runningDynVal;

					// Compute end dynamics value of cresc./decresc.:
					bool toDynValFound = false;

					ScoreNote &lastNoteCrescDecresc = target.getNoteAt(crescDecrescLast);
					if ((!lastNoteCrescDecresc.getDynValue()) == false && lastNoteCrescDecresc.getDynValue() != DynamicsValue::NONE)
					{
						// Last note of cresc./decresc. has dynamics value specified:
						crescDecrescToDynVal = lastNoteCrescDecresc.getDynValue();
						toDynValFound = true;
					}

					if (!toDynValFound && crescDecrescLast+1 < target.getNumNotes())
					{
						ScoreNote &onePastLastNoteCrescDecresc = target.getNoteAt(crescDecrescLast+1);
						if ((!onePastLastNoteCrescDecresc.getDynValue()) == false && onePastLastNoteCrescDecresc.getDynValue() != DynamicsValue::NONE)
						{
							// One-past-last note of cresc./decresc. has dynamics value specified:
							crescDecrescToDynVal = onePastLastNoteCrescDecresc.getDynValue();
							toDynValFound = true;
						}
					}

					if (!toDynValFound)
					{
						const int crescDecrescDepth = 1;

						int toDynValInt;
						if (note.getDynType() == DynamicsType::CRESC)
						{
							toDynValInt = crescDecrescFromDynVal.getAsInt() + crescDecrescDepth;
						}
						else if (note.getDynType() == DynamicsType::DECRESC)
						{
							toDynValInt = crescDecrescFromDynVal.getAsInt() - crescDecrescDepth;
						}
						else
						{
							toDynValInt = crescDecrescFromDynVal.getAsInt(); // flat
						}

						// Make sure in bounds:
						if (toDynValInt < DynamicsValue::SILENCE)
							toDynValInt = DynamicsValue::SILENCE; // min possible dyn value
						else if (toDynValInt > DynamicsValue::FF)
							toDynValInt = DynamicsValue::FF; // max possible dyn value

						crescDecrescToDynVal = DynamicsValue(toDynValInt); // cast to enum
					}
				}

				// Compute ramped dyn value:
				int dynValue = convertCrescDecrescNotePosToDynamicsValue(note.getDynType(), normalizedNotePos, crescDecrescFromDynVal, crescDecrescToDynVal);

				runningDynVal = DynamicsValue(dynValue); // cast to enum
			}

			// Set running dynamics value:
			note.setDynamics(note.getDynType(), runningDynVal); // type stays the same
		}

/*		DynamicsValue fromDynVal;
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			ScoreNote &note = target.getNoteAt(noteIdx);

			if (note.getDynType() == DynamicsType::CRESC || note.getDynType() == DynamicsType::DECRESC)
			{
				int crescDecrescBegin = noteIdx - note.getDynPos();
				int crescDecrescLast = crescDecrescBegin + note.getDynLength() - 1;
				float crescDecrescDurationBeats = (float)target.getNoteAt(crescDecrescLast).getEndBeats() - (float)target.getNoteAt(crescDecrescBegin).getBeginBeats();

				float noteMiddleBeats = (float)note.getBeginBeats() + (float)note.getDurationBeats()/2.0f - (float)target.getNoteAt(crescDecrescBegin).getBeginBeats();
				float normalizedNotePos = noteMiddleBeats/crescDecrescDurationBeats; // [0.0;1.0[

				if (noteIdx == crescDecrescBegin)
				{
					fromDynVal = note.getDynValue();
				}

				int dynValue = convertCrescDecrescNotePosToDynamicsValue(note.getDynType(), normalizedNotePos, fromDynVal);

				// cast to enum:
				note.setDynamics(note.getDynType(), DynamicsValue(dynValue)); // overwrite dynValue
			}
		}*/

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		// Sanity check:
		// All non-rest notes should have dynamics set:
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			ScoreNote &note = target.getNoteAt(noteIdx);

			if (note.isRest())
				continue;

			if (!note.getDynType() || note.getDynType() == DynamicsType::NONE)
				LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Invalid dynamics type (note %d)!", 1+noteIdx));

			if (!note.getDynValue() || note.getDynValue() == DynamicsValue::NONE)
				LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Invalid dynamics value (note %d)!", 1+noteIdx));

			if (note.getDynLength() < 1)
				LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Invalid dynamics gesture length (note %d)!", 1+noteIdx));

			if (note.getDynPos() < 0 || note.getDynPos() >= note.getDynLength())
				LOG_WARN_N("concat.musicxmlparser", formatStr("Warning: Invalid dynamics gesture index (note %d)!", 1+noteIdx));
		}
	}


}


