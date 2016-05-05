#include "WaveSurferScoreParser.hxx"

#include "concat/InputScore/ScoreNote.hxx"
#include "concat/InputScore/InputScore.hxx"
#include "concat/Utilities/Filename.hxx"
#include "concat/InputScore/ViolinScoreConversions.hxx"
#include "concat/Utilities/StringConversions.hxx"

#include <string>

namespace concat
{
	WaveSurferScoreParser::Filenames WaveSurferScoreParser::getFilenames(const char *baseFilename, const char *addFilenameSuffix)
	{
		std::string baseFilenameStr = baseFilename;
		std::string addFilenameSuffixStr = addFilenameSuffix;

		Filenames result;

		result.notesFilename = Filename((baseFilenameStr + ".notes" + addFilenameSuffixStr).c_str());
		result.dynFilename = Filename((baseFilenameStr + ".dynamics" + addFilenameSuffixStr).c_str());
		result.bowDirFilename = Filename((baseFilenameStr + ".bowdir" + addFilenameSuffixStr).c_str());
		result.artTypeFilename = Filename((baseFilenameStr + ".legato" + addFilenameSuffixStr).c_str());
		result.stringFilename = Filename((baseFilenameStr + ".string" + addFilenameSuffixStr).c_str());
		result.accentFilename = Filename((baseFilenameStr + ".accent" + addFilenameSuffixStr).c_str());
		result.bowIdxFilename = Filename((baseFilenameStr + ".bowidx" + addFilenameSuffixStr).c_str());
		result.dynIdxFilename = Filename((baseFilenameStr + ".dynidx" + addFilenameSuffixStr).c_str());

		result.notesFilename.toAbsolute();
		result.dynFilename.toAbsolute();
		result.bowDirFilename.toAbsolute();
		result.artTypeFilename.toAbsolute();
		result.stringFilename.toAbsolute();
		result.accentFilename.toAbsolute();
		result.bowIdxFilename.toAbsolute();
		result.dynIdxFilename.toAbsolute();

		return result;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void WaveSurferScoreParser::load(const char *baseFilename, const char *addFilenameSuffix, concat::InputScore &target, bool allowPerformerAndSampleOverride)
	{
		failed_ = false;

		std::string baseFilenameStr = baseFilename;
		std::string addFilenameSuffixStr = addFilenameSuffix;

		Filenames filenames = getFilenames(baseFilename, addFilenameSuffix);

		WaveSurferFile notesFile;
		WaveSurferFile dynFile;
		WaveSurferFile bowDirFile;
		WaveSurferFile artTypeFile;
		WaveSurferFile stringFile;
		WaveSurferFile accentFile;
		WaveSurferFile bowIdxFile;
		WaveSurferFile dynIdxFile;

		notesFile.loadFromFile(filenames.notesFilename.getFullFilenameAsString().c_str());
		dynFile.loadFromFile(filenames.dynFilename.getFullFilenameAsString().c_str());
		bowDirFile.loadFromFile(filenames.bowDirFilename.getFullFilenameAsString().c_str());
		artTypeFile.loadFromFile(filenames.artTypeFilename.getFullFilenameAsString().c_str());
		stringFile.loadFromFile(filenames.stringFilename.getFullFilenameAsString().c_str());
		accentFile.loadFromFile(filenames.accentFilename.getFullFilenameAsString().c_str());
		bowIdxFile.loadFromFile(filenames.bowIdxFilename.getFullFilenameAsString().c_str());
		dynIdxFile.loadFromFile(filenames.dynIdxFilename.getFullFilenameAsString().c_str());

		if (!notesFile.isOk() || !dynFile.isOk() || !bowDirFile.isOk() || !artTypeFile.isOk() || !stringFile.isOk() || !accentFile.isOk() || !bowIdxFile.isOk() || !dynIdxFile.isOk())
		{
			failed_ = true;
			return; // error, failed opening files
		}

		bool hasSampleOverride = false;
		bool hasPerformerOverride = false;
		WaveSurferFile performerOverrideFile;
		WaveSurferFile sampleOverrideFile;
		if (allowPerformerAndSampleOverride)
		{
			// Optional .sample_override file that overrides the sample selection:
			Filename sampleOverrideFilename((baseFilenameStr + ".sample_override" + addFilenameSuffixStr).c_str());
			sampleOverrideFilename.toAbsolute();
			sampleOverrideFile.loadFromFile(sampleOverrideFilename.getFullFilenameAsString().c_str());
			if (sampleOverrideFile.isOk())
				hasSampleOverride = true;

			// Optional .performer_override file that overrides the performer model (uses recorded performance actions instead):
			Filename performerOverrideFilename((baseFilenameStr + ".performer_override" + addFilenameSuffixStr).c_str());
			performerOverrideFilename.toAbsolute();
			performerOverrideFile.loadFromFile(performerOverrideFilename.getFullFilenameAsString().c_str());
			if (performerOverrideFile.isOk())
				hasPerformerOverride = true;
		}

		target.setTempo(0.0); // ignored
//		target.setTimeSig(); // ignored
//		target.setKeySig(); // ignored
		// XXX: could use .metronome file!!!

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		// Pass 1 - Create notes:

		try
		{
			for (int i = 0; i < notesFile.getNumEntries(); ++i)
			{
				const WaveSurferEntry *noteEntry = notesFile.getEntryAtIndex(i);

				if (noteEntry == NULL)
					continue; // should never happen normally

				if (noteEntry->onsetSeconds > noteEntry->endSeconds)
					continue; // invalid entry

				ScoreNote note;

				note.setNoteIdx(i);
				note.setMeasureFirstNoteIdx(0); // not used
				note.setMeasureIdx(0); // not used
				note.setBeginBeats(0.0); // ignored
				note.setDurationBeats(0.0); // ignored
				note.setBeginSeconds(noteEntry->onsetSeconds);
				note.setDurationSeconds(noteEntry->endSeconds - noteEntry->onsetSeconds);

				// Use center times of notes to look up other labels (safer than using 
				// begin time in case boundaries across different files do not match 
				// 100% exactly):
				const float noteCenterTime = (noteEntry->onsetSeconds + noteEntry->endSeconds)/2.0f;

				// Rest note:
				if (noteEntry->label == "" || noteEntry->label == "rest")
				{
					note.setIsRest(true);
				}
				// Non-rest note:
				else
				{
					// rest:
					note.setIsRest(false);
					
					// pitch:
					note.setPitchStr(noteEntry->label.c_str());

					// bad (for discarding samples when creating db xml):
					if (noteEntry->hasAdditionalLabel("BAD"))
						note.isBad = true;
					else
						note.isBad = false;
					
					// dynamics (type, value):
					WaveSurferEntry dynEntry = dynFile.getEntryAtTime2(noteCenterTime);
					std::string dynLabel = dynEntry.label;
					DynamicsType dynType;
					DynamicsValue dynValue;
					if (dynLabel == "cresc" || dynLabel == "decresc")
					{
						dynType = DynamicsType(dynLabel.c_str());
						if (dynEntry.getNumAdditionalLabels() > 0)
						{
							dynValue = DynamicsValue(dynEntry.getAdditionalLabelNoParameters(0).c_str());
						}
						else
						{
							LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: Invalid additional dynamics value label for cresc./decresc. note %d. Using default (mezzoforte).", dynLabel.c_str(), 1+i));
							dynValue = DynamicsValue(DynamicsValue::MF);
						}
					}
					else
					{
						dynType = DynamicsType::NORMAL;
						dynValue = DynamicsValue(dynLabel.c_str());
					}

					if (!dynType || !dynValue)
					{
						failed_ = true; // fail, but continue (so all errors are logged)
						LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: Invalid dynamics (type/value) label \"%s\" for non-rest note %d.", dynLabel.c_str(), 1+i));
					}
					note.setDynamics(dynType, dynValue);

					// articulation type:
					std::string artTypeLabel = artTypeFile.getEntryAtTime2(noteCenterTime).label;
					ArticulationType artType(artTypeLabel.c_str());
					if (!artType)
					{
						failed_ = true; // fail, but continue (so all errors are logged)
						LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: Invalid articulation type label \"%s\" for non-rest note %d.", artTypeLabel.c_str(), 1+i));
					}
					note.setArticulationType(artType);

					// bow dir:
					std::string bowDirLabel = bowDirFile.getEntryAtTime2(noteCenterTime).label;
					BowDirection bowDir(bowDirLabel.c_str());
					if (!bowDir && !note.isPizzicato())
					{
						failed_ = true; // fail, but continue (so all errors are logged)
						LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: Invalid bow direction label \"%s\" for non-rest note %d.", bowDirLabel.c_str(), 1+i));
					}
					note.setBowDirection(bowDir);
					
					// played string:
					std::string playedStringLabel = stringFile.getEntryAtTime2(noteCenterTime).label;
					note.setPlayedString(playedStringLabel.c_str());
					if (!note.isPlayedStringValid())
					{
						failed_ = true; // fail, but continue (so all errors are logged)
						LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: Invalid played string label \"%s\" for non-rest note %d.", playedStringLabel.c_str(), 1+i));
					}

					// accent:
					std::string accentLabel = accentFile.getEntryAtTime2(noteCenterTime).label;
					Accent accent(accentLabel.c_str());
					if (!accent)
					{
						failed_ = true; // fail, but continue (so all errors are logged)
						LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: Invalid accents label \"%s\" for non-rest note %d.", accentLabel.c_str(), 1+i));
					}
					note.setAccent(accent);

					// additional synthesis controls:
					getAdditionalNoteParameters(*noteEntry, i, note);
				}

				// -----------------------------------------------------------------------

				// Sample override:
				// (For tests Alfonso)
				if (hasSampleOverride)
				{
					std::string dbXmlFilename = sampleOverrideFile.getEntryAtTime2(noteCenterTime).label;
					if (dbXmlFilename != "")
					{
						std::stringstream s2i;
						s2i << sampleOverrideFile.getEntryAtTime2(noteCenterTime).getAdditionalLabelNoParameters(0);
						int sampleIndex;
						s2i >> sampleIndex;
						note.setSampleOverride(dbXmlFilename.c_str(), sampleIndex);
					}
				}

				// Performer override:
				// (For tests Alfonso)
				if (hasPerformerOverride)
				{
					std::string performerBaseFilename = performerOverrideFile.getEntryAtTime2(noteCenterTime).label;
					if (performerBaseFilename != "")
					{
						note.setPerformerOverride(performerBaseFilename.c_str());
					}
				}

				target.pushBackNote(note);
			}
		}
		catch (const std::runtime_error &e) // getEntryAtTime2() may throw exceptions
		{
			LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: %s", e.what()));
			failed_ = true;
			return; // error, one or more labels incorrect
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		// Pass 2 - Create note bow index and bow length fields from .bowidx file:

		if (target.getNumNotes() != bowIdxFile.getNumEntries())
		{
			LOG_ERROR_N("concat.wavesurferfileio", "Error: Bow idx file does not have same number of entries as notes in score. Can't create note bow indexes.");
			failed_ = true;
			return; // error, failed creating bow note indexes
		}

		int nextNoteBowIdx = -1; // invalid
		for (int noteIdx = target.getNumNotes() - 1; noteIdx >= 0; --noteIdx)
		{
			const int curNoteBowIdx = intFromString(bowIdxFile.getEntryAtIndex(noteIdx)->label);

			const bool isLastNote = (noteIdx == target.getNumNotes() - 1);

			// if last note (no noteBowIdx[i+1]) or noteBowIdx[i+1] <= noteBowIdx[i]:
			if (isLastNote || nextNoteBowIdx <= curNoteBowIdx)
			{
				const int bowBeginIdx = noteIdx - curNoteBowIdx; // last - (length-1)
				const int bowLastIdx = noteIdx;
				const int bowLength = curNoteBowIdx+1;

				for (int noteIdxSet = bowBeginIdx; noteIdxSet <= bowLastIdx; ++noteIdxSet)
				{
					ScoreNote &noteSet = target.getNoteAt(noteIdxSet);
					noteSet.setBowNoteIndex(noteIdxSet - bowBeginIdx);
					noteSet.setBowLength(bowLength);
				}
			}

			nextNoteBowIdx = curNoteBowIdx; // iterating backwards
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		// Pass 3 - Set dyn. gesture length/indexes (for cresc./decresc.):

		if (target.getNumNotes() != dynIdxFile.getNumEntries())
		{
			LOG_ERROR_N("concat.wavesurferfileio", "Error: Dynamics gesture idx file does not have same number of entries as notes in score. Can't create dynamics gestures.");
			failed_ = true;
			return; // error, failed creating bow note indexes
		}

		int nextDynGestIdx = -1; // invalid
		for (int noteIdx = target.getNumNotes() - 1; noteIdx >= 0; --noteIdx)
		{
			int curDynGestIdx = intFromString(dynIdxFile.getEntryAtIndex(noteIdx)->label);

			if (curDynGestIdx == -1)
				curDynGestIdx = 0; // work-around for rests (-1 dynamics gesture index)

			const bool isLastNote = (noteIdx == target.getNumNotes() - 1);

			// if last note (no dynGestIdx[i+1]) or dynGestIdx[i+1] <= dynGestIdx[i]:
			if (isLastNote || nextDynGestIdx <= curDynGestIdx)
			{
				const int dynGestBeginIdx = noteIdx - curDynGestIdx; // last - (length-1)
				const int dynGestLastIdx = noteIdx;
				const int dynGestLength = curDynGestIdx+1;

				for (int noteIdxSet = dynGestBeginIdx; noteIdxSet <= dynGestLastIdx; ++noteIdxSet)
				{
					ScoreNote &noteSet = target.getNoteAt(noteIdxSet);
					noteSet.setDynPos(noteIdxSet - dynGestBeginIdx);
					noteSet.setDynLength(dynGestLength);
				}
			}

			nextDynGestIdx = curDynGestIdx; // iterating backwards
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		// Sanity check:
		// See if all note pitches are valid for played string fields.
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			const ScoreNote &note = target.getNoteAt(noteIdx);

			if (note.isRest())
				continue;

			if (!note.isPlayedStringValid())
			{
				LOG_WARN_N("concat.wavesurferfileio", formatStr("Warning: Note %d has no valid played string specified in score.", 1+note.getNoteIdxRelativeToBeginMeasure(), 1+note.getNoteIdx()));
			}
			else
			{
				if (!note.isValidNotePitchForPlayedString(24))
				{
					LOG_WARN_N("concat.wavesurferfileio", formatStr("Warning: Note %d is out of pitch range of played string (allowing up to two octaves).", 1+note.getNoteIdx()));
					LOG_WARN_N("concat.wavesurferfileio", formatStr("         %s (%d) does not lie on %s string; between %s (%d) and %s (%d).",
						note.getPitchStr(), note.getMidiNote(),
						note.getPlayedString(),
						convertNotePitchFromMidiNoteToStringNoKey(note.getPlayedStringAsMidiNote()).c_str(), note.getPlayedStringAsMidiNote(),
						convertNotePitchFromMidiNoteToStringNoKey(note.getPlayedStringAsMidiNote()+24).c_str(), note.getPlayedStringAsMidiNote()+24));
				}
			}
		}

		// Sanity check:
		// All non-rest notes should have dynamics set:
		for (int noteIdx = 0; noteIdx < target.getNumNotes(); ++noteIdx)
		{
			ScoreNote &note = target.getNoteAt(noteIdx);

			if (note.isRest())
				continue;

			if (!note.getDynType() || note.getDynType() == DynamicsType::NONE)
				LOG_WARN_N("concat.wavesurferfileio", formatStr("Warning: Invalid dynamics type (note %d)!", 1+noteIdx));

			if (!note.getDynValue() || note.getDynValue() == DynamicsValue::NONE)
				LOG_WARN_N("concat.wavesurferfileio", formatStr("Warning: Invalid dynamics value (note %d)!", 1+noteIdx));

			if (note.getDynLength() < 1)
				LOG_WARN_N("concat.wavesurferfileio", formatStr("Warning: Invalid dynamics gesture length (note %d)!", 1+noteIdx));

			if (note.getDynPos() < 0 || note.getDynPos() >= note.getDynLength())
				LOG_WARN_N("concat.wavesurferfileio", formatStr("Warning: Invalid dynamics gesture index (note %d)!", 1+noteIdx));
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		// (failed_ set above)
	}

	// -----------------------------------------------------------------------------------

	bool WaveSurferScoreParser::isOk() const
	{
		return (failed_ == false);
	}

	// -----------------------------------------------------------------------------------

	// get time0:value0:time1:value1:...:timeN:valueN break-point function
	// t is normalized over note duration [0;1]
	void getBpf2(std::string label, const concat::WaveSurferEntry &wsNote, int noteIdx, bool &targetHasFlag, std::vector<float> &targetTimePoints, std::vector<float> &targetValuePoints, bool checkValueRange, float minValue = 0.0f, float maxValue = 0.0f)
	{
		targetHasFlag = (wsNote.hasAdditionalLabel(label) && (wsNote.getNumLabelParameters(label) == 1 || wsNote.getNumLabelParameters(label) >= 4));
		if (targetHasFlag)
		{
			if (wsNote.getNumLabelParameters(label) == 1)
			{
				float val = wsNote.getLabelParameter(label, 0);
				if (checkValueRange)
				{
					if (val < minValue || val > maxValue)
					{
						LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: %s break-point function parameters need to be in-range (t=[0.0;1.0], value=[%.2f;%.2f]). Ignoring parameter for targetNote %d.", label.c_str(), minValue, maxValue, noteIdx));
						targetHasFlag = false;
					}
				}
				targetTimePoints.push_back(0.0f);
				targetValuePoints.push_back(val);
				targetTimePoints.push_back(1.0f);
				targetValuePoints.push_back(val);
			}
			else
			{
				if ((wsNote.getNumLabelParameters(label) % 2) != 0)
				{
					LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: %s break-point function number of parameters values need to be a multiple of two. Ignoring parameter for targetNote %d.", label.c_str(), 1+noteIdx));
					targetHasFlag = false;
				}
				else
				{
					for (int j = 0; j < wsNote.getNumLabelParameters(label)/2; ++j)
					{
						float t = wsNote.getLabelParameter(label, 2*j);
						float val = wsNote.getLabelParameter(label, 2*j+1);

						if (val < minValue || val > maxValue)
						{
							if (t < 0.0f || t > 1.0f ||
								val < minValue || val > maxValue)
							{
								LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: %s break-point function parameters need to be in-range (t=[0.0;1.0], value=[%.2f;%.2f]). Ignoring parameter for targetNote %d.", label.c_str(), minValue, maxValue, noteIdx));
								targetHasFlag = false;
								break; // ignore further parameters
							}
						}

						targetTimePoints.push_back(t);
						targetValuePoints.push_back(val);
					}
				}
			}
		}
	}

	void WaveSurferScoreParser::getAdditionalNoteParameters(const concat::WaveSurferEntry &wsNote, int noteIdx, concat::ScoreNote &targetNote)
	{
		// Vibrato:
		targetNote.hasVibratoControls = wsNote.hasAdditionalLabel("VIBRATO");
		if (targetNote.hasVibratoControls && wsNote.getNumLabelParameters("VIBRATO") == 4)
		{
			targetNote.vibratoBeginNorm = wsNote.getLabelParameter("VIBRATO", 0);
			targetNote.vibratoEndNorm = wsNote.getLabelParameter("VIBRATO", 1);
			targetNote.vibratoDepthCents = wsNote.getLabelParameter("VIBRATO", 2);
			targetNote.vibratoRateHz = wsNote.getLabelParameter("VIBRATO", 3);
			targetNote.areVibratoControlsConstant = true;
		}
		else if (targetNote.hasVibratoControls && wsNote.getNumLabelParameters("VIBRATO") == 6)
		{
			targetNote.vibratoBeginNorm = wsNote.getLabelParameter("VIBRATO", 0);
			targetNote.vibratoEndNorm = wsNote.getLabelParameter("VIBRATO", 1);
			targetNote.vibratoDepthCents = wsNote.getLabelParameter("VIBRATO", 2);
			targetNote.vibratoDepthCentsEnd = wsNote.getLabelParameter("VIBRATO", 3);
			targetNote.vibratoRateHz = wsNote.getLabelParameter("VIBRATO", 4);
			targetNote.vibratoRateHzEnd = wsNote.getLabelParameter("VIBRATO", 5);
			targetNote.areVibratoControlsConstant = false;
		}

		// Vibrato break-point function:
		targetNote.hasVibratoBpfControls = (wsNote.hasAdditionalLabel("VIBRATO_BPF") && wsNote.getNumLabelParameters("VIBRATO_BPF") >= 6);
		if (targetNote.hasVibratoBpfControls)
		{
			const int numParams = wsNote.getNumLabelParameters("VIBRATO_BPF");
			if ((numParams % 3) != 0)
			{
				LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: Vibrato break-point function parameters need to be a multiple of three. Ignoring parameter for targetNote %d.", 1+noteIdx));
				targetNote.hasVibratoBpfControls = false;
			}
			else
			{
				for (int j = 0; j < numParams/3; ++j)
				{
					float t = wsNote.getLabelParameter("VIBRATO_BPF", 3*j);
					float depth = wsNote.getLabelParameter("VIBRATO_BPF", 3*j+1);
					float rate = wsNote.getLabelParameter("VIBRATO_BPF", 3*j+2);

					if (t < 0.0f || t > 1.0f ||
						depth < 0.0f || depth > 1200.0f || 
						rate < 0.0f || rate > 500.0f)
					{
						LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: Vibrato break-point function parameters need to be in-range (t=[0;1],d=[0;1200],r=[0;500]). Ignoring parameter for targetNote %d.", 1+noteIdx));
						targetNote.hasVibratoBpfControls = false;
						break;
					}

					targetNote.vibBpfTimePoints.push_back(t);
					targetNote.vibBpfDepthPoints.push_back(depth);
					targetNote.vibBpfRatePoints.push_back(rate);
				}
			}
		}

		// Glissando (glide pitch to next targetNote):
		targetNote.hasGlissandoControls = (wsNote.hasAdditionalLabel("GLISSANDO") && wsNote.getNumLabelParameters("GLISSANDO") >= 1);
		if (targetNote.hasGlissandoControls)
		{
			float x = wsNote.getLabelParameter("GLISSANDO", 0);
			if (x < 0.0f || x > 1.0f)
			{
				LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: Glissando begin should be in normalized time ([0;1]). Ignoring parameter for targetNote %d.", 1+noteIdx));
				targetNote.hasGlissandoControls = false;
			}
			targetNote.glissandoBeginNorm = x;
		}

		// Scoop (glide pitch from previous targetNote):
		targetNote.hasScoopControls = (wsNote.hasAdditionalLabel("SCOOP") && wsNote.getNumLabelParameters("SCOOP") >= 1);
		if (targetNote.hasScoopControls)
		{
			float x = wsNote.getLabelParameter("SCOOP", 0);
			if (x < 0.0f || x > 1.0f)
			{
				LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: Scoop end should be in normalized time ([0;1]). Ignoring parameter for targetNote %d.", 1+noteIdx));
				targetNote.hasScoopControls = false;
			}
			targetNote.scoopEndNorm = x;
		}

		// Dynamics break-point function:
		getBpf2("DYNAMICS", wsNote, noteIdx, targetNote.hasDynamicsControls, targetNote.dynamicsPointXNorm, targetNote.dynamicsPointYNorm, true, 0.0f, 6.0f);

		// Residual break-point function:
		getBpf2("RESIDUAL", wsNote, noteIdx, targetNote.hasResidualControls, targetNote.residualBpfTimeNorm, targetNote.residualBpfGainDb, false);

		// Timbre model apply factor break-point function:
		getBpf2("TIMBRE", wsNote, noteIdx, targetNote.hasTimbreControls, targetNote.timbreBpfTimeNorm, targetNote.timbreBpfFactor, true, 0.0f, 1.0f);

		// Timbre model apply factor break-point function:
		targetNote.hasGlissandoTiltControls = (wsNote.hasAdditionalLabel("GLISSANDO_TILT") && wsNote.getNumLabelParameters("GLISSANDO_TILT") >= 1);
		if (targetNote.hasGlissandoTiltControls)
		{
			float y = wsNote.getLabelParameter("GLISSANDO_TILT", 0);
			if (y < -300.0f || y > 0.0f)
			{
				LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: Glissando tilt attenuation should be a value in decibels (and negative, [-300;0]). Ignoring parameter for targetNote %d.", 1+noteIdx));
				targetNote.hasGlissandoTiltControls = false;
			}
			targetNote.glissandoTiltAttenuationDb = y;
		}

		// Pitch-bend break-point function:
		// Note: Does NOT affect the sample selection.
		// This is on purpose so pitch-shift can be used to synthesize double strings while 
		// the same sample is picked twice.
		getBpf2("PITCH_BEND", wsNote, noteIdx, targetNote.hasPitchBendControls, targetNote.pitchBendBpfTimeNorm, targetNote.pitchBendBpfCents, true, -1200.0f, +1200.0f);

		// Gain break-point function:
		// Note: Affects both harmonics and residual (equally).
		getBpf2("GAIN", wsNote, noteIdx, targetNote.hasGainControls, targetNote.gainBpfTimeNorm, targetNote.gainBpfDb, true, -300.0f, +48.0f);

		// String override (for synthesizing 'fake double strings' manually):
		targetNote.hasStringOverrideControls = (wsNote.hasAdditionalLabel("STRING_OVERRIDE") && wsNote.getNumLabelParameters("STRING_OVERRIDE") >= 1);
		if (targetNote.hasStringOverrideControls)
		{
			float y = wsNote.getLabelParameter("STRING_OVERRIDE", 0);
			if (y < 1.f || y > 4.f)
			{
				LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: STRING_OVERRIDE should be between 1 and 4. Ignoring parameter for targetNote %d.", 1+noteIdx));
				targetNote.hasStringOverrideControls = false;
			}
			targetNote.stringOverrideStrIdx = (int)(y);
		}
	}

}
