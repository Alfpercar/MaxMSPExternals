#ifndef INCLUDED_CONCAT_INIFILE_HXX
#define INCLUDED_CONCAT_INIFILE_HXX

#include <string>
#include <map>
#include <ostream>
#include <strstream>

namespace concat
{
	// A class for reading .ini files.
	class IniFile
	{
	public:
		class Section
		{
		public:
			bool hasValue(const char *key) const;

			bool getValueAsBool(const char *key, bool valueNotExistent) const;
			const char *getValueAsString(const char *key, const char *valueNotExistent) const;
			int getValueAsInt(const char *key, int valueNotExistent) const;
			float getValueAsFloat(const char *key, float valueNotExistent) const;
			double getValueAsDouble(const char *key, double valueNotExistent) const;

			void addValue(const char *key, const char *value);

		private:
			std::map<std::string, std::string> keyValueMap_;

			friend class IniFile;
		};

		IniFile();

		void load(const char *filename);
		bool isOk() const;

		bool hasSection(const char *name) const;
		const Section *getSection(const char *name) const;

		bool hasValue(const char *section, const char *key) const;

		bool getValueAsBool(const char *section, const char *key, bool valueNotExistent) const;
		const char *getValueAsString(const char *section, const char *key, const char *valueNotExistent) const;
		int getValueAsInt(const char *section, const char *key, int valueNotExistent) const;
		float getValueAsFloat(const char *section, const char *key, float valueNotExistent) const;
		double getValueAsDouble(const char *section, const char *key, double valueNotExistent) const;

		Section *addSection(const char *name);

		void getAsFormattedText(std::ostream &ss) const;

	private:
		std::map<std::string, Section> sections_;
		bool isOk_;
	};
}

// ---------------------------------------------------------------------------------------

#include <fstream>

#include "concat/Utilities/StringHelpers.hxx"
#include "concat/Utilities/StringConversions.hxx"

namespace concat
{
	inline IniFile::IniFile()
	{
		isOk_ = false;
	}

	inline void IniFile::load(const char *filename)
	{
		isOk_ = false;

		std::fstream file;
		file.open(filename, std::ifstream::in);

		if (!file)
			return;

		sections_.clear();

		Section *curSection = NULL;

		while (1)
		{
			// get line:
			std::string line;
			if (std::getline(file, line).fail())
				break;

			// strip comment:
			std::string::size_type commentStart = line.find(";");

			if (commentStart == 0)
				continue; // comment only line, skip

			if (commentStart != std::string::npos)
				line = line.substr(0, commentStart - 0);

			// skip empty lines:
			if (line.empty())
				continue; // empty line, skip

			// look for section ([name]):
			std::string lineNoWhitespace = stripWhitespace(line);
			if (lineNoWhitespace.find("[") == 0)
			{
				std::string::size_type sectionEnd = lineNoWhitespace.find("]");
				if (sectionEnd == std::string::npos)
					continue; // failed

				std::string section = lineNoWhitespace.substr(1, sectionEnd - 1);

				curSection = addSection(section.c_str());

				continue; // done with line
			}

			// else look for key (key=value):
			std::string::size_type eqPos = line.find("=");
			if (eqPos != std::string::npos)
			{
				std::string key = line.substr(0, eqPos - 0);
				std::string val = line.substr(eqPos+1, line.size() - (eqPos+1));
                
				key = stripLeadingTrailingWhitespace(key);
				val = stripLeadingTrailingWhitespace(val);

				// strip quotation marks:
				key = stripQuotationMarks(key);
				val = stripQuotationMarks(val);

				// add key-value pair:
				if (curSection != NULL)
					curSection->addValue(key.c_str(), val.c_str());

				continue; // done with line
			}
   		}

		file.close();

		isOk_ = true;
	}

	inline bool IniFile::isOk() const
	{
		return isOk_;
	}

	inline bool IniFile::hasSection(const char *name) const
	{
		std::map<std::string, Section>::const_iterator result = sections_.find(name);
		if (result == sections_.end())
			return false;
		else
			return true;
	}

	inline const IniFile::Section *IniFile::getSection(const char *name) const
	{
		std::map<std::string, Section>::const_iterator result = sections_.find(name);
		if (result == sections_.end())
			return NULL;
		else
			return &(*result).second;
	}

	inline IniFile::Section *IniFile::addSection(const char *name)
	{
		std::pair<std::map<std::string, Section>::iterator, bool> result = 
		sections_.insert(std::map<std::string, Section>::value_type(name, Section()));

		if (!result.second)
			return NULL;
		else
			return &(*result.first).second;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	inline void IniFile::getAsFormattedText(std::ostream &ss) const
	{
		const int tabSize = 2;

		const int maxKeySizeToAlign = 70;

		std::string tab;
		for (int i = 0; i < tabSize; ++i)
			tab += " ";

		std::string assign = " = ";
		const int assignSize = (int)assign.size();

		ss.clear();

		std::map<std::string, Section>::const_iterator sectIt;
		for (sectIt = sections_.begin(); sectIt != sections_.end(); ++sectIt)
		{
			ss << "[" << (*sectIt).first << "]" << std::endl;

			int maxKeyWithTabs = 0;
			
			std::map<std::string, std::string>::const_iterator keyIt;
			for (keyIt = (*sectIt).second.keyValueMap_.begin(); keyIt != (*sectIt).second.keyValueMap_.end(); ++keyIt)
			{
				const int keySize = (int)(*keyIt).first.size();
				float keyWidthTabs = (float)(keySize + assignSize)/(float)(tabSize);
				int keyWithTabsFixed = (int)(keyWidthTabs) + 1; // ceil

				if (keySize < maxKeySizeToAlign && keyWithTabsFixed > maxKeyWithTabs)
					maxKeyWithTabs = keyWithTabsFixed;
			}

			for (keyIt = (*sectIt).second.keyValueMap_.begin(); keyIt != (*sectIt).second.keyValueMap_.end(); ++keyIt)
			{
				const int keySize = (int)(*keyIt).first.size();
				ss << tab << (*keyIt).first << " = ";
				if (keySize < maxKeySizeToAlign)
				{
					for (int i = (int)(*keyIt).first.size() + 3; i < tabSize*maxKeyWithTabs; ++i)
						ss << " ";
				}

				bool valueIsNumerical = false;
				std::stringstream conv;
				conv << (*keyIt).second;
				float tmp;
				conv >> tmp;
				if (!conv.fail())
					valueIsNumerical = true;

				if (valueIsNumerical)
					ss << (*keyIt).second << std::endl;
				else
					ss << "\"" << (*keyIt).second << "\"" << std::endl;
			}

			ss << std::endl;
		}
	}

	// -----------------------------------------------------------------------------------

	inline bool IniFile::Section::hasValue(const char *key) const
	{
		std::map<std::string, std::string>::const_iterator i = keyValueMap_.find(key);
		return (i != keyValueMap_.end());
	}

	inline bool IniFile::Section::getValueAsBool(const char *key, bool valueNotExistent) const
	{
		std::map<std::string, std::string>::const_iterator i = keyValueMap_.find(key);
		if (i != keyValueMap_.end())
		{
			if ((*i).second == "1" || 
				(*i).second == "true" || 
				(*i).second == "TRUE" || 
				(*i).second == "True" ||
				(*i).second == "yes" || 
				(*i).second == "YES" ||
				(*i).second == "Yes")
				return true;
			else
				return false;
		}
		else
		{
			return valueNotExistent;
		}
	}

	inline const char *IniFile::Section::getValueAsString(const char *key, const char *valueNotExistent) const
	{
		std::map<std::string, std::string>::const_iterator i = keyValueMap_.find(key);
		if (i != keyValueMap_.end())
			return (*i).second.c_str();
		else
			return valueNotExistent;
	}

	inline int IniFile::Section::getValueAsInt(const char *key, int valueNotExistent) const
	{
		std::map<std::string, std::string>::const_iterator i = keyValueMap_.find(key);
		if (i != keyValueMap_.end())
			return intFromString((*i).second);
		else
			return valueNotExistent;
	}

	inline float IniFile::Section::getValueAsFloat(const char *key, float valueNotExistent) const
	{
		std::map<std::string, std::string>::const_iterator i = keyValueMap_.find(key);
		if (i != keyValueMap_.end())
			return floatFromString((*i).second);
		else
			return valueNotExistent;
	}

	inline double IniFile::Section::getValueAsDouble(const char *key, double valueNotExistent) const
	{
		std::map<std::string, std::string>::const_iterator i = keyValueMap_.find(key);
		if (i != keyValueMap_.end())
			return doubleFromString((*i).second);
		else
			return valueNotExistent;
	}

	inline void IniFile::Section::addValue(const char *key, const char *value)
	{
		// if a value was already added for key, that value is used and addValue() call has no effect
		keyValueMap_.insert(std::map<std::string, std::string>::value_type(key, value));
	}

	// -----------------------------------------------------------------------------------

	inline bool IniFile::hasValue(const char *section, const char *key) const
	{
		const Section *s = getSection(section);
		if (s == NULL)
			return false;
		else
			return s->hasValue(key);
	}

	inline bool IniFile::getValueAsBool(const char *section, const char *key, bool valueNotExistent) const
	{
		const Section *s = getSection(section);
		if (s == NULL)
			return valueNotExistent;
		else
			return s->getValueAsBool(key, valueNotExistent);
	}
	
	inline const char *IniFile::getValueAsString(const char *section, const char *key, const char *valueNotExistent) const
	{
		const Section *s = getSection(section);
		if (s == NULL)
			return valueNotExistent;
		else
			return s->getValueAsString(key, valueNotExistent);
	}

	inline int IniFile::getValueAsInt(const char *section, const char *key, int valueNotExistent) const
	{
		const Section *s = getSection(section);
		if (s == NULL)
			return valueNotExistent;
		else
			return s->getValueAsInt(key, valueNotExistent);
	}

	inline float IniFile::getValueAsFloat(const char *section, const char *key, float valueNotExistent) const
	{
		const Section *s = getSection(section);
		if (s == NULL)
			return valueNotExistent;
		else
			return s->getValueAsFloat(key, valueNotExistent);
	}

	inline double IniFile::getValueAsDouble(const char *section, const char *key, double valueNotExistent) const
	{
		const Section *s = getSection(section);
		if (s == NULL)
			return valueNotExistent;
		else
			return s->getValueAsDouble(key, valueNotExistent);
	}
}

#endif