#ifndef INCLUDED_CONCAT_WAVESURFERFILE_HXX
#define INCLUDED_CONCAT_WAVESURFERFILE_HXX

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <utility>
#include <stdexcept>

#include "concat/Utilities/Logging.hxx"
//#ifndef INCLUDED_CONCAT_LOGGING_HXX
//#define LOG_INFO_N(unused1, unused2)
//#define LOG_ERROR_N(unused1, unused2)
//#endif

namespace concat
{
	struct WaveSurferEntry
	{
	public:
		WaveSurferEntry();
		WaveSurferEntry(std::string entryStr);
		WaveSurferEntry(float onsetSeconds, float endSeconds, std::string labelStr);

		// Wave Surfer compatible entry:
		// -----------------------------
	public:
		float onsetSeconds;
		float endSeconds;
		std::string label; // no spaces

		// Extensions for input score and marking bad notes in database:
		// -------------------------------------------------------------
	private:
		std::vector<std::string> additionalLabels_; // parsing of parameters is not stored, but done for each access

	public:
		void addAdditionalLabel(const std::string &fullLabel); // label with parameters
		
		int getNumAdditionalLabels() const;
		const std::string &getAdditionalLabelWithParameters(int idx) const;
		std::string getAdditionalLabelNoParameters(int idx) const;
		
		bool hasAdditionalLabel(const std::string &labelNameNoParameters) const;

		int getNumLabelParameters(const std::string &labelName) const;
		float getLabelParameter(const std::string &labelName, int paramIdx) const;

		std::string getFullLabelStr() const; // including all additional labels (with all their parameters)

	private:
		std::string findAdditionalLabelWithParameters(const std::string &labelNameNoParameters) const;
		std::string findAdditionalLabelNoParameters(const std::string &labelNameNoParameters) const;

		// utility for parsing LABEL:n:n:n:n:n (where n are numerical parameters)
		// used for both getting a specific parameter or computing the total number of parameters (paramIdx = -1)
		std::pair<float, int> parseParameters(const std::string &labelName, int paramIdx) const;
	};

	// -----------------------------------------------------------------------------------

	// Wave Surfer file with some custom extensions (multiple labels).
	//
	// Wave Surfer format:
	//    "begin_time end_time label"
	// (begin_time and end_time are floating point numbers and label is a text string 
	// which for this parser may not contain spaces)
	//
	// Extended format:
	//    "begin_time end_time label add_label1 add_label2 add_label3 ..."
	// where labels after the first can have parameters for instance
	// 2.50000 5.00000 A3 GLISSANDO:0.8 VIBRATO:0.3:0.9:1.5:2.0
	class WaveSurferFile
	{
	public:
		typedef std::vector<WaveSurferEntry>::const_iterator ConstIterator;

	public:
		WaveSurferFile();

		void loadFromFile(const char *filename);
		void saveToFile(const char *filename, bool allowGaps = false);
		bool isOk() const; // for checking if loading/saving failed

		ConstIterator beginEntries() const;
		ConstIterator endEntries() const;

		int getNumEntries() const;
		const WaveSurferEntry *getEntryAtIndex(int index) const; // returns NULL if index is invalid
		const WaveSurferEntry *getEntryAtTime(float timeSeconds); // returns NULL if no label is active at specified time
		const WaveSurferEntry &getEntryAtTime2(float timeSeconds); // throws exception if no label is active at specified time

		void collapseEntriesWithIdenticalLabels();
		void collapseEntryWithPrevious(int index);

		void addEntry(const WaveSurferEntry &entry);
		void eraseEntryAtIndex(int index);

	private:
		bool failed_;
		std::vector<WaveSurferEntry> entries_; // sorted

		bool checkIfAllTimesAreValid();
		
		std::string filename_; // for error messages
	};
}

// ---------------------------------------------------------------------------------------

namespace concat
{
	inline WaveSurferFile::WaveSurferFile()
	{
		failed_ = true;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	inline void WaveSurferFile::loadFromFile(const char *filename)
	{
		LOG_INFO_N("concat.wavesurferfileio", formatStr("Loading WaveSurfer file \"%s\"...", Filename(filename).getFilenameOnlyAsString().c_str()));

		std::ifstream file;
		file.open(filename, std::ifstream::in);

		if (!file)
		{
			failed_ = true;
			filename_ = "";
			return;
		}

		entries_.clear();

		// read line by line:
		std::string line;
		while (!std::getline(file, line).fail()) // failed (possibly due to eof)
		{
			if (line == "")
				continue;
			WaveSurferEntry tmp(line); // might fail, onset/end will be set to -1
			if (tmp.onsetSeconds >= 0.0f && tmp.endSeconds >= tmp.onsetSeconds)
				entries_.push_back(tmp);
		}

		file.close();

		if (!checkIfAllTimesAreValid())
		{
			failed_ = true;
			filename_ = "";
			return;
		}

		failed_ = false;
		filename_ = filename;
	}

	inline void WaveSurferFile::saveToFile(const char *filename, bool allowGaps)
	{
		if (!checkIfAllTimesAreValid() && !allowGaps)
		{
			failed_ = true;
			return;
		}

		std::ofstream file;
		file.open(filename, std::ios_base::trunc);

		if (!file)
		{
			failed_ = true;
			return;
		}

		file.setf(std::ios::fixed, std::ios::floatfield);
		file.precision(7); // 7 seems to be what WaveSurfer 1.8.5 uses

		for (int i = 0; i < getNumEntries(); ++i)
		{
			file <<	getEntryAtIndex(i)->onsetSeconds << " " << getEntryAtIndex(i)->endSeconds << " " << getEntryAtIndex(i)->getFullLabelStr() << std::endl;
		}

		file.close();

		failed_ = false;
	}

	inline bool WaveSurferFile::isOk() const
	{
		return (failed_ != true);
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	inline WaveSurferFile::ConstIterator WaveSurferFile::beginEntries() const
	{
		return entries_.begin();
	}

	inline WaveSurferFile::ConstIterator WaveSurferFile::endEntries() const
	{
		return entries_.end();
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	inline int WaveSurferFile::getNumEntries() const
	{
		return (int)entries_.size();
	}

	inline const WaveSurferEntry *WaveSurferFile::getEntryAtIndex(int index) const
	{
		if (index < 0 || index >= getNumEntries())
			return NULL;

		return &entries_[index];
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	inline const WaveSurferEntry *WaveSurferFile::getEntryAtTime(float timeSeconds)
	{
		// XXX: algorithm using std::lower_bound() didn't work for some reason, using 
		// "brute-force" approach instead (below):

		// first element whose end > time
		for (std::vector<WaveSurferEntry>::const_iterator i = entries_.begin(); i != entries_.end(); ++i)
		{
			if ((*i).endSeconds > timeSeconds)
				return &(*i);
		}

		return NULL;
	}

	inline const WaveSurferEntry &WaveSurferFile::getEntryAtTime2(float timeSeconds)
	{
		const WaveSurferEntry *result = getEntryAtTime(timeSeconds);
		if (result == NULL)
		{
			std::stringstream msg;
			msg << "No label active at specified time (file=\"" << filename_ << "\", time=" << timeSeconds << ").";
			throw std::runtime_error(msg.str());
		}
		return *result;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	inline void WaveSurferFile::collapseEntriesWithIdenticalLabels()
	{
		std::vector<WaveSurferEntry>::iterator i = entries_.begin();

		while (1)
		{
			if (i == entries_.end())
				break;

			std::vector<WaveSurferEntry>::iterator next = i;
			++next;

			if (next == entries_.end())
				break; // no next phoneme, thus nothing more to collapse

			if ((*i).getFullLabelStr() == (*next).getFullLabelStr())
			{
				(*i).endSeconds += (*next).endSeconds - (*next).onsetSeconds;
				i = entries_.erase(next); // removes next and returns iterator to element after next (after operation)
				--i; // re-try current in case more than 2 identical phonemes
			}
			else
			{
				i = next;
			}
		}
	}

	inline void WaveSurferFile::collapseEntryWithPrevious(int index)
	{
		if (index <= 0)
		{
			LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: Trying to collapse WaveSurfer entry %d with previous, but there is no previous. Ignoring collapse.", 1+index));
			return;
		}

		if (index >= getNumEntries())
		{
			LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: Trying to collapse WaveSurfer entry %d with previous, but entry doesn't exist for that index. Ignoring collapse.", 1+index));
			return;
		}

		WaveSurferEntry &cur = entries_[index];
		WaveSurferEntry &prev = entries_[index-1];

		if (cur.getFullLabelStr() == prev.getFullLabelStr())
		{
			cur.onsetSeconds = prev.onsetSeconds;
			eraseEntryAtIndex(index-1);
		}
		else
		{
			LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: Trying to collapse WaveSurfer entry %d with previous, but labels are different. Ignoring collapse.", 1+index));
			return;
		}
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	inline void WaveSurferFile::addEntry(const WaveSurferEntry &entry)
	{
		entries_.push_back(entry);
	}

	inline void WaveSurferFile::eraseEntryAtIndex(int index)
	{
		if (index < 0 || index >= getNumEntries())
			return;

		std::vector<WaveSurferEntry>::iterator i = entries_.begin();
		for (int n = 0; n < index; ++n)
			++i;
		entries_.erase(i);
	}

	// -----------------------------------------------------------------------------------

	inline bool WaveSurferFile::checkIfAllTimesAreValid()
	{
		float prevEndSeconds = -1.0f;

		for (int i = 0; i < getNumEntries(); ++i)
		{
			float onset = entries_[i].onsetSeconds;
			float end = entries_[i].endSeconds;

			if (prevEndSeconds != -1.0f && onset != prevEndSeconds)
			{
				if (onset > prevEndSeconds)
					LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: Gap between current WaveSurfer entry (line = %d) and previous.", 1+i));
				else
					LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: Overlap between current WaveSurfer entry (line = %d) and previous.", 1+i));
				return false;
			}

			if (onset > end)
			{
				LOG_ERROR_N("concat.wavesurferfileio", formatStr("Error: WaveSurfer entry onset time exceeds its end time (line = %d).", 1+i));
				return false;
			}

			prevEndSeconds = end;
		}

		return true;
	}
}

// ---------------------------------------------------------------------------------------

namespace concat
{
	inline WaveSurferEntry::WaveSurferEntry()
	{
		onsetSeconds = -1.0f;
		endSeconds = -1.0f;
		label = "";
	}

	inline WaveSurferEntry::WaveSurferEntry(std::string entryStr)
	{
		std::stringstream entryStream(entryStr);

		// read begin time:
		entryStream >> onsetSeconds;
		if (entryStream.fail())
		{
			onsetSeconds = -1.0f;
			return; // error -> valid onset and end are required
		}

		// read end time:
		entryStream >> endSeconds;
		if (entryStream.fail())
		{
			onsetSeconds = -1.0f;
			endSeconds = -1.0f;
			return; // error -> valid onset and end are required
		}

		// read label:
		entryStream >> label;
		if (entryStream.fail())
		{
			label = ""; // empty label
			// (may be ok to continue, label not strictly required)
		}

		// extract other whitespace separated tokens in case label contains spaces
		while (!entryStream.fail())
		{
			std::string labelCont;
			entryStream >> labelCont;
			if (!entryStream.fail())
				addAdditionalLabel(labelCont);
		}
	}

	inline WaveSurferEntry::WaveSurferEntry(float onsetSeconds, float endSeconds, std::string labelStr)
	{
		this->onsetSeconds = onsetSeconds;
		this->endSeconds = endSeconds;
        
		std::stringstream labelStream(labelStr);

		// read label:
		labelStream >> label;
		if (labelStream.fail())
		{
			label = ""; // empty label
			// (may be ok to continue, label not strictly required)
		}

		// extract other whitespace separated tokens in case label contains spaces
		while (!labelStream.fail())
		{
			std::string labelCont;
			labelStream >> labelCont;
			if (!labelStream.fail())
				addAdditionalLabel(labelCont);
		}
	}


	// -----------------------------------------------------------------------------------

	inline void WaveSurferEntry::addAdditionalLabel(const std::string &fullLabel)
	{
		additionalLabels_.push_back(fullLabel);
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	inline int WaveSurferEntry::getNumAdditionalLabels() const
	{
		return (int)additionalLabels_.size();
	}

	inline const std::string &WaveSurferEntry::getAdditionalLabelWithParameters(int idx) const
	{
		return additionalLabels_.at(idx);
	}

	inline std::string WaveSurferEntry::getAdditionalLabelNoParameters(int idx) const
	{
		std::string fullLabel = getAdditionalLabelWithParameters(idx);
		std::string::size_type nameEnd = fullLabel.find(":"); // parameter separator
		if (nameEnd == std::string::npos)
			nameEnd = fullLabel.size();
		return fullLabel.substr(0, nameEnd - 0);
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	inline bool WaveSurferEntry::hasAdditionalLabel(const std::string &labelNameNoParameters) const
	{
		return (findAdditionalLabelNoParameters(labelNameNoParameters) != "");
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	inline int WaveSurferEntry::getNumLabelParameters(const std::string &labelName) const
	{
		return parseParameters(labelName, -1).second;
	}
	
	inline float WaveSurferEntry::getLabelParameter(const std::string &labelName, int paramIdx) const
	{
		return parseParameters(labelName, paramIdx).first;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	inline std::string WaveSurferEntry::getFullLabelStr() const
	{
		std::string result = label;
		for (int i = 0; i < getNumAdditionalLabels(); ++i)
		{
			result += " " + getAdditionalLabelWithParameters(i);
		}

		return result;
	}

	// -----------------------------------------------------------------------------------

	inline std::string WaveSurferEntry::findAdditionalLabelWithParameters(const std::string &labelNameNoParameters) const
	{
		for (int i = 0; i < getNumAdditionalLabels(); ++i)
		{
			if (getAdditionalLabelNoParameters(i) == labelNameNoParameters)
			{
				return getAdditionalLabelWithParameters(i);
			}
		}

		return "";
	}

	inline std::string WaveSurferEntry::findAdditionalLabelNoParameters(const std::string &labelNameNoParameters) const
	{
		for (int i = 0; i < getNumAdditionalLabels(); ++i)
		{
			std::string labelNoParameters = getAdditionalLabelNoParameters(i);
			if (labelNoParameters == labelNameNoParameters)
			{
				return labelNoParameters;
			}
		}

		return "";
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	inline std::pair<float, int> WaveSurferEntry::parseParameters(const std::string &labelName, int paramIdx) const
	{
		std::string label = findAdditionalLabelWithParameters(labelName);
		if (label == "")
			return std::pair<float, int>(0.0f, -1); // failed

		int curParamIdx = 0;

		std::string::size_type beginParameter = label.find(":"); // begin first parameter
		if (beginParameter == std::string::npos)
		{
			return std::pair<float, int>(0.0f, -1); // failed
		}
		else
		{
			while (1)
			{
				beginParameter += 1; // skip ':'
				if (beginParameter == label.size())
					break; // reached end not found parameter (label ending in ':')

				std::string::size_type endParameter = label.find(":", beginParameter); // npos if last parameter

				if (curParamIdx == paramIdx)
				{
					std::stringstream ss;
					ss << label.substr(beginParameter, endParameter - beginParameter);
					float result;
					ss >> result;
					if (ss.fail())
						return std::pair<float, int>(0.0f, -1);
					else
						return std::pair<float, int>(result, -1);
				}

				if (endParameter == std::string::npos)
				{
					curParamIdx += 1;
					break; // reached end not found parameter 
				}
				else
				{
					beginParameter = endParameter;
					curParamIdx += 1;
				}
			}
		}

		return std::pair<float, int>(0.0f, curParamIdx); // failed or returning num. parameters
	}
}

#endif